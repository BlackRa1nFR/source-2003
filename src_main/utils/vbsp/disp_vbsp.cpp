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

#include "disp_vbsp.h"
#include "tier0/dbg.h"
#include "vbsp.h"
#include "mstristrip.h"
#include "disp_lightmap_alpha.h"
#include "writebsp.h"
#include "pacifier.h"


// BUGBUG: This is in common, but the tools use it?
// BUGBUG: I don't like this relative include, but I don't want to add the common directory to the tools.
#include "..\..\common\builddisp.h"


class CDispBox
{
public:
	Vector	m_Min, m_Max;
};



// map displacement info -- runs parallel to the dispinfos struct
int              nummapdispinfo = 0;
mapdispinfo_t    mapdispinfo[MAX_MAP_DISPINFO];

CUtlVector<CCoreDispInfo> g_CoreDispInfos;


// This table swaps MIDPOINT_TO_CORNER and CORNER_TO_MIDPOINT.
static NeighborSpan g_SpanFlip[3] = {CORNER_TO_CORNER, MIDPOINT_TO_CORNER, CORNER_TO_MIDPOINT};
static bool			g_bEdgeNeighborFlip[4] = {false, false, true, true};

// These map CCoreDispSurface neighbor orientations (which are actually edge indices)
// into our 'degrees of rotation' representation.
static int g_CoreDispNeighborOrientationMap[4][4] =
{
	{ORIENTATION_CCW_180, ORIENTATION_CCW_270, ORIENTATION_CCW_0,   ORIENTATION_CCW_90},
	{ORIENTATION_CCW_90,  ORIENTATION_CCW_180, ORIENTATION_CCW_270, ORIENTATION_CCW_0},
	{ORIENTATION_CCW_0,   ORIENTATION_CCW_90,  ORIENTATION_CCW_180, ORIENTATION_CCW_270},
	{ORIENTATION_CCW_270, ORIENTATION_CCW_0,   ORIENTATION_CCW_90,  ORIENTATION_CCW_180}
};



static NeighborSpan NeighborSpanFlip( int iEdge, NeighborSpan span )
{
	if ( g_bEdgeNeighborFlip[iEdge] )
		return g_SpanFlip[span];
	else
		return span;
}

static void AddNeighbor( 
	const CCoreDispInfo *pListBase,

	CCoreDispInfo *pMain,
	int iEdge,				// Which of pMain's sides this is on.
	int iSub,				// Which sub neighbor this takes up in pSide.
	NeighborSpan span,		// What span this fills in pMain.
	
	CCoreDispInfo *pOther,
	int iNeighborEdge,
	NeighborSpan nbSpan 
	)
{
	// The edge iteration before coming in here goes 0-1, 1-2, 2-3, 3-4.
	// This flips the sense of CORNER_TO_MIDPOINT/MIDPOINT_TO_CORNER on the right and 
	// bottom edges and is undone here.
	span = NeighborSpanFlip( iEdge, span );
	nbSpan = NeighborSpanFlip( iNeighborEdge, nbSpan );

	// Get the subspan this fills on our displacement.
	CDispSubNeighbor *pSub = &pMain->GetEdgeNeighbor(iEdge)->m_SubNeighbors[iSub];
	
	// Which subspan does this use in the neighbor?
	CDispSubNeighbor *pNeighborSub;
	if ( nbSpan == MIDPOINT_TO_CORNER )
		pNeighborSub = &pOther->GetEdgeNeighbor(iNeighborEdge)->m_SubNeighbors[1];
	else
		pNeighborSub = &pOther->GetEdgeNeighbor(iNeighborEdge)->m_SubNeighbors[0];

	// Make sure this slot isn't used on either displacement.
	if ( pSub->IsValid() || pNeighborSub->IsValid() )
	{
		ExecuteOnce( Warning( "Found a displacement edge abutting multiple other edges.\n" ) );
		return;
	}

	// Now just copy the data into each displacement.	
	pSub->m_iNeighbor = pOther - pListBase;
	pSub->m_NeighborOrientation = g_CoreDispNeighborOrientationMap[iEdge][iNeighborEdge];
	pSub->m_Span = span;
	pSub->m_NeighborSpan = nbSpan;

	pNeighborSub->m_iNeighbor = pMain - pListBase;
	pNeighborSub->m_NeighborOrientation = g_CoreDispNeighborOrientationMap[iNeighborEdge][iEdge];
	pNeighborSub->m_Span = nbSpan;
	pNeighborSub->m_NeighborSpan = span;

#if defined( _DEBUG )
	// Walk an iterator over the new connection to make sure it works.
	CDispSubEdgeIterator it;
	it.Start( pMain, iEdge, iSub );
	while ( it.Next() )
	{
		CVertIndex nbIndex;
		TransformIntoNeighbor( pMain, iEdge, it.GetVertIndex(), nbIndex );
	}
#endif
}


static bool FindEdge( 
	CCoreDispInfo *pInfo,
	Vector const &vPoint1,
	Vector const &vPoint2,
	int &iEdge 
	)
{
	CCoreDispSurface *pSurface = pInfo->GetSurface();

	for( iEdge=0; iEdge < 4; iEdge++ )
	{
		if( VectorsAreEqual( vPoint1, pSurface->GetPoint( iEdge ), 0.01f ) &&
			VectorsAreEqual( vPoint2, pSurface->GetPoint( (iEdge+1) & 3), 0.01f ) )
		{
			return true;
		}
	}
	
	return false;
}


// This function is symmetric wrt pMain and pOther. It sets up valid neighboring data for
// the relationship between both of them.
static void SetupEdgeNeighbors( 
	const CCoreDispInfo *pListBase,
	CCoreDispInfo *pMain, 
	CCoreDispInfo *pOther )
{
	// Initialize..
	for( int iEdge=0; iEdge < 4; iEdge++ )
	{
		// Setup the edge points and the midpoint.
		Vector pt[2], mid;
		pMain->GetSurface()->GetPoint( iEdge, pt[0] );
		pMain->GetSurface()->GetPoint( (iEdge + 1) & 3, pt[1] );
		mid = (pt[0] + pt[1]) * 0.5f;

		// Find neighbors.
		int iNBEdge;
		if( FindEdge( pOther, pt[1], pt[0], iNBEdge ) )
		{
			AddNeighbor( 
				pListBase, 
				pMain, iEdge, 0, CORNER_TO_CORNER, 
				pOther, iNBEdge, CORNER_TO_CORNER );
		}
		else
		{
			// Look for one that takes up our whole side.
			if( FindEdge( pOther, pt[1], pt[0]*2 - pt[1], iNBEdge ) )
			{
				AddNeighbor( 
					pListBase,
					pMain, iEdge, 0, CORNER_TO_CORNER, 
					pOther, iNBEdge, CORNER_TO_MIDPOINT );
			}
			else if( FindEdge( pOther, pt[1]*2 - pt[0], pt[0], iNBEdge ) )
			{
				AddNeighbor( 
					pListBase,
					pMain, iEdge, 0, CORNER_TO_CORNER, 
					pOther, iNBEdge, MIDPOINT_TO_CORNER );
			}
			else
			{			
				// Ok, look for 1 or two that abut this side.
				if( FindEdge( pOther, mid, pt[0], iNBEdge ) )
				{
					AddNeighbor( 
						pListBase,
						pMain, iEdge, g_bEdgeNeighborFlip[iEdge], CORNER_TO_MIDPOINT, 
						pOther, iNBEdge, CORNER_TO_CORNER );
				}
				
				if( FindEdge( pOther, pt[1], mid, iNBEdge ) )
				{
					AddNeighbor( 
						pListBase,
						pMain, iEdge, !g_bEdgeNeighborFlip[iEdge], MIDPOINT_TO_CORNER, 
						pOther, iNBEdge, CORNER_TO_CORNER );
				}
			}
		}
	}
}


// Returns true if the displacement has an edge neighbor with the given index.
static bool HasEdgeNeighbor( const CCoreDispInfo *pMain, int iNeighbor )
{
	for ( int i=0; i < 4; i++ )
	{
		const CDispCornerNeighbors *pCorner = pMain->GetCornerNeighbors( i );
		for ( int iNB=0; iNB < pCorner->m_nNeighbors; iNB++ )
			if ( pCorner->m_Neighbors[iNB] == iNeighbor )
				return true;

		const CDispNeighbor *pEdge = pMain->GetEdgeNeighbor( i );
		if ( pEdge->m_SubNeighbors[0].GetNeighborIndex() == iNeighbor || 
			pEdge->m_SubNeighbors[1].GetNeighborIndex() == iNeighbor )
		{
			return true;
		}
	}
	return false;
}


static void SetupCornerNeighbors( 
	const CCoreDispInfo *pListBase, 
	CCoreDispInfo *pMain, 
	CCoreDispInfo *pOther,
	int *nOverflows )
{
	if ( HasEdgeNeighbor( pMain, pOther-pListBase ) )
		return;

	// Do these two share a vertex?
	int nShared = 0;
	int iMainSharedCorner = -1;
	int iOtherSharedCorner = -1;

	for ( int iMainCorner=0; iMainCorner < 4; iMainCorner++ )
	{
		Vector const &vMainCorner = pMain->GetCornerPoint( iMainCorner );
		
		for ( int iOtherCorner=0; iOtherCorner < 4; iOtherCorner++ )
		{
			Vector const &vOtherCorner = pOther->GetCornerPoint( iOtherCorner );
		
			if ( VectorsAreEqual( vMainCorner, vOtherCorner, 0.001f ) )
			{
				iMainSharedCorner = iMainCorner;
				iOtherSharedCorner = iOtherCorner;
				++nShared;
			}
		}
	}

	if ( nShared == 1 )
	{
		CDispCornerNeighbors *pMainCorner = pMain->GetCornerNeighbors( iMainSharedCorner );
		CDispCornerNeighbors *pOtherCorner = pOther->GetCornerNeighbors( iOtherSharedCorner );

		if ( pMainCorner->m_nNeighbors < MAX_DISP_CORNER_NEIGHBORS &&
			pOtherCorner->m_nNeighbors < MAX_DISP_CORNER_NEIGHBORS )
		{
			pMainCorner->m_Neighbors[pMainCorner->m_nNeighbors++] = pOther - pListBase;
			pOtherCorner->m_Neighbors[pOtherCorner->m_nNeighbors++] = pMain - pListBase;
		}
		else
		{
			++(*nOverflows);
		}
	}
}


static void ClearNeighborData( CCoreDispInfo *pDisp )
{
	for ( int i=0; i < 4; i++ )
	{
		pDisp->GetEdgeNeighbor( i )->SetInvalid();
		pDisp->GetCornerNeighbors( i )->SetInvalid();
	}
}


bool VerifyNeighborVertConnection( 
	CDispUtilsHelper *pDisp, 
	const CVertIndex &nodeIndex, 
	const CDispUtilsHelper *pTestNeighbor,
	const CVertIndex &testNeighborIndex,
	int mySide )
{
	CVertIndex nbIndex;
	CDispUtilsHelper *pNeighbor;
	if( pNeighbor = TransformIntoNeighbor( pDisp, mySide, nodeIndex, nbIndex ) )
	{
		if ( pTestNeighbor != pNeighbor || nbIndex != testNeighborIndex )
			return false;

		CVertIndex testIndex;
		int iSide = GetEdgeIndexFromPoint( nbIndex, pNeighbor->GetPowerInfo()->m_Power );
		if ( iSide == -1 )
		{
			return false;
		}

		CDispUtilsHelper *pTest = TransformIntoNeighbor( pNeighbor, iSide, nbIndex, testIndex );
		
		if( pTest != pDisp || nodeIndex != testIndex )
		{
			return false;
		}
	}

	return true;
}


static void VerifyNeighborConnections( CCoreDispInfo *pListBase, int nDisps )
{
	while ( 1 )
	{
		bool bHappy = true;

		for ( int i=0; i < nDisps; i++ )
		{
			CCoreDispInfo *pDisp = &pListBase[i];
			CDispUtilsHelper *pHelper = pDisp;

			for ( int iEdge=0; iEdge < 4; iEdge++ )
			{
				CDispEdgeIterator it( pHelper, iEdge );
				while ( it.Next() )
				{
					if ( !VerifyNeighborVertConnection( pHelper, it.GetVertIndex(), it.GetCurrentNeighbor(), it.GetNBVertIndex(), iEdge ) )
					{
						pDisp->GetEdgeNeighbor( iEdge )->SetInvalid();
						Warning( "Warning: invalid neighbor connection on displacement near (%.2f %.2f %.2f)\n", VectorExpand( pDisp->GetCornerPoint(0) ) );
						bHappy = false;
					}
				}			
			}
		}

		if ( bHappy )
			break;
	}
}


void SetupDispBoxes( const CCoreDispInfo *pListBase, int listSize, CUtlVector<CDispBox> &out )
{
	out.SetSize( listSize );
	for ( int i=0; i < listSize; i++ )
	{
		const CCoreDispInfo *pDisp = &pListBase[i];

		// Calculate the bbox for this displacement.
		Vector vMin(  1e24,  1e24,  1e24 );
		Vector vMax( -1e24, -1e24, -1e24 );

		for ( int iVert=0; iVert < 4; iVert++ )
		{
			const Vector &vTest = pDisp->GetSurface()->GetPoint( iVert );
			VectorMin( vTest, vMin, vMin );
			VectorMax( vTest, vMax, vMax );
		}		

		// Puff the box out a little.
		static float flPuff = 0.1f;
		vMin -= Vector( flPuff, flPuff, flPuff );
		vMax += Vector( flPuff, flPuff, flPuff );

		out[i].m_Min = vMin;
		out[i].m_Max = vMax;
	}
}


//-----------------------------------------------------------------------------
// Computes the bounds for a disp info
//-----------------------------------------------------------------------------
void ComputeDispInfoBounds( int dispinfo, Vector& mins, Vector& maxs )
{
	CUtlVector<CDispBox> box;

	// Get a CCoreDispInfo. All we need is the triangles and lightmap texture coordinates.
	mapdispinfo_t *pMapDisp = &mapdispinfo[ dispinfo ];

	CCoreDispInfo coreDispInfo;
	DispMapToCoreDispInfo( pMapDisp, &coreDispInfo, NULL );

	SetupDispBoxes( &coreDispInfo, 1, box );	
	mins = box[0].m_Min;
	maxs = box[0].m_Max;
}


static inline bool DoBBoxesTouch( const CDispBox &a, const CDispBox &b )
{
	for ( int i=0; i < 3; i++ )
	{
		if ( a.m_Max[i] < b.m_Min[i] )
			return false;

		if ( a.m_Min[i] > b.m_Max[i] )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void FindNeighboringDispSurfs( CCoreDispInfo *pListBase, int listSize )
{
	// First, clear all neighboring data.
	for ( int i=0; i < listSize; i++ )
	{
		ClearNeighborData( &pListBase[i] );
	}

	CUtlVector<CDispBox> boxes;
	SetupDispBoxes( pListBase, listSize, boxes );	

	int nCornerOverflows = 0;

	// Now test all pairs of displacements and setup neighboring relations between them.
	for( i=0; i < listSize; i++ )
	{
		CCoreDispInfo *pMain = &pListBase[i];

		for ( int j=i+1; j < listSize; j++ )
		{
			CCoreDispInfo *pOther = &pListBase[j];

			// Trivial reject.
			if ( !DoBBoxesTouch( boxes[i], boxes[j] ) )
				continue;

			SetupEdgeNeighbors( pListBase, pMain, pOther );
			
			// NOTE: this must come after SetupEdgeNeighbors because it makes sure not to add
			// corner neighbors for disps that are already edge neighbors.
			SetupCornerNeighbors( pListBase, pMain, pOther, &nCornerOverflows );
		}
	}

	if ( nCornerOverflows )
	{
		Warning( "Warning: overflowed %d displacement corner-neighbor lists.", nCornerOverflows );
	}

	// Debug check.. make sure the neighbor connections are intact (make sure that any
	// edge vert that gets mapped into a neighbor gets mapped back the same way).
	VerifyNeighborConnections( pListBase, listSize );
}


static int IsCorner( 
	CVertIndex const &index,
	int sideLength )
{
	if( index.x == 0 )
	{
		if( index.y == 0 )
			return true;
		else if( index.y == sideLength-1 )
			return true;
	}
	else if( index.x == sideLength-1 )
	{
		if( index.y == 0 )
			return true;
		else if( index.y == sideLength-1 )
			return true;
	}
	
	return false;
}


static bool IsVertAllowed( CDispUtilsHelper *pDisp, CVertIndex const &sideVert, int iLevel )
{
	if( IsCorner( sideVert, pDisp->GetPowerInfo()->GetSideLength() ) )
		return true;

	int iSide = GetEdgeIndexFromPoint( sideVert, pDisp->GetPowerInfo()->GetPower() );
	if( iSide == -1 )
		return true;

	int iSub = GetSubNeighborIndex( pDisp, iSide, sideVert );
	if ( iSub == -1 )
		return true;

	CDispSubNeighbor *pSub = &pDisp->GetEdgeNeighbor( iSide )->m_SubNeighbors[iSub];
	CDispUtilsHelper *pNeighbor = pDisp->GetDispUtilsByIndex( pSub->m_iNeighbor );
	Assert( pNeighbor );

	// Ok, there is a neighbor.. see if this vertex exists in the neighbor.
	CShiftInfo *pShiftInfo = &g_ShiftInfos[pSub->m_Span][pSub->m_NeighborSpan];
	Assert( pShiftInfo->m_bValid );

	if( (pNeighbor->GetPowerInfo()->GetPower() + pShiftInfo->m_PowerShiftAdd) < (iLevel+1) )
	{
		return false;
	}

	// Ok, it exists. Make sure the neighbor hasn't disallowed it.
	CVertIndex nbIndex;
	TransformIntoSubNeighbor(
		pDisp,
		iSide,
		iSub,
		sideVert,
		nbIndex );
	
	CBitVec<MAX_DISPVERTS> &allowedVerts = CCoreDispInfo::FromDispUtils( pNeighbor )->GetAllowedVerts();
	return !!allowedVerts.Get( pNeighbor->VertIndexToInt( nbIndex ) );
}


static void UnallowVerts_R( 
	CDispUtilsHelper *pDisp,
	CVertIndex const &nodeIndex,
	int &nUnallowed )
{
	int iNodeIndex = pDisp->VertIndexToInt( nodeIndex );
	
	CCoreDispInfo *pCoreDisp = CCoreDispInfo::FromDispUtils( pDisp );	
	if( !pCoreDisp->GetAllowedVerts().Get( iNodeIndex ) )
		return;

	nUnallowed++;
	pCoreDisp->GetAllowedVerts().Clear( iNodeIndex );

	for( int iDep=0; iDep < CVertInfo::NUM_REVERSE_DEPENDENCIES; iDep++ )
	{
		CVertDependency &dep = pDisp->GetPowerInfo()->m_pVertInfo[iNodeIndex].m_ReverseDependencies[iDep];

		if( dep.m_iVert.x != -1 && dep.m_iNeighbor == -1 )
		{
			UnallowVerts_R( pDisp, dep.m_iVert, nUnallowed );
		}
	}
}


static void DisableUnallowedVerts_R( 
	CDispUtilsHelper *pDisp,
	CVertIndex const &nodeIndex,
	int iLevel,
	int &nUnallowed )
{
	int iNodeIndex = pDisp->VertIndexToInt( nodeIndex );

	// This vertex is not allowed if it is on an edge with a neighbor
	// that does not have this vertex.

	// Test side verts.
	for( int iSide=0; iSide < 4; iSide++ )
	{
		CVertIndex const &sideVert = pDisp->GetPowerInfo()->m_pSideVerts[iNodeIndex].m_Verts[iSide];

		if( !IsVertAllowed( pDisp, sideVert, iLevel ) )
		{
			// This vert (and its dependencies) can't exist.
			UnallowVerts_R( pDisp, sideVert, nUnallowed );
		}
	}

	// Test dependencies.
	for( int iDep=0; iDep < 2; iDep++ )
	{
		CVertDependency const &dep = pDisp->GetPowerInfo()->m_pVertInfo[iNodeIndex].m_Dependencies[iDep];

		if( dep.m_iNeighbor == -1 && !IsVertAllowed( pDisp, dep.m_iVert, iLevel ) )
		{
			UnallowVerts_R( pDisp, nodeIndex, nUnallowed );
		}
	}

	// Recurse.
	if( iLevel+1 < pDisp->GetPower() )
	{
		for( int iChild=0; iChild < 4; iChild++ )
		{
			DisableUnallowedVerts_R(
				pDisp,
				pDisp->GetPowerInfo()->m_pChildVerts[iNodeIndex].m_Verts[iChild],
				iLevel+1,
				nUnallowed );
		}
	}
}


void SetupAllowedVerts( CCoreDispInfo *pListBase, int listSize )
{
	// Set all verts to allowed to start with.
	for ( int i=0; i < listSize; i++ )
	{
		pListBase[i].GetAllowedVerts().SetAll();
	}

	// Disable verts that need to be disabled so higher-powered displacements remove
	// the necessary triangles when bordering lower-powered displacements.	
	// It is necessary to loop around here because disabling verts can accumulate into 
	// neighbors.
	bool bContinue;
	do
	{
		bContinue = false;
		for( i=0; i < listSize; i++ )
		{
			CDispUtilsHelper *pDisp = &pListBase[i];
			
			int nUnallowed = 0;
			DisableUnallowedVerts_R( pDisp, pDisp->GetPowerInfo()->m_RootNode, 0, nUnallowed );
			if ( nUnallowed )
				bContinue = true;
		}
	} while( bContinue );
}


// Gets the barycentric coordinates of the position on the triangle where the lightmap
// coordinates are equal to lmCoords. This always generates the coordinates but it
// returns false if the point containing them does not lie inside the triangle.
bool GetBarycentricCoordsFromLightmapCoords( 
	Vector2D tri[3],
	Vector2D const &lmCoords,
	float bcCoords[3] )
{
	GetBarycentricCoords2D( tri[0], tri[1], tri[2], lmCoords, bcCoords );

	return 
		(bcCoords[0] >= 0.0f && bcCoords[0] <= 1.0f) && 
		(bcCoords[1] >= 0.0f && bcCoords[1] <= 1.0f) && 
		(bcCoords[2] >= 0.0f && bcCoords[2] <= 1.0f);
}


bool FindTriIndexMapByUV( CCoreDispInfo *pCoreDisp, Vector2D const &lmCoords,
						  int &iTriangle, float flBarycentric[3] )
{
	const CPowerInfo *pPowerInfo = GetPowerInfo( pCoreDisp->GetPower() );

	// Search all the triangles..
	int nTriCount= pCoreDisp->GetTriCount();
	for ( int iTri = 0; iTri < nTriCount; ++iTri )
	{
		unsigned short iVerts[3];
		pCoreDisp->GetTriIndices( iTri, iVerts[0], iVerts[1], iVerts[2] );

		// Get this triangle's UVs.
		Vector2D vecUV[3];
		for ( int iCoord = 0; iCoord < 3; ++iCoord )
		{
			pCoreDisp->GetLuxelCoord( 0, iVerts[iCoord], vecUV[iCoord] );
		}

		// See if the passed-in UVs are in this triangle's UVs.
		if( GetBarycentricCoordsFromLightmapCoords( vecUV, lmCoords, flBarycentric ) )
		{
			iTriangle = iTri;
			return true;
		}
	}		

	return false;
}


void CalculateLightmapSamplePositions( 
	CCoreDispInfo *pCoreDispInfo, 
	const dface_t *pFace,
	CUtlVector<unsigned char> &out )
{
	int width  = pFace->m_LightmapTextureSizeInLuxels[0] + 1;
	int height = pFace->m_LightmapTextureSizeInLuxels[1] + 1;

	// For each lightmap sample, find the triangle it sits in.
	Vector2D lmCoords;
	for( int y=0; y < height; y++ )
	{
		lmCoords.y = y;

		for( int x=0; x < width; x++ )
		{
			lmCoords.x = x;

			float flBarycentric[3];
			int iTri;

			if( FindTriIndexMapByUV( pCoreDispInfo, lmCoords, iTri, flBarycentric ) )
			{
				if( iTri < 255 )
				{
					out.AddToTail( iTri );
				}
				else
				{
					out.AddToTail( 255 );
					out.AddToTail( iTri - 255 );
				}

				out.AddToTail( (unsigned char)( flBarycentric[0] * 255.9f ) );
				out.AddToTail( (unsigned char)( flBarycentric[1] * 255.9f ) );
				out.AddToTail( (unsigned char)( flBarycentric[2] * 255.9f ) );
			}
			else
			{
				out.AddToTail( 0 );
				out.AddToTail( 0 );
				out.AddToTail( 0 );
				out.AddToTail( 0 );
			}
		}
	}
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetDispInfoEntityNum( mapdispinfo_t *pDisp )
{
	return pDisp->entitynum;
}


// Setup a CCoreDispInfo given a mapdispinfo_t.
// If pFace is non-NULL, then lightmap texture coordinates will be generated.
void DispMapToCoreDispInfo( 
	mapdispinfo_t *pMapDisp,
	CCoreDispInfo *pCoreDispInfo,
	dface_t *pFace )
{
	winding_t *pWinding = pMapDisp->face.originalface->winding;

	Assert( pWinding->numpoints == 4 );

	//
	// set initial surface data
	//
	CCoreDispSurface *pSurf = pCoreDispInfo->GetSurface();

	texinfo_t *pTexInfo = &texinfo[ pMapDisp->face.texinfo ];

	// init material contents
	pMapDisp->contents = textureref[pTexInfo->texdata].contents;
	if (!(pMapDisp->contents & (ALL_VISIBLE_CONTENTS | CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) ) )
	{
		pMapDisp->contents |= CONTENTS_SOLID;
	}

	pSurf->SetContents( pMapDisp->contents );

	// Calculate the lightmap coordinates.
	Vector2D lmCoords[4] = {Vector2D(0,0),Vector2D(0,1),Vector2D(1,0),Vector2D(1,1)};
	Vector2D tCoords[4] = {Vector2D(0,0),Vector2D(0,1),Vector2D(1,0),Vector2D(1,1)};
	if( pFace )
	{
		Assert( pFace->numedges == 4 );

		Vector pt[4];
		for( int i=0; i < 4; i++ )
			pt[i] = pWinding->p[i];

		CalcTextureCoordsAtPoints( 
			pTexInfo->lightmapVecsLuxelsPerWorldUnits,
			pFace->m_LightmapTextureMinsInLuxels,
			pt,
			4,
			lmCoords );

		int zeroOffset[2] = {0,0};
		CalcTextureCoordsAtPoints( 
			pTexInfo->textureVecsTexelsPerWorldUnits,
			zeroOffset,
			pt,
			4,
			tCoords );
	}
	
	//
	// set face point data ...
	//
	pSurf->SetPointCount( 4 );
	for( int i = 0; i < 4; i++ )
	{
		// position
		pSurf->SetPoint( i, pWinding->p[i] );
		for( int j = 0; j < ( NUM_BUMP_VECTS + 1 ); ++j )
		{
			pSurf->SetLuxelCoord( j, i, lmCoords[i] );
		}
		pSurf->SetTexCoord( i, tCoords[i] );
	}
	
	//
	// reset surface given start info
	//
	pSurf->SetPointStart( pMapDisp->startPosition );
	pSurf->FindSurfPointStartIndex();
	pSurf->AdjustSurfPointData();

	//
	// adjust face lightmap data - this will be done a bit more accurately
	// when the common code get written, for now it works!!! (GDC, E3)
	//
	Vector points[4];
	for( int ndxPt = 0; ndxPt < 4; ndxPt++ )
	{
		points[ndxPt] = pSurf->GetPoint( ndxPt );
	}
	Vector edgeU = points[3] - points[0];
	Vector edgeV = points[1] - points[0];
	bool bUMajor = ( edgeU.Length() > edgeV.Length() );

	if( pFace )
	{
		int lightmapWidth = pFace->m_LightmapTextureSizeInLuxels[0];
		int lightmapHeight = pFace->m_LightmapTextureSizeInLuxels[1];
		if ( ( bUMajor && ( lightmapHeight > lightmapWidth ) ) || 
			 ( !bUMajor && ( lightmapWidth > lightmapHeight ) ) )
		{
			pFace->m_LightmapTextureSizeInLuxels[0] = lightmapHeight;
			pFace->m_LightmapTextureSizeInLuxels[1] = lightmapWidth;

			lightmapWidth = lightmapHeight;
			lightmapHeight = pFace->m_LightmapTextureSizeInLuxels[1];
		}

		for ( int ndxBump = 0; ndxBump < ( NUM_BUMP_VECTS + 1 ); ndxBump++ )
		{
			pSurf->SetLuxelCoord( ndxBump, 0, Vector2D( 0.0f, 0.0f ) );
			pSurf->SetLuxelCoord( ndxBump, 1, Vector2D( 0.0f, ( float )lightmapHeight ) );
			pSurf->SetLuxelCoord( ndxBump, 2, Vector2D( ( float )lightmapWidth, ( float )lightmapHeight ) );
			pSurf->SetLuxelCoord( ndxBump, 3, Vector2D( ( float )lightmapWidth, 0.0f ) );
		}
	}

	// Setup the displacement vectors and offsets.
	int size = ( ( ( 1 << pMapDisp->power ) + 1 ) * ( ( 1 << pMapDisp->power ) + 1 ) );

	Vector vectorDisps[2048];
	float dispDists[2048];
	Assert( size < sizeof(vectorDisps)/sizeof(vectorDisps[0]) );

	for( int j = 0; j < size; j++ )
	{
		Vector v;
		float dist;

		VectorScale( pMapDisp->vectorDisps[j], pMapDisp->dispDists[j], v );
		VectorAdd( v, pMapDisp->vectorOffsets[j], v );
		
		dist = VectorLength( v );
		VectorNormalize( v );
		
		vectorDisps[j] = v;
		dispDists[j] = dist;
	}


	// Use CCoreDispInfo to setup the actual vertex positions.
	pCoreDispInfo->InitDispInfo( pMapDisp->power, pMapDisp->minTess, pMapDisp->smoothingAngle,
						 pMapDisp->alphaValues, vectorDisps, dispDists );
	pCoreDispInfo->Create();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void EmitInitialDispInfos( void )
{
	int					i;
	mapdispinfo_t		*pMapDisp;
	ddispinfo_t			*pDisp;
	Vector				v;

	
	// Calculate the total number of verts.
	int nTotalVerts = 0;
	int nTotalTris = 0;
	for ( i=0; i < nummapdispinfo; i++ )
	{
		nTotalVerts += NUM_DISP_POWER_VERTS( mapdispinfo[i].power );
		nTotalTris += NUM_DISP_POWER_TRIS( mapdispinfo[i].power );
	}

	// Clear the output arrays..
	g_dispinfo.Purge();
	g_dispinfo.SetSize( nummapdispinfo );
	g_DispVerts.SetSize( nTotalVerts );
	g_DispTris.SetSize( nTotalTris );

	int iCurVert = 0;
	int iCurTri = 0;
	for( i = 0; i < nummapdispinfo; i++ )
	{
		pDisp = &g_dispinfo[i];
		pMapDisp = &mapdispinfo[i];
		
		CDispVert *pOutVerts = &g_DispVerts[iCurVert];
		CDispTri *pOutTris = &g_DispTris[iCurTri];

		// Setup the vert pointers.
		pDisp->m_iDispVertStart = iCurVert;
		pDisp->m_iDispTriStart = iCurTri;
		iCurVert += NUM_DISP_POWER_VERTS( pMapDisp->power );
		iCurTri += NUM_DISP_POWER_TRIS( pMapDisp->power );

		//
		// save power, minimum tesselation, and smoothing angle
		//
		pDisp->power = pMapDisp->power;
		pDisp->minTess = pMapDisp->minTess;
		pDisp->smoothingAngle = pMapDisp->smoothingAngle;
		pDisp->m_iMapFace = -2;

		// get surface contents
		pDisp->contents = pMapDisp->face.contents;
		
		pDisp->startPosition = pMapDisp->startPosition;
		//
		// add up the vectorOffsets and displacements, save alphas (per vertex)
		//
		int size = ( ( ( 1 << pDisp->power ) + 1 ) * ( ( 1 << pDisp->power ) + 1 ) );
		for( int j = 0; j < size; j++ )
		{
			VectorScale( pMapDisp->vectorDisps[j], pMapDisp->dispDists[j], v );
			VectorAdd( v, pMapDisp->vectorOffsets[j], v );
			
			float dist = VectorLength( v );
			VectorNormalize( v );
			
			VectorCopy( v, pOutVerts[j].m_vVector );
			pOutVerts[j].m_flDist = dist;

			pOutVerts[j].m_flAlpha = pMapDisp->alphaValues[j];
		}

		int nTriCount = ( (1 << (pDisp->power)) * (1 << (pDisp->power)) * 2 );
		for ( int iTri = 0; iTri< nTriCount; ++iTri )
		{
			pOutTris[iTri].m_uiTags = pMapDisp->triTags[iTri];
		}
		//===================================================================
		//===================================================================

		// save the index for face data reference
		pMapDisp->face.dispinfo = i;
	}
}


void ExportCoreDispNeighborData( const CCoreDispInfo *pIn, ddispinfo_t *pOut )
{
	for ( int i=0; i < 4; i++ )
	{
		pOut->m_EdgeNeighbors[i] = *pIn->GetEdgeNeighbor( i );
		pOut->m_CornerNeighbors[i] = *pIn->GetCornerNeighbors( i );
	}
}

void ExportNeighborData( CCoreDispInfo *pListBase, ddispinfo_t *pBSPDispInfos, int listSize )
{
	FindNeighboringDispSurfs( pListBase, listSize );

	// Export the neighbor data.
	for ( int i=0; i < nummapdispinfo; i++ )
	{
		ExportCoreDispNeighborData( &g_CoreDispInfos[i], &pBSPDispInfos[i] );
	}
}


void ExportCoreDispAllowedVertList( const CCoreDispInfo *pIn, ddispinfo_t *pOut )
{
	ErrorIfNot( 
		pIn->GetAllowedVerts().GetNumDWords() == sizeof( pOut->m_AllowedVerts ) / 4,
		("ExportCoreDispAllowedVertList: size mismatch")
		);
	for ( int i=0; i < pIn->GetAllowedVerts().GetNumDWords(); i++ )
		pOut->m_AllowedVerts[i] = pIn->GetAllowedVerts().GetDWord( i );
}


void ExportAllowedVertLists( CCoreDispInfo *pListBase, ddispinfo_t *pBSPDispInfos, int listSize )
{
	SetupAllowedVerts( pListBase, listSize );

	for ( int i=0; i < listSize; i++ )
	{
		ExportCoreDispAllowedVertList( &pListBase[i], &pBSPDispInfos[i] );
	}
}


void EmitDispLMAlphaAndNeighbors()
{
	int i;

	Msg( "Finding displacement neighbors...\n" );

	// Do lightmap alpha.
    g_DispLightmapAlpha.RemoveAll();

	// Build the CCoreDispInfos.
	CUtlVector<dface_t*> faces;

	// Create the core dispinfos and init them for use as CDispUtilsHelpers.
	g_CoreDispInfos.SetSize( nummapdispinfo );
	for ( i=0; i < nummapdispinfo; i++ )
	{
		g_CoreDispInfos[i].SetDispUtilsHelperInfo( g_CoreDispInfos.Base(), nummapdispinfo );
	}

	faces.SetSize( nummapdispinfo );

	for( i = 0; i < numfaces; i++ )
	{
        dface_t *pFace = &dfaces[i];

		if( pFace->dispinfo == -1 )
			continue;

		mapdispinfo_t *pMapDisp = &mapdispinfo[pFace->dispinfo];
		
		// Set the displacement's face index.
		ddispinfo_t *pDisp = &g_dispinfo[pFace->dispinfo];
		pDisp->m_iMapFace = i;

		// Get a CCoreDispInfo. All we need is the triangles and lightmap texture coordinates.
		CCoreDispInfo *pCoreDispInfo = &g_CoreDispInfos[pFace->dispinfo];
		DispMapToCoreDispInfo( pMapDisp, pCoreDispInfo, pFace );
		
		faces[pFace->dispinfo] = pFace;
	}

	
	// Generate and export neighbor data.
	ExportNeighborData( g_CoreDispInfos.Base(), g_dispinfo.Base(), nummapdispinfo );

	// Generate and export the active vert lists.
	ExportAllowedVertLists( g_CoreDispInfos.Base(), g_dispinfo.Base(), nummapdispinfo );

	Msg( "Finding lightmap sample positions...\n" );
	for ( i=0; i < nummapdispinfo; i++ )
	{
		dface_t *pFace = faces[i];
		ddispinfo_t *pDisp = &g_dispinfo[pFace->dispinfo];
		CCoreDispInfo *pCoreDispInfo = &g_CoreDispInfos[i];

		pDisp->m_iLightmapSamplePositionStart = g_DispLightmapSamplePositions.Count();

		CalculateLightmapSamplePositions( 
			pCoreDispInfo, 
			pFace,
			g_DispLightmapSamplePositions );
	}


	StartPacifier( "Displacement Alpha : ");

	// Build lightmap alphas.
	int dispCount = 0;	// How many we've processed.
	for( i = 0; i < nummapdispinfo; i++ )
	{
        dface_t *pFace = faces[i];

		Assert( pFace->dispinfo == i );
		mapdispinfo_t *pMapDisp = &mapdispinfo[pFace->dispinfo];
		ddispinfo_t *pDisp = &g_dispinfo[pFace->dispinfo];

		CCoreDispInfo *pCoreDispInfo = &g_CoreDispInfos[i];

		// Allocate space for the alpha values.
		pDisp->m_iLightmapAlphaStart = g_DispLightmapAlpha.Count();
		int nLuxelsToAdd = (pFace->m_LightmapTextureSizeInLuxels[0]+1) * (pFace->m_LightmapTextureSizeInLuxels[1]+1);
		g_DispLightmapAlpha.AddMultipleToTail( nLuxelsToAdd );

		DispUpdateLightmapAlpha( 
			g_CoreDispInfos.Base(),
			i,
			(float)dispCount     / g_dispinfo.Count(),
			(float)(dispCount+1) / g_dispinfo.Count(),
			pDisp, 
			pFace->m_LightmapTextureSizeInLuxels[0],
			pFace->m_LightmapTextureSizeInLuxels[1] );
	
		++dispCount;
	}

	EndPacifier();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DispGetFaceInfo( mapbrush_t *pBrush )
{
	int		i;
	side_t	*pSide;

	// we don't support displacement on entities at the moment!!
	if( pBrush->entitynum != 0 )
	{
		char* pszEntityName = ValueForKey( &entities[pBrush->entitynum], "classname" );
		Error( "Error: displacement found on a(n) %s entity - not supported\n", pszEntityName );
	}

	for( i = 0; i < pBrush->numsides; i++ )
	{
		pSide = &pBrush->original_sides[i];
		if( pSide->pMapDisp )
		{
			// error checking!!
			if( pSide->winding->numpoints != 4 )
				Error( "Trying to create a non-quad displacement!\n" );

			pSide->pMapDisp->face.originalface = pSide;
			pSide->pMapDisp->face.texinfo = pSide->texinfo;
			pSide->pMapDisp->face.dispinfo = -1;
			pSide->pMapDisp->face.planenum = pSide->planenum;
			pSide->pMapDisp->face.numpoints = pSide->winding->numpoints;
			pSide->pMapDisp->face.w = CopyWinding( pSide->winding );
			pSide->pMapDisp->face.contents = pBrush->contents;

			pSide->pMapDisp->face.merged = FALSE;
			pSide->pMapDisp->face.split[0] = FALSE;
			pSide->pMapDisp->face.split[1] = FALSE;

			pSide->pMapDisp->entitynum = pBrush->entitynum;
			pSide->pMapDisp->brushSideID = pSide->id;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool HasDispInfo( mapbrush_t *pBrush )
{
	int		i;
	side_t	*pSide;

	for( i = 0; i < pBrush->numsides; i++ )
	{
		pSide = &pBrush->original_sides[i];
		if( pSide->pMapDisp )
			return true;
	}

	return false;
}
