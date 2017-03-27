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
#include "iscratchpad3d.h"
#include "decal_private.h"
#include "con_nprint.h"
#include "dispcoll_common.h"
#include "cmodel_private.h"
#include "collisionutils.h"
#include "tier0/dbg.h"
#include "gl_rmain.h"
#include "lightcache.h"
#include "r_local.h"

Vector modelorg;
//
// view origin
//
Vector	vup(0,0,0);
Vector	vpn(0,0,0);
Vector	vright(0,0,0);


// ------------------------------------------------------------------------------------ //
// Globals.
// ------------------------------------------------------------------------------------ //

ConVar r_DispDrawAxes( "r_DispDrawAxes", "0" );
ConVar r_DispSetLOD( "r_DispSetLOD", "0" );

VMatrix	g_DispViewMatrix;
float	g_flMaxError;

// The mesh that we specify indices into while tesselating.
static CMeshBuilder *g_pIndexMesh;

// The dispinfo currently in UpdateLOD.
CDispInfo	*g_pOriginatingDisp;

// Static used once per frame
float CDispInfo::g_flDispRadius = 0.0f;

//-----------------------------------------------------------------------------
// CDispInfo implementation.
//-----------------------------------------------------------------------------

inline CVertIndex CDispInfo::IndexToVert( int index ) const
{
	if( index == -1 )
		return CVertIndex( -1, -1 );
	else
		return CVertIndex( index % GetSideLength(), index / GetSideLength() );
}


void CDispInfo::UpdateBoundingBox()
{
	m_BBoxMin.Init( 1e24, 1e24, 1e24 );
	m_BBoxMax.Init( -1e24, -1e24, -1e24 );

	for( int i=0; i < NumVerts(); i++ )
	{
		VectorMin( m_Verts[i].m_vPos, m_BBoxMin, m_BBoxMin );
		VectorMax( m_Verts[i].m_vPos, m_BBoxMax, m_BBoxMax );
	}

	UpdateCenterAndRadius();
}


void CDispInfo::UpdateCenterAndRadius()
{
	m_vCenter = (m_BBoxMin + m_BBoxMax) * 0.5f;
	m_flRadius = (m_BBoxMax - m_BBoxMin).Length() * 0.5f;
}


bool CDispInfo::IsInsideFrustum()
{
	if( R_CullBox(m_BBoxMin, m_BBoxMax) )
	{
		return false;
	}

	cplane_t nearPlane;
	VectorCopy(vpn, nearPlane.normal);
	nearPlane.dist = DotProduct(vpn, modelorg);
	nearPlane.type = PLANE_ANYZ;
	nearPlane.signbits = SignbitsForPlane (&nearPlane);

	if( BoxOnPlaneSide(m_BBoxMin, m_BBoxMax, &nearPlane) == 2 )
	{
		return false;
	}

	return true;
}


bool CDispInfo::IsTriangleFrontfacing( unsigned short const *indices )
{
	if( g_bDispOrthoRender )
		return true;

	Vector const &v1 = m_Verts[indices[0]].m_vPos;
	Vector const &v2 = m_Verts[indices[1]].m_vPos;
	Vector const &v3 = m_Verts[indices[2]].m_vPos;

	Vector vNormal = ( v3 - v1 ).Cross( v2 - v1 );
	VectorNormalize( vNormal );
	float flDist = vNormal.Dot( v1 );

	return (vNormal.Dot( m_ViewerSphereCenter ) - flDist) > -CDispInfo::g_flDispRadius;
}


inline void CDispInfo::DecalProjectVert( Vector const &vPos, CDispDecalBase *pDecalBase, ShadowInfo_t const* pInfo, Vector &out )
{
	if (!pInfo)
	{
		CDispDecal* pDispDecal = static_cast<CDispDecal*>(pDecalBase);
		out.x = vPos.Dot( pDispDecal->m_TextureSpaceBasis[0] ) - pDispDecal->m_pDecal->dx + .5f;
		out.y = vPos.Dot( pDispDecal->m_TextureSpaceBasis[1] ) - pDispDecal->m_pDecal->dy + .5f;
		out.z = 0;
	}
	else
	{
		// FIXME: Change this multiplication if we do an actual projection
		Vector3DMultiplyPosition( pInfo->m_WorldToShadow, vPos, out );
	}
}


// ----------------------------------------------------------------------------- //
// This version works for normal decals
// ----------------------------------------------------------------------------- //
void CDispInfo::TestAddDecalTri( unsigned short *tempIndices, unsigned short decalHandle, CDispDecal *pDispDecal )
{
	decal_t *pDecal = pDispDecal->m_pDecal;

	// Setup verts.
	CDecalVert verts[3];
	int iVert;
	for( iVert=0; iVert < 3; iVert++ )
	{
		CDecalVert *pOutVert = &verts[iVert];
		CDispRenderVert *pInVert = &m_Verts[tempIndices[iVert]];
		
		pOutVert->m_vPos = pInVert->m_vPos;
		pOutVert->m_LMCoords = pInVert->m_LMCoords;

		// garymcthack - what about m_ParentTexCoords?
		Vector tmp;
		DecalProjectVert( pOutVert->m_vPos, pDispDecal, 0, tmp );
		Vector2DCopy( tmp.AsVector2D(), pOutVert->m_tCoords );
	}

	// Clip them.
	int outCount;
	CDecalVert *pClipped;

	CDecalVert *pOutVerts = NULL;
	pClipped = R_DoDecalSHClip( &verts[0], pOutVerts, pDecal, 3, &outCount );
	if ( outCount > 2 )
	{
		outCount = min( outCount, CDispDecalFragment::MAX_VERTS );

		// Allocate a new fragment...
		CDispDecalFragment* pFragment = AllocateDispDecalFragment( decalHandle );

		// Alrighty, store the triangles!
		for( iVert=0; iVert < outCount; iVert++ )
		{
			pFragment->m_pVerts[iVert].m_vPos = pClipped[iVert].m_vPos;
			// garymcthack - need to make this work for displacements
			//				pFragment->m_tCoords[iVert] = pClipped[iVert].m_tCoords;
			// garymcthack - need to change m_TCoords to m_ParentTexCoords
			pFragment->m_pVerts[iVert].m_tCoords = pClipped[iVert].m_tCoords;
			pFragment->m_pVerts[iVert].m_LMCoords = pClipped[iVert].m_LMCoords;
		}

		pFragment->m_pDecal = pDecal;
		pFragment->m_nVerts = outCount;
		pDispDecal->m_nVerts += pFragment->m_nVerts;
		pDispDecal->m_nTris += pFragment->m_nVerts - 2;
	}
}


// ----------------------------------------------------------------------------- //
// This version works for shadow decals
// ----------------------------------------------------------------------------- //
void CDispInfo::TestAddDecalTri( unsigned short *tempIndices, unsigned short decalHandle, CDispShadowDecal *pDecal )
{
#ifndef SWDS
	// Setup verts.
	Vector* ppPosition[3];
	ppPosition[0] = &m_Verts[tempIndices[0]].m_vPos;
	ppPosition[1] = &m_Verts[tempIndices[1]].m_vPos;
	ppPosition[2] = &m_Verts[tempIndices[2]].m_vPos;

	ShadowVertex_t** ppClipVertex;
	int count = g_pShadowMgr->ProjectAndClipVertices( pDecal->m_Shadow, 3, ppPosition, &ppClipVertex );
	if (count < 3)
		return;

	// Ok, clipping happened; lets create a decal fragment.
	Assert( count <= CDispShadowFragment::MAX_VERTS );

	// Allocate a new fragment...
	CDispShadowFragment* pFragment = AllocateShadowDecalFragment( decalHandle );

	// Copy the fragment data in place
	pFragment->m_nVerts = count;
	for (int i = 0; i < count; ++i )
	{
		VectorCopy( ppClipVertex[i]->m_Position, pFragment->m_Verts[i] );
		VectorCopy( ppClipVertex[i]->m_TexCoord, pFragment->m_TCoords[i] );

		// Make sure it's been clipped
		Assert( pFragment->m_TCoords[i][0] >= -1e-3f );
		Assert( pFragment->m_TCoords[i][0] - 1.0f <= 1e-3f );
		Assert( pFragment->m_TCoords[i][1] >= -1e-3f );
		Assert( pFragment->m_TCoords[i][1] - 1.0f <= 1e-3f );
	}

	// Update the number of triangles in the decal
	pDecal->m_nVerts += pFragment->m_nVerts;
	pDecal->m_nTris += pFragment->m_nVerts - 2;
	Assert( pDecal->m_nTris != 0 );
#endif
}


inline void CDispInfo::EndTriangle( CVertIndex const &nodeIndex, 
							unsigned short *tempIndices, int &iCurTriVert )
{
	// End our current triangle here.
	if( iCurTriVert )
	{
		Assert( iCurTriVert == 2 );
		
		// Finish the triangle.
		tempIndices[2] = (unsigned short)VertIndex( nodeIndex ); 

		// If r_DispEnableLOD is false, then we want to put all triangles in here.
		if( r_DispEnableLOD.GetInt() == 0 || IsTriangleFrontfacing( tempIndices ) )
		{
			int iVertOffset = 0;
			if ( r_DispUseStaticMeshes.GetInt() )
				iVertOffset = m_iVertOffset;

			// Add this tri to our mesh.
			g_pIndexMesh->Index( tempIndices[0] + iVertOffset );
			g_pIndexMesh->AdvanceIndex();

			g_pIndexMesh->Index( tempIndices[1] + iVertOffset );
			g_pIndexMesh->AdvanceIndex();

			g_pIndexMesh->Index( tempIndices[2] + iVertOffset );
			g_pIndexMesh->AdvanceIndex();

			// Store off the indices...
			m_Indices[m_nIndices]	= tempIndices[0];
			m_Indices[m_nIndices+1] = tempIndices[1];
			m_Indices[m_nIndices+2] = tempIndices[2];

			m_nIndices += 3;
		}

		// Add on the last vertex to join to the next triangle.
		tempIndices[0] = tempIndices[1];
		iCurTriVert = 1;
	}
}


void CDispInfo::CullDecals( 
	int iNodeBit,
	CDispDecal **decals, 
	int nDecals, 
	CDispDecal **childDecals, 
	int &nChildDecals )
{
	// Only let the decals through that can affect this node or its children.
	nChildDecals = 0;
	for( int iDecal=0; iDecal < nDecals; iDecal++ )
	{
		if( decals[iDecal]->m_NodeIntersect.Get( iNodeBit ) )
		{
			childDecals[nChildDecals] = decals[iDecal];
			++nChildDecals;
		}
	}
}
	

//-----------------------------------------------------------------------------
// Tesselates a single node, doesn't deal with hierarchy
//-----------------------------------------------------------------------------
void CDispInfo::TesselateDisplacementNode( unsigned short *tempIndices, 
			CVertIndex const &nodeIndex, int iLevel, int* pActiveChildren )
{
	int iPower = m_Power - iLevel;
	int vertInc = 1 << (iPower - 1);

	CTesselateWinding *pWinding = &g_TWinding;

	// Starting at the bottom-left, wind clockwise picking up vertices and
	// generating triangles.
	int iCurTriVert = 0;
	for( int iVert=0; iVert < pWinding->m_nVerts; iVert++ )
	{
		CVertIndex sideVert = BuildOffsetVertIndex( nodeIndex, pWinding->m_Verts[iVert].m_Index, vertInc );
		
		int iVertNode = pWinding->m_Verts[iVert].m_iNode;
		bool bNode = (iVertNode != -1) && pActiveChildren[iVertNode];
		if( bNode )
		{
			if( iCurTriVert == 2 )
				EndTriangle( nodeIndex, tempIndices, iCurTriVert );
			
			iCurTriVert = 0;
		}
		else if( m_ActiveVerts.Get( VertIndex( sideVert ) ) )
		{
			// Ok, add a vert here.
			tempIndices[iCurTriVert] = (unsigned short)VertIndex( sideVert );
			iCurTriVert++;
			if( iCurTriVert == 2 )
				EndTriangle( nodeIndex, tempIndices, iCurTriVert );
		}
	}
}


//-----------------------------------------------------------------------------
// Tesselates in a *breadth first* fashion
//-----------------------------------------------------------------------------
void CDispInfo::TesselateDisplacement_R(
	unsigned short *tempIndices,
	CVertIndex const &nodeIndex,
	int iNodeBitIndex,
	int iLevel )
{
	// Here's the node info for our current node
	Assert( iNodeBitIndex < m_pPowerInfo->m_NodeCount );
	DispNodeInfo_t& nodeInfo = m_pNodeInfo[iNodeBitIndex];

	// Store off the current number of indices
	int oldIndexCount = m_nIndices;

	// Go through each quadrant. If there is an active child node, recurse down.
	int bActiveChildren[4];
	if( iLevel >= m_Power - 1 )
	{
		// This node has no children.
		bActiveChildren[0] = bActiveChildren[1] = bActiveChildren[2] = bActiveChildren[3] = false;
	}
	else
	{
		int iNodeIndex = VertIndex( nodeIndex );

		int iChildNodeBit = iNodeBitIndex + 1;
		for( int iChild=0; iChild < 4; iChild++ )
		{
			CVertIndex const &childNode = m_pPowerInfo->m_pChildVerts[iNodeIndex].m_Verts[iChild];

			// Make sure we really can tesselate here (a smaller neighbor displacement could
			// have inactivated certain edge verts.
			bActiveChildren[iChild] = m_ActiveVerts.Get( VertIndex( childNode ) );

			if( bActiveChildren[iChild] )
			{
				TesselateDisplacement_R( tempIndices, childNode, iChildNodeBit, iLevel+1 );
			}
			else
			{
				// Make sure the triangle counts are cleared on this one because it may visit this
				// node in GenerateDecalFragments_R if nodeInfo's CHILDREN_HAVE_TRIANGLES flag is set.
				m_pNodeInfo[iChildNodeBit].m_Count = 0;
				m_pNodeInfo[iChildNodeBit].m_Flags = 0;
			}

			iChildNodeBit += m_pPowerInfo->m_NodeIndexIncrements[iLevel];
		}
	}

	// Set the child field
	if ( m_nIndices != oldIndexCount )
	{
		nodeInfo.m_Flags = DispNodeInfo_t::CHILDREN_HAVE_TRIANGLES;
		oldIndexCount = m_nIndices;
	}
	else
	{
		nodeInfo.m_Flags = 0;
	}

	// Now tesselate the node itself...
	TesselateDisplacementNode(tempIndices, nodeIndex, iLevel, bActiveChildren);

	// Now that we've tesselated, figure out how many indices we've added at this node
	nodeInfo.m_Count = m_nIndices - oldIndexCount;
	nodeInfo.m_FirstTesselationIndex = oldIndexCount;
	Assert( nodeInfo.m_Count % 3 == 0 );
}


//-----------------------------------------------------------------------------
// Retesselates a displacement
//-----------------------------------------------------------------------------
void CDispInfo::TesselateDisplacement()
{
	// Clear decals. They get regenerated in TesselateDisplacement_R.
	ClearAllDecalFragments();

	// Blow away cached shadow decals
	ClearAllShadowDecalFragments();

	int nMaxIndices = Square( GetSideLength() - 1 ) * 6;

	CMeshBuilder mb;
	mb.BeginModify( m_pMesh->m_pMesh, 0, 0, m_iIndexOffset, nMaxIndices );
	g_pIndexMesh = &mb;

	// Generate the indices.
	unsigned short tempIndices[6];
	m_nIndices = 0;
	TesselateDisplacement_R(
		tempIndices,
		m_pPowerInfo->m_RootNode,
		0,			// node bit indexing CDispDecal::m_NodeIntersects
		0);

	mb.EndModify();

	// Flag that we have retesselated so we don't keep redoing it.
	m_bRetesselate = false;
}


void CDispInfo::SpecifyDynamicMesh()
{
	// Specify the vertices and indices.
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( true );
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLES, NumVerts(), m_nIndices );

		// This should mirror how FillStaticBuffer works.
		int nVerts = NumVerts();
		for( int iVert=0; iVert < nVerts; iVert++ )
		{
			CDispRenderVert *pVert = &m_Verts[iVert];

			builder.Position3fv( pVert->m_vPos.Base() );

			builder.TexCoord2fv( 0, pVert->m_vTexCoord.Base() );
			builder.TexCoord2fv( 1, pVert->m_LMCoords.Base() );
			builder.TexCoord2f( 2, pVert->m_BumpSTexCoordOffset, 0 );
			
			builder.Normal3fv( pVert->m_vNormal.Base() );
			builder.TangentS3fv( pVert->m_vSVector.Base() );
			builder.TangentT3fv( pVert->m_vTVector.Base() );
			
			builder.AdvanceVertex();
		}

		for( int iIndex=0; iIndex < m_nIndices; iIndex++ )
		{
			builder.Index( m_Indices[iIndex] );
			builder.AdvanceIndex();
		}

	builder.End( false, true );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CDispInfo::SpecifyWalkableDynamicMesh( void )
{
	// Specify the vertices and indices.
#ifdef SWDS
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( false, NULL, NULL, NULL );
#else
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( false, NULL, NULL, g_materialTranslucentSingleColor );
	g_materialTranslucentSingleColor->ColorModulate( 1.0f, 1.0f, 0.0f );
	g_materialTranslucentSingleColor->AlphaModulate( 0.33f );
#endif
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLES, NumVerts(), m_nWalkIndexCount );

		int nVerts = NumVerts();
		for( int iVert=0; iVert < nVerts; iVert++ )
		{
			builder.Position3fv( m_Verts[iVert].m_vPos.Base() );
			builder.AdvanceVertex();
		}
		
		for( int iIndex=0; iIndex < m_nWalkIndexCount; iIndex++ )
		{
			builder.Index( m_pWalkIndices[iIndex] );
			builder.AdvanceIndex();
		}

	builder.End( false, true );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CDispInfo::SpecifyBuildableDynamicMesh( void )
{

	// Specify the vertices and indices.
#ifdef SWDS
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( false, NULL, NULL, NULL );
#else
	g_materialTranslucentSingleColor->ColorModulate( 0.0f, 1.0f, 1.0f );
	g_materialTranslucentSingleColor->AlphaModulate( 0.33f );
	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( false, NULL, NULL, g_materialTranslucentSingleColor );
#endif
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLES, NumVerts(), m_nBuildIndexCount );

		int nVerts = NumVerts();
		for( int iVert=0; iVert < nVerts; iVert++ )
		{
			builder.Position3fv( m_Verts[iVert].m_vPos.Base() );
			builder.AdvanceVertex();
		}

		for( int iIndex=0; iIndex < m_nBuildIndexCount; iIndex++ )
		{
			builder.Index( m_pBuildIndices[iIndex] );
			builder.AdvanceIndex();
		}

	builder.End( false, true );
}

// Measures the screenspace distance from collapseVert to
// the centerpoint of edge [vert1,vert2]. Returns true if the distance
// is larger than the LOD threshold.
bool CDispInfo::MeasureLineError( 
	CVertIndex const &collapseVert ) 
{
	int iVertIndex = VertIndex( collapseVert );
	Vector &vIn1 = m_Verts[iVertIndex].m_vPos;

	unsigned short const *verts = m_pPowerInfo->m_pErrorEdges[iVertIndex].m_Values;
	Vector  vIn2 = (m_Verts[verts[0]].m_vPos + m_Verts[verts[1]].m_vPos) * 0.5f;

	Vector vTestVert = m_vCenter + (vIn2 - vIn1);
    Vector v1;
	g_DispViewMatrix.V3Mul( vTestVert, v1 );

	float invz = 1.0f / v1.z;
	v1.x = v1.x * 640.0f * invz;
	v1.y = v1.y * 480.0f * invz;
	return (v1.x*v1.x) + (v1.y*v1.y) > g_flMaxError;
}	


void CDispInfo::ResolveDependencies_R( CVertIndex const &vertIndex )
{
	int index = VertIndex( vertIndex );

	Assert( m_AllowedVerts.Get( index ) );

	// Is the vertex already here?
	if( m_ActiveVerts.Get( index ) )
		return;

	// There are a few dependency configurations involving MID_TO_CORNER or CORNER_TO_MID
	// displacements that are a real pain to detect ahead of time, so this code adds these
	// dependencies as neighbors at runtime by detecting it here.
	if( !m_bInUse )
	{
		g_pOriginatingDisp->AddExtraDependency( GetDispIndex() );
		return;
	}

	// Mark this vertex as needed.
	m_ActiveVerts.Set( index );
	
	// Activate all the parents.
	CVertIndex const &viParent = m_pPowerInfo->m_pVertInfo[index].m_iParent;
	if( viParent.x != -1 )
		ResolveDependencies_R( viParent );

	// Recursively resolve the dependencies.
	for( int iDependency=0; iDependency < 2; iDependency++ )
	{
		CVertDependency *pDependency = &m_pPowerInfo->m_pVertInfo[index].m_Dependencies[iDependency];
		if( !pDependency->IsValid() )
			continue;

		if( pDependency->m_iNeighbor == -1 )
		{		
			ResolveDependencies_R( pDependency->m_iVert );
		}
		else
		{
			CVertIndex depIndex;

			CDispInfo *pNeighbor = (CDispInfo*)TransformIntoNeighbor(
				this,
				pDependency->m_iNeighbor,
				vertIndex,
				depIndex );

			if( pNeighbor )
				pNeighbor->ResolveDependencies_R( depIndex );
		}
	}		
}	


void CDispInfo::ActivateErrorTermVerts_R( CVertIndex const &nodeIndex, int iLevel )
{
	// Figure out the error for this node's vertex.
	if( !m_ErrorVerts.Get( VertIndex( nodeIndex ) ) )
		return;

	// Flag this node and its vert as active.
	int iNodeIndex = VertIndex( nodeIndex );

	// Now test each neighbor vert to see if it should exist.
	for( int i=0; i < 4; i++ )
	{
		CVertIndex const &sideVert = m_pPowerInfo->m_pSideVerts[iNodeIndex].m_Verts[i];

		if( !m_ErrorVerts.Get( VertIndex( sideVert ) ) )
			continue;

		// Ok, this vertex is needed.. update dependencies.
		ResolveDependencies_R( sideVert );
	}

	// Recurse into child nodes.
	if( iLevel+1 < m_Power )
	{
		for( int iChild = 0; iChild < 4; iChild++ )
		{
			ActivateErrorTermVerts_R( m_pPowerInfo->m_pChildVerts[iNodeIndex].m_Verts[iChild], iLevel + 1 );
		}
	}
	else
	{
		ResolveDependencies_R( nodeIndex );
	}
}


void CDispInfo::ActivateErrorTermVerts()
{
	ActivateErrorTermVerts_R( CVertIndex( GetSideLength()/2, GetSideLength()/2 ), 0 );
}


void CDispInfo::MarkForRetesselation( CDispInfo **retesselate, int &nRetesselate )
{
	if( !m_bRetesselate )
	{
		if( nRetesselate < MAX_RETESSELATE )
		{
			retesselate[nRetesselate] = this;
			++nRetesselate;
		}
		else
		{
			Assert( !"ReconcileEdges: retesselation list maxed out" );
		}

		m_bRetesselate = true;
	}
}

void CDispInfo::ActivateActiveNeighborVerts()
{
	// All surfaces should take care of themselves!
	if ( r_DispSetLOD.GetInt() != 0 )
		return;

	for( int iEdge=0; iEdge < 4; iEdge++ )
	{
		CDispNeighbor *pSide = &m_EdgeNeighbors[iEdge];
		
		for( int iSub=0; iSub < 2; iSub++ )
		{
			CDispSubNeighbor *pSub = &pSide->m_SubNeighbors[iSub];
			CDispInfo *pNeighbor = GetDispByIndex( pSub->m_iNeighbor );
			if( !pNeighbor )
				continue;

			CVertIndex myIndex, nbIndex;
			CVertIndex myInc, nbInc;
			int myEnd, iFreeDim;
			SetupEdgeIncrements( this, iEdge, iSub, myIndex, myInc, nbIndex, nbInc, myEnd, iFreeDim );
			
			while( 1 )
			{
				myIndex += myInc;
				nbIndex += nbInc;
				if( myIndex[iFreeDim] >= myEnd )
					break;
				
				// Process this vertex.
				if( pNeighbor->m_ActiveVerts.Get( pNeighbor->VertIndex( nbIndex ) ) )
				{
					ResolveDependencies_R( myIndex );
				}
			}
		}
	}
}


void CDispInfo::InitializeActiveVerts()
{
	// Mark the corners vertices and root node by default..
	m_ActiveVerts.ClearAll();
	
	m_ActiveVerts.Set( VertIndex( 0, 0 ) );
	m_ActiveVerts.Set( VertIndex( GetSideLength()-1, 0 ) );
	m_ActiveVerts.Set( VertIndex( GetSideLength()-1, GetSideLength()-1 ) );
	m_ActiveVerts.Set( VertIndex( 0, GetSideLength()-1 ) );

	m_ActiveVerts.Set( VertIndex( m_pPowerInfo->m_RootNode ) );

	// Force the midpoint active on any edges where there are sub displacements.
	for( int iSide=0; iSide < 4; iSide++ )
	{
		CDispNeighbor *pSide = &m_EdgeNeighbors[iSide];

		if( (pSide->m_SubNeighbors[0].IsValid() && pSide->m_SubNeighbors[0].m_Span != CORNER_TO_CORNER) ||
			(pSide->m_SubNeighbors[1].IsValid() && pSide->m_SubNeighbors[1].m_Span != CORNER_TO_CORNER) )
		{
			int iEdgeDim = g_EdgeDims[iSide];
			
			CVertIndex nodeIndex;
			nodeIndex[iEdgeDim] = g_EdgeSideLenMul[iSide] * m_pPowerInfo->m_SideLengthM1;
			nodeIndex[!iEdgeDim] = m_pPowerInfo->m_MidPoint;
			m_ActiveVerts.Set( VertIndex( nodeIndex ) );
		}
	}
}


void CDispInfo::ActivateVerts( 
	bool bFullyTesselate, 
	const VMatrix &viewMatrix,
	Vector const &vViewOrigin
	 )
{
	if( bFullyTesselate )
		g_flMaxError = 0.0f;
	else
		g_flMaxError = g_flDispToleranceSqr;


	InitializeActiveVerts();

	int nLOD = r_DispSetLOD.GetInt();
	if ( nLOD != 0 )
	{
		nLOD = clamp( nLOD, 2, m_Power );

		const CPowerInfo *pPowerInfo = GetPowerInfo();

		int sideLength = GetSideLength();
		CVertIndex index;
		for( index.y = 0; index.y < sideLength; index.y++ )
		{
			for( index.x = 0; index.x < sideLength; index.x++ )
			{
				int iVertIndex = VertIndex( index );
				if ( pPowerInfo->m_pVertInfo[iVertIndex].m_iNodeLevel <= nLOD )
				{
					m_ActiveVerts.Set( iVertIndex );
				}
			}
		}

		// Copy off the activated verts into our 'needed verts' list.
		m_ErrorVerts.Copy( m_ActiveVerts );

		return;
	}

	// Build a frame of reference staring at the center.
	Vector vViewDir = m_vCenter - vViewOrigin;
	VectorNormalize( vViewDir );

	Vector vUp(0,0,1);
	Vector vLeft = vUp.Cross( vViewDir );
	vUp = vViewDir.Cross( vLeft );
	VectorNormalize( vViewDir );
	VectorNormalize( vUp );

	g_DispViewMatrix.Init(
		vLeft.x,	vLeft.y,	vLeft.z,	-vLeft.Dot(vViewOrigin),
		vUp.x,		vUp.y,		vUp.z,		-vUp.Dot(vViewOrigin),
		vViewDir.x, vViewDir.y, vViewDir.z, -vViewDir.Dot(vViewOrigin),
		0,			0,			0,			1);

	
	// Test the error on each vert.
	int sideLength = GetSideLength();
	CVertIndex index;
	for( index.y=0; index.y < sideLength; index.y++ )
	{
		for( index.x=0; index.x < sideLength; index.x++ )
		{
			int iVertIndex = VertIndex( index );

			if( m_ActiveVerts.Get( iVertIndex ) || !m_AllowedVerts.Get( iVertIndex ) )
				continue;
		
			if( MeasureLineError( index	) )
			{
				ResolveDependencies_R( index );
			}
		}
	}

	// Copy off the activated verts into our 'needed verts' list.
	m_ErrorVerts.Copy( m_ActiveVerts );
}


void CDispInfo::DebugResolveActiveVerts()
{
#if defined(_DEBUG)
	m_bInUse = true;

	CVertIndex index;
	for( index.y=0; index.y < GetSideLength(); index.y++ )
	{
		for( index.x=0; index.x < GetSideLength(); index.x++ )
		{
			int iVertIndex = VertIndex( index );

			if( !m_ActiveVerts.Get( iVertIndex ) )
				continue;
			
			m_ActiveVerts.Clear( iVertIndex );		
			ResolveDependencies_R( index );
		}
	}

	m_bInUse = false;
#endif
}


void CDispInfo::ClearLOD()
{
	// First, everything as inactive.
	m_ActiveVerts.ClearAll();
	m_ErrorVerts.ClearAll();
}


int CDispInfo::GetDependencyList( unsigned short outList[MAX_TOTAL_DISP_DEPENDENCIES] )
{
	// TODO: precalculate this list!
	int nOut = 0;
	bool bOverflow = false;

	int i;
	for ( i=0; i < 4; i++ )
	{
		int j;
		for ( j=0; j < 2; j++ )
		{
			int index = m_EdgeNeighbors[i].m_SubNeighbors[j].m_iNeighbor;
			if ( index != 0xFFFF )
				outList[nOut++] = index;
		}

		for ( j=0; j < m_CornerNeighbors[i].m_nNeighbors; j++ )
		{
			if ( nOut >= MAX_TOTAL_DISP_DEPENDENCIES )
			{
				bOverflow = true;
			}
			else
			{
				outList[nOut++] = m_CornerNeighbors[i].m_Neighbors[j];
			}
		}
	}

	for ( i=0; i < m_nExtraDependencies; i++ )
	{
		if ( nOut >= MAX_TOTAL_DISP_DEPENDENCIES )
		{
			bOverflow = true;
		}
		else
		{
			outList[nOut++] = m_ExtraDependencies[i];
		}
	}

	if ( bOverflow )
	{
		ExecuteNTimes( 10, Warning( "CDispInfo::GetDependencyList: overflowed" ) );
	}

	return nOut;
}


// We get a crash in UpdateLOD without this pragma.
void CDispInfo::UpdateLOD( 
	bool bFullyTesselate,
	VMatrix const& viewMatrix, 
	Vector const &vViewOrigin,
	Vector const &vViewerSphereCenter,
	CDispInfo **retesselate, 
	int &nRetesselate )
{
	CBitVec<CCoreDispInfo::MAX_VERT_COUNT> neighborStates[MAX_TOTAL_DISP_DEPENDENCIES];

	// Remember that we updated this frame so we don't do it again.
	m_LastUpdateFrameCount = r_framecount;

	// Make a list of dependent displacement.s
	unsigned short dependencies[MAX_TOTAL_DISP_DEPENDENCIES];
	int nDependencies;

	// Loop around rebuilding the dependency list until there are no more new dependencies.
	// Note: 99.9% of the time this will only execute once - new dependencies will only show
	//       up the first few times UpdateLOD is called and very rarely afterwards.
	int nStartDependencies;
	do
	{
		nStartDependencies = m_nExtraDependencies;
		g_pOriginatingDisp = this;
		m_bInUse = true;

		nDependencies = GetDependencyList( dependencies );
		
		// First, clear all active verts in the neighbors.
		for ( int iDependency=0; iDependency < nDependencies; iDependency++ )
		{
			CDispInfo *pNeighbor = GetDispByIndex( dependencies[iDependency] );

			// Store off their active verts.
			neighborStates[iDependency].Copy( pNeighbor->m_ActiveVerts, NumVerts() );

			pNeighbor->InitializeActiveVerts();
			pNeighbor->m_bInUse = true;
		}

		// Now query all the error terms and activate required verts.
		ActivateVerts( bFullyTesselate, viewMatrix, vViewOrigin );

	} while( m_nExtraDependencies != nStartDependencies );


	// Now for each neighbor, activate all its error-term verts and all neighbor-active
	// verts along its edges.
	 int iDependency;
	for ( iDependency=0; iDependency < nDependencies; iDependency++ )
	{
		CDispInfo *pNeighbor = GetDispByIndex( dependencies[iDependency] );

		pNeighbor->ActivateErrorTermVerts();
		pNeighbor->ActivateActiveNeighborVerts();
	}

	
	// Clear the in-use flags and mark changed neighbors for retesselation.
	for ( iDependency=0; iDependency < nDependencies; iDependency++ )
	{
		CDispInfo *pNeighbor = GetDispByIndex( dependencies[iDependency] );

		if( !neighborStates[iDependency++].Compare( pNeighbor->m_ActiveVerts, NumVerts() ) )
			pNeighbor->MarkForRetesselation( retesselate, nRetesselate );

		pNeighbor->m_bInUse = false;
	}

	MarkForRetesselation( retesselate, nRetesselate );
	m_bInUse = false;
	m_bFullyTesselated = bFullyTesselate;
	m_ViewerSphereCenter = vViewerSphereCenter;

	g_pOriginatingDisp = NULL;


	// If new neighbors got added, then redo this process.
	if( m_nExtraDependencies != nStartDependencies )
	{
		UpdateLOD( bFullyTesselate, viewMatrix, vViewOrigin, 
			vViewerSphereCenter, retesselate, nRetesselate );
	}
}

bool CDispInfo::Render( CGroupMesh *pGroup )
{
#ifndef SWDS
	if( !m_pMesh )
	{
		Assert( !"CDispInfo::Render: m_pMesh == NULL" );
		return false;
	}

	// Trivial reject?
	if( !IsInsideFrustum() )
	{
		return false;
	}

	// Wireframe? 
	bool bNormalRender = true;
	if( mat_wireframe.GetInt() )
	{
		materialSystemInterface->Bind( g_materialWireframe );
		SpecifyDynamicMesh();
		bNormalRender = false;
	}
	
	if( mat_luxels.GetInt() )
	{
		materialSystemInterface->Bind( MSurf_TexInfo( m_ParentSurfID )->material );
		//SpecifyDynamicMesh();

		pGroup->m_pMesh->Draw( m_iIndexOffset, m_nIndices );

		materialSystemInterface->Bind( g_materialDebugLuxels );
		SpecifyDynamicMesh();
		bNormalRender = false;
	}

	if ( r_DispWalkable.GetInt() || r_DispBuildable.GetInt() )
	{
		materialSystemInterface->Bind( MSurf_TexInfo( m_ParentSurfID )->material );
		pGroup->m_pMesh->Draw( m_iIndexOffset, m_nIndices );

		if ( r_DispWalkable.GetInt() )
			SpecifyWalkableDynamicMesh();

		if ( r_DispBuildable.GetInt() )
			SpecifyBuildableDynamicMesh();

		bNormalRender = false;
	}

//	g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_DISP_TRIANGLES, m_nIndices / 3 );

	// Mark it visible.
	if( bNormalRender )
	{
		if( pGroup->m_nVisible < pGroup->m_Visible.Size() )
		{
			// Don't bother if all faces are backfacing, or somesuch...
			if (m_nIndices)
			{
				pGroup->m_Visible[pGroup->m_nVisible].m_FirstIndex = m_iIndexOffset;
				pGroup->m_Visible[pGroup->m_nVisible].m_NumIndices = m_nIndices;
				pGroup->m_VisibleDisps[pGroup->m_nVisible] = this;
				pGroup->m_nVisible++;
				pGroup->m_pGroup->m_nVisible++;
			}
		}
		else
		{
			Assert( !"Overflowed visible mesh list" );
		}
	}
#endif

	return true;
}


// This goes with AddSingleDynamicLight..
class CSingleLightAdder
{
public:

	inline void	ProcessLightmapSample( Vector const &vPos, int t, int s, int tmax, int smax )
	{
		float distSqr = m_vLightOrigin.DistToSqr( vPos );
		if( distSqr < m_LightDistSqr )
		{
			float scale = (distSqr != 0.0f) ? m_ooQuadraticAttn / distSqr : 1.0f;

			// Apply a little extra attenuation
			scale *= (1.0f - distSqr * m_ooRadiusSq);

			if (scale > 2.0f)
				scale = 2.0f;

			int index = t*smax + s;
			VectorMA( m_pBlocklight[index].AsVector3D(), scale, m_Intensity, 
				m_pBlocklight[index].AsVector3D() );
		}
	}

	float		m_ooQuadraticAttn;
	float		m_ooRadiusSq;
	Vector4D	*m_pBlocklight;
	Vector		m_Intensity;
	float		m_LightDistSqr;
	Vector		m_vLightOrigin;
};


void CDispInfo::AddSingleDynamicLight( dlight_t& dl, Vector4D *blocklight )
{
#ifndef SWDS
	CSingleLightAdder adder;

	adder.m_LightDistSqr = dl.radius * dl.radius;

	float lightStyleValue = LightStyleValue( dl.style );
	adder.m_Intensity[0] = TexLightToLinear( dl.color.r, dl.color.exponent ) * lightStyleValue;
	adder.m_Intensity[1] = TexLightToLinear( dl.color.g, dl.color.exponent ) * lightStyleValue;
	adder.m_Intensity[2] = TexLightToLinear( dl.color.b, dl.color.exponent ) * lightStyleValue;

	float minlight = max( MIN_LIGHTING_VALUE, dl.minlight );
	float ooQuadraticAttn = adder.m_LightDistSqr * minlight; // / maxIntensity;

	adder.m_ooQuadraticAttn = ooQuadraticAttn;
	adder.m_pBlocklight = blocklight;
	adder.m_vLightOrigin = dl.origin;
	adder.m_ooRadiusSq = 1.0f / (dl.radius * dl.radius);

	// Touch all the lightmap samples.
	IterateLightmapSamples( this, adder );
#endif
}


//-----------------------------------------------------------------------------
// Alpha channel modulation
//-----------------------------------------------------------------------------
class CSingleAlphaLightAdder
{
public:

	inline void	ProcessLightmapSample( Vector const &vPos, int t, int s, int tmax, int smax )
	{
		float distSqr = m_vLightOrigin.DistToSqr( vPos );
		if( distSqr < m_LightDistSqr )
		{
			float scale = (distSqr != 0.0f) ? m_ooQuadraticAttn / distSqr : 1.0f;

			// Apply a little extra attenuation
			scale *= (1.0f - distSqr * m_ooRadiusSq);

			if (scale > 1.0f)
				scale = 1.0f;

			int index = t*smax + s;
			m_pBlocklight[index][3] += scale * m_Intensity;
		}
	}

	float		m_ooQuadraticAttn;
	float		m_ooRadiusSq;
	Vector4D	*m_pBlocklight;
	float		m_Intensity;
	float		m_LightDistSqr;
	Vector		m_vLightOrigin;
};

void CDispInfo::AddSingleDynamicAlphaLight( dlight_t& dl, Vector4D *blocklight )
{
#ifndef SWDS
	CSingleAlphaLightAdder adder;

	adder.m_LightDistSqr = dl.radius * dl.radius;

	float lightStyleValue = LightStyleValue( dl.style );
	adder.m_Intensity = TexLightToLinear( dl.color.r, dl.color.exponent ) * lightStyleValue;
	if ( dl.flags & DLIGHT_SUBTRACT_DISPLACEMENT_ALPHA )
		adder.m_Intensity *= -1.0f;

	float minlight = max( MIN_LIGHTING_VALUE, dl.minlight );
	float ooQuadraticAttn = adder.m_LightDistSqr * minlight; // / maxIntensity;

	adder.m_ooQuadraticAttn = ooQuadraticAttn;
	adder.m_pBlocklight = blocklight;
	adder.m_vLightOrigin = dl.origin;
	adder.m_ooRadiusSq = 1.0f / (dl.radius * dl.radius);

	// Touch all the lightmap samples.
	IterateLightmapSamples( this, adder );
#endif
}



//-----------------------------------------------------------------------------
// A little cache to help us not project vertices multiple times
//-----------------------------------------------------------------------------
class CDecalNodeSetupCache
{
public:
	CDecalNodeSetupCache() : m_CurrentCacheIndex(0) {}

	Vector	m_ProjectedVert[MAX_DISPVERTS];
	int		m_CacheIndex[MAX_DISPVERTS];

	bool IsCached( int v )		{ return m_CacheIndex[v] == m_CurrentCacheIndex; }
	void MarkCached( int v )	{ m_CacheIndex[v] = m_CurrentCacheIndex; }
	
	void ResetCache() { ++m_CurrentCacheIndex; }

private:
	int m_CurrentCacheIndex;
};


//-----------------------------------------------------------------------------
// Check to see which nodes are hit by a decal
//-----------------------------------------------------------------------------
bool CDispInfo::SetupDecalNodeIntersect_R( CVertIndex const &nodeIndex,
	int iNodeBitIndex, CDispDecalBase *pDispDecal, ShadowInfo_t const* pInfo, 
	int iLevel, CDecalNodeSetupCache* pCache )
{	
	int iNodeIndex = VertIndex( nodeIndex );

	if( iLevel+1 < m_Power )
	{
		// Recurse into child nodes.
		bool anyChildIntersected = false;
		int iChildNodeBit = iNodeBitIndex + 1;
		for( int iChild=0; iChild < 4; iChild++ )
		{
			CVertIndex const &childNode = m_pPowerInfo->m_pChildVerts[iNodeIndex].m_Verts[iChild];

			// If any of our children intersect, then we do too...
			if (SetupDecalNodeIntersect_R( childNode, iChildNodeBit, pDispDecal, pInfo, iLevel + 1, pCache ))
				anyChildIntersected = true;
			iChildNodeBit += m_pPowerInfo->m_NodeIndexIncrements[iLevel];
		}

		if (anyChildIntersected)
		{
			pDispDecal->m_NodeIntersect.Set( iNodeBitIndex );
			return true;
		}

		// None of our children intersect this decal, so neither does the node
		return false;
	}

	// Expand our box by the node and by its side verts.
	Vector vMin, vMax;
	if (!pCache->IsCached(iNodeIndex))
	{
		DecalProjectVert( m_Verts[iNodeIndex].m_vPos, pDispDecal, pInfo, pCache->m_ProjectedVert[iNodeIndex] );
		pCache->MarkCached(iNodeIndex);
	}
	vMin = pCache->m_ProjectedVert[iNodeIndex];
	vMax = pCache->m_ProjectedVert[iNodeIndex];

	// Now test each neighbor + child vert to see if it should exist.
	for( int i=0; i < 4; i++ )
	{
		CVertIndex const &sideVert = m_pPowerInfo->m_pSideVerts[iNodeIndex].m_Verts[i];
		CVertIndex const &cornerVert = m_pPowerInfo->m_pSideVertCorners[iNodeIndex].m_Verts[i];

		int iSideIndex = VertIndex(sideVert);
		if (!pCache->IsCached(iSideIndex))
		{
			DecalProjectVert( m_Verts[iSideIndex].m_vPos, pDispDecal, pInfo, pCache->m_ProjectedVert[iSideIndex] );
			pCache->MarkCached(iSideIndex);
		}

		VectorMin( pCache->m_ProjectedVert[iSideIndex], vMin, vMin );
		VectorMax( pCache->m_ProjectedVert[iSideIndex], vMax, vMax );

		int iCornerIndex = VertIndex(cornerVert);
		if (!pCache->IsCached(iCornerIndex))
		{
			DecalProjectVert( m_Verts[iCornerIndex].m_vPos, pDispDecal, pInfo, pCache->m_ProjectedVert[iCornerIndex] );
			pCache->MarkCached(iCornerIndex);
		}

		VectorMin( pCache->m_ProjectedVert[iCornerIndex], vMin, vMin );
		VectorMax( pCache->m_ProjectedVert[iCornerIndex], vMax, vMax );
	}

	// Now just see if our bbox intersects the [0,0] - [1,1] bbox, which is where this
	// decal sits.
	if( vMin.x <= 1 && vMax.x >= 0 && vMin.y <= 1 && vMax.y >= 0 )
	{
		// Z cull for shadows...
		if (pInfo)
		{
			if ((vMax.z < 0) || (vMin.z > pInfo->m_MaxDist))
				return false;
		}

		// Ok, this node is needed and its children may be needed as well.
		pDispDecal->m_NodeIntersect.Set( iNodeBitIndex );
		return true;
	}

	return false;
}

void CDispInfo::SetupDecalNodeIntersect( CVertIndex const &nodeIndex, int iNodeBitIndex,
	CDispDecalBase *pDispDecal,	ShadowInfo_t const* pInfo )
{
	pDispDecal->m_NodeIntersect.ClearAll();

	// Generate a vertex cache, so we're not continually reprojecting vertices...
	static CDecalNodeSetupCache cache;
	cache.ResetCache();

	bool anyIntersection = SetupDecalNodeIntersect_R( nodeIndex, iNodeBitIndex,
		pDispDecal, pInfo, 0, &cache );

	pDispDecal->m_Flags |= CDispDecalBase::NODE_BITFIELD_COMPUTED;
	if (anyIntersection)
		pDispDecal->m_Flags &= ~CDispDecalBase::NO_INTERSECTION;
	else
		pDispDecal->m_Flags |= CDispDecalBase::NO_INTERSECTION;
}



//-----------------------------------------------------------------------------
// Computes the texture + lightmap coordinate given a displacement uv
//-----------------------------------------------------------------------------

void CDispInfo::ComputeLightmapAndTextureCoordinate( RayDispOutput_t const& output, 
													Vector2D* luv, Vector2D* tuv )
{	
#ifndef SWDS
	// lightmap coordinate
	if( luv )
	{
		ComputePointFromBarycentric( m_Verts[output.ndxVerts[0]].m_LMCoords,
			m_Verts[output.ndxVerts[1]].m_LMCoords,
			m_Verts[output.ndxVerts[2]].m_LMCoords,
			output.u, output.v, *luv );

		// luv is in the space of the accumulated lightmap page; we need to convert
		// it to be in the space of the surface
		int lightmapPageWidth, lightmapPageHeight;
		materialSystemInterface->GetLightmapPageSize( 
			SortInfoToLightmapPage(MSurf_MaterialSortID( m_ParentSurfID ) ),
			&lightmapPageWidth, &lightmapPageHeight );

		luv->x *= lightmapPageWidth;
		luv->y *= lightmapPageHeight;

		luv->x -= 0.5f + MSurf_OffsetIntoLightmapPage( m_ParentSurfID )[0];
		luv->y -= 0.5f + MSurf_OffsetIntoLightmapPage( m_ParentSurfID )[1];
	}

	// texture coordinate
	if( tuv )
	{
		// Compute base face (u,v) at each of the three vertices
		int size = (1 << m_Power) + 1; 

		Vector2D baseUV[3];
		for (int i = 0; i < 3; ++i )
		{
			baseUV[i].y = (int)(output.ndxVerts[i] / size);
			baseUV[i].x = output.ndxVerts[i] - size * baseUV[i].y;
			baseUV[i] /= size - 1;
		}

		Vector2D basefaceUV;
		ComputePointFromBarycentric( baseUV[0], baseUV[1], baseUV[2],
			output.u, output.v, basefaceUV );

		// Convert the base face uv to a texture uv based on the base face texture coords
		TexCoordInQuadFromBarycentric( m_BaseSurfaceTexCoords[0],
			m_BaseSurfaceTexCoords[3], m_BaseSurfaceTexCoords[2], m_BaseSurfaceTexCoords[1],
			basefaceUV, *tuv ); 
	}
#endif
}



//-----------------------------------------------------------------------------
// Cast a ray against this surface
//-----------------------------------------------------------------------------

bool CDispInfo::TestRay( Ray_t const& ray, float start, float end, float& dist, 
						Vector2D* luv, Vector2D* tuv )
{
	// Get the index associated with this disp info....
	int idx = DispInfo_ComputeIndex( host_state.worldmodel->brush.hDispInfos, this );
	CDispCollTree* pTree = CollisionBSPData_GetCollisionTree( idx );
	if (!pTree)
		return false;

	CBaseTrace tr;
	tr.fraction = 1.0f;

	// Only test the portion of the ray between start and end
	Vector startpt, endpt,endpt2;
	VectorMA( ray.m_Start, start, ray.m_Delta, startpt );
	VectorMA( ray.m_Start, end, ray.m_Delta, endpt );

	Ray_t shortenedRay;
	shortenedRay.Init( startpt, endpt );

	RayDispOutput_t	output;
	if (pTree->RayTest( shortenedRay, output ))
	{
		Assert( (output.u <= 1.0f) && (output.v <= 1.0f ));
		Assert( (output.u >= 0.0f) && (output.v >= 0.0f ));

		// Compute the actual distance along the ray
		dist = start * (1.0f - output.dist) + end * output.dist;

		// Compute lightmap + texture coordinates
		ComputeLightmapAndTextureCoordinate( output, luv, tuv );
		return true;
	}

	return false;
}

const CPowerInfo* CDispInfo::GetPowerInfo() const
{
	return m_pPowerInfo;
}


CDispNeighbor* CDispInfo::GetEdgeNeighbor( int index )
{
	Assert( index >= 0 && index < ARRAYSIZE( m_EdgeNeighbors ) );
	return &m_EdgeNeighbors[index];
}


CDispCornerNeighbors* CDispInfo::GetCornerNeighbors( int index )
{
	Assert( index >= 0 && index < ARRAYSIZE( m_CornerNeighbors ) );
	return &m_CornerNeighbors[index];
}


CDispUtilsHelper* CDispInfo::GetDispUtilsByIndex( int index )
{
	return GetDispByIndex( index );	
}


