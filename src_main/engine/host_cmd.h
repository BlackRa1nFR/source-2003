//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( HOST_CMD_H )
#define HOST_CMD_H
#ifdef _WIN32
#pragma once
#endif

#include "savegame_version.h"
#include "host_saverestore.h"

// The launcher includes this file, too
#ifndef LAUNCHERONLY

void Host_Init( void );
void Host_Shutdown(void);
int  Host_Frame (float time, int iState );
void Host_ShutdownServer(void);
bool Host_NewGame( char *mapName, bool loadGame );
void Host_Changelevel( bool loadfromsavedgame, const char *mapname, const char *start );

extern int  gHostSpawnCount;

#endif // LAUNCHERONLY
#endif // HOST_CMD_H