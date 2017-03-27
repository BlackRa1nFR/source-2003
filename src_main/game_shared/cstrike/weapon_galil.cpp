//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponGalil C_WeaponGalil
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponGalil : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponGalil, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponGalil();

	virtual void Spawn();
	virtual void PrimaryAttack();
	virtual bool Deploy();
	virtual bool Reload();


private:

	CWeaponGalil( const CWeaponGalil & );

	void GalilFire( float flSpread, float flCycleTime );

};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponGalil, DT_WeaponGalil )

BEGIN_NETWORK_TABLE( CWeaponGalil, DT_WeaponGalil )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponGalil )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_galil, CWeaponGalil );
PRECACHE_WEAPON_REGISTER( weapon_galil );



CWeaponGalil::CWeaponGalil()
{
}


void CWeaponGalil::Spawn()
{
	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = 200;
	info.m_flAccuracyOffset = 0.35;
	info.m_flMaxInaccuracy = 1.25;

	info.m_iPenetration = 2;
	info.m_iDamage = 30;
	info.m_flRangeModifier = 0.98;

	info.m_flTimeToIdleAfterFire = 1.28;
	info.m_flIdleInterval = 20;

	CSBaseGunSpawn( &info );
}


bool CWeaponGalil::Deploy( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	return BaseClass::Deploy();
}

void CWeaponGalil::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	// don't fire underwater
	if (pPlayer->GetWaterLevel() == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		return;
	}
	
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		GalilFire( 0.04 + (0.3) * (m_flAccuracy), 0.0875 );
	else if (pPlayer->GetAbsVelocity().Length2D() > 140)
		GalilFire( 0.04 + (0.07) * (m_flAccuracy), 0.0875 );
	else
		GalilFire( (0.0375) * (m_flAccuracy), 0.0875 );
}

void CWeaponGalil::GalilFire( float flSpread, float flCycleTime )
{
	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	CCSPlayer *pPlayer = GetPlayerOwner();
	if (pPlayer->GetAbsVelocity().Length2D() > 0)
		KickBack (1.0, 0.45, 0.28, 0.045, 3.75, 3, 7);
	else if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		KickBack (1.2, 0.5, 0.23, 0.15, 5.5, 3.5, 6);
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		KickBack (0.6, 0.3, 0.2, 0.0125, 3.25, 2, 7);
	else
		KickBack (0.65, 0.35, 0.25, 0.015, 3.5, 2.25, 7);
}


bool CWeaponGalil::Reload()
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

