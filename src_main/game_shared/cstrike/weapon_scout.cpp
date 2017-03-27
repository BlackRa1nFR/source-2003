//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponScout C_WeaponScout
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponScout : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponScout, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponScout();

	virtual void Spawn();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();

	virtual float GetMaxSpeed() const;

private:
	
	CWeaponScout( const CWeaponScout & );

	void SCOUTFire( float flSpread, float flCycleTime );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponScout, DT_WeaponScout )

BEGIN_NETWORK_TABLE( CWeaponScout, DT_WeaponScout )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponScout )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_scout, CWeaponScout );
PRECACHE_WEAPON_REGISTER( weapon_scout );



CWeaponScout::CWeaponScout()
{
}


void CWeaponScout::Spawn()
{
	CCSBaseGunInfo info;

	info.m_flAccuracyDivisor = -1;
	info.m_flAccuracyOffset = 0;
	info.m_flMaxInaccuracy = 0;

	info.m_iPenetration = 3;
	info.m_iDamage = 75;
	info.m_flRangeModifier = 0.98;

	info.m_flTimeToIdleAfterFire = 1.8;
	info.m_flIdleInterval = 60;

	CSBaseGunSpawn( &info );
}


void CWeaponScout::SecondaryAttack()
{
	#ifndef CLIENT_DLL
		CCSPlayer *pPlayer = GetPlayerOwner();

		if (pPlayer->GetFOV() == 90)
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
	#endif

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
	WeaponSound( SPECIAL3 ); // zoom sound
}

void CWeaponScout::PrimaryAttack( void )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		SCOUTFire( 0.2, 1.25 );
	else if (pPlayer->GetAbsVelocity().Length2D() > 170)
		SCOUTFire( 0.075, 1.25 );
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		SCOUTFire( 0.0, 1.25 );
	else
		SCOUTFire( 0.007, 1.25 );
}

void CWeaponScout::SCOUTFire( float flSpread, float flCycleTime )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetFOV() != 90)
	{	
		pPlayer->m_bResumeZoom = true;
		pPlayer->m_iLastZoom = pPlayer->GetFOV();
		
		#ifndef CLIENT_DLL
			pPlayer->SetFOV( 90 );
		#endif
	}
	else
	{
		flSpread += 0.025;
	}

	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	// Delay the shell ejection call because this is a sniper rifle....
	pPlayer->m_flEjectBrass = gpGlobals->curtime + 0.56;

	QAngle angle = pPlayer->GetPunchAngle();
	angle.x -= 2;
	pPlayer->SetPunchAngle( angle );
}


float CWeaponScout::GetMaxSpeed() const
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( pPlayer->GetFOV() == 90 )
		return BaseClass::GetMaxSpeed();
	else
		return 220;	// zoomed in.
}
