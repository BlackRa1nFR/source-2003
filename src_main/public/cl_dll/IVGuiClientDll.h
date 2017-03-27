//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Client DLL VGUI2 Viewport
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef VGUICLIENTDLLINTERFACE_H
#define VGUICLIENTDLLINTERFACE_H

#ifdef _WIN32
#pragma once
#endif

typedef struct {
	const char *name;
	int ping;
	int packetloss;
	bool thisplayer;
	const char *teamname;
	int teamnumber;
	int frags;
	int deaths;
	int playerclass;
	int health;
	bool dead;
} VGuiLibraryPlayer_t;

typedef struct {
	const char *name;
	int frags;
	int deaths;
	int ping;
	int players;
	int packetloss;
	int teamnumber;
} VGuiLibraryTeamInfo_t;

class ConVar;

typedef int (*ININTERMISSION)();
typedef void (*SPECTATOR_FINDNEXTPLAYER)( bool reverse );
typedef void (*SPECTATOR_FINDPLAYER)( const char *playerName );
typedef int  (*SPECTATOR_PIPINSETOFF)();
typedef void (*SPECTATOR_CHECKSETTINGS)();
typedef int (*SPECTATOR_NUMBER)();
typedef float (*HUDTIME)();
typedef void (*MESSAGE_ADD)( void * newMessage );
typedef void (*MESSAGE_HUD)( const char *pszName, int iSize, void *pbuf );
typedef int (*TEAMPLAY)();
typedef void (*CLIENTCMD)( const char *cmd );
typedef const ConVar * (*FINDCVAR)( const char *cvarname );
typedef int (*TEAMNUMBER)();
typedef const char *(*GETLEVELNAME)();
typedef char * (*COMPARSEFILE)(char *data, char *token);
typedef void (*COMFILEBASE)( char *in, char *out );

typedef VGuiLibraryPlayer_t (*GETPLAYERINFO)(int index);
typedef int (*GETMAXPLAYERS)();
typedef bool (*ISHLTVMODE)();
typedef bool (*ISSPECTATOR)();
typedef int	(*GETSPECTATORMODE)();
typedef int	(*GETSPECTATORTARGET)();
typedef bool (*ISSPECTATINGALLOWED)();
typedef int (*GETLOCALPLAYERINDEX)();
typedef void (*VOICESTOPSQUELCH)();
typedef bool (*DEMOPLAYBACK)();

typedef struct {
	ININTERMISSION					InIntermission;
	
	SPECTATOR_FINDPLAYER			FindPlayer;
	SPECTATOR_FINDNEXTPLAYER		FindNextPlayer;
	SPECTATOR_PIPINSETOFF			PipInsetOff;
	SPECTATOR_CHECKSETTINGS			CheckSettings;
	SPECTATOR_NUMBER				SpectatorNumber;

	ISHLTVMODE			IsHLTVMode;
	ISSPECTATOR			IsSpectator;
	GETSPECTATORMODE	GetSpectatorMode;
	GETSPECTATORTARGET	GetSpectatorTarget;
	ISSPECTATINGALLOWED	IsSpectatingAllowed;
	

	HUDTIME HudTime;
	MESSAGE_ADD MessageAdd;
	MESSAGE_HUD MessageHud;
	TEAMPLAY TeamPlay;

	CLIENTCMD ClientCmd;
	FINDCVAR FindCVar;
	TEAMNUMBER TeamNumber;
	GETLEVELNAME GetLevelName;
	GETLOCALPLAYERINDEX GetLocalPlayerIndex;

	COMPARSEFILE COM_ParseFile;
	COMFILEBASE	COM_FileBase;

	GETPLAYERINFO GetPlayerInfo;
	GETMAXPLAYERS GetMaxPlayers;

	VOICESTOPSQUELCH GameVoice_StopSquelchMode;
	DEMOPLAYBACK IsDemoPlayingBack;


} VGuiLibraryInterface_t; 

#endif //VGUICLIENTDLLINTERFACE_H