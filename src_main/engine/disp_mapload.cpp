//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
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

#include "glquake.h"
#include "gl_cvars.h"
#include "gl_model_private.h"
#include "gl_lightmap.h"
#include "disp.h"
#include "mathlib.h"
#include "gl_rsurf.h"
#include "gl_matsysiface.h"
#include "enginestats.h"
#include "r_local.h"
#include "zone.h"
#include "materialsystem/imesh.h"
#include "vector.h"
#include "iscratchpad3d.h"
#include "tier0/fasttimer.h"
#include "lowpassstream.h"
#include "con_nprint.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void BuildTagData( CCoreDispInfo *pCoreDisp, CDispInfo *pDisp );

// This makes sure that whoever is creating and deleting CDispInfos frees them all
// (and calls their destructors) before the module is gone.
class CConstructorChecker
{
public:
			CConstructorChecker()	{m_nConstructedObjects = 0;}
			~CConstructorChecker()	{Assert(m_nConstructedObjects == 0);}
	int		m_nConstructedObjects;
} g_ConstructorChecker;
 


//-----------------------------------------------------------------------------
// Static helpers.
//-----------------------------------------------------------------------------

static void BuildDispGetSurfNormals( Vector points[4], Vector normals[4] )
{
	//
	// calculate the displacement surface normal
	//
	Vector tmp[2];
	Vector normal;
	tmp[0] = points[1] - points[0];
	tmp[1] = points[3] - points[0];
	normal = tmp[1].Cross( tmp[0] );
	VectorNormalize( normal );

	for( int i = 0; i < 4; i++ )
	{
		normals[i] = normal;
	}
}


static bool FindExtraDependency( unsigned short *pDependencies, int nDependencies, int iDisp )
{
	for( int i=0; i < nDependencies; i++ )
	{
		if ( pDependencies[i] == iDisp )
			return true;
	}
	return false;
}


static CDispGroup* FindCombo( CUtlVector<CDispGroup*> &combos, int idLMPage, IMaterial *pMaterial )
{
	for( int i=0; i < combos.Size(); i++ )
	{
		if( combos[i]->m_LightmapPageID == idLMPage && combos[i]->m_pMaterial == pMaterial )
			return combos[i];
	}
	return NULL;
}


static CDispGroup* AddCombo( CUtlVector<CDispGroup*> &combos, int idLMPage, IMaterial *pMaterial )
{
	CDispGroup *pCombo = new CDispGroup;
	pCombo->m_LightmapPageID = idLMPage;
	pCombo->m_pMaterial = pMaterial;
	pCombo->m_nVisible = 0;
	combos.AddToTail( pCombo );
	return pCombo;
}


static inline CDispInfo* GetModelDisp( model_t const *pWorld, int i )
{
	return static_cast< CDispInfo* >(
		DispInfo_IndexArray( pWorld->brush.hDispInfos, i ) );
}			


static void BuildDispSurfInit( 
	model_t *pWorld, 
	CCoreDispInfo *pBuildDisp,
	int worldSurfID )
{
	if( !IS_SURF_VALID( worldSurfID ) )
		return;
	ASSERT_SURF_VALID( worldSurfID );
	
	int	   surfFlag = 0;
	Vector surfPoints[4];
	Vector surfNormals[4];
	Vector2D surfTexCoords[4];
	Vector2D surfLightCoords[4][4];

	if ( MSurf_VertCount( worldSurfID ) != 4 )
		return;

	int nLMVects = 1;
	if( MSurf_Flags( worldSurfID ) & SURFDRAW_BUMPLIGHT )
	{
		surfFlag |= CCoreDispInfo::SURF_BUMPED;
		nLMVects = NUM_BUMP_VECTS + 1;
	}
#ifndef SWDS
	BuildMSurfaceVerts( pWorld, worldSurfID, surfPoints, surfTexCoords, surfLightCoords );
#endif
	BuildDispGetSurfNormals( surfPoints, surfNormals );

	CCoreDispSurface *pDispSurf = pBuildDisp->GetSurface();

	pDispSurf->SetPointCount( 4 );
	for( int i = 0; i < 4; i++ )
	{
		pDispSurf->SetPoint( i, surfPoints[i] );
		pDispSurf->SetPointNormal( i, surfNormals[i] );
		pDispSurf->SetTexCoord( i, surfTexCoords[i] );

		pDispSurf->SetSAxis( MSurf_TexInfo( worldSurfID )->textureVecsTexelsPerWorldUnits[0].AsVector3D() );
		pDispSurf->SetTAxis( MSurf_TexInfo( worldSurfID )->textureVecsTexelsPerWorldUnits[1].AsVector3D() );

		for( int j = 0; j < nLMVects; j++ )
		{
			pDispSurf->SetLuxelCoord( j, i, surfLightCoords[i][j] );
		}
	}

	pDispSurf->SetFlags( surfFlag );
	pDispSurf->FindSurfPointStartIndex();
	pDispSurf->AdjustSurfPointData();

#ifndef SWDS
	//
	// adjust the lightmap coordinates -- this is currently done redundantly!
	// the will be fixed correctly when the displacement common code is written.
	// This is here to get things running for (GDC, E3)
	//
	SurfaceCtx_t ctx;
	SurfSetupSurfaceContext( ctx, worldSurfID );
	int lightmapWidth = MSurf_LightmapExtents( worldSurfID )[0];
	int lightmapHeight = MSurf_LightmapExtents( worldSurfID )[1];

	Vector2D uv( 0.0f, 0.0f );
	for ( int ndxLuxel = 0; ndxLuxel < 4; ndxLuxel++ )
	{
		switch( ndxLuxel )
		{
		case 0: { uv.Init( 0.0f, 0.0f ); break; }
		case 1: { uv.Init( 0.0f, ( float )lightmapHeight ); break; }
		case 2: { uv.Init( ( float )lightmapWidth, ( float )lightmapHeight ); break; }
		case 3: { uv.Init( ( float )lightmapWidth, 0.0f ); break; }
		}
		
		uv.x += 0.5f;
		uv.y += 0.5f;
		
		uv *= ctx.m_Scale; 
		uv += ctx.m_Offset; 
		
		pDispSurf->SetLuxelCoord( 0, ndxLuxel, uv );
	}
#endif
}


void AddEmptyMesh( 
	model_t *pWorld,
	CDispGroup *pCombo, 
	const ddispinfo_t *pMapDisps,
	int *pDispInfos,
	int nDisps,
	int nTotalVerts, 
	int nTotalIndices )
{
	CGroupMesh *pMesh = new CGroupMesh;
	pCombo->m_Meshes.AddToTail( pMesh );
	pMesh->m_pMesh = materialSystemInterface->CreateStaticMesh( pCombo->m_pMaterial, !r_DispUseStaticMeshes.GetBool() );
	pMesh->m_pGroup = pCombo;
	pMesh->m_nVisible = 0;

	CMeshBuilder builder;
	builder.Begin( pMesh->m_pMesh, MATERIAL_TRIANGLES, nTotalVerts, nTotalIndices );

		// Just advance the verts and indices and leave the data blank for now.
		builder.AdvanceIndices( nTotalIndices );
		builder.AdvanceVertices( nTotalVerts );

	builder.End();


	pMesh->m_DispInfos.SetSize( nDisps );
	pMesh->m_Visible.SetSize( nDisps );
	pMesh->m_VisibleDisps.SetSize( nDisps );

	int iVertOffset = 0;
	int iIndexOffset = 0;
	for( int iDisp=0; iDisp < nDisps; iDisp++ )
	{
		CDispInfo *pDisp = GetModelDisp( pWorld, pDispInfos[iDisp] );
		const ddispinfo_t *pMapDisp = &pMapDisps[ pDispInfos[iDisp] ];

		pDisp->m_pMesh = pMesh;
		pDisp->m_iVertOffset = iVertOffset;
		pDisp->m_iIndexOffset = iIndexOffset;

		int nVerts, nIndices;
		CalcMaxNumVertsAndIndices( pMapDisp->power, &nVerts, &nIndices );
		iVertOffset += nVerts;
		iIndexOffset += nIndices;
		
		pMesh->m_DispInfos[iDisp] = pDisp;
	}

	Assert( iVertOffset == nTotalVerts );
	Assert( iIndexOffset == nTotalIndices );
}


void FillStaticBuffer( 
	CGroupMesh *pMesh,
	CDispInfo *pDisp,
	const CCoreDispInfo *pCoreDisp,
	const CDispVert *pVerts,
	int nLightmaps )
{
#ifndef SWDS
	// Put the verts into the buffer.
	int nVerts, nIndices;
	CalcMaxNumVertsAndIndices( pDisp->GetPower(), &nVerts, &nIndices );
	
	CMeshBuilder builder;
	builder.BeginModify( pMesh->m_pMesh, pDisp->m_iVertOffset, nVerts, 0, 0 );

		SurfaceCtx_t ctx;
		SurfSetupSurfaceContext( ctx, pDisp->GetParent() );

		for( int i=0; i < nVerts; i++ )
		{
			// NOTE: position comes from our system-memory buffer so when you're restoring
			//       static buffers (from alt+tab), it includes changes from terrain mods.
			const Vector &vPos = pCoreDisp->GetVert( i );
			builder.Position3f( vPos.x, vPos.y, vPos.z );

			const Vector &vNormal = pCoreDisp->GetNormal( i );
			builder.Normal3f( vNormal.x, vNormal.y, vNormal.z );

			Vector vec;
			pCoreDisp->GetTangentS( i, vec );
			builder.TangentS3f( VectorExpand( vec ) );

			pCoreDisp->GetTangentT( i, vec );
			builder.TangentT3f( VectorExpand( vec ) );

			Vector2D texCoord;
			pCoreDisp->GetTexCoord( i, texCoord );
			builder.TexCoord2f( 0, texCoord.x, texCoord.y );

			Vector2D lightCoord;
			{
				pCoreDisp->GetLuxelCoord( 0, i, lightCoord );
				builder.TexCoord2f( 1, lightCoord.x, lightCoord.y );
			}

			if( nLightmaps > 1 )
			{
				SurfComputeLightmapCoordinate( ctx, pDisp->GetParent(), pDisp->m_Verts[i].m_vPos, lightCoord );
				builder.TexCoord2f( 2, ctx.m_BumpSTexCoordOffset, 0.0f );
			}

			builder.AdvanceVertex();
		}

	builder.EndModify();
#endif
}


void CDispInfo::CopyMapDispData( const ddispinfo_t *pBuildDisp )
{
	m_iLightmapAlphaStart = pBuildDisp->m_iLightmapAlphaStart;
	m_Power = pBuildDisp->power;

	Assert( m_Power >= 2 && m_Power <= NUM_POWERINFOS );
	m_pPowerInfo = ::GetPowerInfo( m_Power );

	// Max # of indices:
	// Take the number of triangles (2 * (size-1) * (size-1))
	// and multiply by 3!
	// These can be non-null in the case of task switch restore
	int size = GetSideLength();
	m_Indices.SetSize( 6 * (size-1) * (size-1) );

	// Per-node information
	if (m_pNodeInfo)
		delete[] m_pNodeInfo;

	m_pNodeInfo = new DispNodeInfo_t[m_pPowerInfo->m_NodeCount];
}


void DispInfo_CreateMaterialGroups( model_t *pWorld, const MaterialSystem_SortInfo_t *pSortInfos )
{
	for ( int iDisp=0; iDisp < pWorld->brush.numDispInfos; iDisp++ )
	{
		CDispInfo *pDisp = GetModelDisp( pWorld, iDisp );

		int idLMPage = pSortInfos[MSurf_MaterialSortID( pDisp->m_ParentSurfID )].lightmapPageID;
		
		CDispGroup *pCombo = FindCombo( g_DispGroups, idLMPage, MSurf_TexInfo( pDisp->m_ParentSurfID )->material );
		if( !pCombo )
			pCombo = AddCombo( g_DispGroups, idLMPage, MSurf_TexInfo( pDisp->m_ParentSurfID )->material );

		pCombo->m_DispInfos.AddToTail( iDisp );
	}
}


void DispInfo_LinkToParentFaces( model_t *pWorld, const ddispinfo_t *pMapDisps, int nDisplacements )
{
	for ( int iDisp=0; iDisp < nDisplacements; iDisp++ )
	{
		const ddispinfo_t *pMapDisp = &pMapDisps[iDisp];
		CDispInfo *pDisp = GetModelDisp( pWorld, iDisp );

		// Set its parent.
		Assert( pMapDisp->m_iMapFace >= 0 && pMapDisp->m_iMapFace < pWorld->brush.numsurfaces );
		Assert( MSurf_Flags( pMapDisp->m_iMapFace, pWorld ) & SURFDRAW_HAS_DISP );
		pWorld->brush.surfaces1[pMapDisp->m_iMapFace].pDispInfo = pDisp;
		pDisp->SetParent( pMapDisp->m_iMapFace );
	}
}


void DispInfo_CreateEmptyStaticBuffers( model_t *pWorld, const ddispinfo_t *pMapDisps )
{
	// For each combo, create empty buffers.
	for( int i=0; i < g_DispGroups.Size(); i++ )
	{
		CDispGroup *pCombo = g_DispGroups[i];

		int nTotalVerts=0, nTotalIndices=0;
		int iStart = 0;
		for( int iDisp=0; iDisp < pCombo->m_DispInfos.Size(); iDisp++ )
		{
			const ddispinfo_t *pMapDisp = &pMapDisps[pCombo->m_DispInfos[iDisp]];

			int nVerts, nIndices;
			CalcMaxNumVertsAndIndices( pMapDisp->power, &nVerts, &nIndices );
			
			// If we're going to pass our vertex buffer limit, or we're at the last one,
			// make a static buffer and fill it up.
			if( (nTotalVerts + nVerts) > MAX_STATIC_BUFFER_VERTS || 
				(nTotalIndices + nIndices) > MAX_STATIC_BUFFER_INDICES )
			{
				AddEmptyMesh( pWorld, pCombo, pMapDisps, &pCombo->m_DispInfos[iStart], iDisp-iStart, nTotalVerts, nTotalIndices );
				Assert( nTotalVerts > 0 && nTotalIndices > 0 );

				nTotalVerts = nTotalIndices = 0;
				iStart = iDisp;
				--iDisp;
			}
			else if( iDisp == pCombo->m_DispInfos.Size()-1 )
			{
				AddEmptyMesh( pWorld, pCombo, pMapDisps, &pCombo->m_DispInfos[iStart], iDisp-iStart+1, nTotalVerts+nVerts, nTotalIndices+nIndices );
				break;
			}
			else
			{
				nTotalVerts += nVerts;
				nTotalIndices += nIndices;
			}
		}
	}
}


bool DispInfo_SetupFromMapDisp( 
	model_t *pWorld, 
	int iDisp, 
	const ddispinfo_t *pMapDisp, 
	const CDispVert *pVerts,
	const CDispTri *pTris,
	const MaterialSystem_SortInfo_t *pSortInfos,
	bool bRestoring )
{
	CDispInfo *pDisp = GetModelDisp( pWorld, iDisp );

	// Setup the CCoreDispInfo using the ddispinfo_t and have it translate the data
	// into a format we'll copy into the rendering structures. This roundaboutness is because
	// of legacy code. It should all just be stored in the map file, but it's not a high priority right now.
	CCoreDispInfo coreDisp;
	coreDisp.GetSurface()->SetPointStart( pMapDisp->startPosition );

	coreDisp.InitDispInfo( pMapDisp->power, pMapDisp->minTess, pMapDisp->smoothingAngle, pVerts, pTris );
	coreDisp.SetNeighborData( pMapDisp->m_EdgeNeighbors, pMapDisp->m_CornerNeighbors );

	// Copy the allowed verts list.
	ErrorIfNot( 
		coreDisp.GetAllowedVerts().GetNumDWords() == sizeof( pMapDisp->m_AllowedVerts ) / 4,
		("DispInfo_StoreMapData: size mismatch in 'allowed verts' list")
		);
	
	for ( int j=0; j < coreDisp.GetAllowedVerts().GetNumDWords(); j++ )
		coreDisp.GetAllowedVerts().SetDWord( j, pMapDisp->m_AllowedVerts[j] );

	BuildDispSurfInit( pWorld, &coreDisp, pDisp->GetParent() );
	
	if ( !coreDisp.Create() )
	{
		return false;
	}

	// Save the point start index - needed for overlays.
	pDisp->m_iPointStart = coreDisp.GetSurface()->GetPointStartIndex();

	// Now setup the CDispInfo.
	pDisp->m_Index = (unsigned short)iDisp;

	// Store ddispinfo_t data.
	pDisp->CopyMapDispData( pMapDisp );

	// Store CCoreDispInfo data.	
	if( !pDisp->CopyCoreDispData( pWorld, pSortInfos, &coreDisp, bRestoring ) )
		return false;

	// Initialize all the active and other verts after setting up neighbors.
	pDisp->InitializeActiveVerts();
	pDisp->m_ErrorVerts.Copy( pDisp->m_ActiveVerts );
	pDisp->m_iLightmapSamplePositionStart = pMapDisp->m_iLightmapSamplePositionStart;

	// Now copy the CCoreDisp's data into the static buffer.
	FillStaticBuffer( pDisp->m_pMesh, pDisp, &coreDisp, pVerts, pDisp->NumLightMaps() );

	// Now build the tagged data for visualization.
	BuildTagData( &coreDisp, pDisp );

	return true;
}


bool DispInfo_LoadDisplacements( model_t *pWorld, bool bRestoring )
{
	const MaterialSystem_SortInfo_t *pSortInfos = materialSortInfoArray;

	int nDisplacements = CMapLoadHelper::LumpSize( LUMP_DISPINFO ) / sizeof( ddispinfo_t );	
	int nLuxels = CMapLoadHelper::LumpSize( LUMP_DISP_LIGHTMAP_ALPHAS );
	int nSamplePositionBytes = CMapLoadHelper::LumpSize( LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS );

	// Setup the world's list of displacements.
	if ( bRestoring )
	{

		/* Breakpoint-able: */
		if(pWorld->brush.numDispInfos != nDisplacements)
		{ 
			volatile int a = 0; a = a + 1; 
		}

		ErrorIfNot( 
			pWorld->brush.numDispInfos == nDisplacements,
			("DispInfo_LoadDisplacments: dispcounts (%d and %d) don't match.", pWorld->brush.numDispInfos, nDisplacements)
			);

		ErrorIfNot(
			g_DispLMAlpha.Count() == nLuxels,
			("DispInfo_LoadDisplacements: lightmap alpha counts (%d and %d) don't match.", g_DispLMAlpha.Count(), nLuxels)
			);
	}
	else
	{
		// Create the displacements.
		pWorld->brush.numDispInfos = nDisplacements;
		pWorld->brush.hDispInfos = DispInfo_CreateArray( pWorld->brush.numDispInfos );

		// Load lightmap alphas.
		g_DispLMAlpha.SetSize( nLuxels );
		CMapLoadHelper::LoadLumpData( LUMP_DISP_LIGHTMAP_ALPHAS, 0, nLuxels, g_DispLMAlpha.Base() );
	
		// Load lightmap sample positions.
		g_DispLightmapSamplePositions.SetSize( nSamplePositionBytes );
		CMapLoadHelper::LoadLumpData( LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS, 0, nSamplePositionBytes, g_DispLightmapSamplePositions.Base() );
	}

	
	// Free old data.
	DispInfo_ReleaseMaterialSystemObjects( pWorld );

	
	// Load the displacement info structures into temporary space.
	ddispinfo_t tempDisps[MAX_MAP_DISPINFO];
	ErrorIfNot( 
		nDisplacements <= MAX_MAP_DISPINFO,
		("DispInfo_LoadDisplacements: nDisplacements (%d) > MAX_MAP_DISPINFO (%d)", nDisplacements, MAX_MAP_DISPINFO)
		);
	CMapLoadHelper::LoadLumpData( LUMP_DISPINFO, 0, nDisplacements * sizeof( ddispinfo_t ), tempDisps );

	
	// Now hook up the displacements to their parents.
	DispInfo_LinkToParentFaces( pWorld, tempDisps, nDisplacements );


	// First, create "groups" (or "combos") which contain all the displacements that 
	// use the same material and lightmap.
	DispInfo_CreateMaterialGroups( pWorld, pSortInfos );

	// Now make the static buffers for each material/lightmap combo.
	DispInfo_CreateEmptyStaticBuffers( pWorld, tempDisps );



	// Now setup each displacement one at a time.
	CDispVert tempVerts[MAX_DISPVERTS];
	CDispTri tempTris[MAX_DISPTRIS];
	int iCurVert = 0;
	int iCurTri = 0;

	for ( int iDisp=0; iDisp < nDisplacements; iDisp++ )
	{
		ddispinfo_t *pMapDisp = &tempDisps[iDisp];
		
		// Load the vertices from the file.
		int nVerts = NUM_DISP_POWER_VERTS( pMapDisp->power );
		ErrorIfNot( nVerts <= MAX_DISPVERTS, ( "DispInfo_LoadDisplacements: invalid vertex count (%d)", nVerts ) );
		CMapLoadHelper::LoadLumpData( LUMP_DISP_VERTS, iCurVert * sizeof(CDispVert), nVerts*sizeof(CDispVert), tempVerts );
		iCurVert += nVerts;

		// Load the vertices from the file.
		int nTris = NUM_DISP_POWER_TRIS( pMapDisp->power );
		ErrorIfNot( nTris <= MAX_DISPTRIS, ( "DispInfo_LoadDisplacements: invalid tri count (%d)", nTris ) );
		CMapLoadHelper::LoadLumpData( LUMP_DISP_TRIS, iCurTri * sizeof(CDispTri), nTris*sizeof(CDispTri), tempTris );
		iCurTri += nTris;
	
		// Now initialize the CDispInfo.
		if ( !DispInfo_SetupFromMapDisp( pWorld, iDisp, pMapDisp, tempVerts, tempTris, 
			pSortInfos, bRestoring ) )
			return false;
	}	

	// If we're not using LOD, then maximally tesselate all the displacements and
	// make sure they never change.
	if ( r_DispEnableLOD.GetInt() == 0 )
	{
		for ( int iDisp=0; iDisp < nDisplacements; iDisp++ )
		{
			CDispInfo *pDisp = GetModelDisp( pWorld, iDisp );
			
			pDisp->m_ActiveVerts = pDisp->m_AllowedVerts;
			pDisp->m_bForceRebuild = false;
			pDisp->m_bFullyTesselated = true;
		}

		for ( iDisp=0; iDisp < nDisplacements; iDisp++ )
		{
			CDispInfo *pDisp = GetModelDisp( pWorld, iDisp );
			pDisp->TesselateDisplacement();			
		}		
	}

	return true;
}


void DispInfo_ReleaseMaterialSystemObjects( model_t *pWorld )
{
	// Free all the static meshes.
	for( int iGroup=0; iGroup < g_DispGroups.Size(); iGroup++ )
	{
		CDispGroup *pGroup = g_DispGroups[iGroup];
	
		for( int iMesh=0; iMesh < pGroup->m_Meshes.Size(); iMesh++ )
		{
			CGroupMesh *pMesh = pGroup->m_Meshes[iMesh];

			materialSystemInterface->DestroyStaticMesh( pMesh->m_pMesh );
		}

		pGroup->m_Meshes.PurgeAndDeleteElements();
	}

	g_DispGroups.PurgeAndDeleteElements();


	// Clear pointers in the dispinfos.
	if( pWorld )
	{
		for( int iDisp=0; iDisp < pWorld->brush.numDispInfos; iDisp++ )
		{
			CDispInfo *pDisp = GetModelDisp( pWorld, iDisp );
			if ( !pDisp )
			{
				Assert( 0 );
				continue;
			}
			
			pDisp->m_pMesh = NULL;
			pDisp->m_iVertOffset = pDisp->m_iIndexOffset = 0;
		}
	}
}


//-----------------------------------------------------------------------------
// CDispInfo implementation.
//-----------------------------------------------------------------------------

CDispInfo::CDispInfo()
{
	m_ParentSurfID = -1;

    m_bTouched = false;

	m_pNext = NULL;

	++g_ConstructorChecker.m_nConstructedObjects;

	m_BBoxMin.Init();
	m_BBoxMax.Init();

	m_idLMPage = -1;

	m_pPowerInfo = NULL;

	m_bRetesselate = false;
	m_ViewerSphereCenter.Init( 1e24, 1e24, 1e24 );
	
	m_bInUse = false;
	m_bFullyTesselated = false;
	m_bForceRebuild = false;
	m_bForceRetesselate = false;

	m_pNodeInfo = 0;

	m_pMesh = NULL;

	m_Tag = NULL;
	m_pDispArray = NULL;
	m_pLeafLinkHead = NULL;

	m_FirstDecal = DISP_DECAL_HANDLE_INVALID;
	m_FirstShadowDecal = DISP_SHADOW_HANDLE_INVALID;

	for ( int i=0; i < 4; i++ )
	{
		m_EdgeNeighbors[i].SetInvalid();
		m_CornerNeighbors[i].SetInvalid();
	}

	m_nExtraDependencies = 0;
}


CDispInfo::~CDispInfo()
{
	if (m_pNodeInfo)
		delete[] m_pNodeInfo;

	--g_ConstructorChecker.m_nConstructedObjects;

	// All the decals should have been freed through 
	// CModelLoader::Map_UnloadModel -> R_DecalTerm
	Assert( m_FirstDecal == DISP_DECAL_HANDLE_INVALID );
	Assert( m_FirstShadowDecal == DISP_SHADOW_HANDLE_INVALID );
}


void CDispInfo::AddExtraDependency( int iNeighborDisp )
{
	if( FindExtraDependency( m_ExtraDependencies, m_nExtraDependencies, iNeighborDisp ) )
		return;

	if( m_nExtraDependencies < MAX_EXTRA_DEPENDENCIES )
	{
		m_ExtraDependencies[m_nExtraDependencies++] = iNeighborDisp;
	}
	else
	{
		Assert( !"CDispInfo::AddExtraCornerNeighbor: overflowed neighbor array" );
	}
}


bool CDispInfo::CopyCoreDispData( 
	model_t *pWorld,
	const MaterialSystem_SortInfo_t *pSortInfos,
	const CCoreDispInfo *pCoreDisp,
	bool bRestoring )
{
	// Make sure we get rebuilt and tesselated next frame.
	m_bForceRebuild = true;

	// When restoring, leave all this data the same.
	if( bRestoring )
		return true;

	m_idLMPage = pSortInfos[MSurf_MaterialSortID( GetParent() )].lightmapPageID;

	const CCoreDispSurface *pSurface = pCoreDisp->GetSurface();
	for( int iTexCoord=0; iTexCoord < 4; iTexCoord++ )
	{
		pSurface->GetTexCoord( iTexCoord, m_BaseSurfaceTexCoords[iTexCoord] );
	}

	SurfaceCtx_t ctx;
	SurfSetupSurfaceContext( ctx, GetParent() );

	// Copy vertex positions (for backfacing tests).
	m_Verts.SetSize( m_pPowerInfo->m_MaxVerts );
	for( int i=0; i < NumVerts(); i++ )
	{
		pCoreDisp->GetVert( i, m_Verts[i].m_vPos );
		m_Verts[i].m_vOriginalPos = m_Verts[i].m_vPos; 

		pCoreDisp->GetTexCoord( i, m_Verts[i].m_vTexCoord );
		pCoreDisp->GetLuxelCoord( 0, i, m_Verts[i].m_LMCoords );

		// Only do this in debug because only mat_normals needs it
		pCoreDisp->GetNormal( i, m_Verts[i].m_vNormal );
		pCoreDisp->GetTangentS( i, m_Verts[i].m_vSVector );
		pCoreDisp->GetTangentT( i, m_Verts[i].m_vTVector );

		if( NumLightMaps() > 1 )
		{
			Vector2D lightCoord;
			SurfComputeLightmapCoordinate( ctx, GetParent(), m_Verts[i].m_vPos, lightCoord );
			m_Verts[i].m_BumpSTexCoordOffset = ctx.m_BumpSTexCoordOffset;
		}
	} 


	// Update the bounding box.
	UpdateBoundingBox();

	// Copy neighbor info.
	for ( int iEdge=0; iEdge < 4; iEdge++ )
	{
		m_EdgeNeighbors[iEdge] = *pCoreDisp->GetEdgeNeighbor( iEdge );
		m_CornerNeighbors[iEdge] = *pCoreDisp->GetCornerNeighbors( iEdge );
	}

	// Copy allowed verts.
	m_AllowedVerts = pCoreDisp->GetAllowedVerts();

	m_nIndices = 0;
	return true;
}


int CDispInfo::NumLightMaps()
{
	return (MSurf_Flags( m_ParentSurfID ) & SURFDRAW_BUMPLIGHT) ? NUM_BUMP_VECTS+1 : 1;
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: You cannot use the builddisp.cpp IsTriWalkable, IsTriBuildable functions
//       because the flags are different having been collapsed in vbsp 
//-----------------------------------------------------------------------------
void BuildTagData( CCoreDispInfo *pCoreDisp, CDispInfo *pDisp )
{
	int nWalkTest = 0;
	int nBuildTest = 0;
	int iTri;
	for ( iTri = 0; iTri < pCoreDisp->GetTriCount(); ++iTri )
	{
		if ( pCoreDisp->IsTriTag( iTri, DISPTRI_TAG_WALKABLE ) )
		{
			nWalkTest++;
		}

		if ( pCoreDisp->IsTriTag( iTri, DISPTRI_TAG_BUILDABLE ) )
		{
			nBuildTest++;
		}
	}

	nWalkTest *= 3;
	nBuildTest *= 3;

	pDisp->m_pWalkIndices = new unsigned short[nWalkTest];
	pDisp->m_pBuildIndices = new unsigned short[nBuildTest];

	int nWalkCount = 0;
	int nBuildCount = 0;
	for ( iTri = 0; iTri < pCoreDisp->GetTriCount(); ++iTri )
	{
		if ( pCoreDisp->IsTriTag( iTri, DISPTRI_TAG_WALKABLE ) )
		{
			pCoreDisp->GetTriIndices( iTri, 
								      pDisp->m_pWalkIndices[nWalkCount],
									  pDisp->m_pWalkIndices[nWalkCount+1],
									  pDisp->m_pWalkIndices[nWalkCount+2] );

			nWalkCount += 3;
		}

		if ( pCoreDisp->IsTriTag( iTri, DISPTRI_TAG_BUILDABLE ) )
		{
			pCoreDisp->GetTriIndices( iTri, 
								      pDisp->m_pBuildIndices[nBuildCount],
									  pDisp->m_pBuildIndices[nBuildCount+1],
									  pDisp->m_pBuildIndices[nBuildCount+2] );

			nBuildCount += 3;
		}
	}

	Assert( nWalkCount == nWalkTest );
	Assert( nBuildCount == nBuildTest );

	pDisp->m_nWalkIndexCount = nWalkCount;
	pDisp->m_nBuildIndexCount = nBuildCount;
}
