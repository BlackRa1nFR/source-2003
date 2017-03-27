//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

class CWeaponCubemap : public CBaseCombatWeapon
{
public:

	DECLARE_CLASS( CWeaponCubemap, CBaseCombatWeapon );

	void	Precache( void );

	bool	HasAnyAmmo( void )	{ return true; }

	DECLARE_SERVERCLASS();
};

LINK_ENTITY_TO_CLASS( weapon_cubemap, CWeaponCubemap );

IMPLEMENT_SERVERCLASS_ST( CWeaponCubemap, DT_WeaponCubemap )
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCubemap::Precache( void )
{
	BaseClass::Precache();
}
