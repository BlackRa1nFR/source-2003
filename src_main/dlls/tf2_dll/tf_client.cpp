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
#include "teamplay_gamerules.h"
#include "tf_gamerules.h"
#include "EntityList.h"
#include "physics.h"
#include "game.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "tf_player.h"
#include "menu_base.h"
#include "shake.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void ClientKill( edict_t *pEdict );
void Host_Say( edict_t *pEdict, int teamonly );
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
void InitializeMenus( void );
void DestroyMenus( void );
void Bot_RunAll( void );

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
	CBaseTFPlayer *pPlayer = CBaseTFPlayer::CreatePlayer( "player", pEdict );
	pPlayer->InitialSpawn();
	pPlayer->PlayerData()->netname = AllocPooledString( playername );
	pPlayer->Spawn();
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
		return "TeamFortress 2";
}

extern int g_iWeaponCheat;

//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	// Materials used by the client effects
	engine->PrecacheModel( "sprites/white.vmt" );
	engine->PrecacheModel( "sprites/physbeam.vmt" );
	engine->PrecacheModel( "effects/human_object_glow.vmt" );

	// Precache player models
	engine->PrecacheModel( "models/player/alien_commando.mdl" );
	engine->PrecacheModel( "models/player/human_commando.mdl" );
	engine->PrecacheModel( "models/player/human_defender.mdl" );
	engine->PrecacheModel( "models/player/medic.mdl" );
	engine->PrecacheModel( "models/player/recon.mdl" );
	engine->PrecacheModel( "models/player/human_medic.mdl" );
	engine->PrecacheModel( "models/player/alien_escort.mdl" );
	engine->PrecacheModel( "models/player/defender.mdl" );
	engine->PrecacheModel( "models/player/technician.mdl" );

	// Precache team message sounds
	enginesound->PrecacheSound( "vox/reinforcement.wav" );
	enginesound->PrecacheSound( "vox/harvester-attack.wav" );
	enginesound->PrecacheSound( "vox/harvester-destroyed.wav" );
	enginesound->PrecacheSound( "vox/new-tech-level.wav" );
	enginesound->PrecacheSound( "vox/resource-zone-emptied.wav" );
}


// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			((CBaseTFPlayer *)pEdict)->CreateCorpse();
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

	Bot_RunAll();
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
CGameRules *InstallGameRules( void )
{
	InitializeMenus();

	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "tf_player_manager", vec3_origin, vec3_angle );

	// teamplay
	CTeamFortress *pEnt = dynamic_cast< CTeamFortress* >( CreateEntityByName( "team_fortress_gamerules" ) );
	if ( !pEnt )
		Error( "InstallGameRules: pEnt == NULL" );

	return pEnt;
}
