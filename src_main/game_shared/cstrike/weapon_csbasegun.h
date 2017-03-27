//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef WEAPON_CSBASE_GUN_H
#define WEAPON_CSBASE_GUN_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_csbase.h"


// This is the base class for pistols and rifles.
#if defined( CLIENT_DLL )

	#define CWeaponCSBaseGun C_WeaponCSBaseGun

#else
#endif


class CCSBaseGunInfo
{
public:
	// Variables that control how fast the weapon's accuracy changes as it is fired.
	float	m_flAccuracyDivisor;
	float	m_flAccuracyOffset;
	float	m_flMaxInaccuracy;

	// Parameters for FireBullets3.
	int		m_iPenetration;
	int		m_iDamage;
	float	m_flRangeModifier;

	// Delay until the next idle animation after shooting.
	float	m_flTimeToIdleAfterFire;
	
	// How long between idles.
	float	m_flIdleInterval;
};


class CWeaponCSBaseGun : public CWeaponCSBase
{
public:
	
	DECLARE_CLASS( CWeaponCSBaseGun, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponCSBaseGun();

	virtual void Spawn();
	virtual void PrimaryAttack();
	virtual bool Deploy();
	virtual bool Reload();
	virtual void WeaponIdle();

	// Derived classes must call this from inside their Spawn() function instead of 
	// just chaining the Spawn() call down.
	void CSBaseGunSpawn( const CCSBaseGunInfo *pInfo );
	
	// Derived classes call this to fire a bullet.
	bool CSBaseGunFire( float flSpread, float flCycleTime );

	// This can be overridden because some weapons change the range modifier depending on their state
	// (like if they're silenced or not).
	virtual float GetRangeModifier() const;

	// Usually plays the shot sound. Guns with silencers can play different sounds.
	virtual void DoFireEffects();


private:

	CWeaponCSBaseGun( const CWeaponCSBaseGun & );
	CCSBaseGunInfo m_GunInfo;
};


#endif // WEAPON_CSBASE_GUN_H
