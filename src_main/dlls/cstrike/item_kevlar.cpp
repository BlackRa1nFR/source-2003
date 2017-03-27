
#include "cbase.h"
#include "items.h"
#include "cs_player.h"


class CItemKevlar : public CItem
{
public:

	DECLARE_CLASS( CItemKevlar, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/w_kevlar.mdl" );
		BaseClass::Spawn( );
	}
	
	void Precache( void )
	{
		engine->PrecacheModel( "models/w_kevlar.mdl" );
	}
	
	bool MyTouch( CBasePlayer *pBasePlayer )
	{
		CCSPlayer *pPlayer = dynamic_cast< CCSPlayer* >( pBasePlayer );
		if ( !pPlayer )
		{
			Assert( false );
			return false;
		}

		if ( pPlayer->m_iKevlar == 0 )
			pPlayer->m_iKevlar = 1; // player now has kevlar only
		
		pPlayer->SetArmorValue( 100 );

		CPASAttenuationFilter filter( pBasePlayer );
		EmitSound( filter, entindex(), "BaseCombatCharacter.ItemPickup2" );

		return true;
	}
};

LINK_ENTITY_TO_CLASS( item_kevlar, CItemKevlar );


