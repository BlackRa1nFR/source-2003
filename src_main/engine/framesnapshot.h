//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( FRAMESNAPSHOT_H )
#define FRAMESNAPSHOT_H
#ifdef _WIN32
#pragma once
#endif

#include "mempool.h"

class PackedEntity;

//-----------------------------------------------------------------------------
// Purpose: Individual entity data, did the entity exist and what was it's serial number
//-----------------------------------------------------------------------------
class CFrameSnapshotEntry
{
public:
	bool					m_bExists;
	byte					m_nSerialNumber;
	string_t				m_ClassName;
};

typedef int PackedEntityHandle_t;

//-----------------------------------------------------------------------------
// Purpose: For all entities, stores whether the entity existed and what frame the
//  snapshot is for.  Also tracks whether the snapshot is still referenced.  When no
//  longer referenced, it's freed
//-----------------------------------------------------------------------------
class CFrameSnapshot
{
	DECLARE_FIXEDSIZE_ALLOCATOR( CFrameSnapshot );

public:

							CFrameSnapshot();
							~CFrameSnapshot();

	// Reference-counting.
	void					AddReference();
	void					ReleaseReference();
						

public:
	unsigned short			m_ListIndex;	// Index info CFrameSnapshotManager::m_FrameSnapshots.

	// Associated frame. This comes from host_tickcount.
	int						m_nTickNumber;

	// State information
	CFrameSnapshotEntry		m_Entities[ MAX_EDICTS ];

	// Keeps track of the fullpack info for this frame for all entities in any pvs
	PackedEntityHandle_t	m_pPackedData[ MAX_EDICTS ];


private:

	// Snapshots auto-delete themselves when their refcount goes to zero.
	unsigned short			m_nReferences;
};

//-----------------------------------------------------------------------------
// Purpose: Public interface to snapshot manager
//-----------------------------------------------------------------------------


class IFrameSnapshot
{
public:
	// Called when a level change happens
	virtual void			LevelChanged() = 0;

	// Called once per frame after simulation to store off all entities.
	// Note: the returned snapshot has a recount of 1 so you MUST call ReleaseReference on it.
	virtual CFrameSnapshot*	TakeTickSnapshot( int ticknumber ) = 0;

	// Creates pack data for a particular entity for a particular snapshot
	virtual PackedEntity*	CreatePackedEntity( CFrameSnapshot* pSnapshot, int entity ) = 0;

	// Do we have a particular packed entity for a particular snapshot
	virtual bool			IsValidPackedEntity( CFrameSnapshot* pSnapshot, int entity ) const = 0; 

	// Returns the pack data for a particular entity for a particular snapshot
	virtual PackedEntity*	GetPackedEntity( CFrameSnapshot* pSnapshot, int entity ) = 0;

	// Uses a previously sent packet
	virtual bool			UsePreviouslySentPacket( CFrameSnapshot* pSnapshot, 
													int entity, int entSerialNumber ) = 0;

	// Return the entity sitting in iEntity's slot if iSerialNumber matches its number.
	virtual PackedEntity*	GetPreviouslySentPacket( int iEntity, int iSerialNumber ) = 0;
};

extern IFrameSnapshot *framesnapshot;

#endif // FRAMESNAPSHOT_H
