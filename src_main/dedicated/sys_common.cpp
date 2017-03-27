//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifdef _WIN32
#include <windows.h> 
#endif
#include <stdio.h>
#include <stdlib.h>
#include "isys.h"
#include "dedicated.h"
#include "checksum_md5.h"
#include "dll_state.h"
#include "engine_hlds_api.h"
#include "filesystem.h"
#include "ifilesystem.h"
#include "vstdlib/strtools.h"
#include "vstdlib/ICommandLine.h"


static long		hDLLThirdParty	= 0L;


/*
==============
Load3rdParty

Load support for third party .dlls ( gamehost )
==============
*/
void Load3rdParty( void )
{
	// Only do this if the server operator wants the support.
	// ( In case of malicious code, too )
	if ( CommandLine()->CheckParm( "-usegh" ) )   
	{
		hDLLThirdParty = sys->LoadLibrary( "ghostinj.dll" );
	}
}

/*
============
StripExtension
 
Strips the extension off a filename.  Works backward to insure stripping
of extensions only, and not parts of the path that might contain a
period (i.e. './hlds_run').
============
*/
void StripExtension (char *in, char *out)
{
    char * in_current  = in + strlen(in);
    char * out_current = out + strlen(in);
    int    found_extension = 0;
	
    while (in_current >= in) {
		if ((found_extension == 0) && (*in_current == '.')) {
			*out_current = 0;
			found_extension = 1;
		}
		else {
			if ((*in_current == '/') || (*in_current == '\\'))
				found_extension = 1;
			
			*out_current = *in_current;
		}
		
		in_current--;
		out_current--;
    }
}

bool MD5_Hash_File(unsigned char digest[16], char *pszFileName )
{
	FileHandle_t fp = FILESYSTEM_INVALID_HANDLE;
	unsigned char chunk[1024];
	int nBytesRead;
	MD5Context_t ctx;
	
	int nSize;

	fp = g_pFileSystem->Open( pszFileName, "rb" );
	if ( fp == FILESYSTEM_INVALID_HANDLE )
		return false;

	nSize = g_pFileSystem->Size( fp );

	if ( nSize <= 0 )
	{
		g_pFileSystem->Close( fp );
		return false;
	}

	memset(&ctx, 0, sizeof(MD5Context_t));

	MD5Init(&ctx);

	// Now read in 1K chunks
	while (nSize > 0)
	{
		if (nSize > 1024)
			nBytesRead = g_pFileSystem->Read(chunk, 1024, fp);
		else
			nBytesRead = g_pFileSystem->Read(chunk, nSize, fp);

		// If any data was received, CRC it.
		if (nBytesRead > 0)
		{
			nSize -= nBytesRead;
			MD5Update(&ctx, chunk, nBytesRead);
		}

		// We we are end of file, break loop and return
		if ( g_pFileSystem->EndOfFile( fp ) )
		{
			g_pFileSystem->Close( fp );
			fp = FILESYSTEM_INVALID_HANDLE;
			break;
		}
		// If there was a disk error, indicate failure.
		else if ( !g_pFileSystem->IsOk(fp) )
		{
			if ( fp != FILESYSTEM_INVALID_HANDLE )
				g_pFileSystem->Close(fp);
			return false;
		}
	}	

	if ( fp != FILESYSTEM_INVALID_HANDLE )
		g_pFileSystem->Close(fp);

	MD5Final(digest, &ctx);

	return true;
}

/*
==============
CheckExeChecksum

Simple self-crc check
==============
*/
int CheckExeChecksum( void )
{
	unsigned char g_MD5[16];
	char szFileName[ 256 ];
	unsigned int newdat = 0;	
	unsigned int olddat;
	char datfile[ 256 ];

	// Get our filename
	if ( !sys->GetExecutableName( szFileName ) )
	{
		return 0;
	}

	// compute raw 16 byte hash value
	if ( !MD5_Hash_File( g_MD5, szFileName ) )
	{
		return 0;
	}

	StripExtension( szFileName, datfile );

	strcat( datfile, ".dat" );

	// Check .dat file ( or write a new one if running with -newdat )
	FileHandle_t fp = g_pFileSystem->Open( datfile, "rb" );
	if ( fp == FILESYSTEM_INVALID_HANDLE || CommandLine()->CheckParm( "-newdat" ) )  // No existing file, or we are asked to create a new one
	{
		if ( fp != FILESYSTEM_INVALID_HANDLE )
		{
			g_pFileSystem->Close( fp );
		}

		newdat = *(unsigned int *)&g_MD5[0];
		fp = g_pFileSystem->Open( datfile, "wb" );
		if ( fp != FILESYSTEM_INVALID_HANDLE )
		{
			g_pFileSystem->Write( &newdat, sizeof( unsigned int ), fp );
			g_pFileSystem->Close( fp );
		}
	}
	else
	{
		int bOk = 0;

		if ( g_pFileSystem->Read( &newdat, sizeof( unsigned int ), fp ) == sizeof( unsigned int ) )
			bOk = 1;

		g_pFileSystem->Close( fp );

		if ( bOk )
		{
			olddat = *(unsigned int *)&g_MD5[0];
			if ( olddat != newdat )
			{
				const char *pmsg = "Your Half-Life executable appears to have been modified.  Please check your system for viruses and then re-install Half-Life.";

#ifdef _WIN32
				MessageBox( NULL, pmsg, "Half-Life Dedicated Server", MB_OK );
#else
				printf( "%s\n", pmsg );
#endif
				return 0;
			}
		}
	}

	return 1;
}

/*
==============
EF_VID_ForceUnlockedAndReturnState

Dummy funcion called by engine
==============
*/
int  EF_VID_ForceUnlockedAndReturnState(void)
{
	return 0;
}

/*
==============
EF_VID_ForceLockState

Dummy funcion called by engine
==============
*/
void EF_VID_ForceLockState(int)
{
}

/*
==============
InitInstance

==============
*/
bool InitInstance( )
{
	// Start up the file system
	if (!FileSystem_Init( ))
		return false;

	Load3rdParty();

#if !defined( _DEBUG )
	if ( !CheckExeChecksum() )
		return false;
#endif

	return true;
}

/*
==============
ProcessConsoleInput

==============
*/
void ProcessConsoleInput( void )
{
	char *s;

	if ( !engine )
		return;

	do
	{
		s = sys->ConsoleInput();
		if (s)
		{
			char szBuf[ 256 ];
			sprintf( szBuf, "%s\n", s );
			engine->AddConsoleText ( szBuf );
		}
	} while (s);
}
