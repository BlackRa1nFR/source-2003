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


class CFlashbangProjectile : public CBaseGrenade
{
public:
	DECLARE_CLASS( CFlashbangProjectile, CBaseGrenade );
	DECLARE_DATADESC();


// Overrides.
public:
	
	virtual void Spawn();
	virtual void Precache();


// Grenade stuff.
public:

	static CFlashbangProjectile* Create( 
		const Vector &position, 
		const QAngle &angles, 
		const Vector &velocity, 
		const AngularImpulse &angVelocity, 
		CBaseCombatCharacter *pOwner, 
		float timer );


protected:

	void FlashbangDetonate();
};


#endif // HEGRENADE_PROJECTILE_H
