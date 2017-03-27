//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseFileSystem.h"
#include "steamlib.h"
#include "steam.h"
#include "tier0/dbg.h"
#include "vstdlib/icommandline.h"

class CFileSystem_Steam : public CBaseFileSystem
{
public:
	CFileSystem_Steam();
	~CFileSystem_Steam();

	// Methods of IAppSystem
	virtual InitReturnVal_t Init();
	virtual void			Shutdown();

	// Higher level filesystem methods requiring specific behavior
	virtual void GetLocalCopy( const char *pFileName );
	virtual void LogLevelLoadStarted( const char *name );
	virtual void LogLevelLoadFinished( const char *name );
	virtual int	ProgressCounter( void );
	virtual int	HintResourceNeed( const char *hintlist, int forgetEverything );
	virtual int	BlockForResources( const char *hintlist );
	virtual int	PauseResourcePreloading( void );
	virtual int	ResumeResourcePreloading( void );
	virtual void TrackProgress( int enable );
	virtual void RegisterAppProgressCallback( void (*fpProgCallBack)(void), int freq );
	virtual void RegisterAppKeepAliveTicCallback( void (*fpKeepAliveTicCallBack)(char *scr_msg) );
	virtual void UpdateProgress( void );
	virtual int	SetVBuf( FileHandle_t stream, char *buffer, int mode, long size );
	virtual void GetInterfaceVersion( char *p, int maxlen );
	virtual CSysModule * CFileSystem_Steam::LoadModule( const char *path );

protected:
	// implementation of CBaseFileSystem virtual functions
	virtual FILE *FS_fopen( const char *filename, const char *options );
	virtual void FS_fclose( FILE *fp );
	virtual void FS_fseek( FILE *fp, long pos, int seekType );
	virtual long FS_ftell( FILE *fp );
	virtual int FS_feof( FILE *fp );
	virtual size_t FS_fread( void *dest, size_t count, size_t size, FILE *fp );
	virtual size_t FS_fwrite( const void *src, size_t count, size_t size, FILE *fp );
	virtual size_t FS_vfprintf( FILE *fp, const char *fmt, va_list list );
	virtual int FS_ferror( FILE *fp );
	virtual int FS_fflush( FILE *fp );
	virtual char *FS_fgets( char *dest, int destSize, FILE *fp );
	virtual int FS_stat( const char *path, struct _stat *buf );
	virtual HANDLE FS_FindFirstFile(char *findname, WIN32_FIND_DATA *dat);
	virtual bool FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA *dat);
	virtual bool FS_FindClose(HANDLE handle);

	virtual bool IsFileImmediatelyAvailable(const char *pFileName);

private:
	bool m_bSteamInitialized;
	bool m_bCurrentlyLoading;
	bool m_bAssertFilesImmediatelyAvailable;

};


//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------

CFileSystem_Steam::CFileSystem_Steam()
{
	m_bSteamInitialized = false;
	m_bCurrentlyLoading = false;
	m_bAssertFilesImmediatelyAvailable = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFileSystem_Steam::~CFileSystem_Steam()
{
	m_bSteamInitialized = false;
}


//-----------------------------------------------------------------------------
// Methods of IAppSystem
//-----------------------------------------------------------------------------
InitReturnVal_t CFileSystem_Steam::Init()
{
	InitReturnVal_t retval = INIT_OK;
	m_bSteamInitialized = true;

	// Initialize the STEAM VFS
	if ( !STEAM_Startup() )
	{
		const char *szErrorMsgFormat = "STEAM VFS failed to initialize: %s";
		const int nErrorMsgBufSize = 1024;
		char szSteamErrorMsg[nErrorMsgBufSize];
		char szErrorMsg[nErrorMsgBufSize+sizeof(szErrorMsgFormat)];
		STEAM_strerror(NULL, szSteamErrorMsg, sizeof(szSteamErrorMsg));
		sprintf(szErrorMsg, szErrorMsgFormat, szSteamErrorMsg);
		::MessageBox(NULL, szErrorMsg, "Half-Life Steam FileSystem Error", MB_OK);
		m_bSteamInitialized = false;
		retval = INIT_FAILED;
	}

	// If we're not running Steam in local mode use the current working directory 
	// as the mount point for the STEAM VFS
	if ( !CommandLine()->CheckParm("-steamlocal") && !STEAM_Mount(NULL) )
	{
		const char *szErrorMsgFormat = "STEAM VFS failed to mount: %s";
		const int nErrorMsgBufSize = 1024;
		char szErrorMsg[nErrorMsgBufSize+sizeof(szErrorMsgFormat)];
		char szSteamErrorMsg[nErrorMsgBufSize];
		STEAM_strerror(NULL, szSteamErrorMsg, sizeof(szSteamErrorMsg));
		sprintf(szErrorMsg, szErrorMsgFormat, szSteamErrorMsg);
		::MessageBox(NULL, szErrorMsg, "Half-Life Steam Filesystem Error", MB_OK);
		m_bSteamInitialized = false;
		retval = INIT_FAILED;
	}

	return retval;
}

void CFileSystem_Steam::Shutdown()
{
	assert( m_bSteamInitialized );

	// If we're not running Steam in local mode, remove all mount points from the STEAM VFS.
	if ( !CommandLine()->CheckParm("-steamlocal") && !STEAM_Unmount() )
	{
		const char *szErrorMsgFormat = "STEAM VFS failed to unmount: %s";
		const int nErrorMsgBufSize = 1024;
		char szErrorMsg[nErrorMsgBufSize+sizeof(szErrorMsgFormat)];
		char szSteamErrorMsg[nErrorMsgBufSize];
		STEAM_strerror(NULL, szSteamErrorMsg, sizeof(szSteamErrorMsg));
		sprintf(szErrorMsg, szErrorMsgFormat, szSteamErrorMsg);
		OutputDebugString(szErrorMsg);
		Assert(!("STEAM VFS failed to unmount"));

		// just continue on as if nothing happened
		// ::MessageBox(NULL, szErrorMsg, "Half-Life FileSystem_Steam Error", MB_OK);
		// exit( -1 );
	}

	STEAM_Shutdown();

	m_bSteamInitialized = false;
}



static CFileSystem_Steam g_FileSystem_Steam;

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
FILE *CFileSystem_Steam::FS_fopen( const char *filename, const char *options )
{
	// make sure the file is immediately available
	if (m_bAssertFilesImmediatelyAvailable && !m_bCurrentlyLoading)
	{
		if (!IsFileImmediatelyAvailable(filename))
		{
			Error("Steam FS: '%s' not immediately available when not in loading dialog", filename);
		}
	}

	return STEAM_fopen(filename, options);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
void CFileSystem_Steam::FS_fclose( FILE *fp )
{
	STEAM_fclose(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
void CFileSystem_Steam::FS_fseek( FILE *fp, long pos, int seekType )
{
	STEAM_fseek(fp, pos, seekType);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
long CFileSystem_Steam::FS_ftell( FILE *fp )
{
	return STEAM_ftell(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
int CFileSystem_Steam::FS_feof( FILE *fp )
{
	return STEAM_feof(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
size_t CFileSystem_Steam::FS_fread( void *dest, size_t count, size_t size, FILE *fp )
{
	return STEAM_fread(dest, count, size, fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
size_t CFileSystem_Steam::FS_fwrite( const void *src, size_t count, size_t size, FILE *fp )
{
	return STEAM_fwrite((void *)src, count, size, fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
size_t CFileSystem_Steam::FS_vfprintf( FILE *fp, const char *fmt, va_list list )
{
	return STEAM_vfprintf(fp, fmt, list);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
int CFileSystem_Steam::FS_ferror( FILE *fp )
{
	return STEAM_ferror(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
int CFileSystem_Steam::FS_fflush( FILE *fp )
{
	return STEAM_fflush(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
char *CFileSystem_Steam::FS_fgets( char *dest, int destSize, FILE *fp )
{
	return STEAM_fgets(dest, destSize, fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
int CFileSystem_Steam::FS_stat( const char *path, struct _stat *buf )
{
	return STEAM_stat(path, buf);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
HANDLE CFileSystem_Steam::FS_FindFirstFile(char *findname, WIN32_FIND_DATA *dat)
{
	return STEAM_FindFirstFile(findname, STEAMFindAll, dat);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
bool CFileSystem_Steam::FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA *dat)
{
	return (STEAM_FindNextFile(handle, dat) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
bool CFileSystem_Steam::FS_FindClose(HANDLE handle)
{
	return (STEAM_FindClose(handle) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: files are always immediately available on disk
//-----------------------------------------------------------------------------
bool CFileSystem_Steam::IsFileImmediatelyAvailable(const char *pFileName)
{
	TSteamError steamError;
	return (SteamIsFileImmediatelyAvailable(pFileName, &steamError) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFileSystem_Steam::GetLocalCopy( const char *pFileName )
{
	// Now try to find the dll under Steam so we can do a GetLocalCopy() on it
	struct _stat StatBuf;
	if ( STEAM_stat(pFileName, &StatBuf) == -1 )
	{
		// Use the environment search path to try and find it
		char* pPath = getenv("PATH");
	
		// Use the .EXE name to determine the root directory
		char srchPath[ MAX_PATH ];
		HINSTANCE hInstance = ( HINSTANCE )GetModuleHandle( 0 );
		if ( !GetModuleFileName( hInstance, srchPath, MAX_PATH ) )
		{
			::MessageBox( 0, "Failed calling GetModuleFileName", "Half-Life Steam Filesystem Error", MB_OK );
			return;
		}
	
		// Get the length of the root directory the .exe is in
		char* pSeperator = strrchr( srchPath, '\\' );
		int nBaseLen = 0;
		if ( pSeperator )
		{
			nBaseLen = pSeperator - srchPath;
		}
		
		// Extract each section of the path
		char* pStart = pPath;
		char* pEnd = 0;
		bool bSearch = true;
		while ( bSearch )
		{
			pEnd = strstr( pStart, ";" );
			if ( !pEnd )
				bSearch = false;

			int nSize = pEnd - pStart;
			
			// Is this path even in the base directory?
			if ( nSize > nBaseLen )
			{
				// Create a new path (relative to the base directory)
				assert( sizeof(srchPath) > nBaseLen + strlen(pFileName) + 2 );
				nSize -= nBaseLen+1;
				memcpy( srchPath, pStart+nBaseLen+1, nSize );
				memcpy( srchPath+nSize, pFileName, strlen(pFileName)+1 );
	
				if ( STEAM_stat(srchPath, &StatBuf) == 0 )
				{
					// Found the dll under Steam
					STEAM_GetLocalCopy( srchPath );
					break;
				}
			}
			pStart = pEnd+1;
		}
	}
	else
	{
		STEAM_GetLocalCopy( pFileName );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Load a DLL
// Input  : *path 
//-----------------------------------------------------------------------------
CSysModule * CFileSystem_Steam::LoadModule( const char *path )
{
	// File must end in .dll
	char szNewPath[512];
	char szExtension[] = ".dll";
	assert( strlen(path) < sizeof(szNewPath) );
	
	strcpy( szNewPath, path );
	if ( !strstr(szNewPath, szExtension) )
	{
		assert( strlen(path) + sizeof(szExtension) < sizeof(szNewPath) );
		strcat( szNewPath, szExtension );
	}

	// Get a local copy from Steam
	GetLocalCopy( szNewPath );

	// Now go load the module 
	return Sys_LoadModule( path );
}


//-----------------------------------------------------------------------------
void CFileSystem_Steam::LogLevelLoadStarted( const char *name )
{
	m_bCurrentlyLoading = true;
	STEAM_LogLevelLoadStarted( name );
}

void CFileSystem_Steam::LogLevelLoadFinished( const char *name )
{
	m_bCurrentlyLoading = false;
	STEAM_LogLevelLoadFinished( name );
}

int CFileSystem_Steam::ProgressCounter( void )
{
	return STEAM_ProgressCounter();
}

int CFileSystem_Steam::HintResourceNeed( const char *hintlist, int forgetEverything )
{
	return STEAM_HintResourceNeed( hintlist, forgetEverything );
}

int CFileSystem_Steam::BlockForResources( const char *hintlist )
{
	return STEAM_BlockForResources( hintlist );
}

int CFileSystem_Steam::PauseResourcePreloading(void)
{
	return STEAM_PauseResourcePreloading();
}

int CFileSystem_Steam::ResumeResourcePreloading(void)
{
	return STEAM_ResumeResourcePreloading();
}

void CFileSystem_Steam::TrackProgress( int enable )
{
	STEAM_TrackProgress( enable );
}

void CFileSystem_Steam::RegisterAppProgressCallback( void(*fpProgCallBack)(void), int freq )
{
	STEAM_RegisterAppProgressCallback( fpProgCallBack, freq );
}

void CFileSystem_Steam::RegisterAppKeepAliveTicCallback(void(*fpKeepAliveTicCallBack)(char *scr_msg) )
{
	STEAM_RegisterAppKeepAliveTicCallback( fpKeepAliveTicCallBack );
}

void CFileSystem_Steam::UpdateProgress( void )
{
	STEAM_UpdateProgress();
}

int CFileSystem_Steam::SetVBuf( FileHandle_t stream, char *buffer, int mode, long size )
{
	CFileHandle *fh = ( CFileHandle *)stream;
	if ( !fh )
	{
		Warning( (FileWarningLevel_t)-1, "FS:  Tried to SetVBuf NULL file handle!\n" );
		return 0;
	}

	return STEAM_setvbuf( fh->m_pFile, buffer, mode, size );
}

void CFileSystem_Steam::GetInterfaceVersion( char *p, int maxlen )
{
	STEAM_GetInterfaceVersion( p, maxlen );
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CFileSystem_Steam, IFileSystem, FILESYSTEM_INTERFACE_VERSION, g_FileSystem_Steam );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CFileSystem_Steam, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION, g_FileSystem_Steam );



