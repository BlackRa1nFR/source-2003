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
#include <windows.h>
#include "tier0/dbg.h"
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include "FileSystem.h"
#include "Interface.h"
#include "vstdlib/icommandline.h"
#include <string.h>
#include <malloc.h>

IFileSystem *g_pFileSystem;

static CSysModule *g_pFileSystemModule = 0;
CreateInterfaceFn g_FileSystemFactory = 0;

CreateInterfaceFn GetFileSystemFactory()
{
	return g_FileSystemFactory;
}


//-----------------------------------------------------------------------------
// Loads, unloads the file system DLL
//-----------------------------------------------------------------------------
bool FileSystem_LoadFileSystemModule( void )
{
	char *sFileSystemModuleName = "filesystem_stdio.dll";

	// There are two command line parameters for Steam:
	//		1) -steam (runs Steam in remote filesystem mode; requires Steam backend)
	//		2) -steamlocal (runs Steam in local filesystem mode (all content off HDD)
	if ( CommandLine()->CheckParm("-steam") || CommandLine()->CheckParm("-steamlocal") )
	{
		sFileSystemModuleName = "filesystem_steam.dll";
	}

	g_pFileSystemModule = Sys_LoadModule( sFileSystemModuleName );
	Assert( g_pFileSystemModule );
	if( !g_pFileSystemModule )
	{
		Error( "Unable to load %s", sFileSystemModuleName );
		return false;
	}

	g_FileSystemFactory = Sys_GetFactory( g_pFileSystemModule );
	if( !g_FileSystemFactory )
	{
		Error( "Unable to get filesystem factory" );
		return false;
	}
	g_pFileSystem = ( IFileSystem * )g_FileSystemFactory( FILESYSTEM_INTERFACE_VERSION, NULL );
	Assert( g_pFileSystem );
	if( !g_pFileSystem )
	{
		Error( "Unable to get IFileSystem interface from filesystem factory" );
		return false;
	}
	return true;
}

void FileSystem_UnloadFileSystemModule( void )
{
	if ( !g_pFileSystemModule )
		return;

	g_FileSystemFactory = NULL;

	Sys_UnloadModule( g_pFileSystemModule );
	g_pFileSystemModule = 0;
}

//-----------------------------------------------------------------------------
// Loads, unloads a DLL ... wrapper around Sys_LoadModule() to ensure a local
//			copy of the DLL is present incase using the Steam FS
//-----------------------------------------------------------------------------
CSysModule *FileSystem_LoadModule(const char *path)
{
	if ( g_pFileSystem )
		return g_pFileSystem->LoadModule( path );
	else
		return Sys_LoadModule(path);
}

//-----------------------------------------------------------------------------
// Purpose: Provided for symmetry sake with FileSystem_LoadModule()...
//-----------------------------------------------------------------------------
void FileSystem_UnloadModule(CSysModule *pModule)
{
	Sys_UnloadModule(pModule);
}


//-----------------------------------------------------------------------------
// Initialize, shutdown the file system 
//-----------------------------------------------------------------------------
bool FileSystem_Init( )
{
	if ( g_pFileSystem->Init() != INIT_OK )
		return false;

	return true;
}


void FileSystem_Shutdown( void )
{
	g_pFileSystem->Shutdown();
}
