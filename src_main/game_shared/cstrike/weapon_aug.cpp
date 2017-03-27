//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponAug C_WeaponAug
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponAug : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponAug, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponAug();

	virtual void Spawn();
	virtual void SecondaryAttack();
	virtual void PrimaryAttack();
	virtual bool Deploy();


private:

	void AUGFire( float flSpread, float flCycleTime );
	
	CWeaponAug( const CWeaponAug & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAug, DT_WeaponAug )

BEGIN_NETWORK_TABLE( CWeaponAug, DT_WeaponAug )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAug )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_aug, CWeaponAug );
PRECACHE_WEAPON_REGISTER( weapon_aug );



CWeaponAug::CWeaponAug()
{
}


void CWeaponAug::Spawn()
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;

	
	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = 215;
	info.m_flAccuracyOffset = 0.3;
	info.m_flMaxInaccuracy = 1;

	info.m_iPenetration = 2;
	info.m_iDamage = 32;
	info.m_flRangeModifier = 0.96;

	info.m_flTimeToIdleAfterFire = 1.9;
	info.m_flIdleInterval = 20;

	CSBaseGunSpawn( &info );
}


bool CWeaponAug::Deploy()
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;

	return BaseClass::Deploy();
}


void CWeaponAug::SecondaryAttack()
{
	#ifndef CLIENT_DLL
		CCSPlayer *pPlayer = GetPlayerOwner();

		if ( pPlayer->GetFOV() == 90)
		{
			pPlayer->SetFOV( 55 );
		}
		else if ( pPlayer->GetFOV() == 55 )
		{
			pPlayer->SetFOV( 90 );
		}
		else 
		{
			pPlayer->SetFOV( 90 );
		}
	#endif

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}


void CWeaponAug::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		AUGFire( 0.035 + (0.4) * (m_flAccuracy), 0.0825 );
	
	else if ( pPlayer->GetAbsVelocity().Length2D() > 140 )
		AUGFire( 0.035 + (0.07) * (m_flAccuracy), 0.0825 );
	
	else if ( pPlayer->GetFOV() == 90 )
		AUGFire( (0.02) * (m_flAccuracy), 0.0825 );
	
	else
		AUGFire( (0.02) * (m_flAccuracy), 0.135 );
}


void CWeaponAug::AUGFire( float flSpread, float flCycleTime )
{
	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( pPlayer->GetAbsVelocity().Length2D() > 0 )
		KickBack ( 1, 0.45, 0.275, 0.05, 4, 2.5, 7 );
	
	else if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		KickBack ( 1.25, 0.45, 0.22, 0.18, 5.5, 4, 5 );
	
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		KickBack ( 0.575, 0.325, 0.2, 0.011, 3.25, 2, 8 );
	
	else
		KickBack ( 0.625, 0.375, 0.25, 0.0125, 3.5, 2.25, 8 );
}


