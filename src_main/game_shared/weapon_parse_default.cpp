//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_parse.h"


// Default implementation for games that don't add custom data to the weapon scripts.
FileWeaponInfo_t* CreateWeaponInfo()
{
	return new FileWeaponInfo_t;
}



