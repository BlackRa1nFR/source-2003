//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef HOST_STATE_H
#define HOST_STATE_H
#ifdef _WIN32
#pragma once
#endif

void	HostState_Init();
void	HostState_RunGameInit();
int		HostState_Frame( float time, int iState );
void	HostState_NewGame( char const *pMapName );
void	HostState_LoadGame( char const *pSaveFileName );
void	HostState_ChangeLevelSP( char const *pNewLevel, char const *pLandmarkName );
void	HostState_ChangeLevelMP( char const *pNewLevel, char const *pLandmarkName );
void	HostState_GameShutdown();
void	HostState_Shutdown();
void	HostState_Restart();
bool	HostState_IsGameShuttingDown();

#endif // HOST_STATE_H
