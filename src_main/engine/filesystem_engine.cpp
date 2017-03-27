//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "quakedef.h" // for MAX_OSPATH
#include <stdlib.h>
#include <assert.h>
#include <malloc.h>
#include "filesystem.h"
#include "tgawriter.h"

IFileSystem *g_pFileSystem;
CreateInterfaceFn g_FileSystemFactory = 0;

void FileSystem_Path_f( void )
{
	if( g_pFileSystem )
	{
		g_pFileSystem->PrintSearchPaths();
	}
}

static ConCommand path( "path", FileSystem_Path_f );

void FileSystem_Init( CreateInterfaceFn fileSystemFactory )
{
	g_FileSystemFactory = fileSystemFactory;

	TGAWriter::SetFileSystem( g_FileSystemFactory );
	g_pFileSystem = ( IFileSystem * )g_FileSystemFactory( FILESYSTEM_INTERFACE_VERSION, NULL );
	assert( g_pFileSystem );
	g_pFileSystem->SetWarningFunc( Warning );
}

void FileSystem_Warning_Level( void )
{
	if( Cmd_Argc() != 2 )
	{
		Warning( "\"fs_warning_level n\" where n is one of:\n" );
		Warning( "\t0:\tFILESYSTEM_WARNING_QUIET\n" );
		Warning( "\t1:\tFILESYSTEM_WARNING_REPORTUNCLOSED\n" );
		Warning( "\t2:\tFILESYSTEM_WARNING_REPORTUSAGE\n" );
		Warning( "\t3:\tFILESYSTEM_WARNING_REPORTALLACCESSES\n" );
		Warning( "\t4:\tFILESYSTEM_WARNING_REPORTALLACCESSES_READ\n" );
		Warning( "\t5:\tFILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE\n" );
		return;
	}

	int level = atoi( Cmd_Argv( 1 ) );
	switch( level )
	{
	case FILESYSTEM_WARNING_QUIET:
		Warning( "fs_warning_level = FILESYSTEM_WARNING_QUIET\n" );
		break;
	case FILESYSTEM_WARNING_REPORTUNCLOSED:
		Warning( "fs_warning_level = FILESYSTEM_WARNING_REPORTUNCLOSED\n" );
		break;
	case FILESYSTEM_WARNING_REPORTUSAGE:
		Warning( "fs_warning_level = FILESYSTEM_WARNING_REPORTUSAGE\n" );
		break;
	case FILESYSTEM_WARNING_REPORTALLACCESSES:
		Warning( "fs_warning_level = FILESYSTEM_WARNING_REPORTALLACCESSES\n" );
		break;
	case FILESYSTEM_WARNING_REPORTALLACCESSES_READ:
		Warning( "fs_warning_level = FILESYSTEM_WARNING_REPORTALLACCESSES_READ\n" );
		break;
	case FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE:
		Warning( "fs_warning_level = FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE\n" );
		break;

	default:
		Warning( "fs_warning_level = UNKNOWN!!!!!!!\n" );
		return;
		break;
	}
	g_pFileSystem->SetWarningLevel( ( FileWarningLevel_t )level );
}

static ConCommand fs_warning_level( "fs_warning_level", FileSystem_Warning_Level );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FileSystem_Shutdown( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Wrap Sys_LoadModule() with a filesystem GetLocalCopy() call to
//			ensure have the file to load when running Steam.
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

