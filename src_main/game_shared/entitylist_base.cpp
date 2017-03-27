//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "entitylist_base.h"
#include "ihandleentity.h"


CBaseEntityList::CBaseEntityList()
{
	// Initially, all the slots are free.
	for ( unsigned short i=MAX_EDICTS; i < NUM_ENT_ENTRIES; i++ )
	{
		m_FreeNonNetworkableSlots.AddToTail( i );
	}
	memset( m_EntPtrArray, 0, sizeof( m_EntPtrArray ) );
}


CBaseEntityList::~CBaseEntityList()
{
	// Remove any active entities we have.
	unsigned short iNext;
	for ( unsigned short iCur=m_Entities.Head(); iCur != m_Entities.InvalidIndex(); iCur = iNext )
	{
		iNext = m_Entities.Next( iCur );
		RemoveEntityAtSlot( m_Entities[iCur] );
	}
}


CBaseHandle CBaseEntityList::AddNetworkableEntity( IHandleEntity *pEnt, int index, int iForcedSerialNum )
{
	Assert( index >= 0 && index < MAX_EDICTS );
	return AddEntityAtSlot( pEnt, index, iForcedSerialNum );
}


CBaseHandle CBaseEntityList::AddNonNetworkableEntity( IHandleEntity *pEnt )
{
	// Find a slot for it.
	if ( m_FreeNonNetworkableSlots.Count() == 0 )
		Error( "CBaseEntityList::AddNonNetworkableEntity: no free slots!" );

	// Move from the free list into the allocated list.
	unsigned short iSlot = m_FreeNonNetworkableSlots[ m_FreeNonNetworkableSlots.Head() ];
	m_FreeNonNetworkableSlots.Remove( m_FreeNonNetworkableSlots.Head() );
	
	return AddEntityAtSlot( pEnt, iSlot, -1 );
}


void CBaseEntityList::RemoveEntity( CBaseHandle handle )
{
	RemoveEntityAtSlot( handle.GetEntryIndex() );
}


CBaseHandle CBaseEntityList::AddEntityAtSlot( IHandleEntity *pEnt, int iSlot, int iForcedSerialNum )
{
	// Init the CSerialEntity.
	CEntInfo *pSlot = &m_EntPtrArray[iSlot];
	Assert( pSlot->m_pEntity == NULL );
	pSlot->m_pEntity = pEnt;

	// Force the serial number (client-only)?
	if ( iForcedSerialNum != -1 )
	{
		pSlot->m_SerialNumber = iForcedSerialNum;
		
		#if !defined( CLIENT_DLL )
			// Only the client should force the serial numbers.
			Assert( false );
		#endif
	}
	
	// Update our list of active entities.
	pSlot->m_Index = m_Entities.AddToTail( iSlot );

	CBaseHandle retVal( iSlot, pSlot->m_SerialNumber );

	// Tell the entity to store its handle.
	pEnt->SetRefEHandle( retVal );

	// Notify any derived class.
	OnAddEntity( pEnt, retVal );
	return retVal;
}


void CBaseEntityList::RemoveEntityAtSlot( int iSlot )
{
	Assert( iSlot >= 0 && iSlot < NUM_ENT_ENTRIES );

	CEntInfo *pInfo = &m_EntPtrArray[iSlot];

	if ( pInfo->m_pEntity )
	{
		pInfo->m_pEntity->SetRefEHandle( INVALID_EHANDLE_INDEX );

		// Notify the derived class that we're about to remove this entity.
		OnRemoveEntity( pInfo->m_pEntity, CBaseHandle( iSlot, pInfo->m_SerialNumber ) );

		// Increment the serial # so ehandles go invalid.
		pInfo->m_pEntity = NULL;
		pInfo->m_SerialNumber++;

		m_Entities.Remove( pInfo->m_Index );

		// Add the slot back to the free list if it's a non-networkable entity.
		if ( iSlot >= MAX_EDICTS )
			m_FreeNonNetworkableSlots.AddToTail( iSlot );
	}
}


void CBaseEntityList::OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
}



void CBaseEntityList::OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
}
