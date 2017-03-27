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

// wrapper for the material system for the engine.

#include "winquake.h"
#include "r_local.h"
#include "gl_matsysiface.h"
#include "gl_model_private.h"
#include "view.h"
#include "gl_cvars.h"
#include "zone.h"
#include "dispchain.h"
#include "gl_texture.h"
#include <float.h>
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "sys_dll.h"
#include "materialsystem/imesh.h"
#include "gl_water.h"
#include "utlrbtree.h"
#include "istudiorender.h"
#include "gl_lightmap.h"
#include "materialsystem/materialsystem_config.h"
#include "tier0/dbg.h"
#include "vstdlib/random.h"
#include "lightcache.h"
#include "sysexternal.h"
#include "cmd.h"
#include "conprint.h"
#include "modelloader.h"
#include "vstdlib/icommandline.h"

extern ConVar developer;
#ifdef _WIN32
#include <crtdbg.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IMaterialSystem *materialSystemInterface;

IMaterial*	g_materialWireframe;
IMaterial*	g_materialTranslucentSingleColor;
IMaterial*	g_materialTranslucentVertexColor;
IMaterial*	g_materialWorldWireframe;
IMaterial*	g_materialWorldWireframeZBuffer;
IMaterial*	g_materialBrushWireframe;
IMaterial*	g_materialDecalWireframe;
IMaterial*	g_materialDebugLightmap;
IMaterial*	g_materialDebugLightmapZBuffer;
IMaterial*	g_materialDebugLuxels;
IMaterial*	g_materialLeafVisWireframe;
IMaterial*	g_materialVolumetricFog;
IMaterial*	g_pMaterialWireframeVertexColor;
IMaterial*  g_pMaterialWireframeVertexColorIgnoreZ;
IMaterial*  g_pMaterialLightSprite;
IMaterial*  g_pMaterialShadowBuild;
IMaterial*  g_pMaterialMRMWireframe;
IMaterial*  g_pMaterialWriteZ;

void WorldStaticMeshCreate( void );
void WorldStaticMeshDestroy( void );


bool TangentSpaceSurfaceSetup( int surfID, Vector &tVect );
void TangentSpaceComputeBasis( Vector& tangent, Vector& binormal, const Vector& normal, const Vector& tVect, bool negateTangent );

//-----------------------------------------------------------------------------
// Converts sort infos to lightmap pages
//-----------------------------------------------------------------------------

int SortInfoToLightmapPage( int sortID )
{
	return materialSortInfoArray[sortID].lightmapPageID;
}

//-----------------------------------------------------------------------------
// A console command to spew out driver information
//-----------------------------------------------------------------------------

void Cmd_MatCrosshair_f (void)
{
	IMaterial* pMaterial = GetMaterialAtCrossHair();
	if (!pMaterial)
		Con_Printf ("no/bad material\n");
	else
		Con_Printf ("hit material \"%s\"\n", pMaterial->GetName());
}

void Cmd_MatInfo_f (void);
void Cmd_MatSuppress_f (void);
void Cmd_MatDebug_f (void);
void Cmd_MatCrosshair_f (void);

static ConCommand mat_info("mat_info",Cmd_MatInfo_f);
static ConCommand mat_suppress("mat_suppress",Cmd_MatSuppress_f);
static ConCommand mat_debug("mat_debug",Cmd_MatDebug_f);
static ConCommand mat_crosshair( "mat_crosshair", Cmd_MatCrosshair_f );

static void RegisterLightmappedSurface( int surfID )
{
	int lightmapSize[2];
	int allocationWidth, allocationHeight;
	bool needsBumpmap;
	
	// fixme: lightmapSize needs to be in msurface_t once we
	// switch over to having lightmap size untied to base texture
	// size
	lightmapSize[0] = ( MSurf_LightmapExtents( surfID )[0] ) + 1;
	lightmapSize[1] = ( MSurf_LightmapExtents( surfID )[1] ) + 1;
	
	// Allocate all bumped lightmaps next to each other so that we can just 
	// increment the s texcoord by pSurf->bumpSTexCoordOffset to render the next
	// of the three lightmaps
	needsBumpmap = SurfNeedsBumpedLightmaps( surfID );
	if( needsBumpmap )
	{
		MSurf_Flags( surfID ) |= SURFDRAW_BUMPLIGHT;
		allocationWidth = lightmapSize[0] * ( NUM_BUMP_VECTS+1 );
	}
	else
	{
		MSurf_Flags( surfID ) &= ~SURFDRAW_BUMPLIGHT;
		allocationWidth = lightmapSize[0];
	}
	
	allocationHeight = lightmapSize[1];

	int offsetIntoLightmapPage[2];
	// register this surface's lightmap
	MSurf_MaterialSortID( surfID ) = materialSystemInterface->AllocateLightmap( 
		allocationWidth, 
		allocationHeight,
		offsetIntoLightmapPage,
		MSurf_TexInfo( surfID )->material );

	MSurf_OffsetIntoLightmapPage( surfID )[0] = offsetIntoLightmapPage[0];
	MSurf_OffsetIntoLightmapPage( surfID )[1] = offsetIntoLightmapPage[1];
}

static void RegisterUnlightmappedSurface( int surfID )
{
	MSurf_MaterialSortID( surfID ) = materialSystemInterface->AllocateWhiteLightmap( MSurf_TexInfo( surfID )->material );
	MSurf_OffsetIntoLightmapPage( surfID )[0] = 0;
	MSurf_OffsetIntoLightmapPage( surfID )[1] = 0;
}

static bool LightmapLess( int const& surfID1, int const& surfID2 )
{
	// FIXME: This really should be in the material system,
	// as it completely depends on the behavior of the lightmap packer
	bool hasLightmap1 = (MSurf_Flags( surfID1 ) & SURFDRAW_NOLIGHT) == 0;
	bool hasLightmap2 = (MSurf_Flags( surfID2 ) & SURFDRAW_NOLIGHT) == 0;

	// We want lightmapped surfaces to show up first
	if (hasLightmap1 != hasLightmap2)
		return hasLightmap1 > hasLightmap2;
	
	// The sort by enumeration ID
	IMaterial* pMaterial1 = MSurf_TexInfo( surfID1 )->material;
	IMaterial* pMaterial2 = MSurf_TexInfo( surfID2 )->material;
	int enum1 = pMaterial1->GetEnumerationID();
	int enum2 = pMaterial2->GetEnumerationID();
	if (enum1 != enum2)
		return enum1 < enum2;

	// Then sort by lightmap height for better packing...
	return MSurf_LightmapExtents( surfID1 )[1] < MSurf_LightmapExtents( surfID2 )[1];
}

void MaterialSystem_RegisterLightmapSurfaces( void )
{
	int surfID = -1;

	materialSystemInterface->BeginLightmapAllocation();

	// Add all the surfaces to a list, sorted by lightmapped or not 
	// and by material enumeration
	CUtlRBTree< int, int >	surfaces( 0, host_state.worldmodel->brush.numsurfaces, LightmapLess );
	for( surfID = 0; surfID < host_state.worldmodel->brush.numsurfaces; surfID++ )
	{
		if( ( MSurf_TexInfo( surfID )->flags & SURF_NOLIGHT ) || 
			( MSurf_Flags( surfID ) & SURFDRAW_NOLIGHT) )
		{
			MSurf_Flags( surfID ) |= SURFDRAW_NOLIGHT;
		}
		else
		{
			MSurf_Flags( surfID ) &= ~SURFDRAW_NOLIGHT;
		}

		surfaces.Insert(surfID);
	}

	surfID = -1;
	for (int i = surfaces.FirstInorder(); i != surfaces.InvalidIndex(); i = surfaces.NextInorder(i) )
	{
		int surfID = surfaces[i];

		bool hasLightmap = ( MSurf_Flags( surfID ) & SURFDRAW_NOLIGHT) == 0;
		if( hasLightmap )
		{
			RegisterLightmappedSurface( surfID );
		}
		else
		{
			RegisterUnlightmappedSurface( surfID );
		}
	}
	materialSystemInterface->EndLightmapAllocation();
}

static void TestBumpSanity( int surfID )
{
	ASSERT_SURF_VALID( surfID );
 // use the last one to check if we need a bumped lightmap, but don't have it so that we can warn.
	bool needsBumpmap = SurfNeedsBumpedLightmaps( surfID );
	bool hasBumpmap = SurfHasBumpedLightmaps( surfID );
	
	if( needsBumpmap && !hasBumpmap )
	{
		Warning( "Need to rebuild map to get bumped lighting on material %s\n", 
			materialSortInfoArray[MSurf_MaterialSortID( surfID )].material->GetName() );
	}
}

void MaterialSytsem_DoBumpWarnings( void )
{
	int sortID;
	int surfID;
	IMaterial *pPrevMaterial = NULL;

	for( sortID = 0; sortID < g_NumMaterialSortBins; sortID++ )
	{
		if( pPrevMaterial == materialSortInfoArray[sortID].material )
		{
			continue;
		}
		// Find one surface in each material sort info type
		for ( surfID = 0; surfID < host_state.worldmodel->brush.numsurfaces; surfID++ )
		{
			if( MSurf_MaterialSortID( surfID ) == sortID )
			{
				TestBumpSanity( surfID );
				break;
			}
		}
		pPrevMaterial = materialSortInfoArray[sortID].material;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void GenerateTexCoordsForPrimVerts( void )
{
	int surfID, j, k, l;
	for( surfID = 0; surfID < host_state.worldmodel->brush.numsurfaces; surfID++ )
	{
/*
		if( pSurf->numPrims > 0 )
		{
			Con_Printf( "pSurf %d has %d prims (normal: %f %f %f dist: %f)\n", 
				( int )i, ( int )pSurf->numPrims, 
				pSurf->plane->normal[0], pSurf->plane->normal[1], pSurf->plane->normal[2], 
				pSurf->plane->dist );
			Con_Printf( "\tfirst primID: %d\n", ( int )pSurf->firstPrimID );
		}
*/
		for( j = 0; j < MSurf_NumPrims( surfID ); j++ )
		{
			mprimitive_t *pPrim;
			assert( MSurf_FirstPrimID( surfID ) + j < host_state.worldmodel->brush.numprimitives );
			pPrim = &host_state.worldmodel->brush.primitives[MSurf_FirstPrimID( surfID ) + j];
			for( k = 0; k < pPrim->vertCount; k++ )
			{
				int lightmapSize[2];
				int lightmapPageSize[2];
				float sOffset, sScale, tOffset, tScale;
				
				materialSystemInterface->GetLightmapPageSize( 
					SortInfoToLightmapPage( MSurf_MaterialSortID( surfID ) ), 
					&lightmapPageSize[0], &lightmapPageSize[1] );
				lightmapSize[0] = ( MSurf_LightmapExtents( surfID )[0] ) + 1;
				lightmapSize[1] = ( MSurf_LightmapExtents( surfID )[1] ) + 1;

				sScale = 1.0f / ( float )lightmapPageSize[0];
				sOffset = ( float )MSurf_OffsetIntoLightmapPage( surfID )[0] * sScale;
				sScale = MSurf_LightmapExtents( surfID )[0] * sScale;

				tScale = 1.0f / ( float )lightmapPageSize[1];
				tOffset = ( float )MSurf_OffsetIntoLightmapPage( surfID )[1] * tScale;
				tScale = MSurf_LightmapExtents( surfID )[1] * tScale;

				for ( l = 0; l < pPrim->vertCount; l++ )
				{
					// world-space vertex
					assert( l+pPrim->firstVert < host_state.worldmodel->brush.numprimverts );
					mprimvert_t &vert = host_state.worldmodel->brush.primverts[l+pPrim->firstVert];
					Vector& vec = vert.pos;

					// base texture coordinate
					vert.texCoord[0] = DotProduct (vec, MSurf_TexInfo( surfID )->textureVecsTexelsPerWorldUnits[0].AsVector3D()) + 
						MSurf_TexInfo( surfID )->textureVecsTexelsPerWorldUnits[0][3];
					vert.texCoord[0] /= MSurf_TexInfo( surfID )->material->GetMappingWidth();

					vert.texCoord[1] = DotProduct (vec, MSurf_TexInfo( surfID )->textureVecsTexelsPerWorldUnits[1].AsVector3D()) + 
						MSurf_TexInfo( surfID )->textureVecsTexelsPerWorldUnits[1][3];
					vert.texCoord[1] /= MSurf_TexInfo( surfID )->material->GetMappingHeight();

					if ( MSurf_Flags( surfID ) & SURFDRAW_NOLIGHT )
					{
						vert.lightCoord[0] = 0.5f;
						vert.lightCoord[1] = 0.5f;
					}
					else
					{
						vert.lightCoord[0] = DotProduct (vec, MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[0].AsVector3D()) + 
							MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[0][3];
						vert.lightCoord[0] -= MSurf_LightmapMins( surfID )[0];
						vert.lightCoord[0] += 0.5f;
						vert.lightCoord[0] /= ( float )MSurf_LightmapExtents( surfID )[0]; //pSurf->texinfo->texture->width;

						vert.lightCoord[1] = DotProduct (vec, MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[1].AsVector3D()) + 
							MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[1][3];
						vert.lightCoord[1] -= MSurf_LightmapMins( surfID )[1];
						vert.lightCoord[1] += 0.5f;
						vert.lightCoord[1] /= ( float )MSurf_LightmapExtents( surfID )[1]; //pSurf->texinfo->texture->height;
						
						vert.lightCoord[0] = sOffset + vert.lightCoord[0] * sScale;
						vert.lightCoord[1] = tOffset + vert.lightCoord[1] * tScale;
					}
				}
			
			}			
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MaterialSystem_CreateSortinfo( void )
{
	assert( !materialSortInfoArray );

	g_NumMaterialSortBins = materialSystemInterface->GetNumSortIDs();
	materialSortInfoArray = ( MaterialSystem_SortInfo_t * )new MaterialSystem_SortInfo_t[ g_NumMaterialSortBins ];
	assert( materialSortInfoArray);
	materialSystemInterface->GetSortInfo( materialSortInfoArray );

	// Create texcoords for subdivided surfaces
	GenerateTexCoordsForPrimVerts();
	// Create the hardware vertex buffers for each face
	WorldStaticMeshCreate();
	if( developer.GetInt() )
	{
		MaterialSytsem_DoBumpWarnings();
	}
}

bool SurfHasBumpedLightmaps( int surfID )
{
	ASSERT_SURF_VALID( surfID );
	bool hasBumpmap = false;
	if( ( MSurf_TexInfo( surfID )->flags & SURF_BUMPLIGHT ) && 
		( !( MSurf_TexInfo( surfID )->flags & SURF_NOLIGHT ) ) &&
		( host_state.worldmodel->brush.lightdata ) &&
		( MSurf_Samples( surfID ) ) )
	{
		hasBumpmap = true;
	}
	return hasBumpmap;
}

bool SurfNeedsBumpedLightmaps( int surfID )
{
	ASSERT_SURF_VALID( surfID );
	assert( MSurf_TexInfo( surfID ) );
	assert( MSurf_TexInfo( surfID )->material );
	return MSurf_TexInfo( surfID )->material->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS );
}

bool SurfHasLightmap( int surfID )
{
	ASSERT_SURF_VALID( surfID );
	bool hasLightmap = false;
	if( ( !( MSurf_TexInfo( surfID )->flags & SURF_NOLIGHT ) ) &&
		( host_state.worldmodel->brush.lightdata ) &&
		( MSurf_Samples( surfID ) ) )
	{
		hasLightmap = true;
	}
	return hasLightmap;
}

bool SurfNeedsLightmap( int surfID )
{
	ASSERT_SURF_VALID( surfID );
	assert( MSurf_TexInfo( surfID ) );
	assert( MSurf_TexInfo( surfID )->material );
	if (MSurf_TexInfo( surfID )->flags & SURF_NOLIGHT)
		return false;

	return MSurf_TexInfo( surfID )->material->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_LIGHTMAP );
}



//-----------------------------------------------------------------------------
// Purpose: This builds the surface info for a terrain face
//-----------------------------------------------------------------------------

void BuildMSurfaceVerts( const model_t *pWorld, int surfID, Vector *verts,
							Vector2D *texCoords, Vector2D lightCoords[][4] )
{ 
	SurfaceCtx_t ctx;
	SurfSetupSurfaceContext( ctx, surfID );

	int vertCount = MSurf_VertCount( surfID );
	int vertFirstIndex = MSurf_FirstVertIndex( surfID );
	for ( int i = 0; i < vertCount; i++ )
	{
		int vertIndex = pWorld->brush.vertindices[vertFirstIndex + i];

		// world-space vertex
		Vector& vec = pWorld->brush.vertexes[vertIndex].position;

		// output to mesh
		if ( verts )
		{
			VectorCopy( vec, verts[i] );
		}

		if ( texCoords )
		{
			SurfComputeTextureCoordinate( ctx, surfID, vec, texCoords[i] );
		}

		//
		// garymct: normalized (within space of surface) lightmap texture coordinates
		//
		if ( lightCoords )
		{
			SurfComputeLightmapCoordinate( ctx, surfID, vec, lightCoords[i][0] );

			if ( MSurf_Flags( surfID ) & SURFDRAW_BUMPLIGHT )
			{
				// bump maps appear left to right in lightmap page memory, calculate the offset for the
				// width of a single map
				for ( int bumpID = 1; bumpID <= NUM_BUMP_VECTS; bumpID++ )
				{
					lightCoords[i][bumpID][0] = lightCoords[i][0][0] + (bumpID * ctx.m_BumpSTexCoordOffset);
					lightCoords[i][bumpID][1] = lightCoords[i][0][1];
				}
			}
		}
	}
}



void BuildMSurfacePrimVerts( model_t *pWorld, mprimitive_t *prim, CMeshBuilder &builder, int surfID )
{
	Vector tVect;
	bool negate = false;
// FIXME: For some reason, normals are screwed up on water surfaces.  Revisit this once we have normals started in primverts.
	if ( MSurf_Flags( surfID ) & SURFDRAW_TANGENTSPACE )
	{
		negate = TangentSpaceSurfaceSetup( surfID, tVect );
	}
//	Vector& normal = pWorld->brush.vertnormals[ pWorld->brush.vertnormalindices[MSurf_FirstVertNormal( surfID )] ];

	for ( int i = 0; i < prim->vertCount; i++ )
	{
		mprimvert_t &primVert = pWorld->brush.primverts[prim->firstVert + i];
		builder.Position3fv( primVert.pos.Base() );
		builder.Normal3fv( MSurf_Plane( surfID ).normal.Base() );
//		builder.Normal3fv( normal.Base() );
		builder.TexCoord2fv( 0, primVert.texCoord );
		builder.TexCoord2fv( 1, primVert.lightCoord );
		if ( MSurf_Flags( surfID ) & SURFDRAW_TANGENTSPACE )
		{
			Vector binormal, tangent;
//			TangentSpaceComputeBasis( tangent, binormal, normal, tVect, negate );
			TangentSpaceComputeBasis( tangent, binormal, MSurf_Plane( surfID ).normal, tVect, false );
			builder.TangentS3fv( tangent.Base() );
			builder.TangentT3fv( binormal.Base() );
		}
		builder.AdvanceVertex();
	}
}

void BuildMSurfacePrimIndices( model_t *pWorld, mprimitive_t *prim, CMeshBuilder &builder )
{
	for ( int i = 0; i < prim->indexCount; i++ )
	{
		unsigned short primIndex = pWorld->brush.primindices[prim->firstIndex + i];
		builder.Index( primIndex - prim->firstVert );
		builder.AdvanceIndex();
	}
}

//-----------------------------------------------------------------------------
// Here's a version of the mesh builder used to allow for client DLL to draw brush models
//-----------------------------------------------------------------------------

void BuildBrushModelVertexArray(model_t *pWorld, int surfID, BrushVertex_t* pVerts )
{
	SurfaceCtx_t ctx;
	SurfSetupSurfaceContext( ctx, surfID );

	Vector tVect;
	bool negate = false;
	if ( MSurf_Flags( surfID ) & SURFDRAW_TANGENTSPACE )
	{
		negate = TangentSpaceSurfaceSetup( surfID, tVect );
	}

	for ( int i = 0; i < MSurf_VertCount( surfID ); i++ )
	{
		int vertIndex = pWorld->brush.vertindices[MSurf_FirstVertIndex( surfID ) + i];

		// world-space vertex
		Vector& vec = pWorld->brush.vertexes[vertIndex].position;

		// output to mesh
		VectorCopy( vec, pVerts[i].m_Pos );

		Vector2D uv;
		SurfComputeTextureCoordinate( ctx, surfID, vec, pVerts[i].m_TexCoord );

		// garymct: normalized (within space of surface) lightmap texture coordinates
		SurfComputeLightmapCoordinate( ctx, surfID, vec, pVerts[i].m_LightmapCoord );

// Activate this if necessary
//		if ( surf->flags & SURFDRAW_BUMPLIGHT )
//		{
//			// bump maps appear left to right in lightmap page memory, calculate 
//			// the offset for the width of a single map. The pixel shader will use 
//			// this to compute the actual texture coordinates
//			builder.TexCoord2f( 2, ctx.m_BumpSTexCoordOffset, 0.0f );
//		}

		Vector& normal = pWorld->brush.vertnormals[ pWorld->brush.vertnormalindices[MSurf_FirstVertNormal( surfID ) + i] ];
		VectorCopy( normal, pVerts[i].m_Normal );

		if ( MSurf_Flags( surfID ) & SURFDRAW_TANGENTSPACE )
		{
			Vector binormal, tangent;
			TangentSpaceComputeBasis( tangent, binormal, normal, tVect, negate );
			VectorCopy( tangent, pVerts[i].m_TangentS );
			VectorCopy( binormal, pVerts[i].m_TangentT );
		}
	}
}

IMaterial *GetMaterialAtCrossHair( void )
{
	Vector endPoint;
	Vector lightmapColor;

#ifdef _WIN32
	// max_range * sqrt(3)
	VectorMA( r_origin, COORD_EXTENT * 1.74f, vpn, endPoint );
	
	int hitSurfID = R_LightVec( r_origin, endPoint, false, lightmapColor );
	if( IS_SURF_VALID( hitSurfID ) )
	{
		return MSurf_TexInfo( hitSurfID )->material;
	}
	else
	{
		return NULL;
	}
#endif
}

// hack
extern void DrawLightmapPage( int lightmapPageID );

static float textureS, textureT;
static int s_CrossHairSurfID;;
static Vector crossHairDiffuseLightColor;
static Vector crossHairBaseColor;
static float lightmapCoords[2];

void SaveSurfAtCrossHair()
{
#ifdef _WIN32
	Vector endPoint;
	Vector lightmapColor;

	// max_range * sqrt(3)
	VectorMA( r_origin, COORD_EXTENT * 1.74f, vpn, endPoint );
	
	s_CrossHairSurfID = R_LightVec( r_origin, endPoint, false, lightmapColor, 
		&textureS, &textureT, &lightmapCoords[0], &lightmapCoords[1] );
#endif
}


void DebugDrawLightmapAtCrossHair()
{
	return;
	IMaterial *pMaterial;
	int lightmapPageSize[2];

	if( s_CrossHairSurfID <= 0 )
	{
		return;
	}
	materialSystemInterface->GetLightmapPageSize( materialSortInfoArray[MSurf_MaterialSortID( s_CrossHairSurfID )].lightmapPageID, 
		&lightmapPageSize[0], &lightmapPageSize[1] );
	pMaterial = MSurf_TexInfo( s_CrossHairSurfID )->material;
//	pMaterial->GetLowResColorSample( textureS, textureT, baseColor );
	DrawLightmapPage( materialSortInfoArray[MSurf_MaterialSortID( s_CrossHairSurfID )].lightmapPageID );

#if 0
	int i;
	for( i = 0; i < 2; i++ )
	{
		xy[i] = 
			( ( float )pCrossHairSurf->offsetIntoLightmapPage[i] / ( float )lightmapPageSize[i] ) +
			lightmapCoord[i] * ( pCrossHairSurf->lightmapExtents[i] / ( float )lightmapPageSize[i] );
	}

	materialSystemInterface->Bind( g_materialWireframe );
	IMesh* pMesh = materialSystemInterface->GetDynamicMesh( g_materialWireframe );
	
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUAD, 1 );

	meshBuilder.Position3f( 
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
#endif
}

void ReleaseMaterialSystemObjects();
void RestoreMaterialSystemObjects();

void ForceMatSysRestore()
{
	ReleaseMaterialSystemObjects();
	RestoreMaterialSystemObjects();
}

