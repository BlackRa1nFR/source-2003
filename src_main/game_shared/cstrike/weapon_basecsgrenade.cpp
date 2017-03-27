//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbase.h"
#include "gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "weapon_basecsgrenade.h"


#ifdef CLIENT_DLL

#else

	#include "cs_player.h"
	#include "items.h"

#endif


#define GRENADE_TIMER	3.0f //Seconds



IMPLEMENT_NETWORKCLASS_ALIASED( BaseCSGrenade, DT_BaseCSGrenade )

BEGIN_NETWORK_TABLE(CBaseCSGrenade, DT_BaseCSGrenade)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CBaseCSGrenade )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_basecsgrenade, CBaseCSGrenade );

#ifndef CLIENT_DLL

	BEGIN_DATADESC( CBaseCSGrenade )
		DEFINE_FIELD( CBaseCSGrenade, m_bRedraw, FIELD_BOOLEAN ),
	END_DATADESC()

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void CBaseCSGrenade::Precache()
	{
		BaseClass::Precache();

		//UTIL_PrecacheOther( "npc_grenade_frag" );

		m_bRedraw = false;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	bool CBaseCSGrenade::Deploy()
	{
		m_bRedraw = false;

		return BaseClass::Deploy();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CBaseCSGrenade::Holster()
	{
		m_bRedraw = false;

		return BaseClass::Holster();
	}


	int CBaseCSGrenade::CapabilitiesGet()
	{
		return bits_CAP_WEAPON_RANGE_ATTACK1; 
	}


	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *pEvent - 
	//			*pOperator - 
	//-----------------------------------------------------------------------------
	void CBaseCSGrenade::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		switch( pEvent->event )
		{
			case EVENT_WEAPON_THROW:
				ThrowGrenade();
				DecrementAmmo( pOwner );
				break;

			default:
				BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
				break;
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CBaseCSGrenade::Reload()
	{
		if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
		{
			//Redraw the weapon
			SendWeaponAnim( ACT_VM_DRAW );

			//Update our times
			m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
			m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
			m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

			//Mark this as done
			m_bRedraw = false;
		}

		return true;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void CBaseCSGrenade::SecondaryAttack()
	{
		if ( m_bRedraw )
			return;

		CBaseCombatCharacter *pOwner  = GetOwner();

		if ( pOwner == NULL )
			return;

		CBasePlayer *pPlayer = ToBasePlayer( pOwner );
		
		if ( pPlayer == NULL )
			return;

		//See if we're ducking
		if ( pPlayer->GetFlags() & FL_DUCKING )
		{
			//Send the weapon animation
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
		}
		else
		{
			//Send the weapon animation
			SendWeaponAnim( ACT_VM_HAULBACK );
		}

		// Don't let weapon idle interfere in the middle of a throw!
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void CBaseCSGrenade::PrimaryAttack()
	{
		if ( m_bRedraw )
			return;

		CBaseCombatCharacter *pOwner  = GetOwner();
		
		if ( pOwner == NULL )
		{ 
			return;
		}

		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;

		if ( pPlayer == NULL )
			return;

		SendWeaponAnim( ACT_VM_THROW );
		
		// Don't let weapon idle interfere in the middle of a throw!
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *pOwner - 
	//-----------------------------------------------------------------------------
	void CBaseCSGrenade::DecrementAmmo( CBaseCombatCharacter *pOwner )
	{
		pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
		if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		{
			pOwner->Weapon_Drop( this );
			UTIL_Remove(this);
		}

	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void CBaseCSGrenade::ItemPostFrame()
	{
		BaseClass::ItemPostFrame();

		if ( m_bRedraw )
		{
			Reload();
		}
	}


	void CBaseCSGrenade::ThrowGrenade()
	{
		// Each CS grenade should override this.
		Assert( false );
	}

#endif

