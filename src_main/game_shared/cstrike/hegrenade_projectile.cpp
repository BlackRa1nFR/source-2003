//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hegrenade_projectile.h"


#define GRENADE_MODEL "models/Weapons/w_grenade.mdl"


LINK_ENTITY_TO_CLASS( hegrenade_projectile, CHEGrenadeProjectile );
PRECACHE_WEAPON_REGISTER( hegrenade_projectile );


CHEGrenadeProjectile* CHEGrenadeProjectile::Create( 
	const Vector &position, 
	const QAngle &angles, 
	const Vector &velocity, 
	const AngularImpulse &angVelocity, 
	CBaseCombatCharacter *pOwner, 
	float timer )
{
	CHEGrenadeProjectile *pGrenade = (CHEGrenadeProjectile*)CBaseEntity::Create( "hegrenade_projectile", position, angles, pOwner );
	
	// Set the timer for 1 second less than requested. We're going to issue a SOUND_DANGER
	// one second before detonation.
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY );
	pGrenade->SetMoveCollide( MOVECOLLIDE_FLY_BOUNCE );
	pGrenade->SetTimer( timer - 1.5 );
	pGrenade->SetAbsVelocity( velocity );
	pGrenade->SetOwner( pOwner );
	pGrenade->m_flDamage = 100;
	pGrenade->ChangeTeam( pOwner->GetTeamNumber() );

	return pGrenade;
}


void CHEGrenadeProjectile::SetTimer( float timer )
{
	SetThink( &CHEGrenadeProjectile::PreDetonate );
	SetNextThink( gpGlobals->curtime + timer );
}


void CHEGrenadeProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );

	BaseClass::Spawn();
}


void CHEGrenadeProjectile::Precache()
{
	engine->PrecacheModel( GRENADE_MODEL );
	BaseClass::Precache();
}

