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

===== tf_client.cpp ========================================================

  HL2 client/server game specific stuff

*/

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "shake.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "cs_player.h"
#include "cs_gamerules.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

extern bool			g_fGameOver;

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBaseTFPlayer for pev, and call spawn
	CCSPlayer *pPlayer = CCSPlayer::CreatePlayer( "player", pEdict );
	pPlayer->InitialSpawn();
	pPlayer->PlayerData()->netname = AllocPooledString( playername );
	pPlayer->Spawn();

	// When the player first joins the server, they
	pPlayer->m_iNumSpawns = 0;
	pPlayer->m_lifeState = LIFE_DEAD;
	pPlayer->m_fEffects |= EF_NODRAW;
	pPlayer->ChangeTeam( TEAM_UNASSIGNED );
	pPlayer->SetThink( NULL );
	pPlayer->AddAccount( startmoney.GetInt() );

	// Move them to the first intro camera.
	pPlayer->MoveToNextIntroCamera();

	char sName[128];
	Q_strncpy( sName, STRING(pPlayer->pl.netname), sizeof( sName ) );
	
	// First parse the name and remove any %'s
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		// Replace it with a space
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );


}

/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "CounterStrike";
}


//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	// Materials used by the client effects
	engine->PrecacheModel( "sprites/white.vmt" );
	engine->PrecacheModel( "sprites/physbeam.vmt" );
	engine->PrecacheModel( "effects/human_object_glow.vmt" );
}


// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			dynamic_cast< CBasePlayer* >( pEdict )->CreateCorpse();
		}

		// respawn player
		pEdict->Spawn();
	}
	else
	{       // restart the entire server
		engine->ServerCommand("reload\n");
	}
}

void GameStartFrame( void )
{
	VPROF("GameStartFrame()");

	if ( g_pGameRules )
		g_pGameRules->Think();

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = teamplay.GetInt() ? true : false;

	// Temp bot code from TF until I get the real deal in.
	extern void Bot_RunAll();
	Bot_RunAll();
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
CGameRules *InstallGameRules( void )
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "player_manager", vec3_origin, vec3_angle );

	// Teamplay
	CCSGameRules *pEnt = dynamic_cast< CCSGameRules* >( CreateEntityByName( "cs_gamerules" ) );
	if ( !pEnt )
		Error( "InstallGameRules: pEnt == NULL" );

	return pEnt;
}
