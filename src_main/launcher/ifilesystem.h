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
// $NoKeywords: $
//=============================================================================
#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"


class IFileSystem;

//-----------------------------------------------------------------------------
// Loads, unloads the file system DLL
//-----------------------------------------------------------------------------
bool FileSystem_LoadFileSystemModule( void );
void FileSystem_UnloadFileSystemModule( void );

CSysModule * FileSystem_LoadModule( const char* path );
void FileSystem_UnloadModule( CSysModule *pModule );

bool FileSystem_Init( );
void FileSystem_Shutdown( void );

CreateInterfaceFn GetFileSystemFactory();

// Sets the file system search path based on the game directory
bool FileSystem_SetGameDirectory( char const* pGameDir );

extern IFileSystem *g_pFileSystem;

#endif // IFILESYSTEM_H