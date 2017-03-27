//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Handling for the suit batteries.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"

ConVar	sk_battery( "sk_battery","0" );			

class CItemBattery : public CItem
{
public:
	DECLARE_CLASS( CItemBattery, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/battery.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/battery.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if ((pPlayer->ArmorValue() < MAX_NORMAL_BATTERY) && pPlayer->IsSuitEquipped())
		{
			int pct;
			char szcharge[64];

			pPlayer->IncrementArmorValue( sk_battery.GetFloat(), MAX_NORMAL_BATTERY );

			CPASAttenuationFilter filter( pPlayer, "ItemBattery.Touch" );
			EmitSound( filter, pPlayer->entindex(), "ItemBattery.Touch" );

			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();

			UserMessageBegin( user, "ItemPickup" );
				WRITE_STRING( GetClassname() );
			MessageEnd();

			
			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)( (float)(pPlayer->ArmorValue() * 100.0) * (1.0/MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;
		
			Q_snprintf( szcharge,sizeof(szcharge),"!HEV_%1dP", pct );
			
			//UTIL_EmitSoundSuit(edict(), szcharge);
			pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return true;		
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);
PRECACHE_REGISTER(item_battery);

