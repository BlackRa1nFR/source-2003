//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponM4A1 C_WeaponM4A1
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponM4A1 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponM4A1, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponM4A1();

	virtual void Spawn();
	virtual void SecondaryAttack();
	virtual void PrimaryAttack();
	virtual bool Deploy();


private:

	CWeaponM4A1( const CWeaponM4A1 & );

	void M4A1Fire( float flSpread, float flCycleTime );
	float GetRangeModifier() const;
	void DoFireEffects();

	bool m_bSilencerOn;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponM4A1, DT_WeaponM4A1 )

BEGIN_NETWORK_TABLE( CWeaponM4A1, DT_WeaponM4A1 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponM4A1 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_m4a1, CWeaponM4A1 );
PRECACHE_WEAPON_REGISTER( weapon_m4a1 );



CWeaponM4A1::CWeaponM4A1()
{
	m_bSilencerOn = false;
}


void CWeaponM4A1::Spawn( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_bDelayFire = true;

	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = 220;
	info.m_flAccuracyOffset = 0.3;
	info.m_flMaxInaccuracy = 1;

	info.m_iPenetration = 2;
	info.m_iDamage = 33;
	info.m_flRangeModifier = 0.99;

	info.m_flTimeToIdleAfterFire = 1.5;
	info.m_flIdleInterval = 60;

	CSBaseGunSpawn( &info );
}


bool CWeaponM4A1::Deploy()
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_bDelayFire = true;

	return BaseClass::Deploy();
}

void CWeaponM4A1::SecondaryAttack()
{
	if ( m_bSilencerOn )
	{
		m_bSilencerOn = false;
		SendWeaponAnim( ACT_VM_DETACH_SILENCER );
		WeaponSound( SPECIAL2 );
	}
	else
	{
		m_bSilencerOn = true;
		SendWeaponAnim( ACT_VM_ATTACH_SILENCER );
		WeaponSound( SPECIAL3 );
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + 2;
	m_flNextPrimaryAttack = gpGlobals->curtime + 2;
	m_flTimeWeaponIdle = gpGlobals->curtime + 2.0;
}

void CWeaponM4A1::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
	{
		M4A1Fire( 0.035 + (0.4) * (m_flAccuracy), 0.0875 );
	}
	else if (pPlayer->GetAbsVelocity().Length2D() > 140)
	{
		M4A1Fire( 0.035 + (0.07) * (m_flAccuracy), 0.0875 );
	}
	else
	{
		if ( m_bSilencerOn )
			M4A1Fire( (0.025) * (m_flAccuracy), 0.0875 );
		else
			M4A1Fire( (0.02) * (m_flAccuracy), 0.0875 );
	}
}


void CWeaponM4A1::M4A1Fire( float flSpread, float flCycleTime )
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


float CWeaponM4A1::GetRangeModifier() const
{
	if ( m_bSilencerOn )
		return 0.95;
	else
		return 0.97;
}


void CWeaponM4A1::DoFireEffects()
{
	if ( m_bSilencerOn )
	{
		WeaponSound( SPECIAL1 );
	}
	else
	{
		m_fEffects |= EF_MUZZLEFLASH;
		WeaponSound( SINGLE );
	}
}


