//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h" 
#include "decals.h" 
#include "util.h" 
#include "cbase.h" 
#include "shake.h" 
#include "weapon_csbase.h"


#if defined( CLIENT_DLL )

	#define CDEagle C_DEagle
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif



#define DEAGLE_WEIGHT   7
#define DEAGLE_MAX_CLIP 7

enum deagle_e {
	DEAGLE_IDLE1 = 0,
	DEAGLE_SHOOT1,
	DEAGLE_SHOOT2,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,	
	DEAGLE_DRAW,
};



class CDEagle : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CDEagle, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CDEagle();

	void Spawn();

	void PrimaryAttack();
	void DEAGLEFire( float flSpread, float flCycleTime, bool fUseAutoAim );
	virtual bool Deploy();
	bool Reload();
	void WeaponIdle();
	void MakeBeam ();
	void BeamUpdate ();
	virtual bool UseDecrement() {return true;};


public:

	float m_flAccuracy;
	float m_flLastFire;


private:
	CDEagle( const CDEagle & );
};



IMPLEMENT_NETWORKCLASS_ALIASED( DEagle, DT_WeaponDEagle )

BEGIN_NETWORK_TABLE( CDEagle, DT_WeaponDEagle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CDEagle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_deagle, CDEagle );
PRECACHE_WEAPON_REGISTER( weapon_deagle );



CDEagle::CDEagle()
{
	m_flLastFire = 0;
}


void CDEagle::Spawn()
{
	BaseClass::Spawn();
	m_flAccuracy = 0.9;
}


bool CDEagle::Deploy()
{
	m_flAccuracy = 0.9;
	return BaseClass::Deploy();
}


void CDEagle::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();
	
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		DEAGLEFire( (1.5) * (1 - m_flAccuracy), 0.3, false );
	
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)
		DEAGLEFire( (0.25) * (1 - m_flAccuracy), 0.3, false );
	
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		DEAGLEFire( (0.115) * (1 - m_flAccuracy), 0.3, false );
	
	else
		DEAGLEFire( (0.13) * (1 - m_flAccuracy), 0.3, false );
}


void CDEagle::DEAGLEFire( float flSpread, float flCycleTime, bool fUseSemi )
{
	CCSPlayer *pPlayer = GetPlayerOwner();


	flCycleTime -= 0.075;
	m_iShotsFired++;

	if (m_iShotsFired > 1)
		return;

	// Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
	if ( m_flLastFire == 0 )
	{
		m_flLastFire = gpGlobals->curtime;
	}
	else 
	{
		m_flAccuracy -= (0.35)*(0.4 - ( gpGlobals->curtime - m_flLastFire ) );

		if (m_flAccuracy > 0.9)
			m_flAccuracy = 0.9;
		else if (m_flAccuracy < 0.55)
			m_flAccuracy = 0.55;

		m_flLastFire = gpGlobals->curtime;
	}


	if (m_iClip1 <= 0)
	{
		if (m_bFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
		}

		return;
	}

	m_iClip1--;

	pPlayer->m_fEffects |= EF_MUZZLEFLASH;

	//SetPlayerShieldAnim();
	
	// player "shoot" animation
	//m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	//pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	//pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecDir = pPlayer->FireBullets3( 
		vecSrc, 
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), 
		flSpread, 
		4096, 
		2, 
		GetPrimaryAmmoType(), 
		54, 
		0.81, 
		pPlayer );

	WeaponSound( SINGLE );

	/*
	#if defined( CLIENT_WEAPONS )
		int flag = FEV_NOTHOST;
	#else
		int flag = 0;
	#endif
	
	PLAYBACK_EVENT_FULL( flag, m_pPlayer->edict(), m_usFireDeagle,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
		vecDir.x,
		vecDir.y,
		m_pPlayer->pev->punchangle.x * 100,
		m_pPlayer->pev->punchangle.y * 100, 
		m_iClip1 ? 0 : 1, 0 );
	*/

	m_flNextPrimaryAttack = gpGlobals->curtime + flCycleTime;

	if ( !m_iClip1 && pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	m_flTimeWeaponIdle = gpGlobals->curtime + 1.8;

	QAngle punchAngle = pPlayer->GetPunchAngle();
	punchAngle.x -= 2;
	pPlayer->SetPunchAngle( punchAngle );

	//ResetPlayerShieldAnim();
}


bool CDEagle::Reload()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) <= 0 )
		return false;

	if ( !DefaultReload( 7, DEAGLE_RELOAD, 2.2 ) )
		return false;

	//m_pPlayer->SetAnimation( PLAYER_RELOAD );
	m_flAccuracy = 0.9;
	return true;
}

void CDEagle::WeaponIdle()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > gpGlobals->curtime )
		return;

	m_flTimeWeaponIdle = gpGlobals->curtime + 20;

	//if ( FBitSet(m_iWeaponState, WPNSTATE_SHIELD_DRAWN) )
	//	 SendWeaponAnim( SHIELDGUN_DRAWN_IDLE, UseDecrement() ? 1:0 );
}

