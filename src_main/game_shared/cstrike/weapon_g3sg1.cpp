//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponG3SG1 C_WeaponG3SG1
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponG3SG1 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponG3SG1, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponG3SG1();

	virtual void Spawn();
	virtual void SecondaryAttack();
	virtual void PrimaryAttack();
	virtual bool Reload();
	virtual bool Deploy();

	virtual float GetMaxSpeed();


private:

	CWeaponG3SG1( const CWeaponG3SG1 & );

	void G3SG1Fire( float flSpread, float flCycleTime, bool fUseAutoAim );


	float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponG3SG1, DT_WeaponG3SG1 )

BEGIN_NETWORK_TABLE( CWeaponG3SG1, DT_WeaponG3SG1 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponG3SG1 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_g3sg1, CWeaponG3SG1 );
PRECACHE_WEAPON_REGISTER( weapon_g3sg1 );



CWeaponG3SG1::CWeaponG3SG1()
{
	m_flLastFire = 0;
}


void CWeaponG3SG1::Spawn()
{
	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = -1;
	info.m_flAccuracyOffset = 0;
	info.m_flMaxInaccuracy = 0;

	info.m_iPenetration = 3;
	info.m_iDamage = 60;
	info.m_flRangeModifier = 0.98;

	info.m_flTimeToIdleAfterFire = 1.8;
	info.m_flIdleInterval = 60;

	CSBaseGunSpawn( &info );
}


bool CWeaponG3SG1::Deploy( )
{
	m_flAccuracy = 0.2;
	return BaseClass::Deploy();
}


void CWeaponG3SG1::SecondaryAttack()
{
	#ifndef CLIENT_DLL
		CCSPlayer *pPlayer = GetPlayerOwner();

		if ( pPlayer->GetFOV() == 90 )
		{
			pPlayer->SetFOV( 40 );
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
		WeaponSound( SPECIAL3 ); // zoom sound
	#endif

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}

void CWeaponG3SG1::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();
	
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		G3SG1Fire( (0.45) * (1 - m_flAccuracy), 0.25, false );
	
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)
		G3SG1Fire( 0.15, 0.25, false );
	
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		G3SG1Fire( (0.035) * (1 - m_flAccuracy), 0.25, false );
	
	else
		G3SG1Fire( (0.055) * (1 - m_flAccuracy), 0.25, false );
}

void CWeaponG3SG1::G3SG1Fire( float flSpread, float flCycleTime, bool fUseAutoAim )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( pPlayer->GetFOV() == 90 )
		flSpread += 0.025;

	// Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
	if (m_flLastFire == 0)
	{
		m_flLastFire = gpGlobals->curtime;
	}
	else 
	{
		m_flAccuracy = 0.55 + (0.3) * (gpGlobals->curtime - m_flLastFire);	

		if (m_flAccuracy > 0.98)
			m_flAccuracy = 0.98;

		m_flLastFire = gpGlobals->curtime;
	}

	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	// Adjust the punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	angle.x -= SHARED_RANDOMFLOAT( 2.75, 3.25) + ( angle.x / 4 );
	angle.y += SHARED_RANDOMFLOAT( -1.25, 1.5);
	pPlayer->SetPunchAngle( angle );
}


bool CWeaponG3SG1::Reload()
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

float CWeaponG3SG1::GetMaxSpeed()
{
	if ( GetPlayerOwner()->GetFOV() == 90 )
		return BaseClass::GetMaxSpeed();
	else
		return 150; // zoomed in
}
