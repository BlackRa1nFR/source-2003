//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FILESYSTEM_WC_H
#define FILESYSTEM_WC_H
#ifdef _WIN32
#pragma once
#endif

class IFileSystem;

bool FileSystem_Init( const char *pModDir, const char *pGameDir );
void FileSystem_Shutdown( );

// must call FileSystem_Init before calling this.
CreateInterfaceFn FileSystem_GetFactory( void );

extern IFileSystem *g_pFileSystem;

#endif // FILESYSTEM_WC_H
