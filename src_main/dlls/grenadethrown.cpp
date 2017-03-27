/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== grenade_base.cpp ========================================================

  Base Handling for all the player's grenades

*/
#include "cbase.h"
#include "grenadethrown.h"
#include "ammodef.h"
#include "vstdlib/random.h"

// Precaches a grenade and ensures clients know of it's "ammo"
void UTIL_PrecacheOtherGrenade( const char *szClassname )
{
	CBaseEntity *pEntity = CreateEntityByName( szClassname );
	if ( !pEntity )
	{
		Msg( "NULL Ent in UTIL_PrecacheOtherGrenade\n" );
		return;
	}
	
	CThrownGrenade *pGrenade = dynamic_cast<CThrownGrenade *>( pEntity );

	if (pGrenade)
	{
		pGrenade->Precache( );
	}

	UTIL_Remove( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Setup basic values for Thrown grens
//-----------------------------------------------------------------------------
void CThrownGrenade::Spawn( void )
{
	// point sized, solid, bouncing
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX );
	UTIL_SetSize(this, vec3_origin, vec3_origin);

	// Movement
	SetGravity(0.81);
	SetFriction(0.6);
	QAngle angles;
	VectorAngles( GetAbsVelocity(), angles );
	SetLocalAngles( angles );
	QAngle vecAngVel( random->RandomFloat ( -100, -500 ), 0, 0 );
	SetLocalAngularVelocity( vecAngVel );
	
	SetTouch( &CThrownGrenade::BounceTouch );
}

//-----------------------------------------------------------------------------
// Purpose: Throw the grenade.
// Input  : vecOrigin - Starting position
//			vecVelocity - Starting velocity
//			flExplodeTime - Time at which to detonate
//-----------------------------------------------------------------------------
void CThrownGrenade::Thrown( Vector vecOrigin, Vector vecVelocity, float flExplodeTime )
{
	// Throw
	SetLocalOrigin( vecOrigin );
	SetAbsVelocity( vecVelocity );
	Relink();

	// Explode in 3 seconds
	SetThink( &CThrownGrenade::Detonate );
	SetNextThink( gpGlobals->curtime + flExplodeTime );
}

