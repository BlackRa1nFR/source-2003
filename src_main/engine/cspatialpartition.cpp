//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// Algorithm:
//
// There's a one-to-one correspondence between "handle" and client object
// The handle may be inserted into many different leaves and may
// be part of many different lists.
//
// Each handle has a client-specified identifier, a bitfield indicating which 
// lists it's a part of, an axis aligned bounding box, and a head to a linked 
// list of leaves that it currently is in (to allow constant time removal of
// a handle from the spatial partition).
//
// Each "AreaNode" is an axis-aligned splitting plane of a KD tree. We can
// have split planes in x, y, and z, but the ones in z are less dense.
// The children indices of the AreaNode, when negative, specify an index
// into the area leaf array.
//
// Each "AreaLeaf" has a list of all handles that extend into the leaf. The
// m_LeafElements linked list actually contains all the linked lists for all
// the leafs. Each linked list entry simply contains a handle and links to the
// next entry, and each the area leaf stores the index of the first entry in 
// the m_LeafElements list.
// 
// The "HandleLeafList" contains all the linked lists of leaves that each handle
// extends into. The m_LeafList element of the HandleInfo is the index into 
// the m_HandleLeafList of the first element of that handle's leaf list.
// Each entry contains a leaf index (into AreaLeaf), and a m_LeafElements index 
// of the entry associated with the handle whose leaf list it is. Having that 
// there was neceesary for a constant-time element removal.
//
//=============================================================================

#include "ispatialpartitioninternal.h"
#include "utllinkedlist.h"
#include "utlvector.h"
#include "vector.h"
#include "collisionutils.h"
#include <float.h>
#include "cmodel_private.h"
#include "bsptreedata.h"
#include "utlhash.h"
#include "tier0/dbg.h"


//-----------------------------------------------------------------------------
// What's the max size of the smallest spatial partition?
//-----------------------------------------------------------------------------
#define SPATIAL_SIZE		(256)
#define NODE_HEIGHT_RATIO	(4)
#define SPATIAL_HEIGHT		(SPATIAL_SIZE * NODE_HEIGHT_RATIO)
#define TEST_EPSILON		(0.03125f)

#define INV_SPATIAL_SIZE	((float)(1.0 / SPATIAL_SIZE))
#define INV_SPATIAL_HEIGHT  ((float)(1.0 / SPATIAL_HEIGHT))


//-----------------------------------------------------------------------------
// All the information associated with a particular handle
//-----------------------------------------------------------------------------
struct HandleInfo_t
{
	int				m_EnumId;		// Prevents objects being sent multiple times in an enumeration
	IHandleEntity	*m_pHandleEntity;
	unsigned int	m_ListFlags;	// which lists is it in?
	BSPTreeDataHandle_t	m_TreeHandle;
	Vector			m_Min;
	Vector			m_Max;
};


//-----------------------------------------------------------------------------
// The spatial partition class
//-----------------------------------------------------------------------------
class CSpatialPartition : public ISpatialPartitionInternal, public ISpatialQuery
{
public:
	// constructor, destructor
	CSpatialPartition();
	virtual ~CSpatialPartition();

	// Methods of ISpatialPartition
	SpatialPartitionHandle_t CreateHandle( IHandleEntity *pHandleEntity );
	SpatialPartitionHandle_t CreateHandle( IHandleEntity *pHandleEntity,
		SpatialPartitionListMask_t listMask, const Vector& mins, const Vector& maxs );
	void DestroyHandle( SpatialPartitionHandle_t handle );

	void Insert( SpatialPartitionListMask_t listMask, SpatialPartitionHandle_t handle );
	void Remove( SpatialPartitionListMask_t listMask, SpatialPartitionHandle_t handle );
	void Remove( SpatialPartitionHandle_t handle );

	SpatialTempHandle_t FastRemove( SpatialPartitionHandle_t handle );
	void FastInsert( SpatialPartitionHandle_t handle, SpatialTempHandle_t tempHandle );

	void ElementMoved( SpatialPartitionHandle_t handle, 
		const Vector& mins, const Vector& maxs );

	void EnumerateElementsAtPoint( SpatialPartitionListMask_t listMask,
		const Vector& pt, bool coarseTest, 
		IPartitionEnumerator* pIterator );
	
	void EnumerateElementsInBox( SpatialPartitionListMask_t listMask,
		const Vector& mins, const Vector& maxs, bool coarseTest, 
		IPartitionEnumerator* pIterator );
	
	void EnumerateElementsInSphere( SpatialPartitionListMask_t listMask,
		const Vector& origin, float radius, bool coarseTest, 
		IPartitionEnumerator* pIterator );
	
	void EnumerateElementsAlongRay( SpatialPartitionListMask_t listMask,
		const Ray_t& ray, bool coarseTest, 
		IPartitionEnumerator* pIterator );

	// For debugging.... suppress queries on particular lists
	virtual void SuppressLists( SpatialPartitionListMask_t nListMask, bool bSuppress );
	virtual SpatialPartitionListMask_t GetSuppressedLists();

	// Methods of ISpatialPartitionInternal
	void	Init( const Vector& worldmin, const Vector& worldmax );

	// Returns the number of leaves
	int		LeafCount() const;

	// Enumerates the leaves along a ray, box, etc.
	bool	EnumerateLeavesAtPoint( const Vector& pt, ISpatialLeafEnumerator* pEnum, int context );
	bool	EnumerateLeavesInBox( const Vector& mins, const Vector& maxs, ISpatialLeafEnumerator* pEnum, int context );
	bool	EnumerateLeavesInSphere( const Vector& center, float radius, ISpatialLeafEnumerator* pEnum, int context );
	bool	EnumerateLeavesAlongRay( const Ray_t& ray, ISpatialLeafEnumerator* pEnum, int context );

	// Enumerates the elements in a leaf
	bool	EnumerateElementsInLeaf( int leaf, IBSPTreeDataEnumerator* pEnum, int context );

	// Gets handle info (for enumerations)
	HandleInfo_t&	HandleInfo( SpatialPartitionHandle_t handle );

private:

	// All the information associated with a node in the KD tree
	// (axis-aligned plane normal + intercept, children)
	struct AreaNode_t
	{
		char	m_Axis;			
		float	m_Dist;
		int		m_Children[2];			// negative means leaf
	};

	// Pointer to an enumeration member method of CSpatialPartition
	typedef IterationRetval_t (CSpatialPartition::*LeafFunc_t)( int leaf, int context );

	// Helper to initialize the tree
	int RecursiveInitTree (int const* mins, int const* maxs);

	// Inserts/Removes a handle from the tree
	void InsertIntoTree( SpatialPartitionHandle_t handle, const Vector& mins, const Vector& maxs );
	void RemoveFromTree( SpatialPartitionHandle_t handle );

	// Enumerates all leaves in a particular volume
	bool EnumerateLeaves_R( int node, const Vector& mins, 
		const Vector& maxs, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeaves_R( int node, const Vector& center, 
		float radius, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesRay_R( int node, const Ray_t& ray, 
		const Vector& invDelta, const Vector& start, 
		const Vector& end, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesExtrudedRay_R( int node, const Ray_t& ray,
		const Vector& invDelta,	const Vector& start, 
		const Vector& end, ISpatialLeafEnumerator* pEnum, int context );

	// The set of area nodes. Assume a uniformly deep tree.
	CUtlVector<AreaNode_t>	m_Node;

	// Stores all unique elements
	CUtlLinkedList< HandleInfo_t, SpatialPartitionHandle_t >	m_Handle;

	// The current enumeration ID
	int m_EnumId;

	// Manages the tree data
	IBSPTreeData* m_pTreeData;

	// Leaf count...
	int	m_LeafCount;

	SpatialPartitionListMask_t m_nSuppressedListMask;
};



//-----------------------------------------------------------------------------
// Expose CSpatialPartition to the game + client DLL.
//-----------------------------------------------------------------------------

static CSpatialPartition	g_SpatialPartition;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSpatialPartition, ISpatialPartition, 
					INTERFACEVERSION_SPATIALPARTITION, g_SpatialPartition);


//-----------------------------------------------------------------------------
// Expose ISpatialPartitionInternal to the engine.
//-----------------------------------------------------------------------------

ISpatialPartitionInternal* SpatialPartition()
{
	return &g_SpatialPartition;
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CSpatialPartition::CSpatialPartition() : m_EnumId(0)
{
	m_pTreeData = CreateBSPTreeData();
}

CSpatialPartition::~CSpatialPartition()
{
	DestroyBSPTreeData( m_pTreeData );
}

//-----------------------------------------------------------------------------
// Recursive method to create areanodes
//-----------------------------------------------------------------------------

int CSpatialPartition::RecursiveInitTree (int const* mins, int const* maxs)
{
	int i;

	// Compute box size
	int		size[3];
	for (i = 0; i < 3; ++i)
		size[i] = maxs[i] - mins[i];

	// Stop once we've reached a point where we're small enough
	if ( (size[0] == 1) && (size[1] == 1) )
	{
		// Make me a leaf...
		int leaf = m_LeafCount++;
		return - leaf - 1;
	}

	// Find largest dimension in the box and split along it
	// Split along the z axis only while the z size is bigger
	// than some amount
	int xyIdx = (size[0] >= size[1]) ? 0 : 1;

	int axis;
	if ((size[2] > NODE_HEIGHT_RATIO) && (size[2] >= size[xyIdx]))
	{
		axis = 2;
	}
	else
	{
		axis = xyIdx;
	}
	
	int node = m_Node.AddToTail();
	AreaNode_t& nodeInfo = m_Node[node];
	
	nodeInfo.m_Axis = axis;
	nodeInfo.m_Dist = 0.5 * SPATIAL_SIZE * (maxs[axis] + mins[axis]);

	// Perform the split
	int maxs1[3], mins2[3];
	for (i = 0; i < 3; ++i)
	{
		maxs1[i] = maxs[i];
		mins2[i] = mins[i];
	}
	
	maxs1[axis] = mins2[axis] = (maxs[axis] + mins[axis]) >> 1;
	
	// NOTE: Realloc from RecursiveInitTree will cause nodeInfo to become bogus
	int lchild = RecursiveInitTree (mins, maxs1);
	int rchild = RecursiveInitTree (mins2, maxs);
	m_Node[node].m_Children[0] = lchild;
	m_Node[node].m_Children[1] = rchild;

	return node;
}


//-----------------------------------------------------------------------------
// Helper for assertion
//-----------------------------------------------------------------------------

static bool IsPowerOfTwo( int i )
{
	while ((i & 0x1) == 0)
	{
		i >>= 1;
	}
	return (i == 1);
}


//-----------------------------------------------------------------------------
// Methods of ISpatialPartitionInternal
//-----------------------------------------------------------------------------
void CSpatialPartition::Init( const Vector& worldmin, const Vector& worldmax )
{
	int i;

	m_nSuppressedListMask = 0;

	// Clean up the tree
	m_pTreeData->Shutdown();

	// Clear stuff out baby
	m_Node.Purge();
	m_Handle.Purge();

	// Reset the leaf count
	m_LeafCount = 0;

	// Reset the enumeration id
	m_EnumId = 0;

	// Reserve some memory
	m_Node.EnsureCapacity( 256 );
	m_Handle.EnsureCapacity( 256 );

	// Compute new world bound that's 2^n * SPATIAL_SIZE
	// That way we always get the same partition size + we can do some early
	// outs in the EntityMoved method
	int intMin[3];
	intMin[0] = Floor2Int(worldmin[0] / SPATIAL_SIZE);
	intMin[1] = Floor2Int(worldmin[1] / SPATIAL_SIZE);
	intMin[2] = Floor2Int(worldmin[2] / SPATIAL_HEIGHT) * NODE_HEIGHT_RATIO;

	int intMax[3];
	for (i = 0; i < 3; ++i )
	{
		int intSize = (i < 2) ? 1 : NODE_HEIGHT_RATIO;
		float size = worldmax[i] - (intMin[i] * SPATIAL_SIZE);
		float t = SPATIAL_SIZE * intSize;
		while (t < size)
		{
			intSize <<= 1;
			t *= 2.0f;
		}
		intMax[i] = intMin[i] + intSize;
	}

	// for this to work, x,y must be a power of 2
	// z must be NODE_HEIGHT_RATIO * a power of 2
	Assert( IsPowerOfTwo( intMax[0] - intMin[0] ) );
	Assert( IsPowerOfTwo( intMax[1] - intMin[1] ) );
	Assert( IsPowerOfTwo( (intMax[2] - intMin[2]) / NODE_HEIGHT_RATIO ) );

	RecursiveInitTree( intMin, intMax );

	// Set up the tree. Has to be done *after* we set up the tree
	// since we won't know the leaf count until then
	m_pTreeData->Init(this);
}


//-----------------------------------------------------------------------------
// Create/destroy handle
//-----------------------------------------------------------------------------
SpatialPartitionHandle_t CSpatialPartition::CreateHandle( IHandleEntity *pHandleEntity )
{
	SpatialPartitionHandle_t handle = m_Handle.AddToTail();
	m_Handle[handle].m_TreeHandle = TREEDATA_INVALID_HANDLE;
	m_Handle[handle].m_pHandleEntity = pHandleEntity;
	m_Handle[handle].m_ListFlags = 0;
	m_Handle[handle].m_Min.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	m_Handle[handle].m_Max.Init( FLT_MIN, FLT_MIN, FLT_MIN );
	m_Handle[handle].m_EnumId = -1;

	return handle;
}

SpatialPartitionHandle_t CSpatialPartition::CreateHandle( IHandleEntity *pHandleEntity,
	SpatialPartitionListMask_t listMask, const Vector& mins, const Vector& maxs )
{
	SpatialPartitionHandle_t handle = CreateHandle( pHandleEntity );
	Insert( listMask, handle );
	InsertIntoTree( handle, mins, maxs );
	return handle;
}

void CSpatialPartition::DestroyHandle( SpatialPartitionHandle_t handle )
{
	if (handle != PARTITION_INVALID_HANDLE)
	{
		RemoveFromTree( handle );
		m_Handle.Remove( handle );
	}
}


//-----------------------------------------------------------------------------
// Insert/remove handles into/from groups
//-----------------------------------------------------------------------------
void CSpatialPartition::Insert( SpatialPartitionListMask_t listId, 
								SpatialPartitionHandle_t handle )
{
	Assert( m_Handle.IsValidIndex(handle) );

	m_Handle[handle].m_ListFlags |=  listId;
}

void CSpatialPartition::Remove( SpatialPartitionListMask_t listId, 
								SpatialPartitionHandle_t handle )
{
	Assert( m_Handle.IsValidIndex(handle) );

	m_Handle[handle].m_ListFlags &= ~listId;
}

void CSpatialPartition::Remove( SpatialPartitionHandle_t handle )
{
	Assert( m_Handle.IsValidIndex(handle) );

	m_Handle[handle].m_ListFlags = 0;
}


//-----------------------------------------------------------------------------
// Fast way to remove a handle from all groups + re-add it to the groups it was in
//-----------------------------------------------------------------------------
SpatialTempHandle_t CSpatialPartition::FastRemove( SpatialPartitionHandle_t handle )
{
	Assert( m_Handle.IsValidIndex(handle) );
	SpatialTempHandle_t oldLists = m_Handle[handle].m_ListFlags;
	m_Handle[handle].m_ListFlags = 0;
	return oldLists;
}

void CSpatialPartition::FastInsert( SpatialPartitionHandle_t handle, SpatialTempHandle_t tempHandle )
{
	Assert( m_Handle.IsValidIndex(handle) );
	m_Handle[handle].m_ListFlags = tempHandle;
}


//-----------------------------------------------------------------------------
// For debugging.... suppress queries on particular lists
//-----------------------------------------------------------------------------
void CSpatialPartition::SuppressLists( SpatialPartitionListMask_t nListMask, bool bSuppress )
{
	if (bSuppress)
	{
		m_nSuppressedListMask |= nListMask;
	}
	else
	{
		m_nSuppressedListMask &= ~nListMask;
	}
}


SpatialPartitionListMask_t CSpatialPartition::GetSuppressedLists()
{
	return m_nSuppressedListMask;
}


//-----------------------------------------------------------------------------
// Enumerates the elements in a leaf
//-----------------------------------------------------------------------------
inline bool CSpatialPartition::EnumerateElementsInLeaf( int leaf, IBSPTreeDataEnumerator* pEnum, int context )
{
	return m_pTreeData->EnumerateElementsInLeaf( leaf, pEnum, context );
}


//-----------------------------------------------------------------------------
// Gets handle info (for enumerations)
//-----------------------------------------------------------------------------
inline HandleInfo_t& CSpatialPartition::HandleInfo( SpatialPartitionHandle_t handle )
{
	return m_Handle[handle];
}


//-----------------------------------------------------------------------------
// Returns the number of leaves
//-----------------------------------------------------------------------------
int	CSpatialPartition::LeafCount() const
{
	return m_LeafCount;
}


//-----------------------------------------------------------------------------
// Enumerates all leaves at a point.... (well, there's only one)
//-----------------------------------------------------------------------------
bool CSpatialPartition::EnumerateLeavesAtPoint( const Vector& pt, 
										ISpatialLeafEnumerator* pEnum, int context )
{
	int node = 0;

	// Keep going until we hit a leaf...
	while (node >= 0)
	{
		AreaNode_t& nodeInfo = m_Node[node];

		if (pt[nodeInfo.m_Axis] <= nodeInfo.m_Dist)
			node = nodeInfo.m_Children[0];
		else
			node = nodeInfo.m_Children[1];
	}

	int leaf = - node - 1;
	return pEnum->EnumerateLeaf( leaf, context );
}


//-----------------------------------------------------------------------------
// Enumerates all leaves in a box...
//-----------------------------------------------------------------------------
bool CSpatialPartition::EnumerateLeaves_R( int node, const Vector& mins, 
				const Vector& maxs, ISpatialLeafEnumerator* pEnum, int context )
{
	bool ret;

	// Keep going until we hit a leaf...
	while (node >= 0)
	{
		AreaNode_t& nodeInfo = m_Node[node];

		// Don't bother recursing if we're not split
		float behind = maxs[nodeInfo.m_Axis] - nodeInfo.m_Dist;
		float front = mins[nodeInfo.m_Axis] - nodeInfo.m_Dist;

		if (behind <= -TEST_EPSILON)
			node = nodeInfo.m_Children[0];
		else if (front >= TEST_EPSILON)
			node = nodeInfo.m_Children[1];
		else
		{
			// Here the box is split by the node
			ret = EnumerateLeaves_R( nodeInfo.m_Children[0], mins, maxs, 
				pEnum, context );
			if (!ret)
				return false;

			return EnumerateLeaves_R( nodeInfo.m_Children[1], mins, maxs, 
				pEnum, context );
		}
	}

	int leaf = - node - 1;
	return pEnum->EnumerateLeaf( leaf, context );
}

bool CSpatialPartition::EnumerateLeavesInBox( const Vector& mins, 
					const Vector& maxs, ISpatialLeafEnumerator* pEnum, int context )
{
	return EnumerateLeaves_R( 0, mins, maxs, pEnum, context );
}


//-----------------------------------------------------------------------------
// Enumerates all leaves in a sphere...
//-----------------------------------------------------------------------------

bool CSpatialPartition::EnumerateLeaves_R( int node, 
	const Vector& center, float radius, ISpatialLeafEnumerator* pEnum, int context )
{
	bool ret;

	// Keep going until we hit a leaf...
	while (node >= 0)
	{
		AreaNode_t& nodeInfo = m_Node[node];

		float behind = center[nodeInfo.m_Axis] + radius - nodeInfo.m_Dist;
		float front = center[nodeInfo.m_Axis] - radius - nodeInfo.m_Dist;

		// Don't bother recursing if we're not split
		if (behind <= -TEST_EPSILON)
			node = nodeInfo.m_Children[0];
		else if (front >= TEST_EPSILON)
			node = nodeInfo.m_Children[1];
		else
		{
			// Here the box is split by the node
			ret = EnumerateLeaves_R( nodeInfo.m_Children[0], center, radius, 
				pEnum, context );
			if (!ret)
				return false;

			return EnumerateLeaves_R( nodeInfo.m_Children[1], center, radius, 
				pEnum, context );
		}
	}

	int leaf = - node - 1;
	return pEnum->EnumerateLeaf( leaf, context );
}

bool CSpatialPartition::EnumerateLeavesInSphere( const Vector& center, 
						float radius, ISpatialLeafEnumerator* pEnum, int context )
{
	return EnumerateLeaves_R( 0, center, radius, pEnum, context );
}


//-----------------------------------------------------------------------------
// Tree traversal for simple ray
//-----------------------------------------------------------------------------

bool CSpatialPartition::EnumerateLeavesRay_R( int node, const Ray_t& ray,
	const Vector& invDelta, const Vector& start, const Vector& end,		
	ISpatialLeafEnumerator* pEnum, int context )
{
	bool ret;

	// Keep going until we hit a leaf...
	while (node >= 0)
	{
		AreaNode_t& nodeInfo = m_Node[node];

		// see if this node splits the ray
		float startRay = start[nodeInfo.m_Axis] - nodeInfo.m_Dist;
		float endRay = end[nodeInfo.m_Axis] - nodeInfo.m_Dist;

		// no split... don't bother recursing..
		if ( (startRay <= -TEST_EPSILON) && (endRay <= -TEST_EPSILON) )
			node = nodeInfo.m_Children[0];
		else if ( (startRay >= TEST_EPSILON) && (endRay >= TEST_EPSILON) )
			node = nodeInfo.m_Children[1];
		else
		{
			float splitfrac;

			// This can't happen or there's a problem with startBehind + endBehind
			if( invDelta[nodeInfo.m_Axis] == 0.0f )
			{
				splitfrac = 1.0f;
			}
			else
			{
				// Here the ray is split by the node
				// Compute the fraction...
				splitfrac = ( nodeInfo.m_Dist - ray.m_Start[nodeInfo.m_Axis] ) *
					invDelta[nodeInfo.m_Axis];
				splitfrac = clamp( splitfrac, 0.0f, 1.0f );
			}

			// Compute the split point
			Vector split;
			VectorMA( ray.m_Start, splitfrac, ray.m_Delta, split );
			
#ifdef _DEBUG
			float tol = (nodeInfo.m_Dist > 1.0) ? nodeInfo.m_Dist * TEST_EPSILON : TEST_EPSILON;
			Assert( fabs(split[nodeInfo.m_Axis] - nodeInfo.m_Dist) < tol );
#endif

			int startLeaf = (startRay < 0) ? 0 : 1;
			ret = EnumerateLeavesRay_R( nodeInfo.m_Children[startLeaf], ray,
				invDelta, start, split, pEnum, context );
			if (!ret)
				return false;

			return EnumerateLeavesRay_R( nodeInfo.m_Children[1-startLeaf], ray,
				invDelta, split, end, pEnum, context );
		}
	}

	int leaf = - node - 1;
	return pEnum->EnumerateLeaf( leaf, context );
}


//-----------------------------------------------------------------------------
// Tree traversal for extruded ray
//-----------------------------------------------------------------------------
bool CSpatialPartition::EnumerateLeavesExtrudedRay_R( int node, const Ray_t& ray,
	const Vector& invDelta,	const Vector& start, const Vector& end,
	ISpatialLeafEnumerator* pEnum, int context )
{
	bool ret;

	// Keep going until we hit a leaf...
	while (node >= 0)
	{
		AreaNode_t& nodeInfo = m_Node[node];

		// Find distances to the separating plane which has been 		
		// thickened by the extents
		// Bloat the extents out by an epsilon so we don't miss nuffin
		float tStart = start[nodeInfo.m_Axis] - nodeInfo.m_Dist;
		float tEnd = end[nodeInfo.m_Axis] - nodeInfo.m_Dist;
		float extents = ray.m_Extents[nodeInfo.m_Axis] + TEST_EPSILON;

		// no split... don't bother recursing..
		if ( (tStart < -extents ) && ( tEnd < -extents ) )
//		if ((tStart <= -extents) && (tEnd <= -extents) )
		{
			node = nodeInfo.m_Children[0];
			continue;
		}

		if ((tStart > extents) && (tEnd > extents) )
		{
			node = nodeInfo.m_Children[1];
			continue;
		}

		// For the segment of the line that we are going to use
		// to test against the back side of the plane, we're going
		// to use the part that goes from start to plane + extent
		// (which causes it to extend somewhat into the front halfspace,
		// since plane + extent is in the front halfspace).
		// Similarly, front the segment which tests against the front side,
		// we use the entire front side part of the ray + a portion of the ray that
		// extends by -extents into the back side.

		if (tStart == tEnd)
		{
			// Parallel case, send entire ray to both children...
			ret = EnumerateLeavesExtrudedRay_R( nodeInfo.m_Children[0], 
				ray, invDelta, start, end, pEnum, context );
			if (!ret)
				return false;
			return EnumerateLeavesExtrudedRay_R( nodeInfo.m_Children[1],
				ray, invDelta, start, end, pEnum, context );
		}

		// Compute the two fractions...
		// We need one at plane + extent and another at plane - extent.
		Assert( invDelta[nodeInfo.m_Axis] != 0.0f );
		float splitfracBehind = ( nodeInfo.m_Dist - extents - ray.m_Start[nodeInfo.m_Axis] ) *
			invDelta[nodeInfo.m_Axis];
		float splitfracInFront = ( nodeInfo.m_Dist + extents - ray.m_Start[nodeInfo.m_Axis] ) *
			invDelta[nodeInfo.m_Axis];
		splitfracBehind = clamp( splitfracBehind, 0.0f, 1.0f );
		splitfracInFront = clamp( splitfracInFront, 0.0f, 1.0f );
			
		// Compute the split points (0 == behind, 1 == in front)
		Vector split[2];
		VectorMA( ray.m_Start, splitfracBehind, ray.m_Delta, split[0] ); 
		VectorMA( ray.m_Start, splitfracInFront, ray.m_Delta, split[1] ); 

		// Moving from back to front
		int nearChild = (tStart < tEnd) ? 0 : 1;

		// For the segment of the line that we are going to use
		// to test against the back side of the plane, we're going
		// to use the part that goes from start to plane + extent
		// (which causes it to extend somewhat into the front halfspace,
		// since plane + extent is in the front halfspace).
		// Similarly, front the segment which tests against the front side,
		// we use the entire front side part of the ray + a portion of the ray that
		// extends by -extents into the back side.

		ret = EnumerateLeavesExtrudedRay_R( nodeInfo.m_Children[nearChild], 
			ray, invDelta, start, split[1 - nearChild], pEnum, context );
		if (!ret)
			return false;
		return EnumerateLeavesExtrudedRay_R( nodeInfo.m_Children[1 - nearChild],
			ray, invDelta, split[nearChild], end, pEnum, context );
	}

	int leaf = - node - 1;
	return pEnum->EnumerateLeaf( leaf, context );
}

bool CSpatialPartition::EnumerateLeavesAlongRay( const Ray_t& ray, 
									ISpatialLeafEnumerator* pEnum, int context )
{
	Vector invDelta, end;

	// Compute inverse delta; we'll need it.
	for (int i = 0; i < 3; ++i)
	{
		if (ray.m_Delta[i] != 0)
			invDelta[i] = 1.0f / ray.m_Delta[i];
		else
			invDelta[i] = 0.0f;
	}

	VectorAdd( ray.m_Start, ray.m_Delta, end );
	if (ray.m_IsRay)
	{
		return EnumerateLeavesRay_R( 0, ray, invDelta, ray.m_Start, end, pEnum, context );
	}
	else
	{
		return EnumerateLeavesExtrudedRay_R( 0, ray, invDelta, ray.m_Start, end, pEnum, context );
	}
}


//-----------------------------------------------------------------------------
// Inserts an element into the tree
//-----------------------------------------------------------------------------

void CSpatialPartition::InsertIntoTree( SpatialPartitionHandle_t handle, const Vector& mins, const Vector& maxs )
{
	m_Handle[handle].m_Min = mins;
	m_Handle[handle].m_Max = maxs;
	m_Handle[handle].m_TreeHandle = m_pTreeData->Insert( handle, m_Handle[handle].m_Min, m_Handle[handle].m_Max );
}

//-----------------------------------------------------------------------------
// Removes an element from the tree
//-----------------------------------------------------------------------------

void CSpatialPartition::RemoveFromTree( SpatialPartitionHandle_t handle )
{
	m_pTreeData->Remove( m_Handle[handle].m_TreeHandle );
}


//-----------------------------------------------------------------------------
// Call this when the element moves
//-----------------------------------------------------------------------------

static inline bool IsSameMultipleOfFactor( const Vector& src1, const Vector& src2,
										   const Vector& factor )
{
	if ( floor(src1[0] / factor[0]) != floor(src2[0] / factor[0]) )
		return false;
	if ( floor(src1[1] / factor[1]) != floor(src2[1] / factor[1]) )
		return false;
	if ( floor(src1[2] / factor[2]) != floor(src2[2] / factor[2]) )
		return false;

	return true;
}

static inline bool IsSameMultipleOfFactor2( const Vector& src1, const Vector& src2 )
{
	if ( floor(src1[0] * INV_SPATIAL_SIZE ) != floor(src2[0] * INV_SPATIAL_SIZE) )
		return false;
	if ( floor(src1[1] * INV_SPATIAL_SIZE ) != floor(src2[1] * INV_SPATIAL_SIZE) )
		return false;
	if ( floor(src1[2] * INV_SPATIAL_HEIGHT) != floor(src2[2] * INV_SPATIAL_HEIGHT) )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Call this when the element moves
//-----------------------------------------------------------------------------

void CSpatialPartition::ElementMoved( SpatialPartitionHandle_t handle, 
		const Vector& mins, const Vector& maxs )
{
	if (m_Handle[handle].m_TreeHandle == TREEDATA_INVALID_HANDLE)
	{
		InsertIntoTree( handle, mins, maxs );
		return;
	} 
 
	// Do an early out to see if there was no change...
#if 1
	Vector bucketSize( SPATIAL_SIZE, SPATIAL_SIZE, SPATIAL_HEIGHT );
	if ( IsSameMultipleOfFactor( mins, m_Handle[handle].m_Min, bucketSize ) &&
		 IsSameMultipleOfFactor( maxs, m_Handle[handle].m_Max, bucketSize ) )
#else
	// NJS: this is much faster than the above, but I got a crash once that I'm not sure is related or not,
	// so I'm leaving it disabled until I can further investigate the problem.
	if ( ( ( mins == m_Handle[handle].m_Min ) || IsSameMultipleOfFactor2( mins, m_Handle[handle].m_Min ) )
	   &&( ( maxs == m_Handle[handle].m_Max ) ||  IsSameMultipleOfFactor2( maxs, m_Handle[handle].m_Max ) )
	   )
 
#endif
	{
		// Still need to copy over the bbox though...
		m_Handle[handle].m_Min = mins;
		m_Handle[handle].m_Max = maxs;

		return;
	}

	// FIXME: We could try to very quickly find the bucket delta and only
	// remove/insert from those particular buckets
	// But we can't really do that easily now that I split the tree into
	// a separate class

	m_Handle[handle].m_Min = mins;
	m_Handle[handle].m_Max = maxs;

	m_pTreeData->ElementMoved( m_Handle[handle].m_TreeHandle, mins, maxs );
}

//-----------------------------------------------------------------------------
// Simple integer hash where the int is equal to the key
static bool Int_CompareFunc( const int &src1, const int &src2 )
{
	return ( src1 == src2 );
}


static unsigned int Int_KeyFunc( const int &src )
{
	return (unsigned)src;
}

//-----------------------------------------------------------------------------
// Base class for performing element enumeration
//-----------------------------------------------------------------------------
class CEnumBase : public ISpatialLeafEnumerator, public IBSPTreeDataEnumerator
{
public:

	// UNDONE: Allow construction based on a set of listIds or a mask?
	CEnumBase( SpatialPartitionListMask_t listMask, bool coarseTest, IPartitionEnumerator* pIterator )
	{
		m_pIterator = pIterator;
		m_ListMask = listMask;
		m_CoarseTest = coarseTest;
		m_pHash = NULL;
		// The enumeration context written into each handle is not reentrant.
		// s_NestLevel is a simple counter that checks for reentrant calls and
		// uses a hash table to store the visit status for nested calls
		if ( s_NestLevel > 0 )
		{
			m_pHash = new CUtlHash<int>( 64, 0, 0, Int_CompareFunc, Int_KeyFunc );
		}
		s_NestLevel++;
	}
	~CEnumBase()
	{
		delete m_pHash;
		m_pHash = NULL;
		s_NestLevel--;
	}

	bool EnumerateLeaf( int leaf, int context )
	{
		return g_SpatialPartition.EnumerateElementsInLeaf( leaf, this, context );
	}

	bool ShouldVisit( int partitionHandle, HandleInfo_t &handleInfo, int enumId )
	{
		// Keep going if this dude isn't in the list
		if ( !( m_ListMask & handleInfo.m_ListFlags ) )
			return false;

		if ( m_pHash )
		{
			if ( m_pHash->InvalidHandle() != m_pHash->Find( partitionHandle )  )
				return false;
			m_pHash->Insert( partitionHandle );
		}
		else
		{
			// Don't bother if we hit this one already...
			if ( handleInfo.m_EnumId == enumId )
				return false;

			// Mark this so we don't test it again
			handleInfo.m_EnumId = enumId;
		}

		return true;
	}

	bool FASTCALL EnumerateElement( int partitionHandle, int enumId )
	{
		// Get at the handle
		HandleInfo_t& handleInfo = g_SpatialPartition.HandleInfo( partitionHandle );

		if ( !ShouldVisit( partitionHandle, handleInfo, enumId ) )
			return true;

		// If it's not a coarse test, actually do the intersection
		if ( !m_CoarseTest )
		{
			if (!Intersect( handleInfo ))
				return true;
		}

		// Okay, this one is good...
		IterationRetval_t retVal = m_pIterator->EnumElement( handleInfo.m_pHandleEntity );

		return (retVal != ITERATION_STOP);
	}
	// Various versions must implement this
	virtual bool Intersect( HandleInfo_t& handleInfo ) = 0;
private:
	static int s_NestLevel;
protected:
	IPartitionEnumerator* m_pIterator;
	int		m_ListMask;
	bool	m_CoarseTest;
	CUtlHash<int> *m_pHash;
};

int CEnumBase::s_NestLevel = 0;


//-----------------------------------------------------------------------------
// Gets all entities surrounding a point
//-----------------------------------------------------------------------------
class CEnumPoint : public CEnumBase
{
public:
	CEnumPoint( SpatialPartitionListMask_t listMask, bool coarseTest, IPartitionEnumerator* pIterator, 
				const Vector& pt ) :
		CEnumBase( listMask, coarseTest, pIterator )
	{
		if (!m_CoarseTest)
		{
			VectorCopy( pt, m_Point );
		}
	}

	bool Intersect( HandleInfo_t& handleInfo )
	{
		return IsPointInBox( m_Point, handleInfo.m_Min, handleInfo.m_Max );
	}

private:
	Vector	m_Point;
};

void CSpatialPartition::EnumerateElementsAtPoint( SpatialPartitionListMask_t listMask, 
	const Vector& pt, bool coarseTest, IPartitionEnumerator* pIterator )
{
	// If this assertion fails, you're using a list
	// at a point where the spatial partition elements aren't set up!
	Assert( (listMask & m_nSuppressedListMask) == 0);
	
	// Early-out.
	if ( listMask == 0 )
		return;

	CEnumPoint enumPoint( listMask, coarseTest, pIterator, pt );
	++m_EnumId;
	m_pTreeData->EnumerateLeavesAtPoint( pt, &enumPoint, m_EnumId );
}


//-----------------------------------------------------------------------------
// Gets all entities in a box...
//-----------------------------------------------------------------------------
class CEnumBox : public CEnumBase
{
public:
	CEnumBox( SpatialPartitionListMask_t listMask, bool coarseTest, IPartitionEnumerator* pIterator, 
				const Vector& mins, const Vector& maxs ) :
		CEnumBase( listMask, coarseTest, pIterator )
	{
		if (!m_CoarseTest)
		{
			VectorCopy( mins, m_Min );
			VectorCopy( maxs, m_Max );
		}
	}

	bool Intersect( HandleInfo_t& handleInfo )
	{
		return IsBoxIntersectingBox( handleInfo.m_Min, handleInfo.m_Max,
			m_Min, m_Max );
	}

private:
	Vector	m_Min;
	Vector	m_Max;
};


void CSpatialPartition::EnumerateElementsInBox( SpatialPartitionListMask_t listMask, 
	const Vector& mins, const Vector& maxs, bool coarseTest, IPartitionEnumerator* pIterator )
{
	// If this assertion fails, you're using a list
	// at a point where the spatial partition elements aren't set up!
	Assert( (listMask & m_nSuppressedListMask) == 0);

	// Early-out.
	if ( listMask == 0 )
		return;

	CEnumBox enumBox( listMask, coarseTest, pIterator, mins, maxs );
	++m_EnumId;
	m_pTreeData->EnumerateLeavesInBox( mins, maxs, &enumBox, m_EnumId );
}


//-----------------------------------------------------------------------------
// Gets all entities in a sphere...
//-----------------------------------------------------------------------------
class CEnumSphere : public CEnumBase
{
public:
	CEnumSphere( SpatialPartitionListMask_t listMask, bool coarseTest, IPartitionEnumerator* pIterator, 
				const Vector& center, float radius ) :
		CEnumBase( listMask, coarseTest, pIterator )
	{
		if (!m_CoarseTest)
		{
			VectorCopy( center, m_Center );
			m_Radius = radius;
		}
	}

	bool Intersect( HandleInfo_t& handleInfo )
	{
		return IsBoxIntersectingSphere( handleInfo.m_Min, handleInfo.m_Max,
			m_Center, m_Radius );
	}

private:
	Vector	m_Center;
	float	m_Radius;
};

void CSpatialPartition::EnumerateElementsInSphere( SpatialPartitionListMask_t listMask, 
	const Vector& origin, float radius, bool coarseTest, IPartitionEnumerator* pIterator )
{
	// If this assertion fails, you're using a list
	// at a point where the spatial partition elements aren't set up!
	Assert( (listMask & m_nSuppressedListMask) == 0);
	
	// Early-out.
	if ( listMask == 0 )
		return;

	CEnumSphere enumSphere( listMask, coarseTest, pIterator, origin, radius );
	++m_EnumId;
	m_pTreeData->EnumerateLeavesInSphere( origin, radius, &enumSphere, m_EnumId );
}


//-----------------------------------------------------------------------------
// Gets all entities along a ray...
//-----------------------------------------------------------------------------

class CEnumRay : public CEnumBase
{
public:
	CEnumRay( SpatialPartitionListMask_t listMask, bool coarseTest, IPartitionEnumerator* pIterator, 
				const Ray_t& ray ) :
		CEnumBase( listMask, coarseTest, pIterator )
	{
		m_pRay = &ray;
	}

	bool Intersect( HandleInfo_t& handleInfo )
	{
		if (m_pRay->m_IsRay)
		{
			return IsBoxIntersectingRay( handleInfo.m_Min, handleInfo.m_Max, 
				m_pRay->m_Start, m_pRay->m_Delta );
		}
		else
		{
			// Thicken the box by the ray extents
			Vector bmin, bmax;
			VectorAdd( handleInfo.m_Max, m_pRay->m_Extents, bmax );
			VectorSubtract( handleInfo.m_Min, m_pRay->m_Extents, bmin );

			return IsBoxIntersectingRay( bmin, bmax, m_pRay->m_Start, m_pRay->m_Delta );
		}
	}

private:
	const Ray_t* m_pRay;
};

void CSpatialPartition::EnumerateElementsAlongRay( SpatialPartitionListMask_t listMask, 
	const Ray_t& ray, bool coarseTest, IPartitionEnumerator* pIterator )
{
	// If this assertion fails, you're using a list
	// at a point where the spatial partition elements aren't set up!
	Assert( (listMask & m_nSuppressedListMask) == 0);
	
	// Early-out.
	if ( listMask == 0 )
		return;
	
	// No ray? Just do a simpler box test
	if (!ray.m_IsSwept)
	{
		Vector mins, maxs;
		VectorSubtract( ray.m_Start, ray.m_Extents, mins );
		VectorAdd( ray.m_Start, ray.m_Extents, maxs );
		EnumerateElementsInBox( listMask, mins, maxs, coarseTest, pIterator );
		return;
	}

	CEnumRay enumRay( listMask, coarseTest, pIterator, ray );
	++m_EnumId;
	m_pTreeData->EnumerateLeavesAlongRay( ray, &enumRay, m_EnumId );
}



