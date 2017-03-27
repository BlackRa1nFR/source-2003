//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hl1_grenade_mp5.h"
#include "hl1_weapon_mp5.h"
#include "soundent.h"
#include "decals.h"
#include "shake.h"
#include "smoke_trail.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "world.h"

extern short	g_sModelIndexFireball;
extern short	g_sModelIndexWExplosion;

extern ConVar sk_plr_dmg_mp5_grenade;
extern ConVar sk_max_mp5_grenade;
extern ConVar sk_mp5_grenade_radius;

BEGIN_DATADESC( CGrenadeMP5 )
	// SR-BUGBUG: These are borked!!!!
//	float				 m_fSpawnTime;

	// Function pointers
	DEFINE_FUNCTION( CGrenadeMP5, GrenadeMP5Touch ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_mp5, CGrenadeMP5 );

void CGrenadeMP5::Spawn( void )
{
	Precache( );
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY );
	AddFlag( FL_GRENADE );

	SetModel( "models/grenade.mdl" );
	UTIL_SetSize(this, Vector(-3, -3, -3), Vector(3, 3, 3));
//	UTIL_SetSize(this, Vector(0, 0, 0), Vector(0, 0, 0));

	SetUse( DetonateUse );
	SetTouch( GrenadeMP5Touch );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_plr_dmg_mp5_grenade.GetFloat();
	m_DmgRadius		= sk_mp5_grenade_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_bIsLive		= true;
	m_iHealth		= 1;

	SetGravity( 0.5 );
	SetFriction( 0.8 );

	SetSequence( 1 );

	m_fSpawnTime	= gpGlobals->curtime;
}


void CGrenadeMP5::Event_Killed( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType )
{
	Detonate( );
}


void CGrenadeMP5::GrenadeMP5Touch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() )
		return;

	// If I'm live go ahead and blow up
	if (m_bIsLive)
	{
		Detonate();
	}
	else
	{
		// If I'm not live, only blow up if I'm hitting an chacter that
		// is not the owner of the weapon
		CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( pOther );
		if (pBCC && GetOwner() != pBCC)
		{
			m_bIsLive = true;
			Detonate();
		}
	}
}

void CGrenadeMP5::Detonate(void)
{
	if (!m_bIsLive)
	{
		return;
	}
	m_bIsLive		= false;
	m_takedamage	= DAMAGE_NO;	

	CPASFilter filter( GetAbsOrigin() );

	te->Explosion( filter, 0.0,
		&GetAbsOrigin(), 
		GetWaterLevel() == 0 ? g_sModelIndexFireball : g_sModelIndexWExplosion,
		(m_flDamage - 50) * .60, 
		15,
		TE_EXPLFLAG_NONE,
		m_DmgRadius,
		m_flDamage );

	Vector vecForward = GetAbsVelocity();
	VectorNormalize(vecForward);
	trace_t		tr;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + 60*vecForward, MASK_SOLID, 
		this, COLLISION_GROUP_NONE, &tr );

	if ( (tr.m_pEnt != GetWorldEntity()) || (tr.hitbox != 0) )
	{
		// non-world needs smaller decals
		UTIL_DecalTrace( &tr, "SmallScorch");
	}
	else
	{
		UTIL_DecalTrace( &tr, "Sorch" );
	}

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	RadiusDamage ( CTakeDamageInfo( this, GetOwner(), m_flDamage, DMG_BLAST ), GetAbsOrigin(), m_flDamage * 2.5, CLASS_NONE );

	CPASAttenuationFilter filter2( this );

	switch ( random->RandomInt( 0, 2 ) )
	{
		case 0: enginesound->EmitSound( filter2, entindex(), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM );	break;
		case 1: enginesound->EmitSound( filter2, entindex(), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM );	break;
		case 2: enginesound->EmitSound( filter2, entindex(), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM );	break;
	}

	if ( GetWaterLevel() == 0 )
	{
		int sparkCount = random->RandomInt( 0,3 );
		QAngle angles;
		VectorAngles( tr.plane.normal, angles );

		for ( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", GetAbsOrigin(), angles, NULL );
	}

	UTIL_Remove( this );
}

void CGrenadeMP5::Precache( void )
{
	engine->PrecacheModel( "models/grenade.mdl" ); 
}
