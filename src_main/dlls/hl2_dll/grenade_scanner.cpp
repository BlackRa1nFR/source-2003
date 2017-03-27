//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: This is the brickbat weapon
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "grenade_scanner.h"
#include "weapon_ar2.h"
#include "soundent.h"
#include "decals.h"
#include "shake.h"
#include "smoke_trail.h"
#include "ar2_explosion.h"
#include "mathlib.h" 
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

extern short	g_sModelIndexFireball;			// (in combatweapon.cpp) holds the index for the smoke cloud

ConVar    sk_dmg_wscanner_grenade	( "sk_dmg_wscanner_grenade","0");
ConVar	  sk_wscanner_grenade_radius	( "sk_wscanner_grenade_radius","0");

BEGIN_DATADESC( CGrenadeScanner )

	DEFINE_FUNCTION( CGrenadeScanner, AimThink ),
	DEFINE_FIELD( CGrenadeScanner, m_pSmokeTrail, FIELD_CLASSPTR ),

	// Function pointers
	DEFINE_FUNCTION( CGrenadeScanner, GrenadeScannerTouch ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_scanner, CGrenadeScanner );

void CGrenadeScanner::Spawn( void )
{
	Precache( );
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY );

	SetModel( "models/Weapons/wscanner_grenade.mdl" );
	UTIL_SetSize(this, Vector(0, 0, 0), Vector(0, 0, 0));

	SetUse( DetonateUse );
	SetTouch( GrenadeScannerTouch );
	SetThink( AimThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_dmg_wscanner_grenade.GetFloat();
	m_DmgRadius		= sk_wscanner_grenade_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	SetGravity( 1.0 );
	SetFriction( 0.8 );
	SetSequence( 1 );

	// -------------
	// Smoke trail.
	// -------------
	m_pSmokeTrail = SmokeTrail::CreateSmokeTrail();
	if(m_pSmokeTrail)
	{
		m_pSmokeTrail->m_SpawnRate = 175;
		m_pSmokeTrail->m_ParticleLifetime = 1;
		m_pSmokeTrail->m_StartColor.Init(0.5, 0.5, 0.5);
		m_pSmokeTrail->m_EndColor.Init(0,0,0);
		m_pSmokeTrail->m_StartSize = m_pSmokeTrail->m_EndSize = 8;
		m_pSmokeTrail->m_SpawnRadius = 1;
		m_pSmokeTrail->m_MinSpeed = 15;
		m_pSmokeTrail->m_MaxSpeed = 25;

		m_pSmokeTrail->SetLifetime(10.0f);
		m_pSmokeTrail->FollowEntity(entindex());
	}
}


void CGrenadeScanner::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate( );
}

void CGrenadeScanner::GrenadeScannerTouch( CBaseEntity *pOther )
{
	// Don't detonate if I hit another scanner just go away
	if (pOther->Classify() == CLASS_WASTE_SCANNER)
	{
		if(m_pSmokeTrail)
		{
			UTIL_RemoveImmediate(m_pSmokeTrail);
			m_pSmokeTrail = NULL;
		}

		UTIL_Remove(this);
	}
	else
	{
		Detonate();
	}
}

void CGrenadeScanner::Detonate(void)
{
	m_takedamage	= DAMAGE_NO;	

	if(m_pSmokeTrail)
	{
		UTIL_RemoveImmediate(m_pSmokeTrail);
		m_pSmokeTrail = NULL;
	}

	// Create a particle explosion.
/* //<<TEMP>> REMOVE FOR NOW. KILLS THE FRAMERATE
	if(AR2Explosion *pExplosion = AR2Explosion::CreateAR2Explosion(GetOrigin()))
	{
		pExplosion->SetLifetime(5);
	}
*/
	CPASFilter filter( GetAbsOrigin() );

	te->Explosion( filter, 0.0,
		&GetAbsOrigin(), 
		g_sModelIndexFireball,
		0.5, 
		15,
		TE_EXPLFLAG_NONE,
		m_DmgRadius,
		m_flDamage );

	Vector vecForward = GetAbsVelocity();
	VectorNormalize(vecForward);
	trace_t		tr;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + 60*vecForward,  MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, & tr);

	UTIL_DecalTrace( &tr, "Scorch" );

	UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 400, 0.2 );

	RadiusDamage ( CTakeDamageInfo( this, GetOwner(), m_flDamage, DMG_BLAST ), GetAbsOrigin(), m_DmgRadius, CLASS_NONE );
	CPASAttenuationFilter filter2( this, "GrenadeScanner.StopSound" );
	EmitSound( filter2, entindex(), "GrenadeScanner.StopSound" );
	UTIL_Remove( this );
}

void CGrenadeScanner::AimThink( void )
{
	QAngle angles;
	VectorAngles( GetAbsVelocity(), angles );
	SetLocalAngles( angles );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CGrenadeScanner::Precache( void )
{
	engine->PrecacheModel("models/Weapons/wscanner_grenade.mdl"); 
}


CGrenadeScanner::CGrenadeScanner(void)
{
	m_pSmokeTrail  = NULL;
}