//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_world.h"
#include "ivmodemanager.h"
#include "activitylist.h"
#include "decals.h"
#include "cdll_util.h"
#include "engine/ivmodelinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CWorld
#undef CWorld
#endif

CUtlLinkedList<C_World*, int> g_WorldEntities;
C_GameRules *g_pGameRules = NULL;


IMPLEMENT_CLIENTCLASS_DT(C_World, DT_World, CWorld)
	RecvPropFloat(RECVINFO(m_flWaveHeight)),
	RecvPropVector(RECVINFO(m_WorldMins)),
	RecvPropVector(RECVINFO(m_WorldMaxs)),
END_RECV_TABLE()

C_World::C_World( void )
{
	g_WorldEntities.AddToTail( this );
	m_flWaveHeight = 0.0f;
	ActivityList_Init();
}

C_World::~C_World( void )
{
	g_WorldEntities.FindAndRemove( this );
	ActivityList_Free();
}

void C_World::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );
}

void C_World::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// Always force reset to normal mode upon receipt of world in new map
	if ( updateType == DATA_UPDATE_CREATED )
	{
		modemanager->SwitchMode( false, true );
	}
}

void C_World::RegisterSharedActivities( void )
{
	ActivityList_RegisterSharedActivities();
}

// -----------------------------------------
//	Sprite Index info
// -----------------------------------------
short		g_sModelIndexLaser;			// holds the index for the laser beam
const char	*g_pModelNameLaser = "sprites/laserbeam.vmt";
short		g_sModelIndexLaserDot;		// holds the index for the laser beam dot
short		g_sModelIndexFireball;		// holds the index for the fireball
short		g_sModelIndexSmoke;			// holds the index for the smoke cloud
short		g_sModelIndexWExplosion;	// holds the index for the underwater explosion
short		g_sModelIndexBubbles;		// holds the index for the bubbles model
short		g_sModelIndexBloodDrop;		// holds the sprite index for the initial blood
short		g_sModelIndexBloodSpray;	// holds the sprite index for splattered blood

//-----------------------------------------------------------------------------
// Purpose: Precache global weapon sounds
//-----------------------------------------------------------------------------
void W_Precache(void)
{
	ResetFileWeaponInfoDatabase();

	g_sModelIndexFireball = modelinfo->GetModelIndex ("sprites/zerogxplode.vmt");// fireball
	g_sModelIndexWExplosion = modelinfo->GetModelIndex ("sprites/WXplo1.vmt");// underwater fireball
	g_sModelIndexSmoke = modelinfo->GetModelIndex ("sprites/steam1.vmt");// smoke
	g_sModelIndexBubbles = modelinfo->GetModelIndex ("sprites/bubble.vmt");//bubbles
	g_sModelIndexBloodSpray = modelinfo->GetModelIndex ("sprites/bloodspray.vmt"); // initial blood
	g_sModelIndexBloodDrop = modelinfo->GetModelIndex ("sprites/blood.vmt"); // splattered blood 
	g_sModelIndexLaser = modelinfo->GetModelIndex( (char *)g_pModelNameLaser );
	g_sModelIndexLaserDot = modelinfo->GetModelIndex("sprites/laserdot.vmt");

	/*
	for( i = 0 ; i < NUM_NEARMISSES ; i ++ )
	{
		enginesound->PrecacheSound ( pNearMissSounds[ i ] );
		g_sNearMissSounds[ i ] = pNearMissSounds[ i ];
	}
	*/
}

void C_World::Precache( void )
{
	// Set up game rules
	Assert( !GameRules() );
	if (GameRules())
	{
		delete g_pGameRules;
	}

	// UNDONE: Make most of these things server systems or precache_registers
	// =================================================
	//	Activities
	// =================================================
	ActivityList_Free();
	RegisterSharedActivities();

	// Get weapon precaches
	W_Precache();	

	// Call all registered precachers.
	CPrecacheRegister::Precache();	
}

void C_World::Spawn( void )
{
	Precache();
}



C_World *GetClientWorldEntity()
{
	if ( g_WorldEntities.Count() )
		return g_WorldEntities[ g_WorldEntities.Head() ];
	else
		return NULL;
}

