//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"

#ifdef CLIENT_DLL
	#include "c_cs_player.h"
#else
	#include "cs_player.h"
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCSBaseGun, DT_WeaponCSBaseGun )

BEGIN_NETWORK_TABLE( CWeaponCSBaseGun, DT_WeaponCSBaseGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCSBaseGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_csbase_gun, CWeaponCSBaseGun );



CWeaponCSBaseGun::CWeaponCSBaseGun()
{
}


void CWeaponCSBaseGun::Spawn()
{
	// Derived classes should call CSBaseGunSpawn() and provide all the necessary parameters.
	Assert( false );
}


bool CWeaponCSBaseGun::Deploy()
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;

	return BaseClass::Deploy();
}


void CWeaponCSBaseGun::PrimaryAttack()
{
	// Derived classes should implement this and call CSBaseGunFire.
	Assert( false );
}


void CWeaponCSBaseGun::CSBaseGunSpawn( const CCSBaseGunInfo *pInfo )
{
	m_GunInfo = *pInfo;

	BaseClass::Spawn();
}


bool CWeaponCSBaseGun::CSBaseGunFire( float flSpread, float flCycleTime )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	m_bDelayFire = true;
	m_iShotsFired++;
	
	// These modifications feed back into flSpread eventually.
	if ( m_GunInfo.m_flAccuracyDivisor != -1 )
	{
		m_flAccuracy = ((m_iShotsFired * m_iShotsFired * m_iShotsFired) / m_GunInfo.m_flAccuracyDivisor) + m_GunInfo.m_flAccuracyOffset;
		if (m_flAccuracy > m_GunInfo.m_flMaxInaccuracy)
			m_flAccuracy = m_GunInfo.m_flMaxInaccuracy;
	}

	// Out of ammo?
	if ( m_iClip1 <= 0 )
	{
		if (m_bFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
		}

		return false;
	}

	m_iClip1--;

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecDir = pPlayer->FireBullets3( 
		pPlayer->Weapon_ShootPosition(), 
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), 
		flSpread, 
		8192, 
		m_GunInfo.m_iPenetration, 
		GetPrimaryAmmoType(), 
		m_GunInfo.m_iDamage, 
		GetRangeModifier(), 
		pPlayer );

	DoFireEffects();

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flCycleTime;

	if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	m_flTimeWeaponIdle = gpGlobals->curtime + m_GunInfo.m_flTimeToIdleAfterFire;
	return true;
}


float CWeaponCSBaseGun::GetRangeModifier() const
{
	return m_GunInfo.m_flRangeModifier;
}


void CWeaponCSBaseGun::DoFireEffects()
{
	WeaponSound( SINGLE );
	m_fEffects |= EF_MUZZLEFLASH;
}


bool CWeaponCSBaseGun::Reload()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) <= 0)
		return false;

	int iResult = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( !iResult )
		return false;

	pPlayer->SetAnimation( PLAYER_RELOAD );

#ifndef CLIENT_DLL
	if ((iResult) && (pPlayer->GetFOV() != 90))
	{
		pPlayer->SetFOV( 90 );
	}
#endif

	m_flAccuracy = 0.0;
	m_iShotsFired = 0;
	m_bDelayFire = false;
	return true;
}

void CWeaponCSBaseGun::WeaponIdle()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	//ResetEmptySound( );

	pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	// only idle if the slid isn't back
	if ( m_iClip1 != 0 )
	{
		m_flTimeWeaponIdle = gpGlobals->curtime + m_GunInfo.m_flIdleInterval;
		SendWeaponAnim( ACT_VM_IDLE );
	}
}
