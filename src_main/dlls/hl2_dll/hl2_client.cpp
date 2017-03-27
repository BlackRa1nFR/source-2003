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
#include "hl2_player.h"
#include "hl2_gamerules.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "EntityList.h"
#include "physics.h"
#include "game.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"

#include "tier0/vprof.h"

void ClientKill( edict_t *pEdict );
void Host_Say( edict_t *pEdict, int teamonly );

extern CBaseEntity*	FindPickerEntityClass( CBasePlayer *pPlayer, char *classname );
extern bool			g_fGameOver;

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBasePlayer for pev, and call spawn
	CHL2_Player *pPlayer = CHL2_Player::CreatePlayer( "player", pEdict );
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
		return "Half-Life 2";
}

extern int g_iWeaponCheat;


//-----------------------------------------------------------------------------
// Purpose: Given a player and optional name returns the entity of that 
//			classname that the player is nearest facing
//			
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity* FindEntity( edict_t *pEdict, char *classname)
{

	// If no name was given set bits based on the picked
	if (FStrEq(classname,"")) 
	{
		return (FindPickerEntityClass( (CBasePlayer*)pEdict->m_pEnt, classname ));
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	engine->PrecacheModel("models/player.mdl");
	engine->PrecacheModel( "models/gibs/metalgibs.mdl");
	engine->PrecacheModel( "models/gibs/agibs.mdl" );
}


// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			((CHL2_Player *)pEdict)->CreateCorpse();
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

	gpGlobals->teamplay = (teamplay.GetInt() != 0);
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
CGameRules *InstallGameRules( void )
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "player_manager", vec3_origin, vec3_angle );

	if ( !gpGlobals->deathmatch )
	{
		// generic half-life
		CHalfLife2 *pEnt = dynamic_cast< CHalfLife2* >( CreateEntityByName( "hl2_gamerules" ) );
		if ( !pEnt )
			Error( "InstallGameRules: pEnt == NULL" );
		
		return pEnt;
	}
	else
	{
		if ( teamplay.GetInt() > 0 )
		{
			// teamplay
			CTeamplayRules *pEnt = dynamic_cast< CTeamplayRules* >( CreateEntityByName( "teamplay_gamerules" ) );
			if ( !pEnt )
				Error( "InstallGameRules: pEnt == NULL" );

			return pEnt;
		}
		else
		{
			// vanilla deathmatch
			CMultiplayRules *pEnt = dynamic_cast< CMultiplayRules* >( CreateEntityByName( "multiplay_gamerules" ) );
			if ( !pEnt )
				Error( "InstallGameRules: pEnt == NULL" );

			return pEnt;
		}
	}
}

