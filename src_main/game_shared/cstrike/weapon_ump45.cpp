//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponUMP45 C_WeaponUMP45
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponUMP45 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponUMP45, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponUMP45();

	virtual void Spawn();
	virtual void PrimaryAttack();
	virtual bool Deploy();


private:

	CWeaponUMP45( const CWeaponUMP45 & );

	void UMP45Fire( float flSpread, float flCycleTime );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponUMP45, DT_WeaponUMP45 )

BEGIN_NETWORK_TABLE( CWeaponUMP45, DT_WeaponUMP45 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponUMP45 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_ump45, CWeaponUMP45 );
PRECACHE_WEAPON_REGISTER( weapon_ump45 );



CWeaponUMP45::CWeaponUMP45()
{
}


void CWeaponUMP45::Spawn()
{
	m_flAccuracy = 0.0;
	m_bDelayFire = false;

	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = 210;
	info.m_flAccuracyOffset = 0.5;
	info.m_flMaxInaccuracy = 1;

	info.m_iPenetration = 1;
	info.m_iDamage = 30;
	info.m_flRangeModifier = 0.82;

	info.m_flTimeToIdleAfterFire = 2;
	info.m_flIdleInterval = 20;

	CSBaseGunSpawn( &info );
}


bool CWeaponUMP45::Deploy()
{
	m_flAccuracy = 0.0;
	m_bDelayFire = false;

	return BaseClass::Deploy();
}

void CWeaponUMP45::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		UMP45Fire( (0.24) * (m_flAccuracy), 0.1 );
	else
		UMP45Fire( (0.04) * (m_flAccuracy), 0.1 );
}

void CWeaponUMP45::UMP45Fire( float flSpread, float flCycleTime )
{
	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	CCSPlayer *pPlayer = GetPlayerOwner();

	// Kick the gun based on the state of the player.
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		KickBack (0.125, 0.65, 0.55, 0.0475, 5.5, 4, 10);
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)
		KickBack (0.55, 0.3, 0.225, 0.03, 3.5, 2.5, 10);
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		KickBack (0.25, 0.175, 0.125, 0.02, 2.25, 1.25, 10);
	else
		KickBack (0.275, 0.2, 0.15, 0.0225, 2.5, 1.5, 10);
}

