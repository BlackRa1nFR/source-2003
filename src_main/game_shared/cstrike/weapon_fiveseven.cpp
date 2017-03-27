//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbase.h"


#if defined( CLIENT_DLL )

	#define CWeaponFiveSeven C_WeaponFiveSeven
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponFiveSeven : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CWeaponFiveSeven, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponFiveSeven();

	virtual void Spawn();
	virtual void Precache();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual bool Deploy();

	virtual bool Reload();

	virtual void WeaponIdle();

private:
	
	CWeaponFiveSeven( const CWeaponFiveSeven & );
	
	void FiveSevenFire( float flSpread, float flCycleTime, bool fUseSemi );

	float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFiveSeven, DT_WeaponFiveSeven )

BEGIN_NETWORK_TABLE( CWeaponFiveSeven, DT_WeaponFiveSeven )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponFiveSeven )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_fiveseven, CWeaponFiveSeven );
PRECACHE_WEAPON_REGISTER( weapon_fiveseven );



CWeaponFiveSeven::CWeaponFiveSeven()
{
	m_flLastFire = 0;
}


void CWeaponFiveSeven::Spawn( )
{
	//m_iDefaultAmmo = 20;
	m_flAccuracy = 0.92;

	//FallInit();// get ready to fall down.
	BaseClass::Spawn();
}


void CWeaponFiveSeven::Precache()
{
	//m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell
	//m_usFireFiveSeven = PRECACHE_EVENT (1, "events/fiveseven.sc");
	BaseClass::Precache();
}

bool CWeaponFiveSeven::Deploy()
{
	m_flAccuracy = 0.92;
	return BaseClass::Deploy();
}

void CWeaponFiveSeven::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		FiveSevenFire( (1.5) * (1 - m_flAccuracy), 0.2, false );
	else if (pPlayer->GetAbsVelocity().Length2D() > 0)
		FiveSevenFire( (0.255) * (1 - m_flAccuracy), 0.2, false );
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		FiveSevenFire( (0.075) * (1 - m_flAccuracy), 0.2, false );
	else
		FiveSevenFire( (0.15) * (1 - m_flAccuracy), 0.2, false );
}

void CWeaponFiveSeven::SecondaryAttack() 
{
}

void CWeaponFiveSeven::FiveSevenFire( float flSpread, float flCycleTime, bool fUseSemi )
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
		m_flAccuracy -= (0.25)*(0.275 - (gpGlobals->curtime - m_flLastFire));

		if (m_flAccuracy > 0.92)
			m_flAccuracy = 0.92;
		else if (m_flAccuracy < 0.725)
			m_flAccuracy = 0.725;

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
		20, 
		0.885, 
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

	PLAYBACK_EVENT_FULL( flag, pPlayer->edict(), m_usFireFiveSeven,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
		vecDir.x,
		vecDir.y,
		pPlayer->pev->punchangle.x * 100, pPlayer->pev->punchangle.y * 100, 
		m_iClip1 ? 0 : 1, 0 );
	*/

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flCycleTime;

	if (!m_iClip1 && pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	m_flTimeWeaponIdle = gpGlobals->curtime + 2;

	QAngle angle = pPlayer->GetPunchAngle();
	angle.x -= 2;
	pPlayer->SetPunchAngle( angle );
}


bool CWeaponFiveSeven::Reload()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) <= 0)
		return true;

	if ( !DefaultReload( 20, 0, ACT_VM_RELOAD ) )
		return true;

	pPlayer->SetAnimation( PLAYER_RELOAD );

	m_flAccuracy = 0.92;
	return true;
}

void CWeaponFiveSeven::WeaponIdle()
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
