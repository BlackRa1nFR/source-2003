//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "disp_common.h"
#include "disp_powerinfo.h"


class CNodeVert
{
public:
						CNodeVert()					{}
						CNodeVert( int ix, int iy ) {x=ix; y=iy;}

	inline int&			operator[]( int i )			{return ((int*)this)[i];}
	inline int const&	operator[]( int i ) const	{return ((int*)this)[i];}

	int x, y;
};

static CNodeVert const g_NodeChildLookup[4][2] =
{
	{CNodeVert(0,0), CNodeVert(1,1)},
	{CNodeVert(1,0), CNodeVert(2,1)},
	{CNodeVert(0,1), CNodeVert(1,2)},
	{CNodeVert(1,1), CNodeVert(2,2)}
};

static CNodeVert const g_NodeTriWinding[9] =
{
	CNodeVert(0, 1), 
	CNodeVert(0, 0), 
	CNodeVert(1, 0), 
	CNodeVert(2, 0), 
	CNodeVert(2, 1), 
	CNodeVert(2, 2), 
	CNodeVert(1, 2), 
	CNodeVert(0, 2), 
	CNodeVert(0, 1) 
};

// Indexed by CORNER_. These store NEIGHBOREDGE_ defines and tell which edges butt up against the corner.
static int g_CornerEdges[4][2] =
{
	{ NEIGHBOREDGE_BOTTOM,	NEIGHBOREDGE_LEFT },	// CORNER_LOWER_LEFT
	{ NEIGHBOREDGE_TOP,		NEIGHBOREDGE_LEFT },	// CORNER_UPPER_LEFT
	{ NEIGHBOREDGE_TOP,		NEIGHBOREDGE_RIGHT },	// CORNER_UPPER_RIGHT
	{ NEIGHBOREDGE_BOTTOM,	NEIGHBOREDGE_RIGHT }	// CORNER_LOWER_RIGHT
};

int g_EdgeDims[4] =
{
	0,		// NEIGHBOREDGE_LEFT   = X
	1,		// NEIGHBOREDGE_TOP    = Y
	0,		// NEIGHBOREDGE_RIGHT  = X
	1		// NEIGHBOREDGE_BOTTOM = Y
};

CShiftInfo g_ShiftInfos[3][3] =
{
	{
		{0,  0, true},		// CORNER_TO_CORNER -> CORNER_TO_CORNER
		{0, -1, true},		// CORNER_TO_CORNER -> CORNER_TO_MIDPOINT
		{2, -1, true}		// CORNER_TO_CORNER -> MIDPOINT_TO_CORNER
	},
	
	{
		{0,  1, true},		// CORNER_TO_MIDPOINT -> CORNER_TO_CORNER
		{0,  0, false},		// CORNER_TO_MIDPOINT -> CORNER_TO_MIDPOINT (invalid)
		{0,  0, false}		// CORNER_TO_MIDPOINT -> MIDPOINT_TO_CORNER (invalid)
	},

	{	
		{-1, 1, true},		// MIDPOINT_TO_CORNER -> CORNER_TO_CORNER
		{0,  0, false},		// MIDPOINT_TO_CORNER -> CORNER_TO_MIDPOINT (invalid)
		{0,  0, false}		// MIDPOINT_TO_CORNER -> MIDPOINT_TO_CORNER (invalid)
	}
};

int g_EdgeSideLenMul[4] =
{
	0,
	1,
	1,
	0
};


// --------------------------------------------------------------------------------- //
// Helper functions.
// --------------------------------------------------------------------------------- //

inline int SignedBitShift( int val, int shift )
{
	if( shift > 0 )
		return val << shift;
	else
		return val >> -shift;
}

static inline void RotateVertIndex( 
	NeighborOrientation neighor, 
	int sideLengthMinus1,
	CVertIndex const &in,
	CVertIndex &out )
{
	if( neighor == ORIENTATION_CCW_0 )
	{
		out = in;
	}
	else if( neighor == ORIENTATION_CCW_90 )
	{
		out.x = in.y;
		out.y = sideLengthMinus1 - in.x;
	}
	else if( neighor == ORIENTATION_CCW_180 )
	{
		out.x = sideLengthMinus1 - in.x;
		out.y = sideLengthMinus1 - in.y;
	}
	else
	{
		out.x = sideLengthMinus1 - in.y;
		out.y = in.x;
	}
}

static inline void RotateVertIncrement( 
	NeighborOrientation neighor, 
	CVertIndex const &in,
	CVertIndex &out )
{
	if( neighor == ORIENTATION_CCW_0 )
	{
		out = in;
	}
	else if( neighor == ORIENTATION_CCW_90 )
	{
		out.x = in.y;
		out.y = -in.x;
	}
	else if( neighor == ORIENTATION_CCW_180 )
	{
		out.x = -in.x;
		out.y = -in.y;
	}
	else
	{
		out.x = -in.y;
		out.y = in.x;
	}
}


// --------------------------------------------------------------------------------- //
// CDispHelper functions.
// --------------------------------------------------------------------------------- //

int GetEdgeIndexFromPoint( CVertIndex const &index, int iMaxPower )
{
	int sideLengthMinus1 = 1 << iMaxPower;

	if( index.x == 0 )
		return NEIGHBOREDGE_LEFT;
	else if( index.y == sideLengthMinus1 )
		return NEIGHBOREDGE_TOP;
	else if( index.x == sideLengthMinus1 )
		return NEIGHBOREDGE_RIGHT;
	else if( index.y == 0 )
		return NEIGHBOREDGE_BOTTOM;
	else
		return -1;
}


int GetCornerIndexFromPoint( CVertIndex const &index, int iPower )
{
	int sideLengthMinus1 = 1 << iPower;

	if( index.x == 0 && index.y == 0 )
		return CORNER_LOWER_LEFT;
	
	else if( index.x == 0 && index.y == sideLengthMinus1 )
		return CORNER_UPPER_LEFT;

	else if( index.x == sideLengthMinus1 && index.y == sideLengthMinus1 )
		return CORNER_UPPER_RIGHT;

	else if( index.x == sideLengthMinus1 && index.y == 0 )
		return CORNER_LOWER_RIGHT;

	else
		return -1;
}


int GetNeighborEdgePower( CDispUtilsHelper *pDisp, int iEdge, int iSub )
{
	CDispNeighbor *pEdge = pDisp->GetEdgeNeighbor( iEdge );
	CDispSubNeighbor *pSub = &pEdge->m_SubNeighbors[iSub];
	if ( !pSub->IsValid() )
		return -1;

	CDispUtilsHelper *pNeighbor = pDisp->GetDispUtilsByIndex( pSub->GetNeighborIndex() );
	
	CShiftInfo *pInfo = &g_ShiftInfos[pSub->m_Span][pSub->m_NeighborSpan];
	Assert( pInfo->m_bValid );

	return pNeighbor->GetPower() + pInfo->m_PowerShiftAdd;
}


CDispUtilsHelper* SetupEdgeIncrements(
	CDispUtilsHelper *pDisp,
	int iEdge,
	int iSub,
	CVertIndex &myIndex,
	CVertIndex &myInc,
	CVertIndex &nbIndex,
	CVertIndex &nbInc,
	int &myEnd,
	int &iFreeDim )
{
	int iEdgeDim = g_EdgeDims[iEdge];
	iFreeDim = !iEdgeDim;

	CDispNeighbor *pSide = pDisp->GetEdgeNeighbor( iEdge );
	CDispSubNeighbor *pSub = &pSide->m_SubNeighbors[iSub];
	if ( !pSub->IsValid() )
		return NULL;

	CDispUtilsHelper *pNeighbor = pDisp->GetDispUtilsByIndex( pSub->m_iNeighbor );

	CShiftInfo *pShiftInfo = &g_ShiftInfos[pSub->m_Span][pSub->m_NeighborSpan];
	Assert( pShiftInfo->m_bValid );

	// Setup a start point and edge increment (NOTE: just precalculate these
	// and store them in the CDispSubNeighbors).
	CVertIndex tempInc;
	
	const CPowerInfo *pPowerInfo = pDisp->GetPowerInfo();
	myIndex[iEdgeDim] = g_EdgeSideLenMul[iEdge] * pPowerInfo->m_SideLengthM1;
	myIndex[iFreeDim] = pPowerInfo->m_MidPoint * iSub;
	TransformIntoSubNeighbor( pDisp, iEdge, iSub, myIndex, nbIndex );
	
	int myPower = pDisp->GetPowerInfo()->m_Power;
	int nbPower = pNeighbor->GetPowerInfo()->m_Power + pShiftInfo->m_PowerShiftAdd;
	
	myInc[iEdgeDim] = tempInc[iEdgeDim] = 0;
	if( nbPower > myPower )
	{
		myInc[iFreeDim] = 1;
		tempInc[iFreeDim] = 1 << (nbPower - myPower);
	}
	else
	{
		myInc[iFreeDim] = 1 << (myPower - nbPower);
		tempInc[iFreeDim] = 1;
	}
	RotateVertIncrement( pSub->GetNeighborOrientation(), tempInc, nbInc );

	// Walk along the edge.
	if( pSub->m_Span == CORNER_TO_MIDPOINT )
		myEnd = pDisp->GetPowerInfo()->m_SideLength >> 1;
	else
		myEnd = pDisp->GetPowerInfo()->m_SideLength - 1;

	return pNeighbor;
}


int GetSubNeighborIndex( 
	CDispUtilsHelper *pDisp,
	int iEdge,
	CVertIndex const &nodeIndex )
{
	const CPowerInfo *pPowerInfo = pDisp->GetPowerInfo();
	const CDispNeighbor *pSide = pDisp->GetEdgeNeighbor( iEdge );

	// Figure out if this is a vertical or horizontal edge.
	int iEdgeDim = g_EdgeDims[iEdge];
	int iFreeDim = !iEdgeDim;

	int iFreeIndex = nodeIndex[iFreeDim];

	// Figure out which of the (up to two) neighbors it lies in.
	int iSub = 0;
	if( iFreeIndex == pPowerInfo->m_MidPoint )
	{
		// If it's in the middle, we only are interested if there's one neighbor
		// next to us (so we can enable its middle vert). If there are any neighbors
		// that touch the midpoint, then we have no need to return them because it would
		// touch their corner verts which are always active.
		if( pSide->m_SubNeighbors[0].m_Span != CORNER_TO_CORNER )
			return -1;
	}
	else if ( iFreeIndex > pPowerInfo->m_MidPoint )
	{
		iSub = 1;
	}

	// Make sure we get a valid neighbor.
	if( !pSide->m_SubNeighbors[iSub].IsValid() )
	{
		if( iSub == 1 && 
			pSide->m_SubNeighbors[0].IsValid() && 
			pSide->m_SubNeighbors[0].m_Span == CORNER_TO_CORNER )
		{
			iSub = 0;
		}
		else
		{
			return -1;
		}
	}

	return iSub;
}


void SetupSpan( int iPower, int iEdge, NeighborSpan span, CVertIndex &viStart, CVertIndex &viEnd )
{
	int iFreeDim = !g_EdgeDims[iEdge];
	const CPowerInfo *pPowerInfo = GetPowerInfo( iPower );

	viStart = pPowerInfo->GetCornerPointIndex( iEdge );
	viEnd = pPowerInfo->GetCornerPointIndex( (iEdge+1) & 3 );;

	if ( iEdge == NEIGHBOREDGE_RIGHT || iEdge == NEIGHBOREDGE_BOTTOM )
	{
		// CORNER_TO_MIDPOINT and MIDPOINT_CORNER are defined where the edge moves up or right,
		// but pPowerInfo->GetCornerPointIndex walks around the edges clockwise, so on the
		// bottom and right edges (where GetCornerPointIndex has us moving down and left) we need to
		// reverse the sense here to make sure we return the right span.
		if ( span == CORNER_TO_MIDPOINT )
			viStart[iFreeDim] = pPowerInfo->GetMidPoint();
		else if ( span == MIDPOINT_TO_CORNER )
			viEnd[iFreeDim] = pPowerInfo->GetMidPoint();
	}
	else
	{
		if ( span == CORNER_TO_MIDPOINT )
			viEnd[iFreeDim] = pPowerInfo->GetMidPoint();
		else if ( span == MIDPOINT_TO_CORNER )
			viStart[iFreeDim] = pPowerInfo->GetMidPoint();
	}
}


CDispUtilsHelper* TransformIntoSubNeighbor( 
	CDispUtilsHelper *pDisp,
	int iEdge,
	int iSub,
	CVertIndex const &nodeIndex, 
	CVertIndex &out
	)
{
	const CDispSubNeighbor *pSub = &pDisp->GetEdgeNeighbor( iEdge )->m_SubNeighbors[iSub];

	// Find the part of pDisp's edge that this neighbor covers.
	CVertIndex viSrcStart, viSrcEnd;
	SetupSpan( pDisp->GetPower(), iEdge, pSub->GetSpan(), viSrcStart, viSrcEnd );

	// Find the corresponding parts on the neighbor.
	CDispUtilsHelper *pNeighbor = pDisp->GetDispUtilsByIndex( pSub->GetNeighborIndex() );
	int iNBEdge = (iEdge + 2 + pSub->GetNeighborOrientation()) & 3;
	
	CVertIndex viDestStart, viDestEnd;
	SetupSpan( pNeighbor->GetPower(), iNBEdge, pSub->GetNeighborSpan(), viDestEnd, viDestStart );


	// Now map the one into the other.
	int iFreeDim = !g_EdgeDims[iEdge];
	int fixedPercent = ((nodeIndex[iFreeDim] - viSrcStart[iFreeDim]) * (1<<16)) / (viSrcEnd[iFreeDim] - viSrcStart[iFreeDim]);
	Assert( fixedPercent >= 0 && fixedPercent <= (1<<16) );

	int nbDim = g_EdgeDims[iNBEdge];
	out[nbDim] = viDestStart[nbDim];
	out[!nbDim] = viDestStart[!nbDim] + ((viDestEnd[!nbDim] - viDestStart[!nbDim]) * fixedPercent) / (1<<16);

	Assert( out.x >= 0 && out.x < pNeighbor->GetSideLength() );
	Assert( out.y >= 0 && out.y < pNeighbor->GetSideLength() );

	return pNeighbor;
}


CDispUtilsHelper* TransformIntoNeighbor( 
	CDispUtilsHelper *pDisp,
	int iEdge,
	CVertIndex const &nodeIndex, 
	CVertIndex &out
	)
{
	if ( iEdge == -1 )
		iEdge = GetEdgeIndexFromPoint( nodeIndex, pDisp->GetPower() );
	
	int iSub = GetSubNeighborIndex( pDisp, iEdge, nodeIndex );
	if ( iSub == -1 )
		return NULL;

	CDispUtilsHelper *pRet = TransformIntoSubNeighbor( pDisp, iEdge, iSub, nodeIndex, out );
	
#if 0
	// Debug check.. make sure it comes back to the same point from the other side.
	#if defined( _DEBUG )
		static bool bTesting = false;
		if ( pRet && !bTesting )
		{
			bTesting = true;

			// We could let TransformIntoNeighbor figure out the index but if this is a corner vert, then
			// it may pick the wrong edge and we'd get a benign assert.
			int nbOrientation = pDisp->GetEdgeNeighbor( iEdge )->m_SubNeighbors[iSub].GetNeighborOrientation();
			int iNeighborEdge = (iEdge + 2 + nbOrientation) & 3;

			CVertIndex testIndex;
			CDispUtilsHelper *pTest = TransformIntoNeighbor( pRet, iNeighborEdge, out, testIndex );
			Assert( pTest == pDisp );
			Assert( testIndex == nodeIndex );
		
			bTesting = false;
		}
	#endif
#endif
		
	return pRet;
}


bool DoesPointHaveAnyNeighbors( 
	CDispUtilsHelper *pDisp,
	const CVertIndex &index )
{
	// See if it connects to a neighbor on the edge.
	CVertIndex dummy;
	if ( TransformIntoNeighbor( pDisp, -1, index, dummy ) )
		return true;

	// See if it connects to a neighbor on a corner.
	int iCorner = GetCornerIndexFromPoint( index, pDisp->GetPower() );
	if ( iCorner == -1 )
		return false;

	// If there are any neighbors on the specified corner, then the point has neighbors.
	if ( pDisp->GetCornerNeighbors( iCorner )->m_nNeighbors > 0 )
		return true;

	// Since points on corners touch two edges, we actually want to test two edges to see
	// if the point has a neighbor on either edge.
	for ( int i=0; i < 2; i++ )
	{
		if ( TransformIntoNeighbor( pDisp, g_CornerEdges[iCorner][i], index, dummy ) )
			return true;
	}

	return false;
}


// ------------------------------------------------------------------------------------ //
// CDispSubEdgeIterator.
// ------------------------------------------------------------------------------------ //

CDispSubEdgeIterator::CDispSubEdgeIterator()
{
	m_pNeighbor = 0;
	m_FreeDim = m_Index.x = m_Inc.x = m_End = 0;	// Setup so Next returns false.
}


void CDispSubEdgeIterator::Start( CDispUtilsHelper *pDisp, int iEdge, int iSub, bool bTouchCorners )
{
	m_pNeighbor = SetupEdgeIncrements( pDisp, iEdge, iSub, m_Index, m_Inc, m_NBIndex, m_NBInc, m_End, m_FreeDim );
	if ( m_pNeighbor )
	{
		if ( bTouchCorners )
		{
			// Back up our current position by 1 so we hit the corner first, and extend the endpoint
			// so we hit the other corner too.
			m_Index -= m_Inc;
			m_NBIndex -= m_NBInc;

			m_End += m_Inc[m_FreeDim];
		}
	}
	else
	{
		m_FreeDim = m_Index.x = m_Inc.x = m_End = 0;	// Setup so Next returns false.
	}
}


bool CDispSubEdgeIterator::Next()
{
	m_Index += m_Inc;
	m_NBIndex += m_NBInc;

	// Were we just at the last point on the edge?
	return m_Index[m_FreeDim] < m_End;
}


bool CDispSubEdgeIterator::IsLastVert() const
{
	return (m_Index[m_FreeDim] + m_Inc[m_FreeDim]) >= m_End;
}


// ------------------------------------------------------------------------------------ //
// CDispEdgeIterator.
// ------------------------------------------------------------------------------------ //

CDispEdgeIterator::CDispEdgeIterator( CDispUtilsHelper *pDisp, int iEdge )
{
	m_pDisp = pDisp;
	m_iEdge = iEdge;
	m_iCurSub = -1;
}


bool CDispEdgeIterator::Next()
{
	while ( !m_It.Next() )
	{
		// Ok, move up to the next sub.
		if ( m_iCurSub == 1 )
			return false;
	
		++m_iCurSub;
		m_It.Start( m_pDisp, m_iEdge, m_iCurSub );
	}
	return true;
}


// ------------------------------------------------------------------------------------ //
// CDispCircumferenceIterator.
// ------------------------------------------------------------------------------------ //

CDispCircumferenceIterator::CDispCircumferenceIterator( int sideLength )
{
	m_iCurEdge = -1;
	m_SideLengthM1 = sideLength - 1;
}


bool CDispCircumferenceIterator::Next()
{
	switch ( m_iCurEdge )
	{
		case -1:
		{
			m_iCurEdge = NEIGHBOREDGE_LEFT;
			m_VertIndex.Init( 0, 0 );
		}
		break;

		case NEIGHBOREDGE_LEFT:
		{
			++m_VertIndex.y;
			if ( m_VertIndex.y == m_SideLengthM1 )
				m_iCurEdge = NEIGHBOREDGE_TOP;
		}
		break;

		case NEIGHBOREDGE_TOP:
		{
			++m_VertIndex.x;
			if ( m_VertIndex.x == m_SideLengthM1 )
				m_iCurEdge = NEIGHBOREDGE_RIGHT;
		}
		break;

		case NEIGHBOREDGE_RIGHT:
		{
			--m_VertIndex.y;
			if ( m_VertIndex.y == 0 )
				m_iCurEdge = NEIGHBOREDGE_BOTTOM;
		}
		break;

		case NEIGHBOREDGE_BOTTOM:
		{
			--m_VertIndex.x;
			if ( m_VertIndex.x == 0 )
				return false; // Done!
		}
		break;
	}

	return true;
}



// Helper function to setup an index either on the edges or the center
// of the box defined by [bottomleft,topRight].
static inline void SetupCoordXY( CNodeVert &out, CNodeVert const &bottomLeft, CNodeVert const &topRight, CNodeVert const &info )
{
	for( int i=0; i < 2; i++ )
	{
		if( info[i] == 0 )
			out[i] = bottomLeft[i];
		else if( info[i] == 1 )
			out[i] = (bottomLeft[i] + topRight[i]) >> 1;
		else
			out[i] = topRight[i];
	}
}


static unsigned short* DispCommon_GenerateTriIndices_R( 
	CNodeVert const &bottomLeft, 
	CNodeVert const &topRight,
	unsigned short *indices, 
	int power,
	int sideLength )
{
	if( power == 1 )
	{
		// Ok, add triangles. All we do here is follow a list of verts (g_NodeTriWinding)
		// around the center vert of this node and make triangles.
		int iCurTri = 0;
		CNodeVert verts[3];

		// verts[0] is always the center vert.
		SetupCoordXY( verts[0], bottomLeft, topRight, CNodeVert(1,1) );
		int iCurVert = 1;

		for( int i=0; i < 9; i++ )
		{
			SetupCoordXY( verts[iCurVert], bottomLeft, topRight, g_NodeTriWinding[i] );
			++iCurVert;

			if( iCurVert == 3 )
			{
				for( int iTriVert=2; iTriVert >= 0; iTriVert-- )
				{
					int index = verts[iTriVert].y * sideLength + verts[iTriVert].x;
					*indices = index;
					++indices;
				}

				// Setup for the next triangle.
				verts[1] = verts[2];
				iCurVert = 2;
				iCurTri++;
			}
		}
	}
	else
	{
		// Recurse into the children.
		for( int i=0; i < 4; i++ )
		{
			CNodeVert childBottomLeft, childTopRight;
			SetupCoordXY( childBottomLeft, bottomLeft, topRight, g_NodeChildLookup[i][0] );
			SetupCoordXY( childTopRight,   bottomLeft, topRight, g_NodeChildLookup[i][1] );

			indices = DispCommon_GenerateTriIndices_R( childBottomLeft, childTopRight, indices, power-1, sideLength );
		}
	}

	return indices;
}


// ------------------------------------------------------------------------------------------- //
// CDispUtilsHelper functions.
// ------------------------------------------------------------------------------------------- //
	
int CDispUtilsHelper::GetPower() const
{
	return GetPowerInfo()->GetPower();
}

int CDispUtilsHelper::GetSideLength() const
{
	return GetPowerInfo()->GetSideLength();
}

const CVertIndex& CDispUtilsHelper::GetCornerPointIndex( int iCorner ) const
{
	return GetPowerInfo()->GetCornerPointIndex( iCorner );
}

int CDispUtilsHelper::VertIndexToInt( const CVertIndex &i ) const
{
	Assert( i.x >= 0 && i.x < GetSideLength() && i.y >= 0 && i.y < GetSideLength() );
	return i.y * GetSideLength() + i.x;
}

CVertIndex CDispUtilsHelper::GetEdgeMidPoint( int iEdge ) const
{
	int end = GetSideLength() - 1;
	int mid = GetPowerInfo()->GetMidPoint();

	if ( iEdge == NEIGHBOREDGE_LEFT )
		return CVertIndex( 0, mid );
	
	else if ( iEdge == NEIGHBOREDGE_TOP )
		return CVertIndex( mid, end );

	else if ( iEdge == NEIGHBOREDGE_RIGHT )
		return CVertIndex( end, mid );
	
	else if ( iEdge == NEIGHBOREDGE_BOTTOM )
		return CVertIndex( mid, 0 );

	Assert( false );
	return CVertIndex( 0, 0 );
}

int DispCommon_GetNumTriIndices( int power )
{
	return (1<<power) * (1<<power) * 2 * 3;
}


void DispCommon_GenerateTriIndices( int power, unsigned short *indices )
{
	int sideLength = 1 << power;
	DispCommon_GenerateTriIndices_R(
		CNodeVert( 0, 0 ), 
		CNodeVert( sideLength, sideLength ),
		indices,
		power,
		sideLength+1 );
}


