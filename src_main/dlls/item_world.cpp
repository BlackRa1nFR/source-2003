//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Handling for the base world item. Most of this was moved from items.cpp.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "engine/IEngineSound.h"


class CWorldItem : public CBaseAnimating
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWorldItem, CBaseAnimating );

	bool	KeyValue( const char *szKeyName, const char *szValue ); 
	void	Spawn( void );
	int		m_iType;
};

LINK_ENTITY_TO_CLASS(world_items, CWorldItem);

BEGIN_DATADESC( CWorldItem )

	DEFINE_FIELD( CWorldItem, m_iType, FIELD_INTEGER ),

END_DATADESC()


bool CWorldItem::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "type"))
	{
		m_iType = atoi(szValue);
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch (m_iType) 
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", GetLocalOrigin(), GetLocalAngles() );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", GetLocalOrigin(), GetLocalAngles() );
		break;
	}

	if (!pEntity)
	{
		Warning("unable to create world_item %d\n", m_iType );
	}
	else
	{
		pEntity->m_target = m_target;
		pEntity->SetName( GetEntityName() );
		pEntity->ClearSpawnFlags();
		pEntity->AddSpawnFlags( m_spawnflags );
	}

	UTIL_RemoveImmediate( this );
}


BEGIN_DATADESC( CItem )

	// Function Pointers
	DEFINE_FUNCTION( CItem, ItemTouch ),
	DEFINE_FUNCTION( CItem, Materialize ),

	// Outputs
	DEFINE_OUTPUT(CItem, m_OnPlayerTouch, "OnPlayerTouch"),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem::Spawn( void )
{
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolid( SOLID_BBOX );
	
	// This will make them not collide with the player, but will collide
	// against other items + weapons
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	UTIL_SetSize(this, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	//Create the object in the physics system
	if ( VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE | FSOLID_TRIGGER, false ) == NULL )
	{
		AddSolidFlags( FSOLID_NOT_STANDABLE );
		AddSolidFlags( FSOLID_TRIGGER );

		// If it's not physical, drop it to the floor
		if (UTIL_DropToFloor(this, MASK_SOLID) == 0)
		{
			Warning( "Item %s fell out of level at %f,%f,%f\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
			UTIL_Remove( this );
			return;
		}
	}
	Relink();
}

extern int gEvilImpulse101;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CItem::ItemTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if ( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch( pPlayer ))
	{
		m_OnPlayerTouch.FireOutput(pOther, this);

		SetTouch( NULL );
		
		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn(); 
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if (gEvilImpulse101)
	{
		UTIL_Remove( this );
	}
}

CBaseEntity* CItem::Respawn( void )
{
	SetTouch( NULL );
	m_fEffects |= EF_NODRAW;

	UTIL_SetOrigin( this, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.

	SetThink ( &CItem::Materialize );
	SetNextThink( gpGlobals->curtime + g_pGameRules->FlItemRespawnTime( this ) );
	return this;
}

void CItem::Materialize( void )
{
	if ( m_fEffects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EmitSound( "Item.Materialize" );
		m_fEffects &= ~EF_NODRAW;
		m_fEffects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CItem::ItemTouch );
}


