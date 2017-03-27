//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdafx.h>
#include "tgaloader.h"
#include "ChunkFile.h"
#include "MapDefs.h"
#include "MapDisp.h"
#include "MapDoc.h"
#include "MapFace.h"
#include "MapSolid.h"
#include "MapWorld.h"
#include "MainFrm.h"
#include "GlobalFunctions.h"
#include "SaveInfo.h"
#include "TextureSystem.h"
#include "materialsystem/IMesh.h"
#include "Material.h"
#include "CollisionUtils.h"
#include "CModel.h"
#include "History.h"
//#include ".\dbg\dbg.h"
#include "ToolSelection.h"		// FIXME: remove, use document interface instead
#include "ToolDisplace.h"
#include "ToolManager.h"

#define OVERLAY_CHECK_BLOAT		16.0f

bool CMapDisp::m_bSelectMask = false;
bool CMapDisp::m_bGridMask = false;

//-----------------------------------------------------------------------------
// Purpose : CMapDisp constructor
//-----------------------------------------------------------------------------
CMapDisp::CMapDisp()
{
	// clear neighbor data
	ResetNeighbors();

	//
	// initialize the hit indices
	//
	ResetTexelHitIndex();
	ResetDispMapHitIndex();

	m_bHasMappingAxes = false;
	VectorClear( m_MapAxes[0] );
	VectorClear( m_MapAxes[1] );

	m_Scale = 1.0f;

	m_bSubdiv = false;
	m_bReSubdiv = false;

	m_CoreDispInfo.InitDispInfo( 4, 0, 0, NULL, NULL, NULL ); 
	Paint_Init( DISPPAINT_CHANNEL_POSITION );
}


//-----------------------------------------------------------------------------
// Purpose : CMapDisp deconstructor
//-----------------------------------------------------------------------------
CMapDisp::~CMapDisp()
{
	m_aWalkableVerts.Purge();
	m_aWalkableIndices.Purge();
	m_aForcedWalkableIndices.Purge();

	m_aBuildableVerts.Purge();
	m_aBuildableIndices.Purge();
	m_aForcedBuildableIndices.Purge();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::InitDispSurfaceData( CMapFace *pFace, bool bGenerateStartPoint )
{
	//
	// verify face is a "quad"
	//
	int pointCount = pFace->GetPointCount();
	if( pointCount != 4 )
		return false;

	// get the displacement surface 
	CCoreDispSurface *pSurf = m_CoreDispInfo.GetSurface();

	//
	// set face point data - pos, normal, texture, etc....
	//
	Vector v3;
	Vector2D v2;
	pSurf->SetPointCount( 4 );
	for( int i = 0; i < 4; i++ )
	{
		// position
		pFace->GetPoint( v3, i );
		pSurf->SetPoint( i, v3 );

		// normal
		pFace->GetFaceNormal( v3 );
		pSurf->SetPointNormal( i, v3 );

		// texture coords
		pFace->GetTexCoord( v2, i );
		pSurf->SetTexCoord( i, v2 );

		// luxel coords
		if( i == 0 )
		{
			for( int j = 0; j < 4; j++ )
			{
				pFace->GetLightmapCoord( v2, j );
				pSurf->SetLuxelCoord( i, j, v2 );
			}
		}
	}

	//
	// get displacement surface point start index
	//
	int pointStartIndex = pSurf->GetPointStartIndex();
	if( m_bHasMappingAxes && ( pointStartIndex == -1 ) )
	{
		pSurf->GeneratePointStartIndexFromMappingAxes( m_MapAxes[0], m_MapAxes[1] );
	}
	else
	{
		if( bGenerateStartPoint )
		{
			pSurf->GenerateSurfPointStartIndex();
		}
		else
		{
			pSurf->FindSurfPointStartIndex();
		}
	}
	pSurf->AdjustSurfPointData();

	// reset the has mapping axes flag (surface has been created! - use new method now)
	m_bHasMappingAxes = false;

	// set the s and t texture mapping axes so that tangent spaces can be calculated
	pSurf->SetSAxis( pFace->texture.UAxis.AsVector3D() );
	pSurf->SetTAxis( pFace->texture.VAxis.AsVector3D() );

	// successful init
	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::ResetFieldData( void )
{
	ResetFieldVectors();
	ResetFieldDistances();
	ResetSubdivPositions();
	ResetSubdivNormals();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::InitData( int power )
{
	// set surface "power" (defines size)
	SetPower( power );

	// clear vector field distances, subdiv positions and normals
	ResetFieldData();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::Create( void )
{
	if ( m_CoreDispInfo.CreateWithoutLOD() )
	{
		PostCreate();
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::PostCreate( void )
{
	UpdateBoundingBox();
	UpdateNeighborDependencies( false );
	UpdateLightmapExtents();
	UpdateWalkable();
	UpdateBuildable();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CMapDisp *CMapDisp::CopyFrom( CMapDisp *pMapDisp, bool bUpdateDependencies )
{
	//
	// check for valid displacement to copy from
	//
    if( !pMapDisp )
        return NULL;

	//
	// copy the base surface data - positions, normals, texture coords, etc...
	//
	CCoreDispSurface *pFromSurf = pMapDisp->m_CoreDispInfo.GetSurface();
	CCoreDispSurface *pToSurf = m_CoreDispInfo.GetSurface();

	int pointCount = pFromSurf->GetPointCount();
	pToSurf->SetPointCount( pointCount );

	Vector2D v2;
	Vector v3;
	for( int i = 0; i < pointCount; i++ )
	{
		pFromSurf->GetPoint( i, v3 );
		pToSurf->SetPoint( i, v3 );

		pFromSurf->GetPointNormal( i, v3 );
		pToSurf->SetPointNormal( i, v3 );

		pFromSurf->GetTexCoord( i, v2 );
		pToSurf->SetTexCoord( i, v2 );

		pFromSurf->GetLuxelCoord( 0, i, v2 );
		pToSurf->SetLuxelCoord( 0, i, v2 );
	}

	pToSurf->SetFlags( pFromSurf->GetFlags() );
	pToSurf->SetContents( pFromSurf->GetContents() );
	pToSurf->SetPointStartIndex( pFromSurf->GetPointStartIndex() );

	//
	// copy displacement surface data
	//
	SetPower( pMapDisp->GetPower() );
	SetElevation( pMapDisp->GetElevation() );

	// save the scale -- don't want to rescale!!
	m_Scale = pMapDisp->GetScale();

	int size = GetSize();
	for( i = 0; i < size; i++ )
	{
		pMapDisp->GetFieldVector( i, v3 );
		SetFieldVector( i, v3 );

		pMapDisp->GetSubdivPosition( i, v3 );
		SetSubdivPosition( i, v3 );

		pMapDisp->GetSubdivNormal( i, v3 );
		SetSubdivNormal( i, v3 );

		SetFieldDistance( i, pMapDisp->GetFieldDistance( i ) );

		pMapDisp->GetVert( i, v3 );
		SetVert( i, v3 );

		SetAlpha( i, pMapDisp->GetAlpha( i ) );
	}

	int renderCount = pMapDisp->m_CoreDispInfo.GetRenderIndexCount();
	m_CoreDispInfo.SetRenderIndexCount( renderCount );
	for( i = 0; i < renderCount; i++ )
	{
		m_CoreDispInfo.SetRenderIndex( i, pMapDisp->m_CoreDispInfo.GetRenderIndex( i ) );
	}

	// Copy the triangle data.
	int nTriCount = GetTriCount();
	for ( int iTri = 0; iTri < nTriCount; ++iTri )
	{
		unsigned short triIndices[3];
		pMapDisp->GetTriIndices( iTri, triIndices[0], triIndices[1], triIndices[2] );
		m_CoreDispInfo.SetTriIndices( iTri, triIndices[0], triIndices[1], triIndices[2] );

		unsigned short triValue = pMapDisp->m_CoreDispInfo.GetTriTagValue( iTri );
		m_CoreDispInfo.SetTriTagValue( iTri, triValue );
	}

	//
	// copy worldcraft specific data
	//
	m_bSubdiv = pMapDisp->IsSubdivided();
	m_bReSubdiv = pMapDisp->NeedsReSubdivision();

	ResetTexelHitIndex();
	ResetDispMapHitIndex();
	ResetTouched();

	//
	// re-build the surface??? an undo, etc...
	//
	if( bUpdateDependencies )
	{
		UpdateData();
	}

    return this;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpdateSurfData( CMapFace *pFace )
{
	InitDispSurfaceData( pFace, false );
	Create();
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpdateSurfDataAndVectorField( CMapFace *pFace )
{
	InitDispSurfaceData( pFace, false );

//	ResetFieldVectors();
	ResetSubdivPositions();
	ResetSubdivNormals();

	Create();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpdateData( void )
{
	Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpdateDataAndNeighborData( void )
{
	// update itself
	Create();

	// update neighbors
	for( int i = 0; i < 4; i++ )
	{
		EditDispHandle_t handle = GetEdgeNeighbor( i );
		if( handle != EDITDISPHANDLE_INVALID )
		{
			CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
			pNeighborDisp->UpdateData();
		}

		int cornerCount = GetCornerNeighborCount( i );
		if( cornerCount > 0 )
		{
			for( int j = 0; j < cornerCount; j++ )
			{
				handle = GetCornerNeighbor( i, j );
				if( handle != EDITDISPHANDLE_INVALID )
				{
					CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
					pNeighborDisp->UpdateData();
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpdateLightmapExtents( void )
{
	// get the parent surface - the face
	CMapFace *pFace = ( CMapFace* )GetParent();
	if( !pFace )
		return;

	// check for valid lightmap size
	if( !pFace->ValidLightmapSize() )
	{
		// correct the lightmap size -- set to max possible
		pFace->AdjustLightmapScale();
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::EntityInBoundingBox( Vector const &vOrigin )
{
	Vector vMin, vMax;

	for( int axis = 0; axis < 3; axis++ )
	{
		vMin[axis] = m_BBox[0][axis] - OVERLAY_CHECK_BLOAT;
		vMax[axis] = m_BBox[1][axis] + OVERLAY_CHECK_BLOAT;
	}

	if( ( vOrigin.x < vMin.x ) || ( vOrigin.x > vMax.x ) ||
		( vOrigin.y < vMin.y ) || ( vOrigin.y > vMax.y ) ||
		( vOrigin.z < vMin.z ) || ( vOrigin.z > vMax.z ) )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::CheckAndUpdateOverlays( void )
{
	CMapFace *pFace = ( CMapFace* )GetParent();
	CMapSolid *pSolid = ( CMapSolid* )pFace->GetParent();
	pSolid->PostUpdate(Notify_Rebuild);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpSample( int oldPower )
{
	//
	// allocate temporary memory to hold new displacement distances
	//
	int width = GetWidth();
	int height = GetHeight();

	float *dists = new float[height*width];
	float *alphas = new float[height*width];
	Vector *dispVectors = new Vector[height*width];
	Vector *subdivPositions = new Vector[height*width];
	Vector *subdivNormals = new Vector[height*width];

	if( !dists || !alphas || !dispVectors || !subdivPositions || !subdivNormals )
	{
		delete [] dists;
		delete [] alphas;
		delete [] dispVectors;
		delete [] subdivPositions;
		delete [] subdivNormals;
		return;
	}

	//
	// get old width and height
	//
	int oldWidth = ( ( 1 << oldPower ) + 1 );
	int oldHeight = ( ( 1 << oldPower ) + 1 );

	for( int oh = 0, nh = 0; oh < oldHeight; oh++, nh += 2 )
	{
		for( int ow = 0, nw = 0; ow < oldWidth; ow++, nw += 2 )
		{
			bool bRight = false;
			bool bUp = false;

			int oldIndex = oh * oldHeight + ow;
			int newIndex = nh * height + nw;

			int x = oldIndex % oldWidth;
			int y = oldIndex / oldHeight;

			float dist = GetFieldDistance( oldIndex );
			dists[newIndex] = dist;

			float alpha = GetAlpha( oldIndex );
			alphas[newIndex] = alpha;

			Vector dVector[2], subPVector[2], subNVector[2];
			GetFieldVector( oldIndex, dVector[0] );
			GetSubdivPosition( oldIndex, subPVector[0] );
			GetSubdivNormal( oldIndex, subNVector[0] );
			dispVectors[newIndex] = dVector[0];
			subdivPositions[newIndex] = subPVector[0];
			subdivNormals[newIndex] = subNVector[0];

			if( ( x + 1 ) < oldWidth )
			{
				dist = ( GetFieldDistance( oldIndex ) + GetFieldDistance( oldIndex + 1 ) ) * 0.5f;
				dists[newIndex+1] = dist;

				alpha = ( GetAlpha( oldIndex ) + GetAlpha( oldIndex + 1 ) ) * 0.5f;
				alphas[newIndex+1] = alpha;

				GetFieldVector( oldIndex, dVector[0] );
				GetFieldVector( oldIndex + 1, dVector[1] );
				dispVectors[newIndex+1] = ( dVector[0] + dVector[1] ) * 0.5f;

				GetSubdivPosition( oldIndex, subPVector[0] );
				GetSubdivPosition( oldIndex + 1, subPVector[1] );
				subdivPositions[newIndex+1] = ( subPVector[0] + subPVector[1] ) * 0.5f;

				GetSubdivNormal( oldIndex, subNVector[0] );
				GetSubdivNormal( oldIndex + 1, subNVector[1] );
				subdivNormals[newIndex+1] = ( subNVector[0] + subNVector[1] ) * 0.5f;

				bRight = true;
			}

			if( ( y + 1 ) < oldHeight )
			{
				dist = ( GetFieldDistance( oldIndex ) + GetFieldDistance( oldIndex + oldHeight ) ) * 0.5f;
				dists[newIndex+height] = dist;

				alpha = ( GetAlpha( oldIndex ) + GetAlpha( oldIndex + oldHeight ) ) * 0.5f;
				alphas[newIndex+height] = alpha;

				GetFieldVector( oldIndex, dVector[0] );
				GetFieldVector( oldIndex + oldHeight, dVector[1] );
				dispVectors[newIndex+height] = ( dVector[0] + dVector[1] ) * 0.5f;

				GetSubdivPosition( oldIndex, subPVector[0] );
				GetSubdivPosition( oldIndex + oldHeight, subPVector[1] );
				subdivPositions[newIndex+height] = ( subPVector[0] + subPVector[1] ) * 0.5f;

				GetSubdivNormal( oldIndex, subNVector[0] );
				GetSubdivNormal( oldIndex + oldHeight, subNVector[1] );
				subdivNormals[newIndex+height] = ( subNVector[0] + subNVector[1] ) * 0.5f;
				
				bUp = true;
			}

			if( bRight && bUp )
			{
				dist = ( GetFieldDistance( oldIndex + 1 ) + GetFieldDistance( oldIndex + oldHeight ) ) * 0.5f;
				dists[newIndex+height+1] = dist;

				alpha = ( GetAlpha( oldIndex + 1 ) + GetAlpha( oldIndex + oldHeight ) ) * 0.5f;
				alphas[newIndex+height+1] = alpha;

				GetFieldVector( oldIndex + 1, dVector[0] );
				GetFieldVector( oldIndex + oldHeight, dVector[1] );
				dispVectors[newIndex+height+1] = ( dVector[0] + dVector[1] ) * 0.5f;

				GetSubdivPosition( oldIndex + 1, subPVector[0] );
				GetSubdivPosition( oldIndex + oldHeight, subPVector[1] );
				subdivPositions[newIndex+height+1] = ( subPVector[0] + subPVector[1] ) * 0.5f;

				GetSubdivNormal( oldIndex + 1, subNVector[0] );
				GetSubdivNormal( oldIndex + oldHeight, subNVector[1] );
				subdivNormals[newIndex+height+1] = ( subNVector[0] + subNVector[1] ) * 0.5f;
			}
		}
	}

	//
	// copy sampled list
	//
	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		SetAlpha( i, alphas[i] );
		SetFieldVector( i, dispVectors[i] );
		SetFieldDistance( i, dists[i] );
		SetSubdivPosition( i, subdivPositions[i] );
		SetSubdivNormal( i, subdivNormals[i] );
	}

	//
	// free temporary memory
	//
	delete [] dists;
	delete [] alphas;
	delete [] dispVectors;
	delete [] subdivPositions;
	delete [] subdivNormals;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::SamplePoints( int index, int width, int height, bool *pValidPoints, 
							 float *pValue, float *pAlpha, Vector& newDispVector, 
							 Vector& newSubdivPos, Vector &newSubdivNormal )
{
	//
	// set initial sample values
	//
	Vector vField, vSPos, vSNormal;
	int value = GetFieldDistance( index );
	float alpha = GetAlpha( index );
	GetFieldVector( index, vField );
	GetSubdivPosition( index, vSPos );
	GetSubdivNormal( index, vSNormal );

	int count = 1;

	//
	// accumulate other sample values from around the given index
	//
	int ndx;
	Vector vTmp;
	for( int i = 0; i < 8; i++ )
	{
		if( !pValidPoints[i] )
			continue;

		switch( i )
		{
		case 0: { ndx = index - height - 1; break; }				// down and left
		case 1: { ndx = index - 1; break; }							// left
		case 2: { ndx = index + height - 1; break; }				// up and left
		case 3: { ndx = index + height; break; }					// up
		case 4: { ndx = index + height + 1; break; }				// up and right
		case 5: { ndx = index + 1; break; }							// right
		case 6: { ndx = index - height + 1; break; }				// down and right
		case 7: { ndx = index - height; break; }					// down
		default: continue;
		}

		value += GetFieldDistance( ndx );
		alpha += GetAlpha( ndx );

		GetFieldVector( ndx, vTmp );
		vField += vTmp;

		GetSubdivPosition( ndx, vTmp );
		vSPos += vTmp;

		GetSubdivNormal( ndx, vTmp );
		vSNormal += vTmp;

		// increment count
		count++;
	}

	// average
	*pValue = value / ( float )count;
	*pAlpha = alpha / ( float )count;
	newDispVector = vField / ( float )count;
	newSubdivPos = vSPos / ( float )count;
	newSubdivNormal = vSNormal / ( float )count;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::GetValidSamplePoints( int index, int width, int height, bool *pValidPoints )
{
	int x = index % width;
	int y = index / height;

	// down and left
	if( ( ( x - 1 ) >= 0 ) && ( ( y - 1 ) >= 0 ) ) { pValidPoints[0] = true; }

	// left
	if( ( x - 1 ) >= 0 ) { pValidPoints[1] = true; }

	// up and left
	if( ( ( x - 1 ) >= 0 ) && ( ( y + 1 ) < height ) ) { pValidPoints[2] = true; }

	// up
	if( ( y + 1 ) < height ) { pValidPoints[3] = true; }

	// up and right
	if( ( ( x + 1 ) < width ) && ( ( y + 1 ) < height ) ) { pValidPoints[4] = true; }

	// right
	if( ( x + 1 ) < width ) { pValidPoints[5] = true; }

	// down and right
	if( ( ( x + 1 ) < width ) && ( ( y - 1 ) >= 0 ) ) { pValidPoints[6] = true; }

	// down
	if( ( y - 1 ) >= 0 ) { pValidPoints[7] = true; }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::DownSample( int oldPower )
{
	//
	// allocate temporary memory to hold new displacement distances
	//
	int width = GetWidth();
	int height = GetHeight();

	float *dists = new float[height*width];
	float *alphas = new float[height*width];
	Vector *dispVectors = new Vector[height*width];
	Vector *subdivPos = new Vector[height*width];
	Vector *subdivNormals = new Vector[height*width];

	if( !dists || !alphas || !dispVectors || !subdivPos || !subdivNormals )
	{
		delete [] dists;
		delete [] alphas;
		delete [] dispVectors;
		delete [] subdivPos;
		delete [] subdivNormals;
		return;
	}

	//
	// get old width and height
	//
	int oldWidth = ( ( 1 << oldPower ) + 1 );
	int oldHeight = ( ( 1 << oldPower ) + 1 );

	for( int oh = 0, nh = 0; oh < oldHeight; oh += 2, nh++ )
	{
		for( int ow = 0, nw = 0; ow < oldWidth; ow += 2, nw++ )
		{
			int oldIndex = oh * oldHeight + ow;
			int newIndex = nh * height + nw;

			//
			// clear valid point list and gather valid sample points
			//
			bool validPoints[8];
			for( int i = 0; i < 8; i++ ) { validPoints[i] = false; }
			GetValidSamplePoints( oldIndex, oldWidth, oldHeight, validPoints );

			//
			// sample the points, vector field vectors, and offset vectors
			//
			float newValue;
			float newAlpha;
			Vector newDispVector;
			Vector newSubdivPos;
			Vector newSubdivNormal;
			SamplePoints( oldIndex, oldWidth, oldHeight, validPoints, &newValue, &newAlpha, 
				          newDispVector, newSubdivPos, newSubdivNormal );

			//
			// save sampled values
			//
			dists[newIndex] = newValue;
			alphas[newIndex] = newAlpha;
			dispVectors[newIndex] = newDispVector;
			subdivPos[newIndex] = newSubdivPos;
			subdivNormals[newIndex] = newSubdivNormal;
		}
	}

	//
	// copy sampled list
	//
	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		SetAlpha( i, alphas[i] );
		SetFieldDistance( i, dists[i] );
		SetFieldVector( i, dispVectors[i] );
		SetSubdivPosition( i, subdivPos[i] );
		SetSubdivNormal( i, subdivNormals[i] );
	}

	//
	// free temporary memory
	//
	delete [] dists;
	delete [] alphas;
	delete [] dispVectors;
	delete [] subdivPos;
	delete [] subdivNormals;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMapDisp::InvertAlpha( void )
{
	int nVertCount = GetSize();
	for ( int iVert = 0; iVert < nVertCount; ++iVert )
	{
		float flAlpha = GetAlpha( iVert );
		float flInvAlpha = 255.0f - flAlpha;
		SetAlpha( iVert, flInvAlpha );
	}

	// Update the surface with new data.
	UpdateData();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Resample( int power )
{
	//
	// save old power for resampling, update to new power
	//
	int oldPower = GetPower();
	if( oldPower > power )
	{
		int delta = oldPower - power;
		for( int i = 0; i < delta; i++ )
		{
			SetPower( oldPower - ( i + 1 ) );
			DownSample( oldPower - i );
		}
	}
	else
	{
		int delta = power - oldPower;
		for( int i = 0; i < delta; i++ )
		{
			SetPower( oldPower + ( i + 1 ) );
			UpSample( oldPower + i );
		}
	}

	// update the surface with the new data
	UpdateData();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Elevate( float elevation )
{
	// set the new elevation
	SetElevation( elevation );

	// update the displacement
	UpdateData();
}

//-----------------------------------------------------------------------------
// Purpose: Resample a displacement map that was on a surface that was clipped.
//          NOTE: The new surface must be a quad as well, otherwise return
//          false;
//-----------------------------------------------------------------------------
bool CMapDisp::Split( CMapFace *pFace, EditDispHandle_t hBuilderDisp )
{
	CMapDisp *pBuilderDisp = EditDispMgr()->GetDisp( hBuilderDisp );

	static Vector vecSurfPoints[4];
	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		GetSurfPoint( iPoint, vecSurfPoints[iPoint] );
	}

	int nVertCount = pBuilderDisp->GetSize();
	pBuilderDisp->Paint_Init( DISPPAINT_CHANNEL_POSITION );
	for ( int iVert = 0; iVert < nVertCount; ++iVert )
	{
		Vector vecVert;
		pBuilderDisp->GetVert( iVert, vecVert );

		Vector2D vecDispUV;
		PointInQuadToBarycentric( vecSurfPoints[0], vecSurfPoints[3], vecSurfPoints[2], vecSurfPoints[1], vecVert, vecDispUV );

		Vector vecNewVert, vecNewNormal;
		float flNewAlpha;
		m_CoreDispInfo.DispUVToSurfWithAttribs( vecDispUV, vecNewVert, vecNewNormal, flNewAlpha );

		pBuilderDisp->SetAlpha( iVert, flNewAlpha );
		pBuilderDisp->Paint_SetValue(iVert, vecNewVert );
	}
	pBuilderDisp->Paint_Update();

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::ComparePoints( const Vector& pt1, const Vector& pt2, const float tolerance )
{
	for( int i = 0 ; i < 3 ; i++ )
	{
		if( fabs( pt1[i] - pt2[i] ) > tolerance )
			return false;
	}
	
	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CMapDisp::CollideWithTriangles( const Vector& RayStart, const Vector& RayEnd, Tri_t *pTris, int triCount,
									  Vector& surfNormal )
{
	// create a ray
	Ray_t ray;
	ray.m_Start = RayStart;
	ray.m_Delta = RayEnd - RayStart;
	ray.m_IsRay = true;

	Vector vNormal;
	float minFraction = 1.0f;
	for( int ndxTri = 0; ndxTri < triCount; ndxTri++ )
	{
		Tri_t &tri = pTris[ndxTri];
		float fraction = IntersectRayWithTriangle( ray, tri.v[0], tri.v[2], tri.v[1], true );
		if( fraction == -1 )
			continue;

		if( fraction < minFraction )
		{
			minFraction = fraction;

			// calculate the triangle normal
			Vector edge1, edge2;
			VectorSubtract( tri.v[2], tri.v[0], edge1 );
			VectorSubtract( tri.v[1], tri.v[0], edge2 );
			CrossProduct( edge1, edge2, surfNormal );
			VectorNormalize( surfNormal );
		}
	}

	return minFraction;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::CreatePlanesFromBoundingBox( Plane_t *planes, const Vector& bbMin, const Vector& bbMax )
{
	for( int i = 0; i < 6; i++ )
	{
		VectorClear( planes[i].normal );
	}

	//
	// use pads to store minor axes
	//
	planes[0].normal[0] = -1;	planes[0].dist = -bbMin[0];
	planes[1].normal[0] = 1;	planes[1].dist = bbMax[0];

	planes[2].normal[1] = -1;	planes[2].dist = -bbMin[1];
	planes[3].normal[1] = 1;	planes[3].dist = bbMax[1];

	planes[4].normal[2] = -1;	planes[4].dist = -bbMin[2];
	planes[5].normal[2] = 1;	planes[5].dist = bbMax[2];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::CollideWithBoundingBoxes( const Vector& rayStart, const Vector& rayEnd, 
										 BBox_t *pBBox, int bboxCount, Tri_t *pTris, int *triCount )
{
	const float DIST_EPSILON = 0.01f;

	//
	// collide against all bounding boxes
	//
	for( int i = 0; i < bboxCount; i++ )
	{
		//
		// make copy of vectors so they can be cut up
		//
		Vector start, end;
		start = rayStart;
		end = rayEnd;

		//
		// make planes for bbox
		//
		Plane_t planes[6];
		CreatePlanesFromBoundingBox( planes, pBBox[i].min, pBBox[i].max );

		//
		// collide against bounding box planes
		//
		for( int j = 0; j < 6; j++ )
		{
			float dist1 = DotProduct( planes[j].normal, start ) - planes[j].dist;
			float dist2 = DotProduct( planes[j].normal, end ) - planes[j].dist;
		
			//
			// entry intersection point - move ray start up to intersection
			//
			if( ( dist1 > DIST_EPSILON ) && ( dist2 < -DIST_EPSILON ) )
			{
				float fraction = ( dist1 / ( dist1 - dist2 ) );
				Vector segment, addOn;
				VectorSubtract( end, start, segment );
				VectorScale( segment, fraction, addOn );
				VectorNormalize( segment );
				VectorAdd( addOn, segment, addOn );
				VectorAdd( start, addOn, start );
			}
			else if( ( dist1 > DIST_EPSILON ) && ( dist2 > DIST_EPSILON ) )
			{
				break;
			}
		}

		//
		// collision add triangles to list
		//
		if( j == 6 )
		{
			// gross! shouldn't know value (64) and handle error better
			if( *triCount >= 256 )
			{
				// error!!!!!
				return;
			}

			int postSpacing = m_CoreDispInfo.GetPostSpacing();
			int index = i + ( i / ( postSpacing - 1 ) );

			m_CoreDispInfo.GetVert( index, pTris[*triCount].v[0] );
			m_CoreDispInfo.GetVert( index+postSpacing, pTris[*triCount].v[1] );
			m_CoreDispInfo.GetVert( index+1, pTris[*triCount].v[2] );
			*triCount += 1;

			m_CoreDispInfo.GetVert( index+1, pTris[*triCount].v[0] );
			m_CoreDispInfo.GetVert( index+postSpacing, pTris[*triCount].v[1] );
			m_CoreDispInfo.GetVert( index+postSpacing+1, pTris[*triCount].v[2] );
			*triCount += 1;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::CreateBoundingBoxes( BBox_t *pBBox, int count, float bloat )
{
	//
	// initialize the bounding boxes
	//
	for( int i = 0; i < count; i++ )
	{
		VectorFill( pBBox[i].min, COORD_NOTINIT );
		VectorFill( pBBox[i].max, -COORD_NOTINIT );
	}

	// get the width and height of the displacement surface
	int postSpacing = m_CoreDispInfo.GetPostSpacing();

	//
	// find bounding box of every two consecutive triangles
	//
	int bboxIndex = 0;
	int index;
	for( i = 0; i < ( postSpacing - 1 ); i++ )
	{
		for( int j = 0; j < ( postSpacing - 1 ); j++ )
		{
			for( int k = 0; k < 4; k++ )
			{
				switch( k )
				{
				case 0: { index = ( postSpacing * i ) + j; break; }
				case 1: { index = ( postSpacing * ( i + 1 ) ) + j; break; }
				case 2: { index = ( postSpacing * i ) + ( j + 1 ); break; }
				case 3: { index = ( postSpacing * ( i + 1 ) ) + ( j + 1 ); break; }
				}

				Vector v;
				m_CoreDispInfo.GetVert( index, v );
				if( v[0] < pBBox[bboxIndex].min[0] ) { pBBox[bboxIndex].min[0] = v[0]; }
				if( v[1] < pBBox[bboxIndex].min[1] ) { pBBox[bboxIndex].min[1] = v[1]; }
				if( v[2] < pBBox[bboxIndex].min[2] ) { pBBox[bboxIndex].min[2] = v[2]; }
				
				if( v[0] > pBBox[bboxIndex].max[0] ) { pBBox[bboxIndex].max[0] = v[0]; }
				if( v[1] > pBBox[bboxIndex].max[1] ) { pBBox[bboxIndex].max[1] = v[1]; }
				if( v[2] > pBBox[bboxIndex].max[2] ) { pBBox[bboxIndex].max[2] = v[2]; }
			}

			// bloat all the boxes a little
			for( int axis = 0; axis < 3; axis++ )
			{
				pBBox[bboxIndex].min[axis] -= bloat;
				pBBox[bboxIndex].max[axis] += bloat;
			}

			bboxIndex++;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::TraceLine( Vector &HitPos, Vector &HitNormal, Vector const &RayStart, Vector const &RayEnd )
{
	//
	// create bounding boxes
	//
	int bboxCount = ( GetHeight() - 1 ) * ( GetWidth() - 1 );
	BBox_t *pBBox = new BBox_t[bboxCount];
	CreateBoundingBoxes( pBBox, bboxCount, 1.0f );

	//
	// collide with bounding boxes and generate a triangle list
	//
	int triCount = 0;
	Tri_t *pTris = new Tri_t[256];
	CollideWithBoundingBoxes( RayStart, RayEnd, pBBox, bboxCount, pTris, &triCount );

	// triangle intersection -- find the closest intersection point
	Vector normal;
	float fraction = CollideWithTriangles( RayStart, RayEnd, pTris, triCount, normal );

	//
	// delete temporary data!!
	//
	delete [] pBBox;
	delete [] pTris;

	// no collision!
	if( fraction == 1.0f )
		return false;

	//
	// get hit position
	//
	Vector ray = RayEnd - RayStart;
//	VectorNormalize( ray );
	ray = ray * fraction;
	HitPos = RayStart + ray;

	HitNormal = normal;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CMapDisp::ClampTo( float &value, float lower, float upper )
{
	if( value < lower ) { value = lower; }
	if( value > upper ) { value = upper; }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::TraceLineSnapTo( Vector &HitPos, Vector &HitNormal, 
							    Vector const &RayStart, Vector const &RayEnd )
{
#define LOWER_TOLERANCE		-0.1f
#define UPPER_TOLERANCE		1.1f

	// get width and height
	int width = GetWidth();
	int height = GetHeight();

	// build the ray
	Ray_t ray;
	ray.m_Start = RayStart;
	ray.m_Delta = RayEnd - RayStart;
	ray.m_IsRay = true;

	float u, v;
	Tri_t tri;

	// test edge 0
	for( int ndx = 0; ndx < ( width - 1 ); ndx++ )
	{
		GetVert( ndx, tri.v[0] );
		GetVert( ndx + width, tri.v[1] );
		GetVert( ndx + 1, tri.v[2] );

		ComputeIntersectionBarycentricCoordinates( ray, tri.v[0], tri.v[1], tri.v[2], u, v );
		
		// along edge (0.0 < v < 1.0) and below (u < 0.0)
		if( ( v >= LOWER_TOLERANCE ) && ( v <= UPPER_TOLERANCE ) && ( u < 0.0f ) )
		{
			ClampTo( v, 0.0f, 1.0f );

			// snap u (u = 0.0)
			HitPos = tri.v[0] + ( tri.v[2] - tri.v[0] ) * v; 
			return true;
		}

		// special corner 0
		if( ( ndx == 0 ) && ( v < 0.0f ) && ( u < 0.0f ) )
		{
			HitPos = tri.v[0];
			return true;
		}
	}

	// test edge 1
	for( ndx = 0; ndx < ( height - 1 ); ndx++ )
	{
		GetVert( ndx * width, tri.v[0] );
		GetVert( ( ndx * width )+ width, tri.v[1] );
		GetVert( ( ndx * width ) + 1, tri.v[2] );

		ComputeIntersectionBarycentricCoordinates( ray, tri.v[0], tri.v[1], tri.v[2], u, v );
		
		// along edge (0.0 < u < 1.0) and left (v < 0.0)
		if( ( u >= LOWER_TOLERANCE ) && ( u <= UPPER_TOLERANCE ) && ( v < 0.0f ) )
		{
			ClampTo( u, 0.0f, 1.0f );

			// snap v (v = 0.0)
			HitPos = tri.v[0] + ( tri.v[1] - tri.v[0] ) * u; 
			return true;
		}	

		// special corner 1
		if( ( ndx == ( height - 2 ) ) && ( u > 1.0f ) && ( v < 0.0f ) )
		{
			HitPos = tri.v[1];
			return true;
		}
	}

	// test edge 2
	for( ndx = 0; ndx < ( width - 1 ); ndx++ )
	{
		GetVert( ( ( height - 1 ) * width ) + ndx + 1, tri.v[0] );
		GetVert( ( ( height - 2 ) * width ) + ndx + 1, tri.v[1] );
		GetVert( ( ( height - 1 ) * width ) + ndx, tri.v[2] );

		ComputeIntersectionBarycentricCoordinates( ray, tri.v[0], tri.v[1], tri.v[2], u, v );
		
		// along edge (0.0 < v < 1.0) and above (u < 0.0)
		if( ( v >= LOWER_TOLERANCE ) && ( v <= UPPER_TOLERANCE ) && ( u < 0.0f ) )
		{
			ClampTo( v, 0.0f, 1.0f );

			// snap u (u = 0.0)
			HitPos = tri.v[0] + ( tri.v[2] - tri.v[0] ) * v; 
			return true;
		}

		// special corner 2
		if( ( ndx == ( width - 2 ) ) && ( v < 0.0f ) && ( u < 0.0f ) )
		{
			HitPos = tri.v[0];
			return true;
		}
	}

	// test edge 3
	for( ndx = 0; ndx < ( height - 1 ); ndx++ )
	{
		GetVert( ( ndx * width ) + ( ( 2 * width ) - 1 ), tri.v[0] );
		GetVert( ( ndx * width ) + ( width - 1 ), tri.v[1] );
		GetVert( ( ndx * width ) + ( ( 2 * width ) - 2 ), tri.v[2] );

		ComputeIntersectionBarycentricCoordinates( ray, tri.v[0], tri.v[1], tri.v[2], u, v );
		
		// along edge (0.0 < u < 1.0) and right (v < 0.0)
		if( ( u >= LOWER_TOLERANCE ) && ( u <= UPPER_TOLERANCE ) && ( v < 0.0f ) )
		{
			ClampTo( u, 0.0f, 1.0f );

			// snap v (v = 0.0)
			HitPos = tri.v[0] + ( tri.v[1] - tri.v[0] ) * u; 
			return true;
		}	

		// special corner 3
		if( ( ndx == 0 ) && ( u > 1.0f ) && ( v < 0.0f ) )
		{
			HitPos = tri.v[1];
			return true;
		}
	}

	return false;

#undef LOWER_TOLERANCE
#undef UPPER_TOLERANCE
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Flip( int flipType )
{
	int width = GetWidth();
	int height = GetHeight();

	switch( flipType )
	{
	case FLIP_HORIZONTAL:
		{
			return;
		}
	case FLIP_VERTICAL:
		{
			return;
		}
	case FLIP_TRANSPOSE:
		{
			for( int ndxHeight = 0; ndxHeight < height; ndxHeight++ )
			{
				for( int ndxWidth = ndxHeight; ndxWidth < width; ndxWidth++ )
				{
					float dist1 = GetFieldDistance( ( ndxHeight * width ) + ndxWidth );
					float dist2 = GetFieldDistance( ( ndxWidth * height ) + ndxHeight );
					SetFieldDistance( ( ndxHeight * width ) + ndxWidth, dist2 );
					SetFieldDistance( ( ndxWidth * height ) + ndxHeight, dist1 );

					Vector v1, v2;
					GetFieldVector( ( ndxHeight * width ) + ndxWidth, v1 );
					GetFieldVector( ( ndxWidth * height ) + ndxHeight, v2 );
					SetFieldVector( ( ndxHeight * width ) + ndxWidth, v2 );
					SetFieldVector( ( ndxWidth * height ) + ndxHeight, v1 );

					GetSubdivPosition( ( ndxHeight * width ) + ndxWidth, v1 );
					GetSubdivPosition( ( ndxWidth * height ) + ndxHeight, v2 );
					SetSubdivPosition( ( ndxHeight * width ) + ndxWidth, v2 );
					SetSubdivPosition( ( ndxWidth * height ) + ndxHeight, v1 );

					GetSubdivNormal( ( ndxHeight * width ) + ndxWidth, v1 );
					GetSubdivNormal( ( ndxWidth * height ) + ndxHeight, v2 );
					SetSubdivNormal( ( ndxHeight * width ) + ndxWidth, v2 );
					SetSubdivNormal( ( ndxWidth * height ) + ndxHeight, v1 );

					float alpha1 = GetAlpha( ( ndxHeight * width ) + ndxWidth );
					float alpha2 = GetAlpha( ( ndxWidth * height ) + ndxHeight );
					SetAlpha( ( ndxHeight * width ) + ndxWidth, alpha2 );
					SetAlpha( ( ndxWidth * height ) + ndxHeight, alpha1 );
				}
			}
			return;
		}
	default:
		{
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::DoTransFlip( const Vector &RefPoint )
{
	//
	// get the face
	//
	CMapFace *pFace = ( CMapFace* )GetParent();
	if( !pFace )
		return;

	// flip the surface
	Flip( FLIP_TRANSPOSE );

	//
	// get the displacement starting point, relative to the newly "flipped" points
	// NOTE: this seems a bit hacky -- if flip goes NUTS later -- look here!!!
	//
	CCoreDispSurface *pSurf = m_CoreDispInfo.GetSurface();	
	Vector startPoint;
	pSurf->GetPoint( 0, startPoint );

	for( int i = 0; i < 3; i++ )
	{
		if( RefPoint[i] != COORD_NOTINIT )
		{
			startPoint[i] = ( RefPoint[i] - ( startPoint[i] - RefPoint[i] ) );
		}
	}

	for( i = 0; i < 4; i++ )
	{
		float dx = startPoint.x - pFace->Points[i].x;
		float dy = startPoint.y - pFace->Points[i].y;
		float dz = startPoint.z - pFace->Points[i].z;

		float dist = sqrt( dx*dx + dy*dy + dz*dz );
		if( dist < 0.01 )
			break;
	}

	if( i == 4 )
		return;

	// get the flip axis
	int flipAxis;
	for( int axis = 0; axis < 3; axis++ )
	{
		if( RefPoint[axis] != COORD_NOTINIT )
		{
			flipAxis = axis;
		}
	}

	// flip in/out direction to be parallel to the flip normal
	Vector v;
	int size = GetSize();
	for( int ndx = 0; ndx < size; ndx++ )
	{
		GetFieldVector( ndx, v );
		v[flipAxis] = -v[flipAxis];
		SetFieldVector( ndx, v );
		
		GetSubdivPosition( ndx, v );
		v[flipAxis] = -v[flipAxis];
		SetSubdivPosition( ndx, v );
		
		GetSubdivNormal( ndx, v );
		v[flipAxis] = -v[flipAxis];
		SetSubdivNormal( ndx, v );
	}

	//
	// I know that this get recreated in the MapSolid calling function, but I need to 
	// clear the vector field here!!!
	//
	pSurf->SetPointStartIndex( i );
	UpdateSurfDataAndVectorField( pFace );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpdateWalkable( void )
{
	// Set the walkable tag.
	int nTriCount = GetTriCount();
	for ( int iTri = 0; iTri < nTriCount; ++iTri )
	{
		Vector v1, v2, v3;
		GetTriPos( iTri, v1, v2, v3 );

		Vector vecEdge1, vecEdge2;
		vecEdge1 = v2 - v1;
		vecEdge2 = v3 - v1;

		Vector vecTriNormal;
		CrossProduct( vecEdge2, vecEdge1, vecTriNormal );
		VectorNormalize( vecTriNormal );

		ResetTriTag( iTri, COREDISPTRI_TAG_WALKABLE );
		if ( vecTriNormal.z >= WALKABLE_NORMAL_VALUE )
		{
			SetTriTag( iTri, COREDISPTRI_TAG_WALKABLE );
		}
	}

	// Create the walkable render list.
	m_aWalkableVerts.RemoveAll();
	m_aWalkableIndices.RemoveAll();
	m_aForcedWalkableIndices.RemoveAll();

	for ( iTri = 0; iTri < nTriCount; ++iTri )
	{
		if ( !IsTriWalkable( iTri ) )
		{
			unsigned short triIndices[3];
			unsigned short newTriIndices[3];
			GetTriIndices( iTri, triIndices[0], triIndices[1], triIndices[2] );

			newTriIndices[0] = m_aWalkableVerts.AddToTail( m_CoreDispInfo.GetDispVert( triIndices[0] ) );
			newTriIndices[1] = m_aWalkableVerts.AddToTail( m_CoreDispInfo.GetDispVert( triIndices[1] ) );
			newTriIndices[2] = m_aWalkableVerts.AddToTail( m_CoreDispInfo.GetDispVert( triIndices[2] ) );

			if ( IsTriTag( iTri, COREDISPTRI_TAG_FORCE_WALKABLE_BIT ) )
			{
				m_aForcedWalkableIndices.AddToTail( newTriIndices[0] );
				m_aForcedWalkableIndices.AddToTail( newTriIndices[1] );
				m_aForcedWalkableIndices.AddToTail( newTriIndices[2] );
			}
			else
			{
				m_aWalkableIndices.AddToTail( newTriIndices[0] );
				m_aWalkableIndices.AddToTail( newTriIndices[1] );
				m_aWalkableIndices.AddToTail( newTriIndices[2] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpdateBuildable( void )
{
	// Set the buildable tag.
	int nTriCount = GetTriCount();
	for ( int iTri = 0; iTri < nTriCount; ++iTri )
	{
		Vector v1, v2, v3;
		GetTriPos( iTri, v1, v2, v3 );

		Vector vecEdge1, vecEdge2;
		vecEdge1 = v2 - v1;
		vecEdge2 = v3 - v1;

		Vector vecTriNormal;
		CrossProduct( vecEdge2, vecEdge1, vecTriNormal );
		VectorNormalize( vecTriNormal );

		ResetTriTag( iTri, COREDISPTRI_TAG_BUILDABLE );
		if ( vecTriNormal.z >= BUILDABLE_NORMAL_VALUE )
		{
			SetTriTag( iTri, COREDISPTRI_TAG_BUILDABLE );
		}
	}

	// Create the buildable render list.
	m_aBuildableVerts.RemoveAll();
	m_aBuildableIndices.RemoveAll();
	m_aForcedBuildableIndices.RemoveAll();

	for ( iTri = 0; iTri < nTriCount; ++iTri )
	{
		if ( !IsTriBuildable( iTri ) )
		{
			unsigned short triIndices[3];
			unsigned short newTriIndices[3];
			GetTriIndices( iTri, triIndices[0], triIndices[1], triIndices[2] );

			newTriIndices[0] = m_aBuildableVerts.AddToTail( m_CoreDispInfo.GetDispVert( triIndices[0] ) );
			newTriIndices[1] = m_aBuildableVerts.AddToTail( m_CoreDispInfo.GetDispVert( triIndices[1] ) );
			newTriIndices[2] = m_aBuildableVerts.AddToTail( m_CoreDispInfo.GetDispVert( triIndices[2] ) );

			if ( IsTriTag( iTri, COREDISPTRI_TAG_FORCE_BUILDABLE_BIT ) )
			{
				m_aForcedBuildableIndices.AddToTail( newTriIndices[0] );
				m_aForcedBuildableIndices.AddToTail( newTriIndices[1] );
				m_aForcedBuildableIndices.AddToTail( newTriIndices[2] );
			}
			else
			{
				m_aBuildableIndices.AddToTail( newTriIndices[0] );
				m_aBuildableIndices.AddToTail( newTriIndices[1] );
				m_aBuildableIndices.AddToTail( newTriIndices[2] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RenderDisplacementNormals( CCoreDispInfo& coreDispInfo, int numVerts )
{
	Vector points[4], normal;

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_LINES, numVerts );

    for( int i = 0; i < numVerts; i++ )
    {
		coreDispInfo.GetVert( i, points[0] );
		coreDispInfo.GetNormal( i, normal );

	    meshBuilder.Color3f( 0.0f, 1.0f, 0.0f );
        meshBuilder.Position3f( points[0][0], points[0][1], points[0][2] );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 0.0f, 1.0f, 0.0f );
        meshBuilder.Position3f( points[0][0] + ( normal[0] * 10.0f ), 
			        points[0][1] + ( normal[1] * 10.0f ), 
					points[0][2] + ( normal[2] * 10.0f ) );
		meshBuilder.AdvanceVertex();
    }
	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RenderDisplacementTangentsS( CCoreDispInfo &coreDispInfo, int numVerts )
{
	Vector points[4], tangentS;

	CMeshBuilder meshBuilder;
	IMesh *pMesh = MaterialSystemInterface()->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_LINES, numVerts );

	for( int i = 0; i < numVerts; i++ )
	{
		coreDispInfo.GetVert( i, points[0] );
		coreDispInfo.GetTangentS( i, tangentS );

	    meshBuilder.Color3f( 1.0f, 0.0f, 0.0f );
        meshBuilder.Position3f( points[0][0], points[0][1], points[0][2] );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0f, 0.0f, 0.0f );
        meshBuilder.Position3f( points[0][0] + ( tangentS[0] * 10.0f ), 
			                    points[0][1] + ( tangentS[1] * 10.0f ), 
					            points[0][2] + ( tangentS[2] * 10.0f ) );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RenderDisplacementTangentsT( CCoreDispInfo &coreDispInfo, int numVerts )
{
	Vector points[4], tangentT;

	CMeshBuilder meshBuilder;
	IMesh *pMesh = MaterialSystemInterface()->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_LINES, numVerts );

	for( int i = 0; i < numVerts; i++ )
	{
		coreDispInfo.GetVert( i, points[0] );
		coreDispInfo.GetTangentT( i, tangentT );

	    meshBuilder.Color3f( 0.0f, 0.0f, 1.0f );
        meshBuilder.Position3f( points[0][0], points[0][1], points[0][2] );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 0.0f, 0.0f, 1.0f );
        meshBuilder.Position3f( points[0][0] + ( tangentT[0] * 10.0f ), 
			                    points[0][1] + ( tangentT[1] * 10.0f ), 
					            points[0][2] + ( tangentT[2] * 10.0f ) );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RenderFaceVertexNormals( CCoreDispInfo& coreDispInfo )
{
	Vector points[4], normal;

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 4 );

	CCoreDispSurface *pSurf = coreDispInfo.GetSurface();
    for( int i = 0; i < 4; i++ )
    {
		pSurf->GetPoint( i, points[0] );
		pSurf->GetPointNormal( i, normal );

		meshBuilder.Color3f( 1.0f, 0.0f, 0.0f );
        meshBuilder.Position3f( points[0][0], points[0][1], points[0][2] );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0f, 0.0f, 0.0f );
        meshBuilder.Position3f( points[0][0] + ( normal[0] * 25.0f ), 
			        points[0][1] + ( normal[1] * 25.0f ), 
					points[0][2] + ( normal[2] * 25.0f ) );
		meshBuilder.AdvanceVertex();
    }
	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RenderDisplacementVectorField( CCoreDispInfo& coreDispInfo, int numVerts )
{
	Vector points[4], normal;

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
	meshBuilder.Begin( pMesh, MATERIAL_LINES, numVerts );

	for( int i = 0; i < numVerts; i++ )
	{
		coreDispInfo.GetVert( i, points[0] );
		coreDispInfo.GetFieldVector( i, normal );

		meshBuilder.Color3f( 1.0f, 1.0f, 0.0f );
		meshBuilder.Position3f( points[0][0], points[0][1], points[0][2] );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0f, 1.0f, 0.0f );
		meshBuilder.Position3f( points[0][0] + ( normal[0] * 50.0f ),
			        points[0][1] + ( normal[1] * 50.0f ),
					points[0][2] + ( normal[2] * 50.0f ) );
		meshBuilder.AdvanceVertex();
	}
	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RenderSubdivPositions( CCoreDispInfo& coreDispInfo, int numVerts )
{
	Vector pt, normal;

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
	meshBuilder.Begin( pMesh, MATERIAL_LINES, numVerts );

	for( int i = 0;  i < numVerts; i++ )
	{
		coreDispInfo.GetFlatVert( i, pt );
		coreDispInfo.GetSubdivPosition( i, normal );

		meshBuilder.Position3f( pt[0], pt[1], pt[2] );
		meshBuilder.Color3f( 1.0f, 0.0f, 1.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( pt[0] + normal[0], pt[1] + normal[1], pt[2] + normal[2] );
		meshBuilder.Color3f( 1.0f, 0.0f, 1.0f );
		meshBuilder.AdvanceVertex();
	}
	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RenderDisplacementEdges( CCoreDispInfo& coreDispInfo )
{
 	Vector points[4];

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 4 );

	CCoreDispSurface *pSurf = coreDispInfo.GetSurface();
	pSurf->GetPoint( 0, points[0] );
	pSurf->GetPoint( 1, points[1] );
	pSurf->GetPoint( 2, points[2] );
	pSurf->GetPoint( 3, points[3] );
	
	meshBuilder.Color3f( 1.0f, 0.0f, 0.0f );
	meshBuilder.Position3f( points[0][0], points[0][1], points[0][2] );
	meshBuilder.AdvanceVertex();
	meshBuilder.Color3f( 1.0f, 0.0f, 0.0f );
	meshBuilder.Position3f( points[1][0], points[1][1], points[1][2] );
	meshBuilder.AdvanceVertex();
	
	meshBuilder.Color3f( 0.0f, 1.0f, 0.0f );
	meshBuilder.Position3f( points[1][0], points[1][1], points[1][2] );
	meshBuilder.AdvanceVertex();
	meshBuilder.Color3f( 0.0f, 1.0f, 0.0f );
	meshBuilder.Position3f( points[2][0], points[2][1], points[2][2] );
	meshBuilder.AdvanceVertex();
    
	meshBuilder.Color3f( 0.0f, 0.0f, 1.0f );
	meshBuilder.Position3f( points[2][0], points[2][1], points[2][2] );
	meshBuilder.AdvanceVertex();
	meshBuilder.Color3f( 0.0f, 0.0f, 1.0f );
	meshBuilder.Position3f( points[3][0], points[3][1], points[3][2] );
	meshBuilder.AdvanceVertex();
	
	meshBuilder.Color3f( 1.0f, 0.0f, 1.0f );
	meshBuilder.Position3f( points[3][0], points[3][1], points[3][2] );
	meshBuilder.AdvanceVertex();
	meshBuilder.Color3f( 1.0f, 0.0f, 1.0f );
	meshBuilder.Position3f( points[0][0], points[0][1], points[0][2] );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Render3DDebug( CRender3D *pRender, bool isSelected )
{
	pRender->SetRenderMode( RENDER_MODE_WIREFRAME );

#if 0
	RenderDisplacementNormals( m_CoreDispInfo, MAPDISP_MAX_VERTS );
	RenderDisplacementTangentsS( m_CoreDispInfo, MAPDISP_MAX_VERTS );
	RenderDisplacementTangentsT( m_CoreDispInfo, MAPDISP_MAX_VERTS );
#endif

//	RenderFaceVertexNormals( m_CoreDispInfo );
	RenderDisplacementVectorField( m_CoreDispInfo, MAPDISP_MAX_VERTS );
	RenderSubdivPositions( m_CoreDispInfo, GetSize() );
//	RenderDisplacementEdges( m_CoreDispInfo );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::CalcColor( CRender3D *pRender, bool bIsSelected, 
						  SelectionState_t faceSelectionState,
						  unsigned int r, unsigned int g, unsigned int b, unsigned int alpha,
						  unsigned char *pColor )
{
	// Get the current render mode.
	RenderMode_t renderMode = pRender->GetCurrentRenderMode();
	
	// White w/no alpha by default
	pColor[0] = pColor[1] = pColor[2] = 255;
	pColor[3] = 255;

	switch ( renderMode )
	{
	case RENDER_MODE_TEXTURED:
		{
			break;
		}
	case RENDER_MODE_SELECTION_OVERLAY:
		{
			if ( faceSelectionState == SELECT_MULTI_PARTIAL )
			{
				pColor[2] = 100;
				pColor[3] = 64;
			}
			else if ( ( faceSelectionState == SELECT_NORMAL ) || bIsSelected )
			{
				SelectFaceColor( pColor );
				pColor[3] = 64;
			}
			break;
		}
	case RENDER_MODE_LIGHTMAP_GRID:
		{
			CMapFace *pFace = ( CMapFace* )GetParent();
			if ( bIsSelected )
			{
				SelectFaceColor( pColor );
			}
			else if( pFace->texture.nLightmapScale != DEFAULT_LIGHTMAP_SCALE )
			{
				pColor[2] = 100;
			}
			break;
		}
	case RENDER_MODE_TRANSLUCENT_FLAT:
	case RENDER_MODE_FLAT:
		{
			if ( bIsSelected )
			{
				SelectFaceColor( pColor );
			}
			else
			{
				pColor[0] = r;
				pColor[1] = g;
				pColor[2] = b;
				pColor[3] = alpha;
			}
			break;
		}
	case RENDER_MODE_WIREFRAME:
		{
			if ( bIsSelected )
			{
				SelectEdgeColor( pColor );
			}
			else
			{
				pColor[0] = r;
				pColor[1] = g;
				pColor[2] = b;
				pColor[3] = alpha;
			}
			break;
		}
	case RENDER_MODE_SMOOTHING_GROUP:
		{			
			// Render the non-smoothing group faces in white, yellow for the others.
			CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
			if ( pDoc )
			{
				CMapFace *pFace = ( CMapFace* )GetParent();
				int iGroup = pDoc->GetSmoothingGroupVisual();
				if ( pFace->InSmoothingGroup( iGroup ) )
				{
					pColor[2] = 0;
				}
			}
			
			break;
		}
	default:
		{
			assert( 0 );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// NOTE: most of the rendering mode is set in the parent face render call!!!
//-----------------------------------------------------------------------------
void CMapDisp::Render3D( CRender3D *pRender, bool bIsSelected, SelectionState_t faceSelectionState )
{	
    // Get the current rendermode.
    RenderMode_t renderMode = pRender->GetCurrentRenderMode();

	if ( renderMode == RENDER_MODE_SELECTION_OVERLAY )
	{
		RenderOverlaySurface( pRender, bIsSelected, faceSelectionState );
	}
	else
	{
		RenderSurface( pRender, bIsSelected, faceSelectionState );

		// Note: This will cause the wireframe to render twice in selection due to
		//       the multiplass operations at the solid and face levels (the render
		//       portion of the hammer code needs to be reworked there).
		if ( renderMode != RENDER_MODE_WIREFRAME && bIsSelected )
		{
			// This renders wireframe twice in selection!
			RenderWireframeSurface( pRender, bIsSelected, faceSelectionState );
		}
	}

	// Note: the rendermode == textured is so that this only gets rendered
	//       once per frame.
	bool bDispWalkableMode = CMapDoc::GetActiveMapDoc()->IsDispDrawWalkable();
	if ( bDispWalkableMode && renderMode == RENDER_MODE_TEXTURED )
	{
		RenderWalkableSurface( pRender, bIsSelected, faceSelectionState );
	}

	// Note: the rendermode == textured is so that this only gets rendered
	//       once per frame.
	bool bDispBuildableMode = CMapDoc::GetActiveMapDoc()->IsDispDrawBuildable();
	if ( bDispBuildableMode && renderMode == RENDER_MODE_TEXTURED )
	{
		RenderBuildableSurface( pRender, bIsSelected, faceSelectionState );
	}

	// Render debug information.
//	Render3DDebug( pRender, bIsSelected );
}

//-----------------------------------------------------------------------------
// Purpose: Render the displacement surface.
//-----------------------------------------------------------------------------
void CMapDisp::RenderOverlaySurface( CRender3D *pRender, bool bIsSelected, SelectionState_t faceSelectionState )
{
	if ( HasSelectMask() )
		return;

	unsigned char color[4];
	CalcColor( pRender, bIsSelected, faceSelectionState, 255, 255, 255, 255, color );

	int nVertCount = m_CoreDispInfo.GetSize();
	int nIndexCount = m_CoreDispInfo.GetRenderIndexCount();
	
	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertCount, nIndexCount );
	
	CoreDispVert_t *pVert = m_CoreDispInfo.GetDispVertList();
	for (int i = 0; i < nVertCount; ++i )
	{
		meshBuilder.Position3fv( pVert[i].m_Vert.Base() );
		meshBuilder.Color4ub( color[0], color[1], color[2], color[3] );
		meshBuilder.Normal3fv( pVert[i].m_Normal.Base() );
		meshBuilder.AdvanceVertex();
	}
	
	unsigned short *pIndex = m_CoreDispInfo.GetRenderIndexList();
	for ( i = 0; i < nIndexCount; ++i )
	{
		meshBuilder.Index( pIndex[i] );
		meshBuilder.AdvanceIndex();
	}
	
	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: Render the displacement surface with a vertex alpha (blending).
//-----------------------------------------------------------------------------
void CMapDisp::RenderSurface( CRender3D *pRender, bool bIsSelected, SelectionState_t faceSelectionState )
{
	unsigned char color[4];
	CalcColor( pRender, bIsSelected, faceSelectionState, 255, 255, 255, 255, color );

	int numVerts = m_CoreDispInfo.GetSize();
	int numIndices = m_CoreDispInfo.GetRenderIndexCount();

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, numVerts,	numIndices );
	
	CoreDispVert_t *pVert = m_CoreDispInfo.GetDispVertList();
	for (int i = 0; i < numVerts; ++i )
	{
		meshBuilder.Position3fv( pVert[i].m_Vert.Base() );
		meshBuilder.Color4ub( color[0], color[1], color[2], 255 - ( unsigned char )( pVert[i].m_Alpha ) );
		meshBuilder.Normal3fv( pVert[i].m_Normal.Base() );
		meshBuilder.TangentS3fv( pVert[i].m_TangentS.Base() );
		meshBuilder.TangentT3fv( pVert[i].m_TangentT.Base() );
		meshBuilder.TexCoord2fv( 0, pVert[i].m_TexCoord.Base() );
		meshBuilder.TexCoord2fv( 1, pVert[i].m_LuxelCoords[0].Base() );
		meshBuilder.AdvanceVertex();
	}
	
	unsigned short *pIndex = m_CoreDispInfo.GetRenderIndexList();
	for ( i = 0; i < numIndices; ++i )
	{
		meshBuilder.Index( pIndex[i] );
		meshBuilder.AdvanceIndex();
	}
	
	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: Render the displacement surface with walkable data.
//-----------------------------------------------------------------------------
void CMapDisp::RenderWalkableSurface( CRender3D *pRender, bool bIsSelected, SelectionState_t faceSelectionState )
{
   // Get the current rendermode and save it.
    RenderMode_t renderModeOld = pRender->GetCurrentRenderMode();

	// Normal
	for ( int iPass = 0; iPass < 2; ++iPass )
	{
		unsigned char color[4];
		if ( iPass == 0 )
		{
			pRender->SetRenderMode( RENDER_MODE_TRANSLUCENT_FLAT );
			CalcColor( pRender, false, faceSelectionState, 255, 255, 0, 64, color );
		}
		else
		{
			pRender->SetRenderMode( RENDER_MODE_WIREFRAME );
			CalcColor( pRender, false, faceSelectionState, 255, 255, 0, 255, color );
		}
		
		int nVertCount = m_aWalkableVerts.Count();
		int nIndexCount = m_aWalkableIndices.Count();

		CMeshBuilder meshBuilder;
		IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertCount, nIndexCount );
		
		CoreDispVert_t **ppVerts = m_aWalkableVerts.Base();
		for (int i = 0; i < nVertCount; ++i )
		{
			CoreDispVert_t *pVert = ppVerts[i];
			
			meshBuilder.Position3fv( pVert->m_Vert.Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], color[3] );
			meshBuilder.Normal3fv( pVert->m_Normal.Base() );
			meshBuilder.AdvanceVertex();
		}
		
		unsigned short *pIndex = m_aWalkableIndices.Base();
		for ( i = 0; i < nIndexCount; ++i )
		{
			meshBuilder.Index( pIndex[i] );
			meshBuilder.AdvanceIndex();
		}
		
		meshBuilder.End();
		pMesh->Draw();
	}

	// Forced
	for ( iPass = 0; iPass < 2; ++iPass )
	{
		unsigned char color[4];
		if ( iPass == 0 )
		{
			pRender->SetRenderMode( RENDER_MODE_TRANSLUCENT_FLAT );
			CalcColor( pRender, false, faceSelectionState, 0, 255, 0, 64, color );
		}
		else
		{
			pRender->SetRenderMode( RENDER_MODE_WIREFRAME );
			CalcColor( pRender, false, faceSelectionState, 0, 255, 0, 255, color );
		}
		
		int nVertCount = m_aWalkableVerts.Count();
		int nIndexCount = m_aForcedWalkableIndices.Count();

		CMeshBuilder meshBuilder;
		IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertCount, nIndexCount );
		
		CoreDispVert_t **ppVerts = m_aWalkableVerts.Base();
		for (int i = 0; i < nVertCount; ++i )
		{
			CoreDispVert_t *pVert = ppVerts[i];
			
			meshBuilder.Position3fv( pVert->m_Vert.Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], color[3] );
			meshBuilder.Normal3fv( pVert->m_Normal.Base() );
			meshBuilder.AdvanceVertex();
		}
		
		unsigned short *pIndex = m_aForcedWalkableIndices.Base();
		for ( i = 0; i < nIndexCount; ++i )
		{
			meshBuilder.Index( pIndex[i] );
			meshBuilder.AdvanceIndex();
		}
		
		meshBuilder.End();
		pMesh->Draw();
	}

	// Reset the render mode.
	pRender->SetRenderMode( renderModeOld );
}

//-----------------------------------------------------------------------------
// Purpose: Render the displacement surface with buildable data.
//-----------------------------------------------------------------------------
void CMapDisp::RenderBuildableSurface( CRender3D *pRender, bool bIsSelected, SelectionState_t faceSelectionState )
{
    // Get the current rendermode and save it.
    RenderMode_t renderModeOld = pRender->GetCurrentRenderMode();

	// Normal
	for ( int iPass = 0; iPass < 2; ++iPass )
	{
		unsigned char color[4];
		if ( iPass == 0 )
		{
			pRender->SetRenderMode( RENDER_MODE_TRANSLUCENT_FLAT );
			CalcColor( pRender, false, faceSelectionState, 255, 100, 25, 64, color );
		}
		else
		{
			pRender->SetRenderMode( RENDER_MODE_WIREFRAME );
			CalcColor( pRender, false, faceSelectionState, 255, 255, 0, 255, color );
		}
		
		int nVertCount = m_aBuildableVerts.Count();
		int nIndexCount = m_aBuildableIndices.Count();

		CMeshBuilder meshBuilder;
		IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertCount, nIndexCount );
		
		CoreDispVert_t **ppVerts = m_aBuildableVerts.Base();
		for (int i = 0; i < nVertCount; ++i )
		{
			CoreDispVert_t *pVert = ppVerts[i];
			
			meshBuilder.Position3fv( pVert->m_Vert.Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], color[3] );
			meshBuilder.Normal3fv( pVert->m_Normal.Base() );
			meshBuilder.AdvanceVertex();
		}
		
		unsigned short *pIndex = m_aBuildableIndices.Base();
		for ( i = 0; i < nIndexCount; ++i )
		{
			meshBuilder.Index( pIndex[i] );
			meshBuilder.AdvanceIndex();
		}
		
		meshBuilder.End();
		pMesh->Draw();
	}

	// Forced
	for ( iPass = 0; iPass < 2; ++iPass )
	{
		unsigned char color[4];
		if ( iPass == 0 )
		{
			pRender->SetRenderMode( RENDER_MODE_TRANSLUCENT_FLAT );
			CalcColor( pRender, false, faceSelectionState, 0, 0, 255, 64, color );
		}
		else
		{
			pRender->SetRenderMode( RENDER_MODE_WIREFRAME );
			CalcColor( pRender, false, faceSelectionState, 0, 0, 255, 255, color );
		}
		
		int nVertCount = m_aBuildableVerts.Count();
		int nIndexCount = m_aForcedBuildableIndices.Count();

		CMeshBuilder meshBuilder;
		IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertCount, nIndexCount );
		
		CoreDispVert_t **ppVerts = m_aBuildableVerts.Base();
		for (int i = 0; i < nVertCount; ++i )
		{
			CoreDispVert_t *pVert = ppVerts[i];
			
			meshBuilder.Position3fv( pVert->m_Vert.Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], color[3] );
			meshBuilder.Normal3fv( pVert->m_Normal.Base() );
			meshBuilder.AdvanceVertex();
		}
		
		unsigned short *pIndex = m_aForcedBuildableIndices.Base();
		for ( i = 0; i < nIndexCount; ++i )
		{
			meshBuilder.Index( pIndex[i] );
			meshBuilder.AdvanceIndex();
		}
		
		meshBuilder.End();
		pMesh->Draw();
	}

	// Reset the render mode.
	pRender->SetRenderMode( renderModeOld );
}

//-----------------------------------------------------------------------------
// Purpose: Render the white wireframe overlay.
//-----------------------------------------------------------------------------
void CMapDisp::RenderWireframeSurface( CRender3D *pRender, bool bIsSelected, SelectionState_t faceSelectionState )
{
	if ( HasGridMask() )
		return;

    // Get the current rendermode.
    RenderMode_t renderModeOld = pRender->GetCurrentRenderMode();

	pRender->SetRenderMode( RENDER_MODE_WIREFRAME );

	unsigned char color[4];
	CalcColor( pRender, bIsSelected, faceSelectionState, 255, 255, 255, 255, color );
	
	int numVerts = m_CoreDispInfo.GetSize();
	int numIndices = m_CoreDispInfo.GetRenderIndexCount();
	
	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, numVerts,	numIndices );
	
	CoreDispVert_t *pVert = m_CoreDispInfo.GetDispVertList();
	for (int i = 0; i < numVerts; ++i )
	{
		meshBuilder.Position3fv( pVert[i].m_Vert.Base() );
		meshBuilder.Color3ub( 255, 255, 255 );
		meshBuilder.TexCoord2fv( 0, pVert[i].m_TexCoord.Base() );
		meshBuilder.TexCoord2fv( 1, pVert[i].m_LuxelCoords[0].Base() );
		meshBuilder.AdvanceVertex();
	}
	
	unsigned short *pIndex = m_CoreDispInfo.GetRenderIndexList();
	for ( i = 0; i < numIndices; ++i )
	{
		meshBuilder.Index( pIndex[i] );
		meshBuilder.AdvanceIndex();
	}
	
	meshBuilder.End();
	pMesh->Draw();

	// Reset the render mode.
	pRender->SetRenderMode( renderModeOld );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpdateNeighborDependencies( bool bDestroy )
{
	if( !bDestroy )
	{
		// reset and find new neighbors
		ResetNeighbors();
		FindNeighbors();
	}
	else
	{
		//
		// update edge neighbors
		//
		for( int i = 0; i < 4; i++ )
		{
			EditDispHandle_t handle = GetEdgeNeighbor( i );
			if( handle == EDITDISPHANDLE_INVALID )
				continue;

			CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
			pNeighborDisp->UpdateNeighborDependencies( false );
		}
		
		//
		// update corner neighbors
		//
		for( i = 0; i < 4; i++ )
		{
			int cornerCount = GetCornerNeighborCount( i );
			for( int j = 0; j < cornerCount; j++ )
			{
				EditDispHandle_t handle = GetCornerNeighbor( i, j );
				if( handle != EDITDISPHANDLE_INVALID )
				{
					CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
					pNeighborDisp->UpdateNeighborDependencies( false );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::FindNeighbors( void )
{
	//
	// find the current neighbors to "this" displacement
	//
	IWorldEditDispMgr *pDispMgr = GetActiveWorldEditDispManager();
	if( !pDispMgr )
		return;

	pDispMgr->FindWorldNeighbors( m_EditHandle );

	//
	// generate the vector field for neighboring surfaces (edges and corners)
	//
	for( int i = 0; i < NUM_EDGES_CORNERS; i++ )
	{
		EditDispHandle_t handle = m_EdgeNeighbors[i];
		if( handle != EDITDISPHANDLE_INVALID )
		{
			CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
			pNeighborDisp->ResetNeighbors();
			pDispMgr->FindWorldNeighbors( pNeighborDisp->GetEditHandle() );
		}

		int cornerCount = m_CornerNeighborCounts[i];
		if( cornerCount != 0 )
		{
			for( int j = 0; j < cornerCount; j++ )
			{
				handle = m_CornerNeighbors[i][j];
				if( handle != EDITDISPHANDLE_INVALID )
				{
					CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
					pNeighborDisp->ResetNeighbors();
					pDispMgr->FindWorldNeighbors( pNeighborDisp->GetEditHandle() );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::UpdateBoundingBox( void )
{
	Vector v;

	VectorFill( m_BBox[0], COORD_NOTINIT );
	VectorFill( m_BBox[1], -COORD_NOTINIT );

	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		m_CoreDispInfo.GetVert( i, v );
		
		if( v[0] < m_BBox[0][0] ) { m_BBox[0][0] = v[0]; }
		if( v[1] < m_BBox[0][1] ) { m_BBox[0][1] = v[1]; }
		if( v[2] < m_BBox[0][2] ) { m_BBox[0][2] = v[2]; }

		if( v[0] > m_BBox[1][0] ) { m_BBox[1][0] = v[0]; }
		if( v[1] > m_BBox[1][1] ) { m_BBox[1][1] = v[1]; }
		if( v[2] > m_BBox[1][2] ) { m_BBox[1][2] = v[2]; }
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Scale( float scale )
{
	// check for a change in scale
	if( scale == m_Scale )
		return;

	int size = GetSize();

	// scale the surface back to its original state and re-scale with the new
	// value
	if( m_Scale != 1.0f )
	{
		float adj = 1.0f / m_Scale;

		for( int i = 0; i < size; i++ )
		{
			// scale the vector field distance
			float dist = GetFieldDistance( i );
			dist *= adj;
			SetFieldDistance( i, dist );

			// scale the subdivision pos
			Vector vPos;
			GetSubdivPosition( i, vPos );
			vPos *= adj;
			SetSubdivPosition( i, vPos );
		}
	}

	for( int i = 0; i < size; i++ )
	{
		// scale the vector field distance
		float dist = GetFieldDistance( i );
		dist *= scale;
		SetFieldDistance( i, dist );
		
		// scale the subdivision pos
		Vector vPos;
		GetSubdivPosition( i, vPos );
		vPos *= scale;
		SetSubdivPosition( i, vPos );
	}

	m_Scale = scale;

	UpdateData();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::ApplyNoise( float min, float max, float rockiness )
{
	if( min == max )
		return;

	// initialize the paint data
	Paint_Init( DISPPAINT_CHANNEL_POSITION );

	//
	// clamp rockiness value between 0.0 and 1.0
	//
	if( rockiness < 0.0f ) { rockiness = 0.0f; }
	if( rockiness > 1.0f ) { rockiness = 1.0f; }

	float delta = max - min;
	float deltaBy2 = delta / 2.0f;

	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		//
		// get a noise value based on the points position
		//
		Vector v;
		GetVert( i, v );

		float noiseX = v.x + v.z;
		float noiseY = v.y + v.z;
		float noise = PerlinNoise2D( noiseX, noiseY, rockiness );

		//
		// clamp noise (can go a little higher and lower due to precision)
		//
		if( noise < -1.0f ) { noise = -1.0f; }
		if( noise > 1.0f ) { noise = 1.0f; }

		noise *= deltaBy2;
		noise += ( deltaBy2 + min );

		// apply noise to the subdivision normal direction
		Vector vNoise;
		
		GetFieldVector( i, vNoise );
		if( ( vNoise.x == 0 ) && ( vNoise.y == 0 ) && ( vNoise.z == 0 ) )
		{
			GetSubdivNormal( i, vNoise );
		}
		vNoise *= noise;
		vNoise += v;

		// set the paint value
		Paint_SetValue( i, vNoise );
	}

	Paint_Update();
}


//=============================================================================
//
// Load/Save Functions
//


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::PostLoad( void )
{
	Vector v;

	//
	// check the subdivision normals -- clean them up (old files)
	//
	bool bUpdateSubdivNormals = false;

	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		GetSubdivNormal( i, v );
		if( ( v.x == 0.0f ) && ( v.y == 0.0f ) && ( v.z == 0.0f ) )
		{
			bUpdateSubdivNormals = true;
			break;
		}
	}

	if( bUpdateSubdivNormals )
	{
		Vector vNormal;
		GetSurfNormal( vNormal );

		for( i = 0; i < size; i++ )
		{
			SetSubdivNormal( i, vNormal );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispDistancesCallback(CChunkFile *pFile, CMapDisp *pDisp)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispDistancesKeyCallback, pDisp));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szKey - 
//			szValue - 
//			pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispDistancesKeyCallback(const char *szKey, const char *szValue, CMapDisp *pDisp)
{
	float dispDistance;

	if (!strnicmp(szKey, "row", 3))
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy(szBuf, szValue);

		int nCols = (1 << pDisp->GetPower()) + 1;
		int nRow = atoi(&szKey[3]);

		char *pszNext = strtok(szBuf, " ");
		int nIndex = nRow * nCols;

		while (pszNext != NULL)
		{
			dispDistance = (float)atof(pszNext);
			pDisp->m_CoreDispInfo.SetFieldDistance( nIndex, dispDistance );
			pszNext = strtok(NULL, " ");
			nIndex++;
		}
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispOffsetsCallback(CChunkFile *pFile, CMapDisp *pDisp)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispOffsetsKeyCallback, pDisp));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szKey - 
//			szValue - 
//			pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispOffsetsKeyCallback(const char *szKey, const char *szValue, CMapDisp *pDisp)
{
	Vector subdivVector;

	if( !strnicmp( szKey, "row", 3 ) )
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy( szBuf, szValue );

		int nCols = ( 1 << pDisp->GetPower() ) + 1;
		int nRow = atoi( &szKey[3] );

		char *pszNext0 = strtok( szBuf, " " );
		char *pszNext1 = strtok( NULL, " " );
		char *pszNext2 = strtok( NULL, " " );

		int nIndex = nRow * nCols;

		while( ( pszNext0 != NULL ) && ( pszNext1 != NULL ) && ( pszNext2 != NULL ) )
		{
			subdivVector[0] = ( float )atof( pszNext0 );
			subdivVector[1] = ( float )atof( pszNext1 );
			subdivVector[2] = ( float )atof( pszNext2 );

			pDisp->m_CoreDispInfo.SetSubdivPosition( nIndex, subdivVector );

			pszNext0 = strtok( NULL, " " );
			pszNext1 = strtok( NULL, " " );
			pszNext2 = strtok( NULL, " " );

			nIndex++;
		}
	}

	return( ChunkFile_Ok );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispOffsetNormalsCallback(CChunkFile *pFile, CMapDisp *pDisp)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispOffsetNormalsKeyCallback, pDisp ));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szKey - 
//			szValue - 
//			pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispOffsetNormalsKeyCallback(const char *szKey, const char *szValue, 
															 CMapDisp *pDisp)
{
	Vector normalVector;

	if( !strnicmp( szKey, "row", 3 ) )
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy( szBuf, szValue );

		int nCols = ( 1 << pDisp->GetPower() ) + 1;
		int nRow = atoi( &szKey[3] );

		char *pszNext0 = strtok( szBuf, " " );
		char *pszNext1 = strtok( NULL, " " );
		char *pszNext2 = strtok( NULL, " " );

		int nIndex = nRow * nCols;

		while( ( pszNext0 != NULL ) && ( pszNext1 != NULL ) && ( pszNext2 != NULL ) )
		{
			normalVector[0] = ( float )atof( pszNext0 );
			normalVector[1] = ( float )atof( pszNext1 );
			normalVector[2] = ( float )atof( pszNext2 );

			pDisp->m_CoreDispInfo.SetSubdivNormal( nIndex, normalVector );

			pszNext0 = strtok( NULL, " " );
			pszNext1 = strtok( NULL, " " );
			pszNext2 = strtok( NULL, " " );

			nIndex++;
		}
	}

	return( ChunkFile_Ok );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szKey - 
//			szValue - 
//			pWorld - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispKeyCallback(const char *szKey, const char *szValue, CMapDisp *pDisp)
{
	if (!stricmp(szKey, "power"))
	{
		int	power;
		CChunkFile::ReadKeyValueInt( szValue, power );
		pDisp->SetPower( power );
	}
	else if (!stricmp(szKey, "uaxis"))
	{
		Vector mapAxis;
		CChunkFile::ReadKeyValueVector3( szValue, mapAxis );
		pDisp->SetHasMappingAxes( true );
		pDisp->m_MapAxes[0] = mapAxis;
	}
	else if (!stricmp(szKey, "vaxis"))
	{
		Vector mapAxis;
		CChunkFile::ReadKeyValueVector3( szValue, mapAxis );
		pDisp->SetHasMappingAxes( true );
		pDisp->m_MapAxes[1] = mapAxis;
	}
	else if( !stricmp( szKey, "startposition" ) )
	{
		Vector startPosition;
		CChunkFile::ReadKeyValueVector3( szValue, startPosition );
		CCoreDispSurface *pSurf = pDisp->m_CoreDispInfo.GetSurface();
		pSurf->SetPointStart( startPosition );
	}
#if 0
	else if (!stricmp(szKey, "mintess"))
	{
		int	minTess;
		CChunkFile::ReadKeyValueInt( szValue, minTess );
		pDisp->SetMinTess( minTess );
	}
	else if (!stricmp(szKey, "smooth"))
	{
		float smoothingAngle;
		CChunkFile::ReadKeyValueFloat( szValue, smoothingAngle );
		pDisp->SetSmoothingAngle( smoothingAngle );
	}
	else if( !stricmp( szKey, "alpha" ) )
	{
		Vector4D alphaValues;
		CChunkFile::ReadKeyValueVector4( szValue, alphaValues );

		for( int i = 0; i < 4; i++ )
		{
			pDisp->m_CoreDispInfo.SetSurfPointAlpha( i, alphaValues[i] );
		}
	}
#endif
	else if( !stricmp( szKey, "elevation" ) )
	{
		float elevation;
		CChunkFile::ReadKeyValueFloat( szValue, elevation );
		pDisp->SetElevation( elevation );
	}
	else if( !stricmp( szKey, "subdiv" ) )
	{
		int bSubdivided;
		CChunkFile::ReadKeyValueInt( szValue, bSubdivided );
		bool bSubdiv = ( bSubdivided != 0 );
		pDisp->SetSubdivided( bSubdiv );
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispAlphasCallback(CChunkFile *pFile, CMapDisp *pDisp)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispAlphasKeyCallback, pDisp));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispAlphasKeyCallback(const char *szKey, const char *szValue, CMapDisp *pDisp)
{
	float alpha;

	if (!strnicmp(szKey, "row", 3))
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy(szBuf, szValue);

		int nCols = (1 << pDisp->GetPower()) + 1;
		int nRow = atoi(&szKey[3]);

		char *pszNext = strtok(szBuf, " ");

		int nIndex = nRow * nCols;

		while (pszNext != NULL) 
		{
			alpha = (float)atof(pszNext);

			pDisp->m_CoreDispInfo.SetAlpha( nIndex, alpha );

			pszNext = strtok(NULL, " ");

			nIndex++;
		}
	}

	return(ChunkFile_Ok);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispTriangleTagsCallback(CChunkFile *pFile, CMapDisp *pDisp)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispTriangleTagsKeyCallback, pDisp));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispTriangleTagsKeyCallback(const char *szKey, const char *szValue, CMapDisp *pDisp)
{
	unsigned short nTriTag;

	if ( !strnicmp( szKey, "row", 3 ) )
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy( szBuf, szValue );

		int nCols = ( 1 << pDisp->GetPower() );
		int nRow = atoi( &szKey[3] );

		char *pszNext = strtok( szBuf, " " );

		int nIndex = nRow * nCols;
		int iTri = nIndex * 2;

		while ( pszNext != NULL ) 
		{
			nTriTag = ( unsigned int )atoi( pszNext );
			pDisp->m_CoreDispInfo.SetTriTagValue( iTri, nTriTag );
			pszNext = strtok( NULL, " " );
			iTri++;
		}
	}

	return( ChunkFile_Ok );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispNormalsCallback(CChunkFile *pFile, CMapDisp *pDisp)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispNormalsKeyCallback, pDisp));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadDispNormalsKeyCallback(const char *szKey, const char *szValue, CMapDisp *pDisp)
{
	Vector vectorFieldVector;

	if (!strnicmp(szKey, "row", 3))
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy(szBuf, szValue);

		int nCols = (1 << pDisp->GetPower()) + 1;
		int nRow = atoi(&szKey[3]);

		char *pszNext0 = strtok(szBuf, " ");
		char *pszNext1 = strtok(NULL, " ");
		char *pszNext2 = strtok(NULL, " ");

		int nIndex = nRow * nCols;

		while ((pszNext0 != NULL) && (pszNext1 != NULL) && (pszNext2 != NULL))
		{
			vectorFieldVector[0] = (float)atof(pszNext0);
			vectorFieldVector[1] = (float)atof(pszNext1);
			vectorFieldVector[2] = (float)atof(pszNext2);

			pDisp->m_CoreDispInfo.SetFieldVector( nIndex, vectorFieldVector );

			pszNext0 = strtok(NULL, " ");
			pszNext1 = strtok(NULL, " ");
			pszNext2 = strtok(NULL, " ");

			nIndex++;
		}
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::LoadVMF(CChunkFile *pFile)
{
	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("normals", (ChunkHandler_t)LoadDispNormalsCallback, this);
	Handlers.AddHandler("distances", (ChunkHandler_t)LoadDispDistancesCallback, this);
	Handlers.AddHandler("offsets", (ChunkHandler_t)LoadDispOffsetsCallback, this);
	Handlers.AddHandler("offset_normals", (ChunkHandler_t)LoadDispOffsetNormalsCallback, this);
	Handlers.AddHandler("alphas", (ChunkHandler_t)LoadDispAlphasCallback, this);
	Handlers.AddHandler("triangle_tags", (ChunkHandler_t)LoadDispTriangleTagsCallback, this );

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadDispKeyCallback, this);
	pFile->PopHandlers();

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Saves the displacement info into a special chunk in the MAP file.
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapDisp::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	ChunkFileResult_t eResult = pFile->BeginChunk("dispinfo");

	int power = GetPower();
	float elevation = GetElevation();

	Vector	startPosition;
	CCoreDispSurface *pSurf = m_CoreDispInfo.GetSurface();
	pSurf->GetPoint( 0, startPosition );

	int bSubdivided = ( int )IsSubdivided();

#if 0	// old
	Vector4D	alphaValues;
	for( int i = 0; i < 4; i++ )
	{
		alphaValues[i] = m_CoreDispInfo.GetSurfPointAlpha( i );
	}
#endif

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("power", power);
	}

#if 0	// old
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueVector3("uaxis", m_BDSurf.uAxis);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueVector3("vaxis", m_BDSurf.vAxis);
	}
#endif

	if( eResult == ChunkFile_Ok )
	{
		eResult = pFile->WriteKeyValueVector3( "startposition", startPosition );
	}

#if 0
	// old
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("mintess", minTess);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueFloat("smooth", smoothingAngle);
	}
	//
	// save the corner alpha values
	//
	if( eResult == ChunkFile_Ok )
	{
		eResult = pFile->WriteKeyValueVector4( "alpha", alphaValues );
	}
#endif

	if( eResult == ChunkFile_Ok )
	{
		eResult = pFile->WriteKeyValueFloat( "elevation", elevation );
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt( "subdiv", bSubdivided );
	}

    //
    // Save displacement map normals.
    //
	if (eResult == ChunkFile_Ok)
	{
		Vector vectorFieldVector;

		eResult = pFile->BeginChunk("normals");
		if (eResult == ChunkFile_Ok)
		{
			char szBuf[MAX_KEYVALUE_LEN];
			char szTemp[80];

			int nRows = (1 << power) + 1;;
			int nCols = nRows;
			
			for (int nRow = 0; nRow < nRows; nRow++)
			{
				bool bFirst = true;
				szBuf[0] = '\0';

				for (int nCol = 0; nCol < nCols; nCol++)
				{
					int nIndex = nRow * nCols + nCol;

					if (!bFirst)
					{
						strcat(szBuf, " ");
					}

					bFirst = false;
					m_CoreDispInfo.GetFieldVector( nIndex, vectorFieldVector );
					sprintf(szTemp, "%g %g %g", (double)vectorFieldVector[0], (double)vectorFieldVector[1], (double)vectorFieldVector[2]);
					strcat(szBuf, szTemp);
				}

				char szKey[10];
				sprintf(szKey, "row%d", nRow);
				eResult = pFile->WriteKeyValue(szKey, szBuf);
			}
		}

		if (eResult == ChunkFile_Ok)
		{
			eResult = pFile->EndChunk();
		}
	}

    //
    // Save displacement map distances.
	//
	if (eResult == ChunkFile_Ok)
	{
		float dispDistance;

		eResult = pFile->BeginChunk("distances");
		if (eResult == ChunkFile_Ok)
		{
			char szBuf[MAX_KEYVALUE_LEN];
			char szTemp[80];

			int nRows = (1 << power) + 1;
			int nCols = nRows;

			for (int nRow = 0; nRow < nRows; nRow++)
			{
				bool bFirst = true;
				szBuf[0] = '\0';

				for (int nCol = 0; nCol < nCols; nCol++)
				{
					int nIndex = nRow * nCols + nCol;

					if (!bFirst)
					{
						strcat(szBuf, " ");
					}

					bFirst = false;
					dispDistance = m_CoreDispInfo.GetFieldDistance( nIndex ); 
					sprintf(szTemp, "%g", (double)dispDistance);
					strcat(szBuf, szTemp);
				}

				char szKey[10];
				sprintf(szKey, "row%d", nRow);
				eResult = pFile->WriteKeyValue(szKey, szBuf);
			}
		}

		if (eResult == ChunkFile_Ok)
		{
			eResult = pFile->EndChunk();
		}
	}

    //
    // Save displacement map offset.
	//
	if (eResult == ChunkFile_Ok)
	{
		Vector subdivPos;

		eResult = pFile->BeginChunk( "offsets" );
		if( eResult == ChunkFile_Ok )
		{
			char szBuf[MAX_KEYVALUE_LEN];
			char szTemp[80];

			int nRows = (1 << power) + 1;
			int nCols = nRows;

			for (int nRow = 0; nRow < nRows; nRow++)
			{
				bool bFirst = true;
				szBuf[0] = '\0';

				for (int nCol = 0; nCol < nCols; nCol++)
				{
					int nIndex = nRow * nCols + nCol;

					if (!bFirst)
					{
						strcat(szBuf, " ");
					}

					bFirst = false;
					m_CoreDispInfo.GetSubdivPosition( nIndex, subdivPos );
					sprintf(szTemp, "%g %g %g", (double)subdivPos[0], (double)subdivPos[1], (double)subdivPos[2]);
					strcat(szBuf, szTemp);
				}

				char szKey[10];
				sprintf(szKey, "row%d", nRow);
				eResult = pFile->WriteKeyValue(szKey, szBuf);
			}
		}

		if (eResult == ChunkFile_Ok)
		{
			eResult = pFile->EndChunk();
		}
	}

    //
    // Save displacement subdivision normals
	//
	if (eResult == ChunkFile_Ok)
	{
		Vector subdivNormal;

		eResult = pFile->BeginChunk( "offset_normals" );
		if( eResult == ChunkFile_Ok )
		{
			char szBuf[MAX_KEYVALUE_LEN];
			char szTemp[80];

			int nRows = (1 << power) + 1;
			int nCols = nRows;

			for (int nRow = 0; nRow < nRows; nRow++)
			{
				bool bFirst = true;
				szBuf[0] = '\0';

				for (int nCol = 0; nCol < nCols; nCol++)
				{
					int nIndex = nRow * nCols + nCol;

					if (!bFirst)
					{
						strcat(szBuf, " ");
					}

					bFirst = false;
					m_CoreDispInfo.GetSubdivNormal( nIndex, subdivNormal );
					sprintf(szTemp, "%g %g %g", (double)subdivNormal[0], (double)subdivNormal[1], (double)subdivNormal[2]);
					strcat(szBuf, szTemp);
				}

				char szKey[10];
				sprintf(szKey, "row%d", nRow);
				eResult = pFile->WriteKeyValue(szKey, szBuf);
			}
		}

		if (eResult == ChunkFile_Ok)
		{
			eResult = pFile->EndChunk();
		}
	}

    //
    // Save displacement alphas
	//
	if (eResult == ChunkFile_Ok)
	{
		float alpha;

		eResult = pFile->BeginChunk( "alphas" );
		if( eResult == ChunkFile_Ok )
		{
			char szBuf[MAX_KEYVALUE_LEN];
			char szTemp[80];

			int nRows = (1 << power) + 1;
			int nCols = nRows;

			for (int nRow = 0; nRow < nRows; nRow++)
			{
				bool bFirst = true;
				szBuf[0] = '\0';

				for (int nCol = 0; nCol < nCols; nCol++)
				{
					int nIndex = nRow * nCols + nCol;

					if (!bFirst)
					{
						strcat(szBuf, " ");
					}

					bFirst = false;
					alpha = m_CoreDispInfo.GetAlpha( nIndex );
					sprintf(szTemp, "%g", (double)alpha);
					strcat(szBuf, szTemp);
				}

				char szKey[10];
				sprintf(szKey, "row%d", nRow);
				eResult = pFile->WriteKeyValue(szKey, szBuf);
			}
		}

		if (eResult == ChunkFile_Ok)
		{
			eResult = pFile->EndChunk();
		}
	}

    // Save Triangle data.
	if (eResult == ChunkFile_Ok)
	{
		unsigned short nTriTag;

		eResult = pFile->BeginChunk( "triangle_tags" );
		if( eResult == ChunkFile_Ok )
		{
			char szBuf[MAX_KEYVALUE_LEN];
			char szTemp[80];

			int nRows = ( 1 << power );			// ( 1 << power ) + 1 - 1
			int nCols = nRows;

			for ( int iRow = 0; iRow < nRows; ++iRow )
			{
				bool bFirst = true;
				szBuf[0] = '\0';

				for ( int iCol = 0; iCol < nCols; ++iCol )
				{
					int nIndex = iRow * nCols + iCol;
					int iTri = nIndex * 2;

					if ( !bFirst )
					{
						strcat( szBuf, " " );
					}
					bFirst = false;

					nTriTag = m_CoreDispInfo.GetTriTagValue( iTri );
					sprintf( szTemp, "%d", (int)nTriTag );
					strcat( szBuf, szTemp );

					nTriTag = m_CoreDispInfo.GetTriTagValue( iTri + 1 );
					sprintf( szTemp, " %d", (int)nTriTag );
					strcat( szBuf, szTemp );
				}

				char szKey[10];
				sprintf( szKey, "row%d", iRow );
				eResult = pFile->WriteKeyValue( szKey, szBuf );
			}
		}

		if (eResult == ChunkFile_Ok)
		{
			eResult = pFile->EndChunk();
		}
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::SerializedLoadMAP( fstream &file, CMapFace *pFace, UINT version )
{
	int		power;
	float	maxData;
	int		minTess;
	float	smoothingAngle;
	Vector	vectorFieldVector;
	float	distance;

	//
    // read off the first line -- burn it!!! and get the second
	//
    static char buf[256];
    file.getline( buf, 256 );
    file.getline( buf, 256 );

	if( version < 350 )
	{
		sscanf( buf, "%d [ %f %f %f ] [ %f %f %f ] %f %d %f",
					&power,
					&m_MapAxes[0][0], &m_MapAxes[0][1], &m_MapAxes[0][2],
					&m_MapAxes[1][0], &m_MapAxes[1][1], &m_MapAxes[1][2],
					&maxData,
					&minTess,
					&smoothingAngle );
	}
	else
	{
		sscanf( buf, "%d [ %f %f %f ] [ %f %f %f ] %d %f",
					&power,
					&m_MapAxes[0][0], &m_MapAxes[0][1], &m_MapAxes[0][2],
					&m_MapAxes[1][0], &m_MapAxes[1][1], &m_MapAxes[1][2],
					&minTess,
					&smoothingAngle );
	}

	m_CoreDispInfo.SetPower( power );

	m_bHasMappingAxes = true;

    //
    // displacement normals
    //
	int size = GetSize();
    for( int i = 0; i < size; i++ )
    {
        file >> vectorFieldVector[0];
        file >> vectorFieldVector[1];
        file >> vectorFieldVector[2];

		m_CoreDispInfo.SetFieldVector( i, vectorFieldVector );
    }
    file.getline( buf, 256 );

    //
    // displacement distances
    //
    for( i = 0; i < size; i++ )
    {
		if( version < 350 )
		{
	        file >> distance;
			distance *= maxData;
		}
		else
		{
	        file >> distance;
		}

		m_CoreDispInfo.SetFieldDistance( i, distance );
    }
    file.getline( buf, 256 );

    // finish the last bit of the "chunk"
    file.getline( buf, 256 );

    // save the parent info
    SetParent( pFace );

    return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::SerializedLoadRMF( fstream &file, CMapFace *pFace, float version )
{
	int		power;
	int		minTess;
	float	smoothingAngle;
	Vector	vectorFieldVectors[MAPDISP_MAX_VERTS];
	float	distances[MAPDISP_MAX_VERTS];

    //
    // get displacement information
    //
	file.read( ( char* )&power, sizeof( int ) );
	file.read( ( char* )m_MapAxes[0].Base(), 3 * sizeof( float ) );
	file.read( ( char* )m_MapAxes[1].Base(), 3 * sizeof( float ) );
	file.read( ( char* )&minTess, sizeof( int ) );
	file.read( ( char* )&smoothingAngle, sizeof( float ) );

	m_CoreDispInfo.SetPower( power );

	m_bHasMappingAxes = true;

    //
    // get displacement map normals and distances
    //
    int size = GetSize();
	int i;
	for ( i = 0; i < size; ++i)
	{
		file.read( ( char* )&vectorFieldVectors[i], 3 * sizeof( float ) );
	}
    file.read( ( char* )distances, size * sizeof( float ) );

	for( i = 0; i < size; i++ )
	{
		m_CoreDispInfo.SetFieldVector( i, vectorFieldVectors[i] );
		m_CoreDispInfo.SetFieldDistance( i, distances[i] );
	}

    // set the parent
    SetParent( pFace );

    // displacement info loaded
    return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CMapDisp::GetEndIndexFromLevel( int levelIndex )
{
	switch( levelIndex )
	{
	case 2: { return 20; }
	case 3: { return 84; }
	case 4: { return 340; }
	default: { return 0; }
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CMapDisp::GetStartIndexFromLevel( int levelIndex )
{
	switch( levelIndex )
	{
	case 2: { return 5; }
	case 3: { return 21; }
	case 4: { return 85; }
	default: { return 0; }
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CreateDispSelectionList( Selection3D *pSelectionList, IWorldEditDispMgr *pDispMgr )
{
	// clear the selection list
	pDispMgr->SelectClear();

    // get the selection list head position
    int nSelCount = pSelectionList->GetCount();

    //
    // find all displacements and add them to the displacement selection list
    //
    for (int nSelIndex = 0; nSelIndex < nSelCount; nSelIndex++)
    {
        // get the next object in the list
        CMapClass *pObject = pSelectionList->GetObject( nSelIndex );

        //
        // check to see if object is a solid
        //
        if( pObject->IsMapClass( MAPCLASS_TYPE( CMapSolid ) ) )
        {
            CMapSolid *pSolid = ( CMapSolid* )pObject;
            
            //
            // get face info and check for displacement faces
            //
            CMapFace *pFace;
            for( int i = 0; i < pSolid->GetFaceCount(); i++ )
            {
                pFace = pSolid->GetFace( i );
                if( pFace->HasDisp() )
                {
					pDispMgr->AddToSelect( pFace->GetDisp() );
				}
            }
        }
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void UpdateDispTrans( Selection3D *pSelectionList )
{
	//
	// create a displacement seleciton list
	//
	IWorldEditDispMgr *pDispMgr = GetActiveWorldEditDispManager();
	if( !pDispMgr )
		return;

	CToolDisplace *pDispTool = g_pToolManager->GetDisplacementTool();
	if ( !pDispTool )
		return;

	CreateDispSelectionList( pSelectionList, pDispMgr );

	//
	// check each displacement in list to see if all of its neighbors reside in the selection
	// list as well, if so then the displacement surface needs to be "re-subdivided"
	//
	int listCount = pDispMgr->SelectCount();
	if ( listCount == 0 )
		return;

	for( int i = 0; i < listCount; i++ )
	{
		CMapDisp *pDisp = pDispMgr->GetFromSelect( i );
		if( !pDisp )
			continue;

		if( pDisp->IsSubdivided() )
		{
			bool bNeighborsInList = true;
			bool bHasNeighbors = false;
			
			//
			// look for edge neighbors in selection list
			//
			for( int j = 0; j < 4; j++ )
			{
				CMapDisp *pNeighborDisp;
				int	neighborOrient;
				EditDispHandle_t handle;
				pDisp->GetEdgeNeighbor( j, handle, neighborOrient );
				if( handle == EDITDISPHANDLE_INVALID )
					continue;

				pNeighborDisp = EditDispMgr()->GetDisp( handle );				
				bHasNeighbors = true;

				if( !pDispMgr->IsInSelect( pNeighborDisp->GetEditHandle() ) )
				{
					bNeighborsInList = false;
				}
			}
			
			//
			// look for corner neighbors in selection list (if necessary)
			//
			if( bNeighborsInList )
			{
				for( j = 0; j < 4; j++ )
				{
					int cornerCount = pDisp->GetCornerNeighborCount( j );
					for( int k = 0; k < cornerCount; k++ )
					{
						CMapDisp *pNeighborDisp;
						int	neighborOrient;
						EditDispHandle_t handle;
						pDisp->GetCornerNeighbor( j, k, handle, neighborOrient );
						if( handle == EDITDISPHANDLE_INVALID )
							continue;

						pNeighborDisp = EditDispMgr()->GetDisp( handle );						
						bHasNeighbors = true;
						
						if( !pDispMgr->IsInSelect( pNeighborDisp->GetEditHandle() ) )
						{
							bNeighborsInList = false;
						}
					}
				}
			}

			if( bHasNeighbors && bNeighborsInList )
			{
				pDisp->SetReSubdivision( true );
			}
		}

		// reset the surface data and vector, offset fields (keeps disp distances)
		pDisp->UpdateSurfData( ( CMapFace* )pDisp->GetParent() );
	}

	//
	// re-subdivide the necessary surfaces
	//
	pDispMgr->SelectClear();
	int worldListCount = pDispMgr->WorldCount();
	for( i = 0; i < worldListCount; i++ )
	{
		CMapDisp *pDisp = pDispMgr->GetFromWorld( i );
		if( !pDisp )
			continue;

		if( pDisp->NeedsReSubdivision() && pDispTool->GetResubdivideState() )
		{
			pDispMgr->AddToSelect( pDisp->GetEditHandle() );
		}
	}

	pDispMgr->CatmullClarkSubdivide();

	// reset the seleciton list
	pDispMgr->SelectClear();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::RotateFieldData( QAngle const &vAngles )
{
	matrix4_t mat;
	IdentityMatrix( mat );

	// find the axis of rotation
	if( vAngles.x != 0.0f )
	{
		RotateX( mat, vAngles[0] );
	}
	else if( vAngles.y != 0.0f )
	{
		RotateY( mat, vAngles[1] );
	}
	else 
	{
		RotateZ( mat, vAngles[2] );
	}

	Vector v, vTmp;
	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		GetFieldVector( i, v );
		MatrixMultiply( vTmp, v, mat );
		SetFieldVector( i, vTmp );

		GetSubdivPosition( i, v );
		if( ( v.x != 0.0f ) && ( v.y != 0.0f ) && ( v.z != 0.0f ) )
		{
			MatrixMultiply( vTmp, v, mat );
			SetSubdivPosition( i, vTmp );
		}

		GetSubdivNormal( i, v );
		MatrixMultiply( vTmp, v, mat );
		SetSubdivNormal( i, vTmp );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool SphereTriEdgePlanesIntersection( Vector const &ptCenter, float radius, cplane_t *pPlanes )
{
	// check all planes
	for( int ndxPlane = 0; ndxPlane < 3; ndxPlane++ )
	{
		float dist = pPlanes[ndxPlane].normal.Dot( ptCenter ) - pPlanes[ndxPlane].dist;
		if( dist > radius )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::PointSurfIntersection( Vector const &ptCenter, float radius, float &distMin,
							          Vector &ptMin )
{
	// initialize the min data
	distMin = radius;
	ptMin.Init();

	// square the radius
	float sqrRadius = radius * radius;

	//
	// get the render list size -- created triangles
	//
	unsigned short *pTriList = m_CoreDispInfo.GetRenderIndexList();
	int listSize = m_CoreDispInfo.GetRenderIndexCount();
	for( int i = 0; i < listSize; i += 3 )
	{
		// get the triangle
		Vector v[3];
		GetVert( pTriList[i], v[0] );
		GetVert( pTriList[i+1], v[1] );
		GetVert( pTriList[i+2], v[2] );

		//
		// create a triangle plane
		//
		Vector seg0, seg1;
		seg0 = v[1] - v[0];
		seg1 = v[2] - v[0];
		cplane_t triPlane;
		triPlane.normal = seg1.Cross( seg0 );
		VectorNormalize( triPlane.normal );
		triPlane.dist = triPlane.normal.Dot( v[0] );

		//
		// plane sphere intersection
		//
		float dist = triPlane.normal.Dot( ptCenter ) - triPlane.dist;
		if( fabs( dist ) < distMin )
		{
			//
			// create edge plane data
			//
			cplane_t edgePlanes[3];
			Vector edges[3];
			edges[0] = v[1] - v[0];
			edges[1] = v[2] - v[1];
			edges[2] = v[0] - v[2];

			for( int j = 0; j < 3; j++ )
			{
				edgePlanes[j].normal = triPlane.normal.Cross( edges[j] );
				VectorNormalize( edgePlanes[j].normal );
				edgePlanes[j].dist = edgePlanes[j].normal.Dot( v[j] );

				// check normal facing
				float distPt = edgePlanes[j].normal.Dot( v[(j+2)%3] ) - edgePlanes[j].dist;
				if( distPt > 0.0f )
				{
					edgePlanes[j].normal.Negate();
					edgePlanes[j].dist = -edgePlanes[j].dist;
				}
			}

			// intersect sphere with triangle
			bool bSphereIntersect = SphereTriEdgePlanesIntersection( ptCenter, distMin, edgePlanes );

			//
			// check to see if the center lies behind all the edge planes
			//
			if( bSphereIntersect )
			{
				bool bPointInside = SphereTriEdgePlanesIntersection( ptCenter, 0.0f, edgePlanes );

				if( bPointInside )
				{
					distMin = fabs( dist );
					ptMin = ptCenter - ( triPlane.normal * dist );
				}
				else
				{
					// check distance to points
					for( int k = 0; k < 3; k++ )
					{
						Vector vTmp;
						vTmp = ptCenter - v[k];
						float distPt = ( float )sqrt( vTmp.Dot( vTmp ) );
						if( distPt < distMin )
						{
							distMin = distPt;
							ptMin = v[k];
						}
					}
				}
			}
		}
	}

	if( distMin != radius )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
EditDispHandle_t CMapDisp::GetHitDispMap( void )
{
	if( m_HitDispIndex == -1 )
	{
		CMapFace *pFace = ( CMapFace* )GetParent();
		return pFace->GetDisp();
	}

	if( m_HitDispIndex <= 3 )
	{
		return m_EdgeNeighbors[m_HitDispIndex];
	}

	return m_CornerNeighbors[m_HitDispIndex-4][2];
}


//-----------------------------------------------------------------------------
// Purpose: UNDO is messy to begin with, and now with handles it gets even 
//          more fun!!!  Call through here to setup undo!!
//-----------------------------------------------------------------------------
void EditDisp_ForUndo( EditDispHandle_t editHandle, char *pszPositionName,
					   bool bNeighborsUndo )
{
	// sanity check on handle
	if( editHandle == EDITDISPHANDLE_INVALID )
		return;

	// get the current displacement given the handle
	CMapDisp *pDisp = EditDispMgr()->GetDisp( editHandle );

	//
	// set the undo name if necessary
	//
	if( pszPositionName )
	{
		GetHistory()->MarkUndoPosition( NULL, pszPositionName );
	}

	//
	// get the solid (face) for the UNDO history
	//
	CMapFace *pFace = ( CMapFace* )pDisp->GetParent();
	CMapSolid *pSolid = ( CMapSolid* )pFace->GetParent();
	GetHistory()->Keep( ( CMapClass* )pSolid );
	
	//
	// neighbors in undo as well
	//
	if ( bNeighborsUndo )
	{
		for ( int ndxNeighbor = 0; ndxNeighbor < 4; ndxNeighbor++ )
		{
			// displacement pointer could have changed due to the undo/copyfrom above
			pDisp = EditDispMgr()->GetDisp( editHandle );

			//
			// edge neighbors
			//
			int neighborOrient;
			EditDispHandle_t neighborHandle;
			pDisp->GetEdgeNeighbor( ndxNeighbor, neighborHandle, neighborOrient );
			if( neighborHandle != EDITDISPHANDLE_INVALID )
			{
				CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( neighborHandle );
				CMapFace *pNeighborFace = ( CMapFace* )pNeighborDisp->GetParent();
				CMapSolid *pNeighborSolid = ( CMapSolid* )pNeighborFace->GetParent();
				GetHistory()->Keep( ( CMapClass* )pNeighborSolid );

				// displacement pointer could have changed due to the undo/copyfrom above
				pDisp = EditDispMgr()->GetDisp( editHandle );
			}

			//
			// corner neighbors
			//
			int cornerCount = pDisp->GetCornerNeighborCount( ndxNeighbor );
			if( cornerCount > 0 )
			{
				for( int ndxCorner = 0; ndxCorner < cornerCount; ndxCorner++ )
				{
					pDisp->GetCornerNeighbor( ndxNeighbor, ndxCorner, neighborHandle, neighborOrient );
					CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( neighborHandle );
					CMapFace *pNeighborFace = ( CMapFace* )pNeighborDisp->GetParent();
					CMapSolid *pNeighborSolid = ( CMapSolid* )pNeighborFace->GetParent();
					GetHistory()->Keep( ( CMapClass* )pNeighborSolid );

					// displacement pointer could have changed due to the undo/copyfrom above
					pDisp = EditDispMgr()->GetDisp( editHandle );
				}
			}
		}
	}
}


//=============================================================================
//
// Painting Functions
//

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Paint_Init( int nType )
{
	m_Canvas.m_nType = nType;
	m_Canvas.m_bDirty = false;

	int nVertCount = GetSize();
	for( int iVert = 0; iVert < nVertCount; iVert++ )
	{
		m_Canvas.m_Values[iVert].Init();
		m_Canvas.m_bValuesDirty[iVert] = false;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Paint_InitSelfAndNeighbors( int nType )
{
	// Initialiuze self.
	Paint_Init( nType );

	// Initialize neighbors.
	for( int iEdge = 0; iEdge < 4; iEdge++ )
	{
		EditDispHandle_t handle = GetEdgeNeighbor( iEdge );
		if( handle != EDITDISPHANDLE_INVALID )
		{
			CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
			pNeighborDisp->Paint_Init( nType );
		}

		int nCornerCount = GetCornerNeighborCount( iEdge );
		if( nCornerCount > 0 )
		{
			for( int iCorner = 0; iCorner < nCornerCount; iCorner++ )
			{
				handle = GetCornerNeighbor( iEdge, iCorner );
				if( handle != EDITDISPHANDLE_INVALID )
				{
					CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
					pNeighborDisp->Paint_Init( nType );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Paint_SetValue( int iVert, Vector const &vPaint )
{
	Assert( iVert >= 0 );
	Assert( iVert < MAPDISP_MAX_VERTS );

	VectorCopy( vPaint, m_Canvas.m_Values[iVert] );
	m_Canvas.m_bValuesDirty[iVert] = true;
	m_Canvas.m_bDirty = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::PaintAlpha_Update( int iVert )
{
	SetAlpha( iVert, m_Canvas.m_Values[iVert].x );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::PaintPosition_Update( int iVert )
{
	Vector vSPos, vFlat;
	GetFlatVert( iVert, vFlat );
	GetSubdivPosition( iVert, vSPos );
				
	Vector vSeg;
	vSeg = m_Canvas.m_Values[iVert] - vFlat;
	vSeg -= vSPos;
				
	// Subtract out the elevation.
	float elev = GetElevation();
	if( elev != 0.0 )
	{
		Vector vNormal;
		GetSurfNormal( vNormal );
		vNormal *= elev;
		
		vSeg -= vNormal;
	}
				
	float flDistance = VectorNormalize( vSeg );
	
	SetFieldVector( iVert, vSeg );
	SetFieldDistance( iVert, flDistance );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Paint_Update( void )
{
	// Check for changes to the canvas.
	if ( !m_Canvas.m_bDirty )
		return;

	int nVertCount = GetSize();
	for ( int iVert = 0; iVert < nVertCount; iVert++ )
	{
		// Check for changes at the vertex.
		if ( m_Canvas.m_bValuesDirty[iVert] )
		{
			if ( m_Canvas.m_nType == DISPPAINT_CHANNEL_POSITION )
			{
				PaintPosition_Update( iVert );
			}
			else if ( m_Canvas.m_nType == DISPPAINT_CHANNEL_ALPHA )
			{
				PaintAlpha_Update( iVert );
			}
		}
	}

	// Update the displacement surface.
	UpdateData();
	CheckAndUpdateOverlays();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::Paint_UpdateSelfAndNeighbors( void )
{
	// Update self.
	Paint_Update();

	// Update neighbors.
	for( int iEdge = 0; iEdge < 4; iEdge++ )
	{
		EditDispHandle_t handle = GetEdgeNeighbor( iEdge );
		if( handle != EDITDISPHANDLE_INVALID )
		{
			CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
			pNeighborDisp->Paint_Update();
		}

		int nCornerCount = GetCornerNeighborCount( iEdge );
		if( nCornerCount > 0 )
		{
			for( int iCorner = 0; iCorner < nCornerCount; iCorner++ )
			{
				handle = GetCornerNeighbor( iEdge, iCorner );
				if( handle != EDITDISPHANDLE_INVALID )
				{
					CMapDisp *pNeighborDisp = EditDispMgr()->GetDisp( handle );
					pNeighborDisp->Paint_Update();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::SetSelectMask( bool bSelectMask ) 
{ 
	m_bSelectMask = bSelectMask; 
}
	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::HasSelectMask( void ) 
{ 
	return m_bSelectMask; 
}	

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapDisp::SetGridMask( bool bGridMask ) 
{ 
	m_bGridMask = bGridMask; 
}
	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapDisp::HasGridMask( void ) 
{ 
	return m_bGridMask; 
}

//-----------------------------------------------------------------------------
// Purpose: Do the dumb thing first and optimize later??
//-----------------------------------------------------------------------------
int CMapDisp::CollideWithDispTri( const Vector &rayStart, const Vector &rayEnd )
{
	int nTriCount = GetTriCount();
	for ( int iTri = 0; iTri < nTriCount; ++iTri )
	{
		unsigned short v1, v2, v3;
		GetTriIndices( iTri, v1, v2, v3 );
		Vector vec1, vec2, vec3;
		GetVert( v1, vec1 );
		GetVert( v2, vec2 );
		GetVert( v3, vec3 );

		Ray_t ray;
		ray.Init( rayStart, rayEnd, Vector( 0.0f, 0.0f, 0.0f ), Vector ( 0.0f, 0.0f, 0.0f ) );

		if ( IntersectRayWithTriangle( ray, vec1, vec2, vec3, false ) > 0.0f )
			return iTri;
	}

	// No collision.
	return -1;
}
