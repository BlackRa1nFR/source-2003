//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbase.h"


#if defined( CLIENT_DLL )

	#define CWeaponFamas C_WeaponFamas
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponFamas : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CWeaponFamas, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponFamas();

	virtual void Spawn();
	virtual void Precache();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual bool Deploy();

	virtual void ItemPostFrame();

	void FamasFire( float flSpread, float flCycleTime, bool fUseAutoAim, bool bFireBurst );
	void FireRemaining( int &shotsFired, float &shootTime, bool bIsGlock );
	
	virtual bool Reload();

	virtual void WeaponIdle();

private:
	
	CWeaponFamas( const CWeaponFamas & );
	bool m_bBurstMode;
	float	m_flFamasShoot;			// time to shoot the remaining bullets of the famas burst fire
	int		m_iFamasShotsFired;		// used to keep track of the shots fired during the Famas burst fire mode....
	float	m_fBurstSpread;			// used to keep track of current spread factor so that all bullets in spread use same spread
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFamas, DT_WeaponFamas )

BEGIN_NETWORK_TABLE( CWeaponFamas, DT_WeaponFamas )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponFamas )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_famas, CWeaponFamas );
PRECACHE_WEAPON_REGISTER( weapon_famas );



CWeaponFamas::CWeaponFamas()
{
	m_bBurstMode = false;
}

void CWeaponFamas::Spawn()
{
	//m_iDefaultAmmo = 25;
	//FallInit();// get ready to fall down.
	BaseClass::Spawn();
}

void CWeaponFamas::Precache()
{
	//m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell
	//m_usFireFamas = PRECACHE_EVENT (1,"events/famas.sc");
	BaseClass::Precache();
}

bool CWeaponFamas::Deploy( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;

	return BaseClass::Deploy();
}

// Secondary attack could be three-round burst mode
void CWeaponFamas::SecondaryAttack()
{	
	CCSPlayer *pPlayer = GetPlayerOwner();

	if ( m_bBurstMode )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Switch_To_FullAuto" );
		m_bBurstMode = false;
	}
	else
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Switch_To_BurstFire" );
		m_bBurstMode = true;
	}
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}

void CWeaponFamas::PrimaryAttack()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	// don't fire underwater
	if (pPlayer->GetWaterLevel() == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		return;
	}
	
	if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )	// if player is in air
		FamasFire( 0.030 + (0.3) * (m_flAccuracy), 0.0825, false, m_bBurstMode );
	
	else if ( pPlayer->GetAbsVelocity().Length2D() > 140 )	// if player is moving
		FamasFire( 0.030 + (0.07) * (m_flAccuracy), 0.0825, false, m_bBurstMode );
	/* new code */
	else
		FamasFire( (0.020) * (m_flAccuracy), 0.0825, false, m_bBurstMode );
}


// GOOSEMAN : FireRemaining used by Glock18
void CWeaponFamas::FireRemaining( int &shotsFired, float &shootTime, bool bIsGlock )
{
	float nexttime = 0.1;

	m_iClip1--;

	if (m_iClip1 < 0)
	{
		m_iClip1 = 0;
		shotsFired = 3;
		shootTime = 0.0f;
		return;
	}

	CCSPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		Error( "!pPlayer" );

	Vector vecSrc = pPlayer->Weapon_ShootPosition();

	// Famas burst mode
	Vector vecDir = pPlayer->FireBullets3( 
		pPlayer->Weapon_ShootPosition(), 
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), 
		m_fBurstSpread, 
		8192, 
		2, 
		GetPrimaryAmmoType(), 
		30, 
		0.96, 
		pPlayer );

	/*
	#if defined( CLIENT_WEAPONS )
		int flag = FEV_NOTHOST;
	#else
		int flag = 0;
	#endif

	PLAYBACK_EVENT_FULL( flag, m_pPlayer->edict(), m_usFireFamas,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
		vecDir.x, 
		vecDir.y,
		m_pPlayer->pev->punchangle.x * 10000000,
		m_pPlayer->pev->punchangle.y * 10000000,
		0, 0 );
	*/

	pPlayer->m_fEffects |= EF_MUZZLEFLASH;
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	shotsFired++;

	if (shotsFired != 3)
		shootTime = gpGlobals->curtime + nexttime;
	else
		shootTime = 0.0;
}


void CWeaponFamas::ItemPostFrame()
{
	if (m_flFamasShoot != 0.0)
		FireRemaining(m_iFamasShotsFired, m_flFamasShoot, FALSE);

	BaseClass::ItemPostFrame();
}


void CWeaponFamas::FamasFire( float flSpread , float flCycleTime, bool fUseAutoAim, bool bFireBurst )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	// change a few things if we're in burst mode
	if (bFireBurst)
	{
		m_iFamasShotsFired = 0;
		flCycleTime = .55f;		// how long it takes to become accurate again
	}
	else
	{
		flSpread += 0.01;
	}
	m_iShotsFired++;	
	
	m_bDelayFire = true;

	m_flAccuracy = (m_iShotsFired * m_iShotsFired * m_iShotsFired / 215) + 0.3;
	if (m_flAccuracy > 1)
		m_flAccuracy = 1;

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

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// non-silenced
	//pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	//pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	int damage = 30;
	if ( bFireBurst )
		damage = 34;

	Vector vecDir = pPlayer->FireBullets3( 
		pPlayer->Weapon_ShootPosition(), 
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), 
		flSpread, 
		8192, 
		2, 
		GetPrimaryAmmoType(), 
		damage, 
		0.96, 
		pPlayer );

	WeaponSound( SINGLE );

	/*
	int flag;
	#if defined( CLIENT_WEAPONS )
		flag = FEV_NOTHOST;
	#else
		flag = 0;
	#endif
		
	// This mask holds three custom flags - first bit is burst mode flag, second bit tells us if we have enough bullets for burst mode, last says whether or not this is our first burst mode shot
	int mask = 0;
	//mask |= (m_iWeaponState & WPNSTATE_FAMAS_BURST_MODE) ? 1 : 0;
	//mask |= ((m_iClip1 < 2) ? 1 : 0) << 4;
	//mask |= ((m_iFamasShotsFired == 0) ? 1 : 0) << 8;
	
	PLAYBACK_EVENT_FULL( flag, pPlayer->edict(), m_usFireFamas,
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
	vecDir.x, 
	vecDir.y,
	pPlayer->pev->punchangle.x * 10000000,
	pPlayer->pev->punchangle.y * 10000000,
	mask, 0 );
	*/

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flCycleTime;

	if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	m_flTimeWeaponIdle = gpGlobals->curtime + 1.1;	// 1.9


	if ( pPlayer->GetAbsVelocity().Length2D() > 0 )
		KickBack ( 1, 0.45, 0.275, 0.05, 4, 2.5, 7 );
	
	else if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
		KickBack ( 1.25, 0.45, 0.22, 0.18, 5.5, 4, 5 );
	
	else if ( FBitSet( pPlayer->GetFlags(), FL_DUCKING ) )
		KickBack ( 0.575, 0.325, 0.2, 0.011, 3.25, 2, 8 );
	
	else
		KickBack ( 0.625, 0.375, 0.25, 0.0125, 3.5, 2.25, 8 );

	if (bFireBurst)
	{
		// Fire off the next two rounds
		m_flFamasShoot = gpGlobals->curtime + 0.05;	// 0.1
		m_fBurstSpread = flSpread;
		m_iFamasShotsFired++;
	}
}


bool CWeaponFamas::Reload()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	// Skip the reload if we're in the middle of a burst fire.
	// This used to be in the base weapon class.
	if ( m_flFamasShoot != 0 )
		return true;

	if (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
		return true;

	int iResult;

	iResult = DefaultReload( 25, 0, ACT_VM_RELOAD );

	if (!iResult)
		return true;

	pPlayer->SetAnimation( PLAYER_RELOAD );

	if ((iResult) && (pPlayer->GetFOV() != 90))
		SecondaryAttack();

	m_flAccuracy = 0.0;
	m_iShotsFired = 0;
	m_bDelayFire = false;
	return true;
}

void CWeaponFamas::WeaponIdle()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	//ResetEmptySound( );

	pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	m_flTimeWeaponIdle = gpGlobals->curtime + 20;
	// only idle if the slid isn't back
	SendWeaponAnim( ACT_VM_IDLE );
}
