//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <assert.h>
#include <float.h>
#include "cmodel_engine.h"
#include "glquake.h"
#include "builddisp.h"
#include "collisionutils.h"
#include "enginestats.h"

Vector trace_StabDir;		// the direction to stab in
int trace_bDispHit;			// hit displacement surface last

int g_DispCollTreeCount = 0;
CDispCollTree *g_pDispCollTrees = NULL;

//csurface_t dispSurf = { "terrain", 0, 0 };

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void SetDispTraceSurfaceProps( trace_t *pTrace, CDispCollTree *pDisp )
{
	// use the default surface properties
	pTrace->surface.name = "**displacement**";
	pTrace->surface.flags = 0;
	pTrace->surface.surfaceProps = pDisp->GetCollisionSurfProp();
}

//-----------------------------------------------------------------------------
// New Collision!
//-----------------------------------------------------------------------------
void CM_PreStab( cleaf_t *pLeaf, Vector &vStabDir, int collisionMask, int &contents )
{
	if( !pLeaf->m_pDisplacements )
		return;

	// if the point wasn't in the bounded area of any of the displacements -- stab in any
	// direction and set contents to "solid"
	CDispCollTree *pDispTree = static_cast<CDispCollTree*>( pLeaf->m_pDisplacements->m_pDispInfo );
	pDispTree->GetStabDirection( vStabDir );
	contents = CONTENTS_SOLID;

	//
	// if the point is inside a displacement's (in the leaf) bounded area
	// then get the direction to stab from it
	//
	for( CDispIterator it( pLeaf->m_pDisplacements, CDispLeafLink::LIST_LEAF ); it.IsValid(); )
	{
		pDispTree = static_cast<CDispCollTree*>( it.Inc()->m_pDispInfo );

		// Respect trace contents
		if( !(pDispTree->GetContents() & collisionMask) )
			continue;

		bool bIsPoint = ( trace_ispoint == 1 );

		if( pDispTree->PointInBounds( trace_start, trace_mins, trace_maxs, bIsPoint ) )
		{
			pDispTree->GetStabDirection( vStabDir );
			contents = pDispTree->GetContents();
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// New Collision!
//-----------------------------------------------------------------------------
void CM_Stab( CCollisionBSPData *pBSPData, const Vector &start, const Vector &vStabDir, int contents )
{
	//
	// initialize the displacement trace parameters
	//
	trace_trace.fraction = 1.0f;
	trace_trace.fractionleftsolid = 0.0f;
	trace_trace.surface = nullsurface;

	trace_trace.startsolid = false;
	trace_trace.allsolid = false;

	trace_bDispHit = false;
	trace_StabDir = vStabDir;

	Vector end = trace_end;

	trace_start = start;
	trace_end = start + ( vStabDir * /* world extents * 2*/99999.9f );

	// increment the checkcount -- so we can retest objects that may have been tested
	// previous to the stab
	BeginCheckCount();

	// increment the stab count -- statistics
	g_CollisionCounts.m_Stabs++;

	// stab
	CM_RecursiveHullCheck( pBSPData, 0 /*root*/, 0.0f, 1.0f, trace_start, trace_end );

	EndCheckCount();

	trace_end = end;
}

//-----------------------------------------------------------------------------
// New Collision!
//-----------------------------------------------------------------------------
void CM_PostStab( void )
{
	//
	// only need to resolve things that impacted against a displacement surface,
	// this is partially resolved in the post trace phase -- so just use that
	// data to determine
	//
	if( trace_bDispHit && trace_trace.startsolid )
	{
		trace_trace.allsolid = true;
		trace_trace.fraction = 0.0f;
		trace_trace.fractionleftsolid = 0.0f;
	}
	else
	{
		trace_trace.startsolid = false;
		trace_trace.allsolid = false;
		trace_trace.contents = 0;
		trace_trace.fraction = 1.0f;
		trace_trace.fractionleftsolid = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// New Collision!
//-----------------------------------------------------------------------------
void CM_TestInDispTree( CCollisionBSPData *pBSPData, cleaf_t *pLeaf, const Vector &traceStart,
		const Vector &boxMin, const Vector &boxMax, int collisionMask, trace_t *pTrace )
{
	int nCurrentCheckCount = CurrentCheckCount();
	int nDepth = CurrentCheckCountDepth();

	bool bIsBox = ( ( boxMin.x != 0.0f ) || ( boxMin.y != 0.0f ) || ( boxMin.z != 0.0f ) ||
		            ( boxMax.x != 0.0f ) || ( boxMax.y != 0.0f ) || ( boxMax.z != 0.0f ) );

	// box test
	if( bIsBox )
	{
		//
		// test box against all displacements in the leaf
		//
		for( CDispIterator it( pLeaf->m_pDisplacements, CDispLeafLink::LIST_LEAF ); it.IsValid(); )
		{
			CDispCollTree *pDispTree = static_cast<CDispCollTree*>( it.Inc()->m_pDispInfo );

			// make sure we only check this brush once per trace/stab
			if( pDispTree->GetCheckCount(nDepth) == nCurrentCheckCount )
				continue;

			// mark the displacement as checked
			pDispTree->SetCheckCount( nDepth, nCurrentCheckCount );

			// Respect trace contents
			if( !(pDispTree->GetContents() & collisionMask) )
				continue;

			// box/tree intersection test
			if( pDispTree->AABBIntersect( traceStart, boxMin, boxMax ) )
			{
				pTrace->startsolid = true;
				pTrace->allsolid = true;
				pTrace->fraction = 0.0f;
				pTrace->fractionleftsolid = 0.0f;
				pTrace->contents = pDispTree->GetContents();
				return;
			}
		}
	}

	Assert( nDepth == CurrentCheckCountDepth() );
	Assert( nCurrentCheckCount == CurrentCheckCount() );

	//
	// need to stab if is was a point test or the box test yeilded no intersection
	//
	Vector stabDir;
	int    contents;
	CM_PreStab( pLeaf, stabDir, collisionMask, contents );
	CM_Stab( pBSPData, traceStart, stabDir, contents );
	CM_PostStab();

	Assert( nDepth == CurrentCheckCountDepth() );
	Assert( nCurrentCheckCount == CurrentCheckCount() );
}

//-----------------------------------------------------------------------------
// New Collision!
//-----------------------------------------------------------------------------
void CM_TraceToDispTree( CDispCollTree *pDispTree, Vector &traceStart, Vector &traceEnd,
						 Vector &boxMin, Vector &boxMax, float startFrac, float endFrac, 
						 trace_t *pTrace, bool bRayCast )
{
	// ray cast
	if( bRayCast )
	{
		if( pDispTree->RayTest( traceStart, traceEnd, startFrac, endFrac, pTrace ) )
		{
			trace_bDispHit = true;
			pTrace->contents = pDispTree->GetContents();
			SetDispTraceSurfaceProps( pTrace, pDispTree );
		}
	}
	// box sweep
	else
	{
		Vector boxExtents = ( ( boxMin + boxMax ) * 0.5f ) - boxMin;

		if( pDispTree->AABBSweep( traceStart, traceEnd, boxExtents,
			                      startFrac, endFrac, pTrace ) )
		{
			trace_bDispHit = true;
			pTrace->contents = pDispTree->GetContents();
			SetDispTraceSurfaceProps( pTrace, pDispTree );
		}
	}
}

//-----------------------------------------------------------------------------
// New Collision!
//-----------------------------------------------------------------------------
void CM_PostTraceToDispTree( void )
{
	// only resolve things that impacted against a displacement surface
	if( !trace_bDispHit )
		return;

	//
	// determine whether or not we are in solid
	//	
	Vector traceDir = trace_end - trace_start;
	
	if( DotProduct( trace_trace.plane.normal, traceDir ) > 0.0f )
	{
		trace_trace.startsolid = true;
		trace_trace.allsolid = true;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DispTreeLeafnum_r( CCollisionBSPData *pBSPData, CDispCollTree *pDispTree, int nodeIndex )
{
    while( 1 )
    {
        //
        // leaf
        //
        if( nodeIndex < 0 )
        {
			//
			// get leaf node
			//
			int leafIndex = -1 - nodeIndex;
			cleaf_t *pLeaf = &pBSPData->map_leafs[leafIndex];
			
			CDispLeafLink *pLink = static_cast< CDispLeafLink* >( g_DispLeafMPool.Alloc( sizeof(CDispLeafLink) ) );
			pLink->Add( pDispTree, pDispTree->GetLeafLinkHead(), pLeaf, pLeaf->m_pDisplacements );

			return;
        }

        //
        // choose side(s) to traverse
        //
        cnode_t *pNode = &pBSPData->map_rootnode[nodeIndex];
        cplane_t *pPlane = pNode->plane;

        // get box position relative to the plane
		Vector mins, maxs;
		pDispTree->GetBounds( mins, maxs );
        int sideResult = BOX_ON_PLANE_SIDE( mins, maxs, pPlane );

        // front side
        if( sideResult == 1 )
        {
            nodeIndex = pNode->children[0];
        }
        // back side
        else if( sideResult == 2 )
        {
            nodeIndex = pNode->children[1];
        }
        //split
        else
        {
            DispTreeLeafnum_r( pBSPData, pDispTree, pNode->children[0] );
            nodeIndex = pNode->children[1];
        }
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CM_DispTreeLeafnum( CCollisionBSPData *pBSPData )
{
	// check to see if there are any displacement trees to push down the bsp tree??
	if( g_DispCollTreeCount == 0 )
		return;

	//
	// get the number of displacements per leaf
	//
	for( int i = 0; i < g_DispCollTreeCount; i++ )
	{
		//
		// get tree and see if it is real (power != 0)
		//
		CDispCollTree *pDispTree = &g_pDispCollTrees[i];
		if( !pDispTree || ( pDispTree->GetPower() == 0 ) )
			continue;

		DispTreeLeafnum_r( pBSPData, pDispTree, pBSPData->map_cmodels[0].headnode );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CDispCollTree **DispCollTrees_AllocLeafList( int size )
{
	if( size == 0 )
		return NULL;

	CDispCollTree **ppDispList = new CDispCollTree*[size];
	return ppDispList;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DispCollTrees_FreeLeafList( CCollisionBSPData *pBSPData )
{
	for( int i = 0; i < pBSPData->numleafs; i++ )
	{
		cleaf_t *pLeaf = &pBSPData->map_leafs[i];
		if( pLeaf )
		{
			// Free and detach all links.
			while( pLeaf->m_pDisplacements )
			{
				CDispLeafLink *pLink = pLeaf->m_pDisplacements;
				CDispCollTree *pTree = static_cast<CDispCollTree*>( pLink->m_pDispInfo );
				pLeaf->m_pDisplacements->Remove( pTree->GetLeafLinkHead(), pLeaf->m_pDisplacements );
				g_DispLeafMPool.Free( pLink );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void ReAddDispSurfToCModelLeafs( CDispCollTree *pDispInfo )
{
	// Remove it from leaves.
	CDispLeafLink *pHead;
	while( pHead = pDispInfo->GetLeafLinkHead() )
	{
		cleaf_t *pLeaf = static_cast<cleaf_t*>( pHead->m_pLeaf );
		pHead->Remove( pDispInfo->GetLeafLinkHead(), pLeaf->m_pDisplacements );
		g_DispLeafMPool.Free( pHead );
	}

	// Add it back.
	CCollisionBSPData *pBSPData = GetCollisionBSPData();
	DispTreeLeafnum_r( pBSPData, pDispInfo, pBSPData->map_cmodels[0].headnode );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CM_ApplyTerrainMod( ITerrainMod *pMod )
{
	Vector modMin, modMax;
	pMod->GetBBox( modMin, modMax );

	for( int i=0; i < g_DispCollTreeCount; i++ )
	{
		CDispCollTree *pDispTree = &g_pDispCollTrees[i];

		Vector bbMin, bbMax;
		pDispTree->GetBounds( bbMin, bbMax );

		if( QuickBoxIntersectTest( modMin, modMax, bbMin, bbMax ) )
		{
			pDispTree->ApplyTerrainMod( pMod );
			ReAddDispSurfToCModelLeafs( pDispTree );
		}
	}
}
