//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include <stdafx.h>
#include "MapOverlay.h"
#include "MapFace.h"
#include "MapSolid.h"
#include "MapWorld.h"
#include "MainFrm.h"
#include "GlobalFunctions.h"
#include "MapDoc.h"
#include "TextureSystem.h"
#include "Material.h"
#include "materialsystem/IMesh.h"
#include "Box3D.h"
#include "MapDefs.h"
#include "CollisionUtils.h"
#include "MapSideList.h"
#include "MapDisp.h"
#include "ToolManager.h"

IMPLEMENT_MAPCLASS( CMapOverlay )

#define OVERLAY_INITSIZE				25.0f		// x2

#define OVERLAY_BASIS_U					0
#define OVERLAY_BASIS_V					1
#define OVERLAY_BASIS_NORMAL			2	

#define OVERLAY_HANDLES_COUNT			4
#define OVERLAY_HANDLES_SIZE			3
#define OVERLAY_HANDLES_BLOATEDSIZE		5

#define OVERLAY_WORLDSPACE_EPSILON		0.03125f
#define OVERLAY_DISPSPACE_EPSILON		0.000001f
#define OVERLAY_BARYCENTRIC_EPSILON		0.001f

#define OVERLAY_BLENDTYPE_VERT			0
#define OVERLAY_BLENDTYPE_EDGE			1
#define OVERLAY_BLENDTYPE_BARY			2
#define OVERLAY_ANGLE0					1
#define OVERLAY_ANGLE45					2
#define OVERLAY_ANGLE90					3
#define OVERLAY_ANGLE135				4

//=============================================================================
//
// Basis Functions
//

//-----------------------------------------------------------------------------
// Purpose: Initialize the basis data.
//-----------------------------------------------------------------------------
void CMapOverlay::BasisInit( void )
{
	m_Basis.m_pFace = NULL;
	m_Basis.m_vecOrigin.Init();

	for( int iAxis = 0; iAxis < 3; iAxis++ )
	{
		m_Basis.m_vecAxes[iAxis].Init();
		m_Basis.m_vecCacheAxes[iAxis].Init();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Build the overlay basis given an entity and base face (CMapFace).
//-----------------------------------------------------------------------------
void CMapOverlay::BasisBuild( CMapFace *pFace )
{
	// Valid face?
	Assert( pFace != NULL );
	if( !pFace )
		return;

	// Set the face the basis are derived from.
	m_Basis.m_pFace = pFace;

	// Set the basis origin.
	BasisSetOrigin();

	//
	// Build the basis axes.
	//
	Vector vecFaceNormal;
	pFace->GetFaceNormal( vecFaceNormal );
	VectorNormalize( vecFaceNormal );
	VectorCopy( vecFaceNormal, m_Basis.m_vecAxes[OVERLAY_BASIS_NORMAL] );

	BasisSetInitialUAxis( vecFaceNormal );

	m_Basis.m_vecAxes[OVERLAY_BASIS_V] = m_Basis.m_vecAxes[OVERLAY_BASIS_NORMAL].Cross( m_Basis.m_vecAxes[OVERLAY_BASIS_U] );
	VectorNormalize( m_Basis.m_vecAxes[OVERLAY_BASIS_V] );

	m_Basis.m_vecAxes[OVERLAY_BASIS_U] = m_Basis.m_vecAxes[OVERLAY_BASIS_V].Cross( m_Basis.m_vecAxes[OVERLAY_BASIS_NORMAL] );
	VectorNormalize( m_Basis.m_vecAxes[OVERLAY_BASIS_U] );

	// Build the cached basis (rendering version).
	BasisBuildCached();	
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::BasisBuildFromSideList( void )
{
	//
	// initialization (don't have or couldn't find the basis face)
	//
	if ( ( !m_Basis.m_pFace ) || ( m_Faces.Find( m_Basis.m_pFace ) == -1 ) )
	{
		if ( m_Faces.Count() > 0 )
		{
			BasisBuild( m_Faces.Element( 0 ) );
		}
		else
		{
			m_Basis.m_pFace = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::BasisReBuild( void )
{
	// Set the basis origin.
	BasisSetOrigin();

	// Build the cached basis (rendering version).
	BasisBuildCached();
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the overlay basis origin given an entity and base face 
//          (CMapFace).
//-----------------------------------------------------------------------------
void CMapOverlay::BasisSetOrigin( void )
{
	// Valid face?
	if ( !m_Basis.m_pFace )
		return;

	CMapEntity *pEntity = ( CMapEntity* )GetParent();
	if ( pEntity )
	{
		Vector vecEntityOrigin;
		pEntity->GetOrigin( vecEntityOrigin );
		
		// Get the distance from entity origin to face plane.
		float flDist = m_Basis.m_pFace->plane.normal.Dot( vecEntityOrigin ) - m_Basis.m_pFace->plane.dist;
		
		// Move the basis origin to the position of the entity projected into the face plane.
		m_Basis.m_vecOrigin = vecEntityOrigin - ( flDist * m_Basis.m_pFace->plane.normal );
	}
}

//-----------------------------------------------------------------------------
// Purpose: A basis building helper function that finds the best guess u-axis
//          given a base face (CMapFace) normal.
//   Input: vecNormal - the base face normal
//-----------------------------------------------------------------------------
void CMapOverlay::BasisSetInitialUAxis( Vector const &vecNormal )
{
	// Find the major vector component.
	int nMajorAxis = 0;
	float flAxisValue = vecNormal[0];
	if ( fabs( vecNormal[1] ) > fabs( flAxisValue ) ) { nMajorAxis = 1; flAxisValue = vecNormal[1]; }
	if ( fabs( vecNormal[2] ) > fabs( flAxisValue ) ) { nMajorAxis = 2; }

	if ( ( nMajorAxis == 1 ) || ( nMajorAxis == 2 ) )
	{
		m_Basis.m_vecAxes[OVERLAY_BASIS_U].Init( 1.0f, 0.0f, 0.0f );
	}
	else
	{
		m_Basis.m_vecAxes[OVERLAY_BASIS_U].Init( 0.0f, 1.0f, 0.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copy the original basis to the cached version.  This version is
//          used in rendering, etc.
//-----------------------------------------------------------------------------
void CMapOverlay::BasisBuildCached( void )
{
	m_Basis.m_vecCacheAxes[OVERLAY_BASIS_U] = m_Basis.m_vecAxes[OVERLAY_BASIS_U];
	m_Basis.m_vecCacheAxes[OVERLAY_BASIS_V] = m_Basis.m_vecAxes[OVERLAY_BASIS_V];
	m_Basis.m_vecCacheAxes[OVERLAY_BASIS_NORMAL] = m_Basis.m_vecAxes[OVERLAY_BASIS_NORMAL];
}

//-----------------------------------------------------------------------------
// Purpose: Copy the basis data from the source into the destination.
//   Input: pSrc - the basis source data
//          pDst (Output) - destination for the  basis data
//-----------------------------------------------------------------------------
void CMapOverlay::BasisCopy( Basis_t *pSrc, Basis_t *pDst )
{
	pDst->m_pFace = pSrc->m_pFace;
	pDst->m_vecOrigin = pSrc->m_vecOrigin;

	for ( int iAxis = 0; iAxis < 3; iAxis++ )
	{
		pDst->m_vecAxes[iAxis] = pSrc->m_vecAxes[iAxis];
		pDst->m_vecCacheAxes[iAxis] = pSrc->m_vecCacheAxes[iAxis];
	}
}

//=============================================================================
//
// Handles Functions
//

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesInit( void )
{
	m_Handles.m_iHit = -1;

	for ( int iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; iHandle++ )
	{
		m_Handles.m_vec3D[iHandle].Init();
		m_Handles.m_vec2D[iHandle].Init();
	}

	m_Handles.m_vecBasisCoords[0].Init( -OVERLAY_INITSIZE, -OVERLAY_INITSIZE );
	m_Handles.m_vecBasisCoords[1].Init( -OVERLAY_INITSIZE, OVERLAY_INITSIZE );
	m_Handles.m_vecBasisCoords[2].Init( OVERLAY_INITSIZE, OVERLAY_INITSIZE );
	m_Handles.m_vecBasisCoords[3].Init( OVERLAY_INITSIZE, -OVERLAY_INITSIZE );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesBuild( void )
{
	// Have to have a valid face to have a valid basis.  Both are needed for building
	// handles.
	if ( !m_Basis.m_pFace )
		return;

	for ( int iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; iHandle++ )
	{
		Vector vecHandle;
		OverlayUVToOverlayPlane( m_Handles.m_vecBasisCoords[iHandle], vecHandle );
		HandlesOverlayPlaneToSurf( m_Basis.m_pFace, vecHandle, m_Handles.m_vec3D[iHandle] );
	}

	// Update the property box with new handle positions.
	HandlesUpdatePropertyBox();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesBuild2D( CRender3D *pRender )
{
	for ( int iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; iHandle++ )
	{
		// Create the 2d version of the handles.
		pRender->WorldToScreen( m_Handles.m_vec2D[iHandle], m_Handles.m_vec3D[iHandle] );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesRender( CRender3D *pRender )
{
	// Build/cache the 2d handles.
	HandlesBuild2D( pRender );

	// Set the render mode to "flat."
	pRender->SetRenderMode( RENDER_MODE_FLAT );

	// Set the color, should be based on selection.
	unsigned char ucColor[4];
	ucColor[0] = ucColor[1] = ucColor[2] = ucColor[3] = 255;

	unsigned char ucSelectColor[4];
	ucSelectColor[0] = ucSelectColor[3] = 255;
	ucSelectColor[1] = ucSelectColor[2] = 0;

	for ( int iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; iHandle++ )
	{
		pRender->BeginRenderHitTarget( this, iHandle );
		if ( m_Handles.m_iHit == iHandle )
		{
			HandlesRender2D( pRender, m_Handles.m_vec2D[iHandle], ucSelectColor );
		}
		else
		{
			HandlesRender2D( pRender, m_Handles.m_vec2D[iHandle], ucColor );
		}

		pRender->EndRenderHitTarget();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesRender2D( CRender3D *pRender, Vector2D const &vecScreenPt, 
							       unsigned char *pColor )
{
	// Render handle as a screen space box.
	pRender->BeginParallel();

	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_POLYGON, OVERLAY_HANDLES_COUNT );

	meshBuilder.Position3f( vecScreenPt[0] - OVERLAY_HANDLES_SIZE, vecScreenPt[1] - OVERLAY_HANDLES_SIZE, 10 );
	meshBuilder.Color3ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vecScreenPt[0] - OVERLAY_HANDLES_SIZE, vecScreenPt[1] + OVERLAY_HANDLES_SIZE, 10 );
	meshBuilder.Color3ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vecScreenPt[0] + OVERLAY_HANDLES_SIZE, vecScreenPt[1] + OVERLAY_HANDLES_SIZE, 10 );
	meshBuilder.Color3ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vecScreenPt[0] + OVERLAY_HANDLES_SIZE, vecScreenPt[1] - OVERLAY_HANDLES_SIZE, 10 );
	meshBuilder.Color3ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	meshBuilder.Begin( pMesh, MATERIAL_LINE_LOOP, OVERLAY_HANDLES_COUNT );

	meshBuilder.Position3f( vecScreenPt[0] - OVERLAY_HANDLES_SIZE, vecScreenPt[1] - OVERLAY_HANDLES_SIZE, 10 );
	meshBuilder.Color3ub( 20, 20, 20 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vecScreenPt[0] - OVERLAY_HANDLES_SIZE, vecScreenPt[1] + OVERLAY_HANDLES_SIZE, 10 );
	meshBuilder.Color3ub( 20, 20, 20 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vecScreenPt[0] + OVERLAY_HANDLES_SIZE, vecScreenPt[1] + OVERLAY_HANDLES_SIZE, 10 );
	meshBuilder.Color3ub( 20, 20, 20 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( vecScreenPt[0] + OVERLAY_HANDLES_SIZE, vecScreenPt[1] - OVERLAY_HANDLES_SIZE, 10 );
	meshBuilder.Color3ub( 20, 20, 20 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	pRender->EndParallel();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesRotate( QAngle const &vecAngles )
{
	// Create a rotation matrix.
	matrix4_t mat;
	IdentityMatrix( mat );

	// Find the axis of rotation - entity rotation can only happen on one axis at 
	// a time.
	if ( vecAngles.x != 0.0f )
	{
		RotateX( mat, vecAngles[0] );
	}
	else if ( vecAngles.y != 0.0f )
	{
		RotateY( mat, vecAngles[1] );
	}
	else 
	{
		RotateZ( mat, vecAngles[2] );
	}

	// Translate the points back to there origin - the origin basis.
	Vector vecHandles[OVERLAY_HANDLES_COUNT];
	for ( int iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; iHandle++ )
	{
		OverlayUVToOverlayPlane( m_Handles.m_vecBasisCoords[iHandle], vecHandles[iHandle] );
		vecHandles[iHandle] -= m_Basis.m_vecOrigin;
	}

	// Update the basis origin.
	BasisSetOrigin();

	//
	// Rotate the handles.
	//
	Vector vecTmp;
	for ( iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; iHandle++ )
	{
		MatrixMultiply( vecTmp, vecHandles[iHandle], mat );
		vecTmp += m_Basis.m_vecOrigin;
		OverlayPlaneToOverlayUV( vecTmp, m_Handles.m_vecBasisCoords[iHandle] );
	}

	// Rebuild the handles.
	HandlesBuild();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesFlip( Vector const &vecRefPoint )
{
	//
	// Flip the basis.
	//
	for ( int iAxis = 0; iAxis < 3; iAxis++ )
	{
		if ( vecRefPoint[iAxis] != COORD_NOTINIT )
		{
			m_Basis.m_vecAxes[OVERLAY_BASIS_U][iAxis]      = -m_Basis.m_vecAxes[OVERLAY_BASIS_U][iAxis];
			m_Basis.m_vecAxes[OVERLAY_BASIS_V][iAxis]      = -m_Basis.m_vecAxes[OVERLAY_BASIS_V][iAxis];
			m_Basis.m_vecAxes[OVERLAY_BASIS_NORMAL][iAxis] = -m_Basis.m_vecAxes[OVERLAY_BASIS_NORMAL][iAxis];
		}
	}
	BasisReBuild();

	//
	// Flip handles.
	//
	Vector2D vecCoords[OVERLAY_HANDLES_COUNT];
	Vector2D vecTexCoords[OVERLAY_HANDLES_COUNT];
	for ( int iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; iHandle++ )
	{
		vecCoords[4-iHandle-1] = m_Handles.m_vecBasisCoords[iHandle];
		vecTexCoords[4-iHandle-1] = m_Material.m_vecTexCoords[iHandle];
	}

	for ( iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; iHandle++ )
	{
		m_Handles.m_vecBasisCoords[iHandle] = vecCoords[iHandle];
		m_Material.m_vecTexCoords[iHandle] = vecTexCoords[iHandle];
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesSurfToOverlayPlane( CMapFace *pFace, Vector const &vecSurf, Vector &vecPoint )
{
	Vector vecWorld;
	if ( pFace->HasDisp() )
	{
		EditDispHandle_t handle = pFace->GetDisp();
		CMapDisp *pDisp = EditDispMgr()->GetDisp( handle );
		pDisp->SurfToBaseFacePlane( vecSurf, vecWorld );
	}
	else
	{
		vecWorld = vecSurf;
	}

	WorldToOverlayPlane( vecWorld, vecPoint );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesOverlayPlaneToSurf( CMapFace *pFace, Vector const &vecPoint, Vector &vecSurf )
{
	int nPointCount = 0;
	static Vector s_vecPoints[64];

	int nEdgeCount = 0;
	static cplane_t s_Planes[64];

	VectorCopy( vecPoint, vecSurf );

	int nFaceCount = m_Faces.Count();
	for ( int iFace = 0; iFace < nFaceCount; iFace++ )
	{
		CMapFace *pFace = m_Faces.Element( iFace );
		if ( pFace )
		{
			// Valid face?
			nPointCount = pFace->nPoints;
			if( nPointCount >= 64 )
				continue;

			//
			// Project all points into the overlay plane.
			//
			for ( int iPoint = 0; iPoint < nPointCount; iPoint++ )
			{
				WorldToOverlayPlane( pFace->Points[iPoint], s_vecPoints[iPoint] );
			}

			//
			// Create edge planes for clipping.
			//
			nEdgeCount = nPointCount;
			BuildEdgePlanes( s_vecPoints, nPointCount, s_Planes, nEdgeCount );

			//
			// Check to see if a handle lies behind all of the edge planes.
			//
			for ( int iEdge = 0; iEdge < nEdgeCount; iEdge++ )
			{
				float flDist = s_Planes[iEdge].normal.Dot( vecPoint ) - s_Planes[iEdge].dist;
				if( flDist >= 0.0f )
					break;
			}

			// Point lies outside off at least one plane.
			if( iEdge != nEdgeCount )
				continue;

			//
			// Project the point up to the base face plane (displacement if necessary).
			//
			OverlayPlaneToWorld( pFace, vecPoint, vecSurf );

			if( pFace->HasDisp() )
			{
				Vector2D vecTmp;
				EditDispHandle_t handle = pFace->GetDisp();
				CMapDisp *pDisp = EditDispMgr()->GetDisp( handle );
				pDisp->BaseFacePlaneToDispUV( vecSurf, vecTmp );
				pDisp->DispUVToSurf( vecTmp, vecSurf );
			}

			return;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesUpdatePropertyBox( void )
{
	char szValue[80];

	CMapEntity *pEntity = ( CMapEntity* )GetParent();
	if ( pEntity )
	{
		sprintf( szValue, "%g %g %g", m_Handles.m_vecBasisCoords[0].x, m_Handles.m_vecBasisCoords[0].y, 0.0f );
		pEntity->NotifyChildKeyChanged( this, "uv0", szValue );

		sprintf( szValue, "%g %g %g", m_Handles.m_vecBasisCoords[1].x, m_Handles.m_vecBasisCoords[1].y, 0.0f );
		pEntity->NotifyChildKeyChanged( this, "uv1", szValue );

		sprintf( szValue, "%g %g %g", m_Handles.m_vecBasisCoords[2].x, m_Handles.m_vecBasisCoords[2].y, 0.0f );
		pEntity->NotifyChildKeyChanged( this, "uv2", szValue );

		sprintf( szValue, "%g %g %g", m_Handles.m_vecBasisCoords[3].x, m_Handles.m_vecBasisCoords[3].y, 0.0f );
		pEntity->NotifyChildKeyChanged( this, "uv3", szValue );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesCopy( Handles_t *pSrc, Handles_t *pDst )
{
	pDst->m_iHit = pSrc->m_iHit;

	for ( int iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; ++iHandle )
	{
		pDst->m_vecBasisCoords[iHandle] = pSrc->m_vecBasisCoords[iHandle];
		pDst->m_vec2D[iHandle] = pSrc->m_vec2D[iHandle];
		pDst->m_vec3D[iHandle] = pSrc->m_vec3D[iHandle];
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesBounds( Vector &vecMin, Vector &vecMax )
{
	vecMin.Init( m_Handles.m_vecBasisCoords[0].x, m_Handles.m_vecBasisCoords[0].y, 0.0f );
	vecMax.Init( m_Handles.m_vecBasisCoords[0].x, m_Handles.m_vecBasisCoords[0].y, 0.0f );

	for( int iHandle = 1; iHandle < OVERLAY_HANDLES_COUNT; ++iHandle )
	{
		for( int iAxis = 0; iAxis < 2; ++iAxis )
		{
			if ( m_Handles.m_vecBasisCoords[iHandle][iAxis] < vecMin[iAxis] )
			{
				vecMin[iAxis] = m_Handles.m_vecBasisCoords[iHandle][iAxis];
			}

			if ( m_Handles.m_vecBasisCoords[iHandle][iAxis] > vecMax[iAxis] )
			{
				vecMax[iAxis] = m_Handles.m_vecBasisCoords[iHandle][iAxis];
			}
		}
	}
}

//=============================================================================
//
// ClipFace Functions
//

static int nClipFaceAlloc = 0;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CMapOverlay::ClipFace_t *CMapOverlay::ClipFaceCreate( int nSize )
{
	ClipFace_t *pClipFace = new ClipFace_t;
	if ( pClipFace )
	{
//		TRACE1( "ClipFace Alloc %d\n", nClipFaceAlloc++ );

		pClipFace->m_nPointCount = nSize;
		if ( nSize > 0 )
		{
			pClipFace->m_aPoints.SetSize( nSize );
			pClipFace->m_aDispPointUVs.SetSize( nSize );
			
			for ( int i=0; i < NUM_CLIPFACE_TEXCOORDS; i++ )
				pClipFace->m_aTexCoords[i].SetSize( nSize );
			
			pClipFace->m_aBlends.SetSize( nSize );
			
			for ( int iPoint = 0; iPoint < nSize; iPoint++ )
			{
				pClipFace->m_aPoints[iPoint].Init();
				pClipFace->m_aDispPointUVs[iPoint].Init();
				
				for ( i=0; i < NUM_CLIPFACE_TEXCOORDS; i++ )
					pClipFace->m_aTexCoords[i][iPoint].Init();
			}
		}
	}

	return pClipFace;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFaceDestroy( ClipFace_t **ppClipFace )
{
	ClipFace_t *pClipFace = *ppClipFace;

	if( pClipFace )
	{
		pClipFace->m_aPoints.Purge();
		pClipFace->m_aDispPointUVs.Purge();
		pClipFace->m_aBlends.Purge();
		
		delete *ppClipFace;
		*ppClipFace = NULL;

//		TRACE1( "ClipFace De-Alloc %d\n", --nClipFaceAlloc );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFacesDestroy( ClipFaces_t &aClipFaces )
{
	int nFaceCount = aClipFaces.Count();
	for ( int iFace = 0; iFace < nFaceCount; ++iFace )
	{
		ClipFace_t *pClipFace = aClipFaces[iFace];
		
		if( pClipFace )
		{
			pClipFace->m_aPoints.Purge();
			pClipFace->m_aDispPointUVs.Purge();
			pClipFace->m_aBlends.Purge();
			
			delete pClipFace;
			
//			TRACE1( "ClipFace De-Alloc %d\n", --nClipFaceAlloc );
		}
	}

	aClipFaces.Purge();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFaceGetBounds( ClipFace_t *pClipFace, Vector &vecMin, Vector &vecMax )
{
	if ( pClipFace )
	{
		vecMin = vecMax = pClipFace->m_aPoints.Element( 0 );
		
		for ( int iPoints = 1; iPoints < pClipFace->m_nPointCount; iPoints++ )
		{
			Vector vecPoint = pClipFace->m_aPoints.Element( iPoints );
			
			// Min
			if ( vecMin.x > vecPoint.x ) { vecMin.x = vecPoint.x; }
			if ( vecMin.y > vecPoint.y ) { vecMin.y = vecPoint.y; }
			if ( vecMin.z > vecPoint.z ) { vecMin.z = vecPoint.z; }
			
			// Max
			if ( vecMax.x < vecPoint.x ) { vecMax.x = vecPoint.x; }
			if ( vecMax.y < vecPoint.y ) { vecMax.y = vecPoint.y; }
			if ( vecMax.z < vecPoint.z ) { vecMax.z = vecPoint.z; }
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFaceClip( ClipFace_t *pClipFace, cplane_t *pClipPlane, float flEpsilon,
							    ClipFace_t **ppFront, ClipFace_t **ppBack )
{
	if ( !pClipFace )
		return;

	float	flDists[128];
	int		nSides[128];
	int		nSideCounts[3];

	// Initialize
	*ppFront = *ppBack = NULL;

	//
	// Determine "sidedness" of all the polygon points.
	//
	nSideCounts[0] = nSideCounts[1] = nSideCounts[2] = 0;
	for ( int iPoint = 0; iPoint < pClipFace->m_nPointCount; iPoint++ )
	{
		flDists[iPoint] = pClipPlane->normal.Dot( pClipFace->m_aPoints.Element( iPoint ) ) - pClipPlane->dist;

		if ( flDists[iPoint] > flEpsilon )
		{
			nSides[iPoint] = SIDE_FRONT;
		}
		else if ( flDists[iPoint] < -flEpsilon )
		{
			nSides[iPoint] = SIDE_BACK;
		}
		else
		{
			nSides[iPoint] = SIDE_ON;
		}

		nSideCounts[nSides[iPoint]]++;
	}

	// Wrap around (close the polygon).
	nSides[iPoint] = nSides[0];
	flDists[iPoint] =  flDists[0];

	// All points in back - no split (copy face to back).
	if( !nSideCounts[SIDE_FRONT] )
	{
		*ppBack = ClipFaceCopy( pClipFace );
		return;
	}

	// All points in front - no split (copy face to front).
	if( !nSideCounts[SIDE_BACK] )
	{
		*ppFront = ClipFaceCopy( pClipFace );
		return;
	}

	// Build new front and back faces. Leave room for two extra points on each side because any
	// point might be on the plane, which would put it into both the front and back sides, and then
	// we need to allow for an additional vertex created by clipping.
	ClipFace_t *pFront = ClipFaceCreate( pClipFace->m_nPointCount + 2 );
	ClipFace_t *pBack = ClipFaceCreate( pClipFace->m_nPointCount + 2 );
	if ( !pFront || !pBack )
	{
		ClipFaceDestroy( &pFront );
		ClipFaceDestroy( &pBack );
		return;
	}

	// Reset the counts as they are used to build the surface.
	pFront->m_nPointCount = 0;
	pBack->m_nPointCount = 0;

	// For every point on the face being clipped, determine which side of the clipping plane it is on
	// and add it to a either a front list or a back list. Points that are on the plane are added to
	// both lists.
	for ( iPoint = 0; iPoint < pClipFace->m_nPointCount; iPoint++ )
	{
		// "On" clip plane.
		if ( nSides[iPoint] == SIDE_ON )
		{
			pFront->m_aPoints[pFront->m_nPointCount] = pClipFace->m_aPoints[iPoint];
			for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
				pFront->m_aTexCoords[iTexCoord][pFront->m_nPointCount] = pClipFace->m_aTexCoords[iTexCoord][iPoint];
			pFront->m_nPointCount++;

			pBack->m_aPoints[pBack->m_nPointCount] = pClipFace->m_aPoints[iPoint];
			for ( iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
				pBack->m_aTexCoords[iTexCoord][pBack->m_nPointCount] = pClipFace->m_aTexCoords[iTexCoord][iPoint];
			pBack->m_nPointCount++;

			continue;
		}

		// "In back" of clip plane.
		if ( nSides[iPoint] == SIDE_BACK )
		{
			pBack->m_aPoints[pBack->m_nPointCount] = pClipFace->m_aPoints[iPoint];
			for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
				pBack->m_aTexCoords[iTexCoord][pBack->m_nPointCount] = pClipFace->m_aTexCoords[iTexCoord][iPoint];
			pBack->m_nPointCount++;
		}

		// "In front" of clip plane.
		if ( nSides[iPoint] == SIDE_FRONT )
		{
			pFront->m_aPoints[pFront->m_nPointCount] = pClipFace->m_aPoints[iPoint];
			for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
				pFront->m_aTexCoords[iTexCoord][pFront->m_nPointCount] = pClipFace->m_aTexCoords[iTexCoord][iPoint];
			pFront->m_nPointCount++;
		}

		if ( nSides[iPoint+1] == SIDE_ON || nSides[iPoint+1] == nSides[iPoint] )
			continue;

		// Split!
		float fraction = flDists[iPoint] / ( flDists[iPoint] - flDists[iPoint+1] );

		Vector vecPoint = pClipFace->m_aPoints[iPoint] + ( pClipFace->m_aPoints[(iPoint+1)%pClipFace->m_nPointCount] - pClipFace->m_aPoints[iPoint] ) * fraction;
		for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
		{
			Vector2D vecTexCoord = pClipFace->m_aTexCoords[iTexCoord][iPoint] + ( pClipFace->m_aTexCoords[iTexCoord][(iPoint+1)%pClipFace->m_nPointCount] - pClipFace->m_aTexCoords[iTexCoord][iPoint] ) * fraction;
			pFront->m_aTexCoords[iTexCoord][pFront->m_nPointCount] = vecTexCoord;
			pBack->m_aTexCoords[iTexCoord][pBack->m_nPointCount] = vecTexCoord;
		}
	
		pFront->m_aPoints[pFront->m_nPointCount] = vecPoint;
		pFront->m_nPointCount++;

		pBack->m_aPoints[pBack->m_nPointCount] = vecPoint;
		pBack->m_nPointCount++;
	}

	*ppFront = pFront;
	*ppBack = pBack;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFaceClipBarycentric( ClipFace_t *pClipFace, cplane_t *pClipPlane, float flEpsilon,
									       int iClip, CMapDisp *pDisp,
									       ClipFace_t **ppFront, ClipFace_t **ppBack )
{
	if ( !pClipFace )
		return;

	float	flDists[128];
	int		nSides[128];
	int		nSideCounts[3];

	//
	// Determine "sidedness" of all the polygon points.
	//
	nSideCounts[0] = nSideCounts[1] = nSideCounts[2] = 0;
	for ( int iPoint = 0; iPoint < pClipFace->m_nPointCount; iPoint++ )
	{
		flDists[iPoint] = pClipPlane->normal.Dot( pClipFace->m_aDispPointUVs.Element( iPoint ) ) - pClipPlane->dist;

		if ( flDists[iPoint] > flEpsilon )
		{
			nSides[iPoint] = SIDE_FRONT;
		}
		else if ( flDists[iPoint] < -flEpsilon )
		{
			nSides[iPoint] = SIDE_BACK;
		}
		else
		{
			nSides[iPoint] = SIDE_ON;
		}

		nSideCounts[nSides[iPoint]]++;
	}

	// Wrap around (close the polygon).
	nSides[iPoint] = nSides[0];
	flDists[iPoint] =  flDists[0];

	// All points in back - no split (copy face to back).
	if( !nSideCounts[SIDE_FRONT] )
	{
		*ppBack = ClipFaceCopy( pClipFace );
		return;
	}

	// All points in front - no split (copy face to front).
	if( !nSideCounts[SIDE_BACK] )
	{
		*ppFront = ClipFaceCopy( pClipFace );
		return;
	}

	// Build new front and back faces.
	// NOTE: We are allowing to go over by 2 and then destroy the surface later.  The old system
	//       allowed for some bad data and we need to be able to load the map and destroy the surface!
	int nMaxPointCount = pClipFace->m_nPointCount + 1;
	ClipFace_t *pFront = ClipFaceCreate( nMaxPointCount + 2 );
	ClipFace_t *pBack = ClipFaceCreate( nMaxPointCount + 2 );
	if ( !pFront || !pBack )
	{
		ClipFaceDestroy( &pFront );
		ClipFaceDestroy( &pBack );
		return;
	}

	// Reset the counts as they are used to build the surface.
	pFront->m_nPointCount = 0;
	pBack->m_nPointCount = 0;

	for ( iPoint = 0; iPoint < pClipFace->m_nPointCount; iPoint++ )
	{
		// "On" clip plane.
		if ( nSides[iPoint] == SIDE_ON )
		{
			pFront->m_aPoints[pFront->m_nPointCount] = pClipFace->m_aPoints[iPoint];
			pFront->m_aDispPointUVs[pFront->m_nPointCount] = pClipFace->m_aDispPointUVs[iPoint];
			
			for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
				pFront->m_aTexCoords[iTexCoord][pFront->m_nPointCount] = pClipFace->m_aTexCoords[iTexCoord][iPoint];
			
			ClipFaceCopyBlendFrom( pFront, &pClipFace->m_aBlends[iPoint] );
			pFront->m_nPointCount++;

			pBack->m_aPoints[pBack->m_nPointCount] = pClipFace->m_aPoints[iPoint];
			pBack->m_aDispPointUVs[pBack->m_nPointCount] = pClipFace->m_aDispPointUVs[iPoint];

			for ( iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
				pBack->m_aTexCoords[iTexCoord][pBack->m_nPointCount] = pClipFace->m_aTexCoords[iTexCoord][iPoint];

			ClipFaceCopyBlendFrom( pBack, &pClipFace->m_aBlends[iPoint] );
			pBack->m_nPointCount++;

			continue;
		}

		// "In back" of clip plane.
		if ( nSides[iPoint] == SIDE_BACK )
		{
			pBack->m_aPoints[pBack->m_nPointCount] = pClipFace->m_aPoints[iPoint];
			pBack->m_aDispPointUVs[pBack->m_nPointCount] = pClipFace->m_aDispPointUVs[iPoint];

			for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
				pBack->m_aTexCoords[iTexCoord][pBack->m_nPointCount] = pClipFace->m_aTexCoords[iTexCoord][iPoint];
			
			ClipFaceCopyBlendFrom( pBack, &pClipFace->m_aBlends[iPoint] );
			pBack->m_nPointCount++;
		}

		// "In front" of clip plane.
		if ( nSides[iPoint] == SIDE_FRONT )
		{
			pFront->m_aPoints[pFront->m_nPointCount] = pClipFace->m_aPoints[iPoint];
			pFront->m_aDispPointUVs[pFront->m_nPointCount] = pClipFace->m_aDispPointUVs[iPoint];

			for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
				pFront->m_aTexCoords[iTexCoord][pFront->m_nPointCount] = pClipFace->m_aTexCoords[iTexCoord][iPoint];

			ClipFaceCopyBlendFrom( pFront, &pClipFace->m_aBlends[iPoint] );
			pFront->m_nPointCount++;
		}

		if ( nSides[iPoint+1] == SIDE_ON || nSides[iPoint+1] == nSides[iPoint] )
			continue;

		// Split!
		float fraction = flDists[iPoint] / ( flDists[iPoint] - flDists[iPoint+1] );

		Vector vecPoint = pClipFace->m_aPoints[iPoint] + ( pClipFace->m_aPoints[(iPoint+1)%pClipFace->m_nPointCount] - pClipFace->m_aPoints[iPoint] ) * fraction;
		Vector vecDispPointUV = pClipFace->m_aDispPointUVs[iPoint] + ( pClipFace->m_aDispPointUVs[(iPoint+1)%pClipFace->m_nPointCount] - pClipFace->m_aDispPointUVs[iPoint] ) * fraction;

		Vector2D vecUV, vecTexCoord;
		PointInQuadToBarycentric( m_pOverlayFace->m_aPoints[0], m_pOverlayFace->m_aPoints[3], 
			                      m_pOverlayFace->m_aPoints[2], m_pOverlayFace->m_aPoints[1],
								  vecPoint, vecUV );

//		PointInQuadToBarycentric( m_pOverlayFace->m_DispPointUVs[0], m_pOverlayFace->m_DispPointUVs[3], 
//			                      m_pOverlayFace->m_DispPointUVs[2], m_pOverlayFace->m_DispPointUVs[1],
//								  vecDispPointUV, vecUV );
		if ( vecUV.x < 0.0f ) { vecUV.x = 0.0f; }
		if ( vecUV.x > 1.0f ) { vecUV.x = 1.0f; }
		if ( vecUV.y < 0.0f ) { vecUV.y = 0.0f; }
		if ( vecUV.y > 1.0f ) { vecUV.y = 1.0f; }

		for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
		{
			TexCoordInQuadFromBarycentric( m_pOverlayFace->m_aTexCoords[iTexCoord][0], m_pOverlayFace->m_aTexCoords[iTexCoord][3], 
										m_pOverlayFace->m_aTexCoords[iTexCoord][2], m_pOverlayFace->m_aTexCoords[iTexCoord][1],
										vecUV, vecTexCoord );
			
			pFront->m_aTexCoords[iTexCoord][pFront->m_nPointCount] = vecTexCoord;
			pBack->m_aTexCoords[iTexCoord][pBack->m_nPointCount] = vecTexCoord;
		}

		pFront->m_aPoints[pFront->m_nPointCount] = vecPoint;
		pFront->m_aDispPointUVs[pFront->m_nPointCount] = vecDispPointUV;
		ClipFaceBuildBlend( pFront, pDisp, pClipPlane, iClip, vecDispPointUV, vecPoint );
		pFront->m_nPointCount++;

		pBack->m_aPoints[pBack->m_nPointCount] = vecPoint;
		pBack->m_aDispPointUVs[pBack->m_nPointCount] = vecDispPointUV;
		ClipFaceBuildBlend( pBack, pDisp, pClipPlane, iClip, vecDispPointUV, vecPoint );
		pBack->m_nPointCount++;
	}

	// Check for a bad surface.
	if ( ( pFront->m_nPointCount > nMaxPointCount ) || ( pBack->m_nPointCount > nMaxPointCount ) )
		return;

	*ppFront = pFront;
	*ppBack = pBack;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFacePreClipDisp( ClipFace_t *pClipFace, CMapDisp *pDisp )
{
	// Valid clip face and/or displacement surface.
	if ( !pClipFace || !pDisp )
		return;

	// Transform all of the overlay points into disp uv space. 
	for ( int iPoint = 0; iPoint < pClipFace->m_nPointCount; iPoint++ )
	{
		Vector2D vecTmp;
		pDisp->BaseFacePlaneToDispUV( pClipFace->m_aPoints[iPoint], vecTmp );
		pClipFace->m_aDispPointUVs[iPoint].x = vecTmp.x;
		pClipFace->m_aDispPointUVs[iPoint].y = vecTmp.y;
		pClipFace->m_aDispPointUVs[iPoint].z = 0.0f;
	}

	// Set initial point barycentric blend types.
	for ( iPoint = 0; iPoint < pClipFace->m_nPointCount; ++iPoint )
	{
		Vector2D vecDispUV;
		vecDispUV.x = pClipFace->m_aDispPointUVs[iPoint].x;
		vecDispUV.y = pClipFace->m_aDispPointUVs[iPoint].y;

		int iTris[3];
		Vector2D vecVertsUV[3];
		GetTriVerts( pDisp, vecDispUV, iTris, vecVertsUV );

		float flCoefs[3];
		if ( ClipFaceCalcBarycentricCooefs( pDisp, vecVertsUV, vecDispUV, flCoefs ) )
		{
			ResolveBarycentricClip( pDisp, pClipFace, iPoint, vecDispUV, flCoefs, iTris, vecVertsUV );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMapOverlay::GetTriVerts( CMapDisp *pDisp, const Vector2D &vecSurfUV, int *pTris, Vector2D *pVertsUV )
{
	// Get the displacement width.
	int nWidth = pDisp->GetWidth();
	int nHeight = pDisp->GetHeight();

	// scale the u, v coordinates the displacement grid size
	float flU = vecSurfUV.x * ( nWidth - 1.000001f );
	float flV = vecSurfUV.y * ( nHeight - 1.000001f );

	// find the triangle the "uv spot" resides in
	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );
	if ( nSnapU == ( nWidth - 1 ) ) { --nSnapU; }
	if ( nSnapV == ( nHeight - 1 ) ) { --nSnapV; }
	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	
	// Fractional portion
	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	bool bOdd = ( ( ( nSnapV * nWidth ) + nSnapU ) % 2 ) == 1;
	if ( bOdd )
	{
		if( ( flFracU + flFracV ) >= ( 1.0f + OVERLAY_DISPSPACE_EPSILON ) )
		{
			pVertsUV[0].x = nSnapU;  pVertsUV[0].y = nNextV;
			pVertsUV[1].x = nNextU;  pVertsUV[1].y = nNextV;
			pVertsUV[2].x = nNextU;  pVertsUV[2].y = nSnapV;
		}
		else
		{
			pVertsUV[0].x = nSnapU;  pVertsUV[0].y = nSnapV;
			pVertsUV[1].x = nSnapU;  pVertsUV[1].y = nNextV;
			pVertsUV[2].x = nNextU;  pVertsUV[2].y = nSnapV;
		}
	}
	else
	{
		if ( flFracU < flFracV )
		{
			pVertsUV[0].x = nSnapU;  pVertsUV[0].y = nSnapV;
			pVertsUV[1].x = nSnapU;  pVertsUV[1].y = nNextV;
			pVertsUV[2].x = nNextU;  pVertsUV[2].y = nNextV;
		}
		else
		{
			pVertsUV[0].x = nSnapU;  pVertsUV[0].y = nSnapV;
			pVertsUV[1].x = nNextU;  pVertsUV[1].y = nNextV;
			pVertsUV[2].x = nNextU;  pVertsUV[2].y = nSnapV;
		}
	}

	// Calculate the triangle indices.
	for( int iVert = 0; iVert < 3; ++iVert )
	{
		pTris[iVert] = pVertsUV[iVert].y * nWidth + pVertsUV[iVert].x;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ResolveBarycentricClip( CMapDisp *pDisp, ClipFace_t *pClipFace, int iClipFacePoint, 
										  const Vector2D &vecPointUV, float *pCoefs, 
										  int *pTris, Vector2D *pVertsUV )
{
	int nInterval = pDisp->GetWidth();
	Vector2D vecScaledPointUV = vecPointUV * ( nInterval - 1.000001f );

	// Find the number of coefficients "equal" to zero.
	int nZeroCount = 0;
	bool bZeroPoint[3];
	for ( int iVert = 0; iVert < 3; ++iVert )
	{
		bZeroPoint[iVert] = false;
		if ( fabs( pCoefs[iVert] ) < OVERLAY_BARYCENTRIC_EPSILON )
		{
			nZeroCount++;
			bZeroPoint[iVert] = true;
		}
	}
	
	// Check for points - set to a point.
	if ( nZeroCount == 2 )
	{
		for ( iVert = 0; iVert < 3; ++iVert )
		{
			if ( !bZeroPoint[iVert] )
			{
				pClipFace->m_aBlends[iClipFacePoint].m_nType = OVERLAY_BLENDTYPE_VERT;
				pClipFace->m_aBlends[iClipFacePoint].m_iPoints[0] = pTris[iVert];
				return;
			}
		}
	}
	
	// Check for edges - setup edge blend.
	if ( nZeroCount == 1 )
	{
		for ( iVert = 0; iVert < 3; ++iVert )
		{
			if ( bZeroPoint[iVert] )
			{
				pClipFace->m_aBlends[iClipFacePoint].m_nType = OVERLAY_BLENDTYPE_EDGE;
				pClipFace->m_aBlends[iClipFacePoint].m_iPoints[0] = pTris[(iVert+1)%3];
				pClipFace->m_aBlends[iClipFacePoint].m_iPoints[1] = pTris[(iVert+2)%3];
				
				Vector2D vecLength1, vecLength2;
				vecLength1 = vecScaledPointUV - pVertsUV[(iVert+1)%3];
				vecLength2 = pVertsUV[(iVert+2)%3] - pVertsUV[(iVert+1)%3];
				float flBlend = vecLength1.Length() / vecLength2.Length();
				pClipFace->m_aBlends[iClipFacePoint].m_flBlends[0] = flBlend;
				return;
			}
		}
	}
	
	// Lies inside triangles - setup full barycentric blend.
	pClipFace->m_aBlends[iClipFacePoint].m_nType = OVERLAY_BLENDTYPE_BARY;
	pClipFace->m_aBlends[iClipFacePoint].m_iPoints[0] = pTris[0];
	pClipFace->m_aBlends[iClipFacePoint].m_iPoints[1] = pTris[1];
	pClipFace->m_aBlends[iClipFacePoint].m_iPoints[2] = pTris[2];
	pClipFace->m_aBlends[iClipFacePoint].m_flBlends[0] = pCoefs[0];
	pClipFace->m_aBlends[iClipFacePoint].m_flBlends[1] = pCoefs[1];
	pClipFace->m_aBlends[iClipFacePoint].m_flBlends[2] = pCoefs[2];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapOverlay::ClipFaceCalcBarycentricCooefs( CMapDisp *pDisp, Vector2D *pVertsUV, 
												 const Vector2D &vecPointUV, float *pCoefs )
{
	// Area in disp UV space is always the same.
	float flTotalArea = 0.5f;		
	float flOOTotalArea = 1.0f / flTotalArea;

	int nInterval = pDisp->GetWidth();
	Vector2D vecScaledPointUV = vecPointUV * ( nInterval - 1.000001f );

	Vector2D vecSegment0, vecSegment1;

	// Get the area for cooeficient 0 (pt, v1, v2).
	vecSegment0 = pVertsUV[1] - vecScaledPointUV;
	vecSegment1 = pVertsUV[2] - vecScaledPointUV;
	// Cross
	float flSubArea = ( ( vecSegment1.x * vecSegment0.y ) - ( vecSegment0.x * vecSegment1.y ) ) * 0.5f;
	pCoefs[0] = flSubArea * flOOTotalArea;

	// Get the area for cooeficient 1 (v0, pt, v2).
	vecSegment0 = vecScaledPointUV - pVertsUV[0];
	vecSegment1 = pVertsUV[2] - pVertsUV[0];
	// Cross
	flSubArea = ( ( vecSegment1.x * vecSegment0.y ) - ( vecSegment0.x * vecSegment1.y ) ) * 0.5f;
	pCoefs[1] = flSubArea * flOOTotalArea;

	// Get the area for cooeficient 2 (v0, v1, pt).
	vecSegment0 = pVertsUV[1] - pVertsUV[0];
	vecSegment1 = vecScaledPointUV - pVertsUV[0];
	// Cross
	flSubArea = ( ( vecSegment1.x * vecSegment0.y ) - ( vecSegment0.x * vecSegment1.y ) ) * 0.5f;
	pCoefs[2] = flSubArea * flOOTotalArea;

	float flCoefTotal = pCoefs[0] + pCoefs[1] + pCoefs[2];
	if ( ( flCoefTotal > 0.99f ) && ( flCoefTotal < 1.01f ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFacePostClipDisp( void )
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFaceBuildBlend( ClipFace_t *pClipFace, CMapDisp *pDisp, 
									  cplane_t *pClipPlane, int iClip, 
									  const Vector &vecUV, const Vector &vecPoint )
{
	// Get the displacement space interval.
	int nWidth = pDisp->GetWidth();
	int nHeight = pDisp->GetHeight();

	float flU = vecUV.x * ( nWidth - 1.000001f );
	float flV = vecUV.y * ( nHeight - 1.000001f );

	// find the triangle the "uv spot" resides in
	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );
	if ( nSnapU == ( nWidth - 1 ) ) { --nSnapU; }
	if ( nSnapV == ( nHeight - 1 ) ) { --nSnapV; }
	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );
		
	int iAxisType = ClipFaceGetAxisType( pClipPlane );
	switch( iAxisType )
	{
	case OVERLAY_ANGLE0:
		{
			// Vert type
			if ( fabs( flFracU ) < OVERLAY_DISPSPACE_EPSILON )
			{
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_nType = OVERLAY_BLENDTYPE_VERT;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[0] = ( nWidth * iClip ) + nSnapU;
			}
			// Edge type
			else
			{
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_nType = OVERLAY_BLENDTYPE_EDGE;
				int iPoint0 = ( nWidth * iClip ) + nSnapU;
				int iPoint1 = ( nWidth * iClip ) + nNextU;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[0] = iPoint0;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[1] = iPoint1;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_flBlends[0] = flFracU;
			}
			return;
		}
	case OVERLAY_ANGLE45:
		{
			// Vert type
			if ( ( fabs( flFracU ) < OVERLAY_DISPSPACE_EPSILON ) &&
				 ( fabs( flFracV ) < OVERLAY_DISPSPACE_EPSILON ) )
			{
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_nType = OVERLAY_BLENDTYPE_VERT;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[0] = ( nWidth * nSnapV ) + nSnapU;
			}
			// Edge type
			else
			{
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_nType = OVERLAY_BLENDTYPE_EDGE;
				int iPoint0 = ( nWidth * nNextV ) + nSnapU;
				int iPoint1 = ( nWidth * nSnapV ) + nNextU;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[0] = iPoint0;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[1] = iPoint1;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_flBlends[0] = flFracU;
			}
			return;
		}
	case OVERLAY_ANGLE90:
		{
			// Vert type
			if ( fabs( flFracV ) < OVERLAY_DISPSPACE_EPSILON )
			{
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_nType = OVERLAY_BLENDTYPE_VERT;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[0] = ( nWidth * nSnapV ) + iClip;
			}
			// Edge type
			else
			{
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_nType = OVERLAY_BLENDTYPE_EDGE;
				int iPoint0 = ( nWidth * nSnapV ) + iClip;
				int iPoint1 = ( nWidth * nNextV ) + iClip;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[0] = iPoint0;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[1] = iPoint1;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_flBlends[0] = flFracV;
			}
			return;
		}
	case OVERLAY_ANGLE135:
		{
			// Vert type
			if ( ( fabs( flFracU ) < OVERLAY_DISPSPACE_EPSILON ) &&
				 ( fabs( flFracV ) < OVERLAY_DISPSPACE_EPSILON ) )
			{
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_nType = OVERLAY_BLENDTYPE_VERT;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[0] = ( nWidth * nSnapV ) + nSnapU;
			}
			// Edge type
			else
			{
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_nType = OVERLAY_BLENDTYPE_EDGE;
				int iPoint0 = ( nWidth * nSnapV ) + nSnapU;
				int iPoint1 = ( nWidth * nNextV ) + nNextU;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[0] = iPoint0;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[1] = iPoint1;
				pClipFace->m_aBlends[pClipFace->m_nPointCount].m_flBlends[0] = flFracU;
			}
			return;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CMapOverlay::ClipFaceGetAxisType( cplane_t *pClipPlane )
{
	if ( pClipPlane->normal[0] == 1.0f ) { return OVERLAY_ANGLE90; }
	if ( pClipPlane->normal[1] == 1.0f ) { return OVERLAY_ANGLE0; }
	if ( ( pClipPlane->normal[0] == 0.707f ) && ( pClipPlane->normal[1] == 0.707f ) ) { return OVERLAY_ANGLE45; }
	if ( ( pClipPlane->normal[0] == -0.707f ) && ( pClipPlane->normal[1] == 0.707f ) ) { return OVERLAY_ANGLE135; }

	return OVERLAY_ANGLE0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFaceCopyBlendFrom( ClipFace_t *pClipFace, BlendData_t *pBlendFrom )
{
	pClipFace->m_aBlends[pClipFace->m_nPointCount].m_nType = pBlendFrom->m_nType;
	for ( int iPoint = 0; iPoint < 3; iPoint++ )
	{
		pClipFace->m_aBlends[pClipFace->m_nPointCount].m_iPoints[iPoint] = pBlendFrom->m_iPoints[iPoint];
		pClipFace->m_aBlends[pClipFace->m_nPointCount].m_flBlends[iPoint] = pBlendFrom->m_flBlends[iPoint];
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ClipFaceBuildFacesFromBlendedData( ClipFace_t *pClipFace )
{
	if( pClipFace->m_pBuildFace->HasDisp() )
	{
		EditDispHandle_t handle = pClipFace->m_pBuildFace->GetDisp();
		CMapDisp *pDisp = EditDispMgr()->GetDisp( handle );

		Vector vecPos[3];
		for ( int iPoint = 0; iPoint < pClipFace->m_nPointCount; iPoint++ )
		{
			if ( pClipFace->m_aBlends[iPoint].m_nType == OVERLAY_BLENDTYPE_VERT )
			{
				pDisp->GetVert( pClipFace->m_aBlends[iPoint].m_iPoints[0], vecPos[0] );
				pClipFace->m_aPoints[iPoint] = vecPos[0];
			}
			else if ( pClipFace->m_aBlends[iPoint].m_nType == OVERLAY_BLENDTYPE_EDGE )
			{
				pDisp->GetVert( pClipFace->m_aBlends[iPoint].m_iPoints[0], vecPos[0] );
				pDisp->GetVert( pClipFace->m_aBlends[iPoint].m_iPoints[1], vecPos[1] );
				pClipFace->m_aPoints[iPoint] = vecPos[0] + ( vecPos[1] - vecPos[0] ) * pClipFace->m_aBlends[iPoint].m_flBlends[0];
			}
			else if ( pClipFace->m_aBlends[iPoint].m_nType == OVERLAY_BLENDTYPE_BARY )
			{
				pDisp->GetVert( pClipFace->m_aBlends[iPoint].m_iPoints[0], vecPos[0] );
				pDisp->GetVert( pClipFace->m_aBlends[iPoint].m_iPoints[1], vecPos[1] );
				pDisp->GetVert( pClipFace->m_aBlends[iPoint].m_iPoints[2], vecPos[2] );
				pClipFace->m_aPoints[iPoint] = ( vecPos[0] * pClipFace->m_aBlends[iPoint].m_flBlends[0] ) +
					                           ( vecPos[1] * pClipFace->m_aBlends[iPoint].m_flBlends[1] ) +
								               ( vecPos[2] * pClipFace->m_aBlends[iPoint].m_flBlends[2] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CMapOverlay::ClipFace_t *CMapOverlay::ClipFaceCopy( ClipFace_t *pSrc )
{
	ClipFace_t *pDst = ClipFaceCreate( pSrc->m_nPointCount );
	if ( pDst )
	{
		for ( int iPoint = 0; iPoint < pSrc->m_nPointCount; iPoint++ )
		{
			pDst->m_aPoints[iPoint]       = pSrc->m_aPoints[iPoint];
			pDst->m_aDispPointUVs[iPoint] = pSrc->m_aDispPointUVs[iPoint];
			for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
				pDst->m_aTexCoords[iTexCoord][iPoint]    = pSrc->m_aTexCoords[iTexCoord][iPoint];
			
			pDst->m_aBlends[iPoint].m_nType = pSrc->m_aBlends[iPoint].m_nType;
			for ( int iBlend = 0; iBlend < 3; iBlend++ )
			{
				pDst->m_aBlends[iPoint].m_iPoints[iBlend] = pSrc->m_aBlends[iPoint].m_iPoints[iBlend];
				pDst->m_aBlends[iPoint].m_flBlends[iBlend] = pSrc->m_aBlends[iPoint].m_flBlends[iBlend];
			}
		}
	}

	return pDst;
}

//=============================================================================
//
// CMapOverlay Material Functions
//

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::MaterialInit( void )
{
	m_Material.m_pTexture = NULL;
	m_Material.m_vecTextureU.Init( 0.0f, 1.0f );
	m_Material.m_vecTextureV.Init( 0.0f, 1.0f );
	MaterialUpdateTexCoordsFromUVs();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::MaterialUpdateTexCoordsFromUVs( void )
{
	m_Material.m_vecTexCoords[0].Init( m_Material.m_vecTextureU.x, m_Material.m_vecTextureV.x );
	m_Material.m_vecTexCoords[1].Init( m_Material.m_vecTextureU.x, m_Material.m_vecTextureV.y );
	m_Material.m_vecTexCoords[2].Init( m_Material.m_vecTextureU.y, m_Material.m_vecTextureV.y );
	m_Material.m_vecTexCoords[3].Init( m_Material.m_vecTextureU.y, m_Material.m_vecTextureV.x );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::MaterialCopy( Material_t *pSrc, Material_t *pDst )
{
	pDst->m_pTexture = pSrc->m_pTexture;
	pDst->m_vecTextureU = pSrc->m_vecTextureU;
	pDst->m_vecTextureV = pSrc->m_vecTextureV;
	for ( int iHandle = 0; iHandle < OVERLAY_HANDLES_COUNT; iHandle++ )
	{
		pDst->m_vecTexCoords[iHandle] = pSrc->m_vecTexCoords[iHandle];
	}
}

//=============================================================================
//
// CMapOverlay Functions
//

//-----------------------------------------------------------------------------
// Purpose: Construct a CMapOverlay instance.
//-----------------------------------------------------------------------------
CMapOverlay::CMapOverlay() : CMapSideList( "sides" )
{
	BasisInit();
	HandlesInit();
	MaterialInit();

	m_pOverlayFace = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destruct a CMapOverlay instance.
//-----------------------------------------------------------------------------
CMapOverlay::~CMapOverlay()
{
	ClipFaceDestroy( &m_pOverlayFace );
	ClipFacesDestroy( m_aRenderFaces );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CMapClass *CMapOverlay::CreateMapOverlay( CHelperInfo *pInfo, CMapEntity *pParent )
{
	CMapOverlay *pOverlay = new CMapOverlay;
	return( pOverlay );
}

//-----------------------------------------------------------------------------
// Purpose: Called after this object is added to the world.
//			NOTE: This function is NOT called during serialization. Use PostloadWorld
//				  to do similar bookkeeping after map load.
// Input  : pWorld - The world that we have been added to.
//-----------------------------------------------------------------------------
void CMapOverlay::OnAddToWorld( CMapWorld *pWorld )
{
}

//-----------------------------------------------------------------------------
// Purpose: Called just after this object has been removed from the world so
//			that it can unlink itself from other objects in the world.
// Input  : pWorld - The world that we were just removed from.
//			bNotifyChildren - Whether we should forward notification to our children.
//-----------------------------------------------------------------------------
void CMapOverlay::OnRemoveFromWorld( CMapWorld *pWorld, bool bNotifyChildren )
{
	CMapSideList::OnRemoveFromWorld( pWorld, bNotifyChildren );
}

//-----------------------------------------------------------------------------
// Purpose: Called after the entire map has been loaded. This allows the object
//			to perform any linking with other map objects or to do other operations
//			that require all world objects to be present.
// Input  : pWorld - The world that we are in.
//-----------------------------------------------------------------------------
void CMapOverlay::PostloadWorld( CMapWorld *pWorld )
{
	CMapSideList::PostloadWorld( pWorld );
	BasisBuildFromSideList();
	HandlesBuild();
}

//-----------------------------------------------------------------------------
// Purpose: Notify me when a key has had a data change, so the overlay can
//          update itself appropriately.
//   Input: szKey - the key that changed
//          szValue - the new value (key/data pair)
//-----------------------------------------------------------------------------
void CMapOverlay::OnParentKeyChanged( LPCSTR szKey, LPCSTR szValue )
{
	// Pass this to the sidelist first.
	CMapSideList::OnParentKeyChanged( szKey, szValue );

	//
	// Read geometry data.
	//
	float flDummy;
	if ( !stricmp( szKey, "uv0" ) )     { sscanf( szValue, "%f %f %f", &m_Handles.m_vecBasisCoords[0].x, &m_Handles.m_vecBasisCoords[0].y, &flDummy ); }	// handle 1 2D
	if ( !stricmp( szKey, "uv1" ) )     { sscanf( szValue, "%f %f %f", &m_Handles.m_vecBasisCoords[1].x, &m_Handles.m_vecBasisCoords[1].y, &flDummy ); }	// handle 2 2D
	if ( !stricmp( szKey, "uv2" ) )     { sscanf( szValue, "%f %f %f", &m_Handles.m_vecBasisCoords[2].x, &m_Handles.m_vecBasisCoords[2].y, &flDummy ); }	// handle 3 2D
	if ( !stricmp( szKey, "uv3" ) )     { sscanf( szValue, "%f %f %f", &m_Handles.m_vecBasisCoords[3].x, &m_Handles.m_vecBasisCoords[3].y, &flDummy ); }	// handle 4 2D

	//
	// Read material data.
	//
	if ( !stricmp( szKey, "material" ) )
	{
		// get the new material
		IEditorTexture *pTex = g_Textures.FindActiveTexture( szValue );
		if ( !pTex )
			return;

		// save the new material
		m_Material.m_pTexture = pTex;
	}

	if ( !stricmp( szKey, "StartU" ) )	{ m_Material.m_vecTextureU.x = atof( szValue ); MaterialUpdateTexCoordsFromUVs(); }
	if ( !stricmp( szKey, "EndU" ) )	{ m_Material.m_vecTextureU.y = atof( szValue ); MaterialUpdateTexCoordsFromUVs(); }
	if ( !stricmp( szKey, "StartV" ) )	{ m_Material.m_vecTextureV.x = atof( szValue ); MaterialUpdateTexCoordsFromUVs(); }
	if ( !stricmp( szKey, "EndV" ) )	{ m_Material.m_vecTextureV.y = atof( szValue ); MaterialUpdateTexCoordsFromUVs(); }

	//
	// Read side data.
	//
	if ( !stricmp( szKey, "sides" ) )	{ BasisBuildFromSideList(); }

	// Re-clip and post "changed."
	DoClip();
	PostUpdate( Notify_Changed );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CMapClass *CMapOverlay::Copy( bool bUpdateDependencies )
{
	CMapOverlay *pCopy = new CMapOverlay;
	if ( pCopy )
	{
		pCopy->CopyFrom( this, bUpdateDependencies );
	}

	return ( pCopy );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CMapClass *CMapOverlay::CopyFrom( CMapClass *pObject, bool bUpdateDependencies )
{
	// Verify the object is of the correct type and cast.
	ASSERT( pObject->IsMapClass( MAPCLASS_TYPE( CMapOverlay ) ) );
	CMapOverlay *pFrom = ( CMapOverlay* )pObject;
	if ( pFrom )
	{
		// Copy the parent class data.
		CMapSideList::CopyFrom( pObject, bUpdateDependencies );

		// Copy basis data.
		BasisCopy( &pFrom->m_Basis, &m_Basis );

		// Copy handle data.
		HandlesCopy( &pFrom->m_Handles, &m_Handles );

		// Copy material data.
		MaterialCopy( &pFrom->m_Material, &m_Material );

		// Copy the face list.
//		m_aRenderFaces.CopyArray( pFrom->m_aRenderFaces.Base(), pFrom->m_aRenderFaces.Count() );
	}

	return ( this );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::OnUndoRedo( void )
{
	PostModified();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::CalcBounds( BOOL bFullUpdate )
{
	// Pass the info along.
	CMapSideList::CalcBounds( bFullUpdate );

	//
	// Calculate the 2d bounds.
	//
	m_Render2DBox.ResetBounds();
	Vector vecMins, vecMaxs;
	vecMins = m_Origin - Vector( 2.0f, 2.0f, 2.0f );
	vecMaxs = m_Origin + Vector( 2.0f, 2.0f, 2.0f );
	m_Render2DBox.UpdateBounds( vecMins, vecMaxs );

	//
	// Calculate the 3d bounds.
	//
	m_CullBox.ResetBounds();

	//
	// If we don't have an attached face then we don't exist in 3d,
	// just update the aligned box.
	//
	int nFaceCount = m_aRenderFaces.Count();
	if ( nFaceCount > 0 )
	{
		for ( int iFace = 0; iFace < nFaceCount; iFace++ )
		{
			ClipFace_t *pRenderFace = m_aRenderFaces.Element( iFace );
			if ( pRenderFace )
			{
				Vector vecBoundsMin, vecBoundsMax;
				ClipFaceGetBounds( pRenderFace, vecBoundsMin, vecBoundsMax );
				m_CullBox.UpdateBounds( vecBoundsMin, vecBoundsMax );
			}
		}
		
		// Add a little slop.
		for( int iAxis = 0; iAxis < 3; iAxis++ )
		{
			if( ( m_CullBox.bmaxs[iAxis] - m_CullBox.bmins[iAxis] ) == 0.0f )
			{
				m_CullBox.bmins[iAxis] -= 0.5f;
				m_CullBox.bmaxs[iAxis] += 0.5f;
			}
		}
	}
	else
	{
		m_CullBox.UpdateBounds( vecMins, vecMaxs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapOverlay::PostModified( void )
{
	HandlesBuild();
	DoClip();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTransBox - 
//-----------------------------------------------------------------------------
void CMapOverlay::DoTransform( Box3D *pTransBox )
{
	CMapSideList::DoTransform( pTransBox );

	// Handle translation
	if ( pTransBox->IsMoving() )
	{
		BasisSetOrigin();
	}
	// Handle rotation
	else if ( pTransBox->IsRotating() )
	{
		QAngle vecAngles;
		pTransBox->GetRotateAngles(	vecAngles );
		HandlesRotate( vecAngles );
	}
	// Handle skew
	else if ( pTransBox->IsShearing() )
	{
		BasisSetOrigin();
	}
	
	PostModified();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//-----------------------------------------------------------------------------
void CMapOverlay::DoTransFlip( const Vector &RefPoint )
{
	CMapSideList::DoTransFlip( RefPoint );
	HandlesFlip( RefPoint );
	PostModified();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Delta - 
//-----------------------------------------------------------------------------
void CMapOverlay::DoTransMove( const Vector &Delta )
{
	CMapSideList::DoTransMove( Delta );
	BasisSetOrigin();
	PostModified();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRefPoint - 
//			Angles - 
//-----------------------------------------------------------------------------
void CMapOverlay::DoTransRotate( const Vector *pRefPoint, const QAngle &Angles )
{
	CMapSideList::DoTransRotate( pRefPoint, Angles );
	HandlesRotate( Angles );
	PostModified();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//			Scale - 
//-----------------------------------------------------------------------------
void CMapOverlay::DoTransScale( const Vector &RefPoint, const Vector &Scale )
{
	CMapSideList::DoTransScale( RefPoint, Scale );
	BasisSetOrigin();
	PostModified();
}

//-----------------------------------------------------------------------------
// Purpose: Notifies us that a copy of ourselves was pasted.
//-----------------------------------------------------------------------------
void CMapOverlay::OnPaste( CMapClass *pCopy, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, 
						   CMapObjectList &OriginalList, CMapObjectList &NewList)
{
	//
	// NOTE: currently pCopy is the Overlay being pasted into the world, "this" is
	//       what is being copied from
	//
	CMapSideList::OnPaste( pCopy, pSourceWorld, pDestWorld, OriginalList, NewList );
	CMapOverlay *pOverlay = dynamic_cast<CMapOverlay*>( pCopy );
	if ( pOverlay )
	{
		pOverlay->BasisBuildFromSideList();
		pOverlay->HandlesBuild();
		pOverlay->PostModified();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Notifies us that we created a copy of ourselves (a clone).
//-----------------------------------------------------------------------------
void CMapOverlay::OnClone( CMapClass *pClone, CMapWorld *pWorld, 
						   CMapObjectList &OriginalList, CMapObjectList &NewList )
{
	CMapSideList::OnClone( pClone, pWorld, OriginalList, NewList );
	CMapOverlay *pOverlay = dynamic_cast<CMapOverlay*>( pClone );
	if ( pOverlay )
	{
		pOverlay->BasisBuildFromSideList();
		pOverlay->HandlesBuild();
		pOverlay->PostModified();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Notifys this decal of a change to a solid that it is attached to.
//-----------------------------------------------------------------------------
void CMapOverlay::OnNotifyDependent( CMapClass *pObject, Notify_Dependent_t eNotifyType )
{
	CMapSideList::OnNotifyDependent( pObject, eNotifyType );

	//
	// NOTE: the solid moving (changing) can update the overlay/solid(face) dependency
	//       so "rebuild" the overlay
	//
	if ( ( eNotifyType == Notify_Changed ) || ( eNotifyType == Notify_Undo ) ||
		 ( eNotifyType == Notify_Transform ) )
	{
		PostModified();
	}

	if( eNotifyType == Notify_Flipped )
	{
		// do nothing
	}

	if( eNotifyType == Notify_Removed )
	{
		// do nothing -- handles in the CMapSideList notify
	}

	// For displacements -- re-calculate yourself barycentrically.
	if( eNotifyType == Notify_Rebuild )
	{
		ReBuild();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::Render3D( CRender3D *pRender )
{
	if ( m_aRenderFaces.Count() != 0 )
	{
		// Default state.
		pRender->SetRenderMode( RENDER_MODE_FLAT );
		
		// Bind the matrial -- if there is one!!
		bool bTextured = false;
		if ( m_Material.m_pTexture )
		{
			pRender->BindTexture( m_Material.m_pTexture );
			pRender->SetRenderMode( RENDER_MODE_TEXTURED );
			bTextured = true;
		}
		
		int nFaceCount = m_aRenderFaces.Count();
		for ( int iFace = 0; iFace < nFaceCount; iFace++ )
		{
			ClipFace_t *pRenderFace = m_aRenderFaces.Element( iFace );
			if( !pRenderFace )
				continue;
			
			MaterialPrimitiveType_t type = MATERIAL_POLYGON;
			
			// Get a dynamic mesh.
			CMeshBuilder meshBuilder;
			IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh( );
			
			meshBuilder.Begin( pMesh, type, pRenderFace->m_nPointCount );
			for ( int iPoint = 0; iPoint < pRenderFace->m_nPointCount; iPoint++ )
			{
				if ( !bTextured )
				{
					meshBuilder.Color3ub( 0, 128, 0 );
				}
				else
				{
					meshBuilder.TexCoord2f( 0, pRenderFace->m_aTexCoords[0][iPoint].x, pRenderFace->m_aTexCoords[0][iPoint].y );
					meshBuilder.TexCoord2f( 2, pRenderFace->m_aTexCoords[1][iPoint].x, pRenderFace->m_aTexCoords[1][iPoint].y );
				}
				meshBuilder.Position3f( pRenderFace->m_aPoints[iPoint].x, pRenderFace->m_aPoints[iPoint].y, pRenderFace->m_aPoints[iPoint].z );
				meshBuilder.AdvanceVertex();
			}
			meshBuilder.End();
			
			pMesh->Draw();
		}
		
		// Render wireframe on top when seleted.
		if ( GetSelectionState() == SELECT_NORMAL )
		{
			pRender->SetRenderMode( RENDER_MODE_FLAT );
			for ( iFace = 0; iFace < nFaceCount; iFace++ )
			{
				ClipFace_t *pRenderFace = m_aRenderFaces.Element( iFace );
				if( !pRenderFace )
					continue;
				
				MaterialPrimitiveType_t type = MATERIAL_LINE_LOOP;
				
				// get a dynamic mesh
				CMeshBuilder meshBuilder;
				IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh( );
				
				meshBuilder.Begin( pMesh, type, pRenderFace->m_nPointCount );
				for( int iPoint = 0; iPoint < pRenderFace->m_nPointCount; iPoint++ )
				{
					meshBuilder.Color3ub( 0, 255, 0 );
					meshBuilder.Position3f( pRenderFace->m_aPoints[iPoint].x, pRenderFace->m_aPoints[iPoint].y, pRenderFace->m_aPoints[iPoint].z );
					meshBuilder.AdvanceVertex();
				}
				meshBuilder.End();
				
				pMesh->Draw();
			}
		}
	}

	// Render the handles - if selected or in overlay tool mode.
	if ( ( g_pToolManager->GetToolID() == TOOL_OVERLAY ) && m_Basis.m_pFace )
	{
		HandlesRender( pRender );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clip the overlay "face" to all of the faces in the overlay sidelist.
//          The sidelist defines all faces affected by the "overlay."
//-----------------------------------------------------------------------------
void CMapOverlay::DoClip( void )
{
	// Check to see if we have any faces to clip against.
	int nFaceCount = m_Faces.Count();
	if( nFaceCount == 0 )
		return;

	// Destroy the render face cache.
	ClipFacesDestroy( m_aRenderFaces );
//	m_aRenderFaces.Purge();

	// clip the overlay against all faces in the sidelist
	for ( int iFace = 0; iFace < nFaceCount; iFace++ )
	{
		CMapFace *pFace = m_Faces.Element( iFace );
		if ( pFace )
		{
			PreClip();
			DoClipFace( pFace );
			PostClip();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::PreClip( void )
{
	//
	// Create the initial face to be clipped - the overlay.
	//
	m_pOverlayFace = ClipFaceCreate( OVERLAY_HANDLES_COUNT );
	if ( m_pOverlayFace )
	{
		for ( int iPoint = 0; iPoint < OVERLAY_HANDLES_COUNT; iPoint++ )
		{
			OverlayUVToOverlayPlane( m_Handles.m_vecBasisCoords[iPoint], m_pOverlayFace->m_aPoints[iPoint] );
			m_pOverlayFace->m_aTexCoords[0][iPoint] = m_Material.m_vecTexCoords[iPoint];

			if ( m_Basis.m_pFace->HasDisp() )
			{
				EditDispHandle_t handle = m_Basis.m_pFace->GetDisp();
				CMapDisp *pDisp = EditDispMgr()->GetDisp( handle );
				Vector2D vecTmp;
				pDisp->BaseFacePlaneToDispUV( m_pOverlayFace->m_aPoints[iPoint], vecTmp );
				m_pOverlayFace->m_aDispPointUVs[iPoint].x = vecTmp.x;
				m_pOverlayFace->m_aDispPointUVs[iPoint].y = vecTmp.y;
				m_pOverlayFace->m_aDispPointUVs[iPoint].z = 0.0f;
			}
		}
		// The second set of texcoords on the overlay is used for alpha by certain shaders,
		// and they want to stretch the texture across the whole overlay.
		m_pOverlayFace->m_aTexCoords[1][0].Init( 0, 0 );
		m_pOverlayFace->m_aTexCoords[1][1].Init( 0, 1 );
		m_pOverlayFace->m_aTexCoords[1][2].Init( 1, 1 );
		m_pOverlayFace->m_aTexCoords[1][3].Init( 1, 0 );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::PostClip( void )
{
	ClipFaceDestroy( &m_pOverlayFace );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::DoClipFace( CMapFace *pFace )
{
	// Valid face?
	Assert( pFace != NULL );
	if( !pFace )
		return;

	// Copy the original overlay to the "clipped" overlay.
	ClipFace_t *pClippedFace = ClipFaceCopy( m_pOverlayFace );
	if ( !pClippedFace )
		return;

	//
	// Project all face points into the overlay plane.
	//
	int nPointCount = pFace->nPoints;
	Vector *pPoints = new Vector[nPointCount];
	int	nEdgePlaneCount = nPointCount;
	cplane_t *pEdgePlanes = new cplane_t[nEdgePlaneCount];
	if ( !pPoints || !pEdgePlanes )
	{
		delete [] pPoints;
		delete [] pEdgePlanes;
		return;
	}

	for ( int iPoint = 0; iPoint < nPointCount; iPoint++ )
	{
		WorldToOverlayPlane( pFace->Points[iPoint], pPoints[iPoint] );
	}

	// Create the face clipping planes (edges cross overlay plane normal).
	BuildEdgePlanes( pPoints, nPointCount, pEdgePlanes, nEdgePlaneCount );

	//
	// Clip overlay against all the edge planes.
	//
	for ( int iClipPlane = 0; iClipPlane < nEdgePlaneCount; iClipPlane++ )
	{
		ClipFace_t *pFront = NULL;
		ClipFace_t *pBack = NULL;

		if ( pClippedFace )
		{
			// Clip the overlay and delete the data (we are done with it - we are only interested in what is left).
			ClipFaceClip( pClippedFace, &pEdgePlanes[iClipPlane], OVERLAY_WORLDSPACE_EPSILON, &pFront, &pBack );
			ClipFaceDestroy( &pClippedFace );

			// Keep the backside -- if it exists and continue clipping.
			if ( pBack )
			{
				pClippedFace = pBack;
			}

			// Destroy the front side -- if it exists.
			if ( pFront )
			{
				ClipFaceDestroy( &pFront );
			}
		}
	}

	//
	// Free temporary memory (clip planes and point).
	//
	delete [] pPoints;
	delete [] pEdgePlanes;


	//
	// If it exists, move points from the overlay plane back into
	// the base face plane.
	//
	if ( !pClippedFace )
		return;

	for ( iPoint = 0; iPoint < pClippedFace->m_nPointCount; iPoint++ )
	{
		Vector2D vecUV;
		PointInQuadToBarycentric( m_pOverlayFace->m_aPoints[0], m_pOverlayFace->m_aPoints[3], 
			                      m_pOverlayFace->m_aPoints[2], m_pOverlayFace->m_aPoints[1],
								  pClippedFace->m_aPoints[iPoint], vecUV );

		Vector vecTmp;
		OverlayPlaneToWorld( pFace, pClippedFace->m_aPoints[iPoint], vecTmp );
		pClippedFace->m_aPoints[iPoint] = vecTmp;

		Vector2D vecTexCoord;
		for ( int iTexCoord=0; iTexCoord < NUM_CLIPFACE_TEXCOORDS; iTexCoord++ )
		{
			TexCoordInQuadFromBarycentric( m_pOverlayFace->m_aTexCoords[iTexCoord][0], m_pOverlayFace->m_aTexCoords[iTexCoord][3],
										m_pOverlayFace->m_aTexCoords[iTexCoord][2], m_pOverlayFace->m_aTexCoords[iTexCoord][1],
										vecUV, vecTexCoord );
			
			pClippedFace->m_aTexCoords[iTexCoord][iPoint] = vecTexCoord;
		}
	}

	//
	// If the face has a displacement map -- continue clipping.
	//
	if( pFace->HasDisp() )
	{
		DoClipDisp( pFace, pClippedFace );
	}
	// Done - save it!
	else
	{
		pClippedFace->m_pBuildFace = pFace;
		m_aRenderFaces.AddToTail( pClippedFace );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapOverlay::BuildEdgePlanes( Vector const *pPoints, int nPointCount,
								   cplane_t *pEdgePlanes, int nEdgePlaneCount )
{
	for ( int iPoint = 0; iPoint < nPointCount; iPoint++ )
	{
		Vector vecEdge;
		vecEdge = pPoints[(iPoint+1)%nPointCount] - pPoints[iPoint];
		VectorNormalize( vecEdge );

		pEdgePlanes[iPoint].normal = m_Basis.m_vecAxes[OVERLAY_BASIS_NORMAL].Cross( vecEdge );
		pEdgePlanes[iPoint].dist = pEdgePlanes[iPoint].normal.Dot( pPoints[iPoint] );

		// Check normal facing.
		float flDist = pEdgePlanes[iPoint].normal.Dot( pPoints[(iPoint+2)%nPointCount] ) - pEdgePlanes[iPoint].dist;
		if( flDist > 0.0f )
		{
			// flip
			pEdgePlanes[iPoint].normal.Negate();
			pEdgePlanes[iPoint].dist = -pEdgePlanes[iPoint].dist;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMapOverlay::Disp_ClipFragments( CMapDisp *pDisp, ClipFaces_t &aDispFragments )
{
	cplane_t clipPlane;

	// Cache the displacement interval.
	int nInterval = pDisp->GetWidth() - 1;

	// Displacement-space clipping in V.
	clipPlane.normal.Init( 1.0f, 0.0f, 0.0f );
	Disp_DoClip( pDisp, aDispFragments, clipPlane, 1.0f, nInterval, 1, nInterval, 1 );

	// Displacement-space clipping in U.
	clipPlane.normal.Init( 0.0f, 1.0f, 0.0f );
	Disp_DoClip( pDisp, aDispFragments, clipPlane, 1.0f, nInterval, 1, nInterval, 1 );

	// Displacement-space clipping UV from top-left to bottom-right.
	clipPlane.normal.Init( 0.707f, 0.707f, 0.0f );  // 45 degrees
	Disp_DoClip( pDisp, aDispFragments, clipPlane, 0.707f, nInterval, 2, ( nInterval * 2 - 1 ), 2 );

	// Displacement-space clipping UV from bottom-left to top-right.
	clipPlane.normal.Init( -0.707f, 0.707f, 0.0f );  // 135 degrees
	Disp_DoClip( pDisp, aDispFragments, clipPlane, 0.707f, nInterval, -( nInterval - 2 ), ( nInterval - 1 ), 2 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMapOverlay::Disp_DoClip( CMapDisp *pDisp, ClipFaces_t &aDispFragments,
							   cplane_t &clipPlane, float clipDistStart, int nInterval,
							   int nLoopStart, int nLoopEnd, int nLoopInc )
{
	// Setup interval information.
	float flInterval = static_cast<float>( nInterval );
	float flOOInterval = 1.0f / flInterval;

	// Holds the current set of clipped faces.
	ClipFaces_t aClippedFragments;

	for ( int iInterval = nLoopStart; iInterval < nLoopEnd; iInterval += nLoopInc )
	{
		// Copy the current list to clipped face list.
		aClippedFragments.CopyArray( aDispFragments.Base(), aDispFragments.Count() );
		aDispFragments.Purge();

		// Clip in V.
		int nFragCount = aClippedFragments.Count();
		for ( int iFrag = 0; iFrag < nFragCount; iFrag++ )
		{
			ClipFace_t *pClipFrag = aClippedFragments[iFrag];
			if ( pClipFrag )
			{
				ClipFace_t *pFront = NULL, *pBack = NULL;

				clipPlane.dist = clipDistStart * ( ( float )iInterval * flOOInterval );
				ClipFaceClipBarycentric( pClipFrag, &clipPlane, OVERLAY_DISPSPACE_EPSILON, iInterval, pDisp, &pFront, &pBack );
				ClipFaceDestroy( &pClipFrag );

				if ( pFront )
				{
					aDispFragments.AddToTail( pFront );
				}

				if ( pBack )
				{
					aDispFragments.AddToTail( pBack );
				}
			}
		}
	}

	// Clean up!
	aClippedFragments.Purge();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::DoClipDisp( CMapFace *pFace, ClipFace_t *pClippedFace )
{
	// Get the displacement data.
	EditDispHandle_t handle = pFace->GetDisp();
	CMapDisp *pDisp = EditDispMgr()->GetDisp( handle );

	// Initialize local clip data.
	ClipFacePreClipDisp( pClippedFace, pDisp );

	// Setup clipped face lists.
	ClipFaces_t aCurrentFaces;
	aCurrentFaces.AddToTail( pClippedFace );

	Disp_ClipFragments( pDisp, aCurrentFaces );

	//
	// Project points back onto the displacement surface.
	//
	int nFaceCount = aCurrentFaces.Count();
	for( int iFace = 0; iFace < nFaceCount; iFace++ )
	{	
		ClipFace_t *pClipFace = aCurrentFaces[iFace];
		if ( pClipFace )
		{
			// Save for re-building later!
			pClipFace->m_pBuildFace = pFace;
			m_aRenderFaces.AddToTail( aCurrentFaces[iFace] );
			ClipFaceBuildFacesFromBlendedData( pClipFace );
		}
	}

	// Clean up!
	aCurrentFaces.Purge();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesReset( void )
{
	m_Handles.m_iHit = -1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapOverlay::HandlesHitTest( CPoint const &pt )
{
	Vector2D  vScreenPt;
	for ( int iPoint = 0; iPoint < 4; iPoint++ )
	{
		CRect rect;
		rect.SetRect( m_Handles.m_vec2D[iPoint].x - OVERLAY_HANDLES_BLOATEDSIZE, m_Handles.m_vec2D[iPoint].y - OVERLAY_HANDLES_BLOATEDSIZE,
			          m_Handles.m_vec2D[iPoint].x + OVERLAY_HANDLES_BLOATEDSIZE, m_Handles.m_vec2D[iPoint].y + OVERLAY_HANDLES_BLOATEDSIZE );
		if( rect.PtInRect( pt ) )
		{
			m_Handles.m_iHit = iPoint;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::HandlesDragTo( Vector &vecImpact, CMapFace *pFace )
{
	// Check handle index range.
	if ( ( m_Handles.m_iHit < 0 ) || ( m_Handles.m_iHit > 3 ) )
		return;

	// Only snap if the surface isn't displaced.
	if ( !pFace->HasDisp() )
	{
		// Get the active map doc.
		CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
		if ( pDoc )
		{			
			// Snap the points.
			pDoc->Snap( vecImpact );
		}
	}
	
	// Save
	m_Handles.m_vec3D[m_Handles.m_iHit] = vecImpact;

	// Project the point into the overlay plane (from face/disp).
	Vector vecOverlay;
	Vector2D vecUVOverlay;
	HandlesSurfToOverlayPlane( pFace, vecImpact, vecOverlay );
	OverlayPlaneToOverlayUV( vecOverlay, vecUVOverlay );
	m_Handles.m_vecBasisCoords[m_Handles.m_iHit] = vecUVOverlay;

	// Update the property box.
	HandlesUpdatePropertyBox();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::ReBuild( void )
{
	//
	// Project points back onto the displacement surface.
	//
	int nFaceCount = m_aRenderFaces.Count();
	for ( int iFace = 0; iFace < nFaceCount; iFace++ )
	{
		// Get the current face and remove it from the list.
		ClipFace_t *pClipFace = m_aRenderFaces[iFace];
		if ( pClipFace )
		{
			if ( pClipFace->m_pBuildFace->HasDisp() )
			{
				ClipFaceBuildFacesFromBlendedData( pClipFace );
			}
		}
	}

	// Update the entity position.
	UpdateParentEntityOrigin();

	// Update the handles.
	HandlesBuild();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::UpdateParentEntityOrigin( void )
{
	Vector vecSurf;
	if( m_Basis.m_pFace->HasDisp() )
	{
		EditDispHandle_t handle = m_Basis.m_pFace->GetDisp();
		CMapDisp *pDisp = EditDispMgr()->GetDisp( handle );

		Vector2D vecTmp;
		pDisp->BaseFacePlaneToDispUV( m_Basis.m_vecOrigin, vecTmp );
		pDisp->DispUVToSurf( vecTmp, vecSurf );
	}
	else
	{
		vecSurf = m_Basis.m_vecOrigin;
	}

	// Update the entity's origin
	CMapEntity *pEntity = ( CMapEntity* )GetParent();
	if ( pEntity )
	{
		pEntity->SetOrigin( vecSurf );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::GetPlane( cplane_t &plane )
{
	plane.normal = m_Basis.m_vecCacheAxes[OVERLAY_BASIS_NORMAL];
	plane.dist = plane.normal.Dot( m_Basis.m_vecOrigin );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::GetHandlePos( int iHandle, Vector &vecPos )
{
	Assert( iHandle >= 0 );
	Assert( iHandle < 4 );

	vecPos = m_Handles.m_vec3D[iHandle];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::AddInitialFaceToSideList( CMapFace *pFace )
{
	// Valid face?
	if ( !pFace )
		return;

	// Purge side list as this should be the initial face!
	m_Faces.PurgeAndDeleteElements();

	m_Faces.AddToTail( pFace );
	UpdateDependency( NULL, ( CMapSolid* )pFace->GetParent() );
	UpdateParentKey();

	BasisBuild( pFace );
	PostModified();			// BuildHandles, Clip, etc.
}

//=============================================================================
//
// Overlay Utility Functions
//

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::OverlayUVToOverlayPlane( Vector2D const &vecUV, Vector &vecPoint )
{
	vecPoint = ( vecUV.x * m_Basis.m_vecCacheAxes[OVERLAY_BASIS_U] +
		         vecUV.y * m_Basis.m_vecCacheAxes[OVERLAY_BASIS_V] );
	vecPoint += m_Basis.m_vecOrigin;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::OverlayPlaneToOverlayUV( Vector const &vecPoint, Vector2D &vecUV )
{
	Vector vecTmp;
	vecTmp = vecPoint - m_Basis.m_vecOrigin;
	vecUV.x = m_Basis.m_vecCacheAxes[OVERLAY_BASIS_U].Dot( vecTmp );
	vecUV.y = m_Basis.m_vecCacheAxes[OVERLAY_BASIS_V].Dot( vecTmp );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::WorldToOverlayPlane( Vector const &vecWorld, Vector &vecPoint )
{
	Vector vecDelta = vecWorld - m_Basis.m_vecOrigin;
	float flDist = m_Basis.m_vecCacheAxes[OVERLAY_BASIS_NORMAL].Dot( vecDelta );
	vecPoint = vecWorld - ( flDist * m_Basis.m_vecCacheAxes[OVERLAY_BASIS_NORMAL] );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::OverlayPlaneToBaseFacePlane( CMapFace *pFace, Vector const &vecPoint, 
											   Vector &vecBasePoint )
{
	Vector vecRayStart = vecPoint - ( m_Basis.m_vecCacheAxes[OVERLAY_BASIS_NORMAL] * 2048.0f );
	Vector vecRayEnd = vecPoint + ( m_Basis.m_vecCacheAxes[OVERLAY_BASIS_NORMAL] * 2048.0f );

	Vector vecNormal;
	pFace->TraceLineInside( vecBasePoint, vecNormal, vecRayStart, vecRayEnd, true );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapOverlay::OverlayPlaneToWorld( CMapFace *pFace, const Vector &vecPlane,
									   Vector &vecWorld )
{
	Vector vecFaceNormal, vecFacePoint;
	pFace->GetFaceNormal( vecFaceNormal );
	pFace->GetPoint( vecFacePoint, 0 );
	VectorNormalize( vecFaceNormal );
	float flPlaneDist = vecFaceNormal.Dot( vecFacePoint );
	float flDistanceToSurface =vecFaceNormal.Dot( vecPlane ) - flPlaneDist;

	float flDistance = flDistanceToSurface;
	float flDot = vecFaceNormal.Dot( m_Basis.m_vecCacheAxes[OVERLAY_BASIS_NORMAL] );
	if ( flDot != 0.0f )
	{
		flDistance = ( 1.0f / vecFaceNormal.Dot( m_Basis.m_vecCacheAxes[OVERLAY_BASIS_NORMAL] ) ) * flDistanceToSurface;
	}

	vecWorld = vecPlane - ( m_Basis.m_vecCacheAxes[OVERLAY_BASIS_NORMAL] * flDistance );
}
