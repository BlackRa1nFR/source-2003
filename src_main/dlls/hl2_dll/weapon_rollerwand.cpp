//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:	'weapon' what lets the player controll the rollerbuddy.
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "entitylist.h"
#include "npc_rollerbuddy.h"
#include "ndebugoverlay.h"


//=========================================================
//=========================================================
class CWeaponRollerWand : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponRollerWand, CBaseHLCombatWeapon );

	CWeaponRollerWand();

	DECLARE_SERVERCLASS();

	void	Precache( void );
	bool	Deploy( void );

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1 | bits_CAP_WEAPON_RANGE_ATTACK2; }

	void PrimaryAttack( void );
	void SecondaryAttack( void );

/*
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
	{		
		switch( pEvent->event )
		{
			default:
				BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
				break;
		}
	}
*/
	DECLARE_ACTTABLE();
};


IMPLEMENT_SERVERCLASS_ST(CWeaponRollerWand, DT_WeaponRollerWand)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_rollerwand, CWeaponRollerWand );
PRECACHE_WEAPON_REGISTER(weapon_rollerwand);

acttable_t	CWeaponRollerWand::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR1, true },
};

IMPLEMENT_ACTTABLE(CWeaponRollerWand);

CWeaponRollerWand::CWeaponRollerWand( )
{
}

void CWeaponRollerWand::Precache( void )
{
	BaseClass::Precache();
}

bool CWeaponRollerWand::Deploy( void )
{
	bool fReturn;

	fReturn = BaseClass::Deploy();

	m_flNextPrimaryAttack = gpGlobals->curtime;
	m_flNextSecondaryAttack = gpGlobals->curtime;

	return fReturn;
}

void CWeaponRollerWand::PrimaryAttack( void )
{
	trace_t tr;

	Vector vecStart, vecDir;

	if(CBasePlayer *pPlayer = ToBasePlayer( GetOwner() ) )
	{
		CBaseEntity *pCommandEntity;
		vecStart = pPlayer->EyePosition();
		pPlayer->EyeVectors( &vecDir );
		UTIL_TraceLine( vecStart, vecStart + vecDir * 8192, MASK_OPAQUE, pPlayer, COLLISION_GROUP_NONE, &tr );

		//NDebugOverlay::Line( vecStart, tr.endpos, 0,255,0, true, 3 );
		//Msg( "Normal: %f %f %f\n", tr.plane.normal.x, tr.plane.normal.y, tr.plane.normal.z );

		// Remember which entity the trace hit.
		pCommandEntity = tr.m_pEnt;

		if( tr.plane.normal.z < 0.5 )
		{
			// trace down.
			vecStart = tr.endpos;

			vecStart += tr.plane.normal * 32;

			UTIL_TraceLine( vecStart, vecStart - Vector( 0, 0, 8192 ), MASK_OPAQUE, pPlayer, COLLISION_GROUP_NONE, &tr );

			//NDebugOverlay::Line( vecStart, tr.endpos, 0,255,0, true, 3 );
		}

		CBaseEntity *pEnt;

		pEnt = gEntList.FindEntityByClassname( NULL, "npc_rollerbuddy" );
		if( pEnt )
		{
			CNPC_RollerBuddy *pBuddy;
			pBuddy = (CNPC_RollerBuddy *)pEnt;

			pBuddy->CommandMoveToLocation( tr.endpos, pCommandEntity );
		}
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
}

void CWeaponRollerWand::SecondaryAttack( void )
{
	CBaseEntity *pEnt;

	pEnt = gEntList.FindEntityByClassname( NULL, "npc_rollerbuddy" );
	if( pEnt )
	{
		CNPC_RollerBuddy *pBuddy;
		pBuddy = (CNPC_RollerBuddy *)pEnt;
		pBuddy->ToggleBuddyMode( true );
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}
