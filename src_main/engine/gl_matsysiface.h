//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

// wrapper for the material system for the engine.

#ifndef GL_MATSYSIFACE_H
#define GL_MATSYSIFACE_H

#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialsystemstats.h"
#include "materialsystem/imaterial.h"
#include "ivrenderview.h"
#include "convar.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class IMaterialSystemHardwareConfig;
class Vector;
struct mprimitive_t;
class CMeshBuilder;
struct model_t;

//-----------------------------------------------------------------------------
// global interfaces
//-----------------------------------------------------------------------------

extern MaterialSystem_Config_t g_materialSystemConfig;

extern IMaterialSystem *materialSystemInterface;
extern IMaterialSystemStats *materialSystemStatsInterface;
extern IMaterialSystemHardwareConfig *g_pMaterialSystemHardwareConfig;
extern MaterialSystem_SortInfo_t *materialSortInfoArray;
extern int g_NumMaterialSortBins;
extern bool g_LostVideoMemory;

void MaterialSystem_DestroySortinfo( void );
void MaterialSystem_CreateSortinfo( void );

void LoadMaterialSystemInterface( void );
void UnloadMaterialSystemInterface( void );
void InitMaterialSystem( void );
void ShutdownMaterialSystem( void );

void UpdateMaterialSystemConfig( void );
bool MaterialConfigLightingChanged();
void ClearMaterialConfigLightingChanged();

IMaterial *GetMaterialAtCrossHair( void );

bool SurfHasBumpedLightmaps( int surfID );
bool SurfNeedsBumpedLightmaps( int surfID );
bool SurfHasLightmap( int surfID );
bool SurfNeedsLightmap( int surfID );

struct SurfRenderInfo_t
{
	int	m_nHeadSurfID;
	int	m_nLastSurfID;
	int	m_nTriangleCount;

	// For the fast z reject test...
	int m_nVertexCountNoDetail;
	int m_nIndexCountNoDetail;

	void Reset()
	{
		m_nHeadSurfID = -1;
		m_nLastSurfID = -1;
		m_nTriangleCount = 0;
		m_nVertexCountNoDetail = 0;
		m_nIndexCountNoDetail = 0;
	}

	void AddPolygon( int nVertCount, bool bIsDetail )
	{
		m_nTriangleCount += nVertCount - 2;

		if (!bIsDetail)
		{
			m_nVertexCountNoDetail += nVertCount;
			m_nIndexCountNoDetail += (nVertCount - 2) * 3;
		}
	}
};

enum
{
	SORT_ARRAY_SENTINEL = -1,
	SORT_ARRAY_NOT_ENCOUNTERED_YET = -2,
};

struct MatSortArray_t
{
	SurfRenderInfo_t m_Surf[MAX_MAT_SORT_GROUPS];
	int	m_DisplacementSurfID[MAX_MAT_SORT_GROUPS];
	IMesh* m_pMesh;
	int m_pNext[MAX_MAT_SORT_GROUPS];

	void Reset()
	{
		for (int j = 0; j < MAX_MAT_SORT_GROUPS; ++j)
		{
			m_Surf[j].Reset();
			m_DisplacementSurfID[j] = -1;
			m_pNext[j] = SORT_ARRAY_NOT_ENCOUNTERED_YET;
		}
	}
};

extern MatSortArray_t *g_pWorldStatic;
extern int g_pFirstMatSort[MAX_MAT_SORT_GROUPS];


//-----------------------------------------------------------------------------
// Converts sort infos to lightmap pages
//-----------------------------------------------------------------------------
int SortInfoToLightmapPage( int sortID );

void BuildMSurfaceVerts( const model_t *pWorld, int surfID, Vector *verts, Vector2D *texCoords, Vector2D lightCoords[][4] );
void BuildMSurfacePrimVerts( model_t *pWorld, mprimitive_t *prim, CMeshBuilder &builder, int surfID );
void BuildMSurfacePrimIndices( model_t *pWorld, mprimitive_t *prim, CMeshBuilder &builder );
void BuildBrushModelVertexArray(model_t *pWorld, int surfID, BrushVertex_t* pVerts );

// Used for debugging - force it to release and restore all material system objects.
void ForceMatSysRestore();


//-----------------------------------------------------------------------------
// Methods associated with getting surface data
//-----------------------------------------------------------------------------
struct SurfaceCtx_t
{
	int m_LightmapSize[2];
	int m_LightmapPageSize[2];
	float m_BumpSTexCoordOffset;
	Vector2D m_Offset;
	Vector2D m_Scale;
};

// Compute a context necessary for creating vertex data
void SurfSetupSurfaceContext( SurfaceCtx_t& ctx, int surfID );

// Compute texture and lightmap coordinates
void SurfComputeTextureCoordinate( SurfaceCtx_t const& ctx, int surfID, 
									    Vector const& vec, Vector2D& uv );
void SurfComputeLightmapCoordinate( SurfaceCtx_t const& ctx, int surfID, 
										 Vector const& vec, Vector2D& uv );

extern int SurfFlagsToSortGroup( int flags );

extern ConVar mat_specular;

#endif // GL_MATSYSIFACE_H
