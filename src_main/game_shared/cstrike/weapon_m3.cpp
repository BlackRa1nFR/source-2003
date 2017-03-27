//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbase.h"


#if defined( CLIENT_DLL )

	#define CWeaponM3 C_WeaponM3
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponM3 : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CWeaponM3, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponM3();

	virtual void Spawn();
	virtual void Precache();
	virtual void PrimaryAttack();
	virtual bool Reload();
	virtual void WeaponIdle();


private:

	CWeaponM3( const CWeaponM3 & );

	float m_flPumpTime;
	int m_fInSpecialReload;

};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponM3, DT_WeaponM3 )

BEGIN_NETWORK_TABLE( CWeaponM3, DT_WeaponM3 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponM3 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_m3, CWeaponM3 );
PRECACHE_WEAPON_REGISTER( weapon_m3 );



CWeaponM3::CWeaponM3()
{
	m_flPumpTime = 0;
}

void CWeaponM3::Spawn()
{
	//m_iDefaultAmmo = M3_DEFAULT_GIVE;
	//FallInit();// get ready to fall
	BaseClass::Spawn();
}


void CWeaponM3::Precache()
{
	//m_iShellId = m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// m3 shell
	//m_usFireM3 = PRECACHE_EVENT (1, "events/m3.sc");
	BaseClass::Precache();
}


void CWeaponM3::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	// don't fire underwater
	if (pPlayer->GetWaterLevel() == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		return;
	}

	if (m_iClip1 <= 0)
	{
		Reload();
		if (m_iClip1 == 0)
			PlayEmptySound( );
		return;
	}

	//pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	//pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip1--;
	pPlayer->m_fEffects |= EF_MUZZLEFLASH;

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	//MIKETODO: CS1 calls a special function that does 9 shots but only 1 decal and effect.
	//MIKETODO: it also scales damage based on distance.
	for ( int iBullet=0; iBullet < 9; iBullet++ )
	{
		pPlayer->FireBullets3( 
			pPlayer->Weapon_ShootPosition(), 
			pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), 
			0.0675,					// spread
			3000,					// distance
			1,						// penetration
			GetPrimaryAmmoType(),	// bullet type
			32, 					// damage
			0.96,					// range modifier
			pPlayer );
	}

	WeaponSound( SINGLE );

	/*
	int flag;
	#if defined( CLIENT_WEAPONS )
		flag = FEV_NOTHOST;
	#else
		flag = 0;
	#endif

	PLAYBACK_EVENT( flag, pPlayer->edict(), m_usFireM3);
	*/

	if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	if (m_iClip1 != 0)
		m_flPumpTime = gpGlobals->curtime + 0.5;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.875;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.875;
	if (m_iClip1 != 0)
		m_flTimeWeaponIdle = gpGlobals->curtime + 2.5;
	else
		m_flTimeWeaponIdle = 0.875;
	m_fInSpecialReload = 0;

	// Update punch angles.
	QAngle angle = pPlayer->GetPunchAngle();
	angle.x -= !(pPlayer->GetFlags() & FL_ONGROUND ) ? SHARED_RANDOMINT( 8, 11 ) : SHARED_RANDOMINT( 4, 6 );
	pPlayer->SetPunchAngle( angle );
	
	pPlayer->m_flEjectBrass = gpGlobals->curtime + 0.45;
}


bool CWeaponM3::Reload()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 || m_iClip1 == GetMaxClip1())
		return true;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return true;

	return BaseClass::Reload();
	
	//MIKETODO: shotgun reloading (wait until we get content)
	/*
	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		pPlayer->SetAnimation( PLAYER_RELOAD );

		SendWeaponAnim( M3_START_RELOAD, UseDecrement() ? 1:0);
		m_fInSpecialReload = 1;
		pPlayer->m_flNextAttack = gpGlobals->curtime + 0.55;
		m_flTimeWeaponIdle = gpGlobals->curtime + 0.55;
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.55;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.55;
		return true;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > gpGlobals->curtime)
			return true;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		SendWeaponAnim( M3_RELOAD, UseDecrement() ? 1: 0);

		m_flNextReload = gpGlobals->curtime + 0.45;
		m_flTimeWeaponIdle = gpGlobals->curtime + 0.45;
	}
	else
	{
		// Add them to the clip
		m_iClip1 += 1;
		pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;

		pPlayer->ammo_buckshot--;
	}
	*/

	return true;
}


void CWeaponM3::WeaponIdle()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	//ResetEmptySound( );

	pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flPumpTime && m_flPumpTime < gpGlobals->curtime)
	{
		// play pumping sound
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle < gpGlobals->curtime)
	{
		if (m_iClip1 == 0 && m_fInSpecialReload == 0 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ))
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip1 != 8 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ))
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				//MIKETODO: shotgun anims
				//SendWeaponAnim( M3_PUMP, UseDecrement() ? 1: 0);
				
				// play cocking sound
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = gpGlobals->curtime + 1.5;
			}
		}
		else
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}
}
