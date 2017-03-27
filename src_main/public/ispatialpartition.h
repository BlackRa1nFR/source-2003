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
//=============================================================================

#ifndef ISPATIALPARTITION_H
#define ISPATIALPARTITION_H

#include "interface.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class Vector;
struct Ray_t;
class IHandleEntity;


#define INTERFACEVERSION_SPATIALPARTITION	"SpatialPartition001"

//-----------------------------------------------------------------------------
// These are the various partition lists. Note some are server only, some
// are client only
//-----------------------------------------------------------------------------

enum
{
	PARTITION_ENGINE_SOLID_EDICTS		= (1 << 0),		// every edict_t that isn't SOLID_TRIGGER or SOLID_NOT (and static props)
	PARTITION_ENGINE_TRIGGER_EDICTS		= (1 << 1),		// every edict_t that IS SOLID_TRIGGER
	PARTITION_CLIENT_SOLID_EDICTS		= (1 << 2),
	PARTITION_CLIENT_RESPONSIVE_EDICTS	= (1 << 3),		// these are client-side only objects that respond to being forces, etc.
	PARTITION_ENGINE_NON_STATIC_EDICTS	= (1 << 4),		// everything in solid & trigger except the static props, includes SOLID_NOTs
	PARTITION_CLIENT_STATIC_PROPS		= (1 << 5),
	PARTITION_ENGINE_STATIC_PROPS		= (1 << 6),
	PARTITION_CLIENT_NON_STATIC_EDICTS	= (1 << 7),		// everything except the static props
};

// Use this to look for all client edicts.
#define PARTITION_ALL_CLIENT_EDICTS	(		\
	PARTITION_CLIENT_NON_STATIC_EDICTS |	\
	PARTITION_CLIENT_STATIC_PROPS |			\
	PARTITION_CLIENT_RESPONSIVE_EDICTS |	\
	PARTITION_CLIENT_SOLID_EDICTS			\
	)

//-----------------------------------------------------------------------------
// Clients that want to know about all elements within a particular
// volume must inherit from this
//-----------------------------------------------------------------------------

enum IterationRetval_t
{
	ITERATION_CONTINUE = 0,
	ITERATION_STOP,
};


typedef unsigned short SpatialPartitionHandle_t;

// A combination of the PARTITION_ flags above.
typedef int SpatialPartitionListMask_t;	

typedef int SpatialTempHandle_t;


//-----------------------------------------------------------------------------
// Any search in the CSpatialPartition must use this to filter out entities it doesn't want.
// You're forced to use listMasks because it can filter by listMasks really fast. Any other
// filtering can be done by EnumElement.
//-----------------------------------------------------------------------------
class IPartitionEnumerator
{
public:
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity ) = 0;
};


//-----------------------------------------------------------------------------
// This is the spatial partition manager, groups objects into buckets
//-----------------------------------------------------------------------------
enum
{
	PARTITION_INVALID_HANDLE = (SpatialPartitionHandle_t)~0
};


class ISpatialPartition
{
public:
	// Create/destroy a handle for this dude in our system. Destroy
	// will also remove it from all lists it happens to be in
	virtual SpatialPartitionHandle_t CreateHandle( IHandleEntity *pHandleEntity ) = 0;

	// A fast method of creating a handle + inserting into the tree in the right place
	virtual SpatialPartitionHandle_t CreateHandle( IHandleEntity *pHandleEntity,
		SpatialPartitionListMask_t listMask, const Vector& mins, const Vector& maxs ) = 0; 

	virtual void DestroyHandle( SpatialPartitionHandle_t handle ) = 0;

	// Adds, removes an handle from a particular spatial partition list
	// There can be multiple partition lists; each has a unique id
	virtual void Insert( SpatialPartitionListMask_t listMask, 
		SpatialPartitionHandle_t handle ) = 0;
	virtual void Remove( SpatialPartitionListMask_t listMask, 
		SpatialPartitionHandle_t handle ) = 0;

	// This will remove a particular handle from all lists
	virtual void Remove( SpatialPartitionHandle_t handle ) = 0;

	// Call this when an entity moves...
	virtual void ElementMoved( SpatialPartitionHandle_t handle, 
		const Vector& mins, const Vector& maxs ) = 0;

	// A fast method to insert + remove a handle from the tree...
	// This is used to suppress collision of a single model..
	virtual SpatialTempHandle_t FastRemove( SpatialPartitionHandle_t handle ) = 0;
	virtual void FastInsert( SpatialPartitionHandle_t handle, SpatialTempHandle_t tempHandle ) = 0;
	
	// Gets all entities in a particular volume...
	// if coarseTest == true, it'll return all elements that are in
	// spatial partitions that intersect the box
	// if coarseTest == false, it'll return only elements that truly intersect
	virtual void EnumerateElementsInBox(
		SpatialPartitionListMask_t listMask,  
		const Vector& mins, 
		const Vector& maxs, 
		bool coarseTest, 
		IPartitionEnumerator* pIterator 
		) = 0;

	virtual void EnumerateElementsInSphere(
		SpatialPartitionListMask_t listMask, 
		const Vector& origin, 
		float radius, 
		bool coarseTest, 
		IPartitionEnumerator* pIterator 
		) = 0;

	virtual void EnumerateElementsAlongRay(
		SpatialPartitionListMask_t listMask, 
		const Ray_t& ray, 
		bool coarseTest, 
		IPartitionEnumerator* pIterator 
		) = 0;

	virtual void EnumerateElementsAtPoint( 
		SpatialPartitionListMask_t listMask, 
		const Vector& pt, 
		bool coarseTest, 
		IPartitionEnumerator* pIterator
		) = 0;

	// For debugging.... suppress queries on particular lists
	virtual void SuppressLists( SpatialPartitionListMask_t nListMask, bool bSuppress ) = 0;
	virtual SpatialPartitionListMask_t GetSuppressedLists() = 0;
};

#endif



