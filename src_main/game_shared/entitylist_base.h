//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef ENTITYLIST_BASE_H
#define ENTITYLIST_BASE_H
#ifdef _WIN32
#pragma once
#endif


#include "const.h"
#include "serial_entity.h"
#include "basehandle.h"
#include "utllinkedlist.h"
#include "ihandleentity.h"


class CBaseEntityList
{
public:
	CBaseEntityList();
	~CBaseEntityList();
	
	// Add and remove entities. iForcedSerialNum should only be used on the client. The server
	// gets to dictate what the networkable serial numbers are on the client so it can send
	// ehandles over and they work.
	CBaseHandle AddNetworkableEntity( IHandleEntity *pEnt, int index, int iForcedSerialNum = -1 );
	CBaseHandle AddNonNetworkableEntity( IHandleEntity *pEnt );
	void RemoveEntity( CBaseHandle handle );

	// Get an ehandle from a networkable entity's index (note: if there is no entity in that slot,
	// then the ehandle will be invalid and produce NULL).
	CBaseHandle GetNetworkableHandle( int iEntity ) const;

	// ehandles use this in their Get() function to produce a pointer to the entity.
	IHandleEntity* LookupEntity( const CBaseHandle &handle ) const;
	IHandleEntity* LookupEntityByNetworkIndex( int edictIndex ) const;

	// Use these to iterate over all the entities.
	CBaseHandle FirstHandle() const;
	CBaseHandle NextHandle( CBaseHandle hEnt ) const;
	static CBaseHandle InvalidHandle();


// Overridables.
protected:

	// These are notifications to the derived class. It can cache info here if it wants.
	virtual void OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle );
	
	// It is safe to delete the entity here. We won't be accessing the pointer after
	// calling OnRemoveEntity.
	virtual void OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle );


private:

	CBaseHandle AddEntityAtSlot( IHandleEntity *pEnt, int iSlot, int iForcedSerialNum );
	void RemoveEntityAtSlot( int iSlot );

	
private:
	
	class CEntInfo : public CSerialEntity
	{
	public:
		unsigned short m_Index;	// Index into m_Entities.
	};

	// The first MAX_EDICTS entities are networkable. The rest are client-only or server-only.
	CEntInfo m_EntPtrArray[NUM_ENT_ENTRIES];

	// Currently-active entities. This indexes into m_EntPtrArray.
	CUtlLinkedList<unsigned short, unsigned short> m_Entities;

	// Slots in m_EntPtrArray above MAX_EDICTS that are free.
	CUtlLinkedList<unsigned short, unsigned short> m_FreeNonNetworkableSlots;
};


// ------------------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------------------ //

inline CBaseHandle CBaseEntityList::GetNetworkableHandle( int iEntity ) const
{
	Assert( iEntity >= 0 && iEntity < MAX_EDICTS );
	if ( m_EntPtrArray[iEntity].m_pEntity )
		return CBaseHandle( iEntity, m_EntPtrArray[iEntity].m_SerialNumber );
	else
		return CBaseHandle();
}


inline IHandleEntity* CBaseEntityList::LookupEntity( const CBaseHandle &handle ) const
{
	if ( handle.m_Index == INVALID_EHANDLE_INDEX )
		return NULL;

	const CEntInfo *pInfo = &m_EntPtrArray[ handle.GetEntryIndex() ];
	if ( pInfo->m_SerialNumber == handle.GetSerialNumber() )
		return (IHandleEntity*)pInfo->m_pEntity;
	else
		return NULL;
}


inline IHandleEntity* CBaseEntityList::LookupEntityByNetworkIndex( int edictIndex ) const
{
	// (Legacy support).
	if ( edictIndex < 0 )
		return NULL;

	Assert( edictIndex < NUM_ENT_ENTRIES );
	return (IHandleEntity*)m_EntPtrArray[edictIndex].m_pEntity;
}


inline CBaseHandle CBaseEntityList::FirstHandle() const
{
	if ( m_Entities.Head() == m_Entities.InvalidIndex() )
		return INVALID_EHANDLE_INDEX;

	int index = m_Entities[ m_Entities.Head() ];
	return CBaseHandle( index, m_EntPtrArray[index].m_SerialNumber );
}

inline CBaseHandle CBaseEntityList::NextHandle( CBaseHandle hEnt ) const
{
	int iSlot = hEnt.GetEntryIndex();
	int iNext = m_Entities.Next( m_EntPtrArray[iSlot].m_Index );
	if ( iNext == m_Entities.InvalidIndex() )
		return INVALID_EHANDLE_INDEX;

	int index = m_Entities[iNext];
	return CBaseHandle( index, m_EntPtrArray[index].m_SerialNumber );
}
	
inline CBaseHandle CBaseEntityList::InvalidHandle()
{
	return INVALID_EHANDLE_INDEX;
}


extern CBaseEntityList *g_pEntityList;

#endif // ENTITYLIST_BASE_H
