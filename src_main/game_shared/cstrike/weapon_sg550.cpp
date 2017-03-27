//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponSG550 C_WeaponSG550
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponSG550 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponSG550, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponSG550();

	virtual void Spawn();
	virtual void SecondaryAttack();
	virtual void PrimaryAttack();
	virtual bool Deploy();
	virtual bool Reload();

	virtual float GetMaxSpeed() const;


private:

	CWeaponSG550( const CWeaponSG550 & );

	void SG550Fire( float flSpread, float flCycleTime );

	float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSG550, DT_WeaponSG550 )

BEGIN_NETWORK_TABLE( CWeaponSG550, DT_WeaponSG550 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSG550 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_sg550, CWeaponSG550 );
PRECACHE_WEAPON_REGISTER( weapon_sg550 );



CWeaponSG550::CWeaponSG550()
{
}


void CWeaponSG550::Spawn( )
{
	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = -1;
	info.m_flAccuracyOffset = 0;
	info.m_flMaxInaccuracy = 0;

	info.m_iPenetration = 2;
	info.m_iDamage = 40;
	info.m_flRangeModifier = 0.98;

	info.m_flTimeToIdleAfterFire = 1.8;
	info.m_flIdleInterval = 60;

	CSBaseGunSpawn( &info );
}


bool CWeaponSG550::Deploy()
{
	m_flAccuracy = 0.2;
	return BaseClass::Deploy();
}

void CWeaponSG550::SecondaryAttack()
{
	#ifndef CLIENT_DLL
		CCSPlayer *pPlayer = GetPlayerOwner();

		if (pPlayer->GetFOV() == 90)
		{
			pPlayer->SetFOV( 40 );;
		}
		else if (pPlayer->GetFOV() == 40)
		{
			pPlayer->SetFOV( 15 );
		}
		else if (pPlayer->GetFOV() == 15)
		{
			pPlayer->SetFOV( 90 );
		}

		pPlayer->ResetMaxSpeed();
	#endif

	WeaponSound( SPECIAL3 ); // zoom sound.
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}

void CWeaponSG550::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		SG550Fire( (0.45) * (1 - m_flAccuracy), 0.25 );
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)
		SG550Fire( 0.15, 0.25 );
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		SG550Fire( (0.04) * (1 - m_flAccuracy), 0.25 );
	else
		SG550Fire( (0.05) * (1 - m_flAccuracy), 0.25 );
}

void CWeaponSG550::SG550Fire( float flSpread, float flCycleTime )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetFOV() == 90)
		flSpread += 0.025;

	// Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
	if (m_flLastFire == 0)
	{
		m_flLastFire = gpGlobals->curtime;
	}
	else 
	{
		m_flAccuracy = 0.65 + (0.35) * (gpGlobals->curtime - m_flLastFire);	

		if (m_flAccuracy > 0.98)
			m_flAccuracy = 0.98;

		m_flLastFire = gpGlobals->curtime;
	}

	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;
	

	QAngle angle = pPlayer->GetPunchAngle();
	angle.x -= SHARED_RANDOMFLOAT( 1.5, 1.75 ) + ( angle.x / 4 );
	angle.y += SHARED_RANDOMFLOAT( -1, 1 );
	pPlayer->SetPunchAngle( angle );
}


bool CWeaponSG550::Reload()
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

float CWeaponSG550::GetMaxSpeed() const
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( pPlayer->GetFOV() == 90 )
		return BaseClass::GetMaxSpeed();
	else
		return 150; // zoomed in
}
