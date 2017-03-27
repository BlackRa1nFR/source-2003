//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "server.h"
#include "framesnapshot.h"
#include "edict.h"
#include "const.h"
#include "utllinkedlist.h"
#include "sys_dll.h"


DEFINE_FIXEDSIZE_ALLOCATOR( CFrameSnapshot, 64, 64 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CFrameSnapshotManager : public IFrameSnapshot
{
friend class CFrameSnapshot;

public:
	CFrameSnapshotManager( void );
	virtual ~CFrameSnapshotManager( void );

// IFrameSnapshot implementation.
public:

	// Called when a level change happens
	virtual void			LevelChanged();

	virtual CFrameSnapshot*	TakeTickSnapshot( int ticknumber );
	
	virtual PackedEntity*	CreatePackedEntity( CFrameSnapshot* pSnapshot, int entity );
	virtual bool			IsValidPackedEntity( CFrameSnapshot* pSnapshot, int entity ) const;

	virtual PackedEntity*	GetPackedEntity( CFrameSnapshot* pSnapshot, int entity );

	virtual bool			UsePreviouslySentPacket( CFrameSnapshot* pSnapshot, int entity, int entSerialNumber );
	virtual PackedEntity*	GetPreviouslySentPacket( int iEntity, int iSerialNumber );

private:
	void	DestroyPackedEntity( PackedEntityHandle_t handle );
	void	DeleteFrameSnapshot( CFrameSnapshot* pSnapshot );
	
	CUtlLinkedList<CFrameSnapshot*, unsigned short>			m_FrameSnapshots;
	CUtlLinkedList< PackedEntity, PackedEntityHandle_t >	m_PackedEntities; 

	// The most recently sent packets for each entity
	PackedEntityHandle_t	m_pPackedData[ MAX_EDICTS ];
	byte					m_pSerialNumber[ MAX_EDICTS ];
};

// Expose interface
static CFrameSnapshotManager g_FrameSnapshotManager;
IFrameSnapshot *framesnapshot = ( IFrameSnapshot * )&g_FrameSnapshotManager;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFrameSnapshotManager::CFrameSnapshotManager( void )
{
	memset( m_pPackedData, 0xFF, MAX_EDICTS * sizeof(PackedEntityHandle_t) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFrameSnapshotManager::~CFrameSnapshotManager( void )
{
#ifdef _DEBUG
	if ( IsInErrorExit() )
	{
		// These may have been freed already. Don't crash when freeing these.
		memset( &m_PackedEntities, 0, sizeof( m_PackedEntities ) );
	}
	else
	{
		Assert( m_FrameSnapshots.Count() == 0 );
	}
#endif
}

//-----------------------------------------------------------------------------
// Called when a level change happens
//-----------------------------------------------------------------------------

void CFrameSnapshotManager::LevelChanged()
{
	// Clear all lists...
	Assert( m_FrameSnapshots.Count() == 0 );

	// Release the most recent snapshot...
	m_PackedEntities.RemoveAll();
	memset( m_pPackedData, 0xFF, MAX_EDICTS * sizeof(PackedEntityHandle_t) );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : framenumber - 
//-----------------------------------------------------------------------------
CFrameSnapshot* CFrameSnapshotManager::TakeTickSnapshot( int ticknumber )
{
	CFrameSnapshot *snap = new CFrameSnapshot;
	snap->AddReference();
	snap->m_nTickNumber = ticknumber;
	memset( snap->m_Entities, 0, sizeof( snap->m_Entities ) );

	edict_t *ed;

	for ( int i = 0; i < sv.num_edicts; i++ )
	{
		CFrameSnapshotEntry *entry = &snap->m_Entities[ i ];
		ed = &sv.edicts[ i ];

		// FIXME, this will cause EF_NODRAW entities to behave like
		//  they need to be deleted from the client, even though
		//  the client was never sent the entity
		entry->m_bExists = ed->m_pEnt && !ed->free;
		entry->m_nSerialNumber = ed->serial_number;
		entry->m_ClassName = ed->classname;
	}

	// Blat out packed data
	memset( snap->m_pPackedData, 0xFF, MAX_EDICTS * sizeof(PackedEntityHandle_t) );

	snap->m_ListIndex = m_FrameSnapshots.AddToTail( snap );
	return snap;
}

//-----------------------------------------------------------------------------
// Cleans up packed entity data
//-----------------------------------------------------------------------------

void CFrameSnapshotManager::DeleteFrameSnapshot( CFrameSnapshot* pSnapshot )
{
	// Decrement reference counts of all packed entities
	for (int i = 0; i < MAX_EDICTS; ++i)
	{
		if (pSnapshot->m_pPackedData[i] != m_PackedEntities.InvalidIndex())
		{
			if (--m_PackedEntities[ pSnapshot->m_pPackedData[i] ].m_ReferenceCount <= 0)
			{
				DestroyPackedEntity( pSnapshot->m_pPackedData[i] );
			}
		}
	}

	m_FrameSnapshots.Remove( pSnapshot->m_ListIndex );
	delete pSnapshot;
}


//-----------------------------------------------------------------------------
// shared packed entity pool for snapshots
//-----------------------------------------------------------------------------

void CFrameSnapshotManager::DestroyPackedEntity( PackedEntityHandle_t handle )
{
	m_PackedEntities.Remove(handle);
}

bool CFrameSnapshotManager::UsePreviouslySentPacket( CFrameSnapshot* pSnapshot, 
											int entity, int entSerialNumber )
{
	PackedEntityHandle_t handle = m_pPackedData[entity]; 
	if ( handle != m_PackedEntities.InvalidIndex() )
	{
		// NOTE: We can't use the previously sent packet if there was a 
		// serial number change....
		if ( m_pSerialNumber[entity] == entSerialNumber )
		{
			pSnapshot->m_pPackedData[entity] = handle;
			++m_PackedEntities[handle].m_ReferenceCount;
			return true;
		}
	}
	
	return false;
}


PackedEntity* CFrameSnapshotManager::GetPreviouslySentPacket( int iEntity, int iSerialNumber )
{
	PackedEntityHandle_t handle = m_pPackedData[iEntity]; 
	if ( handle != m_PackedEntities.InvalidIndex() )
	{
		// NOTE: We can't use the previously sent packet if there was a 
		// serial number change....
		if ( m_pSerialNumber[iEntity] == iSerialNumber )
		{
			return &m_PackedEntities[handle];
		}
	}
	
	return NULL;
}


//-----------------------------------------------------------------------------
// Do we have pack data for a particular entity for a particular snapshot?
//-----------------------------------------------------------------------------

bool CFrameSnapshotManager::IsValidPackedEntity( CFrameSnapshot* pSnapshot, int entity ) const
{
	return m_PackedEntities.IsValidIndex(pSnapshot->m_pPackedData[entity]);
}


//-----------------------------------------------------------------------------
// Returns the pack data for a particular entity for a particular snapshot
//-----------------------------------------------------------------------------

PackedEntity* CFrameSnapshotManager::CreatePackedEntity( CFrameSnapshot* pSnapshot, int entity )
{
	PackedEntityHandle_t handle = m_PackedEntities.AddToTail();

	// Referenced twice: in the mru 
	m_PackedEntities[handle].m_ReferenceCount = 2;
	pSnapshot->m_pPackedData[entity] = handle;

	// Add a reference into the global list of last entity packets seen...
	// and remove the reference to the last entity packet we saw
	if (m_pPackedData[entity] != m_PackedEntities.InvalidIndex())
	{
		if (--m_PackedEntities[ m_pPackedData[entity] ].m_ReferenceCount <= 0)
		{
			DestroyPackedEntity( m_pPackedData[entity] );
		}
	}
	m_pPackedData[entity] = handle;
	m_pSerialNumber[entity] = pSnapshot->m_Entities[entity].m_nSerialNumber;

	return &m_PackedEntities[handle];
}

//-----------------------------------------------------------------------------
// Returns the pack data for a particular entity for a particular snapshot
//-----------------------------------------------------------------------------

PackedEntity* CFrameSnapshotManager::GetPackedEntity( CFrameSnapshot* pSnapshot, int entity )
{
	return &m_PackedEntities[pSnapshot->m_pPackedData[entity]];
}




// ------------------------------------------------------------------------------------------------ //
// CFrameSnapshot
// ------------------------------------------------------------------------------------------------ //

#if defined( _DEBUG )
	int g_nAllocatedSnapshots = 0;
#endif


CFrameSnapshot::CFrameSnapshot()
{
	m_nReferences = 0;

#if defined( _DEBUG )
	++g_nAllocatedSnapshots;
	Assert( g_nAllocatedSnapshots < 80000 ); // this probably would indicate a memory leak.
#endif
}


CFrameSnapshot::~CFrameSnapshot()
{
#if defined( _DEBUG )
	--g_nAllocatedSnapshots;
	Assert( g_nAllocatedSnapshots >= 0 );
#endif
}


void CFrameSnapshot::AddReference()
{
	Assert( m_nReferences < 0xFFFF );
	++m_nReferences;
}

void CFrameSnapshot::ReleaseReference()
{
	Assert( m_nReferences > 0 );
	--m_nReferences;
	if ( m_nReferences == 0 )
	{
		g_FrameSnapshotManager.DeleteFrameSnapshot( this );
	}
}


