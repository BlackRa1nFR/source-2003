//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CAK47 C_AK47
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CAK47 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CAK47, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CAK47();

	virtual void Spawn();
	virtual void PrimaryAttack();


private:

	void AK47Fire( float flSpread, float flCycleTime );

	CAK47( const CAK47 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( AK47, DT_WeaponAK47 )

BEGIN_NETWORK_TABLE( CAK47, DT_WeaponAK47 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CAK47 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_ak47, CAK47 );
PRECACHE_WEAPON_REGISTER( weapon_ak47 );

// ---------------------------------------------------------------------------- //
// CAK47 implementation.
// ---------------------------------------------------------------------------- //

CAK47::CAK47()
{
}


void CAK47::Spawn()
{
	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = 200;
	info.m_flAccuracyOffset = 0.35;
	info.m_flMaxInaccuracy = 1.25;

	info.m_iPenetration = 2;
	info.m_iDamage = 36;
	info.m_flRangeModifier = 0.98;

	info.m_flTimeToIdleAfterFire = 1.9;
	info.m_flIdleInterval = 20;

	CSBaseGunSpawn( &info );
}


void CAK47::AK47Fire( float flSpread, float flCycleTime )
{
	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	CCSPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetAbsVelocity().Length2D() > 0 )
		KickBack ( 1.5, 0.45, 0.225, 0.05, 6.5, 2.5, 7 );
	else if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		KickBack ( 2, 1.0, 0.5, 0.35, 9, 6, 5 );
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		KickBack ( 0.9, 0.35, 0.15, 0.025, 5.5, 1.5, 9 );
	else
		KickBack ( 1, 0.375, 0.175, 0.0375, 5.75, 1.75, 8 );
}


void CAK47::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		Error( "CAK47::PrimaryAttack - !pPlayer" );

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		AK47Fire( 0.04 + (0.4) * m_flAccuracy, 0.0955 );
	else if (pPlayer->GetAbsVelocity().Length2D() > 140)
		AK47Fire( 0.04 + (0.07) * m_flAccuracy, 0.0955 );
	else
		AK47Fire( (0.0275) * m_flAccuracy, 0.0955 );
}

