//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponM249 C_WeaponM249
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponM249 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponM249, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponM249();

	virtual void Spawn();
	virtual void PrimaryAttack();
	virtual bool Deploy();
	virtual bool Reload();

private:

	CWeaponM249( const CWeaponM249 & );

	void M249Fire( float flSpread, float flCycleTime );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponM249, DT_WeaponM249 )

BEGIN_NETWORK_TABLE( CWeaponM249, DT_WeaponM249 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponM249 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_m249, CWeaponM249 );
PRECACHE_WEAPON_REGISTER( weapon_m249 );



CWeaponM249::CWeaponM249()
{
}


void CWeaponM249::Spawn()
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;

	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = 175;
	info.m_flAccuracyOffset = 0.4;
	info.m_flMaxInaccuracy = 0.9;

	info.m_iPenetration = 2;
	info.m_iDamage = 32;
	info.m_flRangeModifier = 0.97;

	info.m_flTimeToIdleAfterFire = 1.6;
	info.m_flIdleInterval = 20;

	CSBaseGunSpawn( &info );
}


bool CWeaponM249::Deploy()
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;

	return BaseClass::Deploy();
}


void CWeaponM249::PrimaryAttack( void )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		M249Fire( 0.045 + (0.5) * (m_flAccuracy), 0.1 );
	else if (pPlayer->GetAbsVelocity().Length2D() > 140)
		M249Fire( 0.045 + (0.095) * (m_flAccuracy), 0.1 );
	else
		M249Fire( (0.03) * (m_flAccuracy), 0.1 );
}

void CWeaponM249::M249Fire( float flSpread, float flCycleTime )
{
	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;
	
	CCSPlayer *pPlayer = GetPlayerOwner();
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		KickBack (1.8, 0.65, 0.45, 0.125, 5, 3.5, 8);
	
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)
		KickBack (1.1, 0.5, 0.3, 0.06, 4, 3, 8);
	
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		KickBack (0.75, 0.325, 0.25, 0.025, 3.5, 2.5, 9);
	
	else
		KickBack (0.8, 0.35, 0.3, 0.03, 3.75, 3, 9);
}


bool CWeaponM249::Reload()
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
