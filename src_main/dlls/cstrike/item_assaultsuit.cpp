
#include "cbase.h"
#include "items.h"
#include "cs_player.h"


class CItemAssaultSuit : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/w_assault.mdl");
		CItem::Spawn( );
	}
	
	void Precache( void )
	{
		engine->PrecacheModel( "models/w_assault.mdl" );
	}
	
	bool MyTouch( CBasePlayer *pBasePlayer )
	{
		CCSPlayer *pPlayer = dynamic_cast< CCSPlayer* >( pBasePlayer );
		if ( !pPlayer )
		{
			Assert( false );
			return false;
		}

		pPlayer->m_iKevlar = 2; // player now has kevlar AND helmet
		pPlayer->SetArmorValue( 100 );

		CPASAttenuationFilter filter( pBasePlayer );
		EmitSound( filter, entindex(), "BaseCombatCharacter.ItemPickup2" );

		return true;		
	}
};

LINK_ENTITY_TO_CLASS( item_assaultsuit, CItemAssaultSuit );


