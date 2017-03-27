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
// gl_rsurf.cpp: surface-related refresh code
//=============================================================================

#include "glquake.h"
#include "gl_model_private.h"
#include "gl_water.h"
#include "gl_cvars.h"
#include "zone.h"
#include "decal.h"
#include "decal_private.h"
#include "gl_lightmap.h"
#include "r_local.h"
#include "gl_matsysiface.h"
#include "enginestats.h"
#include "gl_rsurf.h"
#include "dispchain.h"
#include "materialsystem/imesh.h"
#include "collisionutils.h"
#include "cdll_int.h"
#include "utllinkedlist.h"
#include "r_areaportal.h"
#include "bsptreedata.h"
#include "cmodel_private.h"
#include "tier0/dbg.h"
#include "crtmemdebug.h"
#include "iclientrenderable.h"
#include "icliententitylist.h"
#include "icliententity.h"
#include "gl_rmain.h"
#include "tier0/vprof.h"
#include "debugoverlay.h"
#include "host.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"


#ifndef SWDS
#include "overlay.h"
#endif

// UNDONE: Get this from the material system instead !!!!
#define SHADER_MAX_INDICES	16384
#define MAX_DECALSURFS		500

#define BACKFACE_EPSILON	0.01

#define BRUSHMODEL_DECAL_SORT_GROUP		MAX_MAT_SORT_GROUPS

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class IClientEntity;


void R_MarkLights (dlight_t *light, int bit, mnode_t *node);

// Decal surface list managment
void DecalSurfaceDraw( int renderGroup );
void DecalSurfaceAdd( int surfID, int renderGroup );

// Computes a leaf a particular point is in
int ComputeLeaf( const Vector& pt );

// interface to shader drawing
void Shader_BrushBegin( model_t *model, IClientEntity *baseentity = NULL );
void Shader_BrushSurface( int surfID, model_t *model, IClientEntity *baseentity = NULL );
void Shader_BrushEnd( VMatrix const* brushToWorld, model_t *model, IClientEntity *baseentity = NULL );
void BuildMSurfaceVertexArrays( model_t *pWorld, int surfID, float overbright, CMeshBuilder &builder );
bool TangentSpaceSurfaceSetup( int surfID, Vector &tVect );
void TangentSpaceComputeBasis( Vector& tangent, Vector& binormal, const Vector& normal, const Vector& tVect, bool negateTangent );

#ifndef SWDS
static void DrawDecalsOnSingleSurface( int surfID );
struct decallist_t;
static bool R_DecalUnProject( decal_t *pdecal, decallist_t *entry);
#endif

//-----------------------------------------------------------------------------
// Information about the fog volumes for this pass of rendering
//-----------------------------------------------------------------------------

struct FogState_t
{
	MaterialFogMode_t m_FogMode;
	float m_FogStart;
	float m_FogEnd;
	float m_FogColor[3];
	bool m_FogEnabled;
};

struct FogVolumeInfo_t : public FogState_t
{
	bool	m_InFogVolume;
	float	m_FogSurfaceZ;
	float	m_FogMinZ;
	int		m_FogVolumeID;
};

static FogVolumeInfo_t	s_FogVolume;
static FogState_t		s_WorldFog;

//-----------------------------------------------------------------------------
// Cached convars...
//-----------------------------------------------------------------------------
struct CachedConvars_t
{
	bool	m_bDrawWorld;
	int		m_nDrawLeaf;
};


static CachedConvars_t s_ShaderConvars;

// AR - moved so SWDS can access these vars
Frustum_t g_Frustum;
// hack
float g_lightmapScale = 1.0f;


//-----------------------------------------------------------------------------
// Convars
//-----------------------------------------------------------------------------
static ConVar r_drawtranslucentworld( "r_drawtranslucentworld", "1" );
static ConVar mat_forcedynamic( "mat_forcedynamic", "0" );
static ConVar r_drawleaf( "r_drawleaf", "-1" );
static ConVar r_drawworld( "r_drawworld", "1" );
static ConVar r_drawdecals( "r_drawdecals", "1" );
static ConVar r_drawbrushmodels( "r_drawbrushmodels", "1" );

static ConVar fog_enable_water_fog( "fog_enable_water_fog", "1" );
static ConVar r_fastzreject( "r_fastzreject", "-1", 0, "Activate/deactivates a fast z-setting algorithm to take advantage of hardware with fast z reject. Use -1 to default to hardware settings" );


//-----------------------------------------------------------------------------
// Installs a client-side renderer for brush models
//-----------------------------------------------------------------------------
static IBrushRenderer* s_pBrushRenderOverride = 0;

//-----------------------------------------------------------------------------
// List of decals to render this frame (need an extra one for brush models)
//-----------------------------------------------------------------------------
static CUtlVector<int>	s_DecalSurfaces[ MAX_MAT_SORT_GROUPS + 1 ];

//-----------------------------------------------------------------------------
// list of displacements to render this frame
//-----------------------------------------------------------------------------
CDispChain	s_DispChain[ MAX_MAT_SORT_GROUPS ];

//-----------------------------------------------------------------------------
// Make sure we don't render the same surfaces twice
//-----------------------------------------------------------------------------
int	r_surfacevisframe = 0;


//-----------------------------------------------------------------------------
// Top view bounds
//-----------------------------------------------------------------------------
static bool r_drawtopview = false;
static Vector2D s_OrthographicCenter;
static Vector2D s_OrthographicHalfDiagonal;


//-----------------------------------------------------------------------------
// Used to generate a list of the leaves visited, and in back-to-front order
// for this frame of rendering
//-----------------------------------------------------------------------------
static int			s_VisibleLeafCount = 0;
static int			s_pVisibleLeaf[ MAX_MAP_LEAFS ];
static int			s_pVisibleLeafFogVolume[ MAX_MAP_LEAFS ];

struct AlphaSortInfo_t
{
	int				m_HeadSurfID[MAX_MAT_SORT_GROUPS];
	CDispChain		m_DispChain[MAX_MAT_SORT_GROUPS];
};

// List of translucent surfaces to be rendered in a particular leaf
static CUtlVector< AlphaSortInfo_t > s_AlphaSurface( 0, 1024 );

// List of brushes to render
static int r_brushlist; // surfaceid


#ifndef SWDS
//-----------------------------------------------------------------------------
// Activates top view
//-----------------------------------------------------------------------------

void R_DrawTopView( bool enable )
{
	r_drawtopview = enable;
}

void R_TopViewBounds( Vector2D const& mins, Vector2D const& maxs )
{
	Vector2DAdd( maxs, mins, s_OrthographicCenter );
	s_OrthographicCenter *= 0.5f;
	Vector2DSubtract( maxs, s_OrthographicCenter, s_OrthographicHalfDiagonal );
}



//-----------------------------------------------------------------------------
// Resets lists	of things to render
//-----------------------------------------------------------------------------
static void Shader_ClearChains( )
{
	for ( int j = 0; j < MAX_MAT_SORT_GROUPS; ++j)
	{
		g_pFirstMatSort[j] = SORT_ARRAY_SENTINEL;
	}

	for ( int i = 0; i < g_NumMaterialSortBins; i++ )
	{
		g_pWorldStatic[i].Reset();
	}

	s_AlphaSurface.RemoveAll();
}

static void Shader_ClearDispChain( )
{
	// clear the previous displacement chain
	for (int j = 0; j < MAX_MAT_SORT_GROUPS; ++j)
	{
		s_DispChain[j].Reset();
	}
}

void Shader_FreePerLevelData( void )
{
	// fixme: these should be combined into one array
	for (int j = 0; j < MAX_MAT_SORT_GROUPS; ++j)
	{
		s_DispChain[j].Clear();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Add to the decal list
//-----------------------------------------------------------------------------
void DecalSurfaceAdd( int surfID, int group )
{
#ifdef USE_CONVARS
	if( r_drawdecals.GetInt() )
#endif
	{
		if ( s_DecalSurfaces[group].Count() > MAX_DECALSURFS )
		{
			Con_DPrintf( "Too many decal surfaces!\n");
		}
		else
		{
			s_DecalSurfaces[group].AddToTail( surfID );
		}
	}
}


//-----------------------------------------------------------------------------
// Computes the sort group for a particular face
//-----------------------------------------------------------------------------

int SurfFlagsToSortGroup( int flags )
{
	if( flags & SURFDRAW_WATERSURFACE )
	{
		return MAT_SORT_GROUP_WATERSURFACE;
	}
	else if( ( flags & ( SURFDRAW_UNDERWATER | SURFDRAW_ABOVEWATER ) ) == 
		( SURFDRAW_UNDERWATER | SURFDRAW_ABOVEWATER ) )
	{
		return MAT_SORT_GROUP_INTERSECTS_WATER_SURFACE;
	}
	else if( flags & SURFDRAW_UNDERWATER )
	{
		return MAT_SORT_GROUP_STRICTLY_UNDERWATER;
	}
	else if( flags & SURFDRAW_ABOVEWATER )
	{
		return MAT_SORT_GROUP_STRICTLY_ABOVEWATER;
	}
	else
	{
		static int warningcount = 0;
		if ( ++warningcount < 10 )
		{
			DevMsg( "SurfFlagsToSortGroup:  unhandled flags (%i)!\n",
				flags );	
		}
		//Assert( 0 );
		return MAT_SORT_GROUP_STRICTLY_ABOVEWATER;
	}
}


//-----------------------------------------------------------------------------
// Adds surfaces to list of things to render
//-----------------------------------------------------------------------------
void Shader_TranslucentWorldSurface( int surfID )
{
	Assert( !SurfaceHasDispInfo( surfID ) && (s_VisibleLeafCount > 0) );

	// Hook into the chain of translucent objects for this leaf
	// We need to hook it to the *front* of the list to produce a correct
	// back-to-front rendering of translucent objects, because we 
	// are iterating the tree in front-to-back ordering.
	
	int sortGroup = SurfFlagsToSortGroup( MSurf_Flags( surfID ) );
	MSurf_TextureChain( surfID ) = s_AlphaSurface[s_VisibleLeafCount-1].m_HeadSurfID[sortGroup];
	s_AlphaSurface[s_VisibleLeafCount-1].m_HeadSurfID[sortGroup] = surfID;
}

inline void Shader_WorldSurface( int surfID )
{
	Assert( !SurfaceHasDispInfo( surfID ) );

	// Each surface is in exactly one group
	int nSortGroup = SurfFlagsToSortGroup( MSurf_Flags( surfID ) );

	// Hook it into the list of surfaces to render with this material
	// Do it in a way that generates a front-to-back ordering for fast z reject
	int nMaterialSortID = MSurf_MaterialSortID( surfID );
	MSurf_TextureChain( surfID ) = -1;

	SurfRenderInfo_t &surfInfo = g_pWorldStatic[nMaterialSortID].m_Surf[nSortGroup];
	if ( surfInfo.m_nLastSurfID != -1 )
	{
		MSurf_TextureChain( surfInfo.m_nLastSurfID )= surfID;
	}
	else
	{
		surfInfo.m_nHeadSurfID = surfID;
	}
	surfInfo.m_nLastSurfID = surfID;
	surfInfo.AddPolygon( MSurf_VertCount( surfID ), (MSurf_Flags( surfID ) & SURFDRAW_NODE) == 0 );
	if ( g_pWorldStatic[nMaterialSortID].m_pNext[nSortGroup] == SORT_ARRAY_NOT_ENCOUNTERED_YET )
	{
		g_pWorldStatic[nMaterialSortID].m_pNext[nSortGroup] = g_pFirstMatSort[nSortGroup];
		g_pFirstMatSort[nSortGroup] = nMaterialSortID;
	}

	// Add decals on non-displacement surfaces
	if( MSurf_Decals( surfID ) )
	{
		DecalSurfaceAdd( surfID, nSortGroup );
	}

	// Add overlay fragments to list.
	OverlayMgr()->AddFragmentListToRenderList( MSurf_OverlayFragmentList( surfID ) );
}


//-----------------------------------------------------------------------------
// Adds displacement surfaces to list of things to render
//-----------------------------------------------------------------------------
void Shader_TranslucentDisplacementSurface( int surfID )
{
	Assert( SurfaceHasDispInfo( surfID ) && (s_VisibleLeafCount > 0));

	// For translucent displacement surfaces, they can exist in many
	// leaves. We want to choose the leaf that's closest to the camera
	// to render it in. Thankfully, we're iterating the tree in front-to-back
	// order, so this is very simple.

	// NOTE: You might expect some problems here when displacements cross fog volume 
	// planes. However, these problems go away (I hope!) because the first planes
	// that split a scene are the fog volume planes. That means that if we're
	// in a fog volume, the closest leaf that the displacement will be in will
	// also be in the fog volume. If we're not in a fog volume, the closest
	// leaf that the displacement will be in will not be a fog volume. That should
	// hopefully hide any discontinuities between fog state that occur when 
	// rendering displacements that straddle fog volume boundaries.

	// Each surface is in exactly one group
	int sortGroup = SurfFlagsToSortGroup( MSurf_Flags( surfID ) );
	s_AlphaSurface[s_VisibleLeafCount-1].m_DispChain[sortGroup].AddTo( MSurf_DispInfo( surfID ) );
}

void Shader_DisplacementSurface( int nSurfID )
{
	Assert( SurfaceHasDispInfo( nSurfID ) );

	// For opaque displacement surfaces, we're going to build a temporary list of 
	// displacement surfaces in each material bucket, and then add those to
	// the actual displacement lists in a separate pass.
	// We do this to sort the displacement surfaces by material

	// Each surface is in exactly one group
	int nSortGroup = SurfFlagsToSortGroup( MSurf_Flags( nSurfID ) );
	int nMaterialSortID = MSurf_MaterialSortID( nSurfID );
	MSurf_TextureChain( nSurfID ) = g_pWorldStatic[nMaterialSortID].m_DisplacementSurfID[nSortGroup];
	g_pWorldStatic[nMaterialSortID].m_DisplacementSurfID[nSortGroup] = nSurfID;
	if ( g_pWorldStatic[nMaterialSortID].m_pNext[nSortGroup] == SORT_ARRAY_NOT_ENCOUNTERED_YET )
	{
		g_pWorldStatic[nMaterialSortID].m_pNext[nSortGroup] = g_pFirstMatSort[nSortGroup];
		g_pFirstMatSort[nSortGroup] = nMaterialSortID;
	}
}
 

//-----------------------------------------------------------------------------
// Purpose: This draws a single surface using the dynamic mesh
//-----------------------------------------------------------------------------
void Shader_DrawSurfaceDynamic( int surfID )
{
	if( !MSurf_NumPrims( surfID ) )
	{
		IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_POLYGON, MSurf_VertCount( surfID ) );
		BuildMSurfaceVertexArrays( host_state.worldmodel, surfID, mat_overbright.GetFloat(), meshBuilder );
		meshBuilder.End();
		pMesh->Draw();
	}
	else
	{
		IMesh *pMesh = materialSystemInterface->GetDynamicMesh( false );
		CMeshBuilder meshBuilder;
		mprimitive_t *pPrim = &host_state.worldmodel->brush.primitives[MSurf_FirstPrimID( surfID )];
#ifdef _DEBUG
		int primType = pPrim->type;
#endif
		for( int i = 0; i < MSurf_NumPrims( surfID ); i++, pPrim++ )
		{
			// Can't have heterogeneous primitive lists
			Assert( primType == pPrim->type );
			switch( pPrim->type )
			{
			case PRIM_TRILIST:
				meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, pPrim->vertCount, pPrim->indexCount );
				break;
			case PRIM_TRISTRIP:
				meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, pPrim->vertCount, pPrim->indexCount );
				break;
			default:
				Assert( 0 );
				return;
			}
			Assert( pPrim->vertCount );
			Assert( pPrim->indexCount );
			BuildMSurfacePrimVerts( host_state.worldmodel, pPrim, meshBuilder, surfID );
			BuildMSurfacePrimIndices( host_state.worldmodel, pPrim, meshBuilder );
			meshBuilder.End();
			pMesh->Draw();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: This draws a single surface using its static mesh
//-----------------------------------------------------------------------------

/*
// NOTE: Since a static vb/dynamic ib IMesh doesn't buffer, we shouldn't use this
// since it causes a lock and drawindexedprimitive per surface! (gary)
void Shader_DrawSurfaceStatic( int surfID )
{
	VPROF( "Shader_DrawSurfaceStatic" );
	if ( 
#ifdef USE_CONVARS
		mat_forcedynamic.GetInt() || 
#endif
		(MSurf_Flags( surfID ) & SURFDRAW_WATERSURFACE) )
	{
		Shader_DrawSurfaceDynamic( surfID );
		return;
	}

	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( true, 
		g_pWorldStatic[MSurf_MaterialSortID( surfID )].m_pMesh );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 0, (MSurf_VertCount( surfID )-2)*3 );

	unsigned short startVert = MSurf_VertBufferIndex( surfID );
	Assert(startVert!=0xFFFF);
	for ( int v = 0; v < MSurf_VertCount( surfID )-2; v++ )
	{
		meshBuilder.Index( startVert );
		meshBuilder.AdvanceIndex();
		meshBuilder.Index( startVert + v + 1 );
		meshBuilder.AdvanceIndex();
		meshBuilder.Index( startVert + v + 2 );
		meshBuilder.AdvanceIndex();
	}
	meshBuilder.End();
	pMesh->Draw();
}
*/


//-----------------------------------------------------------------------------
// Sets the lightmapping state
//-----------------------------------------------------------------------------
static inline void Shader_SetChainLightmapState( int surfID )
{
	if( ( mat_showbadnormals.GetInt() != 0 ) ||
		( mat_fullbright.GetInt() == 1 ) )
	{
		if( MSurf_Flags( surfID ) & SURFDRAW_BUMPLIGHT )
		{
			materialSystemInterface->BindLightmapPage( MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE_BUMP );
		}
		else
		{
			materialSystemInterface->BindLightmapPage( MATERIAL_SYSTEM_LIGHTMAP_PAGE_WHITE );
		}
	}
	else
	{
		Assert( MSurf_MaterialSortID( surfID ) >= 0 && MSurf_MaterialSortID( surfID ) < g_NumMaterialSortBins );
		materialSystemInterface->BindLightmapPage( materialSortInfoArray[MSurf_MaterialSortID( surfID )].lightmapPageID );
	}
}


//-----------------------------------------------------------------------------
// Sets the lightmap + texture to render with
//-----------------------------------------------------------------------------
void Shader_SetChainTextureState( 
	int surfID, 
	IClientEntity* pBaseEntity )
{
	materialSystemInterface->Bind( MSurf_TexInfo( surfID )->material, pBaseEntity );
	Shader_SetChainLightmapState( surfID );
}


static void DrawDynamicChain( int surfID )
{
	for ( ; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
	{
		Assert( !SurfaceHasDispInfo( surfID ) );
		Shader_DrawSurfaceDynamic( surfID );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws the surfaces lists using static meshes
//-----------------------------------------------------------------------------
void Shader_DrawChainsStatic( int nSortGroup )
{
	CMeshBuilder meshBuilder;
	int nTriangleCount = 0;
	int nLowendTriangleCount = 0;

	int i = g_pFirstMatSort[nSortGroup];
	for ( i = g_pFirstMatSort[nSortGroup]; i >= 0; i = g_pWorldStatic[i].m_pNext[nSortGroup] )
	{
		SurfRenderInfo_t &info = g_pWorldStatic[i].m_Surf[nSortGroup];
		int nSurfID = info.m_nHeadSurfID;
		if( !IS_SURF_VALID( nSurfID ) )
			continue;

		Assert( info.m_nTriangleCount > 0 );

		g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_WORLD_MATERIALS_RENDERED, 1 );

		IMaterial *pMaterial = MSurf_TexInfo( nSurfID )->material;
		pMaterial->ColorModulate( 1.0f, 1.0f, 1.0f );
		pMaterial->AlphaModulate( 1.0f );
		Shader_SetChainTextureState( nSurfID, 0 );

		nTriangleCount += pMaterial->GetNumPasses() * info.m_nTriangleCount;
		nLowendTriangleCount += info.m_nTriangleCount;

		if ( MSurf_Flags( nSurfID ) & (SURFDRAW_WATERSURFACE|SURFDRAW_DYNAMIC) )
		{
			DrawDynamicChain( nSurfID );
			continue;
		}

		int indexCount = info.m_nTriangleCount * 3;
		while ( indexCount > 0 )
		{
			// batchIndexCount is the number of indices we can fit in this batch
			// each batch must be smaller than SHADER_MAX_INDICES or the material system will fail.
			int batchIndexCount = min(indexCount, SHADER_MAX_INDICES);
			IMesh *pMesh = materialSystemInterface->GetDynamicMesh( false, g_pWorldStatic[i].m_pMesh );
			meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 0, batchIndexCount );

			int nextSurfID;
			for( ; nSurfID >= 0; nSurfID = nextSurfID )
			{
				Assert( !SurfaceHasDispInfo( nSurfID ) );

				int nSurfTriangleCount = MSurf_VertCount( nSurfID ) - 2;
				int surfIndexCount = nSurfTriangleCount * 3;

				// Surface would overflow this batch, start new batch
				if ( batchIndexCount < surfIndexCount )
					break;

				unsigned short startVert = MSurf_VertBufferIndex( nSurfID );
				Assert(startVert!=0xFFFF);

				// Precache the next surface id...
				nextSurfID = MSurf_TextureChain( nSurfID );

				// NOTE: This switch appears to help performance
				// add surface to this batch
				switch (nSurfTriangleCount)
				{
				case 1:
					meshBuilder.FastIndex( startVert );
					meshBuilder.FastIndex( startVert + 1 );
					meshBuilder.FastIndex( startVert + 2 );
					break;

				case 2:
					meshBuilder.FastIndex( startVert );
					meshBuilder.FastIndex( startVert + 1 );
					meshBuilder.FastIndex( startVert + 2 );
					meshBuilder.FastIndex( startVert );
					meshBuilder.FastIndex( startVert + 2 );
					meshBuilder.FastIndex( startVert + 3 );
					break;

				default:
					{
						for ( unsigned short v = 0; v < nSurfTriangleCount; ++v )
						{
							meshBuilder.FastIndex( startVert );
							meshBuilder.FastIndex( startVert + v + 1 );
							meshBuilder.FastIndex( startVert + v + 2 );
						}
					}
					break;
				}

				indexCount -= surfIndexCount;
				batchIndexCount -= surfIndexCount;
			}

			// draw this batch
			meshBuilder.End();
			pMesh->Draw();
		}
	}

	g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_WORLD_TRIS_RENDERED, nTriangleCount );
	g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_LOWEND_WORLD_TRIS_RENDERED, nLowendTriangleCount );
}

void Shader_DrawChainsDynamic( int nSortGroup )
{
	int i = g_pFirstMatSort[nSortGroup];
	for ( i = g_pFirstMatSort[nSortGroup]; i >= 0; i = g_pWorldStatic[i].m_pNext[nSortGroup] )
	{
		int nSurfID = g_pWorldStatic[i].m_Surf[nSortGroup].m_nHeadSurfID;
		if( !IS_SURF_VALID( nSurfID ) )
			continue;

		g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_WORLD_MATERIALS_RENDERED, 1 );
		IMaterial *pMaterial = MSurf_TexInfo( nSurfID )->material;
		pMaterial->ColorModulate( 1.0f, 1.0f, 1.0f );
		pMaterial->AlphaModulate( 1.0f );
		Shader_SetChainTextureState( nSurfID, 0 );

		for( ; nSurfID >= 0; nSurfID = MSurf_TextureChain( nSurfID ) )
		{
			Assert( !SurfaceHasDispInfo( nSurfID ) );

			g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_WORLD_TRIS_RENDERED, MSurf_VertCount( nSurfID ) - 2 );
			Shader_DrawSurfaceDynamic( nSurfID );
		}
	}
}


//-----------------------------------------------------------------------------
// The following methods will display debugging info in the middle of each surface
//-----------------------------------------------------------------------------
typedef void (*SurfaceDebugFunc_t)( int surfID, const Vector &vecCentroid );

void DrawSurfaceID( int surfID, const Vector &vecCentroid )
{
	char buf[32];
	sprintf(buf, "%d", surfID );
	CDebugOverlay::AddTextOverlay( vecCentroid, 0, buf );
}

void DrawSurfaceMaterial( int surfID, const Vector &vecCentroid )
{
	mtexinfo_t * pTexInfo = MSurf_TexInfo(surfID);

	const char *pFullMaterialName = pTexInfo->material ? pTexInfo->material->GetName() : "no material";
	const char *pSlash = strrchr( pFullMaterialName, '/' );
	const char *pMaterialName = strrchr( pFullMaterialName, '\\' );
	if (pSlash > pMaterialName)
		pMaterialName = pSlash;
	if (pMaterialName)
		++pMaterialName;
	else
		pMaterialName = pFullMaterialName;

	CDebugOverlay::AddTextOverlay( vecCentroid, 0, pMaterialName );
}


//-----------------------------------------------------------------------------
// Displays the surface id # in the center of the surface.
//-----------------------------------------------------------------------------
void Shader_DrawSurfaceDebuggingInfo( int surfID, SurfaceDebugFunc_t func )
{
	for( ; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
	{
		Assert( !SurfaceHasDispInfo( surfID ) );

		// Compute the centroid of the surface
		int nCount = MSurf_VertCount( surfID );
		if (nCount < 3)
			continue;

		int nFirstVertIndex = MSurf_FirstVertIndex( surfID );

		float flTotalArea = 0.0f;
		Vector vecNormal;
		Vector vecCentroid(0,0,0);
		int vertIndex = host_state.worldmodel->brush.vertindices[nFirstVertIndex];
		Vector vecApex = host_state.worldmodel->brush.vertexes[vertIndex].position;
		for (int v = 1; v < nCount - 1; ++v )
		{
			vertIndex = host_state.worldmodel->brush.vertindices[nFirstVertIndex+v];
			Vector v1 = host_state.worldmodel->brush.vertexes[vertIndex].position;
			vertIndex = host_state.worldmodel->brush.vertindices[nFirstVertIndex+v+1];
			Vector v2 = host_state.worldmodel->brush.vertexes[vertIndex].position;
			CrossProduct( v2 - v1, v1 - vecApex, vecNormal );
			float flArea = vecNormal.Length();
			flTotalArea += flArea;
			vecCentroid += (vecApex + v1 + v2) * flArea / 3.0f;
		}

		if (flTotalArea)
		{
			vecCentroid /= flTotalArea;
		}

		func( surfID, vecCentroid );
	}
}



//-----------------------------------------------------------------------------
// Doesn't draw internal triangles
//-----------------------------------------------------------------------------
void Shader_DrawWireframePolygons( int surfID )
{
	int nLineCount = 0;
	int i = surfID;
	for( ; i >= 0; i = MSurf_TextureChain( i ) )
	{
		int nCount = MSurf_VertCount( i );
		if (nCount >= 3)
		{
			nLineCount += MSurf_VertCount( i );
		}
	}

	if (nLineCount == 0)
		return;

	materialSystemInterface->Bind( g_materialWorldWireframe );
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( false );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, nLineCount );

	for( ; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
	{
		Assert( !SurfaceHasDispInfo( surfID ) );

		// Compute the centroid of the surface
		int nCount = MSurf_VertCount( surfID );
		if (nCount < 3)
			continue;

		int nFirstVertIndex = MSurf_FirstVertIndex( surfID );
		int nVertIndex = host_state.worldmodel->brush.vertindices[nFirstVertIndex + nCount - 1];
		Vector vecPrevPos = host_state.worldmodel->brush.vertexes[nVertIndex].position;
		for (int v = 0; v < nCount; ++v )
		{
			// world-space vertex
			nVertIndex = host_state.worldmodel->brush.vertindices[nFirstVertIndex + v];
			Vector& vec = host_state.worldmodel->brush.vertexes[nVertIndex].position;

			// output to mesh
			meshBuilder.Position3fv( vecPrevPos.Base() );
			meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv( vec.Base() );
			meshBuilder.AdvanceVertex();

			vecPrevPos = vec;
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Debugging mode, renders the wireframe.
//-----------------------------------------------------------------------------
static void Shader_DrawChainsWireframe(	int surfID )
{
	int nWireFrameMode = mat_wireframe.GetInt();
	switch( nWireFrameMode )
	{
	case 3:
		// Doesn't draw internal triangles
		Shader_DrawWireframePolygons( surfID );
		break;

	default:
		if( nWireFrameMode == 2 )
		{
			materialSystemInterface->Bind( g_materialWorldWireframeZBuffer );
		}
		else
		{
			materialSystemInterface->Bind( g_materialWorldWireframe );
		}
		DrawDynamicChain( surfID );
	}
}


//-----------------------------------------------------------------------------
// Debugging mode, renders the normals
//-----------------------------------------------------------------------------
static void Shader_DrawChainNormals( int surfID )
{
	Vector p, tVect, tangent, binormal;

	model_t *pWorld = host_state.worldmodel;
	materialSystemInterface->Bind( g_pMaterialWireframeVertexColor );

	for( ; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
	{
		IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_LINES, MSurf_VertCount( surfID ) * 3 );
	
		bool negate = TangentSpaceSurfaceSetup( surfID, tVect );

		int vertID;
		for( vertID = 0; vertID < MSurf_VertCount( surfID ); ++vertID )
		{
			int vertIndex = pWorld->brush.vertindices[MSurf_FirstVertIndex( surfID )+vertID];
			Vector& pos = pWorld->brush.vertexes[vertIndex].position;
			Vector& norm = pWorld->brush.vertnormals[ pWorld->brush.vertnormalindices[MSurf_FirstVertNormal( surfID )+vertID] ];

			TangentSpaceComputeBasis( tangent, binormal, norm, tVect, negate );

			meshBuilder.Position3fv( pos.Base() );
			meshBuilder.Color3ub( 255, 0, 0 );
			meshBuilder.AdvanceVertex();

			VectorMA( pos, 5.0f, norm, p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 255, 0, 0 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pos.Base() );
			meshBuilder.Color3ub( 0, 255, 0 );
			meshBuilder.AdvanceVertex();

			VectorMA( pos, 5.0f, binormal, p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 0, 255, 0 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pos.Base() );
			meshBuilder.Color3ub( 0, 0, 255 );
			meshBuilder.AdvanceVertex();

			VectorMA( pos, 5.0f, tangent, p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 0, 0, 255 );
			meshBuilder.AdvanceVertex();
		}
		
		meshBuilder.End();
		pMesh->Draw();
	}
}

static void Shader_DrawChainBumpBasis( int surfID )
{
	Vector p, tVect, tangent, binormal;

	model_t *pWorld = host_state.worldmodel;
	materialSystemInterface->Bind( g_pMaterialWireframeVertexColor );

	for( ; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
	{
		IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_LINES, MSurf_VertCount( surfID ) * 3 );
	
		bool negate = TangentSpaceSurfaceSetup( surfID, tVect );

		int vertID;
		for( vertID = 0; vertID < MSurf_VertCount( surfID ); ++vertID )
		{
			int vertIndex = pWorld->brush.vertindices[MSurf_FirstVertIndex( surfID )+vertID];
			Vector& pos = pWorld->brush.vertexes[vertIndex].position;
			Vector& norm = pWorld->brush.vertnormals[ pWorld->brush.vertnormalindices[MSurf_FirstVertNormal( surfID )+vertID] ];

			TangentSpaceComputeBasis( tangent, binormal, norm, tVect, negate );

			Vector worldSpaceBumpBasis[3];

			int i;
			for( i = 0; i < 3; i++ )
			{
				worldSpaceBumpBasis[i][0] = 
					g_localBumpBasis[i][0] * tangent[0] +
					g_localBumpBasis[i][1] * tangent[1] + 
					g_localBumpBasis[i][2] * tangent[2];
				worldSpaceBumpBasis[i][1] = 
					g_localBumpBasis[i][0] * binormal[0] +
					g_localBumpBasis[i][1] * binormal[1] + 
					g_localBumpBasis[i][2] * binormal[2];
				worldSpaceBumpBasis[i][2] = 
					g_localBumpBasis[i][0] * norm[0] +
					g_localBumpBasis[i][1] * norm[1] + 
					g_localBumpBasis[i][2] * norm[2];
			}

			meshBuilder.Position3fv( pos.Base() );
			meshBuilder.Color3ub( 255, 0, 0 );
			meshBuilder.AdvanceVertex();

			VectorMA( pos, 5.0f, worldSpaceBumpBasis[0], p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 255, 0, 0 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pos.Base() );
			meshBuilder.Color3ub( 0, 255, 0 );
			meshBuilder.AdvanceVertex();

			VectorMA( pos, 5.0f, worldSpaceBumpBasis[1], p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 0, 255, 0 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pos.Base() );
			meshBuilder.Color3ub( 0, 0, 255 );
			meshBuilder.AdvanceVertex();

			VectorMA( pos, 5.0f, worldSpaceBumpBasis[2], p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 0, 0, 255 );
			meshBuilder.AdvanceVertex();
		}
		
		meshBuilder.End();
		pMesh->Draw();
	}
}


//-----------------------------------------------------------------------------
// Debugging mode, renders the luxel grid.
//-----------------------------------------------------------------------------
static void Shader_DrawLuxels( int surfID )
{
	materialSystemInterface->Bind( g_materialDebugLuxels );

	for( ; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
	{
		Assert( !SurfaceHasDispInfo( surfID ) );

		// Gotta bind the lightmap page so the rendering knows the lightmap scale
		materialSystemInterface->BindLightmapPage( materialSortInfoArray[MSurf_MaterialSortID( surfID )].lightmapPageID );
		Shader_DrawSurfaceDynamic( surfID );
	}
}

static struct CShaderDebug
{
	bool		wireframe;
	bool		normals;
	bool		luxels;
	bool		bumpBasis;
	bool		surfaceid;
	bool		surfacematerials;
} g_ShaderDebug;


static ConVar mat_surfaceid("mat_surfaceid", "0");
static ConVar mat_surfacemat("mat_surfacemat", "0");


//-----------------------------------------------------------------------------
// Purpose: 
// Output : static void
//-----------------------------------------------------------------------------
static void ComputeDebugSettings( void )
{
	g_ShaderDebug.wireframe = mat_wireframe.GetBool();
	g_ShaderDebug.normals = mat_normals.GetBool();
	g_ShaderDebug.luxels = mat_luxels.GetBool();
	g_ShaderDebug.bumpBasis = mat_bumpbasis.GetBool();
	g_ShaderDebug.surfaceid = mat_surfaceid.GetBool();
	g_ShaderDebug.surfacematerials = mat_surfacemat.GetBool();
}

//-----------------------------------------------------------------------------
// Draw debugging information
//-----------------------------------------------------------------------------
static void DrawDebugInformation( int surfListID )
{
	// Overlay with wireframe if we're in that mode
	if( g_ShaderDebug.wireframe )
	{
		Shader_DrawChainsWireframe(surfListID);
	}

	// Overlay with normals if we're in that mode
	if( g_ShaderDebug.normals )
	{
		Shader_DrawChainNormals(surfListID);
	}

	if( g_ShaderDebug.bumpBasis )
	{
		Shader_DrawChainBumpBasis( surfListID );
	}
	
	// Overlay with luxel grid if we're in that mode
	if( g_ShaderDebug.luxels )
	{
		Shader_DrawLuxels( surfListID );
	}

	if ( g_ShaderDebug.surfaceid )
	{
		// Draw the surface id in the middle of the surfaces
		Shader_DrawSurfaceDebuggingInfo( surfListID, DrawSurfaceID );
	}
	else if ( g_ShaderDebug.surfacematerials )
	{
		// Draw the material name in the middle of the surfaces
		Shader_DrawSurfaceDebuggingInfo( surfListID, DrawSurfaceMaterial );
	}
}


//-----------------------------------------------------------------------------
// Draws all of the opaque non-displacement surfaces queued up previously
//-----------------------------------------------------------------------------
void Shader_DrawChains( int nSortGroup )
{
	VPROF("Shader_DrawChains");
	// Draw chains...
#ifdef USE_CONVARS
	if ( !mat_forcedynamic.GetInt() && !mat_drawflat.GetInt() )
#else
	if( 1 )
#endif
	{
		Shader_DrawChainsStatic( nSortGroup );
	}
	else
	{
		Shader_DrawChainsDynamic( nSortGroup );
	}

#ifdef USE_CONVARS
	// Debugging information
	int i = g_pFirstMatSort[nSortGroup];
	for ( i = g_pFirstMatSort[nSortGroup]; i >= 0; i = g_pWorldStatic[i].m_pNext[nSortGroup] )
	{
		int nSurfID = g_pWorldStatic[i].m_Surf[nSortGroup].m_nHeadSurfID;
		DrawDebugInformation( nSurfID );
	}
#endif
}


//-----------------------------------------------------------------------------
// Draws all of the opaque displacement surfaces queued up previously
//-----------------------------------------------------------------------------
void Shader_DrawDispChain( CDispChain& dispChain )
{
	VPROF_BUDGET( "Shader_DrawDispChain", VPROF_BUDGETGROUP_DISPLACEMENT_RENDERING );
	if (dispChain.GetHead())
	{
		MEASURE_TIMED_STAT( ENGINE_STATS_DISP_RENDER_TIME );

		DispInfo_RenderChain( dispChain.GetHead(), g_EngineRenderer->ViewGetCurrent().m_bOrtho );

		// Add all displacements to the shadow render list
		for( IDispInfo *pCur=dispChain.GetHead(); pCur; pCur = pCur->GetNextInRenderChain() )
		{
			ShadowDecalHandle_t decalHandle = MSurf_ShadowDecals( pCur->GetParent() );
			if (decalHandle != SHADOW_DECAL_HANDLE_INVALID)
			{
				g_pShadowMgr->AddShadowsOnSurfaceToRenderList( decalHandle );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Builds dynamic lightmaps for the surfaces we're going to render
//-----------------------------------------------------------------------------
static inline void BuildSurfaceChainLightmaps( int surfID )
{
	VPROF( "Shader_BuildDynamicLightmaps" );
	while( surfID >= 0 )
	{
		R_RenderDynamicLightmaps( surfID, vec3_origin );
		surfID = MSurf_TextureChain( surfID );
	}
}

static void BuildDispChainLightmaps( CDispChain &chain )
{
	VPROF( "Shader_BuildDynamicLightmaps" );
	for( IDispInfo *pCur = chain.GetHead(); pCur; pCur = pCur->GetNextInRenderChain() )
	{
		int parentSurfID = pCur->GetParent();
		R_RenderDynamicLightmaps( parentSurfID, vec3_origin );
	}
}

static void Shader_BuildDynamicLightmaps( void )
{
	VPROF( "Shader_BuildDynamicLightmaps" );
	int nSortGroup, i, j;

	// FIXME: Does this correctly build lightmaps for displacements?

	// Build all lightmaps for opaque surfaces
	for ( nSortGroup = 0; nSortGroup < MAX_MAT_SORT_GROUPS; ++nSortGroup)
	{
		i = g_pFirstMatSort[nSortGroup];
		for ( i = g_pFirstMatSort[nSortGroup]; i >= 0; i = g_pWorldStatic[i].m_pNext[nSortGroup] )
		{
			BuildSurfaceChainLightmaps( g_pWorldStatic[i].m_Surf[nSortGroup].m_nHeadSurfID );
		}
	}

	// Build all lightmaps for translucent surfaces
	for ( nSortGroup = 0; nSortGroup < MAX_MAT_SORT_GROUPS; ++nSortGroup)
	{
		for ( j = s_AlphaSurface.Count(); --j >= 0; )
		{
			BuildSurfaceChainLightmaps( s_AlphaSurface[j].m_HeadSurfID[nSortGroup] );
		}
	}

	// Mark disp chains since their msurfaces don't go in the material sort bins.
	for ( nSortGroup = 0; nSortGroup < MAX_MAT_SORT_GROUPS; ++nSortGroup)
	{
		BuildDispChainLightmaps( s_DispChain[nSortGroup] );
	}
	
	materialSystemInterface->FlushLightmaps();
}


//-----------------------------------------------------------------------------
// Compute if we're in or out of a fog volume
//-----------------------------------------------------------------------------
static void ComputeFogVolumeInfo( )
{
	s_FogVolume.m_InFogVolume = false;
	int leafID = ComputeLeaf( r_origin );
	if( leafID < 0 || leafID >= host_state.worldmodel->brush.numleafs )
		return;

	mleaf_t* pLeaf = &host_state.worldmodel->brush.leafs[leafID];
	s_FogVolume.m_FogVolumeID = pLeaf->leafWaterDataID;
	if( s_FogVolume.m_FogVolumeID == -1 )
		return;

	s_FogVolume.m_InFogVolume = true;

	mleafwaterdata_t* pLeafWaterData = &host_state.worldmodel->brush.leafwaterdata[pLeaf->leafWaterDataID];
	if( pLeafWaterData->surfaceTexInfoID == -1 )
	{
		// Should this ever happen?????
		s_FogVolume.m_FogEnabled = false;
		return;
	}
	mtexinfo_t* pTexInfo = &host_state.worldmodel->brush.texinfo[pLeafWaterData->surfaceTexInfoID];

	IMaterial* pMaterial = pTexInfo->material;
	if( pMaterial )
	{
		IMaterialVar* pFogColorVar	= pMaterial->FindVar( "$fogcolor", NULL );
		IMaterialVar* pFogEnableVar = pMaterial->FindVar( "$fogenable", NULL );
		IMaterialVar* pFogStartVar	= pMaterial->FindVar( "$fogstart", NULL );
		IMaterialVar* pFogEndVar	= pMaterial->FindVar( "$fogend", NULL );

		s_FogVolume.m_FogEnabled = pFogEnableVar->GetIntValue() ? true : false;
		pFogColorVar->GetVecValue( s_FogVolume.m_FogColor, 3 );
		s_FogVolume.m_FogStart		= -pFogStartVar->GetFloatValue();
		s_FogVolume.m_FogEnd		= -pFogEndVar->GetFloatValue();
		s_FogVolume.m_FogSurfaceZ	= pLeafWaterData->surfaceZ;
		s_FogVolume.m_FogMinZ		= pLeafWaterData->minZ;
		s_FogVolume.m_FogMode		= MATERIAL_FOG_LINEAR;
	}
	else
	{
		static bool bComplained = false;
		if( !bComplained )
		{
			Warning( "***Water vmt missing . . check console for missing materials!***\n" );
			bComplained = true;
		}
		s_FogVolume.m_FogEnabled = false;
	}
}


//-----------------------------------------------------------------------------
// Call this before rendering; it clears out the lists of stuff to render
//-----------------------------------------------------------------------------
void Shader_WorldBegin( void )
{
	// Cache the convars so we don't keep accessing them...
	s_ShaderConvars.m_bDrawWorld = r_drawworld.GetBool();
	s_ShaderConvars.m_nDrawLeaf = r_drawleaf.GetInt();

	Shader_ClearChains();
	Shader_ClearDispChain();

	// Clear out the decal list
	for (int j = 0; j < MAX_MAT_SORT_GROUPS; ++j)
	{
		s_DecalSurfaces[j].RemoveAll();
	}

	// We haven't found any visible leafs this frame
	s_VisibleLeafCount = 0;

	// Compute fog volume info for rendering
	ComputeFogVolumeInfo();

	// Clear out the render lists of shadows
	g_pShadowMgr->ClearShadowRenderList( );
}


//-----------------------------------------------------------------------------
// Set up render state for a particular sort group
//-----------------------------------------------------------------------------

/*
static void	Shader_SetSortGroupRenderState( int group )
{
	FogState_t* pFogState = (group == MAT_SORT_GROUP_NORMAL) ? &s_WorldFog : &s_FogVolume;

	if( pFogState->m_FogEnabled )
	{
		materialSystemInterface->FogMode( pFogState->m_FogMode );
		materialSystemInterface->FogColor3fv( pFogState->m_FogColor );
		materialSystemInterface->FogStart( pFogState->m_FogStart );
		materialSystemInterface->FogEnd( pFogState->m_FogEnd );
	}
	else
	{
		materialSystemInterface->FogMode( MATERIAL_FOG_NONE );
	}
}
*/


static const int s_pDrawWorldListsToSortGroup[MAX_MAT_SORT_GROUPS] = 
{
	MAT_SORT_GROUP_STRICTLY_ABOVEWATER,
	MAT_SORT_GROUP_STRICTLY_UNDERWATER,
	MAT_SORT_GROUP_INTERSECTS_WATER_SURFACE,
	MAT_SORT_GROUP_WATERSURFACE,
};


//-----------------------------------------------------------------------------
// Performs the z-fill
//-----------------------------------------------------------------------------
static void Shader_WorldZFillSurfChain( int nSurfID, CMeshBuilder &meshBuilder, int &nStartVert )
{
	mvertex_t *pWorldVerts = host_state.worldmodel->brush.vertexes;

	int nNextSurfID;
	for( ; nSurfID >= 0; nSurfID = nNextSurfID )
	{
		// Precache the next surface id...
		nNextSurfID = MSurf_TextureChain( nSurfID );

		if ( (MSurf_Flags( nSurfID ) & SURFDRAW_NODE) == 0 )
			continue;

		int nSurfTriangleCount = MSurf_VertCount( nSurfID ) - 2;

		unsigned short *pVertIndex = &(host_state.worldmodel->brush.vertindices[MSurf_FirstVertIndex( nSurfID )]);

		// add surface to this batch
		switch (nSurfTriangleCount)
		{
		case 1:
			meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
			meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
			meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
			meshBuilder.AdvanceVertex();

			meshBuilder.FastIndex( nStartVert );
			meshBuilder.FastIndex( nStartVert + 1 );
			meshBuilder.FastIndex( nStartVert + 2 );

			break;

		case 2:
			meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
			meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
			meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
			meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
			meshBuilder.AdvanceVertex();

			meshBuilder.FastIndex( nStartVert );
			meshBuilder.FastIndex( nStartVert + 1 );
			meshBuilder.FastIndex( nStartVert + 2 );
			meshBuilder.FastIndex( nStartVert );
			meshBuilder.FastIndex( nStartVert + 2 );
			meshBuilder.FastIndex( nStartVert + 3 );
			break;

		default:
			{
				for ( unsigned short v = 0; v < nSurfTriangleCount; ++v )
				{
					meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
					meshBuilder.AdvanceVertex();

					meshBuilder.FastIndex( nStartVert );
					meshBuilder.FastIndex( nStartVert + v + 1 );
					meshBuilder.FastIndex( nStartVert + v + 2 );
				}

				meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv( pWorldVerts[*pVertIndex++].position.Base() );
				meshBuilder.AdvanceVertex();
			}
			break;
		}

		nStartVert += nSurfTriangleCount + 2;
	}
}


//-----------------------------------------------------------------------------
// Performs the z-fill
//-----------------------------------------------------------------------------
static void Shader_WorldZFill( unsigned long flags )
{
	// First, count the number of vertices + indices
	int nVertexCount = 0;
	int nIndexCount = 0;

	int g;
	for ( g = 0; g < MAX_MAT_SORT_GROUPS; ++g )
	{
		if ( ( flags & ( 1 << g ) ) == 0 )
			continue;

		int nSortGroup = s_pDrawWorldListsToSortGroup[g];

		int i = g_pFirstMatSort[nSortGroup];
		for ( i = g_pFirstMatSort[nSortGroup]; i >= 0; i = g_pWorldStatic[i].m_pNext[nSortGroup] )
		{
			nVertexCount += g_pWorldStatic[i].m_Surf[nSortGroup].m_nVertexCountNoDetail;
			nIndexCount += g_pWorldStatic[i].m_Surf[nSortGroup].m_nIndexCountNoDetail;
		}
	}

	if ( nVertexCount == 0 )
		return;

	materialSystemInterface->Bind( g_pMaterialWriteZ );
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( false );

	// batchIndexCount is the number of indices we can fit in this batch
	// each batch must be smaller than SHADER_MAX_INDICES or the material system will fail.
	int nBatchIndexCount = min(nIndexCount, SHADER_MAX_INDICES);

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertexCount, nBatchIndexCount );

	int nStartVert = 0;
	for ( g = 0; g < MAX_MAT_SORT_GROUPS; ++g )
	{
		if ( ( flags & ( 1 << g ) ) == 0 )
			continue;

		int nSortGroup = s_pDrawWorldListsToSortGroup[g];
		int i = g_pFirstMatSort[nSortGroup];
		for ( i = g_pFirstMatSort[nSortGroup]; i >= 0; i = g_pWorldStatic[i].m_pNext[nSortGroup] )
		{
			// Check to see if we can add this list to the current batch...
			int nCurrIndexCount = g_pWorldStatic[i].m_Surf[nSortGroup].m_nIndexCountNoDetail;
			if ( nCurrIndexCount == 0 )
				continue;

			Assert( nCurrIndexCount <= SHADER_MAX_INDICES ); 
			if ( nBatchIndexCount < nCurrIndexCount )
			{
				// Nope, fire off the current batch...
				meshBuilder.End();
				pMesh->Draw();
				nBatchIndexCount = min(nIndexCount, SHADER_MAX_INDICES);
				pMesh = materialSystemInterface->GetDynamicMesh( false );
				meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertexCount, nBatchIndexCount );
				nStartVert = 0;
			}

			nVertexCount -= g_pWorldStatic[i].m_Surf[nSortGroup].m_nVertexCountNoDetail;
			nBatchIndexCount -= nCurrIndexCount;
			nIndexCount -= nCurrIndexCount;

			int nSurfID = g_pWorldStatic[i].m_Surf[nSortGroup].m_nHeadSurfID;
			Shader_WorldZFillSurfChain( nSurfID, meshBuilder, nStartVert );
		}
	}

	meshBuilder.End();
	pMesh->Draw();

	// FIXME: Do fast z reject on displacements!
}


//-----------------------------------------------------------------------------
// Call this after lists of stuff to render are made; it renders opaque surfaces
//-----------------------------------------------------------------------------
static void Shader_WorldEnd( unsigned long flags )
{
	VPROF("Shader_WorldEnd");

	// Draw the skybox
	if ( flags & DRAWWORLDLISTS_DRAW_SKYBOX )
	{
		R_DrawSkyBox( g_EngineRenderer->GetZFar() );
	}

	// Perform the fast z-fill pass
	bool bFastZReject = g_pMaterialSystemHardwareConfig->HasFastZReject();

	// FIXME: Remove? We may need this to test what we want to use on various cards
	if ( r_fastzreject.GetInt() != -1 )
	{
		bFastZReject = (r_fastzreject.GetInt() != 0);
	}

	if ( bFastZReject )
	{
		Shader_WorldZFill( flags );
	}
	
	// Gotta draw each sort group
	// Draw the fog volume first, if there is one, because it turns out
	// that we only draw fog volumes if we're in the fog volume, which
	// means it's closer. We want to render closer things first to get
	// fast z-reject.
	int i;
	for ( i = 0; i < MAX_MAT_SORT_GROUPS; i++ )
	{
		if( !( flags & ( 1 << i ) ) )
		{
			continue;
		}
		int nSortGroup = s_pDrawWorldListsToSortGroup[i];

		// Set up render state for a particular sort group

//		Shader_SetSortGroupRenderState( nSortGroup );

		// Draws opaque non-displacement surfaces
		Shader_DrawChains( nSortGroup );

		// Draws opaque displacement surfaces
		Shader_DrawDispChain( s_DispChain[nSortGroup] );

		// Render the fragments from the surfaces + displacements.
		// FIXME: Actually, this call is irrelevant because it's done from
		// within DrawDispChain currently, but that should change.
		// We need to split out the disp decal rendering from DrawDispChain
		// and do it after overlays are rendered....
		OverlayMgr()->RenderOverlays();

		// Draws decals lying on opaque non-displacement surfaces
		DecalSurfaceDraw( nSortGroup );

		// Adds shadows to render lists
		int j = g_pFirstMatSort[nSortGroup];
		for ( j = g_pFirstMatSort[nSortGroup]; j >= 0; j = g_pWorldStatic[j].m_pNext[nSortGroup] )
		{
			int nSurfID = g_pWorldStatic[j].m_Surf[nSortGroup].m_nHeadSurfID;

			for( ; nSurfID >= 0; nSurfID = MSurf_TextureChain( nSurfID ) )
			{
				Assert( !SurfaceHasDispInfo( nSurfID ) );

				ShadowDecalHandle_t decalHandle = MSurf_ShadowDecals( nSurfID );
				if (decalHandle != SHADOW_DECAL_HANDLE_INVALID)
				{
					// No shadows on water surfaces
					if ((MSurf_Flags( nSurfID ) & SURFDRAW_WATERSURFACE) == 0)
					{
						g_pShadowMgr->AddShadowsOnSurfaceToRenderList( decalHandle );
					}
				}
			}
		}

		// Draw shadows
		g_pShadowMgr->RenderShadows( );
	}
}


//-----------------------------------------------------------------------------
// Renders translucent surfaces
//-----------------------------------------------------------------------------
static const int s_DrawWorldListsToSortGroup[MAX_MAT_SORT_GROUPS] = 
{
	MAT_SORT_GROUP_STRICTLY_ABOVEWATER,
	MAT_SORT_GROUP_STRICTLY_UNDERWATER,
	MAT_SORT_GROUP_INTERSECTS_WATER_SURFACE,
	MAT_SORT_GROUP_WATERSURFACE,
};

bool Shader_LeafContainsTranslucentSurfaces( int sortIndex, unsigned long flags )
{
	int i;
	for ( i = 0; i < MAX_MAT_SORT_GROUPS; ++i )
	{
		if( !( flags & ( 1 << i ) ) )
			continue;

		int sortGroup = s_DrawWorldListsToSortGroup[i];
	
		// Set the fog state here since it will be the same for all things
		// in this list of translucent objects (except for displacements)
		int surfID = s_AlphaSurface[sortIndex].m_HeadSurfID[sortGroup];
		CDispChain &dispChain = s_AlphaSurface[sortIndex].m_DispChain[sortGroup];

		if ((surfID >= 0) || (dispChain.GetHead() != NULL))
			return true;
	}

	return false;
}

void Shader_DrawTranslucentSurfaces( int sortIndex, unsigned long flags )
{
	// Gotta draw each sort group
	// Draw the fog volume first, if there is one, because it turns out
	// that we only draw fog volumes if we're in the fog volume, which
	// means it's closer. We want to render closer things first to get
	// fast z-reject.
	int i;
	for ( i = 0; i < MAX_MAT_SORT_GROUPS; ++i )
	{
		if( !( flags & ( 1 << i ) ) )
		{
			continue;
		}
		int sortGroup = s_DrawWorldListsToSortGroup[i];
	
		// Set the fog state here since it will be the same for all things
		// in this list of translucent objects (except for displacements)
		int surfID = s_AlphaSurface[sortIndex].m_HeadSurfID[sortGroup];
		CDispChain &dispChain = s_AlphaSurface[sortIndex].m_DispChain[sortGroup];

		// Empty? skip...
		if ((surfID < 0) && (dispChain.GetHead() == NULL))
			continue;

		// Interate in back-to-front order
		for( ; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
		{
			materialSystemInterface->Bind( MSurf_TexInfo( surfID )->material );

			Assert( MSurf_MaterialSortID( surfID ) >= 0 && 
				    MSurf_MaterialSortID( surfID ) < g_NumMaterialSortBins );
			materialSystemInterface->BindLightmapPage( materialSortInfoArray[MSurf_MaterialSortID( surfID )].lightmapPageID );
			
//			NOTE: Since a static vb/dynamic ib IMesh doesn't buffer, we shouldn't use this
//			since it causes a lock and drawindexedprimitive per surface! (gary)
//			Shader_DrawSurfaceStatic( surfID );
			Shader_DrawSurfaceDynamic( surfID );

			g_EngineStats.IncrementCountedStat( ENGINE_STATS_TRANSLUCENT_WORLD_TRIANGLES, 
				MSurf_VertCount( surfID ) - 2 );

			// Draw overlays on the surface.
			OverlayMgr()->AddFragmentListToRenderList( MSurf_OverlayFragmentList( surfID ) );
			OverlayMgr()->RenderOverlays( );

		    // Draw decals on the surface
			DrawDecalsOnSingleSurface( surfID );

			// Add shadows too....
			ShadowDecalHandle_t decalHandle = MSurf_ShadowDecals( surfID );
			if (decalHandle != SHADOW_DECAL_HANDLE_INVALID)
			{
				g_pShadowMgr->AddShadowsOnSurfaceToRenderList( decalHandle );
			}
		}

		// Now draw the translucent displacements; we need to do these *after* the
		// non-displacement surfaces because most likely the displacement will always
		// be in front (or at least not behind) the non-displacement translucent surfaces
		// that exist in the same leaf.
		
		// Draws translucent displacement surfaces
		Shader_DrawDispChain( dispChain );
		
		// Draw shadows
		g_pShadowMgr->RenderShadows( );

		// Draw wireframe, etc information
		DrawDebugInformation( surfID );
	}
}



//=============================================================
//
//	WORLD MODEL
//
//=============================================================

void R_DrawSurface( int surfID )
{
	ASSERT_SURF_VALID( surfID );
	Assert( !SurfaceHasDispInfo( surfID ) );
	if ( MSurf_Flags( surfID ) & SURFDRAW_SKY )
	{
		R_AddSkySurface( surfID );
	}
//	else if ( surf->texinfo->material->IsTranslucent() )
	else if( MSurf_Flags( surfID ) & SURFDRAW_TRANS )
	{
		Shader_TranslucentWorldSurface( surfID );
	}
	else
	{
		Shader_WorldSurface( surfID );
	}
}


//-----------------------------------------------------------------------------
// Draws displacements in a leaf
//-----------------------------------------------------------------------------
static inline void DrawDisplacementsInLeaf( mleaf_t* pLeaf )
{
	// add displacement surfaces
	if (!pLeaf->m_pDisplacements)
		return;

	CDispIterator it( pLeaf->m_pDisplacements, CDispLeafLink::LIST_LEAF );
	while( it.IsValid() )
	{
		CDispLeafLink *pCur = it.Inc();

		// NOTE: We're not using the displacement's touched method here 
		// because we're just using the parent surface's visframe in the
		// surface add methods below...
		IDispInfo *pDispInfo = static_cast<IDispInfo*>( pCur->m_pDispInfo );
		int parentSurfID = pDispInfo->GetParent();

		// already processed this frame? Then don't do it again!
		if ( MSurf_VisFrame( parentSurfID ) != r_surfacevisframe)
		{
			MSurf_VisFrame( parentSurfID ) = r_surfacevisframe;

			if ( MSurf_Flags( parentSurfID ) & SURFDRAW_TRANS)
			{
				Shader_TranslucentDisplacementSurface( parentSurfID );
			}
			else
			{
				Shader_DisplacementSurface( parentSurfID );
			}
		}
	}
}

int LeafToIndex( mleaf_t* pLeaf );

//-----------------------------------------------------------------------------
// Updates visibility + alpha lists
//-----------------------------------------------------------------------------
static inline void UpdateVisibleLeafLists( mleaf_t* pleaf )
{
	// Consistency check...
	Assert( s_AlphaSurface.Count() == s_VisibleLeafCount );
	
	// Add this leaf to the list of visible leafs
	int leafIndex = LeafToIndex( pleaf );
	s_pVisibleLeaf [ s_VisibleLeafCount ] = leafIndex;
	s_pVisibleLeafFogVolume[ s_VisibleLeafCount ] =	pleaf->leafWaterDataID;
	++s_VisibleLeafCount;
	
	int i =	s_AlphaSurface.AddToTail( );
	AlphaSortInfo_t& info = s_AlphaSurface[i];
	
	COMPILE_TIME_ASSERT( MAX_MAT_SORT_GROUPS == 4 );

	info.m_HeadSurfID[0] = -1;
	info.m_HeadSurfID[1] = -1;
	info.m_HeadSurfID[2] = -1;
	info.m_HeadSurfID[3] = -1;
}

 
//-----------------------------------------------------------------------------
// Draws all displacements + surfaces in a leaf
//-----------------------------------------------------------------------------
static void FASTCALL R_DrawLeaf( mleaf_t *pleaf )
{
	// Add this leaf to the list of visible leaves
	UpdateVisibleLeafLists( pleaf );

	// Debugging to only draw at a particular leaf
#ifdef USE_CONVARS
	if ( (s_ShaderConvars.m_nDrawLeaf >= 0) && (s_ShaderConvars.m_nDrawLeaf != LeafToIndex(pleaf)) )
		return;
#endif

	// add displacement surfaces
	DrawDisplacementsInLeaf( pleaf );

#ifdef USE_CONVARS
	if( !s_ShaderConvars.m_bDrawWorld )
		return;
#endif

	// Add non-displacement surfaces
	int i;
	int nSurfaceCount = pleaf->nummarksurfaces;
	int *pSurfID = &host_state.worldmodel->brush.marksurfaces[pleaf->firstmarksurface];
	for ( i = 0; i < nSurfaceCount; ++i, ++pSurfID )
	{
		// garymctoptimize - can we prefetch the next surfaces?
		// We seem to be taking a huge hit here for referencing the surface for the first time.
		int surfID = *pSurfID;
		ASSERT_SURF_VALID( surfID );

		// Don't add surfaces that have displacement; they are handled above
		// In fact, don't even set the vis frame; we need it unset for translucent
		// displacement code
		if ( SurfaceHasDispInfo(surfID) )
			continue;

		// Don't process the same surface twice
		if ( MSurf_VisFrame( surfID ) == r_surfacevisframe)
			continue;

		// Mark this surface as being in a visible leaf this frame. If this
		// surface is meant to be drawn at a node (SURFDRAW_NODE), 
		// then it will be drawn in the recursive code down below.
		MSurf_VisFrame( surfID ) = r_surfacevisframe;

		if ( MSurf_Flags( surfID ) & (SURFDRAW_NODE | SURFDRAW_NODRAW) )
			continue;

		// Back face cull; only func_detail are drawn here
		if ( (MSurf_Flags( surfID ) & SURFDRAW_NOCULL) == 0 )
		{
			if ( (DotProduct(MSurf_Plane( surfID ).normal, modelorg) - 
				MSurf_Plane( surfID ).dist ) < -BACKFACE_EPSILON )
				continue;
		}

		R_DrawSurface( surfID );
	}
}


//-----------------------------------------------------------------------------
// Purpose: recurse on the BSP tree, calling the surface visitor
// Input  : *node - BSP node
//-----------------------------------------------------------------------------
static ConVar r_frustumcullworld( "r_frustumcullworld", "1" );

static void R_RecursiveWorldNode (mnode_t *node, int nCullMask )
{
	int			side;
	cplane_t	*plane;
	float		dot;

	while (true)
	{
		// no polygons in solid nodes
		if (node->contents == CONTENTS_SOLID)
			return;		// solid

		// Check PVS signature
		if (node->visframe != r_visframecount)
			return;

		// Cull against the screen frustum or the appropriate area's frustum.
		if ( nCullMask != FRUSTUM_SUPPRESS_CLIPPING )
		{
			if (node->contents >= -1)
			{
				if ((nCullMask != 0) || ( node->area > 0 ))
				{
					if ( R_CullNode( node, nCullMask ) )
						return;
				}
			}
			else
			{
				// This prevents us from culling nodes that are too small to worry about
				if (node->contents == -2)
				{
					nCullMask = FRUSTUM_SUPPRESS_CLIPPING;
				}
			}
		}

		// if a leaf node, draw stuff
		if (node->contents >= 0)
		{
			R_DrawLeaf( (mleaf_t *)node );
			return;
		}

		// node is just a decision point, so go down the apropriate sides

		// find which side of the node we are on
		plane = node->plane;
		if ( plane->type <= PLANE_Z )
		{
			dot = modelorg[plane->type] - plane->dist;
		}
		else
		{
			dot = DotProduct (modelorg, plane->normal) - plane->dist;
		}

		// recurse down the children, closer side first.
		// We have to do this because we need to find if the surfaces at this node
		// exist in any visible leaves closer to the camera than the node is. If so,
		// their r_surfacevisframe is set to indicate that we need to render them
		// at this node.
		if (dot >= 0)
			side = 0;
		else
			side = 1;

		// Recurse down the side closer to the camera
		R_RecursiveWorldNode (node->children[side], nCullMask );

		// draw stuff on the node

		int i;
		int nSurfaceCount = node->numsurfaces;
		int surfID = node->firstsurface;
		for ( i = 0; i < nSurfaceCount; ++i, ++surfID )
		{
			// Don't add surfaces that have displacement
			if ( SurfaceHasDispInfo( surfID ) )
				continue;

			// If a surface is marked to draw at a node, then it's not a func_detail.
			// Only func_detail render at leaves. In the case of normal world surfaces,
			// we only want to render them if they intersect a visible leaf.
			int nFlags = MSurf_Flags( surfID );

			if ( !( nFlags & SURFDRAW_NODE) )
				continue;

			// Only render things at this node that have previously been marked as visible
			if (MSurf_VisFrame( surfID ) != r_surfacevisframe)
				continue;

			if ( nFlags & SURFDRAW_NODRAW )
				continue;

			if ( !(nFlags & SURFDRAW_UNDERWATER) && ( side ^ !!(nFlags & SURFDRAW_PLANEBACK)) )
				continue;		// wrong side

			R_DrawSurface( surfID );
		}

		// recurse down the side farther from the camera
		// NOTE: With this while loop, this is identical to just calling
		// R_RecursiveWorldNode (node->children[!side], nCullMask );
		node = node->children[!side];
	}
}


//-----------------------------------------------------------------------------
// Set up fog for a particular leaf
//-----------------------------------------------------------------------------

float R_GetWaterHeight( int fogVolume )
{
	if( fogVolume < 0 || fogVolume > host_state.worldmodel->brush.numleafwaterdata )
	{
//		Assert( 0 );
		return 1000000.0f;
	}
	mleafwaterdata_t* pLeafWaterData = &host_state.worldmodel->brush.leafwaterdata[fogVolume];
	return pLeafWaterData->surfaceZ;
}

IMaterial *R_GetFogVolumeMaterial( int fogVolume )
{
	if( fogVolume < 0 || fogVolume > host_state.worldmodel->brush.numleafwaterdata )
	{
		Assert( 0 );
		return NULL;
	}
	mleafwaterdata_t* pLeafWaterData = &host_state.worldmodel->brush.leafwaterdata[fogVolume];
	mtexinfo_t* pTexInfo = &host_state.worldmodel->brush.texinfo[pLeafWaterData->surfaceTexInfoID];

	IMaterial* pMaterial = pTexInfo->material;
	return pMaterial;
}

void R_SetFogVolumeState( int fogVolume, bool useHeightFog )
{
	if( fogVolume < 0 || fogVolume > host_state.worldmodel->brush.numleafwaterdata )
	{
		Assert( 0 );
		return;
	}
	mleafwaterdata_t* pLeafWaterData = &host_state.worldmodel->brush.leafwaterdata[fogVolume];
	mtexinfo_t* pTexInfo = &host_state.worldmodel->brush.texinfo[pLeafWaterData->surfaceTexInfoID];

	IMaterial* pMaterial = pTexInfo->material;
	if( !pMaterial )
	{
		materialSystemInterface->FogMode( MATERIAL_FOG_NONE );
		return;
	}
	IMaterialVar* pFogColorVar	= pMaterial->FindVar( "$fogcolor", NULL );
	IMaterialVar* pFogEnableVar = pMaterial->FindVar( "$fogenable", NULL );
	IMaterialVar* pFogStartVar	= pMaterial->FindVar( "$fogstart", NULL );
	IMaterialVar* pFogEndVar	= pMaterial->FindVar( "$fogend", NULL );

	if( pMaterial && pFogEnableVar->GetIntValue() && fog_enable_water_fog.GetBool() )
	{
		materialSystemInterface->SetFogZ( pLeafWaterData->surfaceZ );
		if( useHeightFog )
		{
			materialSystemInterface->FogMode( MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		}
		else
		{
			materialSystemInterface->FogMode( MATERIAL_FOG_LINEAR );
		}
		float fogColor[3];
		pFogColorVar->GetVecValue( fogColor, 3 );
		materialSystemInterface->FogColor3fv( fogColor );
		materialSystemInterface->FogStart( pFogStartVar->GetFloatValue() );
		materialSystemInterface->FogEnd( pFogEndVar->GetFloatValue() );
	}
	else
	{
		materialSystemInterface->FogMode( MATERIAL_FOG_NONE );
	}
}

//-----------------------------------------------------------------------------
// RecursiveWorldNode set up some temporary lists for displacements
// Now we have to process these into the final enchilada
//-----------------------------------------------------------------------------

static void R_BuildDisplacementLists( )
{
	// For opaque guys, we just need to iterate over the lists in material 
	// order, figure out which fog volume they're in, and add em to the 
	// appropriate list. I'm going to assume the displacement is in the
	// fog volume that its base surface is in.
	int group;
	for( group = 0; group < MAX_MAT_SORT_GROUPS; group++ )
	{
		int i;
		for ( i = 0; i < g_NumMaterialSortBins; ++i)
		{
			int surfID = g_pWorldStatic[i].m_DisplacementSurfID[group];
			for( ; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
			{
				// Hook into the chain of opaque objects to render that use the
				// same material and the same lightmap.
				// Each surface can be in more than one group.
				// Add shadows too....
				Assert( MSurf_DispInfo( surfID ) );
				s_DispChain[group].AddTo( MSurf_DispInfo( surfID ) );
			}
		}
	}
	// FIXME: Add opaque displacement decal code here
}


static inline bool R_CullNodeTopView( mnode_t *pNode )
{
	Vector2D delta, size;
	Vector2DSubtract( pNode->m_vecCenter.AsVector2D(), s_OrthographicCenter, delta );
	Vector2DAdd( pNode->m_vecHalfDiagonal.AsVector2D(), s_OrthographicHalfDiagonal, size );
	return ( FloatMakePositive( delta.x ) > size.x ) ||
		( FloatMakePositive( delta.y ) > size.y );
}


//-----------------------------------------------------------------------------
// Draws all displacements + surfaces in a leaf
//-----------------------------------------------------------------------------
static void R_DrawTopViewLeaf( mleaf_t *pleaf )
{
	// Add this leaf to the list of visible leaves
	UpdateVisibleLeafLists( pleaf );

	// add displacement surfaces
	DrawDisplacementsInLeaf( pleaf );

#ifdef USE_CONVARS
	if( !s_ShaderConvars.m_bDrawWorld )
		return;
#endif

	// Add non-displacement surfaces
	for ( int i = 0; i < pleaf->nummarksurfaces; i++ )
	{
		int surfID = host_state.worldmodel->brush.marksurfaces[pleaf->firstmarksurface + i];

		// Don't add surfaces that have displacement; they are handled above
		// In fact, don't even set the vis frame; we need it unset for translucent
		// displacement code
		if ( SurfaceHasDispInfo(surfID) )
			continue;

		// Don't process the same surface twice
		if (MSurf_VisFrame( surfID ) == r_surfacevisframe)
			continue;

		// Mark this surface as being in a visible leaf this frame. If this
		// surface is meant to be drawn at a node (SURFDRAW_NODE), 
		// then it will be drawn in the recursive code down below.
		MSurf_VisFrame( surfID ) = r_surfacevisframe;

		if ( MSurf_Flags( surfID ) & (SURFDRAW_NODE | SURFDRAW_NODRAW) )
			continue;

		// Back face cull; only func_detail are drawn here
		if ( (MSurf_Flags( surfID ) & SURFDRAW_NOCULL) == 0 )
		{
			if (MSurf_Plane( surfID ).normal.z <= 0.0f)
				continue;
		}

		// FIXME: For now, blow off translucent world polygons.
		// Gotta solve the problem of how to render them all, unsorted,
		// in a pass after the opaque world polygons, and before the
		// translucent entities.

		if ( !( MSurf_Flags( surfID ) & SURFDRAW_TRANS ))
//		if ( !surf->texinfo->material->IsTranslucent() )
			Shader_WorldSurface( surfID );
	}
}


//-----------------------------------------------------------------------------
// Fast path for rendering top-views
//-----------------------------------------------------------------------------
void R_RenderWorldTopView( mnode_t *node )
{
	do
	{
		// no polygons in solid nodes
		if (node->contents == CONTENTS_SOLID)
			return;		// solid

		// Check PVS signature
		if (node->visframe != r_visframecount)
			return;

		// Cull against the screen frustum or the appropriate area's frustum.
		if( R_CullNodeTopView( node ) )
			return;

		// if a leaf node, draw stuff
		if (node->contents >= 0)
		{
			R_DrawTopViewLeaf( (mleaf_t *)node );
			return;
		}

#ifdef USE_CONVARS
		if (s_ShaderConvars.m_bDrawWorld)
#endif
		{
			// draw stuff on the node
			int surfID = node->firstsurface;
			for ( int i = 0; i < node->numsurfaces; i++, surfID++ )
			{
				// Don't add surfaces that have displacement
				if ( SurfaceHasDispInfo( surfID ) )
					continue;
							
				// If a surface is marked to draw at a node, then it's not a func_detail.
				// Only func_detail render at leaves. In the case of normal world surfaces,
				// we only want to render them if they intersect a visible leaf.
				if ( !(MSurf_Flags( surfID ) & SURFDRAW_NODE) )
					continue;

				// Don't render twice.
				if ( MSurf_VisFrame( surfID ) == r_surfacevisframe)
					continue;

				MSurf_VisFrame( surfID ) = r_surfacevisframe;

				if ( MSurf_Flags( surfID ) & (SURFDRAW_NODRAW|SURFDRAW_UNDERWATER|SURFDRAW_SKY) )
					continue;

				// Back face cull
				if ( (MSurf_Flags( surfID ) & SURFDRAW_NOCULL) == 0 )
				{
					if (MSurf_Plane( surfID ).normal.z <= 0.0f)
						continue;
				}

				// FIXME: For now, blow off translucent world polygons.
				// Gotta solve the problem of how to render them all, unsorted,
				// in a pass after the opaque world polygons, and before the
				// translucent entities.

				if ( !( MSurf_Flags( surfID ) & SURFDRAW_TRANS ) )
//				if ( !surf->texinfo->material->IsTranslucent() )
					Shader_WorldSurface( surfID );
			}
		}

		// Recurse down both children, we don't care the order...
		R_RenderWorldTopView (node->children[0]);
		node = node->children[1];

	} while (node);
}


//-----------------------------------------------------------------------------
// Spews the leaf we're in
//-----------------------------------------------------------------------------

static void SpewLeaf()
{
	int leaf = ComputeLeaf( g_EngineRenderer->ViewOrigin() );
	Con_Printf(	"view leaf %d\n", leaf );
}

//-----------------------------------------------------------------------------
// Main entry points for starting + ending rendering the world
//-----------------------------------------------------------------------------

void R_BuildWorldLists( WorldListInfo_t* pInfo, bool updateLightmaps, int iForceViewLeaf )
{
	VPROF( "R_BuildWorldLists" );
	g_EngineStats.BeginDrawWorld();
	VectorCopy( g_EngineRenderer->ViewOrigin(), modelorg );

	static ConVar r_spewleaf("r_spewleaf", "0");
#ifdef USE_CONVARS
	if (r_spewleaf.GetInt())
	{
		SpewLeaf();
	}
#endif

	Shader_WorldBegin();
	++r_surfacevisframe;

	if (!r_drawtopview)
	{
		R_SetupAreaBits( iForceViewLeaf );
		R_RecursiveWorldNode( host_state.worldmodel->brush.nodes, r_frustumcullworld.GetBool() ? FRUSTUM_CLIP_ALL : FRUSTUM_SUPPRESS_CLIPPING );
	}
	else
	{
		R_RenderWorldTopView( host_state.worldmodel->brush.nodes );
	}

	// RecursiveWorldNode set up some temporary lists for displacements
	// Now we have to process these into the final enchilada
	R_BuildDisplacementLists( );

	// User changed gamma or something like that
	if ( g_RebuildLightmaps )
	{
		R_RedownloadAllLightmaps( vec3_origin );
	}

	// This builds all lightmaps, including those for translucent surfaces
	// Don't bother in topview?
	if (updateLightmaps && !r_drawtopview)
	{
		Shader_BuildDynamicLightmaps();
	}

	// Return the back-to-front leaf ordering
	if (pInfo)
	{
		if( s_FogVolume.m_InFogVolume )
		{
			pInfo->m_ViewFogVolume = MAT_SORT_GROUP_STRICTLY_UNDERWATER;
		}
		else
		{
			pInfo->m_ViewFogVolume = MAT_SORT_GROUP_STRICTLY_ABOVEWATER;
		}
		pInfo->m_LeafCount = s_VisibleLeafCount;
		pInfo->m_pLeafList = s_pVisibleLeaf;
		pInfo->m_pLeafFogVolume = s_pVisibleLeafFogVolume;
	}
}

static int s_VisibleFogVolume = -1;
static int s_VisibleFogVolumeLeaf = -1;
Vector s_VisibleFogVolumeSearchPoint;

// return true to continue searching
static bool RecursiveGetVisibleFogVolume( mnode_t *node )
{
	int			side;
	cplane_t	*plane;
	float		dot;

	// no polygons in solid nodes
	if (node->contents == CONTENTS_SOLID)
		return true;		// solid

	// Check PVS signature
	if (node->visframe != r_visframecount)
		return true;

	// Cull against the screen frustum or the appropriate area's frustum.
	int fixmeTempRemove = FRUSTUM_CLIP_ALL;
	if( R_CullNode( node, fixmeTempRemove ) )
		return true;

	// if a leaf node, check if we are in a fog volume and get outta here.
	if (node->contents >= 0)
	{
		mleaf_t *pLeaf = (mleaf_t *)node;
		int fogVolumeID = pLeaf->leafWaterDataID;
		if( fogVolumeID != -1 )
		{
			s_VisibleFogVolume = fogVolumeID;
			s_VisibleFogVolumeLeaf = pLeaf - host_state.worldmodel->brush.leafs;
			return false;  // found it, so stop searching
		}
		else
		{
			return true;  // keep looking
		}
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	plane = node->plane;
	if ( plane->type <= PLANE_Z )
	{
		dot = s_VisibleFogVolumeSearchPoint[plane->type] - plane->dist;
	}
	else
	{
		dot = DotProduct (s_VisibleFogVolumeSearchPoint, plane->normal) - plane->dist;
	}

	// recurse down the children, closer side first.
	// We have to do this because we need to find if the surfaces at this node
	// exist in any visible leaves closer to the camera than the node is. If so,
	// their r_surfacevisframe is set to indicate that we need to render them
	// at this node.
	if (dot >= 0)
		side = 0;
	else
		side = 1;

	// Recurse down the side closer to the camera
	if( !RecursiveGetVisibleFogVolume (node->children[side]) )
	{
		return false;
	}

	// recurse down the side farther from the camera
	return RecursiveGetVisibleFogVolume (node->children[!side]);
}

void R_GetVisibleFogVolume( int *visibleFogVolume, int *visibleFogVolumeLeaf, bool *eyeInFogVolume, float *pDistanceToWater, const Vector& eyePoint )
{
	VPROF_BUDGET( "R_GetVisibleFogVolume", VPROF_BUDGETGROUP_WORLD_RENDERING );
	s_VisibleFogVolume = -1;
	s_VisibleFogVolumeSearchPoint = eyePoint;
	R_SetupAreaBits( -1 );

	int leafID = ComputeLeaf( eyePoint );
	if ( host_state.worldmodel->brush.leafs[leafID].contents & CONTENTS_TESTFOGVOLUME)
	{
		RecursiveGetVisibleFogVolume( host_state.worldmodel->brush.nodes );
	}

	*visibleFogVolume = s_VisibleFogVolume;
	*visibleFogVolumeLeaf = s_VisibleFogVolumeLeaf;
	if( host_state.worldmodel->brush.m_LeafMinDistToWater )
	{
		*pDistanceToWater = ( float )host_state.worldmodel->brush.m_LeafMinDistToWater[leafID];
	}
	else
	{
		*pDistanceToWater = 0.0f;
	}
	if( host_state.worldmodel->brush.leafs[leafID].leafWaterDataID != -1 )
	{
		*eyeInFogVolume = true;
	}
	else
	{
		*eyeInFogVolume = false;
	}
}

//-----------------------------------------------------------------------------
// Draws the list of surfaces build in the BuildWorldLists phase
//-----------------------------------------------------------------------------

// Uncomment this to allow code to draw wireframe over a particular surface for debugging
//#define DEBUG_SURF 1

#ifdef DEBUG_SURF
int g_DebugSurfID = -1;
#endif

void R_DrawWorldLists( unsigned long flags )
{
	VPROF("R_DrawWorldLists");
	Shader_WorldEnd( flags );
	g_EngineStats.EndDrawWorld();

#ifdef DEBUG_SURF
	{
		VPROF("R_DrawWorldLists (DEBUG_SURF)");
		if (g_pDebugSurf)
		{
			materialSystemInterface->Bind( g_materialWorldWireframe );
			Shader_DrawSurfaceDynamic( g_pDebugSurf );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void R_SceneBegin( void )
{
	ComputeDebugSettings();

	R_ClearSkyBox ();
}

void R_SceneEnd( void )
{
}



//-----------------------------------------------------------------------------
// Debugging code to draw the lightmap pages
//-----------------------------------------------------------------------------

void Shader_DrawLightmapPageSurface( int surfID, float red, float green, float blue )
{
	Vector2D lightCoords[32][4];

	int bumpID, count;
	if ( MSurf_Flags( surfID ) & SURFDRAW_BUMPLIGHT )
	{
		count = NUM_BUMP_VECTS + 1;
	}
	else
	{
		count = 1;
	}

	BuildMSurfaceVerts( host_state.worldmodel, surfID, NULL, NULL, lightCoords );

	int lightmapPageWidth, lightmapPageHeight;

	materialSystemInterface->Bind( g_materialWireframe );
	materialSystemInterface->GetLightmapPageSize( 
		SortInfoToLightmapPage(MSurf_MaterialSortID( surfID )), 
		&lightmapPageWidth, &lightmapPageHeight );

	for( bumpID = 0; bumpID < count; bumpID++ )
	{
		// assumes that we are already in ortho mode.
		IMesh* pMesh = materialSystemInterface->GetDynamicMesh( );
					
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_LINES, MSurf_VertCount( surfID ) );
		
		int i;

		for( i = 0; i < MSurf_VertCount( surfID ); i++ )
		{
			float x, y;
			float *texCoord;

			texCoord = &lightCoords[i][bumpID][0];

			x = lightmapPageWidth * texCoord[0];
			y = lightmapPageHeight * texCoord[1];
			
			meshBuilder.Position3f( x, y, 0.0f );
			meshBuilder.AdvanceVertex();

			texCoord = &lightCoords[(i+1)%MSurf_VertCount( surfID )][bumpID][0];

			x = lightmapPageWidth * texCoord[0];
			y = lightmapPageHeight * texCoord[1];
			
			meshBuilder.Position3f( x, y, 0.0f );
			meshBuilder.AdvanceVertex();
		}
		
		meshBuilder.End();
		pMesh->Draw();
	}
}

void Shader_DrawLightmapPageChains( void )
{
	int i;

	for (int j = 0; j < MAX_MAT_SORT_GROUPS; ++j)
	{
		i = g_pFirstMatSort[j];
		for ( i = g_pFirstMatSort[j]; i >= 0; i = g_pWorldStatic[i].m_pNext[j] )
		{
			int surfID = g_pWorldStatic[i].m_Surf[j].m_nHeadSurfID;
			if( !IS_SURF_VALID( surfID ) )
				continue;

			Assert( MSurf_MaterialSortID( surfID ) >= 0 && MSurf_MaterialSortID( surfID ) < g_NumMaterialSortBins );
			if( materialSortInfoArray[MSurf_MaterialSortID( surfID ) ].lightmapPageID != mat_showlightmappage.GetFloat() )
			{
				continue;
			}
			for( ; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
			{
				Assert( !SurfaceHasDispInfo( surfID ) );
				Shader_DrawLightmapPageSurface( surfID, 0.0f, 1.0f, 0.0f );
			}
		}

		// render displacement lightmap page info
		for( IDispInfo *pDispInfo = s_DispChain[j].GetHead(); pDispInfo; pDispInfo = pDispInfo->GetNextInRenderChain() )
		{
			pDispInfo->RenderWireframeInLightmapPage();
		}
	}
}

//-----------------------------------------------------------------------------
//
// All code related to brush model rendering
//
//-----------------------------------------------------------------------------

class CBrushSurface : public IBrushSurface
{
public:
	CBrushSurface( int surfID );

	// Computes texture coordinates + lightmap coordinates given a world position
	virtual void ComputeTextureCoordinate( const Vector& worldPos, Vector2D& texCoord );
	virtual void ComputeLightmapCoordinate( const Vector& worldPos, Vector2D& lightmapCoord );

	// Gets the vertex data for this surface
	virtual int  GetVertexCount() const;
	virtual void GetVertexData( BrushVertex_t* pVerts );

	// Gets at the material properties for this surface
	virtual IMaterial* GetMaterial();

private:
	int m_SurfaceID;
	SurfaceCtx_t m_Ctx;
};


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CBrushSurface::CBrushSurface( int surfID ) : m_SurfaceID(surfID)
{
	Assert(surfID >= 0);
	SurfSetupSurfaceContext( m_Ctx, surfID );
}


//-----------------------------------------------------------------------------
// Computes texture coordinates + lightmap coordinates given a world position
//-----------------------------------------------------------------------------
void CBrushSurface::ComputeTextureCoordinate( const Vector& worldPos, Vector2D& texCoord )
{
	SurfComputeTextureCoordinate( m_Ctx, m_SurfaceID, worldPos, texCoord );
}

void CBrushSurface::ComputeLightmapCoordinate( const Vector& worldPos, Vector2D& lightmapCoord )
{
	SurfComputeLightmapCoordinate( m_Ctx, m_SurfaceID, worldPos, lightmapCoord );
}


//-----------------------------------------------------------------------------
// Gets the vertex data for this surface
//-----------------------------------------------------------------------------
int  CBrushSurface::GetVertexCount() const
{
	if( !MSurf_NumPrims( m_SurfaceID ) )
	{
		// Create a temporary vertex array for the data...
		return MSurf_VertCount( m_SurfaceID );
	}
	else
	{
		// not implemented yet
		Assert(0);
		return 0;
	}
}

void CBrushSurface::GetVertexData( BrushVertex_t* pVerts )
{
	Assert( pVerts );

	if( !MSurf_NumPrims( m_SurfaceID ) )
	{
		// Fill in the vertex data
		BuildBrushModelVertexArray( host_state.worldmodel, m_SurfaceID, pVerts );
	}
	else
	{
		// not implemented yet
		Assert(0);
	}
}

//-----------------------------------------------------------------------------
// Gets at the material properties for this surface
//-----------------------------------------------------------------------------

IMaterial* CBrushSurface::GetMaterial()
{
	return MSurf_TexInfo( m_SurfaceID )->material;
}

//-----------------------------------------------------------------------------
// Installs a client-side renderer for brush models
//-----------------------------------------------------------------------------

void R_InstallBrushRenderOverride( IBrushRenderer* pBrushRenderer )
{
	s_pBrushRenderOverride = pBrushRenderer;
}

//-----------------------------------------------------------------------------
// Here, we allow the client DLL to render brush surfaces however they'd like
// NOTE: This involves a vertex copy, so don't use this everywhere
//-----------------------------------------------------------------------------

bool Shader_DrawBrushSurfaceOverride( int surfID, IClientEntity *baseentity )
{
	// Set the lightmap state
	Shader_SetChainLightmapState( surfID );

	CBrushSurface brushSurface( surfID );
	return s_pBrushRenderOverride->RenderBrushModelSurface( baseentity, &brushSurface );
}


//-----------------------------------------------------------------------------
// Main method to draw brush surfaces
//-----------------------------------------------------------------------------
void Shader_BrushSurface( int surfID, model_t *model, IClientEntity *baseentity )
{
	bool drawDecals;
	if (!s_pBrushRenderOverride)
	{
		drawDecals = true;

		MSurf_TexInfo( surfID )->material->AlphaModulate( r_blend );
		MSurf_TexInfo( surfID )->material->ColorModulate( r_colormod[0], r_colormod[1], r_colormod[2] );

		Shader_SetChainTextureState( surfID, baseentity );

//		NOTE: Since a static vb/dynamic ib IMesh doesn't buffer, we shouldn't use this
//		since it causes a lock and drawindexedprimitive per surface! (gary)
//		Shader_DrawSurfaceStatic( surfID );
		Shader_DrawSurfaceDynamic( surfID );
	}
	else
	{
		drawDecals = Shader_DrawBrushSurfaceOverride( surfID, baseentity );
	}

	int numPasses = MSurf_TexInfo( surfID )->material->GetNumPasses();
	g_EngineStats.IncrementCountedStat( ENGINE_STATS_BRUSH_TRIANGLE_COUNT, 
		 numPasses * ( MSurf_VertCount( surfID ) - 2 ) );

	g_EngineStats.IncrementCountedStat( ENGINE_STATS_LOWEND_BRUSH_TRIANGLE_COUNT, 
		 MSurf_VertCount( surfID ) - 2 );

	// fixme: need to get "allowDecals" from the material
	//	if ( g_BrushProperties.allowDecals && pSurf->pdecals ) 
	if( MSurf_Decals( surfID ) && drawDecals )
	{
		DecalSurfaceAdd( surfID, BRUSHMODEL_DECAL_SORT_GROUP );
	}

	// Add overlay fragments to list.
	// FIXME: A little code support is necessary to get overlays working on brush models
//	OverlayMgr()->AddFragmentListToRenderList( MSurf_OverlayFragmentList( surfID ) );

	// Add shadows too....
	ShadowDecalHandle_t decalHandle = MSurf_ShadowDecals( surfID );
	if (decalHandle != SHADOW_DECAL_HANDLE_INVALID)
	{
		g_pShadowMgr->AddShadowsOnSurfaceToRenderList( decalHandle );
	}
}


//-----------------------------------------------------------------------------
// Adds a brush surface to the list of surfaces to render
//-----------------------------------------------------------------------------
static void R_AddBrushSurfaceToRenderList( int surfID, int side = -1 )
{
	// already processed this frame?
	if ( MSurf_VisFrame( surfID ) == r_surfacevisframe)
		return;

	MSurf_VisFrame( surfID ) = r_surfacevisframe;

	if ( MSurf_Flags( surfID ) & SURFDRAW_NODRAW )
		return;

	// Don't cull if the client is rendering; who knows what it'll do
	if ((side >= 0) && ((MSurf_Flags( surfID ) & SURFDRAW_NOCULL) == 0) && (!s_pBrushRenderOverride) )
	{
		// Backface cull
		if ( ( side ^ !!(MSurf_Flags( surfID ) & SURFDRAW_PLANEBACK)) )
			return;
	}

	MSurf_TextureChain( surfID ) = r_brushlist;
	r_brushlist = surfID;
}


//-----------------------------------------------------------------------------
// Recurses through the brush model tree
//-----------------------------------------------------------------------------
static void R_RecursiveBrushNode( model_t *model, mnode_t *node )
{
	int			side;
	cplane_t	*plane;
	float		dot;

	// no polygons in solid nodes
	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	// if a leaf node, draw stuff
	if (node->contents >= 0)
	{
		mleaf_t *pleaf = (mleaf_t*)node;
		for ( int i = 0; i < pleaf->nummarksurfaces; i++ )
		{
			int surfID = host_state.worldmodel->brush.marksurfaces[pleaf->firstmarksurface + i];
			R_AddBrushSurfaceToRenderList( surfID );
		}
		return;
	}

	// node is just a decision point, so go down the apropriate sides
	// find which side of the node we are on
	plane = node->plane;
	if ( plane->type <= PLANE_Z )
	{
		dot = modelorg[plane->type] - plane->dist;
	}
	else
	{
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

	// recurse down the children, side closer from camera first (for translucency)
	// we do the closer child first because	that'll produce a back to front ordering
	// (since we always add new surfaces to render to the front of the list)
	R_RecursiveBrushNode(model, node->children[side]);

	// draw surfaces on node
	int surfID = node->firstsurface;
	for ( int i = 0; i < node->numsurfaces; i++, surfID++ )
	{
		R_AddBrushSurfaceToRenderList( surfID, side );
	}

	// recurse down the side closer to the camera last
	R_RecursiveBrushNode(model, node->children[!side]);
}


//-----------------------------------------------------------------------------
// Draws an translucent (sorted) brush model
//-----------------------------------------------------------------------------
static void R_DrawSortedBrushModel( IClientEntity *baseentity, model_t *model, 
	const Vector& origin )
{
	r_brushlist = -1;

	// Generate a list of all surfaces to render on this brush model
	R_RecursiveBrushNode( model, model->brush.nodes + model->brush.firstnode );

	// Update the dynamic lightmaps for the brush model
	int surfID;
	for ( surfID = r_brushlist; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
	{
		R_RenderDynamicLightmaps( surfID, origin );
	}
	materialSystemInterface->FlushLightmaps();

	bool bCopiedFrameBuffer = false;
	
	// Render all the surfaces we queued up to render
	for ( surfID = r_brushlist; surfID >= 0; surfID = MSurf_TextureChain( surfID ) )
	{
		if( !bCopiedFrameBuffer && MSurf_TexInfo( surfID )->material->NeedsFrameBufferTexture() )
		{
			materialSystemInterface->CopyRenderTargetToTexture( materialSystemInterface->GetFrameBufferCopyTexture() );
		}
		Shader_BrushSurface( surfID, model, baseentity );

		// After each surface, draw decals + shadows
		// This is important for translucency to work correctly.
		DecalSurfaceDraw(BRUSHMODEL_DECAL_SORT_GROUP);
		s_DecalSurfaces[BRUSHMODEL_DECAL_SORT_GROUP].RemoveAll();

		// Draw all shadows on the brush
		g_pShadowMgr->RenderShadows( );
	}

	// Draw debugging information for this surface chain
	DrawDebugInformation( r_brushlist );
}


//-----------------------------------------------------------------------------
// Draws an opaque brush model
//-----------------------------------------------------------------------------
static void R_DrawOpaqueBrushModel( IClientEntity *baseentity, model_t *model, 
	const Vector& origin )
{
	VPROF( "R_DrawOpaqueBrushModel" );
	int drawnSurfID = -1;
	int surfID = model->brush.firstmodelsurface;
	for (int i=0 ; i<model->brush.nummodelsurfaces ; i++, surfID++)
	{
		if( MSurf_Flags( surfID ) & SURFDRAW_NODRAW )
			continue;
	
		// draw the polygon
		float dot = DotProduct (modelorg, MSurf_Plane( surfID ).normal) - MSurf_Plane( surfID ).dist;
		
		// backfacing surface?
		if (( (MSurf_Flags( surfID )& SURFDRAW_NOCULL) == 0) && (!s_pBrushRenderOverride) )
		{
			if ( dot < -BACKFACE_EPSILON )
				continue;
		}

		// optimize!
		R_RenderDynamicLightmaps( surfID, origin );
		materialSystemInterface->FlushLightmaps();
		Shader_BrushSurface( surfID, model, baseentity );

		// link this to the list of drawn surfaces
		MSurf_TextureChain( surfID ) = drawnSurfID;
		drawnSurfID = surfID;
	}

	// now draw debug for each drawn surface
	DrawDebugInformation( drawnSurfID );
}


//-----------------------------------------------------------------------------
// Purpose: Helper routine to set up transformations
// Input  : *origin - 
//			*angles - 
// Output : void R_RotateForEntity
//-----------------------------------------------------------------------------

static void R_RotateForEntity( const Vector &origin, const QAngle &angles  )
{
    materialSystemInterface->Translate( origin[0],  origin[1],  origin[2] );

	materialSystemInterface->Rotate( angles[1],  0, 0, 1 );
    materialSystemInterface->Rotate( angles[0],  0, 1, 0 );
    materialSystemInterface->Rotate( angles[2],  1, 0, 0 );
}


//-----------------------------------------------------------------------------
// Stuff to do right before and after brush model rendering
//-----------------------------------------------------------------------------
void Shader_BrushBegin( model_t *model, IClientEntity *baseentity /*=NULL*/ )
{
	// Clear out the render list of decals
	s_DecalSurfaces[BRUSHMODEL_DECAL_SORT_GROUP].RemoveAll();

	// Clear out the render lists of shadows
	g_pShadowMgr->ClearShadowRenderList( );
}

void Shader_BrushEnd( VMatrix const* pBrushToWorld, model_t *model, IClientEntity *baseentity /* = NULL */ )
{
	DecalSurfaceDraw(BRUSHMODEL_DECAL_SORT_GROUP);

	// Draw all shadows on the brush
	g_pShadowMgr->RenderShadows( pBrushToWorld );
}


//-----------------------------------------------------------------------------
// Sets up modelorg + the model transformation for a brush model,
// return true if the transform is identity, false if not
//-----------------------------------------------------------------------------
static bool R_SetupBrushModelTransform( const Vector& origin, QAngle const& angles, VMatrix* pBrushToWorld )
{
	bool rotated = ( angles[0] || angles[1] || angles[2] );
	VectorSubtract( g_EngineRenderer->ViewOrigin(), origin, modelorg );
	if (rotated)
	{
		Vector	temp;
		Vector	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (angles, &forward, &right, &up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	bool isIdentity = (origin == vec3_origin) && (!rotated);

	// Don't change state if we don't need to
	if (!isIdentity)
	{
		MatrixFromAngles( angles, *pBrushToWorld );
		pBrushToWorld->SetTranslation( origin );

		materialSystemInterface->MatrixMode( MATERIAL_MODEL );
		materialSystemInterface->PushMatrix();

		// FIXME: Use load matrix instead of R_RotateForEntity.. should work!
//		materialSystemInterface->LoadMatrix( *pBrushToWorld );

		R_RotateForEntity( origin, angles );
	}

	return isIdentity;
}

static void R_CleanupBrushModelTransform( bool isIdentity )
{
	if (!isIdentity)
	{
		materialSystemInterface->MatrixMode( MATERIAL_MODEL );
		materialSystemInterface->PopMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws a brush model using the global shader/surfaceVisitor
// Input  : *e - entity to draw
// Output : void R_DrawBrushModel
//-----------------------------------------------------------------------------
void R_DrawBrushModel( IClientEntity *baseentity, model_t *model, 
						const Vector& origin, const QAngle& angles, bool sort )
{
	VPROF( "R_DrawBrushModel" );
	int			k;
	Vector		mins, maxs;
	int			i;

#ifdef USE_CONVARS
	if( !r_drawbrushmodels.GetInt() )
	{
		return;
	}
#endif

	MEASURE_TIMED_STAT( ENGINE_STATS_BRUSH_TRIANGLE_TIME ); 

	// Store off the model org so we can restore it
	Vector oldModelOrg = modelorg;

	bool rotated = ( angles[0] || angles[1] || angles[2] );
	if ( rotated )
	{
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = origin[i] - model->radius;
			maxs[i] = origin[i] + model->radius;
		}
	}
	else
	{
		VectorAdd (origin, model->mins, mins);
		VectorAdd (origin, model->maxs, maxs);
	}

	if (R_CullBox (mins, maxs))
		return;

	VMatrix brushToWorld;
	bool isIdentity = R_SetupBrushModelTransform( origin, angles, &brushToWorld );

	// calculate dynamic lighting for bmodel if it's not an instanced model
	if (model->brush.firstmodelsurface != 0)
	{
		Vector saveOrigin;

		for (k=0 ; k<MAX_DLIGHTS ; k++)
		{
			if ((cl_dlights[k].die < cl.gettime()) ||
				(!cl_dlights[k].radius))
				continue;

			VectorCopy( cl_dlights[k].origin, saveOrigin );

			// If this is needed, it will have to be computed elsewhere as 
			// baseline.origin will change after a save/restore
			VectorSubtract(cl_dlights[k].origin, origin, cl_dlights[k].origin );

			R_MarkLights( &cl_dlights[k], 1<<k,	model->brush.nodes + model->brush.firstnode );
			VectorCopy( saveOrigin, cl_dlights[k].origin );
		}
	}

	// Draw the puppy...
	Shader_BrushBegin( model, baseentity );

	if ( sort )
	{
		R_DrawSortedBrushModel( baseentity, model, origin );
	}
	else
	{
		R_DrawOpaqueBrushModel( baseentity, model, origin );
	}

	Shader_BrushEnd( isIdentity ? 0 : &brushToWorld, model, baseentity );

	R_CleanupBrushModelTransform( isIdentity );

	// Restore the model origin
	modelorg = oldModelOrg;
}


//-----------------------------------------------------------------------------
// Purpose: Draws a brush model shadow for render-to-texture shadows
//-----------------------------------------------------------------------------
void R_DrawBrushModelShadow( IClientRenderable *pRenderable )
{
#ifdef USE_CONVARS
	if( !r_drawbrushmodels.GetInt() )
	{
		return;
	}
#endif

	MEASURE_TIMED_STAT( ENGINE_STATS_SHADOW_TRIANGLE_TIME ); 

	model_t const* model = pRenderable->GetModel();
	const Vector& origin = pRenderable->GetRenderOrigin();
	QAngle const& angles = pRenderable->GetRenderAngles();

	// Store off the model org so we can restore it
	Vector oldModelOrg = modelorg;

	VMatrix brushToWorld;
	bool isIdentity = R_SetupBrushModelTransform( origin, angles, &brushToWorld );

	materialSystemInterface->Bind( g_pMaterialShadowBuild, pRenderable );

	// Draws all surfaces in the brush model in arbitrary order
	int surfID = model->brush.firstmodelsurface;
	for ( int i=0 ; i<model->brush.nummodelsurfaces ; i++, surfID++)
	{
		if( MSurf_Flags( surfID ) & SURFDRAW_NODRAW )
			continue;
	
		// draw the polygon
//		float dot = DotProduct (modelorg, MSurf_Plane( surfID ).normal) - MSurf_Plane( surfID ).dist;
		
		// backfacing surface?
//		if ( (MSurf_Flags( surfID ) & SURFDRAW_NOCULL) == 0 )
//		{
//			if ( dot < -BACKFACE_EPSILON )
//				continue;
//		}

//		NOTE: Since a static vb/dynamic ib IMesh doesn't buffer, we shouldn't use this
//		since it causes a lock and drawindexedprimitive per surface! (gary)
//		Shader_DrawSurfaceStatic( surfID );
		Shader_DrawSurfaceDynamic( surfID );

		g_EngineStats.IncrementCountedStat( ENGINE_STATS_ACTUAL_SHADOW_RENDER_TO_TEXTURE_TRIANGLES, 
			MSurf_VertCount( surfID ) - 2 );
	}

	R_CleanupBrushModelTransform( isIdentity );

	// Restore the model origin
	modelorg = oldModelOrg;
}


void Shader_DecalDrawPoly( CDecalVert *v, IMaterial *pMaterial, int surfID, int vertCount, decal_t *pdecal )
{
	int vertexFormat = 0;
	int numPasses = pMaterial->GetNumPasses();
#ifdef USE_CONVARS
	if( mat_wireframe.GetInt() )
	{
		materialSystemInterface->Bind( g_materialDecalWireframe );
	}
	else
#endif
	{
		Assert( MSurf_MaterialSortID( surfID ) >= 0 && 
			    MSurf_MaterialSortID( surfID )  < g_NumMaterialSortBins );
		materialSystemInterface->BindLightmapPage( materialSortInfoArray[MSurf_MaterialSortID( surfID )].lightmapPageID );
		materialSystemInterface->Bind( pMaterial, pdecal->userdata );
		vertexFormat = pMaterial->GetVertexFormat();
	}

	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_POLYGON, vertCount );

	byte color[4] = {pdecal->color.r,pdecal->color.g,pdecal->color.b,pdecal->color.a};

	// Deal with fading out... (should this be done in the shader?)
	// Note that we do it with per-vertex color even though the translucency
	// is constant so as to not change any rendering state (like the constant
	// alpha value)
	if (pdecal->flags & FDECAL_DYNAMIC)
	{
		float fadeval;
		// Negative fadeDuration value means to fade in
		if (pdecal->fadeDuration < 0)
		{
			fadeval = - (cl.gettime() - pdecal->fadeStartTime) / pdecal->fadeDuration;
		}
		else
		{
			fadeval = 1.0 - (cl.gettime() - pdecal->fadeStartTime) / pdecal->fadeDuration;
		}

		if (fadeval < 0.0f)
			fadeval = 0.0;
		else if (fadeval > 1.0f)
			fadeval = 1.0f;

		color[3] = (byte) (255 * fadeval);
	}


	Vector normal(0,0,1), tangent(1,0,0), binormal(0,1,0);

	if ( vertexFormat & (VERTEX_NORMAL|VERTEX_TANGENT_SPACE) )
	{
		normal = MSurf_Plane( surfID ).normal;
		if ( vertexFormat & VERTEX_TANGENT_SPACE )
		{
			Vector tVect;
			bool negate = TangentSpaceSurfaceSetup( surfID, tVect );
			TangentSpaceComputeBasis( tangent, binormal, normal, tVect, negate );
		}
	}

	for( int i = 0; i < vertCount; i++, v++ )
	{
		meshBuilder.Position3f( VectorExpand( v->m_vPos ) );
		meshBuilder.TexCoord2f( 0, Vector2DExpand( v->m_tCoords ) );
		meshBuilder.TexCoord2f( 1, Vector2DExpand( v->m_LMCoords ) );
		meshBuilder.Color4ubv( color );
		if ( vertexFormat & VERTEX_NORMAL )
		{
			meshBuilder.Normal3fv( normal.Base() );
		}
		if ( vertexFormat & VERTEX_TANGENT_SPACE )
		{
			meshBuilder.TangentS3fv( tangent.Base() ); 
			meshBuilder.TangentT3fv( binormal.Base() ); 
		}
		meshBuilder.AdvanceVertex();
	}

	g_EngineStats.IncrementCountedStat( ENGINE_STATS_DECAL_TRIANGLES, numPasses * ( vertCount - 2 ) );

	meshBuilder.End();
	pMesh->Draw();
}

void R_DrawIdentityBrushModel( model_t *model )
{
	if ( !model )
		return;

	int surfID = model->brush.firstmodelsurface;
	// calculate dynamic lighting for bmodel if it's not an
	// instanced model
	if ( IS_SURF_VALID( surfID ) )
	{
		for (int k=0 ; k<MAX_DLIGHTS ; k++)
		{
			if ((cl_dlights[k].die < cl.gettime()) ||
				(!cl_dlights[k].radius))
				continue;
			R_MarkLights( &cl_dlights[k], 1<<k,	model->brush.nodes + model->brush.firstnode );
		}
	}
	
	for (int j=0 ; j<model->brush.nummodelsurfaces ; ++j, surfID++)
	{
		if ( MSurf_Flags( surfID ) & SURFDRAW_NODRAW )
			continue;

		// FIXME: Can't insert translucent stuff into the list
		// of translucent surfaces because we don't know what leaf
		// we're in. At the moment, the client doesn't add translucent
		// brushes to the identity brush model list
//		Assert ( (psurf->flags & SURFDRAW_TRANS ) == 0 );

		// OPTIMIZE: Backface cull these guys?!?!?
		if ( MSurf_Flags( surfID ) & SURFDRAW_TRANS)
//		if ( psurf->texinfo->material->IsTranslucent() )
		{
			Shader_TranslucentWorldSurface( surfID );
		}
		else
		{
			Shader_WorldSurface( surfID );
		}
	}
}
#endif
//-----------------------------------------------------------------------------
// Converts leaf pointer to index
//-----------------------------------------------------------------------------
inline int LeafToIndex( mleaf_t* pLeaf )
{
	return pLeaf - host_state.worldmodel->brush.leafs;
}


//-----------------------------------------------------------------------------
// Finds the leaf a particular point is in
//-----------------------------------------------------------------------------
static int ComputeLeaf(mnode_t *node, const Vector& pt )
{
	while (node->contents < 0)
	{
		// Does the node plane split the box?
		// find which side of the node we are on
		cplane_t* plane = node->plane;
		if ( plane->type <= PLANE_Z )
		{
			if (pt[plane->type] <= plane->dist)
				node = node->children[1];
			else
				node = node->children[0];
		}
		else
		{
			if (DotProduct( plane->normal, pt ) <= plane->dist)
				node = node->children[1];
			else
				node = node->children[0];
		}
	}
	return LeafToIndex( (mleaf_t *)node );
}

int ComputeLeaf( const Vector& pt )
{
	return ComputeLeaf( host_state.worldmodel->brush.nodes, pt );
}



//-----------------------------------------------------------------------------
// Structures to help out with enumeration
//-----------------------------------------------------------------------------
enum
{
	ENUM_SPHERE_TEST_X = 0x1,
	ENUM_SPHERE_TEST_Y = 0x2,
	ENUM_SPHERE_TEST_Z = 0x4,

	ENUM_SPHERE_TEST_ALL = 0x7
};

struct EnumLeafBoxInfo_t
{
	Vector m_vecBoxMax;
	Vector m_vecBoxMin;
	Vector m_vecBoxCenter;
	Vector m_vecBoxHalfDiagonal;
	ISpatialLeafEnumerator *m_pIterator;
	int	m_nContext;
};

struct EnumLeafSphereInfo_t
{
	Vector m_vecCenter;
	float m_flRadius;
	Vector m_vecBoxCenter;
	Vector m_vecBoxHalfDiagonal;
	ISpatialLeafEnumerator *m_pIterator;
	int	m_nContext;
};

//-----------------------------------------------------------------------------
// Finds all leaves of the BSP tree within a particular volume
//-----------------------------------------------------------------------------
static bool EnumerateLeafInBox_R(mnode_t *node, EnumLeafBoxInfo_t& info )
{
	// no polygons in solid nodes (don't report these leaves either)
	if (node->contents == CONTENTS_SOLID)
		return true;		// solid

	// rough cull...
	if (!IsBoxIntersectingBoxExtents(node->m_vecCenter, node->m_vecHalfDiagonal, 
		info.m_vecBoxCenter, info.m_vecBoxHalfDiagonal))
	{
		return true;
	}
	
	if (node->contents >= 0)
	{
		// if a leaf node, report it to the iterator...
		return info.m_pIterator->EnumerateLeaf( LeafToIndex( (mleaf_t *)node ), info.m_nContext ); 
	}

	// Does the node plane split the box?
	// find which side of the node we are on
	cplane_t* plane = node->plane;
	if ( plane->type <= PLANE_Z )
	{
		if (info.m_vecBoxMax[plane->type] <= plane->dist)
		{
			return EnumerateLeafInBox_R( node->children[1], info );
		}
		else if (info.m_vecBoxMin[plane->type] >= plane->dist)
		{
			return EnumerateLeafInBox_R( node->children[0], info );
		}
		else
		{
			// Here the box is split by the node
			bool ret = EnumerateLeafInBox_R( node->children[0], info );
			if (!ret)
				return false;

			return EnumerateLeafInBox_R( node->children[1], info );
		}
	}

	// Arbitrary split plane here
	Vector cornermin, cornermax;
	for (int i = 0; i < 3; ++i)
	{
		if (plane->normal[i] >= 0)
		{
			cornermin[i] = info.m_vecBoxMin[i];
			cornermax[i] = info.m_vecBoxMax[i];
		}
		else
		{
			cornermin[i] = info.m_vecBoxMax[i];
			cornermax[i] = info.m_vecBoxMin[i];
		}
	}

	if (DotProduct( plane->normal, cornermax ) <= plane->dist)
	{
		return EnumerateLeafInBox_R( node->children[1], info );
	}
	else if (DotProduct( plane->normal, cornermin ) >= plane->dist)
	{
		return EnumerateLeafInBox_R( node->children[0], info );
	}
	else
	{
		// Here the box is split by the node
		bool ret = EnumerateLeafInBox_R( node->children[0], info );
		if (!ret)
			return false;

		return EnumerateLeafInBox_R( node->children[1], info );
	}
}


//-----------------------------------------------------------------------------
// Returns all leaves that lie within a spherical volume
//-----------------------------------------------------------------------------
bool EnumerateLeafInSphere_R( mnode_t *node, EnumLeafSphereInfo_t& info, int nTestFlags )
{
	while (true)
	{
		// no polygons in solid nodes (don't report these leaves either)
		if (node->contents == CONTENTS_SOLID)
			return true;		// solid

		if (node->contents >= 0)
		{
			// leaf cull...
			// NOTE: using nTestFlags here means that we may be passing in some 
			// leaves that don't actually intersect the sphere, but instead intersect
			// the box that surrounds the sphere.
			if (nTestFlags)
			{
				if (!IsBoxIntersectingSphereExtents (node->m_vecCenter, node->m_vecHalfDiagonal, info.m_vecCenter, info.m_flRadius))
					return true;
			}

			// if a leaf node, report it to the iterator...
			return info.m_pIterator->EnumerateLeaf( LeafToIndex( (mleaf_t *)node ), info.m_nContext ); 
		}
		else if (nTestFlags)
		{
			if (node->contents == -1)
			{
				// faster cull...
				if (nTestFlags & ENUM_SPHERE_TEST_X)
				{
					float flDelta = FloatMakePositive( node->m_vecCenter.x - info.m_vecBoxCenter.x );
					float flSize = node->m_vecHalfDiagonal.x + info.m_vecBoxHalfDiagonal.x;
					if ( flDelta > flSize )
						return true;

					// This checks for the node being completely inside the box...
					if ( flDelta + node->m_vecHalfDiagonal.x < info.m_vecBoxHalfDiagonal.x )
						nTestFlags &= ~ENUM_SPHERE_TEST_X;
				}

				if (nTestFlags & ENUM_SPHERE_TEST_Y)
				{
					float flDelta = FloatMakePositive( node->m_vecCenter.y - info.m_vecBoxCenter.y );
					float flSize = node->m_vecHalfDiagonal.y + info.m_vecBoxHalfDiagonal.y;
					if ( flDelta > flSize )
						return true;

					// This checks for the node being completely inside the box...
					if ( flDelta + node->m_vecHalfDiagonal.y < info.m_vecBoxHalfDiagonal.y )
						nTestFlags &= ~ENUM_SPHERE_TEST_Y;
				}

				if (nTestFlags & ENUM_SPHERE_TEST_Z)
				{
					float flDelta = FloatMakePositive( node->m_vecCenter.z - info.m_vecBoxCenter.z );
					float flSize = node->m_vecHalfDiagonal.z + info.m_vecBoxHalfDiagonal.z;
					if ( flDelta > flSize )
						return true;
														   
					if ( flDelta + node->m_vecHalfDiagonal.z < info.m_vecBoxHalfDiagonal.z )
						nTestFlags &= ~ENUM_SPHERE_TEST_Z;
				}
			}
			else if (node->contents == -2)
			{
				// If the box is too small to bother with testing, then blat out the flags
				nTestFlags = 0;
			}
		}

		// Does the node plane split the sphere?
		// find which side of the node we are on
		float flNormalDotCenter;
		cplane_t* plane = node->plane;
		if ( plane->type <= PLANE_Z )
		{
			flNormalDotCenter = info.m_vecCenter[plane->type];
		}
		else
		{
			// Here, we've got a plane which is not axis aligned, so we gotta do more work
			flNormalDotCenter = DotProduct( plane->normal, info.m_vecCenter );
		}

		if (flNormalDotCenter + info.m_flRadius <= plane->dist)
		{
			node = node->children[1];
		}
		else if (flNormalDotCenter - info.m_flRadius >= plane->dist)
		{
			node = node->children[0];
		}
		else
		{
			// Here the box is split by the node
			if (!EnumerateLeafInSphere_R( node->children[0], info, nTestFlags ))
				return false;

			node = node->children[1];
		}
	}
}


//-----------------------------------------------------------------------------
// Enumerate leaves along a non-extruded ray
//-----------------------------------------------------------------------------

static bool EnumerateLeavesAlongRay_R( mnode_t *node, Ray_t const& ray, 
	float start, float end, ISpatialLeafEnumerator* pEnum, int context )
{
	// no polygons in solid nodes (don't report these leaves either)
	if (node->contents == CONTENTS_SOLID)
		return true;		// solid, keep recursing

	// didn't hit anything
	if (node->contents >= 0)
	{
		// if a leaf node, report it to the iterator...
		return pEnum->EnumerateLeaf( LeafToIndex( (mleaf_t *)node ), context ); 
	}
	
	// Determine which side of the node plane our points are on
	cplane_t* plane = node->plane;

	float startDotN,deltaDotN;
	if (plane->type <= PLANE_Z)
	{
		startDotN = ray.m_Start[plane->type];
		deltaDotN = ray.m_Delta[plane->type];
	}
	else
	{
		startDotN = DotProduct( ray.m_Start, plane->normal );
		deltaDotN = DotProduct( ray.m_Delta, plane->normal );
	}

	float front = startDotN + start * deltaDotN - plane->dist;
	float back = startDotN + end * deltaDotN - plane->dist;

	int side = front < 0;
	
	// If they're both on the same side of the plane, don't bother to split
	// just check the appropriate child
	if ( (back < 0) == side )
	{
		return EnumerateLeavesAlongRay_R (node->children[side], ray, start, end, pEnum, context );
	}
	
	// calculate mid point
	float frac = front / (front - back);
	float mid = start * (1.0f - frac) + end * frac;
	
	// go down front side	
	bool ok = EnumerateLeavesAlongRay_R (node->children[side], ray, start, mid, pEnum, context );
	if (!ok)
		return ok;

	// go down back side
	return EnumerateLeavesAlongRay_R (node->children[!side], ray, mid, end, pEnum, context );
}


//-----------------------------------------------------------------------------
// Enumerate leaves along a non-extruded ray
//-----------------------------------------------------------------------------

static bool EnumerateLeavesAlongExtrudedRay_R( mnode_t *node, Ray_t const& ray, 
	float start, float end, ISpatialLeafEnumerator* pEnum, int context )
{
	// no polygons in solid nodes (don't report these leaves either)
	if (node->contents == CONTENTS_SOLID)
		return true;		// solid, keep recursing

	// didn't hit anything
	if (node->contents >= 0)
	{
		// if a leaf node, report it to the iterator...
		return pEnum->EnumerateLeaf( LeafToIndex( (mleaf_t *)node ), context ); 
	}
	
	// Determine which side of the node plane our points are on
	cplane_t* plane = node->plane;

	//
	float t1, t2, offset;
	float startDotN,deltaDotN;
	if (plane->type <= PLANE_Z)
	{
		startDotN = ray.m_Start[plane->type];
		deltaDotN = ray.m_Delta[plane->type];
		offset = ray.m_Extents[plane->type] + DIST_EPSILON;
	}
	else
	{
		startDotN = DotProduct( ray.m_Start, plane->normal );
		deltaDotN = DotProduct( ray.m_Delta, plane->normal );
		offset = fabs(ray.m_Extents[0]*plane->normal[0]) +
				fabs(ray.m_Extents[1]*plane->normal[1]) +
				fabs(ray.m_Extents[2]*plane->normal[2]) + DIST_EPSILON;
	}
	t1 = startDotN + start * deltaDotN - plane->dist;
	t2 = startDotN + end * deltaDotN - plane->dist;

	// If they're both on the same side of the plane (further than the trace
	// extents), don't bother to split, just check the appropriate child
    if (t1 > offset && t2 > offset )
//	if (t1 >= offset && t2 >= offset)
	{
		return EnumerateLeavesAlongExtrudedRay_R( node->children[0], ray,
			start, end, pEnum, context );
	}
	if (t1 < -offset && t2 < -offset)
	{
		return EnumerateLeavesAlongExtrudedRay_R( node->children[1], ray,
			start, end, pEnum, context );
	}

	// For the segment of the line that we are going to use
	// to test against the back side of the plane, we're going
	// to use the part that goes from start to plane + extent
	// (which causes it to extend somewhat into the front halfspace,
	// since plane + extent is in the front halfspace).
	// Similarly, front the segment which tests against the front side,
	// we use the entire front side part of the ray + a portion of the ray that
	// extends by -extents into the back side.

	if (fabs(t1-t2) < DIST_EPSILON)
	{
		// Parallel case, send entire ray to both children...
		bool ret = EnumerateLeavesAlongExtrudedRay_R( node->children[0], 
			ray, start, end, pEnum, context );
		if (!ret)
			return false;
		return EnumerateLeavesAlongExtrudedRay_R( node->children[1],
			ray, start, end, pEnum, context );
	}
	
	// Compute the two fractions...
	// We need one at plane + extent and another at plane - extent.
	// put the crosspoint DIST_EPSILON pixels on the near side
	float idist, frac2, frac;
	int side;
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset) * idist;
		frac = (t1 - offset) * idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset) * idist;
		frac = (t1 + offset) * idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	frac = clamp( frac, 0, 1 );
	float midf = start + (end - start)*frac;
	bool ret = EnumerateLeavesAlongExtrudedRay_R( node->children[side], ray, start, midf, pEnum, context );
	if (!ret)
		return ret;

	// go past the node
	frac2 = clamp( frac2, 0, 1 );
	midf = start + (end - start)*frac2;
	return EnumerateLeavesAlongExtrudedRay_R( node->children[!side], ray, midf, end, pEnum, context );
}


//-----------------------------------------------------------------------------
//
// Helper class to iterate over leaves
//
//-----------------------------------------------------------------------------

class CEngineBSPTree : public IEngineSpatialQuery
{
public:
	// Returns the number of leaves
	int LeafCount() const;

	// Enumerates the leaves along a ray, box, etc.
	bool EnumerateLeavesAtPoint( const Vector& pt, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesInBox( const Vector& mins, const Vector& maxs, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesInSphere( const Vector& center, float radius, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, int context );
};

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------

static CEngineBSPTree s_ToolBSPTree;
IEngineSpatialQuery* g_pToolBSPTree = &s_ToolBSPTree;


//-----------------------------------------------------------------------------
// Returns the number of leaves
//-----------------------------------------------------------------------------

int CEngineBSPTree::LeafCount() const
{
	return host_state.worldmodel->brush.numleafs;
}

//-----------------------------------------------------------------------------
// Enumerates the leaves at a point
//-----------------------------------------------------------------------------

bool CEngineBSPTree::EnumerateLeavesAtPoint( const Vector& pt, 
									ISpatialLeafEnumerator* pEnum, int context )
{
	int leaf = ComputeLeaf( pt );
	return pEnum->EnumerateLeaf( leaf, context );
}


bool CEngineBSPTree::EnumerateLeavesInBox( const Vector& mins, const Vector& maxs, 
									ISpatialLeafEnumerator* pEnum, int context )
{
	if ( !host_state.worldmodel )
		return false;

	EnumLeafBoxInfo_t info;
	VectorAdd( mins, maxs, info.m_vecBoxCenter );
	info.m_vecBoxCenter *= 0.5f;
	VectorSubtract( maxs, info.m_vecBoxCenter, info.m_vecBoxHalfDiagonal );
	info.m_pIterator = pEnum;
	info.m_nContext = context;
	info.m_vecBoxMax = maxs;
	info.m_vecBoxMin = mins;

	return EnumerateLeafInBox_R( host_state.worldmodel->brush.nodes, info );
}


bool CEngineBSPTree::EnumerateLeavesInSphere( const Vector& center, float radius, 
									ISpatialLeafEnumerator* pEnum, int context )
{
	EnumLeafSphereInfo_t info;
	info.m_vecCenter = center;
	info.m_flRadius = radius;
	info.m_pIterator = pEnum;
	info.m_nContext = context;
	info.m_vecBoxCenter = center;
	info.m_vecBoxHalfDiagonal.Init( radius, radius, radius );

	return EnumerateLeafInSphere_R( host_state.worldmodel->brush.nodes, info, ENUM_SPHERE_TEST_ALL );
}


bool CEngineBSPTree::EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, int context )
{
	if (!ray.m_IsSwept)
	{
		Vector mins, maxs;
		VectorAdd( ray.m_Start, ray.m_Extents, maxs );
		VectorSubtract( ray.m_Start, ray.m_Extents, mins );

		return EnumerateLeavesInBox( mins, maxs, pEnum, context );
	}

	Vector end;
	VectorAdd( ray.m_Start, ray.m_Delta, end );

	if ( ray.m_IsRay )
	{
		return EnumerateLeavesAlongRay_R( host_state.worldmodel->brush.nodes, ray, 0.0f, 1.0f, pEnum, context );
	}
	else
	{
		return EnumerateLeavesAlongExtrudedRay_R( host_state.worldmodel->brush.nodes, ray, 0.0f, 1.0f, pEnum, context );
	}
}

//-----------------------------------------------------------------------------
// Gets the decal material and radius based on the decal index
//-----------------------------------------------------------------------------

void R_DecalGetMaterialAndSize( int decalIndex, IMaterial*& pDecalMaterial, float& w, float& h )
{
	pDecalMaterial = Draw_DecalMaterial( decalIndex );
	if (!pDecalMaterial)
		return;

	float scale = 1.0f;

	// Compute scale of surface
	// FIXME: cache this?
	bool found;
	IMaterialVar* pDecalScaleVar = pDecalMaterial->FindVar( "$decalScale", &found, false );
	if( found )
	{
		scale = pDecalScaleVar->GetFloatValue();
	}

	// compute the decal dimensions in world space
	w = pDecalMaterial->GetMappingWidth() * scale;
	h = pDecalMaterial->GetMappingHeight() * scale;
}



#ifndef SWDS
// UNDONE: !!! Obviously, this belongs in its own module

//-----------------------------------------------------------------------------
//
// Decal system
//
//-----------------------------------------------------------------------------

// This structure contains the information used to create new decals
struct decalinfo_t
{
	Vector		m_Position;			// world coordinates of the decal center
	Vector		m_SAxis;			// the s axis for the decal in world coordinates
	model_t*	m_pModel;			// the model the decal is going to be applied in
	IMaterial*	m_pMaterial;		// The decal material
	int			m_Size;				// Size of the decal (in world coords)
	int			m_Flags;
	int			m_Entity;			// Entity the decal is applied to.
	float		m_scale;
	int			m_decalWidth;
	int			m_decalHeight;
	color32		m_Color;
	Vector		m_Basis[3];
	void		*m_pUserData;
};

// UNDONE: Compress this???  256K here?
static decal_t			gDecalPool[ MAX_DECALS ];
static int				gDecalCount;					// Pool index

static void R_DecalUnlink( decal_t *pdecal, model_t *model );
static void R_DecalCreate( decalinfo_t* pDecalInfo, int surfID, float x, float y, bool bForceForDisplacement );
void R_DecalShoot( int textureIndex, int entity, const model_t *model, const Vector &position, const float *saxis, int flags, const color32 &rgbaColor );

#define DECAL_DISTANCE			4

// Empirically determined constants for minimizing overalpping decals
#define MAX_OVERLAP_DECALS		4
#define DECAL_OVERLAP_DIST		8

typedef struct
{
	int			decalIndex;
	CDecalVert	decalVert[4];
} decalcache_t;

#define DECAL_CACHEENTRY	256		// MUST BE POWER OF 2 or code below needs to change!
decalcache_t	gDecalCache[DECAL_CACHEENTRY];


// Init the decal pool
void R_DecalInit( void )
{
	int i;

	// Traverse all surfaces of map and throw away current decals
	//
	// sort the surfaces into the sort arrays
	if ( host_state.worldmodel )
	{
		for( i = 0; i < host_state.worldmodel->brush.numsurfaces; i++ )
		{
			MSurf_Decals( i ) = NULL;
		}
	}

	memset( gDecalPool, 0, sizeof( decal_t ) * MAX_DECALS );
	for( i = 0; i < MAX_DECALS; i++ )
	{
		gDecalPool[i].surfID = -1;
	}
	gDecalCount = 0;

	for ( i = 0; i < DECAL_CACHEENTRY; i++ )
		gDecalCache[i].decalIndex = -1;
}


void R_DecalTerm( model_t *model )
{
	if( !model )
		return;

	for( int i = 0; i < model->brush.numsurfaces; i++ )
	{
		decal_t *pNext;
		for( decal_t *pDecal=MSurf_Decals( i, model ); pDecal; pDecal=pNext )
		{
			R_DecalUnlink( pDecal, model );
			pNext = pDecal->pnext;
		}

		Assert( MSurf_Decals( i, model ) == NULL );
	}

	// R_DecalInit is more like a 'clear' function.
	R_DecalInit();
}


static int R_DecalIndex( decal_t *pdecal )
{
	return ( pdecal - gDecalPool );
}


static int R_DecalCacheIndex( int index )
{
	return index & (DECAL_CACHEENTRY-1);
}


static decalcache_t *R_DecalCacheSlot( int decalIndex )
{
	int				cacheIndex;

	cacheIndex = R_DecalCacheIndex( decalIndex );	// Find the cache slot

	return gDecalCache + cacheIndex;
}


// Release the cache entry for this decal
static void R_DecalCacheClear( decal_t *pdecal )
{
	int				index;
	decalcache_t	*pCache;

	index = R_DecalIndex( pdecal );
	pCache = R_DecalCacheSlot( index );		// Find the cache slot

	if ( pCache->decalIndex == index )		// If this is the decal that's cached here, clear it.
		pCache->decalIndex = -1;
}


// Unlink pdecal from any surface it's attached to
static void R_DecalUnlink( decal_t *pdecal, model_t *model )
{
	decal_t *tmp;

	R_DecalCacheClear( pdecal );
	if ( IS_SURF_VALID( pdecal->surfID ) )
	{
		if ( MSurf_Decals( pdecal->surfID, model ) == pdecal )
		{
			MSurf_Decals( pdecal->surfID, model ) = pdecal->pnext;
		}
		else 
		{
			tmp = MSurf_Decals( pdecal->surfID, model );
			if ( !tmp )
				Sys_Error("Bad decal list");
			while ( tmp->pnext ) 
			{
				if ( tmp->pnext == pdecal ) 
				{
					tmp->pnext = pdecal->pnext;
					break;
				}
				tmp = tmp->pnext;
			}
		}
		
		// Tell the displacement surface.
		if( SurfaceHasDispInfo( pdecal->surfID, model ) )
		{
			MSurf_DispInfo( pdecal->surfID, model )->NotifyRemoveDecal( pdecal->m_DispDecal );
		}
	}

	pdecal->surfID = -1;
}


// Just reuse next decal in list
// A decal that spans multiple surfaces will use multiple decal_t pool entries, as each surface needs
// it's own.
static decal_t *R_DecalAlloc( decal_t *pdecal )
{
	int limit = MAX_DECALS;

	if ( r_decals.GetInt() < limit )
		limit = r_decals.GetInt();
	
	if ( !limit )
		return NULL;

	if ( !pdecal ) 
	{
		int count;

		count = 0;		// Check for the odd possiblity of infinte loop
		do 
		{
			gDecalCount++;
			if ( gDecalCount >= limit )
				gDecalCount = 0;
			pdecal = gDecalPool + gDecalCount;	// reuse next decal
			count++;
		} while ( (pdecal->flags & FDECAL_PERMANENT) && count < limit );
	}
	
	// If decal is already linked to a surface, unlink it.
	R_DecalUnlink( pdecal, host_state.worldmodel );

	return pdecal;	
}

// The world coordinate system is right handed with Z up.
// 
//      ^ Z
//      |
//      |   
//      | 
//X<----|
//       \
//		  \
//         \ Y

void R_DecalSurface( int surfID, decalinfo_t *decalinfo, bool bForceForDisplacement )
{
	// Get the texture associated with this surface
	mtexinfo_t* tex = MSurf_TexInfo( surfID );

	Vector4D &textureU = tex->textureVecsTexelsPerWorldUnits[0];
	Vector4D &textureV = tex->textureVecsTexelsPerWorldUnits[1];

	// project decal center into the texture space of the surface
	float s = DotProduct( decalinfo->m_Position, textureU.AsVector3D() ) + 
		textureU.w - MSurf_TextureMins( surfID )[0];
	float t = DotProduct( decalinfo->m_Position, textureV.AsVector3D() ) + 
		textureV.w - MSurf_TextureMins( surfID )[1];


	// Determine the decal basis (measured in world space)
	// Note that the decal basis vectors 0 and 1 will always lie in the same
	// plane as the texture space basis vectors	textureVecsTexelsPerWorldUnits.

	R_DecalComputeBasis( MSurf_Plane( surfID ).normal,
		(decalinfo->m_Flags & FDECAL_USESAXIS) ? &decalinfo->m_SAxis : 0,
		decalinfo->m_Basis );

	// Compute an effective width and height (axis aligned)	in the parent texture space
	// How does this work? decalBasis[0] represents the u-direction (width)
	// of the decal measured in world space, decalBasis[1] represents the 
	// v-direction (height) measured in world space.
	// textureVecsTexelsPerWorldUnits[0] represents the u direction of 
	// the surface's texture space measured in world space (with the appropriate
	// scale factor folded in), and textureVecsTexelsPerWorldUnits[1]
	// represents the texture space v direction. We want to find the dimensions (w,h)
	// of a square measured in texture space, axis aligned to that coordinate system.
	// All we need to do is to find the components of the decal edge vectors
	// (decalWidth * decalBasis[0], decalHeight * decalBasis[1])
	// in texture coordinates:

	float w = fabs( decalinfo->m_decalWidth  * DotProduct( textureU.AsVector3D(), decalinfo->m_Basis[0] ) ) +
		fabs( decalinfo->m_decalHeight * DotProduct( textureU.AsVector3D(), decalinfo->m_Basis[1] ) );
	
	float h = fabs( decalinfo->m_decalWidth  * DotProduct( textureV.AsVector3D(), decalinfo->m_Basis[0] ) ) +
		fabs( decalinfo->m_decalHeight * DotProduct( textureV.AsVector3D(), decalinfo->m_Basis[1] ) );

	// move s,t to upper left corner
	s -= ( w * 0.5 );
	t -= ( h * 0.5 );

	// Is this rect within the surface? -- tex width & height are unsigned
	if( !bForceForDisplacement )
	{
		if ( s <= -w || t <= -h || 
			 s > (MSurf_TextureExtents( surfID )[0]+w) || t > (MSurf_TextureExtents( surfID )[1]+h) )
		{
			return; // nope
		}
	}

	// stamp it
	R_DecalCreate( decalinfo, surfID, s, t, bForceForDisplacement );
}

//-----------------------------------------------------------------------------
// iterate over all surfaces on a node, looking for surfaces to decal
//-----------------------------------------------------------------------------

static void R_DecalNodeSurfaces( mnode_t* node, decalinfo_t *decalinfo )
{
	// iterate over all surfaces in the node
	int surfID = node->firstsurface;
	for ( int i=0; i<node->numsurfaces ; ++i, ++surfID) 
	{
		//If this is a water decal, ONLY decal water, reject other surfaces
		if ( decalinfo->m_Flags & FDECAL_WATER )
		{
			if ( ( MSurf_Flags( surfID ) & SURFDRAW_WATERSURFACE ) == false )
				continue;
		}

		// Displacement surfaces get decals in R_DecalLeaf.
        if ( SurfaceHasDispInfo( surfID ) )
            continue;

		R_DecalSurface( surfID, decalinfo, false );
	}
}						 


void R_DecalLeaf( mleaf_t *pLeaf, decalinfo_t *decalinfo )
{
	for ( int i = 0; i < pLeaf->nummarksurfaces; i++ )
	{
		int surfID = host_state.worldmodel->brush.marksurfaces[pLeaf->firstmarksurface + i];
		
		// only process leaf surfaces
		if ( MSurf_Flags( surfID ) & SURFDRAW_NODE )
			continue;

		//If this is a water decal, ONLY decal water, reject other surfaces
		if ( decalinfo->m_Flags & FDECAL_WATER )
		{
			if ( ( MSurf_Flags( surfID ) & SURFDRAW_WATERSURFACE ) == false )
				continue;
		}

		Assert( !MSurf_DispInfo( surfID ) );

		float dist = fabs( DotProduct(decalinfo->m_Position, MSurf_Plane( surfID ).normal) - MSurf_Plane( surfID ).dist);
		if ( dist < DECAL_DISTANCE )
		{
			R_DecalSurface( surfID, decalinfo, false );
		}
	}

	// Add the decal to each displacement in the leaf it touches.
	for( CDispIterator it=pLeaf->GetDispIterator(); it.IsValid(); )
	{
		CDispLeafLink *pCur = it.Inc();
		IDispInfo *pDisp = static_cast<IDispInfo*>( pCur->m_pDispInfo );

		// Make sure the decal hasn't already been added to it.
		if( pDisp->GetTag() )
			continue;

		//Don't allow water decals on dispsurfs
		if ( decalinfo->m_Flags & FDECAL_WATER )
			continue;

		pDisp->SetTag();

		// Trivial bbox reject.
		Vector bbMin, bbMax;
		pDisp->GetBoundingBox( bbMin, bbMax );
		if( decalinfo->m_Position.x - decalinfo->m_Size < bbMax.x && decalinfo->m_Position.x + decalinfo->m_Size > bbMin.x && 
			decalinfo->m_Position.y - decalinfo->m_Size < bbMax.y && decalinfo->m_Position.y + decalinfo->m_Size > bbMin.y && 
			decalinfo->m_Position.z - decalinfo->m_Size < bbMax.z && decalinfo->m_Position.z + decalinfo->m_Size > bbMin.z )
		{
			R_DecalSurface( pDisp->GetParent(), decalinfo, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Recursive routine to find surface to apply a decal to.  World coordinates of 
// the decal are passed in r_recalpos like the rest of the engine.  This should 
// be called through R_DecalShoot()
//-----------------------------------------------------------------------------

static void R_DecalNode( mnode_t *node, decalinfo_t* decalinfo )
{
	cplane_t	*splitplane;
	float		dist;
	
	if (!node )
		return;
	if ( node->contents >= 0 )
	{
		R_DecalLeaf( (mleaf_t *)node, decalinfo );
		return;
	}

	splitplane = node->plane;
	dist = DotProduct (decalinfo->m_Position, splitplane->normal) - splitplane->dist;

	// This is arbitrarily set to 10 right now.  In an ideal world we'd have the 
	// exact surface but we don't so, this tells me which planes are "sort of 
	// close" to the gunshot -- the gunshot is actually 4 units in front of the 
	// wall (see dlls\weapons.cpp). We also need to check to see if the decal 
	// actually intersects the texture space of the surface, as this method tags
	// parallel surfaces in the same node always.
	// JAY: This still tags faces that aren't correct at edges because we don't 
	// have a surface normal

	if (dist > decalinfo->m_Size)
	{
		R_DecalNode (node->children[0], decalinfo);
	}
	else if (dist < -decalinfo->m_Size)
	{
		R_DecalNode (node->children[1], decalinfo);
	}
	else 
	{
		if ( dist < DECAL_DISTANCE && dist > -DECAL_DISTANCE )
			R_DecalNodeSurfaces( node, decalinfo );

		R_DecalNode (node->children[0], decalinfo);
		R_DecalNode (node->children[1], decalinfo);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pList - 
//			count - 
// Output : static int
//-----------------------------------------------------------------------------
static int DecalListAdd( decallist_t *pList, int count )
{
	int			i;
	Vector		tmp;
	decallist_t	*pdecal;

	pdecal = pList + count;
	for ( i = 0; i < count; i++ )
	{
		if ( !Q_strcmp( pdecal->name, pList[i].name ) && 
			pdecal->entityIndex == pList[i].entityIndex )
		{
			VectorSubtract( pdecal->position, pList[i].position, tmp );	// Merge
			if ( VectorLength( tmp ) < 2 )	// UNDONE: Tune this '2' constant
				return count;
		}
	}

	// This is a new decal
	return count + 1;
}


typedef int (*qsortFunc_t)( const void *, const void * );

static int DecalDepthCompare( const decallist_t *elem1, const decallist_t *elem2 )
{
	if ( elem1->depth > elem2->depth )
		return -1;
	if ( elem1->depth < elem2->depth )
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Called by CSaveRestore::SaveClientState
// Input  : *pList - 
// Output : int
//-----------------------------------------------------------------------------
int DecalListCreate( decallist_t *pList )
{
	int total = 0;
	int i, depth;

	if ( host_state.worldmodel )
	{
		for ( i = 0; i < MAX_DECALS; i++ )
		{
			decal_t *decal = &gDecalPool[i];

			// Decal is in use and is not a custom decal
			if ( !IS_SURF_VALID( decal->surfID ) ||
				 (decal->flags & ( FDECAL_CUSTOM | FDECAL_DONTSAVE ) ) )	
				 continue;

			decal_t		*pdecals;
			IMaterial 	*pMaterial;

			// compute depth
			depth = 0;
			pdecals = MSurf_Decals( decal->surfID );
			while ( pdecals && pdecals != decal )
			{
				depth++;
				pdecals = pdecals->pnext;
			}
			pList[total].depth = depth;
			pList[total].flags = decal->flags;
			
			R_DecalUnProject( decal, &pList[total] );

			pMaterial = decal->material;
			Q_strncpy( pList[total].name, pMaterial->GetName(), sizeof( pList[total].name ) );

			// Check to see if the decal should be added
			total = DecalListAdd( pList, total );
		}
	}

	// Sort the decals lowest depth first, so they can be re-applied in order
	qsort( pList, total, sizeof(decallist_t), ( qsortFunc_t )DecalDepthCompare );

	return total;
}
// ---------------------------------------------------------

static bool R_DecalUnProject( decal_t *pdecal, decallist_t *entry )
{
	if ( !pdecal || !IS_SURF_VALID( pdecal->surfID ) )
		return false;

	VectorCopy( pdecal->position, entry->position );
	entry->entityIndex = pdecal->entityIndex;

	// Grab surface plane equation
	cplane_t plane = MSurf_Plane( pdecal->surfID );

	VectorCopy( plane.normal, entry->impactPlaneNormal );
	return true;
}


// Shoots a decal onto the surface of the BSP.  position is the center of the decal in world coords
static void R_DecalShoot_( IMaterial *pMaterial, int entity, const model_t *model, 
						  const Vector &position, const Vector *saxis, int flags, const color32 &rgbaColor, void *userdata = 0 )
{
	decalinfo_t decalInfo;
	VectorCopy( position, decalInfo.m_Position );	// Pass position in global
	if ( model )
	{
		decalInfo.m_pModel = (model_t *)model;
	}
	else
		decalInfo.m_pModel = NULL;

	if ( !model || !decalInfo.m_pModel || decalInfo.m_pModel->type != mod_brush || !pMaterial )
	{
		// Con_DPrintf("Decals must hit mod_brush!\n");
		return;
	}

	// Deal with the s axis if one was passed in
	if (saxis)
	{
		flags |= FDECAL_USESAXIS;
		VectorCopy( *saxis, decalInfo.m_SAxis );
	}

	// More state used by R_DecalNode()
	decalInfo.m_pMaterial = pMaterial;
	decalInfo.m_pUserData = userdata;

	// Don't optimize custom decals
	if ( !(flags & FDECAL_CUSTOM) )
		flags |= FDECAL_CLIPTEST;
	
	decalInfo.m_Flags = flags;
	decalInfo.m_Entity = entity;
	decalInfo.m_Size = pMaterial->GetMappingWidth() >> 1;
	if ( (int)(pMaterial->GetMappingHeight() >> 1) > decalInfo.m_Size )
		decalInfo.m_Size = pMaterial->GetMappingHeight() >> 1;

	// Compute scale of surface
	// FIXME: cache this?
	IMaterialVar *decalScaleVar;
	bool found;
	decalScaleVar = decalInfo.m_pMaterial->FindVar( "$decalScale", &found, false );
	if( found )
	{
		decalInfo.m_scale = 1.0f / decalScaleVar->GetFloatValue();
		decalInfo.m_Size *= decalScaleVar->GetFloatValue();
	}
	else
	{
		decalInfo.m_scale = 1.0f;
	}

	// compute the decal dimensions in world space
	decalInfo.m_decalWidth = pMaterial->GetMappingWidth() / decalInfo.m_scale;
	decalInfo.m_decalHeight = pMaterial->GetMappingHeight() / decalInfo.m_scale;
	decalInfo.m_Color = rgbaColor;

	// Clear the displacement tags because we use them in R_DecalNode.
	DispInfo_ClearAllTags( decalInfo.m_pModel->brush.hDispInfos );

	mnode_t *pnodes = decalInfo.m_pModel->brush.nodes + decalInfo.m_pModel->brush.firstnode;
	R_DecalNode( pnodes, &decalInfo );
}

// Shoots a decal onto the surface of the BSP.  position is the center of the decal in world coords
// This is called from cl_parse.cpp, cl_tent.cpp
void R_DecalShoot( int textureIndex, int entity, const model_t *model, const Vector &position, const Vector *saxis, int flags, const color32 &rgbaColor )
{	
	IMaterial* pMaterial = Draw_DecalMaterial( textureIndex );
	R_DecalShoot_( pMaterial, entity, model, position, saxis, flags, rgbaColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *material - 
//			playerIndex - 
//			entity - 
//			*model - 
//			position - 
//			*saxis - 
//			flags - 
//			&rgbaColor - 
//-----------------------------------------------------------------------------
void R_PlayerDecalShoot( IMaterial *material, void *userdata, int entity, const model_t *model, 
	const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor )
{
	Assert( userdata != 0 );
	R_DecalShoot_( material, entity, model, position, saxis, flags, rgbaColor, userdata );
}

// Generate lighting coordinates at each vertex for decal vertices v[] on surface psurf
static void R_DecalVertsLight( CDecalVert* v, int surfID, int vertCount )
{
	int j;
	float s, t;

	int lightmapPageWidth, lightmapPageHeight;

	materialSystemInterface->GetLightmapPageSize( SortInfoToLightmapPage(MSurf_MaterialSortID( surfID )),
		&lightmapPageWidth, &lightmapPageHeight );
	
	for ( j = 0; j < vertCount; j++, v++ )
	{
		s = DotProduct( v->m_vPos, MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[0].AsVector3D() ) + 
			MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[0][3];
		s -= MSurf_LightmapMins( surfID )[0];
		s += MSurf_OffsetIntoLightmapPage( surfID )[0];
		s += 0.5f;
		s *= ( 1.0f / lightmapPageWidth );

		t = DotProduct( v->m_vPos, MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[1].AsVector3D() ) + 
			MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[1][3];
		t -= MSurf_LightmapMins( surfID )[1];
		t += MSurf_OffsetIntoLightmapPage( surfID )[1];
		t += 0.5f;
		t *= ( 1.0f / lightmapPageHeight );

		v->m_LMCoords.x = s;
		v->m_LMCoords.y = t;
	}
}


static CDecalVert* R_DecalVertsNoclip( decal_t *pdecal, int surfID, IMaterial *pMaterial )
{
	decalcache_t	*pCache;
	int				decalIndex;
	int				outCount;

	decalIndex = R_DecalIndex( pdecal );
	pCache = R_DecalCacheSlot( decalIndex );
	
	// Is the decal cached?
	if ( pCache->decalIndex == decalIndex )
	{
		return &pCache->decalVert[0];
	}

	pCache->decalIndex = decalIndex;

	CDecalVert *vlist = &pCache->decalVert[0];

	// Use the old code for now, and just cache them
	vlist = R_DecalVertsClip( vlist, pdecal, surfID, pMaterial, &outCount );

	R_DecalVertsLight( vlist, surfID, 4 );

	return vlist;
}


//-----------------------------------------------------------------------------
// Purpose: Check for intersecting decals on this surface
// Input  : *psurf - 
//			*pcount - 
//			x - 
//			y - 
// Output : static decal_t
//-----------------------------------------------------------------------------
// UNDONE: This probably doesn't work quite right any more
// we should base overlap on the new decal basis matrix
// decal basis is constant per plane, perhaps we should store it (unscaled) in the shared plane struct
// BRJ: Note, decal basis is not constant when decals need to specify an s direction
// but that certainly isn't the majority case
static decal_t *R_DecalIntersect( decalinfo_t* decalinfo, int surfID, int *pcount )
{
	decal_t		*plast;
	IMaterial	*pMaterial;


	plast = NULL;
	*pcount = 0;


	// (Same as R_SetupDecalClip).
	pMaterial = decalinfo->m_pMaterial;
	
	// Precalculate the extents of decalinfo's decal in world space.
	int mapSize[2] = {pMaterial->GetMappingWidth(), pMaterial->GetMappingHeight()};
	Vector decalExtents[2];
	decalExtents[0] = decalinfo->m_Basis[0] * (mapSize[0] / decalinfo->m_scale) * 0.5f;
	decalExtents[1] = decalinfo->m_Basis[1] * (mapSize[1] / decalinfo->m_scale) * 0.5f;


	float lastArea = 2;
	decal_t *pDecal = MSurf_Decals( surfID );
	while ( pDecal ) 
	{
		pMaterial = pDecal->material;

		// Don't steal bigger decals and replace them with smaller decals
		// Don't steal permanent decals
		if ( !(pDecal->flags & FDECAL_PERMANENT) )
		{
			Vector testBasis[3];
			float testWorldScale[2];
			R_SetupDecalTextureSpaceBasis( pDecal, MSurf_Plane( surfID ).normal, pDecal->material, testBasis, testWorldScale );

			// Here, we project the min and max extents of the decal that got passed in into
			// this decal's (pDecal's) [0,0,1,1] clip space, just like we would if we were
			// clipping a triangle into pDecal's clip space.
			Vector2D vDecalMin(
				DotProduct( decalinfo->m_Position - decalExtents[0], testBasis[0] ) - pDecal->dx + 0.5f,
				DotProduct( decalinfo->m_Position - decalExtents[1], testBasis[1] ) - pDecal->dy + 0.5f );

			Vector2D vDecalMax( 
				DotProduct( decalinfo->m_Position + decalExtents[0], testBasis[0] ) - pDecal->dx + 0.5f,
				DotProduct( decalinfo->m_Position + decalExtents[1], testBasis[1] ) - pDecal->dy + 0.5f );	

			// Now figure out the part of the projection that intersects pDecal's
			// clip box [0,0,1,1].
			Vector2D vUnionMin( max( vDecalMin.x, 0 ), max( vDecalMin.y, 0 ) );
			Vector2D vUnionMax( min( vDecalMax.x, 1 ), min( vDecalMax.y, 1 ) );

			if( vUnionMin.x < 1 && vUnionMin.y < 1 && vUnionMax.x > 0 && vUnionMax.y > 0 )
			{
				// Figure out how much of this intersects the (0,0) - (1,1) bbox.			
				float flArea = (vUnionMax.x - vUnionMin.x) * (vUnionMax.y - vUnionMin.y);

				if( flArea > 0.6 )
				{
					*pcount += 1;
					if ( !plast || flArea <= lastArea ) 
					{
						plast = pDecal;
						lastArea =  flArea;
					}
				}
			}
		}
		
		pDecal = pDecal->pnext;
	}
	
	return plast;
}


// Add the decal to the surface's list of decals.
// If the surface is a displacement, let the displacement precalculate data for the decal.
static void R_AddDecalToSurface( 
	decal_t *pdecal, 
	int surfID,
	decalinfo_t *decalinfo )
{
	pdecal->pnext = NULL;
	decal_t *pold = MSurf_Decals( surfID );
	if ( pold ) 
	{
		while ( pold->pnext ) 
			pold = pold->pnext;
		pold->pnext = pdecal;
	}
	else
	{
		MSurf_Decals( surfID ) = pdecal;
	}

	// Tag surface
	pdecal->surfID = surfID;
	pdecal->m_Size = decalinfo->m_Size;

	// Let the dispinfo reclip the decal if need be.
	if( SurfaceHasDispInfo( surfID ) )
	{
		pdecal->m_DispDecal = MSurf_DispInfo( surfID )->NotifyAddDecal( pdecal );
	}
}


// Allocate and initialize a decal from the pool, on surface with offsets x, y
// UNDONE: offsets are not really meaningful in new decal coordinate system
// the clipping code will recalc the offsets
static void R_DecalCreate( 
	decalinfo_t* decalinfo, 
	int surfID, 
	float x, 
	float y, 
	bool bForceForDisplacement )
{
	decal_t			*pdecal;
	int				count, vertCount;

	if( !IS_SURF_VALID( surfID ) )
	{
		Con_Printf( "psurface NULL in R_DecalCreate!\n" );
		return;
	}
	
	decal_t *pold = R_DecalIntersect( decalinfo, surfID, &count );
	if ( count < MAX_OVERLAP_DECALS ) 
		pold = NULL;
	pdecal = R_DecalAlloc( pold );
	
	pdecal->flags = decalinfo->m_Flags;
	pdecal->color = decalinfo->m_Color;
	VectorCopy( decalinfo->m_Position, pdecal->position );
	if (pdecal->flags & FDECAL_USESAXIS)
		VectorCopy( decalinfo->m_SAxis, pdecal->saxis );
	pdecal->dx = x;
	pdecal->dy = y;
	pdecal->material = decalinfo->m_pMaterial;
	Assert( pdecal->material );
	pdecal->userdata = decalinfo->m_pUserData;

	// Set scaling
	pdecal->scale = decalinfo->m_scale;
	pdecal->entityIndex = decalinfo->m_Entity;

	// Get dynamic information from the material (fade start, fade time)
	bool found;
	IMaterialVar* decalVar = decalinfo->m_pMaterial->FindVar( "$decalFadeDuration", &found, false );
	if ( found  )
	{
		pdecal->flags |= FDECAL_DYNAMIC;
		pdecal->fadeDuration = decalVar->GetFloatValue();
		decalVar = decalinfo->m_pMaterial->FindVar( "$decalFadeTime", &found, false );
		pdecal->fadeStartTime = found ? decalVar->GetFloatValue() : 0.0f;
		pdecal->fadeStartTime += cl.gettime();
	}

	// Is this a second-pass decal?
	decalVar = decalinfo->m_pMaterial->FindVar( "$decalSecondPass", &found, false );
	if ( found  )
		pdecal->flags |= FDECAL_SECONDPASS;

	if( !bForceForDisplacement )
	{
		// Check to see if the decal actually intersects the surface
		// if not, then remove the decal
		R_DecalVertsClip( NULL, pdecal, surfID, 
			decalinfo->m_pMaterial, &vertCount );
		if ( !vertCount )
		{
			R_DecalUnlink( pdecal, host_state.worldmodel );
			return;
		}
	}

	// Add to the surface's list
	R_AddDecalToSurface( pdecal, surfID, decalinfo );
}


//-----------------------------------------------------------------------------
// Updates all decals, returns true if the decal should be retired
//-----------------------------------------------------------------------------

bool DecalUpdate( decal_t* pDecal )
{
	// retire the decal if it's time has come
	if (pDecal->fadeDuration > 0)
	{
		return (cl.gettime() >= pDecal->fadeStartTime + pDecal->fadeDuration);
	}
	return false;
}


// Build the vertex list for a decal on a surface and clip it to the surface.
// This is a template so it can work on world surfaces and dynamic displacement 
// triangles the same way.
CDecalVert* R_DecalSetupVerts( decal_t *pDecal, int surfID, IMaterial *pMaterial, int &outCount )
{
	CDecalVert *v;
	if ( pDecal->flags & FDECAL_NOCLIP )
	{
		v = R_DecalVertsNoclip( pDecal, surfID, pMaterial );
		outCount = 4;
	}
	else
	{
		v = R_DecalVertsClip( NULL, pDecal, surfID, pMaterial, &outCount );
		if ( outCount )
		{
			R_DecalVertsLight( v, surfID, outCount );
		}
	}
	
	return v;
}


//-----------------------------------------------------------------------------
// Renders a single decal, *could retire the decal!!*
//-----------------------------------------------------------------------------

void DecalUpdateAndDrawSingle( int surfID, decal_t* pDecal )
{
	if( !pDecal->material )
		return;

	// Update dynamic decals
	bool retire = false;
	if ( pDecal->flags & FDECAL_DYNAMIC )
		retire = DecalUpdate( pDecal );

	if( SurfaceHasDispInfo( surfID ) )
	{
		// Dispinfos generate lists of tris for decals when the decal is first
		// created.
	}
	else
	{
		int outCount;
		CDecalVert *v = R_DecalSetupVerts( pDecal, surfID, pDecal->material, outCount );

		if ( outCount )
			Shader_DecalDrawPoly( v, pDecal->material, surfID, outCount, pDecal );
	}

	if( retire )
	{
		R_DecalUnlink( pDecal, host_state.worldmodel );
	}
}


//-----------------------------------------------------------------------------
// Renders all decals on a single surface
//-----------------------------------------------------------------------------

static void DrawDecalsOnSingleSurface( int surfID)
{
	decal_t* plist = MSurf_Decals( surfID );

#if 1
	// FIXME!  Make this not truck through the decal list twice.
	while ( plist ) 
	{
		// Store off the next pointer, DecalUpdateAndDrawSingle could unlink
		decal_t* pnext = plist->pnext;

		if (!(plist->flags & FDECAL_SECONDPASS))
		{
			DecalUpdateAndDrawSingle( surfID, plist );
		}
		plist = pnext;
	}
	plist = MSurf_Decals( surfID );
	while ( plist ) 
	{
		// Store off the next pointer, DecalUpdateAndDrawSingle could unlink
		decal_t* pnext = plist->pnext;

		if ((plist->flags & FDECAL_SECONDPASS))
		{
			DecalUpdateAndDrawSingle( surfID, plist );
		}
		plist = pnext;
	}
#else
	// FIXME!!:  This code screws up since DecalUpdateAndDrawSingle can 
	// unlink items from the decal list.
	// The version below is used instead, which trucks through memory twice. . . 
	// not optimal.
	decal_t* pPrev = 0;
	decal_t* pSecondPass = 0;
	while ( plist ) 
	{
		// Store off the next pointer, DecalUpdateAndDrawSingle could unlink
		decal_t* pnext = plist->pnext;

		if ((plist->flags & FDECAL_SECONDPASS) != 0)
		{
			if (pPrev)
				pPrev->pnext = pnext;
			else
				psurf->pdecals = pnext;
			plist->pnext = pSecondPass;
			pSecondPass = plist;
		}
		else
		{
			DecalUpdateAndDrawSingle( psurf, plist );
			pPrev = plist;
		}
		plist = pnext;
	}

	// Re-link second pass...
	if (pPrev)
		pPrev->pnext = pSecondPass;
	else
		psurf->pdecals = pSecondPass;

	plist = pSecondPass;
	while ( plist )
	{
		// Store off the next pointer, DecalUpdateAndDrawSingle could unlink
		decal_t* pnext = plist->pnext;
		DecalUpdateAndDrawSingle( psurf, plist );
		plist = pnext;
	}
#endif
}


//-----------------------------------------------------------------------------
// Renders all decals
//-----------------------------------------------------------------------------
void DecalSurfaceDraw( int renderGroup )
{
	VPROF("DecalSurfaceDraw");
	int count = s_DecalSurfaces[renderGroup].Count();
	if ( !count )
		return;

	MEASURE_TIMED_STAT( ENGINE_STATS_DECAL_RENDER ); 
		
	// The new "unclipped" decals are aligned to texture space and can be reverse winding
	// of the surfaces
	// UNDONE: Now they are simply cached, not recomputed, so we don't need this.

	for ( int i = 0; i < count; ++i ) 
	{
		DrawDecalsOnSingleSurface( s_DecalSurfaces[renderGroup][i] );
	}
}


#pragma check_stack(off)
#endif // SWDS
