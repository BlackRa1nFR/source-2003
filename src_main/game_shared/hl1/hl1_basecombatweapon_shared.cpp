//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hl1_basecombatweapon_shared.h"

#include "hl1_player_shared.h"

LINK_ENTITY_TO_CLASS( basehl1combatweapon, CBaseHL1CombatWeapon );

IMPLEMENT_NETWORKCLASS_ALIASED( BaseHL1CombatWeapon , DT_BaseHL1CombatWeapon )

BEGIN_NETWORK_TABLE( CBaseHL1CombatWeapon , DT_BaseHL1CombatWeapon )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CBaseHL1CombatWeapon )
END_PREDICTION_DATA()


void CBaseHL1CombatWeapon::Spawn( void )
{
	Precache();

	SetSize( Vector( -24, -24, 0 ), Vector( 24, 24, 16 ) );
	SetSolid( SOLID_BBOX );
	m_flNextEmptySoundTime = 0.0f;

	m_iState = WEAPON_NOT_CARRIED;
	// Assume 
	m_nViewModelIndex = 0;

	// If I use clips, set my clips to the default
	if ( UsesClipsForAmmo1() )
	{
		m_iClip1 = GetDefaultClip1();
	}
	else
	{
		m_iClip1 = WEAPON_NOCLIP;
	}
	if ( UsesClipsForAmmo2() )
	{
		m_iClip2 = GetDefaultClip2();
	}
	else
	{
		m_iClip2 = WEAPON_NOCLIP;
	}

	SetModel( GetWorldModel() );

#if !defined( CLIENT_DLL )
	FallInit();
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	// Default to non-removeable, because we don't want the
	// game_weapon_manager entity to remove weapons that have
	// been hand-placed by level designers. We only want to remove
	// weapons that have been dropped by NPC's.
	SetRemoveable( false );
#endif
}
