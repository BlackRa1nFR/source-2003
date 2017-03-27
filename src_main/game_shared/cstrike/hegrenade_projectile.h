//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef HEGRENADE_PROJECTILE_H
#define HEGRENADE_PROJECTILE_H
#ifdef _WIN32
#pragma once
#endif


#include "basegrenade_shared.h"


class CHEGrenadeProjectile : public CBaseGrenade
{
public:
	DECLARE_CLASS( CHEGrenadeProjectile, CBaseGrenade );


// Overrides.
public:
	
	virtual void Spawn();
	virtual void Precache();


// Grenade stuff.
public:

	static CHEGrenadeProjectile* Create( 
		const Vector &position, 
		const QAngle &angles, 
		const Vector &velocity, 
		const AngularImpulse &angVelocity, 
		CBaseCombatCharacter *pOwner, 
		float timer );

	void SetTimer( float timer );
};


#endif // HEGRENADE_PROJECTILE_H
