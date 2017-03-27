//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

//#define MATERIAL_OVERRIDE
//#define IDENTITY_TRANSFORM
//#define NO_FLEX

#define PROTECTED_THINGS_DISABLE
#include <windows.h>
#include <time.h>
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMaterialSystemStats.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "materialsystem/imaterialproxyfactory.h"
#include "FileSystem.h"
#include "IStudioRender.h"
#include "UtlVector.h"
#include "UtlMemory.h"
#include "studio.h"
#include "ImageLoader.h"
#include "clientstats.h"
#include "bone_setup.h"
#include "materialsystem/MaterialSystem_Config.h"
#include <stdlib.h>
#include "vtuneapi.h"
#include "vstdlib/icommandline.h"


static char g_pCommandLine[1024];
static int g_ArgC = 0;
static char* g_ppArgV[32];
static HWND g_HWnd = 0;
static CreateInterfaceFn g_MaterialsFactory;
static IMaterialSystem *g_pMaterialSystem = NULL;

static bool g_WindowMode = false;
static bool g_bUseEmptyShader = false;
static bool g_BenchFinished = false;
static bool g_BenchMode = false;

static int g_RenderWidth = 640;
static int g_RenderHeight = 480;
static int g_RefreshRate = 60;
static int g_LOD = 0;

static int g_NumRows = 10;
static int g_NumCols = 10;

static bool g_SoftwareTL = false;

static int g_dxLevel = 0;

static int g_LightingCombination = -1;

static IMaterial *g_pForceMaterial = NULL;

static CSysModule			*g_MaterialsDLL = NULL;

IMaterialSystemStats *g_pMaterialSystemStatsInterface = 0;
IMaterialSystemHardwareConfig* g_pMaterialSystemHardwareConfig = 0;

static bool g_Exiting = false;

// The maximum number of distinctive models each test may specify.
const int g_nMaxModels = 2;

struct IHVTestModel
{
	studiohwdata_t hwData;
	studiohwdata_t *GetVTXData() { return &hwData; }

	void SetSize( int size )
	{
		size = ((size + 0x1F) & ~0x1F);
		mdlMem.EnsureCapacity( size );
	}

	studiohdr_t *GetMDLData() 
	{
		unsigned char *pBase = mdlMem.Base();
		return (studiohdr_t *)( ((int)pBase + 0x1F) & ~0x1F );
	}

private:
	CUtlMemory<unsigned char> mdlMem;
};

struct BenchRunInfo
{
	const char *pModelName[g_nMaxModels];
	int numFrames;
	int rows;
	int cols;
	float modelSize;
};

struct BenchResults
{
	BenchResults() : totalTime( 0.0f ), totalTris( 0 ) {}
	float totalTime;
	int totalTris;
};

//#define NUM_BENCH_RUNS 3
#define NUM_BENCH_RUNS 1 

static BenchRunInfo g_BenchRuns[NUM_BENCH_RUNS] =
{
//	{ { "ihvtest_bumped" }, 30, 10, 10, 75.0f },
//	{ { "ihvtest_tree" }, 30, 20, 20, 700.0f },
//	{ { "ihvtest" }, 10, 10, 10, 75.0f },
//	{ { "ihvtest_noflex" }, 10, 10, 10 , 75.0f },
//	{ { "ihvtest", "alyx" }, 10, 10, 10, 75.0f },
	{ { "combine_soldier" }, 30, 10, 10, 75.0f },

	// This one tests a static prop with a ton of material changes on it.
//	{ { "combine_soldier_staticprop" }, 10, 10, 10, 75.0f },

	// This one tests 2 static props with a different materials on them.
//	{ { "combine_soldier_staticprop_singlemat", "combine_soldier_staticprop_singlemat2" }, 10, 30, 30, 75.0f },

	// This one tests 2 static props with the same material on both.
//	{ { "combine_soldier_staticprop_singlemat", "combine_soldier_staticprop_samemat" }, 10, 30, 30, 75.0f },

//	{ { "alyx" }, 10, 10, 10, 75.0f },
//	{ { "alyx_nobones" }, 10, 10, 10, 75.0f },
//	{ { "alyx_singlemat_nobones" }, 10, 10, 10, 75.0f },
//	{ { "alyx_singlemat" }, 10, 10, 10, 75.0f },
//	{ { "ihvtest_static" }, 30, 10, 10, 75.0f },
//	{ { "ihvtest_static_singlemat" }, 30, 10, 10, 75.0f },
};

// this is used in "-bench" mode
static IHVTestModel g_BenchModels[NUM_BENCH_RUNS][g_nMaxModels];

// this is used in non-bench mode
static IHVTestModel *g_pIHVTestModel = NULL;

//-----------------------------------------------------------------------------
// Stats
//-----------------------------------------------------------------------------
// itty bitty interface for stat time
class CStatTime : public IClientStatsTime
{
public:
	float GetTime()
	{
		return Sys_FloatTime();
	}
};

CStatTime	g_StatTime;

class CEngineStats
{
public:
	CEngineStats();

	// Allows us to hook in client stats too
	void InstallClientStats( IClientStats* pClientStats );
	void RemoveClientStats( IClientStats* pClientStats );
	
	// Tells the clients to draw its stats at a particular location
	void DrawClientStats( IClientStatsTextDisplay* pDisplay,
		int category, int x, int y, int lineSpacing );

	//
	// stats input
	//

	void BeginRun( void );
	void BeginFrame( void );

	void EndFrame( void );
	void EndRun( void );

	//
	// stats output
	// call these outside of a BeginFrame/EndFrame pair
	//

	// return the frame time in seconds for the whole system (not just graphics)
	double GetCurrentSystemFrameTime( void );
	double GetRunTime( void );
	
private:
	// Allows the client to draw its own stats
	CUtlVector<IClientStats*> m_ClientStats;

	// How many frames worth of data have we logged?
	int m_totalNumFrames;

	// frame timing data
	double m_frameStartTime;
	double m_frameEndTime;
	double m_minFrameTime;
	double m_maxFrameTime;

	// run timing data
	double m_runStartTime;
	double m_runEndTime;

	bool m_InFrame;
};



CEngineStats::CEngineStats() : m_InFrame( false ) 
{
}

void CEngineStats::RemoveClientStats( IClientStats* pClientStats )
{
	int i = m_ClientStats.Find( pClientStats );
	if (i >= 0)
		m_ClientStats.Remove(i);
}

// Allows us to hook in client stats too
void CEngineStats::InstallClientStats( IClientStats* pClientStats )
{
	m_ClientStats.AddToTail(pClientStats);

	// Hook in the time interface
	pClientStats->Init( &g_StatTime );
}

void CEngineStats::BeginRun( void )
{
	m_totalNumFrames = 0;

	int i;
	for ( i = 0; i < m_ClientStats.Size(); ++i )
	{
		m_ClientStats[i]->BeginRun();
	}

	// frame timing data
	m_runStartTime = Sys_FloatTime();
}

void CEngineStats::EndRun( void )
{
	m_runEndTime = Sys_FloatTime();

	for ( int i = 0; i < m_ClientStats.Size(); ++i )
	{
		m_ClientStats[i]->EndRun();
	}
}

void CEngineStats::BeginFrame( void )
{
	int i;
	for ( i = 0; i < m_ClientStats.Size(); ++i )
	{
		m_ClientStats[i]->BeginFrame();
	}
	m_InFrame = true;
	m_frameStartTime = Sys_FloatTime();
}

void CEngineStats::EndFrame( void )
{
	double deltaTime;
	
	m_frameEndTime = Sys_FloatTime();
	deltaTime = GetCurrentSystemFrameTime();

	m_InFrame = false;
	int i;
	for ( i = 0; i < m_ClientStats.Size(); ++i )
	{
		m_ClientStats[i]->EndFrame();
	}
}

void CEngineStats::DrawClientStats( IClientStatsTextDisplay* pDisplay,
								   int category, int x, int y, int lineSpacing )
{
	for ( int i = 0; i < m_ClientStats.Size(); ++i )
	{
		m_ClientStats[i]->DisplayStats( pDisplay, category );
	}
}

double CEngineStats::GetRunTime( void )
{
	return m_runEndTime - m_runStartTime;
}

double CEngineStats::GetCurrentSystemFrameTime( void )
{
	return m_frameEndTime - m_frameStartTime;
}

class CClientStatsTextDisplay : public IClientStatsTextDisplay
{
	// Draws the stats
	virtual void DrawStatsText( const char *fmt, ... )
	{
		va_list		argptr;
		char		msg[1024];
		
		va_start( argptr, fmt );
		vsprintf( msg, fmt, argptr );
		va_end( argptr );

//		g_pMaterialSystem->DrawDebugText( x, y, NULL, msg );
	}

	virtual void SetDrawColor( unsigned char r, unsigned char g, unsigned char b )
	{
	}

	// Sets a color based on a value and its max acceptable limit
	virtual void SetDrawColorFromStatValues( float limit, float value )
	{
	}

};

static CEngineStats g_EngineStats;
static CClientStatsTextDisplay g_ClientStatsTextDisplay;

//-----------------------------------------------------------------------------
// DummyMaterialProxyFactory...
//-----------------------------------------------------------------------------

class DummyMaterialProxyFactory : public IMaterialProxyFactory
{
public:
	virtual IMaterialProxy *CreateProxy( const char *proxyName )	{return NULL;}
	virtual void DeleteProxy( IMaterialProxy *pProxy )				{}
};
static DummyMaterialProxyFactory	g_DummyMaterialProxyFactory;

//-----------------------------------------------------------------------------
// Error...
//-----------------------------------------------------------------------------

void DisplayError( const char* pError, ... )
{
	va_list		argptr;
	char		msg[1024];
	
	va_start( argptr, pError );
	vsprintf( msg, pError, argptr );
	va_end( argptr );

	MessageBox( 0, msg, 0, MB_OK );
}

//-----------------------------------------------------------------------------
// FileSystem
//-----------------------------------------------------------------------------

static IFileSystem *g_pFileSystem;
static CSysModule *g_pFileSystemModule = NULL;
static CreateInterfaceFn g_pFileSystemFactory = NULL;
static bool FileSystem_LoadDLL( void )
{
	g_pFileSystemModule = Sys_LoadModule( "filesystem_stdio.dll" );
	assert( g_pFileSystemModule );
	if( !g_pFileSystemModule )
	{
		return false;
	}

	g_pFileSystemFactory = Sys_GetFactory( g_pFileSystemModule );
	if( !g_pFileSystemFactory )
	{
		return false;
	}
	g_pFileSystem = ( IFileSystem * )g_pFileSystemFactory( FILESYSTEM_INTERFACE_VERSION, NULL );
	assert( g_pFileSystem );
	if( !g_pFileSystem )
	{
		return false;
	}
	return true;
}

void FileSystem_UnloadDLL( void )
{
	if ( !g_pFileSystemModule )
		return;

	Sys_UnloadModule( g_pFileSystemModule );
	g_pFileSystemModule = 0;
}

void FileSystem_Init( )
{
	if( !FileSystem_LoadDLL() )
	{
		return;
	}

	g_pFileSystem->RemoveSearchPath( NULL, "GAME" );
	g_pFileSystem->AddSearchPath( "hl2", "GAME", PATH_ADD_TO_HEAD );

	switch( g_dxLevel )
	{
	case 6:
		g_pFileSystem->AddSearchPath( "hl2/platform_dx7", "GAME", PATH_ADD_TO_HEAD );
		g_pFileSystem->AddSearchPath( "hl2/platform_dx6", "GAME", PATH_ADD_TO_HEAD );
		break;
	case 7:
		g_pFileSystem->AddSearchPath( "hl2/platform_dx7", "GAME", PATH_ADD_TO_HEAD );
		break;
	default:
		break;
	}
}

void FileSystem_Shutdown( void )
{
	g_pFileSystem->Shutdown();

	FileSystem_UnloadDLL();
}


//-----------------------------------------------------------------------------
// StudioRender...
//-----------------------------------------------------------------------------
static IStudioRender *g_pStudioRender;
#define	MAXPRINTMSG	4096
static void MaterialSystem_Warning( const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

	OutputDebugString( msg );
}

// garymcthack
static void MaterialSystem_Warning( char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

	OutputDebugString( msg );
}

static void MaterialSystem_Error( char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

	MessageBox( NULL, (LPCTSTR)msg, "MaterialSystem Fatal Error", MB_OK | MB_ICONINFORMATION );
#ifdef _DEBUG
	assert( 0 );	
#endif
	exit( -1 );
}


void UpdateStudioRenderConfig( void )
{
	StudioRenderConfig_t config;
	
	config.eyeGloss = true;
	config.bEyeMove = true;
	config.fEyeShiftX = 0.0f;
	config.fEyeShiftY = 0.0f;
	config.fEyeShiftZ = 0.0f;
	config.fEyeSize = 0.0f;	
	config.bSoftwareSkin = g_SoftwareTL;
	config.bNoHardware = false;
	config.bNoSoftware = false;
	config.bTeeth = false;
	config.drawEntities = true;
#ifdef NO_FLEX
	config.bFlex = false;
#else
	config.bFlex = true;
#endif
	config.bEyes = false;
	config.bWireframe = false;
	config.bNormals = false;
	config.skin = 0;
	config.bUseAmbientCube = true;
	config.modelLightBias = 1.0f;
	
	config.fullbright = 0;
	config.bSoftwareLighting = g_SoftwareTL;
	config.pConDPrintf = MaterialSystem_Warning;
	config.pConPrintf = MaterialSystem_Warning;

	config.gamma = 2.2f;
	config.texGamma = 2.2f;
	config.overbrightFactor = 1.0;
	if (g_pMaterialSystemHardwareConfig->SupportsOverbright())
		config.overbrightFactor = 2.0f;
	config.brightness = 0.0f;
	
	config.bShowEnvCubemapOnly = false;
	
	g_pStudioRender->UpdateConfig( config );
}

void InitStudioRender( void )
{
	extern CreateInterfaceFn g_MaterialSystemClientFactory;

	CSysModule *studioRenderDLL = NULL;
	studioRenderDLL = Sys_LoadModule( "StudioRender.dll" );
	if( !studioRenderDLL )
	{
		DisplayError( "Can't load StudioRender.dll\n" );
	}
	CreateInterfaceFn studioRenderFactory = Sys_GetFactory( studioRenderDLL );
	if (!studioRenderFactory )
	{
		DisplayError( "Can't get factory for StudioRender.dll\n" );
	}
	g_pStudioRender = ( IStudioRender * )studioRenderFactory( STUDIO_RENDER_INTERFACE_VERSION, NULL );
	if (!g_pStudioRender)
	{
		DisplayError( "Unable to init studio render system version %s\n", STUDIO_RENDER_INTERFACE_VERSION );
	}

	IClientStats* pStudioStats = ( IClientStats * )studioRenderFactory( INTERFACEVERSION_CLIENTSTATS, NULL );
	if (!pStudioStats)
	{
		DisplayError( "Unable to init studio render stats version %s\n", INTERFACEVERSION_CLIENTSTATS );
	}
	g_EngineStats.InstallClientStats( pStudioStats );

	extern CreateInterfaceFn g_MaterialsFactory;
	g_pStudioRender->Init( g_MaterialsFactory, g_MaterialSystemClientFactory );
	UpdateStudioRenderConfig();
}

void ShutdownStudioRender( void )
{
	g_pStudioRender->Shutdown();
	g_pStudioRender = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Unloads the material system .dll
//-----------------------------------------------------------------------------
void UnloadMaterialSystem( void )
{
	if ( !g_MaterialsFactory )
		return;

	Sys_UnloadModule( g_MaterialsDLL );
	g_MaterialsDLL = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Loads the material system dll
//-----------------------------------------------------------------------------
void LoadMaterialSystem( void )
{
	if( g_MaterialsFactory )
	{
		return;
	}

	assert( !g_MaterialsDLL );

	g_MaterialsDLL = Sys_LoadModule( "MaterialSystem.dll" );

	g_MaterialsFactory = Sys_GetFactory( g_MaterialsDLL );
	if ( !g_MaterialsFactory )
	{
		DisplayError( "LoadMaterialSystem:  Failed to load MaterialSystem.DLL\n" );
		return;
	}

	if ( g_MaterialsFactory )
	{
		g_pMaterialSystem = (IMaterialSystem *)g_MaterialsFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
		if ( !g_pMaterialSystem )
		{
			DisplayError( "Could not get the material system interface from materialsystem.dll (a)" );
		}
	}
	else
	{
		DisplayError( "Could not find factory interface in library MaterialSystem.dll" );
	}
}

void InitMaterialSystemConfig(MaterialSystem_Config_t *pConfig)
{
	memset( pConfig, 0, sizeof(MaterialSystem_Config_t) );
	pConfig->screenGamma = 2.2f;
	pConfig->texGamma = 2.2;
	pConfig->overbright = 2;
	pConfig->bAllowCheats = false;
	pConfig->bLinearFrameBuffer = false;
	pConfig->polyOffset = 4;
	pConfig->skipMipLevels = 0;
	pConfig->lightScale = 1.0f;
	pConfig->bFilterLightmaps = true;
	pConfig->bFilterTextures = true;
	pConfig->bMipMapTextures = true;
	pConfig->bShowMipLevels = false;
	pConfig->bShowLowResImage = false;
	pConfig->bReverseDepth = false;
	pConfig->bCompressedTextures = true;
	pConfig->bBumpmap = true;
	pConfig->bShowSpecular = true;
	pConfig->bShowDiffuse = true;
	pConfig->bDrawFlat = false;
	pConfig->bLightingOnly = false;
	pConfig->bSoftwareLighting = g_SoftwareTL;
	pConfig->bEditMode = false;	// No, we're not in WorldCraft.
	pConfig->dxSupportLevel = g_dxLevel;
	pConfig->maxFrameLatency = 1;
}


CreateInterfaceFn g_MaterialSystemClientFactory;
void Shader_Init( HWND mainWindow )
{
	LoadMaterialSystem();
	assert( g_pMaterialSystem );

	// FIXME: Where do we put this?
	const char* pDLLName;
	if ( g_bUseEmptyShader )
	{
		pDLLName = "shaderapiempty";
	}
	else
	{
		pDLLName = "shaderapidx9";
	}

	// assume that IFileSystem paths have already been set via g_pFileSystem 
	g_MaterialSystemClientFactory = g_pMaterialSystem->Init( 
		pDLLName, &g_DummyMaterialProxyFactory, g_pFileSystemFactory );
	if (!g_MaterialSystemClientFactory)
	{
		DisplayError( "Unable to init shader system\n" );
	}

	int modeFlags = 0;
	if( g_WindowMode )
	{
		modeFlags |= MATERIAL_VIDEO_MODE_WINDOWED;
	}
	modeFlags |= MATERIAL_VIDEO_MODE_NO_WAIT_FOR_VSYNC;

	// Get the adapter from the command line....
	int adapter = 0;

	MaterialVideoMode_t mode;
	memset( &mode, 0, sizeof( mode ) );
	mode.m_Width = g_RenderWidth;
	mode.m_Height = g_RenderHeight;
	mode.m_Format = IMAGE_FORMAT_BGRX8888;
	mode.m_RefreshRate = g_RefreshRate;
	bool modeSet = g_pMaterialSystem->SetMode( (void*)mainWindow, adapter, mode, modeFlags );
	if (!modeSet)
	{
		DisplayError( "Unable to set mode\n" );
	}

	g_pMaterialSystemStatsInterface = (IMaterialSystemStats *)g_MaterialSystemClientFactory( 
		MATERIAL_SYSTEM_STATS_INTERFACE_VERSION, NULL );

	g_pMaterialSystemHardwareConfig = (IMaterialSystemHardwareConfig*)
		g_MaterialSystemClientFactory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, 0 );
	if ( !g_pMaterialSystemHardwareConfig )
	{
		DisplayError( "Could not get the material system hardware config interface!" );
	}

	MaterialSystem_Config_t config;
	InitMaterialSystemConfig(&config);
	g_pMaterialSystem->UpdateConfig(&config, false);
	
	if (g_pMaterialSystemStatsInterface)
		g_EngineStats.InstallClientStats( g_pMaterialSystemStatsInterface );
}

void Shader_Shutdown( HWND hwnd )
{
	// Recursive shutdown
	if ( !g_pMaterialSystem )
		return;

	g_pMaterialSystem->Shutdown( );
	g_pMaterialSystem = NULL;
	UnloadMaterialSystem();
}


#define MAX_LIGHTS 2

// If you change the number of lighting combinations, change this enum
enum
{
	LIGHTING_COMBINATION_COUNT = 10
};
enum
{
	NUM_LIGHT_TYPES = 4
};

static LightType_t g_LightCombinations[][MAX_LIGHTS] = 
{
	{ MATERIAL_LIGHT_DISABLE,		MATERIAL_LIGHT_DISABLE },			// 0
	{ MATERIAL_LIGHT_SPOT,			MATERIAL_LIGHT_DISABLE },
	{ MATERIAL_LIGHT_POINT,			MATERIAL_LIGHT_DISABLE },
	{ MATERIAL_LIGHT_DIRECTIONAL,	MATERIAL_LIGHT_DISABLE },
	{ MATERIAL_LIGHT_SPOT,			MATERIAL_LIGHT_SPOT },

	{ MATERIAL_LIGHT_SPOT,			MATERIAL_LIGHT_POINT },				// 5
	{ MATERIAL_LIGHT_SPOT,			MATERIAL_LIGHT_DIRECTIONAL },
	{ MATERIAL_LIGHT_POINT,			MATERIAL_LIGHT_POINT },
	{ MATERIAL_LIGHT_POINT,			MATERIAL_LIGHT_DIRECTIONAL },
	{ MATERIAL_LIGHT_DIRECTIONAL,	MATERIAL_LIGHT_DIRECTIONAL },		// 9
};

static const char *g_LightCombinationNames[] = 
{
	"DISABLE                ",
	"SPOT                   ",
	"POINT                  ",
	"DIRECTIONAL            ",
	"SPOT_SPOT              ",
	"SPOT_POINT             ",
	"SPOT_DIRECTIONAL       ",
	"POINT_POINT            ",
	"POINT_DIRECTIONAL      ",
	"DIRECTIONAL_DIRECTIONAL"
};

static BenchResults g_BenchResults[NUM_BENCH_RUNS][LIGHTING_COMBINATION_COUNT];

LightDesc_t g_TestLights[NUM_LIGHT_TYPES][MAX_LIGHTS];

static void FixLight( LightDesc_t *pLight )
{
	pLight->m_Range = 0.0f;
	pLight->m_Falloff = 1.0f;
	pLight->m_ThetaDot = cos( pLight->m_Theta * 0.5f );
	pLight->m_PhiDot = cos( pLight->m_Phi * 0.5f );
	pLight->m_Flags = 0;
	if( pLight->m_Attenuation0 != 0.0f )
	{
		pLight->m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0;
	}
	if( pLight->m_Attenuation1 != 0.0f )
	{
		pLight->m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1;
	}
	if( pLight->m_Attenuation2 != 0.0f )
	{
		pLight->m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2;
	}
	VectorNormalize( pLight->m_Direction );
}

static void InitTestLights( void )
{
	LightDesc_t *pLight;
	int i;
	for( i = 0; i < MAX_LIGHTS; i++ )
	{
		// MATERIAL_LIGHT_DISABLE		
		pLight = &g_TestLights[MATERIAL_LIGHT_DISABLE][i];
		pLight->m_Type = MATERIAL_LIGHT_DISABLE;
	}

	// x - right
	// y - up
	// z - front of model
	// MATERIAL_LIGHT_SPOT 0
	pLight = &g_TestLights[MATERIAL_LIGHT_SPOT][0];
	memset( pLight, 0, sizeof( LightDesc_t ) );
	pLight->m_Type = MATERIAL_LIGHT_SPOT;
	pLight->m_Color.Init( 5000.0f, 3500.0f, 3500.0f );
	pLight->m_Position.Init( 0.0f, 0.0f, 50.0f );
	pLight->m_Direction.Init( 0.0f, 0.5f, -1.0f );
	pLight->m_Attenuation0 = 0.0f;
	pLight->m_Attenuation1 = 0.0f;
	pLight->m_Attenuation2 = 1.0f;
	pLight->m_Theta = DEG2RAD( 20.0f );
	pLight->m_Phi = DEG2RAD( 30.0f );

	// MATERIAL_LIGHT_SPOT 1
	pLight = &g_TestLights[MATERIAL_LIGHT_SPOT][1];
	memset( pLight, 0, sizeof( LightDesc_t ) );
	pLight->m_Type = MATERIAL_LIGHT_SPOT;
	pLight->m_Color.Init( 3500.0f, 5000.0f, 3500.0f );
	pLight->m_Position.Init( 0.0f, 0.0f, 150.0f );
	pLight->m_Direction.Init( 0.0f, 0.5f, -1.0f );
	pLight->m_Attenuation0 = 0.0f;
	pLight->m_Attenuation1 = 0.0f;
	pLight->m_Attenuation2 = 1.0f;
	pLight->m_Theta = DEG2RAD( 20.0f );
	pLight->m_Phi = DEG2RAD( 30.0f );

	// MATERIAL_LIGHT_POINT 0
	pLight = &g_TestLights[MATERIAL_LIGHT_POINT][0];
	memset( pLight, 0, sizeof( LightDesc_t ) );
	pLight->m_Type = MATERIAL_LIGHT_POINT;
	pLight->m_Color.Init( 1500.0f, 750.0f, 750.0f );
	pLight->m_Position.Init( 200.0f, 200.0f, 200.0f );
	pLight->m_Attenuation0 = 0.0f;
	pLight->m_Attenuation1 = 1.0f;
	pLight->m_Attenuation2 = 0.0f;

	// MATERIAL_LIGHT_POINT 1
	pLight = &g_TestLights[MATERIAL_LIGHT_POINT][1];
	memset( pLight, 0, sizeof( LightDesc_t ) );
	pLight->m_Type = MATERIAL_LIGHT_POINT;
	pLight->m_Color.Init( 750.0f, 750.0f, 1500.0f );
	pLight->m_Position.Init( -200.0f, 200.0f, 200.0f );
	pLight->m_Attenuation0 = 0.0f;
	pLight->m_Attenuation1 = 1.0f;
	pLight->m_Attenuation2 = 0.0f;

	// MATERIAL_LIGHT_DIRECTIONAL 0
	pLight = &g_TestLights[MATERIAL_LIGHT_DIRECTIONAL][0];
	memset( pLight, 0, sizeof( LightDesc_t ) );
	pLight->m_Type = MATERIAL_LIGHT_DIRECTIONAL;
	pLight->m_Color.Init( 2.0f, 2.0f, 1.0f );
	pLight->m_Direction.Init( -1.0f, 0.0f, 0.0f );
	pLight->m_Attenuation0 = 1.0f;
	pLight->m_Attenuation1 = 0.0f;
	pLight->m_Attenuation2 = 0.0f;

	// MATERIAL_LIGHT_DIRECTIONAL 1
	pLight = &g_TestLights[MATERIAL_LIGHT_DIRECTIONAL][1];
	memset( pLight, 0, sizeof( LightDesc_t ) );
	pLight->m_Type = MATERIAL_LIGHT_DIRECTIONAL;
	pLight->m_Color.Init( 1.0f, 1.0f, 2.0f );
	pLight->m_Direction.Init( 1.0f, 0.0f, 0.0f );
	pLight->m_Attenuation0 = 1.0f;
	pLight->m_Attenuation1 = 0.0f;
	pLight->m_Attenuation2 = 0.0f;

	for( i = 0; i < MAX_LIGHTS; i++ )
	{
		int j;
		for( j = 0; j < NUM_LIGHT_TYPES; j++ )
		{
			FixLight( &g_TestLights[j][i] );
		}
	}
}

//-----------------------------------------------------------------------------
// Setup lighting
//-----------------------------------------------------------------------------
static void SetupLighting( int lightingCombination, Vector &lightOffset )
{
	if( lightingCombination == 0 )
	{
		g_pStudioRender->SetLocalLights( 0, NULL );
		g_pMaterialSystem->SetAmbientLight( 1.0, 1.0, 1.0 );

		static Vector white[6] = 
		{
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
		};
		g_pStudioRender->SetAmbientLightColors( white, white );
	}
	else
	{
		g_pMaterialSystem->SetAmbientLight( 0.0f, 0.0f, 0.0f );

		static Vector black[6] = 
		{
			Vector( 0.0, 0.0, 0.0 ),
			Vector( 0.0, 0.0, 0.0 ),
			Vector( 0.0, 0.0, 0.0 ),
			Vector( 0.0, 0.0, 0.0 ),
			Vector( 0.0, 0.0, 0.0 ),
			Vector( 0.0, 0.0, 0.0 ),
		};
		g_pStudioRender->SetAmbientLightColors( black, black );

		int lightID;
		LightDesc_t lightDescs[MAX_LIGHTS];
		for( lightID = 0; lightID < MAX_LIGHTS; lightID++ )
		{
			int lightType = g_LightCombinations[lightingCombination][lightID];
			lightDescs[lightID] = g_TestLights[lightType][lightID];
			lightDescs[lightID].m_Position += lightOffset;
		}

		// Feed disabled lights through?
		if( g_LightCombinations[lightingCombination][1] == MATERIAL_LIGHT_DISABLE )
			g_pStudioRender->SetLocalLights( 1, lightDescs );
		else
			g_pStudioRender->SetLocalLights( MAX_LIGHTS, lightDescs );
	}
	
}

static float s_PoseParameter[4] = { 0.0f, 0.0f, 0.0f, 0.0f };		// animation blending
static int s_Sequence = 16;
static float s_Cycle = 0.0f;

void AdvanceFrame( studiohdr_t *pStudioHdr, float dt )
{
	if (dt > 0.1)
		dt = 0.1f;

	float t = Studio_Duration( pStudioHdr, s_Sequence, s_PoseParameter );

	if (t > 0)
	{
		s_Cycle += dt / t;

		// wrap
		s_Cycle -= (int)(s_Cycle);
	}
	else
	{
		s_Cycle = 0;
	}
}

void SetUpBones ( studiohdr_t *pStudioHdr, const matrix3x4_t &modelMatrix, int boneMask )
{
	int					i;

	mstudiobone_t		*pbones;

	static Vector		pos[MAXSTUDIOBONES];
	matrix3x4_t			bonematrix;
	static Quaternion	q[MAXSTUDIOBONES];
	
	CalcPose( pStudioHdr, NULL, pos, q, s_Sequence, s_Cycle, s_PoseParameter, boneMask );

	pbones = pStudioHdr->pBone( 0 );

	for (i = 0; i < pStudioHdr->numbones; i++) 
	{
#ifndef IDENTITY_TRANSFORM
		QuaternionMatrix( q[i], bonematrix );

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1) 
		{
			ConcatTransforms( modelMatrix, bonematrix, g_pStudioRender->GetBoneToWorld( i )->matrix );
		} 
		else 
		{
			ConcatTransforms (*g_pStudioRender->GetBoneToWorld( pbones[i].parent ), bonematrix, *g_pStudioRender->GetBoneToWorld( i ) );
		}
#else
		memcpy( g_pStudioRender->GetBoneToWorld( i ), &modelMatrix[0][0], sizeof( float ) * 12 );
#endif
	}
}

//-----------------------------------------------------------------------------
// Render a frame
//-----------------------------------------------------------------------------

void RenderFrame( void )
{
	IHVTestModel *pModel = NULL;
	static int currentRun = 0;
	static int currentFrame = 0;
	static int currentLightCombo = 0;
	int modelAlternator = 0;

	if( g_BenchMode )
	{
		if( currentFrame > g_BenchRuns[currentRun].numFrames )
		{
			currentLightCombo++;
			if( currentLightCombo >= LIGHTING_COMBINATION_COUNT )
			{
				currentRun++;
				currentLightCombo = 0;
				if( currentRun >= NUM_BENCH_RUNS )
				{
					g_BenchFinished = true;
					return;
				}
			}
			currentFrame = 0;
		}
	}
	if( g_BenchMode )
	{
		pModel = &g_BenchModels[currentRun][0];		
		g_NumCols = g_BenchRuns[currentRun].cols;
		g_NumRows = g_BenchRuns[currentRun].rows;
	}
	else
	{
		pModel = g_pIHVTestModel;
	}
	assert( pModel );
	
	g_EngineStats.BeginFrame();

	g_pMaterialSystem->BeginFrame();
	g_pStudioRender->BeginFrame();

	g_pMaterialSystem->ClearBuffers( true, true );

	g_pMaterialSystem->Viewport( 0, 0, g_RenderWidth, g_RenderHeight );

	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->LoadIdentity();
	g_pMaterialSystem->PerspectiveX( 90.0f, ( g_RenderWidth / g_RenderHeight), 1.0f, 500000.0f );

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->LoadIdentity();
	if( g_BenchMode )
	{
		g_pMaterialSystem->Translate( 0.0f, 0.0f, ( float )-( g_NumCols  * g_BenchRuns[currentRun].modelSize * 0.6f ) );
	}
	else
	{
		g_pMaterialSystem->Translate( 0.0f, 0.0f, ( float )-( g_NumCols  * 80.0f * 0.5f ) );
	}

	g_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	g_pMaterialSystem->LoadIdentity();

	QAngle angles;
	angles[YAW] = -90.0f;
	angles[PITCH] = -90.0f;
	angles[ROLL] = 0.0f;

	matrix3x4_t cameraMatrix;
	AngleMatrix( angles, cameraMatrix );

	int r, c;
	int trisRendered = 0;
	float boneSetupTime = 0.0f;
	for( r = 0; r < g_NumRows; r++ )
	{
		for( c = 0; c < g_NumCols; c++ )
		{
			// If we are alternating models, select the next valid model.			
			if( g_BenchMode )
			{
				
				do
				{
					// If I pass my maximum number of models, wrap around to model 0, which must always be valid.
					if( ++modelAlternator >= g_nMaxModels )
					{
						modelAlternator = 0;
						break;
					}
				}
				while( !g_BenchRuns[currentRun].pModelName[modelAlternator] );

				pModel = &g_BenchModels[currentRun][modelAlternator];
				assert(pModel);
			}

			if( g_BenchMode )
			{
				cameraMatrix[0][3] = ( ( c + 0.5f ) - ( g_NumCols * .5f ) ) * g_BenchRuns[currentRun].modelSize;
				cameraMatrix[1][3] = ( ( float )r - ( g_NumCols * .5f ) ) * g_BenchRuns[currentRun].modelSize;
			}
			else
			{
				cameraMatrix[0][3] = ( ( c + 0.5f ) - ( g_NumCols * .5f ) ) * 75.0f;
				cameraMatrix[1][3] = ( ( float )r - ( g_NumCols * .5f ) ) * 75.0f;
			}
			
			Vector modelOrigin( cameraMatrix[0][3], cameraMatrix[1][3], 0.0f );
			
			Vector lightOffset( cameraMatrix[0][3], cameraMatrix[1][3], 0.0f );

			if (g_LightingCombination < 0)
				SetupLighting( g_BenchMode ? currentLightCombo : 0, lightOffset );
			else
				SetupLighting( g_LightingCombination, lightOffset );

			float startBoneSetupTime = Sys_FloatTime();
			int boneMask = BONE_USED_BY_VERTEX_LOD0 + g_LOD;
			SetUpBones( pModel->GetMDLData(), cameraMatrix, boneMask );
			boneSetupTime += Sys_FloatTime() - startBoneSetupTime;
			
			g_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
			g_pMaterialSystem->PushMatrix();

			DrawModelInfo_t modelInfo;
			modelInfo.m_pStudioHdr = pModel->GetMDLData();
			modelInfo.m_pHardwareData = &pModel->hwData;
			modelInfo.m_Decals = STUDIORENDER_DECAL_INVALID;
			modelInfo.m_Skin = 0;
			modelInfo.m_Body = 0;
			modelInfo.m_HitboxSet = 0;
			modelInfo.m_pClientEntity = NULL;
			modelInfo.m_Lod = g_LOD;
			modelInfo.m_ppColorMeshes = NULL;
			trisRendered += g_pStudioRender->DrawModel( modelInfo,
				modelOrigin,
				NULL, NULL );

			g_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
			g_pMaterialSystem->PopMatrix();
		}
	}

	g_pMaterialSystem->Flush( true );
	g_EngineStats.EndFrame();

	g_pStudioRender->EndFrame();
	g_pMaterialSystem->EndFrame();

	// hack - don't count the first frame in case there are any state
	// transitions computed on that frame.
	if( currentFrame != 0 )
	{
		g_BenchResults[currentRun][currentLightCombo].totalTime += g_EngineStats.GetCurrentSystemFrameTime();
		g_BenchResults[currentRun][currentLightCombo].totalTris += trisRendered;
	}

	AdvanceFrame( pModel->GetMDLData(), g_EngineStats.GetCurrentSystemFrameTime() );
#if 0 // garymcthack
	g_pMaterialSystem->DrawDebugText( 0, 0, NULL, "renderTime: %f ms", ( float )( g_EngineStats.GetCurrentSystemFrameTime() * 1000.0f ) );
	g_pMaterialSystem->DrawDebugText( 0, 12, NULL, "tris rendered: %d", trisRendered );
	g_pMaterialSystem->DrawDebugText( 0, 24, NULL, "tris/sec: %0.0f", ( float)( ( double )trisRendered / ( g_EngineStats.GetCurrentSystemFrameTime() ) ) );
	g_pMaterialSystem->DrawDebugText( 0, 36, NULL, "boneSetupTime: %f ms", ( float )( boneSetupTime * 1000.0f ) );
	g_pMaterialSystem->DrawDebugText( 0, 48, NULL, "lighting: %s", g_LightCombinationNames[g_BenchMode ? currentLightCombo : g_LightingCombination] );
	g_EngineStats.DrawClientStats( &g_ClientStatsTextDisplay, 8, 0, 60, 12 );
#endif
	g_pMaterialSystem->SwapBuffers();
	currentFrame++;
}

//-----------------------------------------------------------------------------
// Window create, destroy...
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
	case WM_PAINT:
		RenderFrame();
		break;
		// abort when ESC is hit
	case WM_CHAR:
		switch(wParam)
		{
		case VK_ESCAPE:
			SendMessage( g_HWnd, WM_CLOSE, 0, 0 );
			break;
		}
		break;
		
/*
		case VK_F1:
			g_LightingCombination = ( g_LightingCombination + 1 ) % LIGHTING_COMBINATION_COUNT;
			break;
*/

        case WM_DESTROY:
			g_Exiting = true;
            PostQuitMessage( 0 );
            return 0;
    }
	
    return DefWindowProc( hWnd, msg, wParam, lParam );
}

bool CreateAppWindow( const char* pAppName, int width, int height )
{
    // Register the window class
	WNDCLASSEX windowClass = 
	{ 
		sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        pAppName, NULL 
	};

    RegisterClassEx( &windowClass );

    // Create the application's window
    g_HWnd = CreateWindow( pAppName, pAppName,
		WS_OVERLAPPEDWINDOW, 
		0, 0, width, height,
		GetDesktopWindow(), NULL, windowClass.hInstance, NULL );
	
    ShowWindow (g_HWnd, SW_SHOWDEFAULT);
	
	return (g_HWnd != 0);
}

void DestroyAppWindow()
{
	if (g_HWnd)
		DestroyWindow( g_HWnd );
}

//-----------------------------------------------------------------------------
// Command-line...
//-----------------------------------------------------------------------------

int FindCommand( const char *pCommand )
{
	for ( int i = 0; i < g_ArgC; i++ )
	{
		if ( !stricmp( pCommand, g_ppArgV[i] ) )
			return i;
	}
	return -1;
}

const char* CommandArgument( const char *pCommand )
{
	int cmd = FindCommand( pCommand );
	if ((cmd != -1) && (cmd < g_ArgC - 1))
	{
		return g_ppArgV[cmd+1];
	}
	return 0;
}

void SetupCommandLine( const char* pCommandLine )
{
	strcpy( g_pCommandLine, pCommandLine );

	g_ArgC = 0;

	char* pStr = g_pCommandLine;
	while ( *pStr )
	{
		pStr = strtok( pStr, " " );
		if( !pStr )
		{
			break;
		}
		g_ppArgV[g_ArgC++] = pStr;
		pStr += strlen(pStr) + 1;
	}
}

static bool LoadModel( const char *modelName, IHVTestModel *pModel )
{
	if( !modelName )
	{
		return false;
	}
	
	FileHandle_t vtxFileHandle;
	FileHandle_t mdlFileHandle;

	char path[256];
	sprintf( path, "models/%s.mdl", modelName );
	mdlFileHandle = g_pFileSystem->Open( path, "rb" );
	if( !mdlFileHandle )
	{
		DisplayError( "can't load model" );
		return false;
	}

	if( g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() ||
		g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 70 )
	{
		sprintf( path, "models/%s.dx80.vtx", modelName );
	}
	else
	{
		if( g_pMaterialSystemHardwareConfig->MaxBlendMatrices() > 2 )
		{
			sprintf( path, "models/%s.dx7_3bone.vtx", modelName );
		}
		else
		{
			sprintf( path, "models/%s.dx7_2bone.vtx", modelName );
		}
	}
	vtxFileHandle = g_pFileSystem->Open( path, "rb" );
	if( !vtxFileHandle )
	{
		sprintf( path, "models/%s.dx7_2bone.vtx", modelName );
		vtxFileHandle = g_pFileSystem->Open( path, "rb" );
		if( !vtxFileHandle )
		{
			DisplayError( "can't load vtx file" );
			g_pFileSystem->Close( mdlFileHandle );
			return false;
		}
	}

	int vtxSize = g_pFileSystem->Size( vtxFileHandle );
	int mdlSize = g_pFileSystem->Size( mdlFileHandle );
	
	CUtlMemory<unsigned char> vtxMem;
	vtxMem.EnsureCapacity( vtxSize );
	pModel->SetSize( mdlSize );

	g_pFileSystem->Read( vtxMem.Base(), vtxSize, vtxFileHandle );
	g_pFileSystem->Read( pModel->GetMDLData(), mdlSize, mdlFileHandle );
	
	g_pFileSystem->Close( vtxFileHandle );
	g_pFileSystem->Close( mdlFileHandle );

	studiohdr_t *pStudioHdr = pModel->GetMDLData();
	assert( pStudioHdr );

#ifdef IDENTITY_TRANSFORM
	int i;
	for( i = 0; i < pStudioHdr->numbones; i++ )
	{
		mstudiobone_t *pBone = pStudioHdr->pBone( i );
		pBone->poseToBone[0][0] = 1.0f;
		pBone->poseToBone[0][1] = 0.0f;
		pBone->poseToBone[0][2] = 0.0f;
		pBone->poseToBone[0][3] = 0.0f;

		pBone->poseToBone[1][0] = 0.0f;
		pBone->poseToBone[1][1] = 1.0f;
		pBone->poseToBone[1][2] = 0.0f;
		pBone->poseToBone[1][3] = 0.0f;

		pBone->poseToBone[2][0] = 0.0f;
		pBone->poseToBone[2][1] = 0.0f;
		pBone->poseToBone[2][2] = 1.0f;
		pBone->poseToBone[2][3] = 0.0f;
	}
#endif // IDENTITY_TRANSFORM
	
	g_pStudioRender->LoadModel( pStudioHdr, vtxMem.Base(), &pModel->hwData );
	return true;
}

static void CalcWindowSize( int desiredRenderingWidth, int desiredRenderingHeight,
							int *windowWidth, int *windowHeight )
{
    int     borderX, borderY;
	borderX = (GetSystemMetrics(SM_CXFIXEDFRAME) + 1) * 2;
	borderY = (GetSystemMetrics(SM_CYFIXEDFRAME) + 1) * 2 + GetSystemMetrics(SM_CYSIZE) + 1;
	*windowWidth = desiredRenderingWidth + borderX;
	*windowHeight = desiredRenderingHeight + borderY;
}

static void SetIdentityBones( void )
{
	int i;
	for( i = 0; i < NUM_MODEL_TRANSFORMS; i++ )
	{
		g_pMaterialSystem->MatrixMode( MATERIAL_MODEL_MATRIX( i ) );
		g_pMaterialSystem->LoadIdentity();
	}
}

bool Init( const char* pCommands, IHVTestModel *pIHVTestModel )
{
	// Store off the command line...
	SetupCommandLine( pCommands );

	if( FindCommand( "-bench" ) >= 0 )
	{
		g_BenchMode = true;
	}
	
	if( FindCommand( "-null" ) >= 0 )
	{
		g_bUseEmptyShader = true;
	}

	if( !g_BenchMode && CommandArgument( "-i" ) == 0 )
	{
		DisplayError("usage: ihvtest1 -bench -i <input> [-null] [-window|-fullscreen] [-width n] [-height n] [-refresh n] [-lod n] [-light n] [-dx n]");
		return false;
	}

	if( FindCommand( "-softwaretl" ) >= 0 )
	{
		g_SoftwareTL = true;
	}

	// Explicitly in window/fullscreen mode?
	if (FindCommand("-window") >= 0)
		g_WindowMode = true;
	else if (FindCommand("-fullscreen") >= 0)
		g_WindowMode = false;

extern void Sys_InitFloatTime( void ); //garymcthack
	Sys_InitFloatTime();

	if( CommandArgument( "-rowcol" ) )
	{
		g_NumRows = g_NumCols = atoi( CommandArgument( "-rowcol" ) );
	}
	
	/* figure out which LOD we are going to render */
	if( CommandArgument( "-lod" ) )
	{
		g_LOD = atoi( CommandArgument( "-lod" ) );
	}

	// Check for forced dx level.
	if( CommandArgument( "-dx" ) )
	{
		g_dxLevel = atoi( CommandArgument( "-dx" ) );
	}
	
	FileSystem_Init( );

	/* figure out g_Renderwidth and g_RenderHeight */
	g_RenderWidth = -1;
	g_RenderHeight = -1;

	if( CommandArgument( "-width" ) )
	{
		g_RenderWidth = atoi( CommandArgument( "-width" ) );
	}
	if( CommandArgument( "-height" ) )
	{
		g_RenderHeight = atoi( CommandArgument( "-height" ) );
	}

	if( g_RenderWidth == -1 && g_RenderHeight == -1 )
	{
		g_RenderWidth = 640;
		g_RenderHeight = 480;
	}
	else if( g_RenderWidth != -1 && g_RenderHeight == -1 )
	{
		switch( g_RenderWidth )
		{
		case 320:
			g_RenderHeight = 240;
			break;
		case 512:
			g_RenderHeight = 384;
			break;
		case 640:
			g_RenderHeight = 480;
			break;
		case 800:
			g_RenderHeight = 600;
			break;
		case 1024:
			g_RenderHeight = 768;
			break;
		case 1280:
			g_RenderHeight = 1024;
			break;
		case 1600:
			g_RenderHeight = 1200;
			break;
		default:
			DisplayError( "Can't figure out window dimensions!!" );
			exit( -1 );
			break;
		}
	}

	if( g_RenderWidth == -1 || g_RenderHeight == -1 )
	{
		DisplayError( "Can't figure out window dimensions!!" );
		exit( -1 );
	}

	// figure out g_RefreshRate
	if( CommandArgument( "-refresh" ) )
	{
		g_RefreshRate = atoi( CommandArgument( "-refresh" ) );
	}
	
	if( CommandArgument( "-light" ) )
	{
		g_LightingCombination = atoi( CommandArgument( "-light" ) );
		if( g_LightingCombination < 0 )
		{
			g_LightingCombination = 0;
		}
		if( g_LightingCombination >= LIGHTING_COMBINATION_COUNT )
		{
			g_LightingCombination = LIGHTING_COMBINATION_COUNT - 1;
		}
	}
	
	int windowWidth, windowHeight;
	CalcWindowSize( g_RenderWidth, g_RenderHeight, &windowWidth, &windowHeight );

	if( !CreateAppWindow( "ihvtest1", windowWidth, windowHeight ) )
	{
		return false;
	}

	Shader_Init( g_HWnd );
	InitStudioRender();

	g_pForceMaterial = g_pMaterialSystem->FindMaterial( "models/alyx/thigh", NULL );
#ifdef MATERIAL_OVERRIDE
	g_pStudioRender->ForcedMaterialOverride( g_pForceMaterial );
#endif
	SetIdentityBones();

	InitTestLights();

	if( g_BenchMode )
	{
		int i;
		for( i = 0; i < NUM_BENCH_RUNS; i++ )
		{
			// Load each of the potentially alternating models:
			int k;
			for( k = 0; k < g_nMaxModels; k++ )
			{
				if( g_BenchRuns[i].pModelName[k] ) 
				{
					if( !LoadModel( g_BenchRuns[i].pModelName[k], &g_BenchModels[i][k] ) )
					{
						return false;
					}
				}
			}
		}
	}
	else
	{
		if( !LoadModel( CommandArgument( "-i" ), pIHVTestModel ) )
		{
			return false;
		}
	}
	g_pMaterialSystem->CacheUsedMaterials();
	
	return true;
}

//-----------------------------------------------------------------------------
// Pump windows messages
//-----------------------------------------------------------------------------

void PumpWindowsMessages ()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	InvalidateRect(g_HWnd, NULL, false);
	UpdateWindow(g_HWnd);
}

//-----------------------------------------------------------------------------
// Update
//-----------------------------------------------------------------------------

bool Update()
{
	PumpWindowsMessages();
	return g_Exiting;
}

static void WriteBenchResults( void )
{
	if( !g_BenchMode )
	{
		return;
	}

	FILE *fp = fopen( "ihvtest1.txt", "a+" );
	assert( fp );
	if( !fp )
	{
		return;
	}

	fprintf( fp, "------------------------------------------------------------------\n" );
	
	time_t ltime;
	time( &ltime );
	
	fprintf( fp, "%s\n", GetCommandLine() );
	fprintf( fp, "Run at: %s", ctime( &ltime ) );

	int i;
	for( i = 0; i < NUM_BENCH_RUNS; i++ )
	{
		int j;
		fprintf( fp, "model\tlight combo\ttotal tris\ttotal time\ttris/sec\n" );
		for( j = 0; j < LIGHTING_COMBINATION_COUNT; j++ )
		{
			int k;
			for( k = 0; k < g_nMaxModels; k++ )
			{
				if( g_BenchRuns[i].pModelName[k] )
				{
					fprintf( fp, "%s%s", k ? ", " : "", g_BenchRuns[i].pModelName[k] );
				}
			}
			fprintf( fp, "\t" );
			fprintf( fp, "%s\t", g_LightCombinationNames[j] );
			fprintf( fp, "%d\t", g_BenchResults[i][j].totalTris );
			fprintf( fp, "%0.2f\t", ( float )g_BenchResults[i][j].totalTime );
			fprintf( fp, "%0.0lf\n", ( double )g_BenchResults[i][j].totalTris /
				( double )g_BenchResults[i][j].totalTime );
		}
	}
	
	fclose( fp );
}

//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------

void Shutdown()
{
	// Close the window
	DestroyAppWindow();

	Shader_Shutdown( g_HWnd );

	WriteBenchResults();
}
			  
//-----------------------------------------------------------------------------
// Purpose: Return the directory where this .exe is running from
// Output : char
//-----------------------------------------------------------------------------

static char *GetBaseDir( const char *pszBuffer )
{
	static char	basedir[ MAX_PATH ];
	char szBuffer[ MAX_PATH ];
	int j;
	char *pBuffer = NULL;

	strcpy( szBuffer, pszBuffer );

	pBuffer = strrchr( szBuffer,'\\' );
	if ( pBuffer )
	{
		*(pBuffer+1) = '\0';
	}

	strcpy( basedir, szBuffer );

	j = strlen( basedir );
	if (j > 0)
	{
		if ( ( basedir[ j-1 ] == '\\' ) || 
			 ( basedir[ j-1 ] == '/' ) )
		{
			basedir[ j-1 ] = 0;
		}
	}

	return basedir;
}

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE hInstance, LPSTR pCommands, INT )
{
	CommandLine()->CreateCmdLine( pCommands );

	// Must add 'bin' to the path....
	char* pPath = getenv("PATH");

	// Use the .EXE name to determine the root directory
	char moduleName[ MAX_PATH ];
	char szBuffer[ 4096 ];
	if ( !GetModuleFileName( hInstance, moduleName, MAX_PATH ) )
	{
		MessageBox( 0, "Failed calling GetModuleFileName", "Launcher Error", MB_OK );
		return 0;
	}

	// Get the root directory the .exe is in
	char* pRootDir = GetBaseDir( moduleName );

#ifdef _DEBUG
	int len = 
#endif

	sprintf( szBuffer, "PATH=%s;%s", pRootDir, pPath );
	assert( len < 4096 );
	_putenv( szBuffer );
		    
	MathLib_Init( 2.2f, 2.2f, 0.0f, 1.0f );
	// Start up
	IHVTestModel model;
	if (!Init( pCommands, &model ) )
		return false;

	g_pIHVTestModel = &model;

	VTResume();
	bool done = false;
	while (!done && !g_BenchFinished)
	{
		done = Update();
	}
	VTPause();

	Shutdown();

    return 0;
}

