//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

CClientThinkList g_ClientThinkList;


CClientThinkList::CClientThinkList()
{
}


CClientThinkList::~CClientThinkList()
{
}


void CClientThinkList::SetNextClientThink( ClientEntityHandle_t hEnt, float nextTime )
{
	if ( nextTime == CLIENT_THINK_NEVER )
	{
		RemoveThinkable( hEnt );
	}
	else
	{
		IClientThinkable *pThink = ClientEntityList().GetClientThinkableFromHandle( hEnt );
		if ( pThink )
		{
			ClientThinkHandle_t hThink = pThink->GetThinkHandle();

			// Add it to the list if it's not already in there.
			if ( hThink == INVALID_THINK_HANDLE )
			{
				hThink = (ClientThinkHandle_t)m_ThinkEntries.AddToTail();
				pThink->SetThinkHandle( hThink );

				GetThinkEntry( hThink )->m_hEnt = hEnt;
			}

			// Set the next think time..
			GetThinkEntry( hThink )->m_flNextClientThink = nextTime;
		}
	}
}


void CClientThinkList::RemoveThinkable( ClientEntityHandle_t hEnt )
{
	IClientThinkable *pThink = ClientEntityList().GetClientThinkableFromHandle( hEnt );
	if ( pThink )
	{
		ClientThinkHandle_t hThink = pThink->GetThinkHandle();

		// It will never think so make sure it's NOT in the list.
		if ( hThink != INVALID_THINK_HANDLE )
		{
			m_ThinkEntries.Remove( (unsigned long)hThink );
			pThink->SetThinkHandle( INVALID_THINK_HANDLE );
		}
	}
}


bool CClientThinkList::Init()
{
	return true;
}


void CClientThinkList::Shutdown()
{
}



void CClientThinkList::LevelInitPreEntity()
{
}



void CClientThinkList::LevelShutdownPreEntity()
{
}


void CClientThinkList::LevelShutdownPostEntity()
{
}


void CClientThinkList::PreRender()
{
}


void CClientThinkList::Update( float frametime )
{
}


void CClientThinkList::PerformThinkFunctions()
{
	float curtime = gpGlobals->curtime;

	unsigned long iNext;
	for ( unsigned long iCur=m_ThinkEntries.Head(); iCur != m_ThinkEntries.InvalidIndex(); iCur = iNext )
	{
		iNext = m_ThinkEntries.Next( iCur );

		CThinkEntry *pEntry = &m_ThinkEntries[iCur];
		
		IClientThinkable *pThink = ClientEntityList().GetClientThinkableFromHandle( pEntry->m_hEnt );
		if ( pThink )
		{
			if ( pEntry->m_flNextClientThink == CLIENT_THINK_ALWAYS )
			{
				// NOTE: The Think function here could call SetNextClientThink
				// which would cause it to be removed + readded into the list
				pThink->ClientThink();

				// NOTE: The Think() call can cause other things to be added to the Think
				// list, which could reallocate memory and invalidate the pEntry pointer
				pEntry = &m_ThinkEntries[iCur];
			}
			else if ( pEntry->m_flNextClientThink < curtime )
			{
				pEntry->m_flNextClientThink = curtime;

				// NOTE: The Think function here could call SetNextClientThink
				// which would cause it to be readded into the list
				pThink->ClientThink();

				// NOTE: The Think() call can cause other things to be added to the Think
				// list, which could reallocate memory and invalidate the pEntry pointer
				pEntry = &m_ThinkEntries[iCur];

				// If they haven't changed the think time, then it should be removed.
				if ( pEntry->m_flNextClientThink == curtime )
				{
					RemoveThinkable( pEntry->m_hEnt );
				}
			}

			Assert( pEntry == &m_ThinkEntries[iCur] );

			// Set this after the Think calls in case they look at LastClientThink
			m_ThinkEntries[iCur].m_flLastClientThink = curtime;
		}
		else
		{
			// This should be almost impossible. When ClientEntityHandle_t's are versioned,
			// this should be totally impossible.
			Assert( false );
			m_ThinkEntries.Remove( iCur );
		}
	}

	// Clear out the client-side entity deletion list.
	CleanUpDeleteList();
}

void CClientThinkList::AddToDeleteList( ClientEntityHandle_t hEnt )
{
	// Sanity check!
	Assert( hEnt != ClientEntityList().InvalidHandle() );
	if ( hEnt == ClientEntityList().InvalidHandle() )
		return;

	// Check to see if entity is networkable -- don't let it release!
	C_BaseEntity *pEntity = ClientEntityList().GetBaseEntityFromHandle( hEnt );
	if ( pEntity )
	{
		// Check to see if the entity is already being removed!
		if ( pEntity->IsMarkedForDeletion() )
			return;

		// Don't add networkable entities to delete list -- the server should
		// take care of this.  The delete list is for client-side only entities.
		if ( !pEntity->GetClientNetworkable() )
		{
			m_aDeleteList.AddToTail( hEnt );
			pEntity->SetRemovalFlag( true );
		}
	}
}

void CClientThinkList::RemoveFromDeleteList( ClientEntityHandle_t hEnt )
{
	// Sanity check!
	Assert( hEnt != ClientEntityList().InvalidHandle() );
	if ( hEnt == ClientEntityList().InvalidHandle() )
		return;

	int nSize = m_aDeleteList.Count();
	for ( int iHandle = 0; iHandle < nSize; ++iHandle )
	{
		if ( m_aDeleteList[iHandle] == hEnt )
		{
			m_aDeleteList[iHandle] = ClientEntityList().InvalidHandle();

			C_BaseEntity *pEntity = ClientEntityList().GetBaseEntityFromHandle( hEnt );
			if ( pEntity )
			{
				pEntity->SetRemovalFlag( false );
			}
		}
	}
}

void CClientThinkList::CleanUpDeleteList()
{
	int nThinkCount = m_aDeleteList.Count();
	for ( int iThink = 0; iThink < nThinkCount; ++iThink )
	{
		ClientEntityHandle_t handle = m_aDeleteList[iThink];
		if ( handle != ClientEntityList().InvalidHandle() )
		{
			C_BaseEntity *pEntity = ClientEntityList().GetBaseEntityFromHandle( handle );
			if ( pEntity )
			{
				pEntity->SetRemovalFlag( false );
			}

			IClientThinkable *pThink = ClientEntityList().GetClientThinkableFromHandle( handle );
			if ( pThink )
			{
				pThink->Release();
			}
		}
	}

	m_aDeleteList.RemoveAll();
}

