#include "winquake.h"
#include "quakedef.h"
#include "idedicatedexports.h"
#include "engine_launcher_api.h"
#include "ivideomode.h"
#include "common.h"
#include "iregistry.h"
#include "keys.h"
#include "cdll_engine_int.h"
#include "traceinit.h"
#include "iengine.h"
#include "igame.h"
#include "tier0/vcrmode.h"
#include "engine_hlds_api.h"
#include "filesystem_engine.h"
#include "vstdlib/ICommandLine.h"
#ifndef SWDS
#include "sys_mainwind.h"
#include "vgui/isystem.h"
#include "vgui_controls/controls.h"
#endif

static IDedicatedExports *dedicated = NULL;
extern CreateInterfaceFn g_AppSystemFactory;
	
extern void	(*VID_FlipScreen)( void );
int		(*D_SurfaceCacheForRes)(int width, int height);
extern void	(*Launcher_ConsolePrintf)(char *fmt, ...);

void GameSetState(int iState );
void Host_GetHostInfo(float *fps, int *nActive, int *nMaxPlayers, char *pszMap);
void Sys_InitFloatTime (void);
void Sys_ShutdownFloatTime(void);
void SeedRandomNumberGenerator(void);
const char *Key_BindingForKey( int keynum );
char *Key_KeynumToString (int keynum);
int			LoadGame(const char *pName);
BOOL		SaveGame(const char *pszSlot, const char *pszComment);
void COM_ShutdownFileSystem( void );
void COM_InitFilesystem( void );

#ifdef _WIN32
void VGui_GetMouse();
void VGui_ReleaseMouse();
int GL_SetMode (HWND mainwindow, HDC *pmaindc, HGLRC *pbaseRC, int fD3D, const char *pszDriver, const char *pszCmdLine );
void GL_Shutdown (HWND hwnd, HDC hdc, HGLRC hglrc);
BOOL  ( WINAPI * qwglSwapBuffers)(HDC);
#endif

void MaskExceptions (void);
// FIXME, move to a header?
void	Shader_InitDedicated( void );
void	Shader_Init( void );
void	Shader_Shutdown( void );
void	Shader_SetMode( void *mainWindow, bool windowed );

extern int	gD3DMode;

#ifdef _WIN32
HWND				*pmainwindow = NULL;
#endif

#ifndef SWDS

//-----------------------------------------------------------------------------
// Purpose: exports an interface that can be used by the launcher to run the engine
//			this is the exported function when compiled as a blob
//-----------------------------------------------------------------------------
void EXPORT F( IEngineAPI **api )
{
	*api = ( IEngineAPI * )(Sys_GetFactoryThis()(VENGINE_LAUNCHER_API_VERSION, NULL));
}

#endif // SWDS

#ifndef SWDS
HGLRC	baseRC;
HDC		maindc;

void Sys_VID_FlipScreen( void )
{
#if defined( GLQUAKE )
	qwglSwapBuffers( maindc );
#else
	videomode->FlipScreen();
#endif
}
#endif

#define SURFCACHE_SIZE_AT_320X200	1024*1024

int Sys_GetSurfaceCacheSize(int width, int height)
{
	int size, pix;

	const char *pCacheSize = CommandLine()->ParmValue("-surfcachesize");
	if ( pCacheSize )
	{
		size = atoi( pCacheSize ) * 1024;
		return size;
	}
	
	size = SURFCACHE_SIZE_AT_320X200 * 3;
	pix = width*height*2;

	if (pix > 64000)
	{
		size += (pix-64000)*3;
	}

	return size;
}

#ifndef SWDS
void Sys_GetCDKey( char *pszCDKey, int *nLength, int *bDedicated )
{
	char key[ 32 ];
	Q_strncpy( key, registry->ReadString( "key", "" ), 31 );
	key[ 31 ] = 0;

	strcpy( pszCDKey, key );
	if ( nLength )
	{
		*nLength = strlen( key );
	}
	if ( bDedicated )
	{
		*bDedicated = false;
	}
}
#endif

extern qboolean cs_initialized;
extern int			lowshift;
extern double		pfreq;
static char	*empty_string = "";


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern void SCR_UpdateScreen(void);
extern qboolean g_bMajorMapChange; 
extern qboolean g_bPrintingKeepAliveDots;

void Sys_ShowProgressTicks(char* specialProgressMsg)
{
#ifdef LATER
#define MAX_NUM_TICS 40

	static long numTics = 0;

	// Nothing to do if not using Steam
	if ( ! COM_CheckParm(STEAM_PARM) )
		return;

	// Update number of tics to show...
	numTics++;
	if ( isDedicated )
	{
		if ( g_bMajorMapChange )
		{
			g_bPrintingKeepAliveDots = TRUE;
			Sys_Printf(".");
		}
	}
	else
	{
		int i;
		int numTicsToPrint = numTics % (MAX_NUM_TICS-1);
		char msg[MAX_NUM_TICS+1];

		Q_strncpy(msg, ".", sizeof(msg)-1);
		msg[sizeof(msg)-1] = '\0';

		// Now add in the growing number of tics...
		for ( i = 1 ; i < numTicsToPrint ; i++ )
		{
			strncat(msg, ".", sizeof(msg)-1);
			msg[sizeof(msg)-1] = '\0';
		}

		Cvar_Set("scr_connectmsg1", msg);
		Cvar_Set("scr_connectmsg2", "Downloading resources...");
		SCR_UpdateScreen();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClearIOStates( void )
{
#ifndef SWDS
	int		i;
	
	// send an up event for each key, to make sure the server clears them all
	for (i=0 ; i<256 ; i++)
	{
		Key_Event( i, false );
	}
	
	Key_ClearStates ();
	if ( g_ClientDLL )  g_ClientDLL->IN_ClearStates ();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *instance - 
//			*basedir - 
//			*cmdline - 
//			launcherFactory - 
//-----------------------------------------------------------------------------
#ifndef SWDS
#include "gl_matsysiface.h"

bool RunListenServer( void *instance, char *basedir, CreateInterfaceFn launcherFactory )
{
	// Store off the app system factory...
	g_AppSystemFactory = launcherFactory;

	bool restart = false;

	eng->SetQuitting( IEngine::QUIT_NOTQUITTING );

	registry->Init();

	VideoMode_Create();

	TRACEINIT( FileSystem_Init(g_AppSystemFactory), FileSystem_Shutdown() );

	host_parms.basedir = basedir;

	TRACEINIT( COM_InitFilesystem(), COM_ShutdownFileSystem() );

	TRACEINIT( Shader_Init(), Shader_Shutdown() );

	if ( videomode->Init( (void *)instance ) )
	{
		Shader_SetMode( game->GetMainWindow(), videomode->IsWindowedMode() );

		// Initialize general game stuff and create the main window
		if ( game->Init( ( void *)instance ) )
		{
			// Start up the game engine
			if ( eng->Load( 
				false,				// dedicated?
				basedir ) )			// basedir ( d:\\hl1 )					
			{

				// Main message pump
				while ( 1 )
				{
					MSG msg;

					// Pump messages
					while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
					{
						TranslateMessage( &msg );
						DispatchMessage( &msg );
					}

					game->DispatchAllStoredGameMessages();

					// Bail if someone wants to quit.
					if ( eng->GetQuitting() != IEngine::QUIT_NOTQUITTING )
					{
						if ( eng->GetQuitting() != IEngine::QUIT_TODESKTOP )
						{
							restart = true;
						}
						break;
					}

					// Run engine frame
					g_pVCR->SyncToken( "Frame" );
					eng->Frame();
				}

				// Unload GL, Sound, etc.
				eng->Unload();
			}

			// Shut down memory, etc.
			game->Shutdown();
		}

		videomode->Shutdown();
	}

	TRACESHUTDOWN( Shader_Shutdown() );

	TRACESHUTDOWN( COM_ShutdownFileSystem() );

	TRACESHUTDOWN( FileSystem_Shutdown() );

	registry->Shutdown();

	return restart;
}

//-----------------------------------------------------------------------------
// Purpose: Expose engine interface to launcher
//-----------------------------------------------------------------------------
class CEngineAPI : public IEngineAPI
{
public:
	bool Run( void *instance, char *basedir, CreateInterfaceFn launcherFactory );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 0 == normal, 1 == dedicated server
//			*instance - 
//			*basedir - 
//			*cmdline - 
//			launcherFactory - 
//-----------------------------------------------------------------------------
bool CEngineAPI::Run( void *instance, char *basedir, CreateInterfaceFn launcherFactory )
{
	return RunListenServer( instance, basedir, launcherFactory );
}

EXPOSE_SINGLE_INTERFACE( CEngineAPI, IEngineAPI, VENGINE_LAUNCHER_API_VERSION );
#endif

//-----------------------------------------------------------------------------
// Purpose: Expose engine interface to launcher
//-----------------------------------------------------------------------------
class CDedicatedServerAPI : public IDedicatedServerAPI
{
public:
	bool		Init( const char *rootdir, CreateInterfaceFn launcherFactory );
	void		Shutdown( void );

	bool		RunFrame( void );

	void		AddConsoleText( char *text );
	void		UpdateStatus(float *fps, int *nActive, int *nMaxPlayers, char *pszMap);
};


/*
==================
BuildMapCycleListHints

Creates the hint list for a multiplayer map rotation from the map cycle.
The map cycle message is a text string with CR/CRLF separated lines.
	-removes comments
	-removes arguments
*/
char *szCommonPreloads  = "MP_Preloads";
char *szReslistsBaseDir = "reslists2";
char *szReslistsExt     = ".lst";

int BuildMapCycleListHints(char **hints)
{
	char szMap[ MAX_OSPATH + 2 ]; // room for one path plus <CR><LF>
	unsigned int length;
	char *pFileList;
	char szMod[MAX_OSPATH];

	// Determine the mod directory.
	COM_FileBase(com_gamedir, szMod);

	// Open mapcycle.txt
	char cszMapCycleTxtFile[MAX_OSPATH];
	Q_snprintf(cszMapCycleTxtFile, sizeof( cszMapCycleTxtFile ), "%s\\mapcycle.txt", szMod);
	FileHandle_t pFile = g_pFileSystem->Open(cszMapCycleTxtFile, "rb");
	if ( pFile == FILESYSTEM_INVALID_HANDLE )
	{
		Con_Printf("Unable to open %s", cszMapCycleTxtFile);
		return 0;
	}

	// Start off with the common preloads.
	Q_snprintf(szMap, sizeof( szMap ), "%s\\%s\\%s%s\r\n", szReslistsBaseDir, szMod, szCommonPreloads, szReslistsExt);
	int hintsSize = strlen(szMap) + 1;
	*hints = (char*)malloc( hintsSize );
	if ( *hints == NULL )
	{
		Con_Printf("Unable to allocate memory for map cycle hints list");
		return 0;
	}
	strcpy(*hints, szMap);
		
	// Read in and parse mapcycle.txt
	length = g_pFileSystem->Size(pFile);
	if ( length )
	{
		pFileList = (char *)malloc(length);
		if (   pFileList 
			&& ( 1 == g_pFileSystem->Read(pFileList, length, pFile) )
		   )
		{
			while ( 1 )
			{
				pFileList = COM_Parse( pFileList );
				if ( strlen( com_token ) <= 0 )
					break;

				Q_strncpy(szMap, com_token, sizeof(szMap)-1);
				szMap[sizeof(szMap)-1] = '\0';

				// Any more tokens on this line?
				if ( COM_TokenWaiting( pFileList ) )
				{
					pFileList = COM_Parse( pFileList );
				}

				char mapLine[sizeof(szMap)];
				Q_snprintf(mapLine, sizeof(mapLine), "%s\\%s\\%s%s\r\n", szReslistsBaseDir, szMod, szMap, szReslistsExt);
				*hints = (char*)realloc(*hints, strlen(*hints) + 1 + strlen(mapLine) + 1); // count NULL string terminators
				if ( *hints == NULL )
				{
					Con_Printf("Unable to reallocate memory for map cycle hints list");
					return 0;
				}
				Q_strncat(*hints, mapLine, hintsSize);
			}
		}
	}

	g_pFileSystem->Close(pFile);

	// Tack on <moddir>\mp_maps.txt to the end to make sure we load reslists for all multiplayer maps we know of
	Q_snprintf(szMap, sizeof( szMap ), "%s\\%s\\mp_maps.txt\r\n", szReslistsBaseDir, szMod);
	*hints = (char*)realloc(*hints, strlen(*hints) + 1 + strlen(szMap) + 1); // count NULL string terminators
	Q_strncat(*hints, szMap, hintsSize);

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 0 == normal, 1 == dedicated server
//			*instance - 
//			*basedir - 
//			*cmdline - 
//			launcherFactory - 
//-----------------------------------------------------------------------------
bool CDedicatedServerAPI::Init( const char *rootdir, CreateInterfaceFn launcherFactory )
{
	// Store off the app system factory...
	g_AppSystemFactory = launcherFactory;

	dedicated = ( IDedicatedExports * )launcherFactory( VENGINE_DEDICATEDEXPORTS_API_VERSION, NULL );
	if ( !dedicated )
		return false;

	eng->SetQuitting( IEngine::QUIT_NOTQUITTING );

	// AR: does the dedicated server need registry access? Probably not, so leave this commented out for now...
	//	registry->Init();

	TRACEINIT( FileSystem_Init(g_AppSystemFactory), FileSystem_Shutdown() );

	host_parms.basedir = (char *)rootdir;

	TRACEINIT( COM_InitFilesystem(), COM_ShutdownFileSystem() );

	TRACEINIT( Shader_InitDedicated(), Shader_Shutdown() );

	// Initialize general game stuff and create the main window
	if ( game->Init( NULL ) )
	{
		// Start up the game engine
		if ( eng->Load( 
			true,  // dedicated?
			rootdir ) )				   // rootdir ( d:\\hl2 )
		{

			// If we're using STEAM, pass the map cycle list as resource hints...
#if LATER
			if ( COM_CheckParm(STEAM_PARM) )
			{
				char *hints;
				if ( BuildMapCycleListHints(&hints) )
				{
					g_pFileSystem->HintResourceNeed(hints, true);
				}
				if ( hints )
				{
					free(hints);
				}
			}
#endif
			return true;
		}
	}

	return false;
}

void CDedicatedServerAPI::Shutdown( void )
{
	// Unload GL, Sound, etc.
	eng->Unload();

	// Shut down memory, etc.
	game->Shutdown();

	TRACESHUTDOWN( Shader_Shutdown() );

	TRACESHUTDOWN( COM_ShutdownFileSystem() );

	TRACESHUTDOWN( FileSystem_Shutdown() );

	// AR: does the dedicated server need registry access? Probably not, so leave this commented out for now...
	//	registry->Shutdown();
}

bool CDedicatedServerAPI::RunFrame( void )
{
	// Bail if someone wants to quit.
	if ( eng->GetQuitting() != IEngine::QUIT_NOTQUITTING )
	{
		return false;
	}

	// Run engine frame
	eng->Frame();
	return true;
}

void CDedicatedServerAPI::AddConsoleText( char *text )
{
	Cbuf_AddText( text );
}

void CDedicatedServerAPI::UpdateStatus(float *fps, int *nActive, int *nMaxPlayers, char *pszMap)
{
	Host_GetHostInfo( fps, nActive, nMaxPlayers, pszMap );
}


EXPOSE_SINGLE_INTERFACE( CDedicatedServerAPI, IDedicatedServerAPI, VENGINE_HLDS_API_VERSION );

#ifndef SWDS
#include "igameuifuncs.h"
#include "kbutton.h"

class CGameUIFuncs : public IGameUIFuncs
{
public:
	bool		IsKeyDown( char const *keyname, bool& isdown )
	{
		kbutton_t *key = g_ClientDLL ? g_ClientDLL->IN_FindKey( keyname ) : NULL;
		if ( !key )
			return false;

		isdown = ( key->state & 1 ) ? true : false;

		return true;
	}

	const char	*Key_NameForKey( int keynum )
	{
		return ::Key_KeynumToString( keynum );
	}

	const char	*Key_BindingForKey( int keynum )
	{
		return ::Key_BindingForKey( keynum );
	}

	void		GetVideoModes( struct vmode_s **liststart, int *count )
	{
		VideoMode_GetVideoModes( liststart, count );
	}

	void		GetCurrentVideoMode( int *wide, int *tall, int *bpp )
	{
		VideoMode_GetCurrentVideoMode( wide, tall, bpp );
	}

	void		GetCurrentRenderer( char *name, int namelen, int *windowed )
	{
		VideoMode_GetCurrentRenderer( name, namelen, windowed );
	}

	virtual vgui::KeyCode GetVGUI2KeyCodeForBind( const char *bind )
	{
		int engineKeyCode = 0;
		char *keyname = const_cast<char *>( Key_NameForBinding( bind ) );
		if( keyname )
		{
			engineKeyCode = Key_StringToKeynum( _strupr( keyname ) ) ;
		}

		if( keyname )	
		{
			int virtualKey = MapEngineKeyToVirtualKey( engineKeyCode );
			return vgui::system()->KeyCode_VirtualKeyToVGUI( virtualKey );
		}
		else
		{
			return vgui::KEY_NONE;
		}
	}
};

EXPOSE_SINGLE_INTERFACE( CGameUIFuncs, IGameUIFuncs, VENGINE_GAMEUIFUNCS_VERSION );

#endif
