//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FILESYSTEM_ENGINE_H
#define FILESYSTEM_ENGINE_H
#ifdef _WIN32
#pragma once
#endif


#include "interface.h"


class IFileSystem;

extern IFileSystem *g_pFileSystem;

void FileSystem_Init( CreateInterfaceFn fileSystemFactory );
void FileSystem_Shutdown( void );
CSysModule *FileSystem_LoadModule(const char *path);
void FileSystem_UnloadModule(CSysModule *pModule);

#endif // FILESYSTEM_ENGINE_H
