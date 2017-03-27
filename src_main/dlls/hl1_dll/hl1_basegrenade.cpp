//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"
#include "decals.h"
#include "basecombatcharacter.h"
#include "shake.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "EntityList.h"
#include "hl1_basegrenade.h"


extern short	g_sModelIndexFireball;		// (in combatweapon.cpp) holds the index for the fireball 
extern short	g_sModelIndexWExplosion;	// (in combatweapon.cpp) holds the index for the underwater explosion

unsigned int CHL1BaseGrenade::PhysicsSolidMaskForEntity( void ) const
{
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX;
}


void CHL1BaseGrenade::Explode( trace_t *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer

	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetLocalOrigin( pTrace->endpos + (pTrace->plane.normal * 0.6) );
	}

	UTIL_Relink( this );

	Vector vecAbsOrigin = GetAbsOrigin();
	int contents = UTIL_PointContents ( vecAbsOrigin );

	if ( pTrace->fraction != 1.0 )
	{
		Vector vecNormal = pTrace->plane.normal;
		surfacedata_t *pdata = physprops->GetSurfaceData( pTrace->surface.surfaceProps );	
		CPASFilter filter( vecAbsOrigin );
		te->Explosion( filter, 0.0, 
			&vecAbsOrigin,
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius * .03, 
			25,
			TE_EXPLFLAG_NONE,
			m_DmgRadius,
			m_flDamage,
			&vecNormal,
			(char) pdata->gameMaterial );
	}
	else
	{
		CPASFilter filter( vecAbsOrigin );
		te->Explosion( filter, 0.0,
			&vecAbsOrigin, 
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius * .03, 
			25,
			TE_EXPLFLAG_NONE,
			m_DmgRadius,
			m_flDamage );
	}

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	// Use the owner's position as the reported position
	Vector vecReported = GetOwner() ? GetOwner()->GetAbsOrigin() : vec3_origin;
	
	CTakeDamageInfo info( this, GetOwner(), GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported );

	RadiusDamage( info, GetAbsOrigin(), m_DmgRadius, CLASS_NONE );

	UTIL_DecalTrace( pTrace, "Scorch" );

	flRndSound = random->RandomFloat( 0 , 1 );

	EmitSound( "BaseGrenade.Explode" );

	SetTouch( NULL );
	
	m_fEffects		|= EF_NODRAW;
	SetAbsVelocity( vec3_origin );

	SetThink( Smoke );
	SetNextThink( gpGlobals->curtime + 0.3);

	if ( GetWaterLevel() == 0 )
	{
		int sparkCount = random->RandomInt( 0,3 );
		QAngle angles;
		VectorAngles( pTrace->plane.normal, angles );

		for ( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", GetAbsOrigin(), angles, NULL );
	}
}
