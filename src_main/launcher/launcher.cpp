// launcher.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <stdio.h>
#include "vstdlib/icommandline.h"
#include "engine_launcher_api.h"
#include "iengine.h"
#include "tier0/vcrmode.h"
#include "IFileSystem.h"
#include "interface.h"
#include "tier0/dbg.h"
#include "iregistry.h"
#include "appframework/iappsystem.h"
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include "tier0/platform.h"
#include "tier0/memalloc.h"
#include "filesystem.h"
#include <direct.h>



//-----------------------------------------------------------------------------
// Modules...
//-----------------------------------------------------------------------------
static CSysModule *s_hMatSystemModule;	
static CSysModule *s_hVguiMatSurfaceModule;	
static CSysModule *s_hEngineModule;
static CSysModule *s_hVGuiModule;

static CreateInterfaceFn s_MaterialSystemFactory;
static CreateInterfaceFn s_VGuiMatSurfaceFactory;
static CreateInterfaceFn s_EngineFactory;
static CreateInterfaceFn s_VGuiFactory;

static char g_szBasedir[ MAX_PATH ];
static char g_szGamedir[ MAX_PATH ];
//-----------------------------------------------------------------------------
// Spew function!
//-----------------------------------------------------------------------------
SpewRetval_t LauncherDefaultSpewFunc( SpewType_t spewType, char const *pMsg )
{
	OutputDebugString( pMsg );
	switch( spewType )
	{
	case SPEW_MESSAGE:
	case SPEW_WARNING:
	case SPEW_LOG:
		OutputDebugString( pMsg );
		return SPEW_CONTINUE;

	case SPEW_ASSERT:
	case SPEW_ERROR:
	default:
		::MessageBox( NULL, pMsg, "Error!", MB_OK );
		return SPEW_DEBUGGER;
	}
}


//-----------------------------------------------------------------------------
// Implementation of VCRHelpers.
//-----------------------------------------------------------------------------
class CVCRHelpers : public IVCRHelpers
{
public:
	virtual void	ErrorMessage( const char *pMsg )
	{
		NOVCR(
			::MessageBox( NULL, pMsg, "VCR Error", MB_OK )
		);
	}

	virtual void*	GetMainWindow()
	{
		//return game->GetMainWindow();
		return NULL;
	}
};

static CVCRHelpers g_VCRHelpers;



//-----------------------------------------------------------------------------
// Purpose: 
// Output : char
//-----------------------------------------------------------------------------
char *GetGameDirectory( void )
{
	return g_szGamedir;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char
//-----------------------------------------------------------------------------
char *GetBaseDirectory( void )
{
	return g_szBasedir;
}


//-----------------------------------------------------------------------------
// Gets the executable name
//-----------------------------------------------------------------------------
static bool GetExecutableName( char *out )
{
	if ( !::GetModuleFileName( ( HINSTANCE )GetModuleHandle( NULL ), out, MAX_PATH ) )
	{
		return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Return the directory where this .exe is running from
// Output : char
//-----------------------------------------------------------------------------
static const char *UTIL_GetExecutableDir( )
{
	static char	exedir[ MAX_PATH ];

	exedir[ 0 ] = 0;
	if ( !GetExecutableName( exedir ) )
		return NULL;

	char *pSlash;
	char *pSlash2;
	pSlash = strrchr( exedir,'\\' );
	pSlash2 = strrchr( exedir,'/' );
	if ( pSlash2 > pSlash )
	{
		pSlash = pSlash2;
	}
	if (pSlash)
	{
		*pSlash = 0;
	}

	// Return the bin directory as the executable dir if it's not in there
	// because that's really where we're running from...
	char *p = Q_stristr( exedir, "bin" );
	if ( !p )
	{
		strcat( exedir, "\\bin" );
	}

	return exedir;
}


//-----------------------------------------------------------------------------
// An aggregate class factory encompassing all factories in all loaded DLLs
//-----------------------------------------------------------------------------
void* LauncherFactory(const char *pName, int *pReturnCode)
{
	void *pRetVal = NULL;

	// FIXME: Really, we should just load the DLLs in order
	// and search in the order loaded. This is sort of a half-measure hack
	// to get to where we need to go

	// First ask ourselves
	pRetVal = Sys_GetFactoryThis()( pName, pReturnCode );
	if (pRetVal)
		return pRetVal;

	// First ask the file system...
	pRetVal = GetFileSystemFactory()( pName, pReturnCode );
	if (pRetVal)
		return pRetVal;

	// Next the material system...
	pRetVal = s_MaterialSystemFactory( pName, pReturnCode );
	if (pRetVal)
		return pRetVal;

	// Next the mat surface
	pRetVal = s_VGuiMatSurfaceFactory( pName, pReturnCode );
	if (pRetVal)
		return pRetVal;

	// Next the engine
	pRetVal = s_EngineFactory( pName, pReturnCode );
	if (pRetVal)
		return pRetVal;

	// Next vgui2
	pRetVal = s_VGuiFactory( pName, pReturnCode );
	if (pRetVal)
		return pRetVal;

	return NULL;
}


//-----------------------------------------------------------------------------
// Loads, unloads major systems
//-----------------------------------------------------------------------------
static bool LoadAppSystems( )
{
	// Start up the file system
	FileSystem_LoadFileSystemModule();
	if (!FileSystem_Init( ))
		return false;

	// Load up the material system
	s_hMatSystemModule = FileSystem_LoadModule("materialsystem.dll");
	if (!s_hMatSystemModule)
	{
		Error( "Unable to load materialsystem.dll" );
		return false;
	}

	s_MaterialSystemFactory = Sys_GetFactory(s_hMatSystemModule);
	if (!s_MaterialSystemFactory)
	{
		Error( "Unable to get materialsystem factory" );
		return false;
	}

	// Load up the engine
	s_hEngineModule = FileSystem_LoadModule( "engine.dll" );
	if ( !s_hEngineModule )
	{
		Error( "Unable to load engine.dll" );
		return false;
	}

	s_EngineFactory = Sys_GetFactory( s_hEngineModule );
	if ( !s_EngineFactory )
	{
		Error( "Unable to get engine factory" );
		return false;
	}

	// Load up vgui2
	s_hVGuiModule = FileSystem_LoadModule( "vgui2.dll" );
	if ( !s_hVGuiModule )
	{
		Error( "Unable to load vgui2.dll" );
		return false;
	}

	s_VGuiFactory = Sys_GetFactory( s_hVGuiModule );
	if ( !s_VGuiFactory )
	{
		Error( "Unable to get vgui2 factory" );
		return false;
	}

	// Load up the mat system surface
	s_hVguiMatSurfaceModule = FileSystem_LoadModule("vguimatsurface.dll");
	if (!s_hVguiMatSurfaceModule)
	{
		Error( "Unable to load vguimatsurface.dll" );
		return false;
	}

	s_VGuiMatSurfaceFactory = Sys_GetFactory(s_hVguiMatSurfaceModule);
	if (!s_VGuiMatSurfaceFactory)
	{
		Error( "Unable to get vguimatsurface factory" );
		return false;
	}

	return true;
}


static void UnloadAppSystems()
{
	if (s_hVguiMatSurfaceModule)
	{
		FileSystem_UnloadModule(s_hVguiMatSurfaceModule);
		s_VGuiMatSurfaceFactory = NULL;
		s_hVguiMatSurfaceModule = 0;
	}

	if ( s_hVGuiModule )
	{
		FileSystem_UnloadModule( s_hVGuiModule );
		s_VGuiFactory = NULL;
		s_hVGuiModule = 0;
	}

	if ( s_hEngineModule )
	{
		FileSystem_UnloadModule( s_hEngineModule );
		s_EngineFactory = NULL;
		s_hEngineModule = 0;
	}

	if (s_hMatSystemModule)
	{
		FileSystem_UnloadModule(s_hMatSystemModule);

		s_MaterialSystemFactory = NULL;
		s_hMatSystemModule = 0;
	}

	FileSystem_UnloadFileSystemModule();
}


//-----------------------------------------------------------------------------
// System connection...
//-----------------------------------------------------------------------------
static bool InitAppSystems( void* hInstance )
{
	vgui::ISurface *matsurf = static_cast<vgui::ISurface *>( s_VGuiMatSurfaceFactory( VGUI_SURFACE_INTERFACE_VERSION, NULL ) );
	if ( matsurf )
	{
		matsurf->Connect( LauncherFactory );
	}
	else
	{
		Error( "Unable to get vguimatsurface IAppSystem * interface" );
	}

	return true;
}

static void ShutdownAppSystems()
{
	// Shut down the file system
	FileSystem_Shutdown();
}

#define HARDWARE_ENGINE "engine.dll"

// This is a big chunk of uninitialized memory that we reserve for loading blobs into.
// For more information, see LoadBlob.cpp in the engine
//char g_rgchBlob[0x03800000];


//-----------------------------------------------------------------------------
// Purpose: Return the directory where this .exe is running from
// Output : char
//-----------------------------------------------------------------------------
void UTIL_GetBaseAndGameDir( char *basedir, char *gamedir )
{
	int j;
	char *pBuffer = NULL;

	basedir[ 0 ] = 0;
	gamedir[ 0 ] = 0;

	if ( GetExecutableName( basedir ) )
	{
		_splitpath
		( 
			basedir, // Input
			NULL,  // drive
			NULL,  // dir
			gamedir, // filename
			NULL // extension
		);

		pBuffer = strrchr( basedir,'\\' );
		if ( *pBuffer )
		{
			*(pBuffer+1) = '\0';
		}

		j = strlen( basedir );
		if (j > 0)
		{
			if ( ( basedir[ j-1 ] == '\\' ) || 
				 ( basedir[ j-1 ] == '/' ) )
			{
				basedir[ j-1 ] = 0;
			}
		}
	}

	char const *pOverrideDir = CommandLine()->CheckParm( "-basedir" );
	if ( pOverrideDir )
	{
		strcpy( basedir, pOverrideDir );
	}
}


//-----------------------------------------------------------------------------
// Purpose: The real entry point for the application
// Input  : hInstance - 
//			hPrevInstance - 
//			lpCmdLine - 
//			nCmdShow - 
// Output : int APIENTRY
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) int LauncherMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	// Register myself as the primary thread.
	//Plat_RegisterPrimaryThread();

	// Hook the debug output stuff.
	SpewOutputFunc( LauncherDefaultSpewFunc );

	// Quickly check the hardware key, essentially a warning shot.  
	if( !Plat_VerifyHardwareKeyPrompt() )
	{
		return -1;
	}

	// Start VCR mode?
	const char *filename;
	CommandLine()->CreateCmdLine( g_pVCR->Hook_GetCommandLine() );
	if( CommandLine()->CheckParm( "-vcrrecord", &filename ) )
	{
		if ( !g_pVCR->Start( filename, true, &g_VCRHelpers ) )
		{
			Error( "-vcrrecord: can't open '%s' for writing.\n", filename );
			return -1;
		}
	}
	else if( CommandLine()->CheckParm( "-vcrplayback", &filename ) )
	{
		if ( !g_pVCR->Start( filename, false, &g_VCRHelpers ) )
		{
			Error( "-vcrplayback: can't open '%s' for reading.\n", filename );
			return -1;
		}
	}

	// initialize winsock
	WSAData wsaData;
	int nReturnCode = ::WSAStartup( MAKEWORD(2,0), &wsaData );

	registry->Init();

	CommandLine()->CreateCmdLine( g_pVCR->Hook_GetCommandLine() );

	UTIL_GetBaseAndGameDir( GetBaseDirectory(), GetGameDirectory() );

	// Set default game directory
	if ( !CommandLine()->CheckParm( "-defaultgamedir" ) )
	{
		CommandLine()->AppendParm( "-defaultgamedir", GetGameDirectory() );
	}

	bool memdump = CommandLine()->CheckParm( "-leakcheck" ) ? true : false;

	_chdir( GetBaseDirectory() );

	bool restart = true;
	while ( restart )
	{
		// Set engine string (from registry or command line override), 
		//  remove -gl, -d3d, etc. from command line and write current value
		//  into registry
		restart = false;

		if ( LoadAppSystems() )
		{
			// NOTE: This is sort of a hack required because we're running hl2/hl2.exe
			// instead of running directly from the bin directory
			g_pFileSystem->RemoveSearchPath( NULL, "EXECUTABLE_PATH" );
			g_pFileSystem->AddSearchPath( UTIL_GetExecutableDir(), "EXECUTABLE_PATH" );

			if ( InitAppSystems( (void *)hInstance ) )
			{
#if 1 // #ifdef LAUNCHER_LOADS_ENGINE_AS_DLL
				// running in debug mode, load engine via normal interface
				IEngineAPI *engineAPI = ( IEngineAPI * )LauncherFactory( VENGINE_LAUNCHER_API_VERSION, NULL );
				if ( engineAPI )
				{
					restart = engineAPI->Run( ( void * )hInstance, GetBaseDirectory(), LauncherFactory );
				}
#else  

				// blob unload information
				BlobFootprint_t g_blobfootprintClient;

				// running securely, load blob'd engine
				IEngineAPI *engineAPI = NULL;
				
				NLoadBlobFile(enginedll, &g_blobfootprintClient, &engineAPI, FALSE);

				if (engineAPI)
				{
					// Unmount and remount the filesystem as the engine will mount it on its own.
					restart = engineAPI->Run( ( void * )hInstance, UTIL_GetBaseDir(), (char *)CommandLine()->GetCmdLine(), Sys_GetFactoryThis() );
				}

				// free the blob
				FreeBlob(&g_blobfootprintClient);

			
#endif	// LAUNCHER_LOADS_ENGINE_AS_DLL
				ShutdownAppSystems();
			}

			UnloadAppSystems();
		}
		
		// Remove any overrides in case settings changed
		CommandLine()->RemoveParm( "-sw" );
		CommandLine()->RemoveParm( "-startwindowed" );
		CommandLine()->RemoveParm( "-windowed" );
		CommandLine()->RemoveParm( "-window" );
		CommandLine()->RemoveParm( "-full" );
		CommandLine()->RemoveParm( "-fullscreen" );
	};

	registry->Shutdown();

	// shutdown winsock
	::WSACleanup();

	if ( memdump )
	{
		g_pMemAlloc->DumpStats();
	}

	return 0;
}



