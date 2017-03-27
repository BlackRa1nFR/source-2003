//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "vrad.h"
#include "VRAD_DispColl.h"
#include "DispColl_Common.h"
#include "radial.h"
#include "CollisionUtils.h"
#include "tier0\dbg.h"

#define SAMPLE_BBOX_SLOP	5.0f
#define TRIEDGE_EPSILON		0.001f

static FileHandle_t pDispFile = FILESYSTEM_INVALID_HANDLE;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CVRADDispColl::CVRADDispColl() : m_pLuxelCoords(0)
{
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CVRADDispColl::~CVRADDispColl()
{
	if (m_pLuxelCoords)
		delete[] m_pLuxelCoords;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::Create( CCoreDispInfo *pDisp )
{
	// save the face index for later reference
	CCoreDispSurface *pSurf = pDisp->GetSurface();
	m_ndxParent = pSurf->GetHandle();

	// save the size of the displacement surface
	m_Power = pDisp->GetPower();

	// save the displacement to base face point offset
	m_PointOffset = pSurf->GetPointStartIndex();

	//
	// get base surface points and normals
	//
	int i;
	m_PointCount = pSurf->GetPointCount();
	for( i = 0; i < m_PointCount; i++ )
	{
		Vector point, normal;
		pSurf->GetPoint( i, point );
		pSurf->GetPointNormal( i, normal );

		m_Points[i].Init( point[0], point[1], point[2] );
		m_PointNormals[i].Init( normal[0], normal[1], normal[2] );
	}

	// create the collision data
	if( !CDispCollTree::Create( pDisp ) )
		return false;

	// Set up the luxel data
	m_pLuxelCoords = new Vector2D[GetSize()];
	assert( m_pLuxelCoords );
	for ( i = GetSize(); --i >= 0; )
	{
		pDisp->GetLuxelCoord( 0, i, m_pLuxelCoords[i] );	
	}


	//
	// re-calculate the lightmap size (in uv) so that the luxels give
	// a better world-space uniform approx. due to the non-linear nature
	// of the displacement surface in uv-space
	//
	dface_t *pFace = &dfaces[m_ndxParent];
	if( pFace )
	{
		CalcSampleRadius2AndBox( pFace );	
	}

	//
	// create the vrad specific node data - build for radiosity transfer,
	// don't create for direct lighting only!
	//
	m_pVNodes = new VNode_t[m_NodeCount];
	if( !m_pVNodes )
		return false;
	
	BuildVNodes();

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::CalcSampleRadius2AndBox( dface_t *pFace )
{
	texinfo_t *pTex = &texinfo[pFace->texinfo];
	if( !pTex )
		return;

	Vector boxMin, boxMax;
	GetSurfaceMinMax( boxMin, boxMax );

#if 0
	//
	// get the box width, height projected into the lightmap space
	// NOTE: use the texture projection axes due to the face that the
	//       lightmap vecs are the same divided by the surface area
	//
	float width = ( ( boxMax.x * pTex->textureVecsTexelsPerWorldUnits[0][0] +
		              boxMax.y * pTex->textureVecsTexelsPerWorldUnits[0][1] +
					  boxMax.z * pTex->textureVecsTexelsPerWorldUnits[0][2] ) -
					( boxMin.x * pTex->textureVecsTexelsPerWorldUnits[0][0] +
					  boxMin.y * pTex->textureVecsTexelsPerWorldUnits[0][1] +
					  boxMin.z * pTex->textureVecsTexelsPerWorldUnits[0][2] ) );

	float height = ( ( boxMax.x * pTex->textureVecsTexelsPerWorldUnits[1][0] +
		               boxMax.y * pTex->textureVecsTexelsPerWorldUnits[1][1] +
				   	   boxMax.z * pTex->textureVecsTexelsPerWorldUnits[1][2] ) -
					 ( boxMin.x * pTex->textureVecsTexelsPerWorldUnits[1][0] +
					   boxMin.y * pTex->textureVecsTexelsPerWorldUnits[1][1] +
					   boxMin.z * pTex->textureVecsTexelsPerWorldUnits[1][2] ) );
#endif

	Vector vBaseFaceNormal;
	GetStabDirection( vBaseFaceNormal );
	int axis0, axis1;
	GetMinorAxes( vBaseFaceNormal, axis0, axis1 );

	float width = boxMax[axis0] - boxMin[axis0];
	float height = boxMax[axis1] - boxMin[axis1];

	width /= pFace->m_LightmapTextureSizeInLuxels[0];
	height /= pFace->m_LightmapTextureSizeInLuxels[1];

	// calculate the sample radius squared
	float sampleRadius = sqrt( ( ( width*width) + ( height*height) ) ) * RADIALDIST2; 
	m_SampleRadius2 = sampleRadius * sampleRadius;

	float patchSampleRadius = sqrt( ( maxchop*maxchop ) + ( maxchop*maxchop ) ) * RADIALDIST2;
	m_PatchSampleRadius2 = patchSampleRadius * patchSampleRadius;

	//
	// calculate the sampling bounding box
	//
	m_SampleBBox[0] = boxMin;
	m_SampleBBox[1] = boxMax;
	for( int i = 0; i < 3; i++ )
	{
		m_SampleBBox[0][i] -= SAMPLE_BBOX_SLOP;
		m_SampleBBox[1][i] += SAMPLE_BBOX_SLOP;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::GetSurfaceMinMax( Vector &boxMin, Vector &boxMax )
{
	// initialize the minimum and maximum box
	boxMin = m_pVerts[0];
	boxMax = m_pVerts[0];

	for( int i = 1; i < m_VertCount; i++ )
	{
		if( m_pVerts[i].x < boxMin.x ) { boxMin.x = m_pVerts[i].x; }
		if( m_pVerts[i].y < boxMin.y ) { boxMin.y = m_pVerts[i].y; }
		if( m_pVerts[i].z < boxMin.z ) { boxMin.z = m_pVerts[i].z; }

		if( m_pVerts[i].x > boxMax.x ) { boxMax.x = m_pVerts[i].x; }
		if( m_pVerts[i].y > boxMax.y ) { boxMax.y = m_pVerts[i].y; }
		if( m_pVerts[i].z > boxMax.z ) { boxMax.z = m_pVerts[i].z; }
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::BuildVNodes( void )
{
	//
	// get leaf indices (last level in tree)
	//
	int ndxStart = Nodes_CalcCount( m_Power - 1 );
	int ndxEnd = Nodes_CalcCount( m_Power );

	int width = GetWidth();

	for( int ndxNode = ndxStart; ndxNode < ndxEnd; ndxNode++ )
	{
		// get the current node
		Node_t *pNode = &m_pNodes[ndxNode];
		VNode_t *pVNode = &m_pVNodes[ndxNode];

		if( pNode )
		{
			//
			// get the surface area of the triangles in the node
			//
			int ndxTri;
			Vector tris[2][3];
			Vector triNormals[2][3];
			for( ndxTri = 0; ndxTri < 2; ndxTri++ )	
			{
				for( int ndxVert = 0; ndxVert < 3; ndxVert++ )
				{
					tris[ndxTri][ndxVert] = m_pVerts[m_pTris[pNode->m_iTris[ndxTri]].m_uiVerts[ndxVert]];;
					triNormals[ndxTri][ndxVert] = m_pVertNormals[m_pTris[pNode->m_iTris[ndxTri]].m_uiVerts[ndxVert]];
				}
			}

			pVNode->patchArea = 0.0f;
			for( ndxTri = 0; ndxTri < 2; ndxTri++ )
			{
				Vector edge0, edge1, cross;
				VectorSubtract( tris[ndxTri][1], tris[ndxTri][0], edge0 );
				VectorSubtract( tris[ndxTri][2], tris[ndxTri][0], edge1 );
				CrossProduct( edge0, edge1, cross );
				pVNode->patchArea += 0.5f * VectorLength( cross );
			}			

			//
			// get the patch origin (along the diagonal!)
			//
			int ndxVert;
			int	   edgePtCount = 0;
			Vector edgePt[2];
			int	   edgeIndices[2];
			pVNode->patchOrigin.Init();
			for( ndxVert = 0; ndxVert < 3; ndxVert++ )
			{
				for( int ndxVert2 = 0; ndxVert2 < 3; ndxVert2++ )
				{
					if( m_pTris[pNode->m_iTris[0]].m_uiVerts[ndxVert] == m_pTris[pNode->m_iTris[1]].m_uiVerts[ndxVert2] )
					{
						edgeIndices[edgePtCount] = m_pTris[pNode->m_iTris[0]].m_uiVerts[ndxVert];
						edgePt[edgePtCount] = tris[0][ndxVert];
						edgePtCount++;
						break;
					}
				}
			}
			pVNode->patchOrigin = ( edgePt[0] + edgePt[1] ) * 0.5f;

			Vector2D uv0, uv1;
			float scale = 1.0f / ( float )( width - 1 );
			uv0.x = edgeIndices[0] % width;
			uv0.y = edgeIndices[0] / width;
			uv0.x *= scale;
			uv0.y *= scale;

			uv1.x = edgeIndices[1] % width;
			uv1.y = edgeIndices[1] / width;
			uv1.x *= scale;
			uv1.y *= scale;
			
			pVNode->patchOriginUV = ( uv0 + uv1 ) * 0.5f;

			if( pVNode->patchOriginUV.x > 1.0f || pVNode->patchOriginUV.x < 0.0f )
				_asm int 3;

			if( pVNode->patchOriginUV.y > 1.0f || pVNode->patchOriginUV.y < 0.0f )
				_asm int 3;

			//
			// get the averaged patch normal
			//
			pVNode->patchNormal.Init();
			for( ndxVert = 0; ndxVert < 3; ndxVert++ )
			{
				VectorAdd( pVNode->patchNormal, triNormals[0][ndxVert], pVNode->patchNormal );

				if( ( tris[1][ndxVert] != tris[0][0] ) &&
					( tris[1][ndxVert] != tris[0][1] ) &&
					( tris[1][ndxVert] != tris[0][2] ) )
				{
					VectorAdd( pVNode->patchNormal, triNormals[1][ndxVert], pVNode->patchNormal );
				}
			}
			VectorNormalize( pVNode->patchNormal );

			//
			// copy the bounds
			//
			pVNode->patchBounds[0] = pNode->m_BBox[0];
			pVNode->patchBounds[1] = pNode->m_BBox[1];
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::GetMinorAxes( Vector const &vNormal, int &axis0, int &axis1 )
{
	axis0 = 0;
	axis1 = 1;

	if( fabs( vNormal.x ) > fabs( vNormal.y ) )
	{
		if( fabs( vNormal.x ) > fabs( vNormal.z ) )
		{
			axis0 = 1;
			axis1 = 2;
		}
	}
	else
	{
		if( fabs( vNormal.y ) > fabs( vNormal.z ) )
		{
			axis0 = 0;
			axis1 = 2;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::ClampUV( float &value )
{
	if( value < 0.0f ) 
	{ 
		value = 0.0f; 
	}
	else if( value > 1.0f ) 
	{ 
		value = 1.0f; 
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::BaseFacePlaneToDispUV( Vector const &planePt, Vector2D &dispUV )
{
	PointInQuadToBarycentric( m_Points[0], m_Points[3], m_Points[2], m_Points[1], planePt, dispUV );
	return;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurfDiagonalBLtoTR( int snapU, int snapV, int nextU, int nextV,
											    float fracU, float fracV, Vector &surfPt,
												float pushEps )
{
	// get displacement width
	int width = GetWidth();

	if( ( fracU - fracV ) >= TRIEDGE_EPSILON )
	{
		int triIndices[3];
		triIndices[0] = snapV * width + snapU;
		triIndices[1] = nextV * width + nextU;
		triIndices[2] = snapV * width + nextU;

		Vector triVerts[3];
		for( int i = 0; i < 3; i++ )
		{
			GetVert( triIndices[i], triVerts[i] );
		}

		Vector edgeU, edgeV;
		edgeU = triVerts[0] - triVerts[2];
		edgeV = triVerts[1] - triVerts[2];

		surfPt = triVerts[2] + edgeU * ( 1.0f - fracU ) + edgeV * fracV;

		// get face normal and push point away from surface the given pushEps
		Vector vNormal;
		vNormal = CrossProduct( edgeV, edgeU );
		VectorNormalize( vNormal );
		surfPt += ( vNormal * pushEps );
	}
	else
	{
		int triIndices[3];
		triIndices[0] = snapV * width + snapU;
		triIndices[1] = nextV * width + snapU;
		triIndices[2] = nextV * width + nextU;

		Vector triVerts[3];
		for( int i = 0; i < 3; i++ )
		{
			GetVert( triIndices[i], triVerts[i] );
		}

		Vector edgeU, edgeV;
		edgeU = triVerts[2] - triVerts[1];
		edgeV = triVerts[0] - triVerts[1];

		// get the position
		surfPt = triVerts[1] + edgeU * fracU + edgeV * ( 1.0f - fracV );

		// get face normal and push point away from surface the given pushEps
		Vector vNormal;
		vNormal = CrossProduct( edgeV, edgeU );
		VectorNormalize( vNormal );
		surfPt += ( vNormal * pushEps );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurfDiagonalTLtoBR( int snapU, int snapV, int nextU, int nextV,
											    float fracU, float fracV, Vector &surfPt, 
												float pushEps )
{
	// get displacement width
	int width = GetWidth();

	if( ( fracU + fracV ) >= ( 1.0f + TRIEDGE_EPSILON ) )
	{
		int triIndices[3];
		triIndices[0] = nextV * width + snapU;
		triIndices[1] = nextV * width + nextU;
		triIndices[2] = snapV * width + nextU;

		Vector triVerts[3];
		for( int i = 0; i < 3; i++ )
		{
			GetVert( triIndices[i], triVerts[i] );
		}

		Vector edgeU, edgeV;
		edgeU = triVerts[0] - triVerts[1];
		edgeV = triVerts[2] - triVerts[1];

		surfPt = triVerts[1] + edgeU * ( 1.0f - fracU ) + edgeV * ( 1.0f - fracV );

		// get face normal and push point away from surface the given pushEps
		Vector vNormal;
		vNormal = CrossProduct( edgeU, edgeV );
		VectorNormalize( vNormal );
		surfPt += ( vNormal * pushEps );
	}
	else
	{
		int triIndices[3];
		triIndices[0] = snapV * width + snapU;
		triIndices[1] = nextV * width + snapU;
		triIndices[2] = snapV * width + nextU;

		Vector triVerts[3];
		for( int i = 0; i < 3; i++ )
		{
			GetVert( triIndices[i], triVerts[i] );
		}

		Vector edgeU, edgeV;
		edgeU = triVerts[2] - triVerts[0];
		edgeV = triVerts[1] - triVerts[0];

		// get the position
		surfPt = triVerts[0] + edgeU * fracU + edgeV * fracV;

		// get face normal and push point away from surface the given pushEps
		Vector vNormal;
		vNormal = CrossProduct( edgeU, edgeV );
		VectorNormalize( vNormal );
		surfPt += ( vNormal * pushEps );
	}
}


//-----------------------------------------------------------------------------
// NOTE: changed the algorithm to use the newer tesselation scheme
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurfPt( Vector2D const &dispUV, Vector &surfPt, float pushEps )
{
	// get the displacement width and height
	int width = GetWidth();
	int height = GetHeight();

	// scale the u, v coordinates the displacement grid size
	float sU = dispUV.x * ( width - 1.000001f );
	float sV = dispUV.y * ( height - 1.000001f );

	//
	// find the triangle the "uv spot" resides in
	//
	int snapU = ( int )sU;
	int snapV = ( int )sV;

	int nextU = snapU + 1;
	int nextV = snapV + 1;
	if (nextU == width) { --nextU; }
	if (nextV == height) { --nextV; }

	float fracU = sU - snapU;
	float fracV = sV - snapV;
	
	//
	// get triangle indices (fan tesselation!)
	//
	//   |    |    |
	//   |    |    |
	//   10---11---12----
	//   |\   |  / |
	//   |  \ |/   |
	//   5----6----7-----
	//   |  / |\   |
	//   |/   |  \ |
	//   0----1----2-----
	//
	//	 a "5x5"
	//
	//
	
	// get the x component and test oddness
	bool bOdd = ( ( ( snapV * width ) + snapU ) % 2 ) == 1;

	if( bOdd )
	{
		DispUVToSurfDiagonalTLtoBR( snapU, snapV, nextU, nextV, fracU, fracV, surfPt, pushEps );
	}
	else
	{
		DispUVToSurfDiagonalBLtoTR( snapU, snapV, nextU, nextV, fracU, fracV, surfPt, pushEps );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurfNormal( Vector2D const &dispUV, Vector &surfNormal )
{
	// get the post spacing
	int width = GetWidth();
	int height = GetHeight();

	// scale the u, v coordinates the displacement grid size
	float sU = dispUV.x * ( width - 1.000001f );
	float sV = dispUV.y * ( height - 1.000001f );

	//
	// find the triangle the "uv spot" resides in
	//
	int snapU = ( int )sU;
	int snapV = ( int )sV;

	int nextU = snapU + 1;
	int nextV = snapV + 1;
	if (nextU == width) { --nextU; }
	if (nextV == height) { --nextV; }

	float fracU = sU - snapU;
	float fracV = sV - snapV;

	//
	// get the four normals "around" the "spot"
	//
	float ndxQuad[4];
	ndxQuad[0] = ( snapV * width ) + snapU;
	ndxQuad[1] = ( nextV * width ) + snapU;
	ndxQuad[2] = ( nextV * width ) + nextU;
	ndxQuad[3] = ( snapV * width ) + nextU;

	//
	// find the blended normal (bi-linear)
	//
	Vector tmpNormals[2], blendedNormals[2], dispNormals[4];
	
	for( int i = 0; i < 4; i++ )
	{
		GetVertNormal( ndxQuad[i], dispNormals[i] );
	}

	tmpNormals[0] = dispNormals[0] * ( 1.0f - fracU );
	tmpNormals[1] = dispNormals[3] * fracU;
	blendedNormals[0] = tmpNormals[0] + tmpNormals[1];
	VectorNormalize( blendedNormals[0] );

	tmpNormals[0] = dispNormals[1] * ( 1.0f - fracU );
	tmpNormals[1] = dispNormals[2] * fracU;
	blendedNormals[1] = tmpNormals[0] + tmpNormals[1];
	VectorNormalize( blendedNormals[1] );

	tmpNormals[0] = blendedNormals[0] * ( 1.0f - fracV );
	tmpNormals[1] = blendedNormals[1] * fracV;

	surfNormal = tmpNormals[0] + tmpNormals[1];
	VectorNormalize( surfNormal );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::ClosestBaseFaceData( Vector const &worldPt, Vector &closePt, 
										 Vector &closeNormal )
{
	Vector delta;
	float minDist = 99999.0f;
	int ndxMin = -1;
	for( int ndxPt = 0; ndxPt < 4; ndxPt++ )
	{
		delta = worldPt - m_Points[ndxPt];
		float dist = delta.Length();
		if( dist < minDist )
		{
			minDist = dist;
			ndxMin = ndxPt;
		}
	}

	int width = GetWidth();
	int height = GetHeight();

	switch( ndxMin )
	{
	case 0: { ndxMin = 0; break; }
	case 1: { ndxMin = ( height - 1 ) * width; break; }
	case 2: { ndxMin = ( height * width ) - 1; break; }
	case 3: { ndxMin = width - 1; break; }
	default: { return; }
	}

	GetVert( ndxMin, closePt );
	GetVertNormal( ndxMin, closeNormal );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::InitPatch( int ndxPatch, int ndxParentPatch, bool bFirst )
{
	// get the current patch
	patch_t *pPatch = &patches[ndxPatch];
	if( pPatch )
	{
		// clear the structure
		memset( pPatch, 0, sizeof( patch_t ) );

		// set children and parent indices
		pPatch->child1 = patches.InvalidIndex();
		pPatch->child2 = patches.InvalidIndex();
		pPatch->parent = ndxParentPatch;

		// set next pointers (add to lists)
		pPatch->ndxNextParent = patches.InvalidIndex();
		pPatch->ndxNextClusterChild = patches.InvalidIndex();

		// set chop info (setting this data -- just in case (not used))
		pPatch->scale[0] = pPatch->scale[1] = 1.0f;
		pPatch->chop = 64;
		
		// set flags (setting this data -- just in case (not used))
		pPatch->sky = false;
		
		// (setting this data -- just in case (not used))
		pPatch->winding = NULL;
		pPatch->plane = NULL;

		// special case for parent surface (accumulate from all)
		pPatch->origin.Init();
		pPatch->normal.Init();
		pPatch->area = 0.0f;

		// get the parent patch -- if it exists (ie this isn't the parent)
		if( ndxParentPatch != patches.InvalidIndex() )
		{
			patch_t *pParentPatch = &patches[ndxParentPatch];

			if( bFirst )
			{
				pParentPatch->child1 = ndxPatch;
			}
			else
			{
				pParentPatch->child2 = ndxPatch;
			}

			// setup the next list
//			pPatch->ndxNext = facePatches.Element( pParentPatch->faceNumber );
//			facePatches[pParentPatch->faceNumber] = ndxPatch;

			// get the face number from the parent
			pPatch->faceNumber = pParentPatch->faceNumber;

			// get the face bounds from the parent
			pPatch->face_mins = pParentPatch->face_mins;
			pPatch->face_maxs = pParentPatch->face_maxs;

			pPatch->reflectivity = pParentPatch->reflectivity;

			pPatch->normalMajorAxis = pParentPatch->normalMajorAxis;
		}
		else
		{
			// set next pointers (add to lists)
			pPatch->ndxNext = facePatches.Element( m_ndxParent );
			facePatches[m_ndxParent] = ndxPatch;

			// used data (for displacement surfaces)
			pPatch->faceNumber = m_ndxParent;

			// set face mins/maxs -- calculated later (this is the face patch)
			pPatch->face_mins.Init( 99999.0f, 99999.0f, 99999.0f );
			pPatch->face_maxs.Init( -99999.0f, -99999.0f, -99999.0f );
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::MakeParentPatch( int ndxPatch )
{
#if 0
	// debugging!
	if( !pDispFile )
	{
		pDispFile = g_pFileSystem->Open( "vraddisp.txt", "w" );
	}
#endif

	// get the current patch
	patch_t *pPatch = &patches[ndxPatch];
	if( pPatch )
	{
		int ndxStart = Nodes_CalcCount( m_Power - 1 );
		int ndxEnd = Nodes_CalcCount( m_Power );

		for( int ndxNode = ndxStart; ndxNode < ndxEnd; ndxNode++ )
		{
			VNode_t *pVNode = &m_pVNodes[ndxNode];
			if( pVNode )
			{
				VectorAdd( pPatch->normal, pVNode->patchNormal, pPatch->normal );
				pPatch->area += pVNode->patchArea;
				
				for( int ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
				{
					if( pPatch->face_mins[ndxAxis] > pVNode->patchBounds[0][ndxAxis] )
					{
						pPatch->face_mins[ndxAxis] = pVNode->patchBounds[0][ndxAxis];
					}
					
					if( pPatch->face_maxs[ndxAxis] < pVNode->patchBounds[1][ndxAxis] )
					{
						pPatch->face_maxs[ndxAxis] = pVNode->patchBounds[1][ndxAxis];
					}
				}
			}
		}
	
#if 0
		// debugging!
		g_pFileSystem->FPrintf( pDispFile, "Parent Patch %d\n", ndxPatch );
		g_pFileSystem->FPrintf( pDispFile, "	Area: %lf\n", pPatch->area );
#endif

		// set the patch bounds to face bounds (as this is the face patch)
		pPatch->mins = pPatch->face_mins;
		pPatch->maxs = pPatch->face_maxs;

		VectorNormalize( pPatch->normal );
		DispUVToSurfPt( Vector2D( 0.5f, 0.5f ), pPatch->origin, 0.0f );		

		// fill in the patch plane into given the normal and origin (have to alloc one - lame!)
		pPatch->plane = new dplane_t;
		if( pPatch->plane )
		{
			pPatch->plane->normal = pPatch->normal;
			pPatch->plane->dist = pPatch->normal.Dot( pPatch->origin );
		}

		// copy the patch origin to face_centroids for main patch
		VectorCopy( pPatch->origin, face_centroids[m_ndxParent] );
		
		VectorAdd( pPatch->origin, pPatch->normal, pPatch->origin );

		// approximate patch winding - used for debugging!
		pPatch->winding = AllocWinding( 4 );
		if( pPatch->winding )
		{
			pPatch->winding->numpoints = 4;
			for( int ndxPt = 0; ndxPt < 4; ndxPt++ )
			{
				GetPoint( ndxPt, pPatch->winding->p[ndxPt] );
			}
		}

		// get base face normal (stab direction is base face normal)
		Vector vBaseFaceNormal;
		GetStabDirection( vBaseFaceNormal );
		int majorAxis = 0;
		float majorValue = vBaseFaceNormal[0];
		if( vBaseFaceNormal[1] > majorValue ) { majorAxis = 1; majorValue = vBaseFaceNormal[1]; }
		if( vBaseFaceNormal[2] > majorValue ) { majorAxis = 2; }
		pPatch->normalMajorAxis = majorAxis;

		// get the base light for the face
		BaseLightForFace( &dfaces[m_ndxParent], pPatch->baselight, &pPatch->basearea, pPatch->reflectivity );

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::GetNodesInPatch( int ndxPatch, int *pVNodes, int &vNodeCount )
{
	// get the current patch
	patch_t *pPatch = &patches.Element( ndxPatch );
	if( !pPatch )
		return;

	//
	// get leaf indices (last level in tree)
	//
	int ndxStart = Nodes_CalcCount( m_Power - 1 );
	int ndxEnd = Nodes_CalcCount( m_Power );

	for( int ndxNode = ndxStart; ndxNode < ndxEnd; ndxNode++ )
	{
		VNode_t *pVNode = &m_pVNodes[ndxNode];
		if( !pVNode )
			continue;

		bool bInside = true;
			
		for( int ndxAxis = 0; ndxAxis < 3 && bInside; ndxAxis++ )
		{
			for( int ndxSide = -1; ndxSide < 2; ndxSide += 2 )
			{
				float dist;
				if( ndxSide == -1 )
				{
					dist = -pVNode->patchOrigin[ndxAxis] + pPatch->mins[ndxAxis];
				}
				else
				{
					dist = pVNode->patchOrigin[ndxAxis] - pPatch->maxs[ndxAxis];
				}

				if( dist > 0.0f )
				{
					bInside = false;
					break;
				}
			}
		}

		if( bInside )
		{
			pVNodes[vNodeCount] = ndxNode;
			vNodeCount++;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::MakeChildPatch( int ndxPatch )
{
	int	vNodeCount = 0;
	int	ndxVNodes[256];

	// find all the nodes that reside behind all of the planes
	GetNodesInPatch( ndxPatch, ndxVNodes, vNodeCount );
	if( vNodeCount <= 0 )
		return false;

	// accumulate data into current patch
	Vector2D uv( 0.0f, 0.0f );
	Vector2D uvBounds[2];
	uvBounds[0].Init( 99999.0f, 99999.0f );
	uvBounds[1].Init( -99999.0f, -99999.0f );

	patch_t *pPatch = &patches.Element( ndxPatch );
	if( pPatch )
	{
		for( int ndxNode = 0; ndxNode < vNodeCount; ndxNode++ )
		{
			VNode_t *pVNode = &m_pVNodes[ndxVNodes[ndxNode]];
			if( pVNode )
			{
				VectorAdd( pPatch->normal, pVNode->patchNormal, pPatch->normal );
				pPatch->area += pVNode->patchArea;
				Vector2DAdd( uv, pVNode->patchOriginUV, uv );

				if( uvBounds[0].x > pVNode->patchOriginUV.x ) { uvBounds[0].x = pVNode->patchOriginUV.x; }
				if( uvBounds[0].y > pVNode->patchOriginUV.y ) { uvBounds[0].y = pVNode->patchOriginUV.y; }

				if( uvBounds[1].x < pVNode->patchOriginUV.x ) { uvBounds[1].x = pVNode->patchOriginUV.x; }
				if( uvBounds[1].y < pVNode->patchOriginUV.y ) { uvBounds[1].y = pVNode->patchOriginUV.y; }
			}
		}

		VectorNormalize( pPatch->normal );

		uv /= vNodeCount;
		DispUVToSurfPt( uv, pPatch->origin, 1.0f );		

		for( int i = 0; i < 2; i++ )
		{
			uvBounds[0][i] -= 0.05f;
			uvBounds[1][i] += 0.05f;
		}

		// approximate patch winding - used for debugging!
		pPatch->winding = AllocWinding( 4 );
		if( pPatch->winding )
		{
			pPatch->winding->numpoints = 4;
			DispUVToSurfPt( uvBounds[0], pPatch->winding->p[0], 0.0f );
			DispUVToSurfPt( Vector2D( uvBounds[0].x, uvBounds[1].y ), pPatch->winding->p[1], 0.0f );
			DispUVToSurfPt( uvBounds[1], pPatch->winding->p[2], 0.0f );
			DispUVToSurfPt( Vector2D( uvBounds[1].x, uvBounds[0].y ), pPatch->winding->p[3], 0.0f );
		}

		// get the parent patch
		patch_t *pParentPatch = &patches.Element( pPatch->parent );
		if( pParentPatch )
		{
			// make sure the area is down by at least a little above half the
			// parent's area we will test at 30% (so we don't spin forever on 
			// weird patch center sampling problems
			float deltaArea = pParentPatch->area - pPatch->area;
			if( deltaArea < ( pParentPatch->area * 0.3 ) )
				return false;
		}

#if 0
		// debugging!
		g_pFileSystem->FPrintf( pDispFile, "Child Patch %d\n", ndxPatch );
		g_pFileSystem->FPrintf( pDispFile, "	Parent %d\n", pPatch->parent );
		g_pFileSystem->FPrintf( pDispFile, "	Area: %lf\n", pPatch->area );
#endif

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::MakePatch( int ndxPatch )
{
	patch_t *pPatch = &patches.Element( ndxPatch );
	if( !pPatch )
		return false;

	// special case: the parent patch accumulates from all the child nodes
	if( pPatch->parent == patches.InvalidIndex() )
	{
		return MakeParentPatch( ndxPatch );
	}
	// or, accumulate the data from the child nodes that reside behind the defined planes
	else
	{
		return MakeChildPatch( ndxPatch );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::GetMajorAxisUV( void )
{
	Vector edgeU = m_Points[3] - m_Points[0];
	Vector edgeV = m_Points[1] - m_Points[0];

	return( edgeU.Length() > edgeV.Length() );
}
