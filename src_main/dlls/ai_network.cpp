//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "ai_network.h"
#include "ai_node.h"
#include "ai_basenpc.h"
#include "ai_link.h"
#include "ai_navigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// For now we just have one AINetwork called "BigNet".  At some
// later point we will probabaly have multiple AINetworkds per level
CAI_Network*		g_pBigAINet;			

//-----------------------------------------------------------------------------

class INodeListFilter
{
public:
	virtual bool	NodeIsValid( CAI_Node &node ) = 0;
	virtual float	NodeDistanceSqr( CAI_Node &node ) = 0;
};

//-------------------------------------
// Purpose: Filters nodes for a position in space
//-------------------------------------

class CNodePosFilter : public INodeListFilter
{
public:
	CNodePosFilter( const Vector &pos ) : m_pos(pos) {}

	virtual bool	NodeIsValid( CAI_Node &node ) { return true; }
	virtual float	NodeDistanceSqr( CAI_Node &node )
	{
		return (node.GetOrigin() - m_pos).LengthSqr();
	}

	const Vector &m_pos;
};

//-------------------------------------
// Purpose: Filters nodes for an NPC
//-------------------------------------

class CNodeNPCFilter : public INodeListFilter
{
public:
	CNodeNPCFilter( CAI_BaseNPC *pNPC, const Vector &pos ) : m_pNPC(pNPC), m_pos(pos) 
	{
		m_capabilities = m_pNPC->CapabilitiesGet();
	}

	virtual bool	NodeIsValid( CAI_Node &node )
	{
		// Check that node is of proper type for this NPC's navigation ability
		if ((node.GetType() == NODE_AIR    && !(m_capabilities & bits_CAP_MOVE_FLY))		||
			(node.GetType() == NODE_GROUND && !(m_capabilities & bits_CAP_MOVE_GROUND))	)
			return false;

		return true;
	}

	virtual float	NodeDistanceSqr( CAI_Node &node )
	{
		// UNDONE: This call to Position() really seems excessive here.  What is the real
		// error % relative to 800 units (MAX_NODE_LINK_DIST) ?
		return (node.GetPosition(m_pNPC->GetHullType()) - m_pos).LengthSqr();
	}

	const Vector &m_pos;
	CAI_BaseNPC	*m_pNPC;
	int			m_capabilities;	// cache this
};

//-----------------------------------------------------------------------------
// CAI_Network
//-----------------------------------------------------------------------------

// for searching for the nearest node
// PERFORMANCE: Tune this number
#define MAX_NEAR_NODES	10			// Trace to 10 nodes at most

//-----------------------------------------------------------------------------

CAI_Network::CAI_Network()
{
	m_iNumNodes				= 0;		// Number of nodes in this network
	m_pAInode				= NULL;		// Array of all nodes in this network

	m_nNearestCacheIndex	= 0;
	// Force empty node caches to be rebuild
	for (int node=0;node<NEARNODE_CACHE_SIZE;node++)
	{
		m_pNearestCache[node].fTime	= -2*NEARNODE_CACHE_LIFE;
	}
}

//-----------------------------------------------------------------------------

CAI_Network::~CAI_Network()
{
	if ( m_pAInode )
	{
		for (int node = 0; node < m_iNumNodes; node++)
		{
			for (int link = 0; link < m_pAInode[node]->m_iNumLinks; link++)
			{
				if (m_pAInode[node]->m_Links[link])
				{
					// @TODO clean this up. relocated from node destructor, and not pretty

					// Only link in the other node if after the current node so we don't delete it twice!
					int destID = m_pAInode[node]->m_Links[link]->DestNodeID(m_pAInode[node]->m_iID);
					if (destID > m_pAInode[node]->m_iID)
					{
						for (int slink = 0; slink < GetNode(destID)->NumLinks();slink++)
						{
							if (GetNode(destID)->GetLinkByIndex(slink) &&
							(GetNode(destID)->GetLinkByIndex(slink)->m_iDestID == m_pAInode[node]->m_iID ||
							 GetNode(destID)->GetLinkByIndex(slink)->m_iSrcID == m_pAInode[node]->m_iID  ))
							{
								GetNode(destID)->m_Links[slink] = NULL;
							}
						}
						delete m_pAInode[node]->m_Links[link];
						m_pAInode[node]->m_Links[link] = NULL;
					}
				}
			}
			delete m_pAInode[node];
		}
	}
	delete[] m_pAInode;
	m_pAInode = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Given an bitString and float array of size array_size, return the 
//			index of the smallest number in the array whose it is set
//-----------------------------------------------------------------------------

int	CAI_Network::FindBSSmallest(CBitString *bitString, float *float_array, int array_size) 
{
	int	  winIndex = -1;
	float winSize  = FLT_MAX;
	for (int i=0;i<array_size;i++) 
	{
		if (bitString->GetBit(i) && (float_array[i]<winSize)) 
		{
			winIndex = i;
			winSize  = float_array[i];
		}
	}
	return winIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Build a list of nearby nodes sorted by distance
// Input  : &list - 
//			maxListCount - 
//			*pFilter - 
// Output : int - count of list
//-----------------------------------------------------------------------------

int CAI_Network::ListNodesInBox( CNodeList &list, int maxListCount, const Vector &mins, const Vector &maxs, INodeListFilter *pFilter )
{
	CNodeList result;
	
	result.SetLessFunc( CNodeList::RevIsLowerPriority );
	
	// NOTE: maxListCount must be > 0 or this will crash
	bool full = false;
	float flClosest = 1000000.0 * 1000000;
	int closest = 0;

// UNDONE: Store the nodes in a tree and query the tree instead of the entire list!!!
	for ( int node = 0; node < m_iNumNodes; node++ )
	{
		if ( !pFilter->NodeIsValid(*m_pAInode[node]) )
			continue;

		// in box?
		if ( m_pAInode[node]->GetOrigin().x < mins.x || m_pAInode[node]->GetOrigin().x > maxs.x ||
			m_pAInode[node]->GetOrigin().y < mins.y || m_pAInode[node]->GetOrigin().y > maxs.y ||
			m_pAInode[node]->GetOrigin().z < mins.z || m_pAInode[node]->GetOrigin().z > maxs.z )
			continue;

		float flDist = pFilter->NodeDistanceSqr(*m_pAInode[node]);

		if ( flDist < flClosest )
		{
			closest = node;
			flClosest = flDist;
		}

		if ( !full || (flDist < result.ElementAtHead().dist) )
		{
			if ( full )
				result.RemoveAtHead();

			result.Insert( AI_NearNode_t(node, flDist) );
	
			full = (result.Count() == maxListCount);
		}
	}
	
	list.RemoveAll();
	while ( result.Count() )
	{
		list.Insert( result.ElementAtHead() );
		result.RemoveAtHead();
	}

	return list.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Return ID of node nearest of vecOrigin for pNPC with the given
//			tolerance distance.  If a route is required to get to the node
//			node_route is set.
//-----------------------------------------------------------------------------

int	CAI_Network::NearestNodeToNPCAtPoint( CAI_BaseNPC *pNPC, const Vector &vecOrigin )
{
	// --------------------------------
	//  Check if network has no nodes
	// --------------------------------
	if (m_iNumNodes == 0)
		return NO_NODE;

	CNodeNPCFilter filter( pNPC, vecOrigin );
	// ----------------------------------------------------------------
	//  First check cached nearest node positions
	// ----------------------------------------------------------------
	int nodeID = GetCachedNode(vecOrigin);
	if (nodeID >= 0 )
	{
		if ( filter.NodeIsValid( *m_pAInode[nodeID] ) && pNPC->GetNavigator()->CanFitAtNode(nodeID) )
			return nodeID;
	}

	
	// ---------------------------------------------------------------
	// First get nodes distances and eliminate those that are beyond 
	// the maximum allowed distance for local movements
	// ---------------------------------------------------------------
#ifdef AI_PERF_MON
		m_nPerfStatNN++;
#endif

	AI_NearNode_t *pBuffer = (AI_NearNode_t *)stackalloc( sizeof(AI_NearNode_t) * MAX_NEAR_NODES );
	CNodeList list( pBuffer, MAX_NEAR_NODES );

	// OPTIMIZE: If not flying, this box should be smaller in Z (2 * height?)
	Vector ext(MAX_NODE_LINK_DIST, MAX_NODE_LINK_DIST, MAX_NODE_LINK_DIST);
	// If the NPC can fly, check further
	if ( pNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY ) 
	{
		ext.Init( MAX_AIR_NODE_LINK_DIST, MAX_AIR_NODE_LINK_DIST, MAX_AIR_NODE_LINK_DIST );
	}

	ListNodesInBox( list, MAX_NEAR_NODES, vecOrigin - ext, vecOrigin + ext, &filter );

	// --------------------------------------------------------------
	//  Now find a reachable node searching the close nodes first
	// --------------------------------------------------------------
	//int smallestVisibleID = NO_NODE;

	for( ;list.Count(); list.RemoveAtHead() )
	{
		int smallest = list.ElementAtHead().nodeIndex;

		// Check that this node is usable by the current hull size
		if (!pNPC->GetNavigator()->CanFitAtNode(smallest))
			continue;

		trace_t tr;
		// test node visibility
		AI_TraceLine ( vecOrigin, m_pAInode[smallest]->GetPosition(pNPC->GetHullType()) + pNPC->GetViewOffset(), 
			MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0 )
			return smallest;
	}

	// Store inability to reach in cache for later use
	SetCachedNearestNode(vecOrigin,NO_NODE,pNPC->GetHullType());

	return NO_NODE;
}


//-----------------------------------------------------------------------------
// Purpose: Find the nearest node to an entity without regard to how whether
//			the node can be reached
//-----------------------------------------------------------------------------

int	CAI_Network::NearestNodeToPoint(const Vector &vPosition)
{
	// ----------------------------------------------------------------
	//  Makes sure we have nodes in this network
	// ----------------------------------------------------------------
	if (m_iNumNodes == 0)
		return NO_NODE;

	// ----------------------------------------------------------------
	//  First check cached nearest node positions
	// ----------------------------------------------------------------
	int nodeID = GetCachedNode(vPosition);
	if (nodeID != NOT_CACHED)
		return nodeID;

	// ---------------------------------------------------------------
	// First get nodes distances and eliminate those that are beyond 
	// the maximum allowed distance for local movements
	// ---------------------------------------------------------------
#ifdef AI_PERF_MON
	m_nPerfStatNN++;
#endif

	AI_NearNode_t *pBuffer = (AI_NearNode_t *)stackalloc( sizeof(AI_NearNode_t) * MAX_NEAR_NODES );
	CNodeList list( pBuffer, MAX_NEAR_NODES );

	CNodePosFilter filter( vPosition );

	// Robin: If I had context here, I'd be able to search a smaller distance if it was a ground node.
	//		  It should have context, because if I can't fly, nearest air node's useless to me anyway.
	//Vector ext(MAX_NODE_LINK_DIST, MAX_NODE_LINK_DIST, MAX_NODE_LINK_DIST);
	Vector ext = Vector( MAX_AIR_NODE_LINK_DIST, MAX_AIR_NODE_LINK_DIST, MAX_AIR_NODE_LINK_DIST );

	ListNodesInBox( list, MAX_NEAR_NODES, vPosition - ext, vPosition + ext, &filter );

	// --------------------------------------------------------------
	//  Now find a visible node searching the close nodes first
	// --------------------------------------------------------------
	for( ;list.Count(); list.RemoveAtHead() )
	{
		trace_t tr;
		int smallest = list.ElementAtHead().nodeIndex;
		AI_TraceLine( vPosition, m_pAInode[smallest]->GetOrigin(), MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction == 1.0 )
		{
			// Store nearest node in cache for later use
			SetCachedNearestNode(vPosition,smallest,HULL_NONE);
			return smallest;
		}
	}
	// Store inability to reach in cache for later use
	SetCachedNearestNode(vPosition,NO_NODE,HULL_NONE);

	return NO_NODE;
}
	
//-----------------------------------------------------------------------------
// Purpose: Check nearest node cache for checkPos and return cached nearest
//			node if it exists in the cache.  Doesn't care about reachability,
//			only if the node is visible
//-----------------------------------------------------------------------------

int	CAI_Network::GetCachedNode(const Vector &checkPos)
{
	// undone: check if this type of npc can actually get there...
	for (int node=0;node<NEARNODE_CACHE_SIZE;node++)
	{
		// Check if data is stale
		if ((m_pNearestCache[node].fTime + NEARNODE_CACHE_LIFE) < gpGlobals->curtime) 
		{
			continue;
		}

		// If hull type isn't HULL_NONE, skip as we aren't concerned about 
		// reachablility, we only care if this is the nearest reachable node
		if (m_pNearestCache[node].nHullType != HULL_NONE)
		{
			continue;
		}

		// Check if positions match
		if ((m_pNearestCache[node].vTestPosition - checkPos).Length() < 24.0)
		{
			return m_pNearestCache[node].nNearestNode;
		}
	}
	return NOT_CACHED;
}

//-----------------------------------------------------------------------------
// Purpose: Update nearest node cache with new data
//			if nHull == HULL_NONE, reachability of this node wasn't checked
//-----------------------------------------------------------------------------

void CAI_Network::SetCachedNearestNode(const Vector &checkPos, int nodeID, Hull_t nHull)
{
	if (m_pNearestCache[m_nNearestCacheIndex].fTime == gpGlobals->curtime)
	{
		Msg("AI NearestNode Cache is full\n");
	}

	m_pNearestCache[m_nNearestCacheIndex].vTestPosition = checkPos;
	m_pNearestCache[m_nNearestCacheIndex].nNearestNode  = nodeID;
	m_pNearestCache[m_nNearestCacheIndex].nHullType		= nHull;
	m_pNearestCache[m_nNearestCacheIndex].fTime			= gpGlobals->curtime;

	m_nNearestCacheIndex++;
	if (m_nNearestCacheIndex == NEARNODE_CACHE_SIZE)
	{
		m_nNearestCacheIndex = 0;
	}
}

//-----------------------------------------------------------------------------

Vector CAI_Network::GetNodePosition( Hull_t hull, int nodeID )
{
	if ( !m_pAInode )
	{
		Assert( 0 );
		return vec3_origin;
	}
	
	if ( ( nodeID < 0 ) || ( nodeID > m_iNumNodes ) )
	{
		Assert( 0 );
		return vec3_origin;
	}

	return m_pAInode[nodeID]->GetPosition( hull );
}

//-----------------------------------------------------------------------------

Vector CAI_Network::GetNodePosition( CBaseCombatCharacter *pNPC, int nodeID )
{
	if ( pNPC == NULL )
	{
		Assert( 0 );
		return vec3_origin;
	}

	return GetNodePosition( pNPC->GetHullType(), nodeID );
}

//-----------------------------------------------------------------------------

float CAI_Network::GetNodeYaw( int nodeID )
{
	if ( !m_pAInode )
	{
		Assert( 0 );
		return 0.0f;
	}
	
	if ( ( nodeID < 0 ) || ( nodeID > m_iNumNodes ) )
	{
		Assert( 0 );
		return 0.0f;
	}

	return m_pAInode[nodeID]->GetYaw();
}

//-----------------------------------------------------------------------------
// Purpose: Adds an ainode to the network
//-----------------------------------------------------------------------------

CAI_Node *CAI_Network::AddNode( const Vector &origin, float yaw )
{
	// ---------------------------------------------------------------------
	// When loaded from a file will know the number of nodes from 
	// the start.  Otherwise init MAX_NODES of them if not initialized
	// ---------------------------------------------------------------------
	if (!m_pAInode || !m_pAInode[0])
	{
		m_pAInode	= new CAI_Node*[MAX_NODES];
	}

	if (m_iNumNodes >= MAX_NODES)
	{
		Msg( "ERROR: too many nodes in map, deleting last node.\n", MAX_NODES );
		m_iNumNodes--;
	}

	m_pAInode[m_iNumNodes] = new CAI_Node( m_iNumNodes, origin, yaw );
	m_iNumNodes++;

	return m_pAInode[m_iNumNodes-1];
};

//-----------------------------------------------------------------------------
// Purpose: Returns true is two nodes are connected by the network graph
//-----------------------------------------------------------------------------

bool CAI_Network::IsConnected(int srcID, int destID)
{
	if (srcID > m_iNumNodes || destID > m_iNumNodes)
	{
		Msg("IsConnected called with invalid node IDs!\n");
		return false;
	}
	
	if ( srcID == destID )
		return true;
	
	int srcZone = m_pAInode[srcID]->GetZone();
	int destZone = m_pAInode[destID]->GetZone();
	
	if ( srcZone == AI_NODE_ZONE_SOLO || destZone == AI_NODE_ZONE_SOLO )
		return false;
		
	if ( srcZone == AI_NODE_ZONE_UNIVERSAL || destZone == AI_NODE_ZONE_UNIVERSAL ) // only happens in WC edit case
		return true;

#ifdef DEBUG
	if ( srcZone == AI_NODE_ZONE_UNKNOWN || destZone == AI_NODE_ZONE_UNKNOWN )
	{
		Msg( "Warning: Node found in unknown zone\n" );
		return true;
	}
#endif
		
	return ( srcZone == destZone );
}

//=============================================================================
