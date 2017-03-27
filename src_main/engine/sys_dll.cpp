//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// sys_win.c -- Win32 system interface code
//=============================================================================

#ifdef _WIN32
#include <windows.h>
#include <dsound.h>
#endif

#include "quakedef.h"
#include "errno.h"
#include "profiling.h"
#include "server.h"
#include "vengineserver_impl.h"
#include "filesystem_engine.h"
#include "sys.h"
#include "ivideomode.h"
#include "host_cmd.h"
#include "crtmemdebug.h"
#include "dll_state.h"
#include "sv_log.h"
#include "traceinit.h"
#include "dt_test.h"
#include "vgui_int.h"
#include "keys.h"
#include "gl_matsysiface.h"
#include "tier0/vcrmode.h"
#include <Color.h>
#include "vstdlib/ICommandLine.h"
#include "cmd.h"
#include "glquake.h"

#ifdef _WIN32
#include "conproc.h"
#include "procinfo.h"	//MRM
#include <io.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define FIFTEEN_MB				(15 * 1024 * 1024)

#define MINIMUM_WIN_MEMORY		0x0e00000 
#define WARNING_MEMORY          0x0200000
#define MAXIMUM_WIN_MEMORY		0x2800000 // Ask for 40 MB max
#define MAXIMUM_DEDICATED_MEMORY	0x2800000 // Ask for 40 MB max

char				*CheckParm(const char *psz, char **ppszValue = NULL);
void				SeedRandomNumberGenerator( bool random_invariant );

#ifdef _WIN32
void				S_GetDSPointer(LPDIRECTSOUND *lpDS, LPDIRECTSOUNDBUFFER *lpDSBuf);
void				*S_GetWAVPointer(void);
void				StoreProfile ( void );
void				VID_UpdateVID(struct viddef_s *pvid);
void				Host_GetHostInfo(float *fps, int *nActive, int *nMaxPlayers, char *pszMap);

#endif

void COM_ShutdownFileSystem( void );
void COM_InitFilesystem( void );

modinfo_t			gmodinfo;
qboolean			Win32AtLeastV4; 

#ifdef _WIN32
extern HWND			*pmainwindow;
#endif

// 0 = not active, 1 = active, 2 = pause
int					giActive = 0;	
// Assume full internet authentication
qboolean			gfUseLANAuthentication = false;  
char				*argv[MAX_NUM_ARGVS];
static char			*empty_string = "";
char				gszDisconnectReason[256];
qboolean			gfExtendedError = false;

CreateInterfaceFn	physicsFactory = NULL;
CSysModule			*g_PhysicsDLL = NULL;
CreateInterfaceFn	g_AppSystemFactory = NULL;

qboolean			isDedicated;

// Set to true when we exit from an error.
bool g_bInErrorExit = false;

static FileFindHandle_t	g_hfind = FILESYSTEM_INVALID_FIND_HANDLE;

// Directory that we scan for extension DLLs.
static const char g_szExtensionDllSubDir[] = EXTENSION_DLL_SUBDIR;

// The extension DLL directory--one entry per loaded DLL
static CSysModule *g_GameDLL = NULL;

// Prototype of an global method function
typedef void (DLLEXPORT * PFN_GlobalMethod)( edict_t *pEntity );

IServerGameDLL	*serverGameDLL = NULL;
IServerGameEnts *serverGameEnts = NULL;
IServerGameClients *serverGameClients = NULL;

void Sys_InitArgv( char *lpCmdLine );
void Sys_ShutdownArgv( void );

//-----------------------------------------------------------------------------
// Purpose: Compare file times
// Input  : ft1 - 
//			ft2 - 
// Output : int
//-----------------------------------------------------------------------------
int Sys_CompareFileTime( long ft1, long ft2 )
{
	if ( ft1 < ft2 )
	{
		return -1;
	}
	else if ( ft1 > ft2 )
	{
		return 1;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Create specified directory
// Input  : *path - 
// Output : void Sys_mkdir
//-----------------------------------------------------------------------------
void Sys_mkdir ( const char *path )
{
	char testpath[ MAX_OSPATH ];

	// Remove any terminal backslash or /
	strcpy( testpath, path );
	if ( strlen( testpath ) > 0 &&
		testpath[ strlen( testpath ) - 1 ] == '/' ||
		testpath[ strlen( testpath ) - 1 ] == '\\' )
	{
		testpath[ strlen( testpath ) - 1 ] = 0;
	}

	if( g_pFileSystem->FileExists( testpath, "GAME" ) )
	{
		if( g_pFileSystem->IsDirectory( testpath, "GAME" ) )
		{
			return;
		}
		else
		{
			g_pFileSystem->RemoveFile( testpath, "GAME" );
		}
	}

	g_pFileSystem->CreateDirHierarchy( path, "GAME" );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *path - 
//			*basename - 
// Output : char *Sys_FindFirst
//-----------------------------------------------------------------------------
const char *Sys_FindFirst (const char *path, char *basename)
{
	if (g_hfind != FILESYSTEM_INVALID_FIND_HANDLE)
	{
		Sys_Error ("Sys_FindFirst without close");
		g_pFileSystem->FindClose(g_hfind);		
	}

	const char* psz = g_pFileSystem->FindFirst(path, &g_hfind);
	if (basename && psz)
	{
		COM_FileBase(psz, basename);
	}

	return psz;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *basename - 
// Output : char *Sys_FindNext
//-----------------------------------------------------------------------------
const char* Sys_FindNext (char *basename)
{
	const char *psz = g_pFileSystem->FindNext(g_hfind);
	if ( basename && psz )
	{
		COM_FileBase(psz, basename);
	}

	return psz;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Sys_FindClose
//-----------------------------------------------------------------------------

void Sys_FindClose (void)
{
	if ( FILESYSTEM_INVALID_FIND_HANDLE != g_hfind )
	{
		g_pFileSystem->FindClose(g_hfind);
		g_hfind = FILESYSTEM_INVALID_FIND_HANDLE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set OS version # for Win32
//-----------------------------------------------------------------------------
void Sys_CheckOSVersion( void )
{
#ifdef _WIN32
	OSVERSIONINFO	vinfo;
	memset( &vinfo, 0, sizeof( vinfo ) );

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if ( !GetVersionEx (&vinfo) )
	{
		Sys_Error ("Couldn't get OS info");
	}

	if (vinfo.dwMajorVersion < 4)
	{
		Win32AtLeastV4 = FALSE;
	}
	else
	{
		Win32AtLeastV4 = TRUE;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: OS Specific initializations
//-----------------------------------------------------------------------------
void Sys_Init( void )
{
	Sys_CheckOSVersion();
	// Set default FPU control word to truncate (chop) mode for optimized _ftol()
	// This does not "stick", the mode is restored somewhere down the line.
//	Sys_TruncateFPU();	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Sys_Shutdown( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Exit engine with error
// Input  : *error - 
//			... - 
// Output : void Sys_Error
//-----------------------------------------------------------------------------
void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[1024];
	static      qboolean bReentry = false; // Don't meltdown

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof( text ), error, argptr);
	va_end (argptr);

	if ( bReentry )
	{
		fprintf(stderr, "%s\n", text);
		return;
	}

	bReentry = true;

	if (isDedicated)
	{
		printf( "%s\n", text );
	}
	else
	{
		Sys_Printf( "%s\n", text );
	}

	g_bInErrorExit = true;
	
#ifndef SWDS
	if ( videomode )
		videomode->Shutdown();
#endif
#ifdef _WIN32
	::MessageBox( NULL, text, "Engine Error", MB_OK | MB_TOPMOST );
#endif
	exit( 1 );
}


bool IsInErrorExit()
{
	return g_bInErrorExit;
}


//-----------------------------------------------------------------------------
// Purpose: Print to system console
// Input  : *fmt - 
//			... - 
// Output : void Sys_Printf
//-----------------------------------------------------------------------------
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr,fmt);
	Q_vsnprintf (text, sizeof( text ), fmt, argptr);
	va_end (argptr);
		
	if ( developer.GetInt() )
	{
#ifdef _WIN32
		OutputDebugString( text );
		Sleep( 0 );
#endif
	}

	if ( isDedicated )
	{
		printf( text );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Sys_Quit
//-----------------------------------------------------------------------------
void Sys_Quit (void)
{
	Host_Shutdown();

	if ( isDedicated )
	{
#ifdef _WIN32
		FreeConsole();
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msec - 
// Output : void Sys_Sleep
//-----------------------------------------------------------------------------
void Sys_Sleep ( int msec )
{
#ifdef _WIN32
	Sleep ( msec );
#elif _LINUX
	usleep( msec );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hInst - 
//			ulInit - 
//			lpReserved - 
// Output : BOOL WINAPI   DllMain
//-----------------------------------------------------------------------------
#ifdef _WIN32
BOOL WINAPI   DllMain (HANDLE hInst, ULONG ulInit, LPVOID lpReserved)
{
	InitCRTMemDebug();
	if (ulInit == DLL_PROCESS_ATTACH)
	{
	} 
	else if (ulInit == DLL_PROCESS_DETACH)
	{
	}

	return TRUE;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iState - 
//-----------------------------------------------------------------------------
void GameSetState(int iState )
{
	giActive = iState; 
}

//-----------------------------------------------------------------------------
// Purpose: Allocate memory for engine hunk
// Input  : *parms - 
//-----------------------------------------------------------------------------
void Sys_InitMemory( void )
{
#ifdef _WIN32
	MEMORYSTATUS	lpBuffer;

	// Get OS Memory status
	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus (&lpBuffer);

	// take the greater of all the available memory or half the total memory,
	// but at least 10 Mb and no more than 32 Mb, unless they explicitly
	// request otherwise
	host_parms.memsize = lpBuffer.dwTotalPhys;

	if ( host_parms.memsize < FIFTEEN_MB )
	{
		Sys_Error( "Available memory less than 15MB!!! %i\n", host_parms.memsize );
	}

	// take half the physical memory
	host_parms.memsize = (int)( lpBuffer.dwTotalPhys >> 1 );

	// At least MINIMUM_WIN_MEMORY mb, even if we have to swap a lot.
	if (host_parms.memsize < MINIMUM_WIN_MEMORY)
	{
		host_parms.memsize = MINIMUM_WIN_MEMORY;
	}

	// Apply cap
	if (host_parms.memsize > MAXIMUM_WIN_MEMORY)
	{
		host_parms.memsize = MAXIMUM_WIN_MEMORY;
	}

#else

        //FILE *meminfo=fopen("/proc/meminfo","r"); // read in meminfo file?
        // sysinfo() system call??

        // hard code 32 mb for dedicated servers
        host_parms.memsize = MAXIMUM_DEDICATED_MEMORY;
#endif


	// Allow overrides
	int nHeapSize = CommandLine()->ParmValue( "-heapsize", 0 ); 
	if ( nHeapSize )
	{
		host_parms.memsize = nHeapSize * 1024;
	}

	if ( CommandLine()->FindParm( "-minmemory" ) )
	{
		host_parms.memsize = MINIMUM_WIN_MEMORY;
	}
	
	host_parms.membase = (unsigned char *)new unsigned char[ host_parms.memsize ];
	if ( !host_parms.membase )
	{
		Sys_Error( "Unable to allocate %.2f MB\n", (float)(host_parms.memsize)/( 1024.0 * 1024.0 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Sys_LoadPhysicsDLL( void )
{
	// Load the dependent components
	g_PhysicsDLL = FileSystem_LoadModule( "VPHYSICS.DLL" );
	physicsFactory = Sys_GetFactory( g_PhysicsDLL );
	if ( !physicsFactory )
	{
		Sys_Error( "Sys_LoadPhysicsDLL:  Failed to load VPHYSICS.DLL\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Sys_UnloadPhysicsDLL( void )
{
	if ( !g_PhysicsDLL )
		return;

	physicsFactory = NULL;
	FileSystem_UnloadModule( g_PhysicsDLL );
	g_PhysicsDLL = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parms - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void Sys_ShutdownMemory( void )
{
	delete[] host_parms.membase;
	host_parms.membase = 0;
	host_parms.memsize = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Sys_InitAuthentication( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Sys_ShutdownAuthentication( void )
{
}

//-----------------------------------------------------------------------------
// Debug library spew output
//-----------------------------------------------------------------------------

SpewRetval_t Sys_SpewFunc( SpewType_t spewType, const char *pMsg )
{
	char temp[8192];
	char *pFrom = (char *)pMsg;
	char *pTo = temp;
	char *pLimit = &temp[sizeof(temp) - 2]; // always space for 2 chars plus null (ie %%)

	while ( *pFrom && pTo < pLimit )
	{
		*pTo = *pFrom++;
		if ( *pTo++ == '%' )
			*pTo++ = '%';
	}
	*pTo = 0;
	
	if ((spewType != SPEW_LOG) || (svs.maxclients == 1))
	{
#ifndef SWDS
		switch ( spewType )
		{
		case SPEW_WARNING:
			{
				static Color red( 255, 90, 90, 255 );
				Con_ColorPrintf( red, temp );
			}
			break;
		case SPEW_ASSERT:
			{
				static Color brightred( 255, 20, 20, 255 );
				Con_ColorPrintf( brightred, temp );
			}
			break;
		case SPEW_ERROR:
			{
				static Color brightblue( 20, 70, 255, 255 );
				Con_ColorPrintf( brightblue, temp );
			}
		default:
#endif
			{
				Con_Printf( temp );
			}
#ifndef SWDS
			break;
		}
#endif
	}
	else
	{
		g_Log.Printf( temp );
	}

	if (spewType == SPEW_ERROR)
	{
		Sys_Error( temp );
		return SPEW_ABORT;
	}
	if (spewType == SPEW_ASSERT) 
		return SPEW_DEBUGGER;
	return SPEW_CONTINUE;
}

void DeveloperChangeCallback( ConVar *var, const char *pOldString )
{
	// Set the "developer" spew group to the value...
	int val = var->GetInt();
	SpewActivate( "developer", val );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *lpOrgCmdLine - 
//			launcherFactory - 
//			*pwnd - 
//			bIsDedicated - 
// Output : int
//-----------------------------------------------------------------------------
int Sys_InitGame( CreateInterfaceFn appSystemFactory, const char* pBaseDir, void *pwnd, int bIsDedicated )
{
#ifdef BENCHMARK
	if ( bIsDedicated )
	{
		Error( "Dedicated server isn't supported by this benchmark!" );
	}
#endif

	extern void InitMathlib( void );
	InitMathlib();

	// Install debug spew output....
	developer.InstallChangeCallback( DeveloperChangeCallback );
	SpewOutputFunc( Sys_SpewFunc );

	// Assume failure
	host_initialized = false;
#ifdef _WIN32
	// Grab main window pointer
	pmainwindow = ( HWND * )pwnd;
#endif
	// Remember that this is a dedicated server
	isDedicated = bIsDedicated;

	memset( &gmodinfo, 0, sizeof( modinfo_t ) );

	static char s_pBaseDir[256];
	strcpy( s_pBaseDir, pBaseDir );
	host_parms.basedir = s_pBaseDir;

	// Initialize clock
	TRACEINIT( Sys_Init(), Sys_Shutdown() );

#ifdef _DEBUG
	if( !CommandLine()->FindParm( "-nodttest" ) && !CommandLine()->FindParm( "-dti" ) )
	{
		RunDataTableTest();	
	}
#endif

	// NOTE: Can't use COM_CheckParm here because it hasn't been set up yet.
	SeedRandomNumberGenerator( CommandLine()->FindParm( "-random_invariant" ) != 0 );

	TRACEINIT( Sys_InitMemory(), Sys_ShutdownMemory() );

	TRACEINIT( Sys_LoadPhysicsDLL(), Sys_UnloadPhysicsDLL() );

	TRACEINIT( Host_Init(), Host_Shutdown() );

	if ( !host_initialized )
	{
		return 0;
	}

	TRACEINIT( Sys_InitAuthentication(), Sys_ShutdownAuthentication() );

	TRACEINIT( SV_InitGameDLL(), SV_ShutdownGameDLL() );

	if ( isDedicated )
	{
		NET_Config( true ); // Start up networking on dedicated machines.
	}

 	// Benchmarking mode to render a particular frame
	// Need to do it this way because benchframe has 2 arguments instead of 1
	int nIndex = CommandLine()->FindParm( "-benchframe" );
	if( nIndex && nIndex < CommandLine()->ParmCount() - 2 )
	{
		char buf[256];
		Q_snprintf( buf, 256, "benchframe %s %s %s\n", CommandLine()->GetParm( nIndex + 1 ), CommandLine()->GetParm( nIndex + 2 ), CommandLine()->GetParm( nIndex + 3 ) );
		Cbuf_AddText( buf );
		mat_norendering.SetValue ( 1 );
	}

	// timedemo with 2 args from the command-line
	// Need to do it this way because benchframe has 2 arguments instead of 1
	nIndex = CommandLine()->FindParm( "-timedemoquit" );
	if( nIndex && nIndex < CommandLine()->ParmCount() - 2 )
	{
		char buf[256];
		Q_snprintf( buf, 256, "timedemoquit %s %s\n", CommandLine()->GetParm( nIndex + 1 ), CommandLine()->GetParm( nIndex + 2 ) );
		Cbuf_AddText( buf );
	}
	
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Sys_ShutdownGame( void )
{
	TRACESHUTDOWN( SV_ShutdownGameDLL() );

	TRACESHUTDOWN( Sys_ShutdownAuthentication() );

	TRACESHUTDOWN( Host_Shutdown() );

	if ( isDedicated )
	{
		NET_Config( false );
	}

	TRACESHUTDOWN( Sys_UnloadPhysicsDLL() );

	TRACESHUTDOWN( Sys_ShutdownMemory() );

	// TRACESHUTDOWN( Sys_ShutdownArgv() );

	TRACESHUTDOWN( Sys_Shutdown() );

	// Remove debug spew output....
	developer.InstallChangeCallback( 0 );
	SpewOutputFunc( 0 );
}

//
// Try to load a single DLL.  If it conforms to spec, keep it loaded, and add relevant
// info to the DLL directory.  If not, ignore it entirely.
//
#pragma optimize( "g", off )
static bool LoadThisDll( char *szDllFilename )
{
	CreateInterfaceFn serverFactory;

	CSysModule *pDLL = NULL;
	// Load DLL, ignore if cannot
	if ((pDLL = FileSystem_LoadModule(szDllFilename)) == NULL)
	{
		Con_Printf("Failed to load %s\n", szDllFilename);
		goto IgnoreThisDLL;
	}

	// Load interface factory and any interfaces exported by the game .dll
	serverFactory = Sys_GetFactory( pDLL );
	if ( serverFactory )
	{
		serverGameDLL = (IServerGameDLL*)serverFactory(INTERFACEVERSION_SERVERGAMEDLL, NULL);
		if ( !serverGameDLL )
		{
			Con_Printf( "Could not get IServerGameDLL interface from library %s", szDllFilename );
			goto IgnoreThisDLL;
		}
		serverGameEnts = (IServerGameEnts*)serverFactory(INTERFACEVERSION_SERVERGAMEENTS, NULL);
		if ( !serverGameEnts )
		{
			Con_Printf( "Could not get IServerGameEnts interface from library %s", szDllFilename );
			goto IgnoreThisDLL;
		}
		serverGameClients = (IServerGameClients*)serverFactory(INTERFACEVERSION_SERVERGAMECLIENTS, NULL);
		if ( !serverGameClients )
		{
			Con_Printf( "Could not get IServerGameClients interface from library %s", szDllFilename );
			goto IgnoreThisDLL;
		}
	}
	else
	{
		Con_Printf( "Could not find factory interface in library %s", szDllFilename );
		goto IgnoreThisDLL;
	}

	g_GameDLL = pDLL;
	return true;

IgnoreThisDLL:
	if (pDLL != NULL)
	{
		FileSystem_UnloadModule(pDLL);
		serverGameDLL = NULL;
		serverGameEnts = NULL;
		serverGameClients = NULL;
	}
	return false;
}
#pragma optimize( "", on )

void DLL_SetModKey( modinfo_t *pinfo, char *pkey, char *pvalue )
{
	if ( !stricmp( pkey, "url_info" ) )
	{
		pinfo->bIsMod = true;
		strcpy( pinfo->szInfo, pvalue );
	}
	else if ( !stricmp( pkey, "url_dl" ) )
	{
		pinfo->bIsMod = true;
		strcpy( pinfo->szDL, pvalue );
	}
	else if ( !stricmp( pkey, "version" ) )
	{
		pinfo->bIsMod = true;
		pinfo->version = atoi( pvalue );
	}
	else if ( !stricmp( pkey, "size" ) )
	{
		pinfo->bIsMod = true;
		pinfo->size = atoi( pvalue );
	}
	else if ( !stricmp( pkey, "svonly" ) )
	{
		pinfo->bIsMod = true;
		pinfo->svonly = atoi( pvalue ) ? true : false;
	}
	else if ( !stricmp( pkey, "cldll" ) )
	{
		pinfo->bIsMod = true;
		pinfo->cldll = atoi( pvalue ) ? true : false;
	}
	else if ( !stricmp( pkey, "hlversion" ) )
	{
		strcpy( pinfo->szHLVersion, pvalue );
	}
}

//
// Scan DLL directory, load all DLLs that conform to spec.
//
void LoadEntityDLLs( char *szBaseDir )
{
	char szDllFilename[ MAX_PATH ];

	// vars for .gam file loading:
	char *pszInputStream;
	char *pStreamPos;
	char szDllListFile[ MAX_PATH ];
	int  nFileSize = -1;
	int  nBytesRead;
	FileHandle_t  hLibListFile;
	char szGameDir[64];
	char szKey[64];
	char szValue[256];

	SV_ResetModInfo();

	// Run through all DLLs found in the extension DLL directory
	g_GameDLL = NULL;

	// Check for modified game
	const char *pGame = CommandLine()->ParmValue( "-game" );
	if (pGame)
	{
		strcpy(szGameDir, pGame);
	}
	else
	{
		strcpy(szGameDir, com_defaultgamedir);  // resolves to VALVE by default
	}

	if ( Q_stricmp( szGameDir, com_defaultgamedir ) )
	{
		gmodinfo.bIsMod = true;
	}

	Q_snprintf(szDllListFile					// Listing file for this game.
		, sizeof( szDllListFile )
		, "scripts\\%s"						// GAME_LIST_FILE defined in prdlls.h
		, GAME_LIST_FILE);

	hLibListFile = g_pFileSystem->Open( szDllListFile, "rb" );
	if ( hLibListFile )
	{
		nFileSize = g_pFileSystem->Size( hLibListFile );
	}

	if (nFileSize != -1)
	{
		if ((nFileSize == 0) ||
			(nFileSize > 1024*256))				// 0 or 256K .gam file is probably bogus
			Sys_Error("Game listing file size is bogus [%s: size %i]",GAME_LIST_FILE,nFileSize);

		pszInputStream = (char *)malloc(nFileSize + 1);  // Leave room for terminating 0 just in case
		if (!pszInputStream)
			Sys_Error("Could not allocate space for game listing file of %i bytes", nFileSize + 1);
		
		nBytesRead = g_pFileSystem->Read(
			(void *)pszInputStream,
			nFileSize,
			hLibListFile );

		if (nBytesRead != nFileSize)			// Check amound actually read
			Sys_Error("Error reading in game listing file, expected %i bytes, read %i", nFileSize, nBytesRead);
		
		pszInputStream[nFileSize] = '\0';		// Prevent overrun
		pStreamPos = pszInputStream;			// File loaded ok.
		
		// Skip the first two tokens:  game "Half-Life" which are used by the front end to determine the game type for this liblist.gam
		com_ignorecolons = true;

		while ( 1 )
		{
			pStreamPos = COM_Parse(pStreamPos);        // Read a key
			if ( strlen ( com_token ) <= 0 )  // Done
				break;

			strcpy( szKey, com_token );

			pStreamPos = COM_Parse(pStreamPos);			// Read key value

			// Second token can be "blank"
			strcpy( szValue, com_token );

			DLL_SetModKey( &gmodinfo, szKey, szValue );
		}	
			
		com_ignorecolons = false;

		free(pszInputStream);		            // Clean up
		g_pFileSystem->Close(hLibListFile);
	}
	
	// Load the game .dll
#ifdef _WIN32
	Q_snprintf( szDllFilename, sizeof( szDllFilename ), "%s\\%s\\bin\\server.dll", szBaseDir, szGameDir );
#elif _LINUX
	Q_snprintf( szDllFilename, sizeof( szDllFilename ), "%s/%s/bin/server_i486.so", szBaseDir, szGameDir );
#else
#error "define server.dll type"
#endif

	Con_DPrintf( "Adding:  %s\n",szDllFilename );
	LoadThisDll( szDllFilename );

	if ( serverGameDLL && gmodinfo.bIsMod )
	{
		Msg("Game .dll loaded for \"%s\"\n", (char *)serverGameDLL->GetGameDescription());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Retrieves a value from the registry
// Input  : *pszSubKey - 
//			*pszElement - 
//			*pszReturnString - 
//			nReturnLength - 
//			*pszDefaultValue - 
//-----------------------------------------------------------------------------
#if defined(_WIN32)
void Sys_GetRegKeyValueUnderRoot( HKEY rootKey, const char *pszSubKey, const char *pszElement, char *pszReturnString, int nReturnLength, const char *pszDefaultValue )
{
	LONG lResult;           // Registry function result code
	HKEY hKey;              // Handle of opened/created key
	char szBuff[128];       // Temp. buffer
	DWORD dwDisposition;    // Type of key opening event
	DWORD dwType;           // Type of key
	DWORD dwSize;           // Size of element data

	// Assume the worst
	Q_snprintf(pszReturnString, nReturnLength, pszDefaultValue);

	// Create it if it doesn't exist.  (Create opens the key otherwise)
	lResult = g_pVCR->Hook_RegCreateKeyEx(
		rootKey,	// handle of open key 
		pszSubKey,			// address of name of subkey to open 
		0,					// DWORD ulOptions,	  // reserved 
		"String",			// Type of value
		REG_OPTION_NON_VOLATILE, // Store permanently in reg.
		KEY_ALL_ACCESS,		// REGSAM samDesired, // security access mask 
		NULL,
		&hKey,				// Key we are creating
		&dwDisposition);    // Type of creation
	
	if (lResult != ERROR_SUCCESS)  // Failure
		return;

	// First time, just set to Valve default
	if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		// Just Set the Values according to the defaults
		lResult = g_pVCR->Hook_RegSetValueEx( hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszDefaultValue, Q_strlen(pszDefaultValue) + 1 ); 
	}
	else
	{
		// We opened the existing key. Now go ahead and find out how big the key is.
		dwSize = nReturnLength;
		lResult = g_pVCR->Hook_RegQueryValueEx( hKey, pszElement, 0, &dwType, (unsigned char *)szBuff, &dwSize );

		// Success?
		if (lResult == ERROR_SUCCESS)
		{
			// Only copy strings, and only copy as much data as requested.
			if (dwType == REG_SZ)
			{
				Q_strncpy(pszReturnString, szBuff, nReturnLength);
				pszReturnString[nReturnLength - 1] = '\0';
			}
		}
		else
		// Didn't find it, so write out new value
		{
			// Just Set the Values according to the defaults
			lResult = g_pVCR->Hook_RegSetValueEx( hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszDefaultValue, Q_strlen(pszDefaultValue) + 1 ); 
		}
	};

	// Always close this key before exiting.
	g_pVCR->Hook_RegCloseKey(hKey);

}

void Sys_SetRegKeyValueUnderRoot( HKEY rootKey, const char *pszSubKey, const char *pszElement, const char *pszValue )
{
	LONG lResult;           // Registry function result code
	HKEY hKey;              // Handle of opened/created key
	//char szBuff[128];       // Temp. buffer
	DWORD dwDisposition;    // Type of key opening event
	//DWORD dwType;           // Type of key
	//DWORD dwSize;           // Size of element data

	// Create it if it doesn't exist.  (Create opens the key otherwise)
	lResult = g_pVCR->Hook_RegCreateKeyEx(
		rootKey,			// handle of open key 
		pszSubKey,			// address of name of subkey to open 
		0,					// DWORD ulOptions,	  // reserved 
		"String",			// Type of value
		REG_OPTION_NON_VOLATILE, // Store permanently in reg.
		KEY_ALL_ACCESS,		// REGSAM samDesired, // security access mask 
		NULL,
		&hKey,				// Key we are creating
		&dwDisposition);    // Type of creation
	
	if (lResult != ERROR_SUCCESS)  // Failure
		return;

	// First time, just set to Valve default
	if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		// Just Set the Values according to the defaults
		lResult = g_pVCR->Hook_RegSetValueEx( hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszValue, Q_strlen(pszValue) + 1 ); 
	}
	else
	{
		/*
		// FIXE:  We might want to support a mode where we only create this key, we don't overwrite values already present
		// We opened the existing key. Now go ahead and find out how big the key is.
		dwSize = nReturnLength;
		lResult = g_pVCR->Hook_RegQueryValueEx( hKey, pszElement, 0, &dwType, (unsigned char *)szBuff, &dwSize );

		// Success?
		if (lResult == ERROR_SUCCESS)
		{
			// Only copy strings, and only copy as much data as requested.
			if (dwType == REG_SZ)
			{
				Q_strncpy(pszReturnString, szBuff, nReturnLength);
				pszReturnString[nReturnLength - 1] = '\0';
			}
		}
		else
		*/
		// Didn't find it, so write out new value
		{
			// Just Set the Values according to the defaults
			lResult = g_pVCR->Hook_RegSetValueEx( hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszValue, Q_strlen(pszValue) + 1 ); 
		}
	};

	// Always close this key before exiting.
	g_pVCR->Hook_RegCloseKey(hKey);
}

#endif

void Sys_GetRegKeyValue( char *pszSubKey, char *pszElement,	char *pszReturnString, int nReturnLength, char *pszDefaultValue )
{
#if defined(_WIN32)
	Sys_GetRegKeyValueUnderRoot( HKEY_CURRENT_USER, pszSubKey, pszElement, pszReturnString, nReturnLength, pszDefaultValue );
#endif
}

void Sys_SetRegKeyValue( char *pszSubKey, char *pszElement,	char *pszValue )
{
#if defined(_WIN32)
	Sys_SetRegKeyValueUnderRoot( HKEY_CURRENT_USER, pszSubKey, pszElement, pszValue );
#endif
}

#define SOURCE_ENGINE_APP_CLASS "Valve.Source"

void Sys_CreateFileAssociations( int count, FileAssociationInfo *list )
{
#if defined(_WIN32)
	char appname[ 512 ];

	GetModuleFileName( 0, appname, sizeof( appname ) );
	COM_FixSlashes( appname );
	Q_strlower( appname );

	char quoted_appname_with_arg[ 512 ];
	Q_snprintf( quoted_appname_with_arg, sizeof( quoted_appname_with_arg ), "\"%s\" \"%%1\"", appname );
	char base_exe_name[ 256 ];
	COM_FileBase( appname, base_exe_name );
	COM_DefaultExtension( base_exe_name, ".exe", sizeof( base_exe_name ) );

	// HKEY_CLASSES_ROOT/Valve.Source/shell/open/command == "u:\tf2\hl2.exe" "%1" quoted
	Sys_SetRegKeyValueUnderRoot( HKEY_CLASSES_ROOT, va( "%s\\shell\\open\\command", SOURCE_ENGINE_APP_CLASS ), "", quoted_appname_with_arg );
	// HKEY_CLASSES_ROOT/Applications/hl2.exe/shell/open/command == "u:\tf2\hl2.exe" "%1" quoted
	Sys_SetRegKeyValueUnderRoot( HKEY_CLASSES_ROOT, va( "Applications\\%s\\shell\\open\\command", base_exe_name ), "", quoted_appname_with_arg );

	for ( int i = 0; i < count ; i++ )
	{
		FileAssociationInfo *fa = &list[ i ];
		// Create file association for our .exe
		// HKEY_CLASSES_ROOT/.dem == "Valve.Source"
		Sys_SetRegKeyValueUnderRoot( HKEY_CLASSES_ROOT, fa->extension, "", SOURCE_ENGINE_APP_CLASS );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnloadEntityDLLs( void )
{
	// Unlink the cvars associated with game DLL
	cv->UnlinkExternals();		

	FileSystem_UnloadModule( g_GameDLL );
	g_GameDLL = NULL;
	serverGameDLL = NULL;
	serverGameEnts = NULL;
	serverGameClients = NULL;
}
