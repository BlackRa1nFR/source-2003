//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponSG552 C_WeaponSG552
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponSG552 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponSG552, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponSG552();

	virtual void Spawn();
	virtual void SecondaryAttack();
	virtual void PrimaryAttack();
	virtual bool Deploy();
	virtual bool Reload();

	virtual float GetMaxSpeed() const;


private:

	CWeaponSG552( const CWeaponSG552 & );

	void SG552Fire( float flSpread, float flCycleTime );

};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSG552, DT_WeaponSG552 )

BEGIN_NETWORK_TABLE( CWeaponSG552, DT_WeaponSG552 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSG552 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_sg552, CWeaponSG552 );
PRECACHE_WEAPON_REGISTER( weapon_sg552 );



CWeaponSG552::CWeaponSG552()
{
}


void CWeaponSG552::Spawn( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;

	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = 220;
	info.m_flAccuracyOffset = 0.3;
	info.m_flMaxInaccuracy = 1;

	info.m_iPenetration = 2;
	info.m_iDamage = 3;
	info.m_flRangeModifier = 0.955;

	info.m_flTimeToIdleAfterFire = 2;
	info.m_flIdleInterval = 20;

	CSBaseGunSpawn( &info );
}


bool CWeaponSG552::Deploy( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;

	return BaseClass::Deploy();
}


void CWeaponSG552::SecondaryAttack()
{
	#ifndef CLIENT_DLL
		CCSPlayer *pPlayer = GetPlayerOwner();

		if (pPlayer->GetFOV() == 90)
		{
			pPlayer->SetFOV( 55 );
		}
		else if (pPlayer->GetFOV() == 55)
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

void CWeaponSG552::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		SG552Fire( 0.035 + (0.45) * (m_flAccuracy), 0.0825 );
	else if (pPlayer->GetAbsVelocity().Length2D() > 140)
		SG552Fire( 0.035 + (0.075) * (m_flAccuracy), 0.0825 );
	else if (pPlayer->GetFOV() == 90)
		SG552Fire( (0.02) * (m_flAccuracy), 0.0825 );
	else
		SG552Fire( (0.02) * (m_flAccuracy), 0.135 );
}


void CWeaponSG552::SG552Fire( float flSpread, float flCycleTime )
{
	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	CCSPlayer *pPlayer = GetPlayerOwner();
	if (pPlayer->GetAbsVelocity().Length2D() > 0)
		KickBack (1, 0.45, 0.28, 0.04, 4.25, 2.5, 7);
	else if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		KickBack (1.25, 0.45, 0.22, 0.18, 6, 4, 5);
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		KickBack (0.6, 0.35, 0.2, 0.0125, 3.7, 2, 10);
	else
		KickBack (0.625, 0.375, 0.25, 0.0125, 4, 2.25, 9);
}


bool CWeaponSG552::Reload()
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


float CWeaponSG552::GetMaxSpeed() const
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( pPlayer->GetFOV() == 90 )
		return BaseClass::GetMaxSpeed();
	else
		return 200; // zoomed in.
}	
