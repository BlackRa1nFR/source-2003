/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== item_security.cpp ========================================================

  handling for the security item
*/

#include "cbase.h"
#include "player.h"
//#include "weapons.h"
#include "gamerules.h"
#include "items.h"

class CItemSecurity : public CItem
{
public:
	DECLARE_CLASS( CItemSecurity, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/w_security.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/w_security.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);

