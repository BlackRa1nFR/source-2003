//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbase.h"


#if defined( CLIENT_DLL )

	#define CWeaponUSP C_WeaponUSP
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponUSP : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CWeaponUSP, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponUSP();

	virtual void Spawn();
	virtual void Precache();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual bool Deploy();

	virtual bool Reload();
	virtual void WeaponIdle();

private:
	
	CWeaponUSP( const CWeaponUSP & );
	void USPFire( float flSpread, float flCycleTime, bool fUseSemi );

	bool m_bSilencerOn;
	float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponUSP, DT_WeaponUSP )

BEGIN_NETWORK_TABLE( CWeaponUSP, DT_WeaponUSP )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponUSP )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_usp, CWeaponUSP );
PRECACHE_WEAPON_REGISTER( weapon_usp );



CWeaponUSP::CWeaponUSP()
{
}


void CWeaponUSP::Spawn()
{
	//m_iDefaultAmmo = 12;
	m_flAccuracy = 0.92;

	//FallInit();// get ready to fall down.
	BaseClass::Spawn();
}


void CWeaponUSP::Precache()
{
	//m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell
	//m_usFireUSP = PRECACHE_EVENT(1, "events/usp.sc");
	BaseClass::Precache();
}

bool CWeaponUSP::Deploy()
{
	m_flAccuracy = 0.92;

	return BaseClass::Deploy();
}

void CWeaponUSP::SecondaryAttack()
{
	if ( m_bSilencerOn )
	{
		WeaponSound( SPECIAL2 );	// Taking off silencer.
	}
	else
	{
		WeaponSound( SPECIAL3 );	// Putting on silencer.
	}
	m_bSilencerOn = !m_bSilencerOn;

	m_flNextSecondaryAttack = gpGlobals->curtime + 3;
	m_flNextPrimaryAttack = gpGlobals->curtime + 3;
	m_flTimeWeaponIdle = gpGlobals->curtime + 3;
}


void CWeaponUSP::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( m_bSilencerOn )
	{
		if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
			USPFire( (1.3) * (1 - m_flAccuracy), 0.225, false );
		else if (pPlayer->GetAbsVelocity().Length2D() > 0)
			USPFire( (0.25) * (1 - m_flAccuracy), 0.225, false );
		else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
			USPFire( (0.125) * (1 - m_flAccuracy), 0.225, false );
		else
			USPFire( (0.15) * (1 - m_flAccuracy), 0.225, false );
	}
	else
	{
		if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
			USPFire( (1.2) * (1 - m_flAccuracy), 0.225, false );
		else if (pPlayer->GetAbsVelocity().Length2D() > 0)
			USPFire( (0.225) * (1 - m_flAccuracy), 0.225, false );
		else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
			USPFire( (0.08) * (1 - m_flAccuracy), 0.225, false );
		else
			USPFire( (0.1) * (1 - m_flAccuracy), 0.225, false );
	}

}

void CWeaponUSP::USPFire( float flSpread, float flCycleTime, bool fUseSemi )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	flCycleTime -= 0.075;
	m_iShotsFired++;

	if (m_iShotsFired > 1)
		return;
	
	// Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
	if (m_flLastFire == 0)
		m_flLastFire = gpGlobals->curtime;
	else 
	{
		m_flAccuracy -= (0.275)*(0.3 - (gpGlobals->curtime - m_flLastFire));

		if (m_flAccuracy > 0.92)
			m_flAccuracy = 0.92;
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

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flCycleTime;

	m_iClip1--;

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	
	int damageAmt;
	if ( m_bSilencerOn )
	{
		WeaponSound( SPECIAL1 );
		damageAmt = 30;
	}
	else
	{
		WeaponSound( SINGLE );
		damageAmt = 34;
		pPlayer->m_fEffects |= EF_MUZZLEFLASH;
	}
	
	Vector vecDir = pPlayer->FireBullets3( 
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(),
		flSpread, 
		4096, 
		1, 
		GetPrimaryAmmoType(), 
		damageAmt, 
		0.79, 
		pPlayer );


	/*
	pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	int flag;
	#if defined( CLIENT_WEAPONS )
		flag = FEV_NOTHOST;
	#else
		flag = 0;
	#endif

	PLAYBACK_EVENT_FULL( flag, pPlayer->edict(), m_usFireUSP,
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
	vecDir.x,
	vecDir.y,
	pPlayer->pev->punchangle.x * 100, 0, m_iClip1 ? 0 : 1, (m_iWeaponState & WPNSTATE_USP_SILENCER_ON) ? 1 : 0 );
	*/

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


bool CWeaponUSP::Reload()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if (pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) <= 0)
		return true;

	int iResult = DefaultReload( 12, 0, ACT_VM_RELOAD );
	if (!iResult)
		return true;

	pPlayer->SetAnimation( PLAYER_RELOAD );
	m_flAccuracy = 0.92;
	return true;
}

void CWeaponUSP::WeaponIdle()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	//ResetEmptySound( );

	pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	// only idle if the slid isn't back
	if (m_iClip1 != 0)
	{
		m_flTimeWeaponIdle = gpGlobals->curtime + 60.0;

		SendWeaponAnim( ACT_VM_IDLE );
	}
}
