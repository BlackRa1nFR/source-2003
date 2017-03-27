//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "FileSystem.h"
#include "tier0/dbg.h"


IFileSystem *g_pFileSystem;

static CreateInterfaceFn g_pFileSystemFactory = NULL;

static void FixSlashes( char *pname )
{
	while ( *pname ) {
		if ( *pname == '/' )
			*pname = '\\';
		pname++;
	}
}

// strip off the last directory from dirName
static bool StripLastDir( char *dirName )
{
	int len = strlen( dirName );

	// skip trailing slash
	if ( dirName[len-1] == '/' || dirName[len-1] == '\\' )
		len--;

	while ( len > 0 )
	{
		if ( dirName[len-1] == '/' || dirName[len-1] == '\\' )
		{
			dirName[len] = 0;
			FixSlashes( dirName );
			return true;
		}
		len--;
	}

	return false;
}

static bool FileSystem_LoadDLL( const char *basegamedir )
{
	CSysModule *pModule;
	pModule = Sys_LoadModule( "filesystem_stdio" );
	Assert( pModule );
	if( !pModule )
	{
		return false;
	}

	g_pFileSystemFactory = Sys_GetFactory( "filesystem_stdio" );
	if( !g_pFileSystemFactory )
	{
		return false;
	}
	g_pFileSystem = ( IFileSystem * )g_pFileSystemFactory( FILESYSTEM_INTERFACE_VERSION, NULL );
	Assert( g_pFileSystem );
	if( !g_pFileSystem )
	{
		return false;
	}
	return true;
}

CreateInterfaceFn FileSystem_GetFactory( void )
{
	return g_pFileSystemFactory;
}

bool FileSystem_Init( const char *pModDir, const char *pGameDir )
{
	if( !pGameDir || *pGameDir == '\0' )
		return false;
	
	if( !FileSystem_LoadDLL( pGameDir ) )
		return false;

	if ( g_pFileSystem->Init() != INIT_OK )
		return false;

	g_pFileSystem->AddSearchPath( pGameDir, "GAME", PATH_ADD_TO_HEAD );
	if( pModDir && *pModDir != '\0' )
	{
		g_pFileSystem->AddSearchPath( pModDir, "GAME", PATH_ADD_TO_HEAD );
	}

	return true;
}


void FileSystem_Shutdown( )
{
	if (g_pFileSystem)
	{
		g_pFileSystem->Shutdown();
		g_pFileSystem = NULL;
	}

	g_pFileSystemFactory = NULL;
}
