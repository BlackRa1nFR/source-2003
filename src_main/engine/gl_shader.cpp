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
// Entry points to the main material system interface
//=============================================================================

#include "glquake.h"
#include "gl_shader.h"
#include "const.h"
#include "gl_cvars.h"
#include "gl_lightmap.h"
#include "decal_private.h"
#include "gl_model_private.h"
#include "gl_texture.h"
#include "r_local.h"
#include "materialproxyfactory.h"
#include "enginestats.h"
#include "editor_sendcommand.h"
#include "gl_matsysiface.h"
#include "vmodes.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "vstdlib/strtools.h"
#include "filesystem.h"
#include "traceinit.h"
#include "vid.h"
#include "sys_dll.h"
#include "vstdlib/ICommandLine.h"


static CMaterialProxyFactory s_MaterialProxyFactory;

// chains of surfaces sorted by shader.
static int numShaders = 0;

void LoadScriptShaderInterface( void );
void LoadScriptShaderDummyInterface( void );
static int ScriptShaderFindTexture( const char *textureName );
static void GenerateShaders( const char *fileName );
static void DrawSurface( int surfID, bool generateDebugFlatColors );
void R_Shader_DecalSurfaceDraw( void );
static void GenerateFlatColorTable( void );

static float g_floatFlatColors[256*4];

extern vrect_t	window_rect;

CreateInterfaceFn g_MaterialSystemClientFactory;


void Shader_Shutdown( void )
{
	// Recursive shutdown
	if ( !materialSystemInterface )
		return;

	cv->UnlinkMatSysVars();
	materialSystemInterface->Shutdown( );
	materialSystemInterface = NULL;
	TRACESHUTDOWN( UnloadMaterialSystemInterface() );
}


//-----------------------------------------------------------------------------
// stuff that is exported to the dedicated server
//-----------------------------------------------------------------------------
void Shader_InitDedicated( void )
{
	TRACEINIT( LoadMaterialSystemInterface(), UnloadMaterialSystemInterface() );
	assert( materialSystemInterface );

	// assume that paths have already been set via g_pFileSystem 
	g_MaterialSystemClientFactory = materialSystemInterface->Init( 
		"shaderapiempty", &s_MaterialProxyFactory, g_AppSystemFactory );
	if (!g_MaterialSystemClientFactory)
	{
		Sys_Error( "Unable to init shader system\n" );
	}

	g_pMaterialSystemHardwareConfig = (IMaterialSystemHardwareConfig*)
		g_MaterialSystemClientFactory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, 0 );
	if ( !g_pMaterialSystemHardwareConfig )
	{
		Shader_Shutdown();
		Sys_Error( "Could not get the material system hardware config interface! (2)" );
	}

}


//-----------------------------------------------------------------------------
// stuff that is exported to the launcher
//-----------------------------------------------------------------------------
#ifndef SWDS
void Shader_Init( void )
{
	TRACEINIT( LoadMaterialSystemInterface(), UnloadMaterialSystemInterface() );
	assert( materialSystemInterface );

	// FIXME: Where do we put this?
	char const* pDLLName = "shaderapidx9";
	
	int nAdapter = CommandLine()->ParmValue( "-adapter", 0 );
	int nModeFlags = 0;
	if ( CommandLine()->FindParm( "-ref" ) )
	{
		nModeFlags |= MATERIAL_INIT_REFERENCE_RASTERIZER;
	}
	materialSystemInterface->SetAdapter( nAdapter, nModeFlags );

	// assume that IFileSystem paths have already been set via g_pFileSystem 
	g_MaterialSystemClientFactory = materialSystemInterface->Init( 
		pDLLName, &s_MaterialProxyFactory, g_AppSystemFactory, Sys_GetFactoryThis() );
	if (!g_MaterialSystemClientFactory)
	{
		Shader_Shutdown();
		Sys_Error( "Unable to init shader system\n" );
	}

	materialSystemStatsInterface = (IMaterialSystemStats *)g_MaterialSystemClientFactory( 
		MATERIAL_SYSTEM_STATS_INTERFACE_VERSION, NULL );
	if ( !materialSystemStatsInterface )
	{
		Shader_Shutdown();
		Sys_Error( "Could not get the material system stats interface from materialsystem.dll (3)" );
	}

	g_pMaterialSystemHardwareConfig = (IMaterialSystemHardwareConfig*)
		g_MaterialSystemClientFactory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, 0 );
	if ( !g_pMaterialSystemHardwareConfig )
	{
		Shader_Shutdown();
		Sys_Error( "Could not get the material system hardware config interface!(4)" );
	}

	g_EngineStats.InstallClientStats( materialSystemStatsInterface );
}

void Shader_ForceDXLevelDefaults( void )
{
	int dxLevel = g_pMaterialSystemHardwareConfig->GetDXSupportLevel();
	
	if( dxLevel < 70 )
	{
		// dx6
		// turn off shadows
		extern ConVar r_shadows;
		r_shadows.SetValue( false );

		// force mat_picmip of at least 2
		if( mat_picmip.GetInt() < 2 )
		{
			mat_picmip.SetValue( 2 );
		}

		// force the fastest sound mode possible
		extern ConVar dsp_off, s_surround;
		dsp_off.SetValue( true );
		s_surround.SetValue( false );

		extern ConVar r_decals;
		// force 
		if( r_decals.GetInt() > 300 )
		{
			r_decals.SetValue( 300 );
		}
	}
}

void Shader_SetMode( void *mainWindow, bool windowed )
{
	int modeFlags = 0;
	if ( windowed )
		modeFlags |= MATERIAL_VIDEO_MODE_WINDOWED;
	if ( CommandLine()->CheckParm( "-resizing" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_RESIZING;
	if ( CommandLine()->CheckParm( "-mat_softwaretl" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_SOFTWARE_TL;
	if ( CommandLine()->CheckParm( "-mat_novsync" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_NO_WAIT_FOR_VSYNC;
	if ( CommandLine()->CheckParm( "-mat_preloadshaders" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_PRELOAD_SHADERS;
	if ( CommandLine()->CheckParm( "-mat_quitafterpreload" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_QUIT_AFTER_PRELOAD;

	int nAntialias = CommandLine()->ParmValue( "-mat_antialias", -1 );
	if ( nAntialias != -1 )
		modeFlags |= MATERIAL_VIDEO_MODE_ANTIALIAS;

	MaterialVideoMode_t mode;
	mode.m_Width = mode.m_Height = 0;
	bool modeSet = materialSystemInterface->SetMode( (void*)mainWindow, mode, modeFlags, nAntialias );
	if (!modeSet)
	{
		Sys_Error( "Unable to set mode\n" );
	}
}



void Shader_SwapBuffers( void )
{
	assert( materialSystemInterface );
	materialSystemInterface->SwapBuffers();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *x - 
//			*y - 
//			*width - 
//			*height - 
// Output : void VID_BeginRendering
//-----------------------------------------------------------------------------
void Shader_BeginRendering ( void )
{
	vid.width  = window_rect.width;
	vid.height = window_rect.height;
}
#endif
