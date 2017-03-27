//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbase.h"


#if defined( CLIENT_DLL )

	#define CWeaponGlock C_WeaponGlock
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponGlock : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CWeaponGlock, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponGlock();

	virtual void Spawn();
	virtual void Precache();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual bool Deploy();

	virtual void ItemPostFrame();

	void GLOCK18Fire( float flSpread, float flCycleTime, bool bFireBurst );
	void FireRemaining( int &shotsFired, float &shootTime );
	
	virtual bool Reload();

	virtual void WeaponIdle();

private:
	
	CWeaponGlock( const CWeaponGlock & );
	bool m_bBurstMode;
	int	m_iGlock18ShotsFired;	// used to keep track of the shots fired during the Glock18 burst fire mode.
	float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponGlock, DT_WeaponGlock )

BEGIN_NETWORK_TABLE( CWeaponGlock, DT_WeaponGlock )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponGlock )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_glock, CWeaponGlock );
PRECACHE_WEAPON_REGISTER( weapon_glock );



CWeaponGlock::CWeaponGlock()
{
}


void CWeaponGlock::Spawn( )
{
	//m_iDefaultAmmo = 20;
	m_bBurstMode = false;
	m_iGlock18ShotsFired = 0;
	m_flGlock18Shoot = 0.0;
	m_flAccuracy = 0.9;
	
	//ClearBits(m_iWeaponState, WPNSTATE_SHIELD_DRAWN);

	//FallInit();// get ready to fall down.
	BaseClass::Spawn();
}


void CWeaponGlock::Precache()
{
	//m_iShellId = m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell
	//m_usFireGlock18 = PRECACHE_EVENT(1, "events/glock18.sc");
	BaseClass::Precache();
}

bool CWeaponGlock::Deploy( )
{
	// pev->body = 1;
	m_bBurstMode = false;
	m_iGlock18ShotsFired = 0;
	m_flGlock18Shoot = 0.0;
	m_flAccuracy = 0.9;

	BaseClass::Deploy();
	
	// MIKETODO: shields
	/*
	//ClearBits(m_iWeaponState, WPNSTATE_SHIELD_DRAWN);
	if ( pPlayer->HasShield() )
	{
		m_iWeaponState &= ~WPNSTATE_GLOCK18_BURST_MODE;
		return DefaultDeploy( "models/shield/v_shield_glock18.mdl", "models/shield/p_shield_glock18.mdl", GLOCK18_SHIELD_DRAW, "shieldgun" , UseDecrement() ? 1: 0 );
	}
	else
	{
		if (RANDOM_LONG (0,1) )
			return DefaultDeploy( "models/v_glock18.mdl", "models/p_glock18.mdl", GLOCK18_DRAW, "onehanded", UseDecrement() ? 1: 0 );
		else
			return DefaultDeploy( "models/v_glock18.mdl", "models/p_glock18.mdl", GLOCK18_DRAW2, "onehanded", UseDecrement() ? 1: 0 );
	}
	*/

	return true;
}

void CWeaponGlock::SecondaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	//MIKETODO: shields
	//if ( ShieldSecondaryFire( GLOCK18_SHIELD_UP, GLOCK18_SHIELD_DOWN ) == true )
	//	 return;

	if ( m_bBurstMode )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Switch_To_SemiAuto" );
		m_bBurstMode = false;
	}
	else
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Switch_To_BurstFire" );
		m_bBurstMode = true;
	}
	
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}

void CWeaponGlock::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( m_bBurstMode )
	{
		if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
			GLOCK18Fire( (1.2) * (1 - m_flAccuracy), 0.5, true );
		
		else if (pPlayer->GetAbsVelocity().Length2D() > 0)
			GLOCK18Fire( (0.185) * (1 - m_flAccuracy), 0.5, true );
		
		else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
			GLOCK18Fire( (0.095) * (1 - m_flAccuracy), 0.5, true );
		
		else
		    GLOCK18Fire( (0.3) * (1 - m_flAccuracy), 0.5, true );
	}
	else
	{
		if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
			GLOCK18Fire( (1.0) * (1 - m_flAccuracy), 0.2, false );
		
		else if (pPlayer->GetAbsVelocity().Length2D() > 0)
			GLOCK18Fire( (0.165) * (1 - m_flAccuracy), 0.2, false );
		
		else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
			GLOCK18Fire( (0.075) * (1 - m_flAccuracy), 0.2, false );
		
		else
		    GLOCK18Fire( (0.1) * (1 - m_flAccuracy), 0.2, false );
	}
}


// GOOSEMAN : FireRemaining used by Glock18
void CWeaponGlock::FireRemaining( int &shotsFired, float &shootTime )
{
	CCSPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		Error( "!pPlayer" );

	float nexttime = 0.1;
	m_iClip1--;
	if (m_iClip1 < 0)
	{
		m_iClip1 = 0;
		shotsFired = 3;
		shootTime = 0.0f;
		return;
	}

	Vector vecDir = pPlayer->FireBullets3( 
		pPlayer->Weapon_ShootPosition(), 
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), 
		0.05, 
		8192, 
		1, 
		GetPrimaryAmmoType(), 
		18, 
		0.9, 
		pPlayer );

	WeaponSound( SINGLE );

	/*
	#if defined( CLIENT_WEAPONS )
		int flag = FEV_NOTHOST;
	#else
		int flag = 0;
	#endif

	PLAYBACK_EVENT_FULL( flag, m_pPlayer->edict(), m_usFireGlock18,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
		vecDir.x,
		vecDir.y,
		m_pPlayer->pev->punchangle.x * 10000,
		m_pPlayer->pev->punchangle.y * 10000, 
		m_iClip1 ? 0 : 1, 0 );
	*/

	//pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	//pPlayer->SetAnimation( PLAYER_ATTACK1 );

	shotsFired++;

	if (shotsFired != 3)
		shootTime = gpGlobals->curtime + nexttime;
	else
		shootTime = 0.0;
}


void CWeaponGlock::ItemPostFrame()
{
	if ( m_flGlock18Shoot != 0.0 )
		FireRemaining( m_iGlock18ShotsFired, m_flGlock18Shoot );

	BaseClass::ItemPostFrame();
}


void CWeaponGlock::GLOCK18Fire( float flSpread, float flCycleTime, bool bFireBurst )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( bFireBurst == true )
	{
		m_iGlock18ShotsFired = 0;
	}
	else
	{
		flCycleTime -= 0.05;
		m_iShotsFired++;

		if (m_iShotsFired > 1)
			return;
	}

	// Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
	if (m_flLastFire == 0)
	{
		m_flLastFire = gpGlobals->curtime;
	}
	else 
	{
		m_flAccuracy -= (0.275)*(0.325 - (gpGlobals->curtime - m_flLastFire));

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

	pPlayer->m_fEffects |= EF_MUZZLEFLASH;

	//SetPlayerShieldAnim();
	
	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// non-silenced
	//pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	//pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecDir = pPlayer->FireBullets3( 
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(),
		flSpread, 
		8192, 
		1, 
		GetPrimaryAmmoType(), 
		25, 
		0.75, 
		pPlayer );

	WeaponSound( SINGLE );

	/*
	int flag;
	#if defined( CLIENT_WEAPONS )
		flag = FEV_NOTHOST;
	#else
		flag = 0;
	#endif

	PLAYBACK_EVENT_FULL( flag, pPlayer->edict(), m_usFireGlock18,
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
	vecDir.x,
	vecDir.y,
	pPlayer->pev->punchangle.x * 100,
	pPlayer->pev->punchangle.y * 100, 
	m_iClip1 ? 0 : 1, 0 );
	*/

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flCycleTime;

	if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	m_flTimeWeaponIdle = gpGlobals->curtime + 2.5;

	if (bFireBurst)
	{
		// Fire off the next two rounds
		m_flGlock18Shoot = gpGlobals->curtime + 0.1;
		m_iGlock18ShotsFired++;
	}

	//ResetPlayerShieldAnim();
}


bool CWeaponGlock::Reload()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( m_flGlock18Shoot != 0 )
		return true;

	if (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
		return true;

	int iResult;

	/*if ( pPlayer->HasShield() )
		iResult = DefaultReload( 20, GLOCK18_SHIELD_RELOAD, 2.2);
	else */
	iResult = DefaultReload( 20, 0, ACT_VM_RELOAD );
	if ( !iResult )
		return true;

	pPlayer->SetAnimation( PLAYER_RELOAD );

	m_flAccuracy = 0.9;
	return true;
}

void CWeaponGlock::WeaponIdle()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	//ResetEmptySound( );

	pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	if ( pPlayer->HasShield() )
	{
		m_flTimeWeaponIdle = gpGlobals->curtime + 20;
		
		//MIKETODO: shields
		//if ( FBitSet(m_iWeaponState, WPNSTATE_SHIELD_DRAWN) )
		//	 SendWeaponAnim( GLOCK18_SHIELD_IDLE, UseDecrement() ? 1:0 );
	}
	else
	{
		// only idle if the slid isn't back
		if (m_iClip1 != 0)
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}
}
