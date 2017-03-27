//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include <stdlib.h>
#include <tier0/dbg.h>
#include "interface.h"
#include "istudiorender.h"
#include "matsys.h"
#include "optimize.h"
#include "studio.h"

static CSysModule *g_pStudioRenderModule = NULL;
static IStudioRender *g_pStudioRender = NULL;
static void UpdateStudioRenderConfig( void );
static StudioRenderConfig_t s_StudioRenderConfig;


void InitStudioRender( void )
{
	if ( g_pStudioRenderModule )
		return;
	Assert( g_MatSysFactory );

	g_pStudioRenderModule = Sys_LoadModule( "StudioRender.dll" );
	if( !g_pStudioRenderModule )
	{
		Error( "Can't load StudioRender.dll\n" );
	}
	CreateInterfaceFn studioRenderFactory = Sys_GetFactory( g_pStudioRenderModule );
	if (!studioRenderFactory )
	{
		Error( "Can't get factory for StudioRender.dll\n" );
	}
	g_pStudioRender = ( IStudioRender * )studioRenderFactory( STUDIO_RENDER_INTERFACE_VERSION, NULL );
	if (!g_pStudioRender)
	{
		Error( "Unable to init studio render system version %s\n", STUDIO_RENDER_INTERFACE_VERSION );
	}

	g_pStudioRender->Init( g_MatSysFactory, g_ShaderAPIFactory );
	UpdateStudioRenderConfig();
}



static void UpdateStudioRenderConfig( void )
{
	memset( &s_StudioRenderConfig, 0, sizeof(s_StudioRenderConfig) );

	s_StudioRenderConfig.eyeGloss = true;
	s_StudioRenderConfig.bEyeMove = true;
	s_StudioRenderConfig.fEyeShiftX = 0.0f;
	s_StudioRenderConfig.fEyeShiftY = 0.0f;
	s_StudioRenderConfig.fEyeShiftZ = 0.0f;
	s_StudioRenderConfig.fEyeSize = 10.0f;
	s_StudioRenderConfig.bSoftwareSkin = false;
	s_StudioRenderConfig.bNoHardware = false;
	s_StudioRenderConfig.bNoSoftware = false;
	s_StudioRenderConfig.bTeeth = true;
	s_StudioRenderConfig.drawEntities = true;
	s_StudioRenderConfig.bFlex = true;
	s_StudioRenderConfig.bEyes = true;
	s_StudioRenderConfig.bWireframe = false;
	s_StudioRenderConfig.bNormals = false;
	s_StudioRenderConfig.skin = 0;
	s_StudioRenderConfig.bUseAmbientCube = true;
	s_StudioRenderConfig.modelLightBias = 1.0f;
	s_StudioRenderConfig.maxDecalsPerModel = 0;
	s_StudioRenderConfig.bWireframeDecals = false;
	s_StudioRenderConfig.fullbright = false;
	s_StudioRenderConfig.bSoftwareLighting = false;
	s_StudioRenderConfig.pConDPrintf = Warning;
	s_StudioRenderConfig.pConPrintf = Warning;
	s_StudioRenderConfig.gamma = 2.2f;
	s_StudioRenderConfig.texGamma = 2.2f;
	s_StudioRenderConfig.overbrightFactor =  2.0;
	s_StudioRenderConfig.brightness = 0.0f;
	s_StudioRenderConfig.bShowEnvCubemapOnly = false;
	s_StudioRenderConfig.bStaticLighting = true;
	s_StudioRenderConfig.r_speeds = 0;
	g_pStudioRender->UpdateConfig( s_StudioRenderConfig );
}

void ShutdownStudioRender( void )
{
	if ( !g_pStudioRenderModule )
		return;

	if ( g_pStudioRender )
	{
		g_pStudioRender->Shutdown();
	}
	g_pStudioRender = NULL;

	Sys_UnloadModule( g_pStudioRenderModule );
	g_pStudioRenderModule = NULL;
}


void SpewPerfStats( studiohdr_t *pStudioHdr, OptimizedModel::FileHeader_t *vtxFile )
{
	// Need to load up StudioRender.dll to spew perf stats.
	InitStudioRender();
	DrawModelInfo_t drawModelInfo;
	studiohwdata_t studioHWData;
	g_pStudioRender->LoadModel( pStudioHdr, vtxFile, &studioHWData );
	memset( &drawModelInfo, 0, sizeof( DrawModelInfo_t ) );
	drawModelInfo.m_pStudioHdr = pStudioHdr;
	drawModelInfo.m_pHardwareData = &studioHWData;	
	int i;
	for( i = 0; i < studioHWData.m_NumLODs; i++ )
	{
		CUtlBuffer statsOutput( 0, 0, true /* text */ );
		printf( "LOD: %d\n", i );
		drawModelInfo.m_Lod = i;
		g_pStudioRender->GetPerfStats( drawModelInfo, &statsOutput );
		printf( "\tactual tris: %d\n", ( int )drawModelInfo.m_ActualTriCount );
		printf( "\ttexture memory bytes: %d\n", ( int )drawModelInfo.m_TextureMemoryBytes );
		printf( ( char * )statsOutput.Base() );
	}
	g_pStudioRender->UnloadModel( &studioHWData );
}

