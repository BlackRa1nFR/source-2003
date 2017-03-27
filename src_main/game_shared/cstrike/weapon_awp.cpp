//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponAWP C_WeaponAWP
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponAWP : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponAWP, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponAWP();

	virtual void Spawn();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();

	virtual void AWPFire( float flSpread , float flCycleTime );

	virtual float GetMaxSpeed() const;

private:
	
	CWeaponAWP( const CWeaponAWP & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAWP, DT_WeaponAWP )

BEGIN_NETWORK_TABLE( CWeaponAWP, DT_WeaponAWP )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAWP )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_awp, CWeaponAWP );
PRECACHE_WEAPON_REGISTER( weapon_awp );



CWeaponAWP::CWeaponAWP()
{
}


void CWeaponAWP::Spawn()
{
	CCSBaseGunInfo info;

	// (Doesn't matter.. AWP doesn't use m_flAccuracy).
	info.m_flAccuracyDivisor = -1;
	info.m_flAccuracyOffset = 0;
	info.m_flMaxInaccuracy = 0;

	info.m_iPenetration = 3;
	info.m_iDamage = 115;
	info.m_flRangeModifier = 0.99;

	info.m_flTimeToIdleAfterFire = 2;
	info.m_flIdleInterval = 60;

	CSBaseGunSpawn( &info );
}


void CWeaponAWP::SecondaryAttack()
{
	#ifndef CLIENT_DLL
		CCSPlayer *pPlayer = GetPlayerOwner();

		if ( pPlayer->GetFOV() == 90)
		{
			pPlayer->SetFOV( 40 );
		}
		else if ( pPlayer->GetFOV() == 40 )
		{
			pPlayer->SetFOV( 10 );
		}
		else
		{
			pPlayer->SetFOV( 90 );
		}

		pPlayer->ResetMaxSpeed();
		
		WeaponSound( SPECIAL3 ); // zoom sound
	#endif

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}


void CWeaponAWP::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		AWPFire( 0.85, 1.45 );
	
	else if ( pPlayer->GetAbsVelocity().Length2D() > 140 )
		AWPFire( 0.25, 1.45 );
	
	else if ( pPlayer->GetAbsVelocity().Length2D() > 10 )
		AWPFire( 0.10, 1.45 );
	
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		AWPFire( 0.0, 1.45 );
	
	else
		AWPFire( 0.001, 1.45 );
}


void CWeaponAWP::AWPFire( float flSpread, float flCycleTime )
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
		flSpread += 0.08;
	}

	if ( !CSBaseGunFire( flSpread, flCycleTime ) )
		return;

	pPlayer->m_flEjectBrass = gpGlobals->curtime + 0.55;

	QAngle angle = pPlayer->GetPunchAngle();
	angle.x -= 2;
	pPlayer->SetPunchAngle( angle );
}


float CWeaponAWP::GetMaxSpeed() const
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( pPlayer->GetFOV() == 90 )
	{
		return BaseClass::GetMaxSpeed();
	}
	else
	{
		// Slower speed when zoomed in.
		return 150;
	}
}

