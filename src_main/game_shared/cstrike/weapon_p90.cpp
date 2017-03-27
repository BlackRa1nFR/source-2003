//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponP90 C_WeaponP90
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponP90 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponP90, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponP90();

	virtual void Spawn();
	virtual void PrimaryAttack();
	virtual bool Deploy();
	virtual bool Reload();


private:

	CWeaponP90( const CWeaponP90 & );

	void P90Fire( float flSpread, float flCycleTime );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponP90, DT_WeaponP90 )

BEGIN_NETWORK_TABLE( CWeaponP90, DT_WeaponP90 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponP90 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_p90, CWeaponP90 );
PRECACHE_WEAPON_REGISTER( weapon_p90 );



CWeaponP90::CWeaponP90()
{
}


void CWeaponP90::Spawn( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_bDelayFire = false;

	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = -1;
	info.m_flAccuracyOffset = 0;
	info.m_flMaxInaccuracy = 0;

	info.m_iPenetration = 1;
	info.m_iDamage = 26;
	info.m_flRangeModifier = 0.84;

	info.m_flTimeToIdleAfterFire = 2;
	info.m_flIdleInterval = 20;

	CSBaseGunSpawn( &info );
}


bool CWeaponP90::Deploy()
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_bDelayFire = false;

	return BaseClass::Deploy();
}


void CWeaponP90::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();
	
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		P90Fire( (0.3) * (m_flAccuracy), 0.066 );
	else if (pPlayer->GetAbsVelocity().Length2D() > 170)
		P90Fire( (0.115) * (m_flAccuracy), 0.066 );
	else
		P90Fire( (0.045) * (m_flAccuracy), 0.066 );
}


void CWeaponP90::P90Fire( float flSpread, float flCycleTime )
{
	m_flAccuracy = (m_iShotsFired * m_iShotsFired/ 175) + 0.45;
	if (m_flAccuracy > 1)
		m_flAccuracy = 1;

	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	CCSPlayer *pPlayer = GetPlayerOwner();

	// Kick the gun based on the state of the player.
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		KickBack (0.9, 0.45, 0.35, 0.04, 5.25, 3.5, 4);
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)
		KickBack (0.45, 0.3, 0.2, 0.0275, 4, 2.25, 7);
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		KickBack (0.275, 0.2, 0.125, 0.02, 3, 1, 9);
	else
		KickBack (0.3, 0.225, 0.125, 0.02, 3.25, 1.25, 8);
}


bool CWeaponP90::Reload()
{
	if ( BaseClass::Reload() )
	{
		m_flAccuracy = 0.2;
		return true;
	}
	else
	{
		return false;
	}
}

