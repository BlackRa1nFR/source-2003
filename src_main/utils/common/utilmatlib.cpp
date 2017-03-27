//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

// C callable material system interface for the utils.

#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"
#include <cmdlib.h>
#include "utilmatlib.h"
#include "tier0/dbg.h"
#include <windows.h>
#include "FileSystem.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "Mathlib.h"

IMaterialSystem *g_pMaterialSystem = NULL;

void LoadMaterialSystemInterface( CreateInterfaceFn fileSystemFactory )
{
	if( g_pMaterialSystem )
		return;
	
	// materialsystem.dll should be in the path, it's in bin along with vbsp.
	const char *pDllName = "materialsystem.dll";
	HINSTANCE materialSystemDLLHInst;
	materialSystemDLLHInst = LoadLibrary( pDllName );
	if( !materialSystemDLLHInst )
	{
		Error( "Can't load MaterialSystem.dll\n" );
	}

	CreateInterfaceFn clientFactory = Sys_GetFactory( "MaterialSystem.dll" );
	if ( clientFactory )
	{
		g_pMaterialSystem = (IMaterialSystem *)clientFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
		if ( !g_pMaterialSystem )
		{
			Error( "Could not get the material system interface from materialsystem.dll (" __FILE__ ")" );
		}
	}
	else
	{
		Error( "Could not find factory interface in library MaterialSystem.dll" );
	}

	if (!g_pMaterialSystem->Init( "shaderapiempty.dll", 0, fileSystemFactory ))
	{
		Error( "Could not start the empty shader (shaderapiempty.dll)!" );
	}
}

void InitMaterialSystem( const char *materialBaseDirPath, CreateInterfaceFn fileSystemFactory )
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

	LoadMaterialSystemInterface( fileSystemFactory );
	g_pMaterialSystem->UpdateConfig( &config, false );
}

MaterialSystemMaterial_t FindMaterial( const char *materialName, bool *pFound, bool bComplain )
{
	MaterialSystemMaterial_t matHandle;
	matHandle = g_pMaterialSystem->FindMaterial( materialName, pFound, bComplain );
	return matHandle;
}

void GetMaterialDimensions( MaterialSystemMaterial_t materialHandle, int *width, int *height )
{
	PreviewImageRetVal_t retVal;
	ImageFormat dummyImageFormat;
	IMaterial *material = ( IMaterial * )materialHandle;
	bool translucent;
	retVal = material->GetPreviewImageProperties( width, height, &dummyImageFormat, &translucent );
	if (retVal != MATERIAL_PREVIEW_IMAGE_OK ) 
	{
#if 0
		if (retVal == MATERIAL_PREVIEW_IMAGE_BAD ) 
		{
			Error( "problem getting preview image for %s", 
				g_pMaterialSystem->GetMaterialName( materialInfo[matID].materialHandle ) );
		}
#else
		*width = 128;
		*height = 128;
#endif
	}
}

void GetMaterialReflectivity( MaterialSystemMaterial_t materialHandle, float *reflectivityVect )
{
	IMaterial *material = ( IMaterial * )materialHandle;
	const IMaterialVar *reflectivityVar;

	bool found;
	reflectivityVar = material->FindVar( "$reflectivity", &found, false );
	if( !found )
	{
		Vector tmp;
		material->GetReflectivity( tmp );
		VectorCopy( tmp.Base(), reflectivityVect );
	}
	else
	{
		reflectivityVar->GetVecValue( reflectivityVect, 3 );
	}
}

int GetMaterialShaderPropertyBool( MaterialSystemMaterial_t materialHandle, int propID )
{
	IMaterial *material = ( IMaterial * )materialHandle;
	switch( propID )
	{
	case UTILMATLIB_NEEDS_BUMPED_LIGHTMAPS:
		return material->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS );

	case UTILMATLIB_NEEDS_LIGHTMAP:
		return material->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_LIGHTMAP );

	default:
		Assert( 0 );
		return 0;
	}
}

int GetMaterialShaderPropertyInt( MaterialSystemMaterial_t materialHandle, int propID )
{
	IMaterial *material = ( IMaterial * )materialHandle;
	switch( propID )
	{
	case UTILMATLIB_OPACITY:
		if (material->IsTranslucent())
			return UTILMATLIB_TRANSLUCENT;
		if (material->IsAlphaTested())
			return UTILMATLIB_ALPHATEST;
		return UTILMATLIB_OPAQUE;

	default:
		Assert( 0 );
		return 0;
	}
}

const char *GetMaterialVar( MaterialSystemMaterial_t materialHandle, const char *propertyName )
{
	IMaterial *material = ( IMaterial * )materialHandle;
	IMaterialVar *var;
	bool found;
	var = material->FindVar( propertyName, &found, false );
	if( found )
	{
		return var->GetStringValue();
	}
	else
	{
		return NULL;
	}
}
