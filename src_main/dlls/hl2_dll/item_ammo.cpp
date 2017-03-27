//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: The various ammo types for HL2	
//
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"


// ========================================================================
//	>> BoxSRounds
// ========================================================================
#define SIZE_BOX_SROUNDS 20

class CItem_BoxSRounds : public CItem
{
public:
	DECLARE_CLASS( CItem_BoxSRounds, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/boxsrounds.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/boxsrounds.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( SIZE_BOX_SROUNDS, "SmallRound"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_box_srounds, CItem_BoxSRounds);

// ========================================================================
//	>> LargeBoxSRounds
// ========================================================================
#define SIZE_LARGE_BOX_SROUNDS 100

class CItem_LargeBoxSRounds : public CItem
{
public:
	DECLARE_CLASS( CItem_LargeBoxSRounds, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/largeboxsrounds.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/largeboxsrounds.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( SIZE_LARGE_BOX_SROUNDS, "SmallRound"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_large_box_srounds, CItem_LargeBoxSRounds);

// ========================================================================
//	>> BoxMRounds
// ========================================================================
#define SIZE_BOX_MROUNDS 20

class CItem_BoxMRounds : public CItem
{
public:
	DECLARE_CLASS( CItem_BoxMRounds, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/boxmrounds.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/boxmrounds.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( SIZE_BOX_MROUNDS, "MediumRound"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_box_mrounds, CItem_BoxMRounds);

// ========================================================================
//	>> LargeBoxMRounds
// ========================================================================
#define SIZE_LARGE_BOX_MROUNDS 100

class CItem_LargeBoxMRounds : public CItem
{
public:
	DECLARE_CLASS( CItem_LargeBoxMRounds, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/largeboxmrounds.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/largeboxmrounds.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( SIZE_LARGE_BOX_MROUNDS, "MediumRound"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_large_box_mrounds, CItem_LargeBoxMRounds);

// ========================================================================
//	>> BoxLRounds
// ========================================================================
#define SIZE_BOX_LROUNDS 20

class CItem_BoxLRounds : public CItem
{
public:
	DECLARE_CLASS( CItem_BoxLRounds, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/boxbrounds.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/boxbrounds.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( SIZE_BOX_LROUNDS, "LargeRound"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_box_lrounds, CItem_BoxLRounds);

// ========================================================================
//	>> LargeBoxLRounds
// ========================================================================
#define SIZE_LARGE_BOX_LROUNDS 100

class CItem_LargeBoxLRounds : public CItem
{
public:
	DECLARE_CLASS( CItem_LargeBoxLRounds, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/largeboxbrounds.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/largeboxbrounds.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( SIZE_LARGE_BOX_LROUNDS, "LargeRound"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_large_box_lrounds, CItem_LargeBoxLRounds);

// ========================================================================
//	>> FlareRound
// ========================================================================
class CItem_FlareRound : public CItem
{
public:
	DECLARE_CLASS( CItem_FlareRound, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/flare.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/flare.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( 1, "FlareRound"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_flare_round, CItem_FlareRound);

// ========================================================================
//	>> BoxFlareRounds
// ========================================================================
#define SIZE_BOX_FLARE_ROUNDS 5

class CItem_BoxFlareRounds : public CItem
{
public:
	DECLARE_CLASS( CItem_BoxFlareRounds, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/boxflares.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/boxflares.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( SIZE_BOX_FLARE_ROUNDS, "FlareRound"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_box_flare_rounds, CItem_BoxFlareRounds);

// ========================================================================
//	>> ML_Grenade
// ========================================================================
class CItem_ML_Grenade : public CItem
{
public:
	DECLARE_CLASS( CItem_ML_Grenade, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/ml_grenade.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/ml_grenade.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( 1, "ML_Grenade"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_ml_grenade, CItem_ML_Grenade);

// ========================================================================
//	>> AR2_Grenade
// ========================================================================
class CItem_AR2_Grenade : public CItem
{
public:
	DECLARE_CLASS( CItem_AR2_Grenade, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/ar2_grenade.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/ar2_grenade.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( 1, "AR2_Grenade"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_ar2_grenade, CItem_AR2_Grenade);

// ========================================================================
//	>> BoxSniperRounds
// ========================================================================
#define SIZE_BOX_SNIPER_ROUNDS 10

class CItem_BoxSniperRounds : public CItem
{
public:
	DECLARE_CLASS( CItem_BoxSniperRounds, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/boxsniperrounds.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/boxsniperrounds.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( SIZE_BOX_SNIPER_ROUNDS, "SniperRound"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_box_sniper_rounds, CItem_BoxSniperRounds);


// ========================================================================
//	>> BoxBuckshot
// ========================================================================
#define SIZE_BOX_BUCKSHOT 20

class CItem_BoxBuckshot : public CItem
{
public:
	DECLARE_CLASS( CItem_BoxBuckshot, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/boxbuckshot.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		engine->PrecacheModel ("models/items/boxbuckshot.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->GiveAmmo( SIZE_BOX_BUCKSHOT, "Buckshot"))
		{
			UTIL_Remove(this);	
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_box_buckshot, CItem_BoxBuckshot);