//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/MaterialSystem_Config.h"
#include <cmdlib.h>
#include "tier0/dbg.h"
#include <windows.h>
#include "FileSystem.h"
#include "FileSystem_Tools.h"

IMaterialSystem *g_pMaterialSystem = NULL;
CreateInterfaceFn g_MatSysFactory = NULL;
CreateInterfaceFn g_ShaderAPIFactory = NULL;

static void LoadMaterialSystem( void )
{
	if( g_pMaterialSystem )
		return;
	
	const char *pDllName = "materialsystem.dll";
	HINSTANCE materialSystemDLLHInst;
	materialSystemDLLHInst = LoadLibrary( pDllName );
	if( !materialSystemDLLHInst )
	{
		Error( "Can't load MaterialSystem.dll\n" );
	}

	g_MatSysFactory = Sys_GetFactory( "MaterialSystem.dll" );
	if ( g_MatSysFactory )
	{
		g_pMaterialSystem = (IMaterialSystem *)g_MatSysFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
		if ( !g_pMaterialSystem )
		{
			Error( "Could not get the material system interface from materialsystem.dll" );
		}
	}
	else
	{
		Error( "Could not find factory interface in library MaterialSystem.dll" );
	}

	FileSystem_Init();
	if (!( g_ShaderAPIFactory = g_pMaterialSystem->Init( "shaderapiempty.dll", 0, FileSystem_GetFactory() )) )
	{
		Error( "Could not start the empty shader (shaderapiempty.dll)!" );
	}
}

void InitMaterialSystem( const char *materialBaseDirPath )
{
	MaterialSystem_Config_t config;

	memset( &config, 0, sizeof(config) );
	config.screenGamma = 2.2f;
	config.texGamma = 2.2f;
	config.overbright = 1.0f;
	config.bAllowCheats = false;
	config.bLinearFrameBuffer = false;
	config.polyOffset = 0.0f;
	config.skipMipLevels = 0;
	config.lightScale = 1.0f;
	config.bFilterLightmaps = false;
	config.bFilterTextures = false;
	config.bMipMapTextures = false;
	config.bBumpmap = true;
	config.bShowSpecular = true;
	config.bShowDiffuse = true;
	config.maxFrameLatency = 1;
	config.bLightingOnly = false;
	config.bCompressedTextures = false;
	config.bShowMipLevels = false;
	config.bEditMode = false;	// No, we're not in WorldCraft.
	config.m_bForceTrilinear = false;
	config.m_nForceAnisotropicLevel = 0;
	config.m_bForceBilinear = false;

	LoadMaterialSystem();
	g_pMaterialSystem->UpdateConfig( &config, false );
}
