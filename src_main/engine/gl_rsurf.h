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

#ifndef GL_RSURF_H
#define GL_RSURF_H

#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "decal_clip.h"
#include "bsptreedata.h"

class Vector;
struct WorldListInfo_t;
class IMaterial;
class IClientRenderable;
class IBrushRenderer;
class IClientEntity;
struct model_t;


//-----------------------------------------------------------------------------
// Helper class to iterate over leaves
//-----------------------------------------------------------------------------
class IEngineSpatialQuery : public ISpatialQuery
{
public:
};

extern IEngineSpatialQuery* g_pToolBSPTree;

void R_SceneBegin( void );
void R_SceneEnd( void );
void R_BuildWorldLists( WorldListInfo_t* pInfo, bool updateLightmaps, int iForceViewLeaf );
void R_DrawWorldLists( unsigned long flags );
void R_GetVisibleFogVolume( int *visibleFogVolume, int *visibleFogVolumeLeaf, bool *eyeInFogVolume, float *pDistanceToWater, const Vector& eyePoint );
void R_SetFogVolumeState( int fogVolume, bool useHeightFog );
IMaterial *R_GetFogVolumeMaterial( int fogVolume );
float R_GetWaterHeight( int fogVolume );
void R_SetupSkyTexture( model_t *pWorld );
void Shader_DrawLightmapPageChains( void );
void Shader_DrawLightmapPageSurface( int surfID, float red, float green, float blue );
void Shader_FreePerLevelData( void );
void Shader_WorldSurface( int surfID );
void Shader_DrawTranslucentSurfaces( int sortIndex, unsigned long flags );
bool Shader_LeafContainsTranslucentSurfaces( int sortIndex, unsigned long flags );
void R_DrawTopView( bool enable );
void R_TopViewBounds( const Vector2D & mins, const Vector2D & maxs );

int ComputeLeaf( const Vector & pt );

// Installs a client-side renderer for brush models
void R_InstallBrushRenderOverride( IBrushRenderer* pBrushRenderer );

void R_DrawBrushModel( 
	IClientEntity *baseentity, 
	model_t *model, 
	const Vector& origin, 
	const QAngle& angles, 
	bool sort );

void R_DrawBrushModelShadow( IClientRenderable* pRender );

// Initialize and shutdown the decal stuff.
// R_DecalTerm unlinks all the active decals (which frees their counterparts in displacements).
void R_DecalInit();
void R_DecalTerm( model_t *model );


// --------------------------------------------------------------- //
// Decal functions used for displacements.
// --------------------------------------------------------------- //

// Figure out where the decal maps onto the surface.
void R_SetupDecalClip( 
	CDecalVert* &pOutVerts,
	decal_t *pDecal,
	Vector &vSurfNormal,
	IMaterial *pMaterial,
	Vector textureSpaceBasis[3],
	float decalWorldScale[2] );


//-----------------------------------------------------------------------------
// Gets the decal material and radius based on the decal index
//-----------------------------------------------------------------------------
void R_DecalGetMaterialAndSize( int decalIndex, IMaterial*& pDecalMaterial, float& w, float& h );

#endif // GL_RSURF_H
