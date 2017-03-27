//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "grenade_spit.h"
#include "soundent.h"
#include "decals.h"
#include "smoke_trail.h"
#include "hl2_shareddefs.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

ConVar    sk_dmg_spit_grenade		( "sk_dmg_spit_grenade","0");
ConVar	  sk_spit_grenade_radius	( "sk_spit_grenade_radius","0");

BEGIN_DATADESC( CGrenadeSpit )

	DEFINE_FIELD( CGrenadeSpit, m_nSquidSpitSprite, FIELD_INTEGER ),
	DEFINE_FIELD( CGrenadeSpit, m_fSpitDeathTime, FIELD_FLOAT ),

	// Function pointers
	DEFINE_FUNCTION( CGrenadeSpit, SpitThink ),
	DEFINE_FUNCTION( CGrenadeSpit, GrenadeSpitTouch ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_spit, CGrenadeSpit );

void CGrenadeSpit::Spawn( void )
{
	Precache( );
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY );

	// FIXME, if these is a sprite, then we need a base class derived from CSprite rather than
	// CBaseAnimating.  pev->scale becomes m_flSpriteScale in that case.
	SetModel( "models/spitball_large.mdl" );
	UTIL_SetSize(this, Vector(0, 0, 0), Vector(0, 0, 0));

	m_nRenderMode		= kRenderTransAdd;
	SetRenderColor( 255, 255, 255, 255 );
	m_nRenderFX		= kRenderFxNone;

	SetThink( SpitThink );
	SetUse( DetonateUse );
	SetTouch( GrenadeSpitTouch );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_dmg_spit_grenade.GetFloat();
	m_DmgRadius		= sk_spit_grenade_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	SetGravity( SPIT_GRAVITY );
	SetFriction( 0.8 );
	SetSequence( 1 );

	SetCollisionGroup( HL2COLLISION_GROUP_SPIT );
}


void CGrenadeSpit::SetSpitSize(int nSize)
{
	switch (nSize)
	{
		case SPIT_LARGE:
		{
			SetModel( "models/spitball_large.mdl" );
			break;
		}
		case SPIT_MEDIUM:
		{
			SetModel( "models/spitball_medium.mdl" );
			break;
		}
		case SPIT_SMALL:
		{
			SetModel( "models/spitball_small.mdl" );
			break;
		}
	}
}

void CGrenadeSpit::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate( );
}

void CGrenadeSpit::GrenadeSpitTouch( CBaseEntity *pOther )
{
	if (m_fSpitDeathTime != 0)
	{
		return;
	}
	if ( pOther->GetCollisionGroup() == HL2COLLISION_GROUP_SPIT)
	{
		return;
	}
	if ( !pOther->m_takedamage )
	{

		// make a splat on the wall
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * 10, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		UTIL_DecalTrace(&tr, "BeerSplash" );

		// make some flecks
		// CPVSFilter filter( tr.endpos );
		//te->SpriteSpray( filter, 0.0,
		//	tr.endpos, tr.plane.normal, m_nSquidSpitSprite, 30, 0.8, 5 );
	}
	else
	{
		RadiusDamage ( CTakeDamageInfo( this, GetOwner(), m_flDamage, DMG_BLAST ), GetAbsOrigin(), m_DmgRadius, CLASS_NONE );
	}

	Detonate();
}

void CGrenadeSpit::SpitThink( void )
{
	if (m_fSpitDeathTime != 0 &&
		m_fSpitDeathTime < gpGlobals->curtime)
	{
		UTIL_Remove( this );
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CGrenadeSpit::Detonate(void)
{
	m_takedamage	= DAMAGE_NO;	

	int		iPitch;

	// splat sound
	iPitch = random->RandomFloat( 90, 110 );

	EmitSound( "GrenadeSpit.Acid" );	
	EmitSound( "GrenadeSpit.Hit" );	

	UTIL_Remove( this );
}

void CGrenadeSpit::Precache( void )
{
	m_nSquidSpitSprite = engine->PrecacheModel("sprites/greenglow1.vmt");// client side spittle.

	engine->PrecacheModel("models/spitball_large.mdl"); 
	engine->PrecacheModel("models/spitball_medium.mdl"); 
	engine->PrecacheModel("models/spitball_small.mdl"); 
}


CGrenadeSpit::CGrenadeSpit(void)
{
}