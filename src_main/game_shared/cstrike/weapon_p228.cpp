//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbase.h"


#if defined( CLIENT_DLL )

	#define CWeaponP228 C_WeaponP228
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponP228 : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CWeaponP228, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponP228();

	virtual void Spawn();
	virtual void Precache();

	virtual void PrimaryAttack();
	virtual bool Deploy();

	virtual bool Reload();
	virtual void WeaponIdle();

private:
	
	CWeaponP228( const CWeaponP228 & );
	void P228Fire( float flSpread, float flCycleTime, bool fUseSemi );

	float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponP228, DT_WeaponP228 )

BEGIN_NETWORK_TABLE( CWeaponP228, DT_WeaponP228 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponP228 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_p228, CWeaponP228 );
PRECACHE_WEAPON_REGISTER( weapon_p228 );



CWeaponP228::CWeaponP228()
{
}


void CWeaponP228::Spawn( )
{
	//m_iDefaultAmmo = 13;
	m_flAccuracy = 0.9;
	
	//ClearBits(m_iWeaponState, WPNSTATE_SHIELD_DRAWN);

	//FallInit();// get ready to fall down.
	BaseClass::Spawn();
}


void CWeaponP228::Precache()
{
	//m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell
	//m_usFireP228 = PRECACHE_EVENT (1, "events/p228.sc");
	BaseClass::Precache();
}

bool CWeaponP228::Deploy( )
{
	m_flAccuracy = 0.9;

	return BaseClass::Deploy();
}

void CWeaponP228::PrimaryAttack( void )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		P228Fire( (1.5) * (1 - m_flAccuracy), 0.2, false );
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)
		P228Fire( (0.255) * (1 - m_flAccuracy), 0.2, false );
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		P228Fire( (0.075) * (1 - m_flAccuracy), 0.2, false );
	else
		P228Fire( (0.15) * (1 - m_flAccuracy), 0.2, false );
}

void CWeaponP228::P228Fire( float flSpread, float flCycleTime, bool fUseSemi )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	flCycleTime -= 0.05;
	m_iShotsFired++;

	if (m_iShotsFired > 1)
		return;

	// Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
	if (m_flLastFire == 0)
		m_flLastFire = gpGlobals->curtime;
	else 
	{
		m_flAccuracy -= (0.3)*(0.325 - (gpGlobals->curtime - m_flLastFire));

		if (m_flAccuracy > 0.9)
			m_flAccuracy = 0.9;
		else if (m_flAccuracy < 0.6)
			m_flAccuracy = 0.6;

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
	m_fEffects |= EF_MUZZLEFLASH;

	//SetPlayerShieldAnim();

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
	// Aiming
	Vector vecDir = pPlayer->FireBullets3( 
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(),
		flSpread, 
		4096, 
		1, 
		GetPrimaryAmmoType(), 
		32, 
		0.8, 
		pPlayer );

	WeaponSound( SINGLE );

	/*
	// Special f/x
	pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	int flag;
	#if defined( CLIENT_WEAPONS )
		flag = FEV_NOTHOST;
	#else
		flag = 0;
	#endif

	PLAYBACK_EVENT_FULL( flag, pPlayer->edict(), m_usFireP228,
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
	vecDir.x,
	vecDir.y,
	pPlayer->pev->punchangle.x * 100, 
	pPlayer->pev->punchangle.y * 100, m_iClip1 ? 0 : 1, 0 );
	*/

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flCycleTime;

	if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	m_flTimeWeaponIdle = gpGlobals->curtime + 2;

	//ResetPlayerShieldAnim();

	QAngle angle = pPlayer->GetPunchAngle();
	angle.x -= 2;
	pPlayer->SetPunchAngle( angle );
}


bool CWeaponP228::Reload()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
		return true;

	int iResult = 0;
	iResult = DefaultReload( 13, 0, ACT_VM_RELOAD );

	if ( iResult == 0 )
		 return true;

	pPlayer->SetAnimation( PLAYER_RELOAD );

	m_flAccuracy = 0.9;
	return true;
}

void CWeaponP228::WeaponIdle()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	//ResetEmptySound( );

	pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	// only idle if the slid isn't back
	if (m_iClip1 != 0)
	{	
		m_flTimeWeaponIdle = gpGlobals->curtime + 49.0 / 16;
		SendWeaponAnim( ACT_VM_IDLE );
	}
}
