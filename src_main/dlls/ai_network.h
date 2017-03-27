//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_NETWORK_H
#define AI_NETWORK_H

#ifdef _WIN32
#pragma once
#endif

#include "utlpriorityqueue.h"

// ------------------------------------

class CAI_Node;
class CBitString;
class INodeListFilter;

struct AI_Waypoint_t;
class CAI_BaseNPC;
class CAI_Link;

//-----------------------------------------------------------------------------

class CAI_NetworkManager;

//-----------------------------------------------------------------------------

#define	AI_MAX_NODE_LINKS 30
#define MAX_NODES 1500

//-----------------------------------------------------------------------------
// 
// Utility classes used by CAI_Network
//
//-----------------------------------------------------------------------------

struct AI_NearNode_t
{
	AI_NearNode_t() {}
	AI_NearNode_t( int index, float nodedist ) { dist = nodedist; nodeIndex = index; }
	float	dist;
	int		nodeIndex;
};

//-------------------------------------

class CNodeList : public CUtlPriorityQueue<AI_NearNode_t>
{
public:
	static bool IsLowerPriority( const AI_NearNode_t &node1, const AI_NearNode_t &node2 )
	{
		// nodes with greater distance are lower priority
		return node1.dist > node2.dist;
	}
	static bool RevIsLowerPriority( const AI_NearNode_t &node1, const AI_NearNode_t &node2 )
	{
		// nodes with lower distance are lower priority
		return node2.dist > node1.dist;
	}

	CNodeList( int growSize = 0, int initSize = 0 ) : CUtlPriorityQueue<AI_NearNode_t>( growSize, initSize, IsLowerPriority ) {}
	CNodeList( AI_NearNode_t *pMemory, int count ) : CUtlPriorityQueue<AI_NearNode_t>( pMemory, count, IsLowerPriority ) {}
};

//-----------------------------------------------------------------------------
// CAI_Network
//
// Purpose: Stores a node graph through which an AI may pathfind
//-----------------------------------------------------------------------------

class CAI_Network
{
public:
	CAI_Network();
	~CAI_Network();

	CAI_Node *		AddNode( const Vector &origin, float yaw );						// Returns a new node in the network

	bool			IsConnected(int srcID, int destID);	// Use during run time
	void			TestIsConnected(int startID, int endID);	// Use only for initialization!
	
	Vector			GetNodePosition( CBaseCombatCharacter *pNPC, int nodeID );
	Vector			GetNodePosition( Hull_t hull, int nodeID );
	float			GetNodeYaw( int nodeID );

	static int		FindBSSmallest(CBitString *bitString, float *float_array, int array_size); 

	int				NearestNodeToNPCAtPoint( CAI_BaseNPC* pNPC, const Vector &vecOrigin );
	int				NearestNodeToPoint(const Vector &vPosition);
	
	int				NumNodes() const 	{ return m_iNumNodes; }
	CAI_Node*		GetNode( int id )	{ if (id < m_iNumNodes ) return m_pAInode[id]; AssertMsg(0, "Node out of range"); return NULL; }
	
	CAI_Node**		AccessNodes() const	{ return m_pAInode; }
	
private:
	friend class CAI_NetworkManager;

	int				GetCachedNode(const Vector &checkPos);
	void			SetCachedNearestNode(const Vector &checkPos, int nodeID, Hull_t nHull);

	int				ListNodesInBox( CNodeList &list, int maxListCount, const Vector &mins, const Vector &maxs, INodeListFilter *pFilter );

	void			InitZones();
	void			FloodFillZone( CAI_Node *pNode, int zone );

	//---------------------------------

	enum
	{
		NEARNODE_CACHE_SIZE = 20,
		NEARNODE_CACHE_LIFE = 5,
	};

	struct NearNodeCache_T
	{
		Vector	vTestPosition;		
		float	fTime;						// Time tested
		int		nNearestNode;				// Nearest Node to position
		int		nHullType;					// Hull	type tested (or HULL_NONE is only visibility tested)

	};

	int					m_iNumNodes;				// Number of nodes in this network
	CAI_Node**			m_pAInode;					// Array of all nodes in this network

	NearNodeCache_T		m_pNearestCache[NEARNODE_CACHE_SIZE];	// Cache of nearest nodes
	int					m_nNearestCacheIndex;					// Oldest record in the cache
};

//-----------------------------------------------------------------------------
// CAI_NetworkEditTools
//
// Purpose: Provides the operations used when building levels, whether in-game
//			debugging tools or editor related items.
//
//-----------------------------------------------------------------------------


// ------------------------------------
// Debug overlay bits

enum DebugNetOverlayBits_e
{
	bits_debugOverlayNodes			=	0x00000001,		// show node
	bits_debugOverlayNodesLev2		=	0x00000002,		// show nodes and text

	bits_debugOverlayHulls			=	0x00000004,		// show hulls
	bits_debugOverlayConnections	=	0x00000008,		// show connections
	bits_debugOverlayVisibility		=	0x00000010,		// show visibility
	bits_debugOverlayGraphConnect	=   0x00000020,		// show graph connectivity
	bits_debugOverlayGrid			=   0x00000040,		// show grid
	bits_debugOverlayHints			=	0x00000080,		// show hints
	bits_debugOverlayJumpConnections=	0x00000100,		// show jump connections
	bits_debugOverlayFlyConnections	=	0x00000200,		// show fly connections

	bits_debugNeedRebuild			=	0x10000000,		// network needs rebuilding
};

// ------------------------------------

// ----------------

//-----------------------------------------------------------------------------
// Useful utility function defined by AI_network.cpp 
Vector PointOnLineNearestPoint(const Vector& vStartPos, const Vector& vEndPos, const Vector& vPoint);

//-----------------------------------------------------------------------------

// For now just using one big AI network
extern CAI_NetworkManager *	g_pAINetworkManager;			
extern CAI_Network * 		g_pBigAINet;			

//=============================================================================

#endif // AI_NETWORK_H
