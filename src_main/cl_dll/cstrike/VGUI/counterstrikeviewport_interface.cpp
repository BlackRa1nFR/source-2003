//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Client DLL interface routines for client dll library
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#pragma warning( disable : 4800  )  // disable forcing int to bool performance warning

#include <cl_dll/IVGuiClientDll.h>
 
// client dll/engine defines
#include "hud.h"
#include "cs_gamerules.h" 
#include "voice_status.h"
#include "ivrenderview.h"
#include "gamevars_shared.h"
//#include "cl_util.h" // COM_* functions
//#include "demo.h"
//#include "demo_api.h" // demo_api_s

//-----------------------------------------------------------------------------
// Purpose: various wrapper functions for the interface 
//-----------------------------------------------------------------------------
char *COM_ParseFile(char *data, char *token)
{
	return engine->COM_ParseFile( data, token );
}

void COM_FileBase ( char *in, char *out)
{
	engine->COM_FileBase( const_cast<const char *>(in), out );
}

int InIntermission() { return CSGameRules()->IsIntermission(); }
int PipInsetOff() { return 0;/*(gHUD.m_Spectator.m_pip->value==INSET_OFF);*/ }

int	SpectatorNumber() { return 0;/* return gHUD.m_Spectator.m_iSpectatorNumber;*/ }
float HudTime() { return gpGlobals->curtime; }
int	TeamPlay() { return 1; }
void FindNextPlayer( bool reverse ) { reverse?engine->ServerCmd("specprev"):engine->ServerCmd("specnext"); }
void FindPlayer(const char *name) { /*gHUD.m_Spectator.FindPlayer(name);*/ }
void CheckSettings() { /*gHUD.m_Spectator.CheckSettings();*/ }

void MessageAdd(void *msg) 
{ 
	//MIKETODO
	//gHUD.m_Message.MessageAdd( (client_textmessage_t *)msg);
}

void MessageHud( const char *pszName, int iSize, void *pbuf ) 
{ 
	//MIKETODO
	//gHUD.m_TextMessage.MsgFunc_TextMsg( pszName, iSize, pbuf ); 
}

const ConVar * FindCVar ( const char * cvarname ) { return cvar->FindVar( cvarname ); }

void ClientCmdFunc( const char *cmd ) { engine->ClientCmd( const_cast<char *>(cmd) ); }

int TeamNumber() 
{ 
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	
	if ( pPlayer )
		return pPlayer->GetTeamNumber(); 
	else
		return TEAM_UNASSIGNED;
}

const char *GetLevelName() { return engine->GetLevelName(); }
int GetMaxPlayers() { return engine->GetMaxClients(); }
int GetMaxTeams() { return TEAM_MAXCOUNT; }
bool IsHLTVMode() { return false; /*return engine->is.IsSpectateOnly();*/ } //TODO
bool IsSpectatingAllowed() { return (allow_spectators.GetInt() != 0); }

bool IsSpectator()
{
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player )
		return (player->m_iObserverMode != OBS_MODE_NONE);
	else
		return false;	// game not started yet
}

int  GetSpectatorMode()
{
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player )
		return player->m_iObserverMode;
	else
		return  OBS_MODE_NONE;	// game not started yet
}

int  GetSpectatorTarget()
{
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player )
		return player->m_hObserverTarget.Get()->entindex();
	else
		return  0;	// game not started yet
}

int  GetLocalPlayerIndex()
{
	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player )
		return player->entindex();
	else
		return  0;	// game not started yet
}

void StopSquelch() { GetClientVoiceMgr()->StopSquelchMode(); }
bool IsDemoPlayingBack() { return render->IsPlayingDemo(); }

VGuiLibraryPlayer_t GetPlayerInfoFunc(int index) 
{
	VGuiLibraryPlayer_t player;

	//MIKETODO: scoreboard
	memset( &player, 0, sizeof( player ) );
	/*
	GetPlayerInfo( index, &g_PlayerInfoList[index] ); // make sure the structure is up to date

	player.name = g_PlayerInfoList[index].name;
	player.ping = g_PlayerInfoList[index].ping;
	player.packetloss = g_PlayerInfoList[index].packetloss;
	player.thisplayer = (g_PlayerInfoList[index].thisplayer!=0);
	player.dead = (g_PlayerExtraInfo[index].dead!=0);
	
	player.teamname = g_PlayerExtraInfo[index].teamname;
	player.teamnumber = g_PlayerExtraInfo[index].teamnumber;
	player.frags = g_PlayerExtraInfo[index].frags;
	player.deaths = g_PlayerExtraInfo[index].deaths;
	player.playerclass = g_PlayerExtraInfo[index].playerclass;
	player.health = g_PlayerExtraInfo[index].health;
	*/

	return player;
}



//-----------------------------------------------------------------------------
// Purpose: the interface class that passes functions to the client library
//-----------------------------------------------------------------------------
VGuiLibraryInterface_t clientInterface = 
{
	InIntermission,
	FindPlayer,
	FindNextPlayer,
	PipInsetOff,
	CheckSettings,
	SpectatorNumber,
	IsHLTVMode,
	IsSpectator,
	GetSpectatorMode,
	GetSpectatorTarget,
	IsSpectatingAllowed,
	HudTime,
	MessageAdd,
	MessageHud,
	TeamPlay,
	ClientCmdFunc,
	FindCVar,
	TeamNumber,
	GetLevelName,
	GetLocalPlayerIndex,
	COM_ParseFile,
	COM_FileBase,
	GetPlayerInfoFunc,
	GetMaxPlayers,
	StopSquelch,
	IsDemoPlayingBack,
};

