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

===== item_longjump.cpp ========================================================

  handling for the longjump module
*/

#include "cbase.h"
#include "player.h"
//#include "weapons.h"
#include "gamerules.h"
#include "items.h"

class CItemLongJump : public CItem
{
public:
	DECLARE_CLASS( CItemLongJump, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/w_longjump.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/w_longjump.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->m_fLongJump )
		{
			return FALSE;
		}

		if ( pPlayer->IsSuitEquipped() )
		{
			pPlayer->m_fLongJump = TRUE;// player now has longjump module

			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();

			UserMessageBegin( user, "ItemPickup" );
				WRITE_STRING( STRING(pev->classname) );
			MessageEnd();

			UTIL_EmitSoundSuit( pPlayer->edict(), "!HEV_A1" );	// Play the longjump sound UNDONE: Kelly? correct sound?
			return true;		
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump );
