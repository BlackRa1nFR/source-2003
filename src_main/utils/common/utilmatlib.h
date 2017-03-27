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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef UTILMATLIB_H
#define UTILMATLIB_H
#pragma once

#define MATERIAL_NOT_FOUND NULL
	
class IMaterialSystem;
extern IMaterialSystem *g_pMaterialSystem;

typedef void *MaterialSystemMaterial_t;

#define UTILMATLIB_NEEDS_BUMPED_LIGHTMAPS 0
#define UTILMATLIB_NEEDS_LIGHTMAP 1
#define UTILMATLIB_OPACITY 2

enum { UTILMATLIB_ALPHATEST = 0, UTILMATLIB_OPAQUE, UTILMATLIB_TRANSLUCENT };

void InitMaterialSystem( const char *materialBaseDirPath, CreateInterfaceFn fileSystemFactory );
MaterialSystemMaterial_t FindMaterial( const char *materialName, bool *pFound, bool bComplain = true );
void GetMaterialDimensions( MaterialSystemMaterial_t materialHandle, int *width, int *height );
int GetMaterialShaderPropertyBool( MaterialSystemMaterial_t materialHandle, int propID );
int GetMaterialShaderPropertyInt( MaterialSystemMaterial_t materialHandle, int propID );
const char *GetMaterialVar( MaterialSystemMaterial_t materialHandle, const char *propertyName );
void GetMaterialReflectivity( MaterialSystemMaterial_t materialHandle, float *reflectivityVect );


#endif // UTILMATLIB_H
