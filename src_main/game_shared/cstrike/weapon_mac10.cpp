//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponMAC10 C_WeaponMAC10
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponMAC10 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponMAC10, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponMAC10();

	virtual void Spawn();
	virtual void Precache();
	virtual void PrimaryAttack();
	virtual bool Deploy();


private:

	void MAC10Fire( float flSpread, float flCycleTime );

	CWeaponMAC10( const CWeaponMAC10 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMAC10, DT_WeaponMAC10 )

BEGIN_NETWORK_TABLE( CWeaponMAC10, DT_WeaponMAC10 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponMAC10 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_mac10, CWeaponMAC10 );
PRECACHE_WEAPON_REGISTER( weapon_mac10 );



CWeaponMAC10::CWeaponMAC10()
{
}


void CWeaponMAC10::Spawn( )
{
	//m_iDefaultAmmo = 30;
	m_flAccuracy = 0.15;
	m_bDelayFire = false;

	CCSBaseGunInfo info;
	
	info.m_flAccuracyDivisor = 200;
	info.m_flAccuracyOffset = 0.6;
	info.m_flMaxInaccuracy = 1.65;
	
	info.m_iPenetration = 1;
	info.m_iDamage = 29;
	info.m_flRangeModifier = 0.82;
	
	info.m_flTimeToIdleAfterFire = 2;
	info.m_flIdleInterval = 20;

	CSBaseGunSpawn( &info );
}


void CWeaponMAC10::Precache()
{
	BaseClass::Precache();
}


bool CWeaponMAC10::Deploy()
{
	m_flAccuracy = 0.15;
	m_bDelayFire = false;

	return BaseClass::Deploy();
}


void CWeaponMAC10::MAC10Fire( float flSpread, float flCycleTime )
{
	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	CCSPlayer *pPlayer = GetPlayerOwner();
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )	// jumping
		KickBack (1.3, 0.55, 0.4, 0.05, 4.75, 3.75, 5);
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)				// running
		KickBack (0.9, 0.45, 0.25, 0.035, 3.5, 2.75, 7);
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )	// ducking
		KickBack (0.75, 0.4, 0.175, 0.03, 2.75, 2.5, 10);
	else														// standing
		KickBack (0.775, 0.425, 0.2, 0.03, 3, 2.75, 9);
}


void CWeaponMAC10::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		MAC10Fire( (0.375) * (m_flAccuracy), 0.07 );
	else
		MAC10Fire( (0.03) * (m_flAccuracy), 0.07 );
}

