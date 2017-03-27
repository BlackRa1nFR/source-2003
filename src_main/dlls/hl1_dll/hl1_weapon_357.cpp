//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "NPCEvent.h"
#include "hl1_basecombatweapon_shared.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"


//-----------------------------------------------------------------------------
// CWeapon357
//-----------------------------------------------------------------------------


extern int g_iWeaponCheat;


class CWeapon357 : public CBaseHL1CombatWeapon
{
	DECLARE_CLASS( CWeapon357, CBaseHL1CombatWeapon );
public:

	CWeapon357( void );

	void	Precache( void );
	bool	Deploy( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	bool	Reload( void );
	void	WeaponIdle( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	void	ToggleZoom( void );

private:
	bool	m_fInZoom;
};

LINK_ENTITY_TO_CLASS( weapon_357, CWeapon357 );

PRECACHE_WEAPON_REGISTER( weapon_357 );

IMPLEMENT_SERVERCLASS_ST( CWeapon357, DT_Weapon357 )
END_SEND_TABLE()

BEGIN_DATADESC( CWeapon357 )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeapon357::CWeapon357( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
	m_fInZoom			= false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeapon357::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "ammo_357" );
}

bool CWeapon357::Deploy( void )
{
// Bodygroup stuff not currently working correctly
//	if ( g_pGameRules->IsMultiplayer() )
//	{
		// enable laser sight geometry.
//		SetBodygroup( 4, 1 );
//	}
//	else
//	{
//		SetBodygroup( 4, 0 );
//	}

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon357::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	WeaponSound( SINGLE );
	pPlayer->m_fEffects |= EF_MUZZLEFLASH;

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );	

	pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	pPlayer->ViewPunch( QAngle( -10, 0, 0 ) );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2 );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon357::SecondaryAttack( void )
{
	// only in multiplayer
	if ( !g_pGameRules->IsMultiplayer() )
	{
		// unless we have cheats on
		if ( g_iWeaponCheat == 0 )
		{
			return;
		}
	}

	ToggleZoom();

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}


bool CWeapon357::Reload( void )
{
	bool fRet;

	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		if ( m_fInZoom )
		{
			ToggleZoom();
		}
	}

	return fRet;
}


void CWeapon357::WeaponIdle( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}

	BaseClass::WeaponIdle();
}


bool CWeapon357::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_fInZoom )
	{
		ToggleZoom();
	}

	return BaseClass::Holster( pSwitchingTo );
}


void CWeapon357::ToggleZoom( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	if ( m_fInZoom )
	{
		pPlayer->m_Local.m_iFOV = 0;
		m_fInZoom = false;
		pPlayer->ShowViewModel( true );
	}
	else
	{
		pPlayer->m_Local.m_iFOV = 40;
		m_fInZoom = true;
		pPlayer->ShowViewModel( false );
	}
}
