//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include <sys/stat.h>
#include "vstdlib/strtools.h"
#include "filesystem_tools.h"


// ---------------------------------------------------------------------------------------------------- //
// CBaseStdioFileSystem implementation.
// ---------------------------------------------------------------------------------------------------- //

FileHandle_t CBaseStdioFileSystem::Open( const char *pFileName, const char *pOptions, const char *pathID )
{
	return (FileHandle_t)fopen( pFileName, pOptions );
}


void CBaseStdioFileSystem::Close( FileHandle_t file )
{
	if ( file )
		fclose( (FILE*)file );
}


void CBaseStdioFileSystem::Seek( FileHandle_t file, int pos, FileSystemSeek_t seekType )
{
	if ( file )
	{
		if ( seekType == FILESYSTEM_SEEK_HEAD )
			fseek( (FILE*)file, pos, SEEK_SET );
		else if ( seekType == FILESYSTEM_SEEK_CURRENT )
			fseek( (FILE*)file, pos, SEEK_CUR );
		else
			fseek( (FILE*)file, pos, SEEK_END );
	}
}


unsigned int CBaseStdioFileSystem::Tell( FileHandle_t file )
{
	return ftell( (FILE*)file );
}


unsigned int CBaseStdioFileSystem::Size( FileHandle_t file )
{
	long cur = ftell( (FILE*)file );
	fseek( (FILE*)file, 0, SEEK_END );
	long size = ftell( (FILE*)file );
	fseek( (FILE*)file, cur, SEEK_SET );
	return size;
}

unsigned int CBaseStdioFileSystem::Size( const char *pFilename, const char *pPathID )
{
	struct	_stat buf;
	int sr = _stat( pFilename, &buf );
	if ( sr == -1 )
	{
		return 0;
	}

	return buf.st_size;
}


void CBaseStdioFileSystem::Flush( FileHandle_t file )
{
	fflush( (FILE*)file );
}

bool CBaseStdioFileSystem::Precache( const char *pFileName, const char *pPathID )
{
	// Really simple, just open, the file, read it all in and close it. 
	// We probably want to use file mapping to do this eventually.
	FILE* f = fopen(pFileName,"rt");
	if( !f )
		return false;


	char buffer[16384];
	while( sizeof(buffer) == fread(buffer,1,sizeof(buffer),f) )
		;

	fclose(f);

	return true;
}

long CBaseStdioFileSystem::GetFileTime( const char *pFilename, const char *pPathID )
{
	struct	_stat buf;
	int sr = _stat( pFilename, &buf );
	if ( sr == -1 )
	{
		return 0;
	}

	return buf.st_mtime;
}


int CBaseStdioFileSystem::Read( void* pOutput, int size, FileHandle_t file )
{
	return fread( pOutput, 1, size, (FILE*)file );
}


int CBaseStdioFileSystem::Write( void const* pInput, int size, FileHandle_t file )
{
	return fwrite( pInput, 1, size, (FILE*)file );
}


bool CBaseStdioFileSystem::FileExists( const char *pFileName, const char *pPathID )
{
	FileHandle_t hFile = Open( pFileName, "rb" );
	if ( hFile )
	{
		Close( hFile );
		return true;
	}
	else
	{
		return false;
	}
}

bool CBaseStdioFileSystem::IsFileWritable( const char *pFileName, const char *pPathID )
{
	struct	_stat buf;
	int sr = _stat( pFileName, &buf );
	if ( sr == -1 )
	{
		return false;
	}

	if( buf.st_mode & _S_IWRITE )
	{
		return true;
	}

	return false;
}





// ---------------------------------------------------------------------------------------------------- //
// Module interface.
// ---------------------------------------------------------------------------------------------------- //

static CBaseStdioFileSystem g_BaseStdioFileSystem;
IBaseFileSystem *g_pFileSystem = &g_BaseStdioFileSystem;

// These are only used for tools that need the search paths that the engine's file system provides.
IFileSystem			*g_pFullFileSystem = NULL;
CSysModule			*g_pFullFileSystemModule = NULL;
CreateInterfaceFn	g_FullFileSystemFactory = NULL;


void* FileSystemToolsFactory( const char *pName, int *pReturnCode )
{
	if ( Q_stricmp( pName, BASEFILESYSTEM_INTERFACE_VERSION ) == 0 )
		return &g_BaseStdioFileSystem;
	else
		return NULL;
}


CreateInterfaceFn FileSystem_GetFactory( void )
{
	if ( g_pFullFileSystemModule )
		return g_FullFileSystemFactory;
	else
		return FileSystemToolsFactory;
}


bool FileSystem_Init( bool bUseEngineFileSystem )
{
	if ( bUseEngineFileSystem )
	{
		// Ok, use the engine's file system.
		g_pFullFileSystemModule = Sys_LoadModule( "filesystem_stdio" );
		if ( !g_pFullFileSystemModule )
			return false;

		g_FullFileSystemFactory = Sys_GetFactory( g_pFullFileSystemModule );
		if ( !g_FullFileSystemFactory )
		{
			FileSystem_Term();
			return false;
		}
	
		g_pFileSystem = g_pFullFileSystem = (IFileSystem*)g_FullFileSystemFactory( FILESYSTEM_INTERFACE_VERSION, NULL );
		if ( !g_pFileSystem )
		{
			FileSystem_Term();
			return false;
		}

		if ( g_pFullFileSystem->Init() != INIT_OK )
			return false;
	}

	return true;
}


void FileSystem_Term()
{
	if ( g_pFullFileSystem )
	{
		g_pFullFileSystem->Shutdown();
		g_pFullFileSystem = NULL;
		g_pFileSystem = NULL;
	}

	if ( g_pFullFileSystemModule )
	{
		Sys_UnloadModule( g_pFullFileSystemModule );
		g_pFullFileSystemModule = NULL;
	}
	g_FullFileSystemFactory = NULL;
}

