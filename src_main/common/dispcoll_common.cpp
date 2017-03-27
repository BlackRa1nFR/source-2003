//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cmodel.h"
#include "dispcoll_common.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef DIST_EPSILON
#define DIST_EPSILON  0.03125f
#endif

#ifndef NEVER_UPDATED
#define NEVER_UPDATED	-99999
#endif

#define COLLTREE_ROOTNODE	0
#define ONPLANE_EPSILON		0.03125

//-----------------------------------------------------------------------------
// Purpose: displacement collision axial-aligned bounding-box initialization
//-----------------------------------------------------------------------------
void CDispCollTree::AABB_Init( AABB_t &box )
{
	static Vector	s_Normals[DISPCOLL_AABB_SIDE_COUNT] = 
	{
		Vector( -1.0f,  0.0f,  0.0f ),
		Vector(  1.0f,  0.0f,  0.0f ),
		Vector(  0.0f, -1.0f,  0.0f ),
		Vector(  0.0f,  1.0f,  0.0f ),
		Vector(  0.0f,  0.0f, -1.0f ),
		Vector(  0.0f,  0.0f,  1.0f ),
	};

	// create an axial-aligned box
	box.m_Normals = s_Normals;
}

//-----------------------------------------------------------------------------
// Purpose: displacement collision triangle initialization
//-----------------------------------------------------------------------------
void CDispCollTree::Tris_Init( void )
{
	for ( int iTri = 0; iTri < m_nTriCount; ++iTri )
	{
		m_pTris[iTri].m_vecNormal.Init();
		m_pTris[iTri].m_flDist = 0.0f;

		m_pTris[iTri].m_nFlags = 0;

		// Triangle vertex data.
		for ( int iVert = 0; iVert < 3; ++iVert )
		{
			m_pTris[iTri].m_uiVerts[iVert] = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
void CDispCollTree::Node_Init( Node_t &node )
{
	node.m_BBox[0].Init( 99999.0f, 99999.0f, 99999.0f );
	node.m_BBox[1].Init( -99999.0f, -99999.0f, -99999.0f );

	node.m_iTris[0] = -1;
	node.m_iTris[1] = -1;

	node.m_fFlags = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollTree::Node_SetLeaf( Node_t &node, bool bLeaf )
{
	if( bLeaf )
	{
		node.m_fFlags |= 0x1;
	}
	else
	{
		node.m_fFlags &= ~0x1;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline bool CDispCollTree::Node_IsLeaf( Node_t const &node )
{
	return ( node.m_fFlags & 0x1 );
}

//=============================================================================
//
// Collision Tree Functions
//

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CDispCollTree::CDispCollTree()
{
	m_Power = 0;

	m_NodeCount = 0;
	m_pNodes = NULL;

	m_nTriCount = 0;
	m_pTris = NULL;

	m_VertCount = 0;
	m_pVerts = m_pOriginalVerts = NULL;
	m_pVertNormals = NULL;

	for (int i = 0; i < MAX_CHECK_COUNT_DEPTH; ++i)
	{
		m_CheckCount[i] = 0;
	}

	m_BBoxWithFace[0].Init( 99999.0f, 99999.0f, 99999.0f );
	m_BBoxWithFace[1].Init( -99999.0f, -99999.0f, -99999.0f );

	m_StabDir.Init();

	m_Contents = -1;
	m_SurfaceProps[0] = 0;
	m_SurfaceProps[1] = 0;
	m_iLatestSurfProp = 0;
	m_pLeafLinkHead = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: deconstructor
//-----------------------------------------------------------------------------
CDispCollTree::~CDispCollTree()
{
	Nodes_Free();
	FreeVertData();
	Tris_Free();
}

//-----------------------------------------------------------------------------
// Purpose: allocate and initialize the displacement collision tree data
//   Input: pDisp - displacement surface data
//  Output: success? (true/false)
//-----------------------------------------------------------------------------
bool CDispCollTree::Create( CCoreDispInfo *pDisp )
{
	// Displacement size.
	m_Power = pDisp->GetPower();

	// Displacement contents.
	CCoreDispSurface *pSurf = pDisp->GetSurface();
	m_Contents = pSurf->GetContents();

	// Displacement stab direction = base face normal.
	pSurf->GetNormal( m_StabDir );

	// Copy the base surface points.
	for ( int iPt = 0; iPt < 4; iPt++ )
	{
		pSurf->GetPoint( iPt, m_SurfPoints[iPt] );
	}

	// Allocate collision tree data.
	int nResultNode = Nodes_Alloc();
	int nResultVert = AllocVertData();
	int nResultTri = Tris_Alloc();
	if( !nResultNode || !nResultVert || !nResultTri )
	{
		Nodes_Free();
		FreeVertData();
		Tris_Free();
		return false;
	}

	// Copy the vertices and vertex normals.
	for ( int iVert = 0; iVert < m_VertCount; iVert++ )
	{
		pDisp->GetVert( iVert, m_pVerts[iVert] );
		m_pOriginalVerts[iVert] = m_pVerts[iVert];
		pDisp->GetNormal( iVert, m_pVertNormals[iVert] );
	}

	// Copy the triangle flags data.
	for ( int iTri = 0; iTri < m_nTriCount; ++iTri )
	{
		pDisp->GetTriIndices( iTri, m_pTris[iTri].m_uiVerts[0], m_pTris[iTri].m_uiVerts[1], m_pTris[iTri].m_uiVerts[2] );
		m_pTris[iTri].m_nFlags = pDisp->GetTriTagValue( iTri );

		// Calculate the surface props.
		float flTotalAlpha = 0.0f;
		for ( int iVert = 0; iVert < 3; ++iVert )
		{
			flTotalAlpha += pDisp->GetAlpha( m_pTris[iTri].m_uiVerts[iVert] );
		}
		m_pTris[iTri].m_iSurfProp = 0;
		if ( flTotalAlpha > DISP_ALPHA_PROP_DELTA )
		{
			m_pTris[iTri].m_iSurfProp = 1;
		}

		// Add the displacement surface flag!
		m_pTris[iTri].m_nFlags |= DISPSURF_FLAG_SURFACE;

		Tri_CalcPlane( iTri );
	}

	// create leaf nodes
	Leafs_Create(); 

	// create tree nodes
	Nodes_Create();

	// create the bounding box of the displacement surface + the base face
	CalcFullBBox();

	// tree successfully created!
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::Leafs_Create()
{
	// Find the bottom leftmost node.
	int iMinNode = 0;
	for ( int iPower = 0; iPower < m_Power; ++iPower )
	{
		iMinNode = Nodes_GetChild( iMinNode, 0 );
	}

	int nWidth = GetWidth() - 1;
	int nHeight = nWidth;

	for ( int iHgt = 0; iHgt < nHeight; ++iHgt )
	{
		for ( int iWid = 0; iWid < nWidth; ++iWid )
		{
			int iNode = Nodes_GetIndexFromComponents( iWid, iHgt );
			iNode += iMinNode;
			Node_t *pNode = &m_pNodes[iNode];
			if ( !pNode )
				continue;

			Node_SetLeaf( *pNode, true );

			int nIndex = iHgt * nWidth + iWid;
			int iTri = nIndex * 2;

			pNode->m_iTris[0] = iTri;
			pNode->m_iTris[1] = iTri + 1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::Nodes_Create()
{
	//
	// create all nodes in tree
	//
	int power = GetPower() + 1;
    for( int level = power; level > 0; level-- )
    {
		Nodes_CreateRecur( 0 /* rootIndex */, level );
    }
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::Nodes_CreateRecur( int ndxNode, int termLevel )
{
	int nodeLevel = Nodes_GetLevel( ndxNode );
	
	//
	// terminating condition -- set node info (leaf or otherwise)
	//
	Node_t *pNode = &m_pNodes[ndxNode];
	if( nodeLevel == termLevel )
	{
		Nodes_CalcBounds( pNode, ndxNode );
		return;
	}

	//
	// recurse into children
	//
	pNode->m_BBox[0].Init( 1e24,  1e24,  1e24);
	pNode->m_BBox[1].Init(-1e24, -1e24, -1e24);
	
	for( int ndxChild = 0; ndxChild < 4; ndxChild++ )
	{
		int iChildNode = Nodes_GetChild( ndxNode, ndxChild );
		Nodes_CreateRecur( iChildNode, termLevel );
		
		Node_t *pChildNode = &m_pNodes[iChildNode];
		VectorMin( pChildNode->m_BBox[0], pNode->m_BBox[0], pNode->m_BBox[0] );
		VectorMax( pChildNode->m_BBox[1], pNode->m_BBox[1], pNode->m_BBox[1] );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::Nodes_CalcBounds( Node_t *pNode, int ndxNode )
{
	//
	// leaf nodes have special cases (caps, etc.)
	//
	if( Node_IsLeaf( *pNode ) )
	{
		Nodes_CalcLeafBounds( pNode, ndxNode );
		return;
	}

	//
	// get the maximum and minimum bounds of all leaf nodes -- that is the
	// bounding box for this node
	//
	pNode->m_BBox[0].Init( 99999.0f, 99999.0f, 99999.0f );
	pNode->m_BBox[1].Init( -99999.0f, -99999.0f, -99999.0f );
	
	for( int ndxChild = 0; ndxChild < 4; ndxChild++ )
	{
		//
		// get the current child node
		//
		int ndxChildNode = Nodes_GetChild( ndxNode, ndxChild );
		Node_t *pChildNode = &m_pNodes[ndxChildNode];

		// update the bounds
		VectorMin( pChildNode->m_BBox[0], pNode->m_BBox[0], pNode->m_BBox[0] );
		VectorMax( pChildNode->m_BBox[1], pNode->m_BBox[1], pNode->m_BBox[1] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::Nodes_CalcLeafBounds( Node_t *pNode, int ndxNode )
{
	// Find the minimum and maximum component values for all triangles in 
	// the leaf node (including caps tris)
	for ( int iTri = 0; iTri < 2; iTri++ )
	{
		for ( int iVert = 0; iVert < 3; iVert++ )
		{
			// Minimum checks
			Vector vecTmp;
			Tri_t *pTri = &m_pTris[pNode->m_iTris[iTri]];
			vecTmp = m_pVerts[pTri->m_uiVerts[iVert]];

			VectorMin( vecTmp, pNode->m_BBox[0], pNode->m_BBox[0] );
			VectorMax( vecTmp, pNode->m_BBox[1], pNode->m_BBox[1] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::CalcFullBBox()
{
	//
	// initialize the full bounding box with the displacement bounding box
	//
	m_BBoxWithFace[0] = m_pNodes[0].m_BBox[0];
	m_BBoxWithFace[1] = m_pNodes[0].m_BBox[1];

	//
	// expand to contain the base face if necessary
	//
	for( int ndxPt = 0; ndxPt < 4; ndxPt++ )
	{
		const Vector &v = m_SurfPoints[ndxPt];
		
		VectorMin( v, m_BBoxWithFace[0], m_BBoxWithFace[0] );
		VectorMax( v, m_BBoxWithFace[1], m_BBoxWithFace[1] );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::Tri_CalcPlane( short iTri )
{
	Tri_t *pTri = &m_pTris[iTri];
	if ( !pTri )
		return;

	Vector vecTmp[3];
	Vector vecEdge1, vecEdge2;
	vecTmp[0] = m_pVerts[pTri->m_uiVerts[0]];
	vecTmp[1] = m_pVerts[pTri->m_uiVerts[1]];
	vecTmp[2] = m_pVerts[pTri->m_uiVerts[2]];
	vecEdge1 = vecTmp[1] - vecTmp[0];
	vecEdge2 = vecTmp[2] - vecTmp[0];

	pTri->m_vecNormal = vecEdge2.Cross( vecEdge1 );
	VectorNormalize( pTri->m_vecNormal );
	pTri->m_flDist = pTri->m_vecNormal.Dot( vecTmp[0] );
}

//-----------------------------------------------------------------------------
// Purpose: get the parent node index given the current node
//   Input: nodeIndex - current node index
//  Output: int - the index of the parent node
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetParent( int ndxNode )
{
	// node range [0...m_NodeCount)
	assert( ndxNode >= 0 );
	assert( ndxNode < m_NodeCount );

	// ( node index - 1 ) / 4
	return ( ( ndxNode - 1 ) >> 2 );
}


//-----------------------------------------------------------------------------
// Purpose: get the child node index given the current node index and direction
//          of the child (1 of 4)
//   Input: nodeIndex - current node index
//          direction - direction of the child ( [0...3] - SW, SE, NW, NE )
//  Output: int - the index of the child node
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetChild( int ndxNode, int direction )
{
	// node range [0...m_NodeCount)
	assert( ndxNode >= 0 );
	assert( ndxNode < m_NodeCount );

    // ( node index * 4 ) + ( direction + 1 )
    return ( ( ndxNode << 2 ) + ( direction + 1 ) );	
}


//-----------------------------------------------------------------------------
// Purpose:
// TODO: should make this a function - not a hardcoded set of statements!!!
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetLevel( int ndxNode )
{
	// node range [0...m_NodeCount)
	assert( ndxNode >= 0 );
	assert( ndxNode < m_NodeCount );

	// level = 2^n + 1
	if( ndxNode == 0 )  { return 1; }
	if( ndxNode < 5 )   { return 2; }
	if( ndxNode < 21 )  { return 3; }
	if( ndxNode < 85 )  { return 4; }
	if( ndxNode < 341 ) { return 5; }

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetIndexFromComponents( int x, int y )
{
	int index = 0;

	//
	// interleave bits from the x and y values to create the index
	//
	int shift;
	for( shift = 0; x != 0; shift += 2, x >>= 1 )
	{
		index |= ( x & 1 ) << shift;
	}

	for( shift = 1; y != 0; shift += 2, y >>= 1 )
	{
		index |= ( y & 1 ) << shift;
	}

	return index;
}


//-----------------------------------------------------------------------------
// Purpose: allocate memory for the displacement collision tree nodes
//  Output: sucess? (true/false)
//-----------------------------------------------------------------------------
int CDispCollTree::Nodes_Alloc( void )
{
	m_NodeCount = ( short )Nodes_CalcCount( m_Power );
	if( m_NodeCount == 0 )
		return false;

	m_pNodes = new Node_t[m_NodeCount];
	if( !m_pNodes )
	{
		m_NodeCount = 0;
		return false;
	}

	//
	// initialize the nodes
	//
	for( int i = 0;i < m_NodeCount; i++ )
	{
		Node_Init( m_pNodes[i] );
	}

	// tree successfully allocated!
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: de-allocate memory for the displacement collision tree nodes
//-----------------------------------------------------------------------------
void CDispCollTree::Nodes_Free( void )
{
	if( m_pNodes )
	{
		delete [] m_pNodes;
		m_pNodes = NULL;
	}
	m_NodeCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::Tris_Alloc( void )
{
	// Calculate the number of triangles.
	m_nTriCount = ( (1 << (m_Power)) * (1 << (m_Power)) * 2 );

	// Allocate triangle memory.
	m_pTris = new Tri_t[m_nTriCount];
	if ( !m_pTris )
	{
		m_nTriCount = 0;
		return false;
	}

	// Initialize the triangles.
	Tris_Init();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::Tris_Free( void )
{
	if ( m_pTris )
	{
		delete [] m_pTris;
		m_pTris = NULL;
	}

	m_nTriCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CDispCollTree::AllocVertData( void )
{
	m_VertCount = ( short )( ( ( 1 << m_Power ) + 1 ) * ( ( 1 << m_Power ) + 1 ) );
	if( m_VertCount == 0 )
		return false;

	m_pVerts = new Vector[m_VertCount];
	m_pOriginalVerts = new Vector[m_VertCount];
	if( !m_pVerts || !m_pOriginalVerts )
	{
		m_VertCount = 0;
		return false;
	}

	m_pVertNormals = new Vector[m_VertCount];
	if( !m_pVertNormals )
	{
		m_VertCount = 0;
		delete [] m_pVerts;
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::FreeVertData( void )
{
	if( m_pVerts )
	{
		delete [] m_pVerts;
		m_pVerts = NULL;
	}

	if( m_pOriginalVerts )
	{
		delete [] m_pOriginalVerts;
		m_pOriginalVerts = NULL;
	}

	if( m_pVertNormals )
	{
		delete [] m_pVertNormals;
		m_pVertNormals = NULL;
	}

	m_VertCount = 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CDispCollTree::VectorsEqualWithTolerance( Vector &v1, Vector &v2, float tolerance )
{
	for( int i = 0 ; i < 3 ; i++ )
	{
		if( FloatMakePositive( v1[i] - v2[i] ) > tolerance )
			return false;
	}
	
	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CDispCollTree::GetEdgePointIndex( CCoreDispInfo *pDisp, int edgeIndex, 
									  int edgePtIndex, bool bClockWise )
{
	int height = pDisp->GetHeight();
	int width = pDisp->GetWidth();

	if( bClockWise )
	{
		switch( edgeIndex )
		{
		case 0: { return ( edgePtIndex * height ); }
		case 1: { return ( ( ( height - 1 ) * width ) + edgePtIndex ); }
		case 2: { return ( ( height * width - 1 ) - ( edgePtIndex * height ) ); }
		case 3: { return ( ( width - 1 ) - edgePtIndex ); }
		default: { return -1; }
		}
	}
	else
	{
		switch( edgeIndex )
		{
		case 0: { return ( ( ( height - 1 ) * width ) - ( edgePtIndex * height ) ); }
		case 1: { return ( ( height * width - 1 ) - edgePtIndex ); }
		case 2: { return ( ( width - 1 ) + ( edgePtIndex * height ) ); }
		case 3: { return ( edgePtIndex ); }
		default: { return -1; }
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::PointInBounds( const Vector &pos, const Vector &boxMin, const Vector &boxMax,
								   bool bIsPoint )
{
	//
	// point test inside bounds
	//
	if( bIsPoint )
	{
		return PointInBox( pos, m_BBoxWithFace[0], m_BBoxWithFace[1] );
	}
	
	//
	// box test inside bounds
	//
	Vector extents;
	extents = ( ( boxMax - boxMin ) * 0.5f ) - boxMin;

	Vector expandBoxMin, expandBoxMax;
	expandBoxMin = m_BBoxWithFace[0] - extents;
	expandBoxMax = m_BBoxWithFace[1] + extents;

	return PointInBox( pos, expandBoxMin, expandBoxMax );
}


void CDispCollTree::ApplyTerrainMod( ITerrainMod *pMod )
{
	int size = GetSize();
	for( int i=0; i < size; i++ )
	{
		Vector &vPos = m_pVerts[i];
		pMod->ApplyMod( vPos, m_pOriginalVerts[i] );
	}

	// create leaf nodes
	Leafs_Create(); 

	// create tree nodes
	Nodes_Create();

	CalcFullBBox();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline bool CDispCollTree::PointInBox( const Vector &pos, const Vector &boxMin, Vector &boxMax )
{
	if( ( pos.x < boxMin.x ) || ( pos.x > boxMax.x ) ) { return false; }
	if( ( pos.y < boxMin.y ) || ( pos.y > boxMax.y ) ) { return false; }
	if( ( pos.z < boxMin.z ) || ( pos.z > boxMax.z ) ) { return false; }
	
	return true;
}


//=============================================================================
//
// New Ray Traces!!!!!!
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::RayTest( Ray_t const &ray, bool bOneSided )
{
	// Check for opacity?!
	if ( !( m_Contents & MASK_OPAQUE ) )
		return false;

	// create and initialize the triangle list
	TriList_t triList;
	triList.m_Count = 0;

	// create and initialize the primary AABB
	AABB_t box;
	AABB_Init( box );

	//
	// collide against the bboxed quad-tree and generate a initial list of
	// collision tris
	//
	Ray_BuildTriList( ray, COLLTREE_ROOTNODE, box, triList );
	for ( int iTri = 0; iTri < triList.m_Count; iTri++ )
	{
		Tri_t *pTri = triList.m_ppTriList[iTri];
		float frac = IntersectRayWithTriangle( ray, m_pVerts[pTri->m_uiVerts[0]], m_pVerts[pTri->m_uiVerts[1]],
			                                   m_pVerts[pTri->m_uiVerts[2]], bOneSided );

		// collision
		if( ( frac > 0.0f ) && ( frac < 1.0f ) )
			return true;
	}

	// no collision
	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::RayTest( Ray_t const &ray, RayDispOutput_t &output )
{
	// Check for opacity?!
	if ( !( m_Contents & MASK_OPAQUE ) )
		return false;

	// create and initialize the triangle list
	TriList_t triList;
	triList.m_Count = 0;

	// create and initialize the primary AABB
	AABB_t box;
	AABB_Init( box );

	//
	// collide against the bboxed quad-tree and generate a initial list of
	// collision tris
	//
	float minT = 1.0f;
	int minNdx = -1;
	float minU = 0.0f, minV = 0.0f;

	Ray_BuildTriList( ray, COLLTREE_ROOTNODE, box, triList );
	for ( int iTri = 0; iTri < triList.m_Count; iTri++ )
	{
		float u, v, t;
		Tri_t *pTri = triList.m_ppTriList[iTri];
		if( ComputeIntersectionBarycentricCoordinates( ray,
													   m_pVerts[pTri->m_uiVerts[0]],
													   m_pVerts[pTri->m_uiVerts[2]],
												       m_pVerts[pTri->m_uiVerts[1]],
													   u, v, &t ) )
		{
			// Make sure it's inside the range
			if ((u < 0.0f) || (v < 0.0f) || (u+v > 1.0f))
				continue;

			if( ( t > 0.0f ) && ( t < minT ) )
			{
				minT = t;
				minNdx = iTri;
				minU = u;
				minV = v;
			}
		}
	}

	if( minNdx != -1 )
	{
		Tri_t *pTri = triList.m_ppTriList[minNdx];
		output.ndxVerts[0] = pTri->m_uiVerts[0];
		output.ndxVerts[1] = pTri->m_uiVerts[2];
		output.ndxVerts[2] = pTri->m_uiVerts[1];
		output.u = minU;
		output.v = minV;
		output.dist = minT;

		assert( (output.u <= 1.0f) && (output.v <= 1.0f ));
		assert( (output.u >= 0.0f) && (output.v >= 0.0f ));

		return true;
	}

	// no collision
	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::Ray_BuildTriList( Ray_t const &ray, int ndxNode, AABB_t &box, TriList_t &triList )
{
	// get the current node
	Node_t &node = m_pNodes[ndxNode];

	//
	// create node bounding box planes (create the AABB)
	// NOTE: only need the six distances, plane vectors are always the same!
	//
	box.m_Dists[0] = -( node.m_BBox[0].x - DIST_EPSILON );
	box.m_Dists[1] = ( node.m_BBox[1].x + DIST_EPSILON );
	box.m_Dists[2] = -( node.m_BBox[0].y - DIST_EPSILON );
	box.m_Dists[3] = ( node.m_BBox[1].y + DIST_EPSILON );
	box.m_Dists[4] = -( node.m_BBox[0].z - DIST_EPSILON );
	box.m_Dists[5] = ( node.m_BBox[1].z + DIST_EPSILON );

	// test the ray against the given node
	if( Ray_NodeTest( ray, box ) )
	{
		//
		// if leaf add tris to list
		//
		if( Node_IsLeaf( node ) )
		{
			// debugging!!!
			assert( triList.m_Count >= 0 );
			assert( triList.m_Count < DISPCOLL_TRILIST_SIZE );

			if( triList.m_Count < DISPCOLL_TRILIST_SIZE )
			{
				triList.m_ppTriList[triList.m_Count] = &m_pTris[node.m_iTris[0]];
				triList.m_ppTriList[triList.m_Count+1] = &m_pTris[node.m_iTris[1]];
				triList.m_Count += 2;
			}

			return;
		}
		// continue testing with children nodes
		else
		{
			Ray_BuildTriList( ray, Nodes_GetChild( ndxNode, 0 ), box, triList );
			Ray_BuildTriList( ray, Nodes_GetChild( ndxNode, 1 ), box, triList );
			Ray_BuildTriList( ray, Nodes_GetChild( ndxNode, 2 ), box, triList );
			Ray_BuildTriList( ray, Nodes_GetChild( ndxNode, 3 ), box, triList );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::Ray_NodeTest( Ray_t const &ray, AABB_t const &box )
{
	// initialize enter and exit fractions
	float enterFrac = 0.0f;
	float exitFrac = 1.0f;

	float distStart, distEnd;
	float frac;

	for( int axis = 0; axis < 3; axis++ )
	{
		// negative
		distStart = -ray.m_Start[axis] - box.m_Dists[(axis<<1)];
		distEnd = -(ray.m_Start+ray.m_Delta)[axis] - box.m_Dists[(axis<<1)];
	
		if( ( distStart > 0.0f ) && ( distEnd < 0.0f ) ) 
		{ 
			frac = ( distStart - DIST_EPSILON ) / ( distStart - distEnd );
			if( frac > enterFrac ) { enterFrac = frac; }
		}

		if( ( distStart < 0.0f ) && ( distEnd > 0.0f ) ) 
		{ 
			frac = ( distStart + DIST_EPSILON ) / ( distStart - distEnd );
			if( frac < exitFrac ) { exitFrac = frac; }
		}

		if( ( distStart > 0.0f ) && ( distEnd > 0.0f ) )
			return false;

		// positive
		distStart = ray.m_Start[axis] - box.m_Dists[(axis<<1)+1];
		distEnd = (ray.m_Start+ray.m_Delta)[axis] - box.m_Dists[(axis<<1)+1];

		if( ( distStart > 0.0f ) && ( distEnd < 0.0f ) ) 
		{ 
			frac = ( distStart - DIST_EPSILON ) / ( distStart - distEnd );
			if( frac > enterFrac ) { enterFrac = frac; }
		}

		if( ( distStart < 0.0f ) && ( distEnd > 0.0f ) ) 
		{ 
			frac = ( distStart + DIST_EPSILON ) / ( distStart - distEnd );
			if( frac < exitFrac ) { exitFrac = frac; }
		}

		if( ( distStart > 0.0f ) && ( distEnd > 0.0f ) )
			return false;
	}

	if( exitFrac < enterFrac )
		return false;

	return true;
}



//=============================================================================
//
// Old Ray Traces!!!!!! Still here until all the old code has been converted!!!
//


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::RayTest( const Vector &rayStart, const Vector &rayEnd )
{
	// Check for opacity?!
	if ( !( m_Contents & MASK_OPAQUE ) )
		return false;

	//
	// create and initialize the triangle list
	//
	TriList_t triList;
	triList.m_Count = 0;

	//
	// create and initialize the primary AABB
	//
	AABB_t AABBox;
	AABB_Init( AABBox );

	//
	// collide against the bboxed quad-tree and generate a initial list of
	// collision tris
	//
	Ray_BuildTriList( rayStart, rayEnd, 0 /*root node*/, AABBox, triList );

	if( triList.m_Count != 0 )
	{
		return Ray_IntersectTriListTest( rayStart, rayEnd, triList );
	}

	// no collision
	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::RayTest( const Vector &rayStart, const Vector &rayEnd, 
							 float startFrac, float endFrac, CBaseTrace *pTrace )
{
	// Check for opacity?!
	if ( !( m_Contents & MASK_OPAQUE ) )
		return false;

	// Create and initialize the triangle list
	TriList_t triList;
	triList.m_Count = 0;

	// Create and initialize the primary AABB
	AABB_t AABBox;
	AABB_Init( AABBox );

	bool bHasTrace = true;
	if( !pTrace )
	{
		pTrace = new CBaseTrace;
		pTrace->fraction = 1.0f;
		bHasTrace = false;
	}

	// Clip the ray to the leaf node for testing.
	Vector clipRayStart, clipRayEnd, delta;
	VectorSubtract( rayEnd, rayStart, delta );
	VectorMA( rayStart, startFrac, delta, clipRayStart );
	VectorMA( rayStart, endFrac, delta, clipRayEnd );

	// Collide against the bboxed quad-tree and generate a initial list of collision tris.
	Ray_BuildTriList( clipRayStart, clipRayEnd, 0 /*root node*/, AABBox, triList );

	// Save the starting fraction
	float preIntersectFrac = pTrace->fraction;

	if( triList.m_Count != 0 )
	{
		Ray_IntersectTriList( rayStart, rayEnd, startFrac, endFrac, pTrace, triList );
	}

	// Collision
	if( preIntersectFrac > pTrace->fraction )
	{
		if( !bHasTrace )
		{
			delete pTrace;
		}
		return true;
	}

	// delete trace!!
	if( !bHasTrace )
	{
		delete pTrace;
	}

	// no collision
	return false; 
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::Ray_BuildTriList( const Vector &rayStart, const Vector &rayEnd, int ndxNode, 
					                  AABB_t &AABBox, TriList_t &triList )
{
	// get the current node
	Node_t *pNode = &m_pNodes[ndxNode];

	//
	// create node bounding box planes (create the AABB)
	// NOTE: only need the six distances, plane vectors are always the same!
	//
	AABBox.m_Dists[0] = -( pNode->m_BBox[0].x - DIST_EPSILON );
	AABBox.m_Dists[1] = ( pNode->m_BBox[1].x + DIST_EPSILON );
	AABBox.m_Dists[2] = -( pNode->m_BBox[0].y - DIST_EPSILON );
	AABBox.m_Dists[3] = ( pNode->m_BBox[1].y + DIST_EPSILON );
	AABBox.m_Dists[4] = -( pNode->m_BBox[0].z - DIST_EPSILON );
	AABBox.m_Dists[5] = ( pNode->m_BBox[1].z + DIST_EPSILON );

	// test the ray against the given node
	if( Ray_NodeTest( rayStart, rayEnd, AABBox ) )
	{
		//
		// if leaf add tris to list
		//
		if( Node_IsLeaf( *pNode ) )
		{
			// debugging!!!
			assert( triList.m_Count >= 0 );
			assert( triList.m_Count < DISPCOLL_TRILIST_SIZE );

			if( triList.m_Count < DISPCOLL_TRILIST_SIZE )
			{
				triList.m_ppTriList[triList.m_Count] = &m_pTris[pNode->m_iTris[0]];
				triList.m_ppTriList[triList.m_Count+1] = &m_pTris[pNode->m_iTris[1]];
				triList.m_Count += 2;
			}

			return;
		}
		// continue testing with children nodes
		else
		{
			Ray_BuildTriList( rayStart, rayEnd, Nodes_GetChild( ndxNode, 0 ), AABBox, triList );
			Ray_BuildTriList( rayStart, rayEnd, Nodes_GetChild( ndxNode, 1 ), AABBox, triList );
			Ray_BuildTriList( rayStart, rayEnd, Nodes_GetChild( ndxNode, 2 ), AABBox, triList );
			Ray_BuildTriList( rayStart, rayEnd, Nodes_GetChild( ndxNode, 3 ), AABBox, triList );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool FASTCALL CDispCollTree::Ray_NodeTest( const Vector &rayStart, const Vector &rayEnd, AABB_t const &AABBox )
{
	//
	// de-normalize the paramter space so that we don't have to divide to the the
	// fractional amount later
	// NOTE: deltas are clamped for precision
	//
#if 0
	Vector delta, scalar;
	delta = rayEnd - rayStart;
	if( ( delta.x < DIST_EPSILON ) && ( delta.x > -DIST_EPSILON ) ) { delta.x = 1.0f; }
	if( ( delta.y < DIST_EPSILON ) && ( delta.y > -DIST_EPSILON ) ) { delta.y = 1.0f; }
	if( ( delta.z < DIST_EPSILON ) && ( delta.z > -DIST_EPSILON ) ) { delta.z = 1.0f; }
	scalar.x = delta.y * delta.z;
	scalar.y = delta.x * delta.z;
	scalar.z = delta.x * delta.y;
#endif

	//
	// create and initialize the enter and exit fractions
	//
	float enterFraction = 0.0f;
	float exitFraction = 1.0f/*FLT_MAX*/;

	//
	// test the ray against the AABB (reduced to 1d tests)
	//

	for( int ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		float fraction;

		//
		// test negative axial direction
		//
		float distStart = -rayStart[ndxAxis] - AABBox.m_Dists[(ndxAxis<<1)];
		float distEnd = -rayEnd[ndxAxis] - AABBox.m_Dists[(ndxAxis<<1)];

		if( distStart > 0.0f )
		{
			if( distEnd < 0.0f )
			{
				fraction = ( distStart - DIST_EPSILON ) / ( distStart - distEnd );
	//			fraction = ( distStart - DIST_EPSILON ) * scalar[ndxAxis];
				if( fraction > enterFraction )
				{
					enterFraction = fraction;
				}
			} 
			else if( distEnd > 0.0f )
			{
				return false;
			}

		}
		else if( ( distStart < 0.0f ) && ( distEnd > 0.0f ) )
		{
			fraction = ( distStart + DIST_EPSILON ) / ( distStart - distEnd );
//			fraction = -( distStart + DIST_EPSILON ) * scalar[ndxAxis];
			if( fraction < exitFraction )
			{
				exitFraction = fraction;
			}
		}
		//
		// test positive axial direction
		//
		distStart = rayStart[ndxAxis] - AABBox.m_Dists[(ndxAxis<<1)+1];
		distEnd = rayEnd[ndxAxis] - AABBox.m_Dists[(ndxAxis<<1)+1];

		if( ( distStart > 0.0f ) ) 
		{
			if( distEnd < 0.0f )
			{
				fraction = ( distStart - DIST_EPSILON ) / ( distStart - distEnd );
	//			fraction = ( distStart - DIST_EPSILON ) * scalar[ndxAxis];
				if( fraction > enterFraction )
				{
					enterFraction = fraction;
				}
			} 
			else if( distEnd > 0.0f )
			{
				return false;
			}
		}
		else if( ( distStart < 0.0f ) && ( distEnd > 0.0f ) )
		{
			fraction = ( distStart + DIST_EPSILON ) / ( distStart - distEnd );
//			fraction = -( distStart + DIST_EPSILON ) * scalar[ndxAxis];
			if( fraction < exitFraction )
			{
				exitFraction = fraction;
			}
		}
	}

	// test results
	return !(exitFraction < enterFraction);
}


//-----------------------------------------------------------------------------
// Purpose: just need to know if it does intersect - not where!!
//-----------------------------------------------------------------------------
bool CDispCollTree::Ray_IntersectTriListTest( const Vector &rayStart, const Vector &rayEnd,
										      TriList_t const &triList )
{
	// initialize the ray structure
	Ray_t ray;
	ray.m_Start = rayStart;
	ray.m_Delta = rayEnd - rayStart;
	ray.m_Extents.Init();

	for( int iTri = 0; iTri < triList.m_Count; iTri++ )
	{
		// get the current displacement collision triangle
		Tri_t *pTri = triList.m_ppTriList[iTri];
		float fraction = IntersectRayWithTriangle( ray,
			                                       m_pVerts[pTri->m_uiVerts[0]],
						 						   m_pVerts[pTri->m_uiVerts[2]],
												   m_pVerts[pTri->m_uiVerts[1]],
												   true );
		
		// a negative value means no collision
		if( fraction < 0.0f )
			continue;

		// a negative fraction means no collision
		if( fraction != 1.0f )
			return true;
	}

	// no collision
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::Ray_IntersectTriList( const Vector &rayStart, const Vector &rayEnd,
										  float startFrac, float endFrac, CBaseTrace *pTrace, 
										  TriList_t const &triList )
{
	// initialize the ray structure
	Ray_t ray;
	ray.m_Start = rayStart;
	ray.m_Delta = rayEnd - rayStart;
	ray.m_Extents.Init();

	for( int iTri = 0; iTri < triList.m_Count; iTri++ )
	{
		// get the current displacement collision triangle
		Tri_t *pTri = triList.m_ppTriList[iTri];

		float intFrac = IntersectRayWithTriangle( ray,
			                                      m_pVerts[pTri->m_uiVerts[0]],
												  m_pVerts[pTri->m_uiVerts[2]],
												  m_pVerts[pTri->m_uiVerts[1]],
												  true );
		
		// a negative fraction means no collision
		if( intFrac < 0.0f )
			continue;

		// fraction of the fractional ray
		float fraction = /*startFrac + ( endFrac - startFrac ) * */ intFrac;
		
		if( fraction < pTrace->fraction )
		{
			pTrace->fraction = fraction;
			pTrace->plane.normal = pTri->m_vecNormal;
			pTrace->plane.dist = pTri->m_flDist;
			pTrace->dispFlags = pTri->m_nFlags;

			m_iLatestSurfProp = pTri->m_iSurfProp;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBIntersect( const Vector &boxCenter, const Vector &boxMin, const Vector &boxMax )
{
	//
	// create and initialize the triangle list
	//
	TriList_t triList;
	triList.m_Count = 0;

	//
	// calc box extents
	//
	Vector boxExtents;
	boxExtents = ( ( boxMax + boxMin ) * 0.5f ) - boxMin;

	//
	// collide against the bboxed quad-tree and generate a initial list of
	// collision tris
	//
	AABB_BuildTriList( boxCenter, boxExtents, 0 /*root node*/, triList );

	if( triList.m_Count != 0 )
	{
		// the the axial-aligned box against all tris in list 
		if( AABB_IntersectTriList( boxCenter, boxExtents, triList ) )
			return true;
	}

	// no collision
	return false; 
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::AABB_BuildTriList( const Vector &boxCenter, const Vector &boxExtents,
									   int ndxNode, TriList_t &triList )
{
	// get the current node
	Node_t *pNode = &m_pNodes[ndxNode];

	// test the box against the given node
	if( AABB_NodeTest( boxCenter, boxExtents, pNode ) )
	{
		//
		// if leaf add tris to list
		//
		if( Node_IsLeaf( *pNode ) )
		{
			// debugging!!!
			assert( triList.m_Count >= 0 );
			assert( triList.m_Count < DISPCOLL_TRILIST_SIZE );

			if( triList.m_Count < DISPCOLL_TRILIST_SIZE )
			{
				triList.m_ppTriList[triList.m_Count] = &m_pTris[pNode->m_iTris[0]];
				triList.m_ppTriList[triList.m_Count+1] = &m_pTris[pNode->m_iTris[1]];
				triList.m_Count += 2;
			}

			return;
		}
		// continue testing with children nodes
		else
		{
			AABB_BuildTriList( boxCenter, boxExtents, Nodes_GetChild( ndxNode, 0 ), triList );
			AABB_BuildTriList( boxCenter, boxExtents, Nodes_GetChild( ndxNode, 1 ), triList );
			AABB_BuildTriList( boxCenter, boxExtents, Nodes_GetChild( ndxNode, 2 ), triList );
			AABB_BuildTriList( boxCenter, boxExtents, Nodes_GetChild( ndxNode, 3 ), triList );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::AABB_NodeTest( const Vector &boxCenter, const Vector &boxExtents, 
								   Node_t *pNode )
{
	//
	// box test inside bounds
	//
	Vector expandBoxMin, expandBoxMax;
	expandBoxMin = pNode->m_BBox[0] - boxExtents;
	expandBoxMax = pNode->m_BBox[1] + boxExtents;

	// test to see if box is outside
	if( ( boxCenter.x < expandBoxMin.x ) || ( boxCenter.x > expandBoxMax.x ) ) { return false; }
	if( ( boxCenter.y < expandBoxMin.y ) || ( boxCenter.y > expandBoxMax.y ) ) { return false; }
	if( ( boxCenter.z < expandBoxMin.z ) || ( boxCenter.z > expandBoxMax.z ) ) { return false; }
	
	// inside
	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::AABB_IntersectTriList( const Vector &boxCenter, const Vector &boxExtents,
										   TriList_t const &triList )
{
	//
	// test each tri in list against the given box
	//
	for( int iTri = 0; iTri < triList.m_Count; iTri++ )
	{
		// get the current displacement collision triangle
		Tri_t *pTri = triList.m_ppTriList[iTri];

		if( SeparatingAxisAABoxTriangle( boxCenter, boxExtents,
			                             m_pVerts[pTri->m_uiVerts[0]], 
									     m_pVerts[pTri->m_uiVerts[2]],
									     m_pVerts[pTri->m_uiVerts[1]],
									     pTri->m_vecNormal, pTri->m_flDist ) )
		{
			m_iLatestSurfProp = pTri->m_iSurfProp;
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBSweep( const Vector &rayStart, const Vector &rayEnd, const Vector &boxExtents, 
							   float startFrac, float endFrac, CBaseTrace *pTrace )
{
	static bool bRender = false;

	//
	// create and initialize the triangle list
	//
	TriList_t triList;
	triList.m_Count = 0;

	//
	// create and initialize the primary AABB
	//
	AABB_t AABBox;
	AABB_Init( AABBox );

	//
	// sweep box against the axial-aligned bboxed quad-tree and generate an initial
	// list of collision tris
	//
	SweptAABB_BuildTriList( rayStart, rayEnd, boxExtents, 0, AABBox, triList );

	// save the starting fraction
	float preIntersectFrac = pTrace->fraction;

	//
	// sweep axis-aligned bounding box against the triangles in the list
	//
	if( triList.m_Count > 0 )
	{
		SweptAABB_IntersectTriList( rayStart, rayEnd, boxExtents, startFrac,
				                    endFrac, pTrace, triList );
	}

	// collision
	if( preIntersectFrac > pTrace->fraction )
	{
		return true;
	}

	// no collision
	return false; 
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::SweptAABB_BuildTriList( const Vector &rayStart, const Vector &rayEnd,
										    const Vector &boxExtents, int ndxNode,
											AABB_t &AABBox, TriList_t &triList )
{
	// get the current node
	Node_t *pNode = &m_pNodes[ndxNode];

	//
	// fill in AABBox plane distances
	//
	AABBox.m_Dists[0] = -( pNode->m_BBox[0].x - DIST_EPSILON );
	AABBox.m_Dists[1] = ( pNode->m_BBox[1].x + DIST_EPSILON );
	AABBox.m_Dists[2] = -( pNode->m_BBox[0].y - DIST_EPSILON );
	AABBox.m_Dists[3] = ( pNode->m_BBox[1].y + DIST_EPSILON );
	AABBox.m_Dists[4] = -( pNode->m_BBox[0].z - DIST_EPSILON );
	AABBox.m_Dists[5] = ( pNode->m_BBox[1].z + DIST_EPSILON );

	// test the swept box against the given node
	if( SweptAABB_NodeTest( rayStart, rayEnd, boxExtents, AABBox ) )
	{
		//
		// if leaf add tris to list
		//
		if( Node_IsLeaf( *pNode ) )
		{
			// debugging!!!
			assert( triList.m_Count >= 0 );
			assert( triList.m_Count < DISPCOLL_TRILIST_SIZE );

			if( triList.m_Count < DISPCOLL_TRILIST_SIZE )
			{
				triList.m_ppTriList[triList.m_Count] = &m_pTris[pNode->m_iTris[0]];
				triList.m_ppTriList[triList.m_Count+1] = &m_pTris[pNode->m_iTris[1]];
				triList.m_Count += 2;
			}

			return;
		}
		// continue testing with children nodes
		else
		{
			SweptAABB_BuildTriList( rayStart, rayEnd, boxExtents, Nodes_GetChild( ndxNode, 0 ), AABBox, triList );
			SweptAABB_BuildTriList( rayStart, rayEnd, boxExtents, Nodes_GetChild( ndxNode, 1 ), AABBox, triList );
			SweptAABB_BuildTriList( rayStart, rayEnd, boxExtents, Nodes_GetChild( ndxNode, 2 ), AABBox, triList );
			SweptAABB_BuildTriList( rayStart, rayEnd, boxExtents, Nodes_GetChild( ndxNode, 3 ), AABBox, triList );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::SweptAABB_NodeTest( const Vector &rayStart, const Vector &rayEnd,
									    const Vector &boxExtents, AABB_t const &AABBox )
{
	//
	// de-normalize the paramter space so that we don't have to divide to the the
	// fractional amount later
	// NOTE: deltas are clamped for precision
	//
#if 0
	Vector delta, scalar;
	delta = rayEnd - rayStart;
	if( ( delta.x < DIST_EPSILON ) && ( delta.x > -DIST_EPSILON ) ) { delta.x = 1.0f; }
	if( ( delta.y < DIST_EPSILON ) && ( delta.y > -DIST_EPSILON ) ) { delta.y = 1.0f; }
	if( ( delta.z < DIST_EPSILON ) && ( delta.z > -DIST_EPSILON ) ) { delta.z = 1.0f; }
	scalar.x = delta.y * delta.z;
	scalar.y = delta.x * delta.z;
	scalar.z = delta.x * delta.y;
#endif

	//
	// create and initialize the enter and exit fractions
	//
	float enterFraction = 0.0f;
	float exitFraction = 1.0f;

	//
	// test the ray against the AABB (reduced to 1d tests)
	//
	float distStart, distEnd, fraction;

	for( int ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		//
		// test negative axial direction
		//
		distStart = -rayStart[ndxAxis] - ( AABBox.m_Dists[(ndxAxis<<1)] + boxExtents[ndxAxis] /*+ DIST_EPSILON*/ );
		distEnd = -rayEnd[ndxAxis] - ( AABBox.m_Dists[(ndxAxis<<1)] + boxExtents[ndxAxis] /*+ DIST_EPSILON*/ );

		if( ( distStart > 0.0f ) && ( distEnd < 0.0f ) )
		{
			fraction = ( distStart - DIST_EPSILON ) / ( distStart - distEnd );
//			fraction = distStart * scalar[ndxAxis];
			if( fraction > enterFraction )
			{
				enterFraction = fraction;
			}
		}
		else if( ( distStart < 0.0f ) && ( distEnd > 0.0f ) )
		{
			fraction = ( distStart + DIST_EPSILON ) / ( distStart - distEnd );
//			fraction = distStart * scalar[ndxAxis];
			if( fraction < exitFraction )
			{
				exitFraction = fraction;
			}
		}
		else if( ( distStart > 0.0f ) && ( distEnd > 0.0f ) )
		{
			return false;
		}

		//
		// test positive axial direction
		//
		distStart = rayStart[ndxAxis] - ( AABBox.m_Dists[(ndxAxis<<1)+1] + boxExtents[ndxAxis] /*+ DIST_EPSILON*/ );
		distEnd = rayEnd[ndxAxis] - ( AABBox.m_Dists[(ndxAxis<<1)+1] + boxExtents[ndxAxis] /*+ DIST_EPSILON*/ );

		if( ( distStart > 0.0f ) && ( distEnd < 0.0f ) )
		{
			fraction = ( distStart - DIST_EPSILON ) / ( distStart - distEnd );
//			fraction = distStart * scalar[ndxAxis];
			if( fraction > enterFraction )
			{
				enterFraction = fraction;
			}
		}
		else if( ( distStart < 0.0f ) && ( distEnd > 0.0f ) )
		{
			fraction = ( distStart + DIST_EPSILON ) / ( distStart - distEnd );
//			fraction = distStart * scalar[ndxAxis];
			if( fraction < exitFraction )
			{
				exitFraction = fraction;
			}
		}
		else if( ( distStart > 0.0f ) && ( distEnd > 0.0f ) )
		{
			return false;
		}
	}

	// test results
	if( exitFraction < enterFraction )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CDispCollTree::SweptAABB_IntersectTriList( const Vector &rayStart, const Vector &rayEnd, 
											    const Vector &boxExtents, float startFrac, float endFrac, 
												CBaseTrace *pTrace, TriList_t const &triList )
{
	Tri_t *pTri;

	Vector  impactNormal;
	float	impactDist;

	// initialize impact data
	impactNormal.Init();
	impactDist = 0.0f;

	//
	// intersect against all the flagged triangles in trilist
	//
	for( int iTri = 0; iTri < triList.m_Count; iTri++ )
	{
		// get the current displacement
		pTri = triList.m_ppTriList[iTri];

#if 0
		float fraction = 1.0f;
#endif

		// intersection test -- should change the name to IntersectAABoxSweptManifoldTriangle
		IntersectAABoxSweptTriangle( rayStart, rayEnd, boxExtents,
 			                         m_pVerts[pTri->m_uiVerts[0]],
									 m_pVerts[pTri->m_uiVerts[2]],
									 m_pVerts[pTri->m_uiVerts[1]],
									 pTri->m_vecNormal, pTri->m_flDist,
									 pTri->m_nFlags, pTri->m_iSurfProp,
									 /*fraction,*/ pTrace, true );

#if 0
		// a negative fraction means no collision
		if( fraction < 0.0f )
			continue;

		// compare intersection fraction to current trace fraction
		if( fraction < pTrace->fraction )
		{
			pTrace->fraction = fraction;
			pTrace->plane.normal = pTri->m_Normal;
			pTrace->plane.dist = pTri->m_Dist;

			m_iLatestSurfProp = pTri->m_iSurfProp;
		}
#endif
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void FindMin( float v1, float v2, float v3, float &min )
{
	min = v1;
	if( v2 < min ) { min = v2; }
	if( v3 < min ) { min = v3; }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void FindMax( float v1, float v2, float v3, float &max )
{
	max = v1;
	if( v2 > max ) { max = v2; }
	if( v3 > max ) { max = v3; }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CalcClosestBoxPoint( const Vector &planeNormal, const Vector &boxStart, 
						         const Vector &boxExtents, Vector &boxPt )
{
	boxPt = boxStart;
	( planeNormal[0] < 0.0f ) ? boxPt[0] += boxExtents[0] : boxPt[0] -= boxExtents[0];
	( planeNormal[1] < 0.0f ) ? boxPt[1] += boxExtents[1] : boxPt[1] -= boxExtents[1];
	( planeNormal[2] < 0.0f ) ? boxPt[2] += boxExtents[2] : boxPt[2] -= boxExtents[2];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CalcClosestExtents( const Vector &planeNormal, const Vector &boxExtents, 
						   Vector &boxPt )
{
	( planeNormal[0] < 0.0f ) ? boxPt[0] = boxExtents[0] : boxPt[0] = -boxExtents[0];
	( planeNormal[1] < 0.0f ) ? boxPt[1] = boxExtents[1] : boxPt[1] = -boxExtents[1];
	( planeNormal[2] < 0.0f ) ? boxPt[2] = boxExtents[2] : boxPt[2] = -boxExtents[2];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*inline*/ bool CDispCollTree::ResolveRayPlaneIntersect( float distStart, float distEnd,
									                 float &fracStart, float &fracEnd,
													 const Vector &boxDir, const Vector &normal, 
													 float planeDist, bool &bStartOutSolid, 
													 bool &bEndOutSolid, int index )
{
	if( ( distStart > 0.0f ) && ( distEnd > 0.0f ) ) 
		return false; 

//	if( ( distStart <= 0.0f ) && ( distEnd <= 0.0f ) ) { return true; }
	if( ( distStart < 0.0f ) && ( distEnd < 0.0f ) ) 
		return true; 

	if( ( distStart >= 0.0f ) && ( distEnd <= 0.0f ) )
	{
		// find t - the parametric distance along the trace line
		float t = ( distStart - DIST_EPSILON ) / ( distStart - distEnd );
		if( t > fracStart )
		{
			fracStart = t;
		}
	}
	else
//	else if( ( distStart <= 0.0f ) && ( distEnd > 0.0f ) )
	{
		// find t - the parametric distance along the trace line
		float t = ( distStart + DIST_EPSILON ) / ( distStart - distEnd );
		if( t < fracEnd )
		{
			fracEnd = t;
		}	
	}
	
	return true;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool NeedToNegate( const Vector &normal, float dist, const Vector &v1, const Vector &v2,
				   const Vector &v3 )
{
	if( normal.Dot( v1 ) - dist > 0.0f ) { return true; }
	if( normal.Dot( v2 ) - dist > 0.0f ) { return true; }
	if( normal.Dot( v3 ) - dist > 0.0f ) { return true; }

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void FindMinMaxPointsInPlaneDirection( const Vector &v1, const Vector &v2, const Vector &v3,
		   								      const Vector &planeNormal, Vector &vMin, Vector &vMax,
										      float &minDist, float &maxDist )
{
	//
	// find distance to each point on the triangle
	//
	float dist1 = planeNormal.Dot( v1 );
	float dist2 = planeNormal.Dot( v2 );
	float dist3 = planeNormal.Dot( v3 );

	//
	// find the min, max points
	//
	vMin = vMax = v1;
	minDist = maxDist = dist1;
	if( dist2 < minDist ) { vMin = v2; minDist = dist2; }
	if( dist2 > maxDist ) { vMax = v2; maxDist = dist2; }
	if( dist3 < minDist ) { vMin = v3; minDist = dist3; }
	if( dist3 > maxDist ) { vMax = v3; maxDist = dist3; }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CDispCollTree::AxialPlanesXYZ( const Vector &v1, const Vector &v2, const Vector &v3,
										   const Vector &boxStart, const Vector &boxEnd, const Vector &boxExtents,
										   const Vector &sweptDir, const Vector &triNormal, float &fracStart, float &fracEnd,
										   bool &bStartOutSolid, bool &bEndOutSolid )
{
	// verify
	Vector boxDir;
	boxDir = sweptDir;
	VectorNormalize( boxDir );

	//
	// test axial planes (x, y, z)
	//
	float dist, distStart, distEnd;

	float closeValue;
	Vector boxPt;
	CalcClosestBoxPoint( triNormal, boxStart, boxExtents, boxPt );

	for( int ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		if( triNormal[ndxAxis] > 0.0f )
		{
			Vector normal( 0.0f, 0.0f, 0.0f );
			normal[ndxAxis] = 1.0f;				

			FindMax( v1[ndxAxis], v2[ndxAxis], v3[ndxAxis], closeValue );
			dist = closeValue + boxExtents[ndxAxis];

			distStart = boxPt[ndxAxis] - closeValue;
			distEnd = ( boxPt[ndxAxis] + sweptDir[ndxAxis] ) - closeValue;

			if( !ResolveRayPlaneIntersect( distStart, distEnd, fracStart, fracEnd, boxDir, normal, dist, bStartOutSolid, bEndOutSolid, ndxAxis + 1 ) )
				return false;
		}
		else
		{
			Vector normal( 0.0f, 0.0f, 0.0f );
			normal[ndxAxis] = -1.0f;

			FindMin( v1[ndxAxis], v2[ndxAxis], v3[ndxAxis], closeValue );
			dist = closeValue - boxExtents[ndxAxis];

			distStart = closeValue - boxPt[ndxAxis];
			distEnd = closeValue - ( boxPt[ndxAxis] + sweptDir[ndxAxis] );

			if( !ResolveRayPlaneIntersect( distStart, distEnd, fracStart, fracEnd, boxDir, normal, dist, bStartOutSolid, bEndOutSolid, ndxAxis + 1 ) )
				return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CDispCollTree::FacePlane( const Vector &triNormal, float triDist,
									  const Vector &boxStart, const Vector &boxEnd, const Vector &boxExtents,
									  float &fracStart, float &fracEnd,
									  /* debug */ const Vector &v1, const Vector &v2, const Vector &v3,
									  bool &bStartOutSolid, bool &bEndOutSolid )
{
	// calculate the closest point on box to plane (get extents in that direction)
	Vector ptExtent;
	CalcClosestExtents( triNormal, boxExtents, ptExtent );

	//
	// expand the plane by the extents of the box to reduce the swept box/triangle
	// test to a ray/extruded triangle test (one of the triangles extruded planes
	// was just calculated above
	//
	float expandDist = triDist - triNormal.Dot( ptExtent );
	float distStart = triNormal.Dot( boxStart ) - expandDist;
	float distEnd = triNormal.Dot( boxEnd ) - expandDist;

	Vector boxDir;
	boxDir = boxEnd - boxStart;
	VectorNormalize( boxDir );

	// resolve the ray/plane collision
	if( !ResolveRayPlaneIntersect( distStart, distEnd, fracStart, fracEnd, boxDir, triNormal, expandDist, bStartOutSolid, bEndOutSolid, 0 ) )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossAxialX( const Vector &edge, const Vector &ptOnEdge,
										    const Vector &ptOffEdge, const Vector &boxExtents,
											const Vector &boxStart, const Vector &boxEnd,
											const Vector &triNormal, float triDist,
											float &fracStart, float &fracEnd, 
											bool &bStartOutSolid, bool &bEndOutSolid, int index )
{	
	// calculate the normal - edge x axialX = ( 0.0, edgeZ, -edgeY )
	Vector normal( 0.0f, edge.z, -edge.y );
	VectorNormalize( normal );

	// check for zero length normals
	if( ( normal.x == 0.0f ) && ( normal.y == 0.0f ) && ( normal.z == 0.0f ) )
		return true;

	// finish the plane definition - get distance
	float dist = ( normal.y * ptOnEdge.y ) + ( normal.z * ptOnEdge.z );

	// special case the point off edge in plane
	float ptOffDist = ( normal.y * ptOffEdge.y ) + ( normal.z * ptOffEdge.z );
	if( FloatMakePositive( ptOffDist - dist ) < ONPLANE_EPSILON )
	{
		normal.x = triNormal.x;
		normal.y = triNormal.y;
		normal.z = triNormal.z;
		dist = triDist;
	}
	// adjust plane facing if necessay - triangle should be behind the plane
	else if( ptOffDist > dist )
	{
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}

	// calculate the closest point on box to plane (get extents in that direction)
	Vector ptExtent;
	CalcClosestExtents( normal, boxExtents, ptExtent );

	//
	// expand the plane by the extents of the box to reduce the swept box/triangle
	// test to a ray/extruded triangle test (one of the triangles extruded planes
	// was just calculated above
	//
	float expandDist = dist - ( ( normal.y * ptExtent.y ) + ( normal.z * ptExtent.z ) );
	float distStart = ( normal.y * boxStart.y ) + ( normal.z * boxStart.z ) - expandDist;
	float distEnd = ( normal.y * boxEnd.y ) + ( normal.z * boxEnd.z ) - expandDist;

	Vector boxDir;
	boxDir = boxEnd - boxStart;
	VectorNormalize( boxDir );

	// resolve the ray/plane collision
	if( !ResolveRayPlaneIntersect( distStart, distEnd, fracStart, fracEnd, boxDir, normal, expandDist, bStartOutSolid, bEndOutSolid, index ) )
		return false;

	return true;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossAxialY( const Vector &edge, const Vector &ptOnEdge,
											const Vector &ptOffEdge, const Vector &boxExtents,
											const Vector &boxStart, const Vector &boxEnd,
											const Vector &triNormal, float triDist,
											float &fracStart, float &fracEnd, 
											bool &bStartOutSolid, bool &bEndOutSolid, int index )
{
	// calculate the normal - edge x axialY = ( -edgeZ, 0.0, edgeX )
	Vector normal( -edge.z, 0.0f, edge.x );
	VectorNormalize( normal );

	// check for zero length normals
	if( ( normal.x == 0.0f ) && ( normal.y == 0.0f ) && ( normal.z == 0.0f ) )
		return true;

	// finish the plane definition - get distance
	float dist = ( normal.x * ptOnEdge.x ) + ( normal.z * ptOnEdge.z );
	float ptOffDist = ( normal.x * ptOffEdge.x ) + ( normal.z * ptOffEdge.z );

	// special case the point off edge in plane
	if( FloatMakePositive( ptOffDist - dist ) < ONPLANE_EPSILON )
	{
		normal.x = triNormal.x;
		normal.y = triNormal.y;
		normal.z = triNormal.z;
		dist = triDist;
	}
	// adjust plane facing if necessay - triangle should be behind the plane
	else if( ptOffDist > dist )
	{
		normal.x = -normal.x;
		normal.z = -normal.z;
		dist = -dist;
	}

	// calculate the closest point on box to plane (get extents in that direction)
	Vector ptExtent;
	CalcClosestExtents( normal, boxExtents, ptExtent );

	//
	// expand the plane by the extents of the box to reduce the swept box/triangle
	// test to a ray/extruded triangle test (one of the triangles extruded planes
	// was just calculated above
	//
	float expandDist = dist - ( ( normal.x * ptExtent.x ) + ( normal.z * ptExtent.z ) );
	float distStart = ( normal.x * boxStart.x ) + ( normal.z * boxStart.z ) - expandDist;
	float distEnd = ( normal.x * boxEnd.x ) + ( normal.z * boxEnd.z ) - expandDist;

	Vector boxDir;
	boxDir = boxEnd - boxStart;
	VectorNormalize( boxDir );

	// resolve the ray/plane collision
	if( !ResolveRayPlaneIntersect( distStart, distEnd, fracStart, fracEnd, boxDir, normal, expandDist, bStartOutSolid, bEndOutSolid, index ) )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossAxialZ( const Vector &edge, const Vector &ptOnEdge,
											const Vector &ptOffEdge, const Vector &boxExtents,
											const Vector &boxStart, const Vector &boxEnd,
											const Vector &triNormal, float triDist,
											float &fracStart, float &fracEnd, 
											bool &bStartOutSolid, bool &bEndOutSolid, int index )
{
	// calculate the normal - edge x axialY = ( edgeY, -edgeX, 0.0 )
	Vector normal( edge.y, -edge.x, 0.0f );
	VectorNormalize( normal );

	// check for zero length normals
	if( ( normal.x == 0.0f ) && ( normal.y == 0.0f ) && ( normal.z == 0.0f ) )
		return true;

	// finish the plane definition - get distance
	float dist = ( normal.x * ptOnEdge.x ) + ( normal.y * ptOnEdge.y );
	float ptOffDist = ( normal.x * ptOffEdge.x ) + ( normal.y * ptOffEdge.y );

	// special case the point off edge in plane
	if( FloatMakePositive( ptOffDist - dist ) < ONPLANE_EPSILON )
	{
		normal.x = triNormal.x;
		normal.y = triNormal.y;
		normal.z = triNormal.z;
		dist = triDist;
	}
	// adjust plane facing if necessay - triangle should be behind the plane
	else if( ptOffDist > dist )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		dist = -dist;
	}

	// calculate the closest point on box to plane (get extents in that direction)
	Vector ptExtent;
	CalcClosestExtents( normal, boxExtents, ptExtent );

	//
	// expand the plane by the extents of the box to reduce the swept box/triangle
	// test to a ray/extruded triangle test (one of the triangles extruded planes
	// was just calculated above
	//
	float expandDist = dist - ( ( normal.x * ptExtent.x ) + ( normal.y * ptExtent.y ) );
	float distStart = ( normal.x * boxStart.x ) + ( normal.y * boxStart.y ) - expandDist;
	float distEnd = ( normal.x * boxEnd.x ) + ( normal.y * boxEnd.y ) - expandDist;

	Vector boxDir;
	boxDir = boxEnd - boxStart;
	VectorNormalize( boxDir );

	// resolve the ray/plane collision
	if( !ResolveRayPlaneIntersect( distStart, distEnd, fracStart, fracEnd, boxDir, normal, expandDist, bStartOutSolid,
		                           bEndOutSolid, index ) )
		return false;

	return true;
}


#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossSweptDir( const Vector &edge, const Vector &ptOnEdge,
											  const Vector &ptOffEdge, const Vector &sweptDir, 
											  const Vector &boxExtents, const Vector &boxStart, 
											  const Vector &boxEnd, float &fracStart, float &fracEnd )
{
	// calculate the normal - edge x sweep direction
	Vector normal;
	normal = edge.Cross( sweptDir );
	VectorNormalize( normal );

	// finish the plane definition - get distance
	float dist = normal.Dot( ptOnEdge );

	// adjust plane facing if necessay - triangle should be behind the plane
	if( normal.Dot( ptOffEdge ) > dist )
	{
		normal.Negate();
		dist = -dist;
	}

	// calculate the closest point on box to plane (get extents in that direction)
	Vector ptExtent;
	CalcClosestExtents( normal, boxExtents, ptExtent );

	//
	// expand the plane by the extents of the box to reduce the swept box/triangle
	// test to a ray/extruded triangle test (one of the triangles extruded planes
	// was just calculated above
	//
	float expandDist = dist - normal.Dot( ptExtent );
	float distStart = normal.Dot( boxStart ) - expandDist;
	float distEnd = normal.Dot( boxEnd ) - expandDist;

	// debug mode -- draw the data
	if( bDrawDebug )
	{
		Vector tmp = edge - ptOnEdge;
		ScratchPad3D_RenderSeparatingAxis( boxStart, boxEnd, ptOnEdge, tmp, ptOffEdge, normal, expandDist, true, true );
	}

	// resolve the ray/plane collision
	if( !ResolveRayPlaneIntersect( distStart, distEnd, fracStart, fracEnd, -1 ) )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CDispCollTree::DirectionOfMotionCrossAxialPlanes( const Vector &boxStart, const Vector &boxEnd, const Vector &vDir,
														      const Vector &boxExtents, float &fracStart, float &fracEnd,
														      const Vector &v1, const Vector &v2, const Vector &v3 )
{
	// normalize the direction of motion
	Vector vDoM = vDir;
	VectorNormalize( vDoM );

	//
	// Direction of Motion (DoM) cross axial-x ( 0, DoMz, -DoMy )
	//
	Vector vNormal( 0.0f, vDoM.z, -vDoM.y );

	Vector vMin, vMax;
	float  minDist, maxDist;
	FindMinMaxPointsInPlaneDirection( v1, v2, v3, vNormal, vMin, vMax, minDist, maxDist );

	//
	// test plane at maximum
	//
	Vector ptExtent;
	CalcClosestExtents( vNormal, boxExtents, ptExtent );
	maxDist -= vNormal.Dot( ptExtent );
	float sDist = vNormal.Dot( boxStart ) - maxDist;
	float eDist = vNormal.Dot( boxEnd ) - maxDist;

	if( bDrawDebug )
	{
		ScratchPad3D_RenderSeparatingAxis( boxStart, boxEnd, v1, v2, v3, vNormal, maxDist, true, true );
	}

	if( !ResolveRayPlaneIntersect( sDist, eDist, fracStart, fracEnd, -1 ) )
		return false;

	//
	// test plane at minimum (flip normal)
	//
	vNormal.Negate();
	minDist = -minDist;
	CalcClosestExtents( vNormal, boxExtents, ptExtent );
	minDist -= vNormal.Dot( ptExtent );
	sDist = vNormal.Dot( boxStart ) - minDist;
	eDist = vNormal.Dot( boxEnd ) - minDist;

	if( bDrawDebug )
	{
		ScratchPad3D_RenderSeparatingAxis( boxStart, boxEnd, v1, v2, v3, vNormal, minDist, true, true );
	}

	if( !ResolveRayPlaneIntersect( sDist, eDist, fracStart, fracEnd, -1 ) )
		return false;

	//
	// DoM cross axial-y ( -DoMz, 0, DoMx )
	//
	vNormal.Init( -vDoM.z, 0.0f, vDoM.x );
	FindMinMaxPointsInPlaneDirection( v1, v2, v3, vNormal, vMin, vMax, minDist, maxDist );

	//
	// test plane at maximum
	//
	CalcClosestExtents( vNormal, boxExtents, ptExtent );
	maxDist -= vNormal.Dot( ptExtent );
	sDist = vNormal.Dot( boxStart ) - maxDist;
	eDist = vNormal.Dot( boxEnd ) - maxDist;

	if( bDrawDebug )
	{
		ScratchPad3D_RenderSeparatingAxis( boxStart, boxEnd, v1, v2, v3, vNormal, maxDist, true, true );
	}

	if( !ResolveRayPlaneIntersect( sDist, eDist, fracStart, fracEnd, -1 ) )
		return false;

	//
	// test plane at minimum (flip normal)
	//
	vNormal.Negate();
	minDist = -minDist;
	CalcClosestExtents( vNormal, boxExtents, ptExtent );
	minDist -= vNormal.Dot( ptExtent );
	sDist = vNormal.Dot( boxStart ) - minDist;
	eDist = vNormal.Dot( boxEnd ) - minDist;

	if( bDrawDebug )
	{
		ScratchPad3D_RenderSeparatingAxis( boxStart, boxEnd, v1, v2, v3, vNormal, minDist, true, true );
	}

	if( !ResolveRayPlaneIntersect( sDist, eDist, fracStart, fracEnd, -1 ) )
		return false;

	//
	// DoM cross axial-z ( DoMy, -DoMx, 0 )
	//
	vNormal.Init( vDoM.y, -vDoM.x, 0.0f );
	FindMinMaxPointsInPlaneDirection( v1, v2, v3, vNormal, vMin, vMax, minDist, maxDist );

	//
	// test plane at maximum
	//
	CalcClosestExtents( vNormal, boxExtents, ptExtent );
	maxDist -= vNormal.Dot( ptExtent );
	sDist = vNormal.Dot( boxStart ) - maxDist;
	eDist = vNormal.Dot( boxEnd ) - maxDist;

	if( bDrawDebug )
	{
		ScratchPad3D_RenderSeparatingAxis( boxStart, boxEnd, v1, v2, v3, vNormal, maxDist, true, true );
	}

	if( !ResolveRayPlaneIntersect( sDist, eDist, fracStart, fracEnd, -1 ) )
		return false;

	//
	// test plane at minimum (flip normal)
	//
	vNormal.Negate();
	minDist = -minDist;
	CalcClosestExtents( vNormal, boxExtents, ptExtent );
	minDist -= vNormal.Dot( ptExtent );
	sDist = vNormal.Dot( boxStart ) - minDist;
	eDist = vNormal.Dot( boxEnd ) - minDist;

	if( bDrawDebug )
	{
		ScratchPad3D_RenderSeparatingAxis( boxStart, boxEnd, v1, v2, v3, vNormal, minDist, true, true );
	}

	if( !ResolveRayPlaneIntersect( sDist, eDist, fracStart, fracEnd, -1 ) )
		return false;

	return true;
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CDispCollTree::IntersectAABoxSweptTriangle( const Vector &boxStart, const Vector &boxEnd,
								                 const Vector &boxExtents, const Vector &v1,
								                 const Vector &v2, const Vector &v3,
								                 const Vector &triNormal, float triDist,
												 unsigned short triFlags, unsigned short triSurfProp,
												 CBaseTrace *pTrace, bool bStartOutside )
{
	//
	// make sure the box and triangle are not initially intersecting!!
	// NOTE: if bStartOutside is set -- this is assumed
	//
	if( !bStartOutside )
	{
		// check for interection -- if not intersecting continue, otherwise
		// return and let the "in solid" functions handle it
	}

	//
	// initialize the axial-aligned box sweep triangle test
	//
	bool  bStartOutSolid = false;
	bool  bEndOutSolid = false;
	float fracStart = NEVER_UPDATED;
	float fracEnd = 1.0f;

	// calculate the box direction - the sweep direction
	Vector boxDir = boxEnd - boxStart;

	//
	// OPTIMIZATION: make sure objects are traveling toward one another
	//
#if 1
	float angle = triNormal.Dot( boxDir );
	if( angle/*triNormal.Dot( boxDir )*/ > DIST_EPSILON )
	{
		return;
	}
#endif

#if 0
	if( triNormal[2] < 0.7 )
	{
		bDrawDebug = true;
		m_pScratchPad3D->Clear();
	}
#endif

	// test against the triangle face plane
	if( !FacePlane( triNormal, triDist, boxStart, boxEnd, boxExtents, fracStart, fracEnd, v1, v2, v3,
		            bStartOutSolid, bEndOutSolid ) ) { return; }

	// test against axial planes (of the aabb)
	if( !AxialPlanesXYZ( v1, v2, v3, boxStart, boxEnd, boxExtents, boxDir, triNormal, fracStart, fracEnd,
		                 bStartOutSolid, bEndOutSolid ) ) { return; }

	//
	// There are 9 edge tests - edges 1, 2, 3 cross with the box edges (symmetry) 1, 2, 3.  However, the box
	// is axial-aligned resulting in axially directional edges -- thus each test is edges 1, 2, and 3 vs. 
	// axial planes x, y, and z
	//
	// There are potentially 9 more tests with edges, the edge's edges and the direction of motion!
	// NOTE: I don't think these tests are necessary for a manifold surface -- but they still remain if
	// it ever becomes a problem in the future!
	//
	Vector edge;
		
	// edge 1 - axial tests are 2d tests, swept direction is a 3d test
	edge = v2 - v1;
	if( !EdgeCrossAxialX( edge, v1, v3, boxExtents, boxStart, boxEnd, triNormal, triDist, fracStart, fracEnd, bStartOutSolid, bEndOutSolid, 4 ) ) { return; }
	if( !EdgeCrossAxialY( edge, v1, v3, boxExtents, boxStart, boxEnd, triNormal, triDist, fracStart, fracEnd, bStartOutSolid, bEndOutSolid, 5 ) ) { return; }
	if( !EdgeCrossAxialZ( edge, v1, v3, boxExtents, boxStart, boxEnd, triNormal, triDist, fracStart, fracEnd, bStartOutSolid, bEndOutSolid, 6 ) ) { return; }
//	if( !EdgeCrossSweptDir( edge, v1, v3, boxDir, boxExtents, boxStart, boxEnd, fracStart, fracEnd ) ) { fraction = fracStart; return; }
	
	// edge 2 - axial tests are 2d tests, swept direction is a 3d test
	edge = v3 - v2;
	if( !EdgeCrossAxialX( edge, v2, v1, boxExtents, boxStart, boxEnd, triNormal, triDist, fracStart, fracEnd, bStartOutSolid, bEndOutSolid, 7 ) ) { return; }
	if( !EdgeCrossAxialY( edge, v2, v1, boxExtents, boxStart, boxEnd, triNormal, triDist, fracStart, fracEnd, bStartOutSolid, bEndOutSolid, 8 ) ) { return; }
	if( !EdgeCrossAxialZ( edge, v2, v1, boxExtents, boxStart, boxEnd, triNormal, triDist, fracStart, fracEnd, bStartOutSolid, bEndOutSolid, 9 ) ) { return; }
//	if( !EdgeCrossSweptDir( edge, v2, v1, boxDir, boxExtents, boxStart, boxEnd, fracStart, fracEnd ) ) { fraction = fracStart; return; }
	
	// edge 3 - axial tests are 2d tests, swept direction is a 3d test
	edge = v1 - v3;
	if( !EdgeCrossAxialX( edge, v3, v2, boxExtents, boxStart, boxEnd, triNormal, triDist, fracStart, fracEnd, bStartOutSolid, bEndOutSolid, 10 ) ) { return; }
	if( !EdgeCrossAxialY( edge, v3, v2, boxExtents, boxStart, boxEnd, triNormal, triDist, fracStart, fracEnd, bStartOutSolid, bEndOutSolid, 11 ) ) { return; }
	if( !EdgeCrossAxialZ( edge, v3, v2, boxExtents, boxStart, boxEnd, triNormal, triDist, fracStart, fracEnd, bStartOutSolid, bEndOutSolid, 12 ) ) { return; }
//	if( !EdgeCrossSweptDir( edge, v2, v1, boxDir, boxExtents, boxStart, boxEnd, fracStart, fracEnd ) ) { fraction = fracStart; return; }

	//
	// the direction of motion crossed with the axial planes is equivolent
	// to cross the box (axial planes) and the 
	//
//	if( !DirectionOfMotionCrossAxialPlanes( boxStart, boxEnd, boxDir, boxExtents, fracStart, fracEnd, v1, v2, v3 ) ) { fraction = fracStart; return; }

	//
	// didn't have a separating axis -- update trace data -- should I handle a fraction left solid here!????
	//

#if 0
	// started inside of a solid
	if( !bStartOutSolid )
	{
		pTrace->startsolid = true;

		// check to see if the whole sweep is behind??
		if( !bEndOutSolid )
		{
			pTrace->allsolid = true;
		}

		return;
	}
#endif

	if( fracStart < fracEnd )
	{
		if( ( fracStart > NEVER_UPDATED ) && ( fracStart < pTrace->fraction ) )
		{
			// clamp -- shouldn't really ever be here!???
			if( fracStart < 0.0f )
			{
				fracStart = 0.0f;
			}
			
			pTrace->fraction = fracStart;
			pTrace->plane.normal = triNormal;
			pTrace->plane.dist = triDist;
			pTrace->dispFlags = triFlags;
			m_iLatestSurfProp = triSurfProp;
		}
	}
}


#define SA_EPSILON	0.0f 

//--------------------------------------------------------------------------
// Purpose:
//
// NOTE:
//	triangle points are given in clockwise order
//
//    1				edge0 = 1 - 0
//    | \           edge1 = 2 - 1
//    |  \          edge2 = 0 - 2
//    |   \
//    |    \
//    0-----2
//
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossX2( float edgeZ, float edgeY, float absEdgeZ, float absEdgeY,
							     const Vector &p1, const Vector &p3, const Vector &extents,
								 int index )
{
	//
	// Cross Product( axialX(1,0,0) x edge ):
	//   normal.x = 0.0f
	//   normal.y = edge.z
	//   normal.z = -edge.y
	//
	// Triangle Point Distances:
	//   dist(x) = normal.y * pt(x).y + normal.z * pt(x).z
	//
	float dist1 = edgeZ * p1.y - edgeY * p1.z;
	float dist3 = edgeZ * p3.y - edgeY * p3.z;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.y ) * extents.y + abs( normal.z ) * extents.z
	//
	float distBox = absEdgeZ * extents.y + absEdgeY * extents.z;

	//
	// either dist1, dist3 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//
	if( dist1 < dist3 )
	{
		if( ( dist1 > ( distBox + SA_EPSILON ) ) || ( dist3 < -( distBox + SA_EPSILON ) ) )
			return false;
	}
	else
	{
		if( ( dist3 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossX3( float edgeZ, float edgeY, float absEdgeZ, float absEdgeY,
								 const Vector &p1, const Vector &p2, const Vector &extents,
								 int index )
{
	//
	// Cross Product( axialX(1,0,0) x edge ):
	//   normal.x = 0.0f
	//   normal.y = edge.z
	//   normal.z = -edge.y
	//
	// Triangle Point Distances:
	//   dist(x) = normal.y * pt(x).y + normal.z * pt(x).z
	//
	float dist1 = edgeZ * p1.y - edgeY * p1.z;
	float dist2 = edgeZ * p2.y - edgeY * p2.z;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.y ) * extents.y + abs( normal.z ) * extents.z
	//
	float distBox = absEdgeZ * extents.y + absEdgeY * extents.z;

	//
	// either dist1, dist2 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist1 < dist2 )
	{
		if( ( dist1 > ( distBox + SA_EPSILON ) ) || ( dist2 < -( distBox + SA_EPSILON ) ) )
			return false;
	}
	else
	{
		if( ( dist2 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossY2( float edgeZ, float edgeX, float absEdgeZ, float absEdgeX,
								 const Vector &p1, const Vector &p3, const Vector &extents,
								 int index )
{
	//
	// Cross Product( axialY(0,1,0) x edge ):
	//   normal.x = -edge.z
	//   normal.y = 0.0f
	//   normal.z = edge.x
	//
	// Triangle Point Distances:
	//   dist(x) = normal.x * pt(x).x + normal.z * pt(x).z
	//
	float dist1 = -edgeZ * p1.x + edgeX * p1.z;
	float dist3 = -edgeZ * p3.x + edgeX * p3.z;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.x ) * extents.x + abs( normal.z ) * extents.z
	//
	float distBox = absEdgeZ * extents.x + absEdgeX * extents.z;

	//
	// either dist1, dist3 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist1 < dist3 )
	{
		if( ( dist1 > distBox ) || ( dist3 < -( distBox + SA_EPSILON ) ) )
			return false;
	}
	else
	{
		if( ( dist3 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossY3( float edgeZ, float edgeX, float absEdgeZ, float absEdgeX,
								 const Vector &p1, const Vector &p2, const Vector &extents,
								 int index )
{
	//
	// Cross Product( axialY(0,1,0) x edge ):
	//   normal.x = -edge.z
	//   normal.y = 0.0f
	//   normal.z = edge.x
	//
	// Triangle Point Distances:
	//   dist(x) = normal.x * pt(x).x + normal.z * pt(x).z
	//
	float dist1 = -edgeZ * p1.x + edgeX * p1.z;
	float dist2 = -edgeZ * p2.x + edgeX * p2.z;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.x ) * extents.x + abs( normal.z ) * extents.z
	//
	float distBox = absEdgeZ * extents.x + absEdgeX * extents.z;

	//
	// either dist1, dist2 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist1 < dist2 )
	{
		if( ( dist1 > ( distBox + SA_EPSILON ) ) || ( dist2 < -( distBox + SA_EPSILON ) ) )
			return false;
	}
	else
	{
		if( ( dist2 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossZ1( float edgeY, float edgeX, float absEdgeY, float absEdgeX,
								 const Vector &p2, const Vector &p3, const Vector &extents,
								 int index )
{
	//
	// Cross Product( axialZ(0,0,1) x edge ):
	//   normal.x = edge.y
	//   normal.y = -edge.x
	//   normal.z = 0.0f
	//
	// Triangle Point Distances:
	//   dist(x) = normal.x * pt(x).x + normal.y * pt(x).y
	//
	float dist2 = edgeY * p2.x - edgeX * p2.y;
	float dist3 = edgeY * p3.x - edgeX * p3.y;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.x ) * extents.x + abs( normal.y ) * extents.y
	//
	float distBox = absEdgeY * extents.x + absEdgeX * extents.y; 

	//
	// either dist2, dist3 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist3 < dist2 )
	{
		if( ( dist3 > ( distBox + SA_EPSILON ) ) || ( dist2 < -( distBox + SA_EPSILON ) ) ) 
			return false;
	}
	else
	{
		if( ( dist2 > ( distBox + SA_EPSILON ) ) || ( dist3 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossZ2( float edgeY, float edgeX, float absEdgeY, float absEdgeX,
								 const Vector &p1, const Vector &p3, const Vector &extents,
								 int index )
{
	//
	// Cross Product( axialZ(0,0,1) x edge ):
	//   normal.x = edge.y
	//   normal.y = -edge.x
	//   normal.z = 0.0f
	//
	// Triangle Point Distances:
	//   dist(x) = normal.x * pt(x).x + normal.y * pt(x).y
	//
	float dist1 = edgeY * p1.x - edgeX * p1.y;
	float dist3 = edgeY * p3.x - edgeX * p3.y;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.x ) * extents.x + abs( normal.y ) * extents.y
	//
	float distBox = absEdgeY * extents.x + absEdgeX * extents.y; 

	//
	// either dist1, dist3 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist1 < dist3 )
	{
		if( ( dist1 > ( distBox + SA_EPSILON ) ) || ( dist3 < -( distBox + SA_EPSILON ) ) ) 
			return false;
	}
	else
	{
		if( ( dist3 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: find the minima and maxima of the 3 given values
//-----------------------------------------------------------------------------
inline void FindMinMax( float v1, float v2, float v3, float &min, float &max )
{
	min = max = v1;
	if( v2 < min ) { min = v2; }
	if( v2 > max ) { max = v2; }
	if( v3 < min ) { min = v3; }
	if( v3 > max ) { max = v3; }
}


//--------------------------------------------------------------------------
// Purpose: test for intersection between a plane and axial-aligned box
//--------------------------------------------------------------------------
inline bool OverlapPlaneAABox( const Vector &planeNormal, float planeDist,
								 const Vector &boxCenter, const Vector &boxExtents )
{
	Vector vMin, vMax;
	for( int ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		if( planeNormal[ndxAxis] < 0.0f )
		{
			vMin[ndxAxis] = boxExtents[ndxAxis];
			vMax[ndxAxis] = -boxExtents[ndxAxis];
		}
		else
		{
			vMin[ndxAxis] = -boxExtents[ndxAxis];
			vMax[ndxAxis] = boxExtents[ndxAxis];
		}
	}

	float distMin = planeDist - planeNormal.Dot( vMin );
	float distMax = planeDist - planeNormal.Dot( vMax );

	if( planeNormal.Dot( boxCenter ) - distMin > 0.0f ) { return false; }
	if( planeNormal.Dot( boxCenter ) - distMax >= 0.0f ) { return true; }

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Test for an intersection (overlap) between an axial-aligned bounding 
//          box (AABB) and a triangle.
//
// Using the "Separating-Axis Theorem" to test for intersections between
// a triangle and an axial-aligned bounding box (AABB).
// 1. 4 Face Plane Tests - these are defined by the axial planes (x,y,z) making 
//                         up the AABB and the triangle plane
// 2. 9 Edge Planes Tests - the 3 edges of the triangle crossed with all 3 axial 
//                          planes (x, y, z)
// Output: false = separating axis (no intersection)
//         true = intersection
//-----------------------------------------------------------------------------
bool CDispCollTree::SeparatingAxisAABoxTriangle( const Vector &boxCenter, const Vector &boxExtents,
						          const Vector &v1, const Vector &v2, const Vector &v3,
						          const Vector &planeNormal, float planeDist )
{
	//
	// test against the triangle face plane
	//
	if( !OverlapPlaneAABox( planeNormal, planeDist, boxCenter, boxExtents ) ) 
	{
		return false; 
	}

	//
	// test the axial planes (x,y,z) against the min, max of the triangle
	//
	float min, max;
	Vector p1, p2, p3;

	// x plane
	p1.x = v1.x - boxCenter.x;
	p2.x = v2.x - boxCenter.x;
	p3.x = v3.x - boxCenter.x;
	FindMinMax( p1.x, p2.x, p3.x, min, max );
	if( ( min > ( boxExtents.x + SA_EPSILON ) ) || ( max < -( boxExtents.x + SA_EPSILON ) ) ) 
	{
		return false; 
	}

	// y plane
	p1.y = v1.y - boxCenter.y;
	p2.y = v2.y - boxCenter.y;
	p3.y = v3.y - boxCenter.y;
	FindMinMax( p1.y, p2.y, p3.y, min, max );
	if( ( min > ( boxExtents.y + SA_EPSILON ) ) || ( max < -( boxExtents.y + SA_EPSILON ) ) ) 
	{ 
		return false; 
	}

	// z plane
	p1.z = v1.z - boxCenter.z;
	p2.z = v2.z - boxCenter.z;
	p3.z = v3.z - boxCenter.z;
	FindMinMax( p1.z, p2.z, p3.z, min, max );
	if( ( min > ( boxExtents.z + SA_EPSILON ) ) || ( max < -( boxExtents.z + SA_EPSILON ) ) ) 
	{ 
		return false; 
	}

	//
	// test the 9 edge cases
	//
	Vector edge, absEdge;

	// edge 0 (cross x,y,z)
	edge = p2 - p1;
	absEdge.y = FloatMakePositive( edge.y );
	absEdge.z = FloatMakePositive( edge.z );
	if( !AxisTestEdgeCrossX2( edge.z, edge.y, absEdge.z, absEdge.y, p1, p3, boxExtents, 4 ) ) 
	{ 
		return false; 
	}
	absEdge.x = FloatMakePositive( edge.x );
	if( !AxisTestEdgeCrossY2( edge.z, edge.x, absEdge.z, absEdge.x, p1, p3, boxExtents, 5 ) ) 
	{ 
		return false; 
	}
	if( !AxisTestEdgeCrossZ1( edge.y, edge.x, absEdge.y, absEdge.x, p2, p3, boxExtents, 6 ) ) 
	{ 
		return false; 
	}

	// edge 1 (cross x,y,z)
	edge = p3 - p2;
	absEdge.y = FloatMakePositive( edge.y );
	absEdge.z = FloatMakePositive( edge.z );
	if( !AxisTestEdgeCrossX2( edge.z, edge.y, absEdge.z, absEdge.y, p1, p2, boxExtents, 7 ) ) 
	{ 
		return false; 
	}
	absEdge.x = FloatMakePositive( edge.x );
	if( !AxisTestEdgeCrossY2( edge.z, edge.x, absEdge.z, absEdge.x, p1, p2, boxExtents, 8 ) ) 
	{ 
		return false; 
	}
	if( !AxisTestEdgeCrossZ2( edge.y, edge.x, absEdge.y, absEdge.x, p1, p3, boxExtents, 9 ) ) 
	{ 
		return false; 
	}

	// edge 2 (cross x,y,z)
	edge = p1 - p3;
	absEdge.y = FloatMakePositive( edge.y );
	absEdge.z = FloatMakePositive( edge.z );
	if( !AxisTestEdgeCrossX3( edge.z, edge.y, absEdge.z, absEdge.y, p1, p2, boxExtents, 10 ) ) 
	{ 
		return false; 
	}
	absEdge.x = FloatMakePositive( edge.x );
	if( !AxisTestEdgeCrossY3( edge.z, edge.x, absEdge.z, absEdge.x, p1, p2, boxExtents, 11 ) ) 
	{ 
		return false; 
	}
	if( !AxisTestEdgeCrossZ1( edge.y, edge.x, absEdge.y, absEdge.x, p2, p3, boxExtents, 12 ) ) 
	{ 
		return false; 
	}

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CDispCollTree *DispCollTrees_Alloc( int count )
{
	CDispCollTree *pTrees;

	pTrees = new CDispCollTree[count];
	if( !pTrees )
		return NULL;

	return pTrees;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DispCollTrees_Free( CDispCollTree *pTrees )
{
	if( pTrees )
	{
		delete [] pTrees;
		pTrees = NULL;
	}
}
