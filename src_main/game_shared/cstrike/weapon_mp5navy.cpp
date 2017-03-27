//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponMP5Navy C_WeaponMP5Navy
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponMP5Navy : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponMP5Navy, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponMP5Navy();

	virtual void Spawn();
	virtual void PrimaryAttack();
	virtual bool Deploy();


private:

	CWeaponMP5Navy( const CWeaponMP5Navy & );

	void MP5NFire( float flSpread, float flCycleTime );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMP5Navy, DT_WeaponMP5Navy )

BEGIN_NETWORK_TABLE( CWeaponMP5Navy, DT_WeaponMP5Navy )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponMP5Navy )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_mp5navy, CWeaponMP5Navy );
PRECACHE_WEAPON_REGISTER( weapon_mp5navy );



CWeaponMP5Navy::CWeaponMP5Navy()
{
}


void CWeaponMP5Navy::Spawn( )
{
	m_flAccuracy = 0.0;
	m_bDelayFire = false;

	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = 220.1;
	info.m_flAccuracyOffset = 0.45;
	info.m_flMaxInaccuracy = 0.75;

	info.m_iPenetration = 1;
	info.m_iDamage = 26;
	info.m_flRangeModifier = 0.84;

	info.m_flTimeToIdleAfterFire = 2;
	info.m_flIdleInterval = 20;

	CSBaseGunSpawn( &info );
}


bool CWeaponMP5Navy::Deploy( )
{
	m_flAccuracy = 0.0;
	m_bDelayFire = false;

	return BaseClass::Deploy();
}

void CWeaponMP5Navy::PrimaryAttack( void )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		MP5NFire( (0.2) * (m_flAccuracy), 0.075 );
	else
		MP5NFire( (0.04) * (m_flAccuracy), 0.075 );
}

void CWeaponMP5Navy::MP5NFire( float flSpread, float flCycleTime )
{
	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	CCSPlayer *pPlayer = GetPlayerOwner();

	// Kick the gun based on the state of the player.
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		KickBack (0.9, 0.475, 0.35, 0.0425, 5, 3, 6);	
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)
		KickBack (0.5, 0.275, 0.2, 0.03, 3, 2, 10);
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		KickBack (0.225, 0.15, 0.1, 0.015, 2, 1, 10);
	else
		KickBack (0.25, 0.175, 0.125, 0.02, 2.25, 1.25, 10);
}


