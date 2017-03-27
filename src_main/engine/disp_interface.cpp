//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "glquake.h"
#include "decal_private.h"
#include "disp_defs.h"
#include "disp.h"
#include "gl_model_private.h"
#include "gl_matsysiface.h"
#include "gl_cvars.h"
#include "gl_rsurf.h"
#include "enginestats.h"
#include "gl_lightmap.h"
#include "con_nprint.h"
#include "surfinfo.h"
#include "r_local.h"
#include "Overlay.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ----------------------------------------------------------------------------- //
// 	Shadow decals + fragments
// ----------------------------------------------------------------------------- //
static CUtlLinkedList< CDispShadowDecal,	DispShadowHandle_t >			s_DispShadowDecals;
static CUtlLinkedList< CDispShadowFragment, DispShadowFragmentHandle_t >	s_DispShadowFragments;
static CUtlLinkedList< CDispDecal,			DispShadowHandle_t >			s_DispDecals;
static CUtlLinkedList< CDispDecalFragment,	DispShadowFragmentHandle_t >	s_DispDecalFragments;



void CDispInfo::GetIntersectingSurfaces( GetIntersectingSurfaces_Struct *pStruct )
{
	if ( !m_Verts.Count() || !m_nIndices )
		return;

	// Walk through all of our triangles and add them one by one.
	unsigned short *pEnd = m_Indices.Base() + m_nIndices;
	SurfInfo *pOut = &pStruct->m_pInfos[ pStruct->m_nSetInfos ];
	for ( unsigned short *pTri = m_Indices.Base(); pTri < pEnd; pTri += 3 )
	{
		// Is the list going to overflow?
		if ( pStruct->m_nSetInfos >= pStruct->m_nMaxInfos )
			return;

		Vector const &a = m_Verts[pTri[0]].m_vPos;
		Vector const &b = m_Verts[pTri[1]].m_vPos;
		Vector const &c = m_Verts[pTri[2]].m_vPos;

		// Get the boundaries.
		Vector vMin;
		VectorMin( a, b, vMin );
		VectorMin( c, vMin, vMin );
		
		Vector vMax;
		VectorMax( a, b, vMax );
		VectorMax( c, vMax, vMax );

		// See if it touches the sphere.
		int iDim;
		for ( iDim=0; iDim < 3; iDim++ )
		{
			if ( ((*pStruct->m_pCenter)[iDim]+pStruct->m_Radius) < vMin[iDim] || 
				((*pStruct->m_pCenter)[iDim]-pStruct->m_Radius) > vMax[iDim] )
			{
				break;
			}
		}
		
		if ( iDim == 3 )
		{
			// Couldn't reject the sphere in the loop above, so add this surface.
			pOut->m_nVerts = 3;
			pOut->m_Verts[0] = a;
			pOut->m_Verts[1] = b;
			pOut->m_Verts[2] = c;
			pOut->m_Plane.m_Normal = ( c - a ).Cross( b - a );
			VectorNormalize( pOut->m_Plane.m_Normal );
			pOut->m_Plane.m_Dist = pOut->m_Plane.m_Normal.Dot( a );
			
			++pStruct->m_nSetInfos;
			++pOut;
		}
	}
}


// ----------------------------------------------------------------------------- //
// CDispInfo implementation of IDispInfo.
// ----------------------------------------------------------------------------- //
void CDispInfo::RenderWireframeInLightmapPage( void )
{
#ifndef SWDS
    // render displacement as wireframe into lightmap pages
	int surfID = GetParent();

	Assert( ( MSurf_MaterialSortID( surfID ) >= 0 ) && ( MSurf_MaterialSortID( surfID ) < g_NumMaterialSortBins ) );

	if( materialSortInfoArray[MSurf_MaterialSortID( surfID ) ].lightmapPageID != mat_showlightmappage.GetInt() )
		return;

	Shader_DrawLightmapPageSurface( surfID, 0.0f, 0.0f, 1.0f );
#endif
}


void CDispInfo::UpdateLightmapAlpha( Vector4D *pLightmapPage, int maxLMPageLuxels )
{
#ifndef SWDS
	// Make sure we won't go out of bounds.
	if( m_iLightmapAlphaStart + (MSurf_LightmapExtents( m_ParentSurfID )[0]+1) * 
		(MSurf_LightmapExtents( m_ParentSurfID )[1]+1) > g_DispLMAlpha.Size() ||
		(MSurf_LightmapExtents( m_ParentSurfID )[0]+1) * 
		(MSurf_LightmapExtents( m_ParentSurfID )[1]+1) > maxLMPageLuxels )
	{
		Sys_Error( "CDispInfo::UpdateLightmapAlpha - out-of-bounds lightmap alpha data" );
		return;
	}

	unsigned char *pCurLineSrc = &g_DispLMAlpha[ m_iLightmapAlphaStart ];
	Vector4D *pCurLineDest = pLightmapPage;
	for( int y=0; y <= MSurf_LightmapExtents( m_ParentSurfID )[1]; y++ )
	{
		unsigned char *pCurLuxelSrc = pCurLineSrc;
		Vector4D *pCurLuxelDest = pCurLineDest;
		for( int x=0; x <= MSurf_LightmapExtents( m_ParentSurfID )[0]; x++ )
		{
			// NOTE: The lightmap page passed in has (1 + alpha lights) in it.
			// This is for the nifty displacement alpha dlight trick. Let's try to
			// get alpha lights + displacement alpha into it.
			pCurLuxelDest->w -= 1.0f;	// Eliminate the '1' placed into it by the default brush code
			pCurLuxelDest->w += (float)*pCurLuxelSrc / 255.001f;
			pCurLuxelDest->w = clamp( pCurLuxelDest->w, 0, 1 );
			++pCurLuxelSrc;			
			++pCurLuxelDest;
		}

		pCurLineSrc += MSurf_LightmapExtents( m_ParentSurfID )[0]+1;
		pCurLineDest = pCurLuxelDest;
	}
#endif
}


void CDispInfo::GetBoundingBox( Vector& bbMin, Vector& bbMax )
{
	bbMin = m_BBoxMin;
	bbMax = m_BBoxMax;
}


void CDispInfo::SetParent( int surfID )
{
	m_ParentSurfID = surfID;
}


// returns surfID
int CDispInfo::GetParent( void )
{
	return m_ParentSurfID;
}


void CDispInfo::SetNextInRenderChain( IDispInfo *pDispInfoNext )
{
	m_pNext = pDispInfoNext;
}

void CDispInfo::SetNextInRayCastChain( IDispInfo *pDispInfoNext )
{
	m_pRayCastNext = pDispInfoNext;
}


IDispInfo *CDispInfo::GetNextInRenderChain( void )
{
	return m_pNext;
}

IDispInfo *CDispInfo::GetNextInRayCastChain( void )
{
	return m_pRayCastNext;
}


void CDispInfo::ApplyTerrainMod( ITerrainMod *pMod )
{
	// New bbox.
	Vector bbMin(  1e24,  1e24,  1e24 );
	Vector bbMax( -1e24, -1e24, -1e24 );

	int nVerts, nIndices;
	CalcMaxNumVertsAndIndices( m_Power, &nVerts, &nIndices );

	// Ok, it probably touches us. Lock our buffer and change the verts.
	CMeshBuilder mb;
	mb.BeginModify( m_pMesh->m_pMesh, m_iVertOffset, nVerts );
		
		for( int iVert=0; iVert < nVerts; iVert++ )
		{
			if( m_AllowedVerts.Get( iVert ) )
			{
				Vector &vPos = m_Verts[iVert].m_vPos;
				Vector &vOriginalPos = m_Verts[iVert].m_vOriginalPos;
				
				if( pMod->ApplyMod( vPos, vOriginalPos ) )
					mb.Position3f( VectorExpand( vPos ) );

				VectorMin( vPos, bbMin, bbMin );
				VectorMax( vPos, bbMax, bbMax );
			}
			
			mb.AdvanceVertex();
		}

	mb.EndModify();

	// Set our new bounding box.
	m_BBoxMin = bbMin;
	m_BBoxMax = bbMax;
	UpdateCenterAndRadius();

	// Next time this displacement is seen, force it to rebuild and retesselate.
	m_bForceRebuild = true;
}


int CDispInfo::ApplyTerrainMod_Speculative( 
	ITerrainMod *pMod,
	CSpeculativeTerrainModVert *pSpeculativeVerts,
	int nMaxVerts )
{
	int nModified = 0;

	if( nMaxVerts == 0 )
		return 0;

	int nVerts = NumVerts();
	for( int iVert=0; iVert < nVerts; iVert++ )
	{
		if( m_AllowedVerts.Get( iVert ) )
		{
			pSpeculativeVerts[nModified].m_vNew = m_Verts[iVert].m_vPos;
			Vector &vOriginalPos = m_Verts[iVert].m_vOriginalPos;
			
			if( pMod->ApplyMod( pSpeculativeVerts[nModified].m_vNew, vOriginalPos ) )
			{
				pSpeculativeVerts[nModified].m_vOriginal = vOriginalPos;
				pSpeculativeVerts[nModified].m_vCurrent = m_Verts[iVert].m_vPos;
				++nModified;


				if( nModified >= nMaxVerts )
					break;
			}
		}
	}

	return nModified;
}


void CDispInfo::AddDynamicLights( Vector4D *blocklight )
{
#ifndef SWDS
	if( !IS_SURF_VALID( m_ParentSurfID ) )
	{
		Assert( !"CDispInfo::AddDynamicLights: no parent surface" );
		return;
	}

	for( int lnum=0 ; lnum<MAX_DLIGHTS ; lnum++ )
	{
		// If the light's not active, then continue
		if ( (r_dlightactive & (1 << lnum)) == 0 )
			continue;

		// not lit by this light
		if ( !(MSurf_DLightBits( m_ParentSurfID ) & (1<<lnum) ) )
			continue;

		// This light doesn't affect the world
		if ( cl_dlights[lnum].flags & DLIGHT_NO_WORLD_ILLUMINATION)
			continue;
	
		if ( (cl_dlights[lnum].flags & DLIGHT_DISPLACEMENT_MASK) == 0)
		{
			AddSingleDynamicLight( cl_dlights[lnum], blocklight );
		}
		else
		{
			AddSingleDynamicAlphaLight( cl_dlights[lnum], blocklight );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Allocates fragments...
//-----------------------------------------------------------------------------
CDispDecalFragment* CDispInfo::AllocateDispDecalFragment( DispDecalHandle_t h )
{
	DispDecalFragmentHandle_t f = s_DispDecalFragments.Alloc(true);
	s_DispDecalFragments.LinkBefore( s_DispDecals[h].m_FirstFragment, f ); 
	s_DispDecals[h].m_FirstFragment = f;
	return &s_DispDecalFragments[f];
}


//-----------------------------------------------------------------------------
// Clears decal fragment lists
//-----------------------------------------------------------------------------
void CDispInfo::ClearDecalFragments( DispDecalHandle_t h )
{
	// Iterate over all fragments associated with each shadow decal
	CDispDecal& decal = s_DispDecals[h];
	DispDecalFragmentHandle_t f = decal.m_FirstFragment;
	DispDecalFragmentHandle_t next;
	while( f != DISP_DECAL_FRAGMENT_HANDLE_INVALID )
	{
		next = s_DispDecalFragments.Next(f);
		s_DispDecalFragments.Free(f);
		f = next;
	}

	// Blat out the list
	decal.m_FirstFragment = DISP_DECAL_FRAGMENT_HANDLE_INVALID;

	// Mark is as not computed
	decal.m_Flags &= ~CDispDecalBase::FRAGMENTS_COMPUTED;

	// Update the number of triangles in the decal
	decal.m_nTris = 0;
	decal.m_nVerts = 0;
}

void CDispInfo::ClearAllDecalFragments()
{
	// Iterate over all shadow decals on the displacement
	DispDecalHandle_t h = m_FirstDecal;
	while( h != DISP_SHADOW_HANDLE_INVALID )
	{
		ClearDecalFragments( h );
		h = s_DispDecals.Next(h);
	}
}


// ----------------------------------------------------------------------------- //
// Add/remove decals
// ----------------------------------------------------------------------------- //
DispDecalHandle_t CDispInfo::NotifyAddDecal( decal_t *pDecal )
{
	// FIXME: Add decal retirement...
//	if( m_Decals.Size() < MAX_DISP_DECALS )
//		return;

	// Create a new decal, link it in
	DispDecalHandle_t h = s_DispDecals.Alloc( true );

	// When linking, insert it in sorted order based on material enumeration ID
	// This will help us when rendering later
	int last = DISP_DECAL_HANDLE_INVALID;
	int i = m_FirstDecal;
	int enumerationId = pDecal->material->GetEnumerationID();
	while( i != s_DispDecals.InvalidIndex() )
	{
		int testId = s_DispDecals[i].m_pDecal->material->GetEnumerationID();
		if (enumerationId <= testId)
			break;

		last = i;
		i = s_DispDecals.Next(i);
	}

	// NOTE: when the list is used in multimode, we can't use LinkBefore( i, INVALID_INDEX ),
	// since the Head and Tail of the linked list are meaningless...

	if ( last != DISP_DECAL_HANDLE_INVALID )
		s_DispDecals.LinkAfter( last, h );
	else
	{
		s_DispDecals.LinkBefore( m_FirstDecal, h );
		m_FirstDecal = h;
	}

	CDispDecal *pDispDecal = &s_DispDecals[h];
	pDispDecal->m_pDecal = pDecal;
	pDispDecal->m_FirstFragment = DISP_DECAL_FRAGMENT_HANDLE_INVALID;
	pDispDecal->m_nVerts = 0;
	pDispDecal->m_nTris = 0;

	// Setup a basis for it.
	CDecalVert *pOutVerts = NULL;
	R_SetupDecalClip( 
		pOutVerts,
		pDispDecal->m_pDecal,
		MSurf_Plane( m_ParentSurfID ).normal,
		pDispDecal->m_pDecal->material,
		pDispDecal->m_TextureSpaceBasis, 
		pDispDecal->m_DecalWorldScale );

	// Recurse and precalculate which nodes this thing can touch.
	SetupDecalNodeIntersect( 
		m_pPowerInfo->m_RootNode, 
		0,			// node bit index into CDispDecal::m_NodeIntersects
		pDispDecal, 
		0 );

	return h;
}

void CDispInfo::NotifyRemoveDecal( DispDecalHandle_t h )
{
	// Any fragments we got we don't need
	ClearDecalFragments(h);

	// Reset the head of the list
	if (m_FirstDecal == h)
		m_FirstDecal = s_DispDecals.Next(h);

	// Blow away the decal
	s_DispDecals.Free( h );
}


//-----------------------------------------------------------------------------
// Allocates fragments...
//-----------------------------------------------------------------------------
CDispShadowFragment* CDispInfo::AllocateShadowDecalFragment( DispShadowHandle_t h )
{
	DispShadowFragmentHandle_t f = s_DispShadowFragments.Alloc(true);
	s_DispShadowFragments.LinkBefore( s_DispShadowDecals[h].m_FirstFragment, f ); 
	s_DispShadowDecals[h].m_FirstFragment = f;
	return &s_DispShadowFragments[f];
}


//-----------------------------------------------------------------------------
// Clears shadow decal fragment lists
//-----------------------------------------------------------------------------
void CDispInfo::ClearShadowDecalFragments( DispShadowHandle_t h )
{
	// Iterate over all fragments associated with each shadow decal
	CDispShadowDecal& decal = s_DispShadowDecals[h];
	DispShadowFragmentHandle_t f = decal.m_FirstFragment;
	DispShadowFragmentHandle_t next;
	while( f != DISP_SHADOW_FRAGMENT_HANDLE_INVALID )
	{
		next = s_DispShadowFragments.Next(f);
		s_DispShadowFragments.Free(f);
		f = next;
	}

	// Blat out the list
	decal.m_FirstFragment = DISP_SHADOW_FRAGMENT_HANDLE_INVALID;

	// Mark is as not computed
	decal.m_Flags &= ~CDispDecalBase::FRAGMENTS_COMPUTED;

	// Update the number of triangles in the decal
	decal.m_nTris = 0;
	decal.m_nVerts = 0;
}

void CDispInfo::ClearAllShadowDecalFragments()
{
	// Iterate over all shadow decals on the displacement
	DispShadowHandle_t h = m_FirstShadowDecal;
	while( h != DISP_SHADOW_HANDLE_INVALID )
	{
		ClearShadowDecalFragments( h );
		h = s_DispShadowDecals.Next(h);
	}
}


// ----------------------------------------------------------------------------- //
// Add/remove shadow decals
// ----------------------------------------------------------------------------- //
DispShadowHandle_t CDispInfo::AddShadowDecal( ShadowHandle_t shadowHandle )
{
	// Create a new shadow decal, link it in
	DispShadowHandle_t h = s_DispShadowDecals.Alloc( true );
	s_DispShadowDecals.LinkBefore( m_FirstShadowDecal, h );
	m_FirstShadowDecal = h;

	CDispShadowDecal* pShadowDecal = &s_DispShadowDecals[h];
	pShadowDecal->m_nTris = 0;
	pShadowDecal->m_nVerts = 0;
	pShadowDecal->m_Shadow = shadowHandle;
	pShadowDecal->m_FirstFragment = DISP_SHADOW_FRAGMENT_HANDLE_INVALID;

	return h;
}

void CDispInfo::RemoveShadowDecal( DispShadowHandle_t h )
{
	// Any fragments we got we don't need
	ClearShadowDecalFragments(h);

	// Reset the head of the list
	if (m_FirstShadowDecal == h)
		m_FirstShadowDecal = s_DispShadowDecals.Next(h);

	// Blow away the decal
	s_DispShadowDecals.Free( h );
}


// ----------------------------------------------------------------------------- //
// This little beastie generate decal fragments
// ----------------------------------------------------------------------------- //
void CDispInfo::GenerateDecalFragments_R( CVertIndex const &nodeIndex, 
	int iNodeBitIndex, unsigned short decalHandle, CDispDecalBase *pDispDecal, int iLevel )
{
	// Get the node info for this node...
	Assert( iNodeBitIndex < m_pPowerInfo->m_NodeCount );
	DispNodeInfo_t const& nodeInfo = m_pNodeInfo[iNodeBitIndex];

	int iNodeIndex = VertIndex( nodeIndex );
	
	// Don't bother adding decals if the node doesn't have decal info.
	if( !pDispDecal->m_NodeIntersect.Get( iNodeBitIndex ) )
		return;

	// Recurse into child nodes, but only if they have triangles.
	if ( ( iLevel+1 < m_Power ) && (nodeInfo.m_Flags & DispNodeInfo_t::CHILDREN_HAVE_TRIANGLES) )
	{
		int iChildNodeBit = iNodeBitIndex + 1;
		for( int iChild=0; iChild < 4; iChild++ )
		{
			CVertIndex const &childNode = m_pPowerInfo->m_pChildVerts[iNodeIndex].m_Verts[iChild];

			bool bActiveChild = m_ActiveVerts.Get( VertIndex( childNode ) ) != 0;
			if ( bActiveChild )
				GenerateDecalFragments_R( childNode, iChildNodeBit, decalHandle, pDispDecal, iLevel + 1 );

			iChildNodeBit += m_pPowerInfo->m_NodeIndexIncrements[iLevel];
		}
	}

	// Create the decal fragments on the node triangles
	bool isShadow = (pDispDecal->m_Flags & CDispDecalBase::DECAL_SHADOW) != 0;

	int index = nodeInfo.m_FirstTesselationIndex;
	for ( int i = 0; i < nodeInfo.m_Count; i += 3 )
	{
		if (isShadow)
			TestAddDecalTri( &m_Indices[index + i], decalHandle, static_cast<CDispShadowDecal*>(pDispDecal) );
		else
			TestAddDecalTri( &m_Indices[index + i], decalHandle, static_cast<CDispDecal*>(pDispDecal) );
	}
}

void CDispInfo::GenerateDecalFragments( CVertIndex const &nodeIndex, 
	int iNodeBitIndex, unsigned short decalHandle, CDispDecalBase *pDispDecal )
{
	GenerateDecalFragments_R( nodeIndex, iNodeBitIndex, decalHandle, pDispDecal, 0 );
	pDispDecal->m_Flags |= CDispDecalBase::FRAGMENTS_COMPUTED;
}



// ----------------------------------------------------------------------------- //
// Compute shadow fragments for a particular shadow
// ----------------------------------------------------------------------------- //
bool CDispInfo::ComputeShadowFragments( DispShadowHandle_t h, int& vertexCount, int& indexCount )
{
	if ( !r_DrawDisp.GetBool() )
	{
		vertexCount	= 0;
		indexCount = 0;
		return true;
	}

	CDispShadowDecal* pShadowDecal = &s_DispShadowDecals[h];

	// If we already have fragments, that means the data's already cached.
	if ((pShadowDecal->m_Flags & CDispDecalBase::FRAGMENTS_COMPUTED) != 0)
	{
		vertexCount = pShadowDecal->m_nVerts;
		indexCount = 3 * pShadowDecal->m_nTris;
		return true;
	}

	// Check to see if the bitfield wasn't computed. If so, compute it.
	// This should happen whenever the shadow moves
#ifndef SWDS
	if ((pShadowDecal->m_Flags & CDispDecalBase::NODE_BITFIELD_COMPUTED ) == 0)
	{
		// First, determine the nodes that the shadow decal should affect		
		ShadowInfo_t const& info = g_pShadowMgr->GetInfo( pShadowDecal->m_Shadow );
		SetupDecalNodeIntersect( 
			m_pPowerInfo->m_RootNode, 
			0,			// node bit index into CDispDecal::m_NodeIntersects
			pShadowDecal, 
			&info );
	}
#endif

	// Check to see if there are any bits set in the bitfield, If not,
	// this displacement should be taken out of the list of potential displacements
	if (pShadowDecal->m_Flags & CDispDecalBase::NO_INTERSECTION)
		return false;

	// Now that we have the bitfield, compute the fragments
	// It may happen that we have invalid fragments but valid bitfield.
	// This can happen when a retesselation occurs
	Assert( pShadowDecal->m_nTris == 0);
	Assert( pShadowDecal->m_FirstFragment == DISP_SHADOW_FRAGMENT_HANDLE_INVALID);

	GenerateDecalFragments( m_pPowerInfo->m_RootNode, 0, h, pShadowDecal );

	// Compute the index + vertex counts
	vertexCount = pShadowDecal->m_nVerts;
	indexCount = 3 * pShadowDecal->m_nTris;

	return true;
}


//-----------------------------------------------------------------------------
// Generate vertex lists
//-----------------------------------------------------------------------------

bool CDispInfo::GetTag()
{
	return m_Tag == m_pDispArray->m_CurTag;
}


void CDispInfo::SetTag()
{
	m_Tag = m_pDispArray->m_CurTag;
}


CDispLeafLink*& CDispInfo::GetLeafLinkHead()
{
	return m_pLeafLinkHead;
}


// ----------------------------------------------------------------------------- //
// Helpers for global functions.
// ----------------------------------------------------------------------------- //

// This function crashes in release without this pragma.
#pragma optimize( "ag", off )

// Goes through the list and updates the active vertices on any displacements it has to.
void DispInfo_UpdateDisplacements(
	IDispInfo *pHead,
	CDispInfo *retesselate[MAX_RETESSELATE],
	int &nRetesselate )
{
#ifndef SWDS
	VPROF("DispInfo_UpdateDisplacements");

	// Precalculate this.
	g_flDispToleranceSqr = Square( r_DispTolerance.GetFloat() );

	float dispRadius = CDispInfo::g_flDispRadius;
	float dispRadiusTol = dispRadius * 0.2f;

	float dispRadiusSquared = Square( dispRadius );

	float dispFullRadius = r_DispFullRadius.GetFloat();

	// Process r_DispUpdateAll. 
	// If 1, then update them all only this frame.
	// If 2, update them all each frame.
	bool bUpdateAll = false;
	if( r_DispUpdateAll.GetInt() )
	{
		bUpdateAll = true;
		if( r_DispUpdateAll.GetInt() == 1 )
			r_DispUpdateAll.SetValue( "0" );
	}

	// The LOD is being forced to a specific level if this is set.
	bool bForcedLOD = ( r_DispSetLOD.GetInt() != 0 );

	// Iterate over all the displacements and update some of them.
	int nUpdated = 0;
	Vector const &vViewOrigin = g_EngineRenderer->UnreflectedViewOrigin();

	for( IDispInfo *pCur=pHead; pCur; pCur = pCur->GetNextInRenderChain() )
	{
		CDispInfo *pDisp = static_cast<CDispInfo*>( pCur );

		// If it got marked for retesselation (maybe new decals are on it), then 
		// add it to the list of ones to retesselate.
		if( pDisp->m_bForceRetesselate )
		{
			pDisp->MarkForRetesselation( retesselate, nRetesselate );
			pDisp->m_bForceRetesselate = false;
		}

		bool bWithinFullRadius = (vViewOrigin - pDisp->m_vCenter).LengthSqr() < Square(pDisp->m_flRadius + dispFullRadius );
		float distanceToCenterSquared = (vViewOrigin - pDisp->m_ViewerSphereCenter).LengthSqr();
		bool bUpdateThisDisp = bUpdateAll || pDisp->m_bForceRebuild;
		
		// Don't update LOD more than once per frame unless we're being forced to.
		if ( pDisp->m_LastUpdateFrameCount != r_framecount )
		{
			// If they're within the fully-tesselated radius and it isn't currently
			// fully tesselated, then do it.
			if( !bUpdateThisDisp )
			{
				if( bWithinFullRadius && !pDisp->m_bFullyTesselated && !bForcedLOD )
					bUpdateThisDisp = true;
			}

			// If they moved outside of the sphere it was tesselated for (and can see
			// triangles that were backface-removed), redo it.
			if( !bUpdateThisDisp )
			{
				if( r_DispEnableLOD.GetInt() && distanceToCenterSquared > dispRadiusSquared && !bForcedLOD )
					bUpdateThisDisp = true;
			}
		}

		if( bUpdateThisDisp )
		{
			pDisp->UpdateLOD( 
				( bWithinFullRadius || !r_DispEnableLOD.GetBool() ), 
				g_EngineRenderer->ViewMatrix(), 
				vViewOrigin,
				vViewOrigin + RandomVector( -dispRadiusTol, dispRadiusTol ),
				retesselate, 
				nRetesselate );

			++nUpdated;
		}
		pDisp->m_bForceRebuild = false;
	}

#endif
}

#pragma optimize( "", on )

void DispInfo_BuildPrimLists( 
	IDispInfo *pHead, 
	CDispInfo *visibleDisps[MAX_MAP_DISPINFO],
	int &nVisibleDisps )
{
	VPROF("DispInfo_BuildPrimLists");

	nVisibleDisps = 0;
	
	for( IDispInfo *pCur=pHead; pCur; pCur = pCur->GetNextInRenderChain() )
	{
		CDispInfo *pDisp = static_cast<CDispInfo*>( pCur );
		
		if( pDisp->Render( pDisp->m_pMesh ) )
		{
			// Add it to the list of visible displacements.
			if( nVisibleDisps < MAX_MAP_DISPINFO )
			{
				visibleDisps[nVisibleDisps++] = pDisp;
			}

#ifndef SWDS
			int nSurfID = pDisp->GetParent();
			OverlayMgr()->AddFragmentListToRenderList( MSurf_OverlayFragmentList( nSurfID ) );
#endif
		}

//		g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_DISP_TRIANGLES, 1 );	
	}
}

ConVar disp_numiterations( "disp_numiterations", "1" );
ConVar disp_dynamic( "disp_dynamic", "0" );

void DispInfo_DrawPrimLists()
{
#ifndef SWDS
	VPROF("DispInfo_DrawPrimLists");

	int nDispGroupsSize = g_DispGroups.Size();

	for( int iGroup=0; iGroup < nDispGroupsSize; iGroup++ )
	{
		CDispGroup *pGroup = g_DispGroups[iGroup];
		if( pGroup->m_nVisible == 0 )
			continue;
	
		int numPasses = pGroup->m_pMaterial->GetNumPasses();
		materialSystemInterface->Bind( pGroup->m_pMaterial );
		materialSystemInterface->BindLightmapPage( pGroup->m_LightmapPageID );

		int nMeshesSize = pGroup->m_Meshes.Size();

		if ( r_DispUseStaticMeshes.GetInt() )
		{
			for( int iMesh=0; iMesh < nMeshesSize; iMesh++ )
			{
				CGroupMesh *pMesh = pGroup->m_Meshes[iMesh];
				if( pMesh->m_nVisible == 0 )
					continue;

				int numTris = 0;
				for ( int iIteration=0; iIteration < disp_numiterations.GetInt(); iIteration++ )
				{
					if ( disp_dynamic.GetInt() )
					{
						for ( int iVisible=0; iVisible < pMesh->m_nVisible; iVisible++ )
						{
							pMesh->m_VisibleDisps[iVisible]->SpecifyDynamicMesh();
						}
					}
					else
					{
						pMesh->m_pMesh->Draw( pMesh->m_Visible.Base(), pMesh->m_nVisible );
					}

					int i;
					for( i = 0; i < pMesh->m_nVisible; i++ )
					{
						numTris += pMesh->m_Visible[i].m_NumIndices / 3;
					}
				}

				g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_DISP_TRIANGLES, numPasses * numTris );
				pMesh->m_nVisible = 0;
			}
		}
		else
		{
			// First, calculate totals.
			int nTotalVerts = 0, nTotalIndices = 0;
			for( int iMesh=0; iMesh < nMeshesSize; iMesh++ )
			{
				CGroupMesh *pMesh = pGroup->m_Meshes[iMesh];
				if( pMesh->m_nVisible == 0 )
					continue;
			
				for ( int iVisible=0; iVisible < pMesh->m_nVisible; iVisible++ )
				{
					nTotalVerts += pMesh->m_VisibleDisps[iVisible]->NumVerts();
					nTotalIndices += pMesh->m_VisibleDisps[iVisible]->m_nIndices;
				}
			}

			// Now lock the dynamic mesh and specify the indices.
			IMesh *pMesh = materialSystemInterface->GetDynamicMesh( true );
			CMeshBuilder builder;
			builder.Begin( pMesh, MATERIAL_TRIANGLES, nTotalVerts, nTotalIndices );

				int indexOffset = 0;

				for( iMesh=0; iMesh < nMeshesSize; iMesh++ )
				{
					CGroupMesh *pMesh = pGroup->m_Meshes[iMesh];
					if( pMesh->m_nVisible == 0 )
						continue;
				
					for ( int iVisible=0; iVisible < pMesh->m_nVisible; iVisible++ )
					{
						CDispInfo *pDisp = pMesh->m_VisibleDisps[iVisible];

						pMesh->m_pMesh->CopyToMeshBuilder( 
							pDisp->m_iVertOffset,
							pDisp->NumVerts(),
							pDisp->m_iIndexOffset,
							pDisp->m_nIndices,
							indexOffset,
							builder );

						indexOffset += pDisp->NumVerts();
					}
					
					pMesh->m_nVisible = 0;
				}

			builder.End( false, true );
		}

		pGroup->m_nVisible = 0;
	}
#endif
}


void DispInfo_DrawDecals( CDispInfo *visibleDisps[MAX_MAP_DISPINFO], int nVisibleDisps )
{
#ifndef SWDS
	VPROF("DispInfo_DrawDecals");
	if( !nVisibleDisps )
		return;

	int nTrisDrawn = 0;

	// FIXME: We should bucket all decals (displacement + otherwise)
	// and sort them by material enum id + lightmap
	// To do this, we need to associate a sort index from 0-n for all
	// decals we've seen this level. Then we add those decals to the
	// appropriate search list, (sorted by lightmap id)?

	for( int i=0; i < nVisibleDisps; i++ )
	{
		CDispInfo *pDisp = visibleDisps[i];

		// Don't bother if there's no decals
		if( pDisp->m_FirstDecal == DISP_DECAL_HANDLE_INVALID )
			continue;

		materialSystemInterface->BindLightmapPage( pDisp->m_pMesh->m_pGroup->m_LightmapPageID );

		// At the moment, all decals in a single displacement are sorted by material
		// so we get a little batching at least
		DispDecalHandle_t d = pDisp->m_FirstDecal;
		while( d != DISP_DECAL_HANDLE_INVALID )
		{
			CDispDecal& decal = s_DispDecals[d];

			// Compute decal fragments if we haven't already....
			if ((decal.m_Flags & CDispDecalBase::FRAGMENTS_COMPUTED) == 0)
			{
				pDisp->GenerateDecalFragments( pDisp->m_pPowerInfo->m_RootNode, 0, d, &decal );
			}

			// Don't draw if there's no triangles
			if (decal.m_nTris == 0)
			{
				d = s_DispDecals.Next(d);
				continue;
			}

			// Now draw all the fragments with this material.
			IMaterial* pMaterial = decal.m_pDecal->material;

			IMesh *pMesh = materialSystemInterface->GetDynamicMesh( true, NULL, NULL, pMaterial );

			CMeshBuilder meshBuilder;
			meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, decal.m_nVerts, decal.m_nTris * 3 );

			int baseIndex = 0;
			DispDecalFragmentHandle_t f = decal.m_FirstFragment;
			while (f != DISP_DECAL_FRAGMENT_HANDLE_INVALID)
			{
				CDispDecalFragment& fragment = s_DispDecalFragments[f];
				int v;
				for ( v = 0; v < fragment.m_nVerts - 2; ++v)
				{
					meshBuilder.Position3fv( fragment.m_pVerts[v].m_vPos.Base() );
					meshBuilder.TexCoord2fv( 0, fragment.m_pVerts[v].m_tCoords.Base() );
					meshBuilder.TexCoord2fv( 1, fragment.m_pVerts[v].m_LMCoords.Base() );
					meshBuilder.AdvanceVertex();

					meshBuilder.Index( baseIndex );
					meshBuilder.AdvanceIndex();
					meshBuilder.Index( v + baseIndex + 1 );
					meshBuilder.AdvanceIndex();
					meshBuilder.Index( v + baseIndex + 2 );
					meshBuilder.AdvanceIndex();
				}

				meshBuilder.Position3fv( fragment.m_pVerts[v].m_vPos.Base() );
				meshBuilder.TexCoord2fv( 0, fragment.m_pVerts[v].m_tCoords.Base() );
				meshBuilder.TexCoord2fv( 1, fragment.m_pVerts[v].m_LMCoords.Base() );
				meshBuilder.AdvanceVertex();

				++v;
				meshBuilder.Position3fv( fragment.m_pVerts[v].m_vPos.Base() );
				meshBuilder.TexCoord2fv( 0, fragment.m_pVerts[v].m_tCoords.Base() );
				meshBuilder.TexCoord2fv( 1, fragment.m_pVerts[v].m_LMCoords.Base() );
				meshBuilder.AdvanceVertex();

				baseIndex += fragment.m_nVerts;

				f = s_DispDecalFragments.Next(f);
			}
			meshBuilder.End( false, true );

			nTrisDrawn += decal.m_nTris * pMaterial->GetNumPasses();

			d = s_DispDecals.Next(d);
		}
	}
	g_EngineStats.IncrementCountedStat( ENGINE_STATS_DECAL_TRIANGLES, nTrisDrawn );
#endif
}

// ----------------------------------------------------------------------------- //
// Adds shadow rendering data to a particular mesh builder
// ----------------------------------------------------------------------------- //
int DispInfo_AddShadowsToMeshBuilder( CMeshBuilder& meshBuilder, 	   
									DispShadowHandle_t h, int baseIndex )
{
#ifndef SWDS
	if ( !r_DrawDisp.GetBool() )
	{
		return baseIndex;
	}

	CDispShadowDecal* pShadowDecal = &s_DispShadowDecals[h];
	int shadow = pShadowDecal->m_Shadow;
	ShadowInfo_t const& info = g_pShadowMgr->GetInfo( shadow );

	// It had better be computed by now...
	Assert( pShadowDecal->m_Flags & CDispDecalBase::FRAGMENTS_COMPUTED );

#ifdef _DEBUG
	int triCount = 0;
	int vertCount = 0;
#endif

	DispShadowFragmentHandle_t f = pShadowDecal->m_FirstFragment;
	while ( f != DISP_SHADOW_FRAGMENT_HANDLE_INVALID )
	{
		CDispShadowFragment& fragment = s_DispShadowFragments[f];
		
		// Add in the vertices + indices, use two loops to minimize tests...
		int i;
		for ( i = 0; i < fragment.m_nVerts - 2; ++i )
		{
			meshBuilder.Position3fv( fragment.m_Verts[i].Base() );

			// Transform + offset the texture coords
			Vector2D texCoord;
			Vector2DMultiply( fragment.m_TCoords[i].AsVector2D(), info.m_TexSize, texCoord );
			texCoord += info.m_TexOrigin;
			meshBuilder.TexCoord2fv( 0, texCoord.Base() );

			unsigned char c = g_pShadowMgr->ComputeDarkness( fragment.m_TCoords[i].z, info );
			meshBuilder.Color4ub( c, c, c, c );

			meshBuilder.AdvanceVertex();

			meshBuilder.Index( baseIndex );
			meshBuilder.AdvanceIndex();
			meshBuilder.Index( i + baseIndex + 1 );
			meshBuilder.AdvanceIndex();
			meshBuilder.Index( i + baseIndex + 2 );
			meshBuilder.AdvanceIndex();

#ifdef _DEBUG
			++triCount;
			++vertCount;
#endif
		}

		for ( ; i < fragment.m_nVerts; ++i )
		{
			meshBuilder.Position3fv( fragment.m_Verts[i].Base() );

			// Transform + offset the texture coords
			Vector2D texCoord;
			Vector2DMultiply( fragment.m_TCoords[i].AsVector2D(), info.m_TexSize, texCoord );
			texCoord += info.m_TexOrigin;
			meshBuilder.TexCoord2fv( 0, texCoord.Base() );

			unsigned char c = g_pShadowMgr->ComputeDarkness( fragment.m_TCoords[i].z, info );
			meshBuilder.Color4ub( c, c, c, c );

			meshBuilder.AdvanceVertex();

#ifdef _DEBUG
			++vertCount;
#endif
		}
		
		baseIndex += fragment.m_nVerts;

		f = s_DispShadowFragments.Next(f);
	}

#ifdef _DEBUG
	Assert( triCount == pShadowDecal->m_nTris );
	Assert( vertCount == pShadowDecal->m_nVerts );
#endif

	return baseIndex;
#else
	return 0;
#endif
}


// ----------------------------------------------------------------------------- //
// IDispInfo globals implementation.
// ----------------------------------------------------------------------------- //

void DispInfo_InitMaterialSystem()
{
}


void DispInfo_ShutdownMaterialSystem()
{
}


HDISPINFOARRAY DispInfo_CreateArray( int nElements )
{
	CDispArray *pRet = new CDispArray;

	pRet->m_CurTag = 1;

	pRet->m_nDispInfos = nElements;
	if ( nElements )
	{
		pRet->m_pDispInfos = new CDispInfo[nElements];
	}
	else
	{
		pRet->m_pDispInfos = NULL;
	}
	for( int i=0; i < nElements; i++ )
		pRet->m_pDispInfos[i].m_pDispArray = pRet;

	return (HDISPINFOARRAY)pRet;
}


void DispInfo_DeleteArray( HDISPINFOARRAY hArray )
{
	CDispArray *pArray = static_cast<CDispArray*>( hArray );
	if( !pArray )
		return;

	delete [] pArray->m_pDispInfos;
	delete pArray;
}


IDispInfo* DispInfo_IndexArray( HDISPINFOARRAY hArray, int iElement )
{
	CDispArray *pArray = static_cast<CDispArray*>( hArray );
	if( !pArray )
		return NULL;

	Assert( iElement >= 0 && iElement < pArray->m_nDispInfos );
	return &pArray->m_pDispInfos[iElement];
}

int DispInfo_ComputeIndex( HDISPINFOARRAY hArray, IDispInfo* pInfo )
{
	CDispArray *pArray = static_cast<CDispArray*>( hArray );
	if( !pArray )
		return NULL;

	int iElement = ((int)pInfo - (int)(pArray->m_pDispInfos)) / sizeof(CDispInfo);

	Assert( iElement >= 0 && iElement < pArray->m_nDispInfos );
	return iElement;
}

void DispInfo_ClearAllTags( HDISPINFOARRAY hArray )
{
	CDispArray *pArray = static_cast<CDispArray*>( hArray );
	if( !pArray )
		return;

	++pArray->m_CurTag;
	if( pArray->m_CurTag == 0xFFFF )
	{
		// Reset all the tags.
		pArray->m_CurTag = 1;
		for( int i=0; i < pArray->m_nDispInfos; i++ )
			pArray->m_pDispInfos[i].m_Tag = 0;
	}
}


//-----------------------------------------------------------------------------
// Renders normals for the displacements
//-----------------------------------------------------------------------------

static void DispInfo_DrawChainNormals( IDispInfo *pHead )
{
#ifndef SWDS
#ifdef _DEBUG
	// Only do it in debug because we're only storing the info then
	Vector p;

	materialSystemInterface->Bind( g_pMaterialWireframeVertexColor );

	for( IDispInfo *pCur=pHead; pCur; pCur = pCur->GetNextInRenderChain() )
	{
		CDispInfo *pDisp = static_cast<CDispInfo*>( pCur );
		
		int nVerts = pDisp->NumVerts();

		IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_LINES, nVerts * 3 );

		for( int iVert=0; iVert < nVerts; iVert++ )
		{
			CDispRenderVert* pVert = pDisp->GetVertex(iVert);
			meshBuilder.Position3fv( pVert->m_vPos.Base() );
			meshBuilder.Color3ub( 255, 0, 0 );
			meshBuilder.AdvanceVertex();

			VectorMA( pVert->m_vPos, 5.0f, pVert->m_vNormal, p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 255, 0, 0 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pVert->m_vPos.Base() );
			meshBuilder.Color3ub( 0, 255, 0 );
			meshBuilder.AdvanceVertex();

			VectorMA( pVert->m_vPos, 5.0f, pVert->m_vSVector, p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 0, 255, 0 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pVert->m_vPos.Base() );
			meshBuilder.Color3ub( 0, 0, 255 );
			meshBuilder.AdvanceVertex();

			VectorMA( pVert->m_vPos, 5.0f, pVert->m_vTVector, p );
			meshBuilder.Position3fv( p.Base() );
			meshBuilder.Color3ub( 0, 0, 255 );
			meshBuilder.AdvanceVertex();
		}

		meshBuilder.End();
		pMesh->Draw();
	}
#endif
#endif
}


//-----------------------------------------------------------------------------
// Renders debugging information for displacements 
//-----------------------------------------------------------------------------

static void DispInfo_DrawDebugInformation( IDispInfo *pHead )
{
#ifndef SWDS
	VPROF("DispInfo_DrawDebugInformation");
	// Overlay with normals if we're in that mode
	if( mat_normals.GetInt() )
	{
		DispInfo_DrawChainNormals(pHead);
	}
#endif
}


//-----------------------------------------------------------------------------
// Renders all displacements in sorted order 
//-----------------------------------------------------------------------------
void DispInfo_RenderChain( IDispInfo *pHead, bool bOrtho )
{
#ifndef SWDS
	if( !r_DrawDisp.GetInt() || !pHead )
		return;

	// Cache this off only once per frame
	CDispInfo::g_flDispRadius = r_DispRadius.GetFloat();

	g_bDispOrthoRender = bOrtho;

	// The list of dispinfos that need to be retesselated.
	CDispInfo *retesselate[MAX_RETESSELATE];
	int nRetesselate = 0;

	if( !r_DispLockLOD.GetInt() )
	{
		DispInfo_UpdateDisplacements( pHead, retesselate, nRetesselate );
	}

	{
		// Kept outside to keep it from becoming insane.
		VPROF("DispInfo_RenderChain.TesselateDisplacement()");
		// Retesselate dispinfos that need to be.
		for( int iRetesselate=0; iRetesselate < nRetesselate; iRetesselate++ )
		{
			retesselate[iRetesselate]->TesselateDisplacement();
		}
	}

	// Build up the CPrimLists for all the displacements.
	CDispInfo *visibleDisps[MAX_MAP_DISPINFO];
	int nVisibleDisps;

	DispInfo_BuildPrimLists( pHead, visibleDisps, nVisibleDisps );

	// Draw..
	DispInfo_DrawPrimLists();

	// Debugging rendering..
	DispInfo_DrawDebugInformation(pHead);

	// FIXME: Move these two calls into a separate function
	// so that all overlays can be rendered, followed by all decals.

	// Overlays
	OverlayMgr()->RenderOverlays( );

	// Decals
	DispInfo_DrawDecals( visibleDisps, nVisibleDisps );
#endif
}

