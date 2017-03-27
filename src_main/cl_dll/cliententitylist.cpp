//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: a global list of all the entities in the game.  All iteration through
//			entities is done through this object.
//-----------------------------------------------------------------------------
#include "cbase.h"


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

// Create interface
static CClientEntityList s_EntityList;
CBaseEntityList *g_pEntityList = &s_EntityList;

// Expose list to engine
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientEntityList, IClientEntityList, VCLIENTENTITYLIST_INTERFACE_VERSION, s_EntityList );

// Store local pointer to interface for rest of client .dll only 
//  (CClientEntityList instead of IClientEntityList )
CClientEntityList *cl_entitylist = &s_EntityList; 


								 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClientEntityList::CClientEntityList( void )
{
	m_iMaxUsedServerIndex = -1;
	m_iMaxServerEnts = 0;
	Release();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClientEntityList::~CClientEntityList( void )
{
	Release();
}

//-----------------------------------------------------------------------------
// Purpose: Clears all entity lists and releases entities
//-----------------------------------------------------------------------------
void CClientEntityList::Release( void )
{
	// Free all the entities.
	ClientEntityHandle_t iter = FirstHandle();
	ClientEntityHandle_t next;
	while( iter != InvalidHandle() )
	{
		next = NextHandle( iter );
		
		// Try to call release on anything we can.
		IClientNetworkable *pNet = GetClientNetworkableFromHandle( iter );
		if ( pNet )
		{
			pNet->Release();
		}
		else
		{
			// Try to call release on anything we can.
			IClientThinkable *pThinkable = GetClientThinkableFromHandle( iter );
			if ( pThinkable )
			{
				pThinkable->Release();
			}
		}
		RemoveEntity( iter );

		iter = next;
	}

	m_iNumServerEnts = 0;
	m_iMaxServerEnts = 0;
	m_iMaxUsedServerIndex = -1;
}

IClientNetworkable* CClientEntityList::GetClientNetworkable( int entnum )
{
	Assert( entnum >= 0 );
	Assert( entnum < MAX_EDICTS );
	return m_EntityCacheInfo[entnum].m_pNetworkable;
}


IClientEntity* CClientEntityList::GetClientEntity( int entnum )
{
	IClientUnknown *pEnt = GetListedEntity( entnum );
	return pEnt ? pEnt->GetIClientEntity() : 0;
}


int CClientEntityList::NumberOfEntities( void )
{
	return m_iNumServerEnts;
}


void CClientEntityList::SetMaxEntities( int maxents )
{
	m_iMaxServerEnts = maxents;
}


int CClientEntityList::GetMaxEntities( void )
{
	return m_iMaxServerEnts;
}

//-----------------------------------------------------------------------------
// Purpose: Because m_iNumServerEnts != last index
// Output : int
//-----------------------------------------------------------------------------
int CClientEntityList::GetHighestEntityIndex( void )
{
	return m_iMaxUsedServerIndex;
}

void CClientEntityList::RecomputeHighestEntityUsed( void )
{
	m_iMaxUsedServerIndex = -1;

	// Walk backward looking for first valid index
	int i;
	for ( i = MAX_EDICTS - 1; i >= 0; i-- )
	{
		if ( GetListedEntity( i ) != NULL )
		{
			m_iMaxUsedServerIndex = i;
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Add a raw C_BaseEntity to the entity list.
// Input  : index - 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------

C_BaseEntity* CClientEntityList::GetBaseEntity( int entnum )
{
	IClientUnknown *pEnt = GetListedEntity( entnum );
	return pEnt ? pEnt->GetBaseEntity() : 0;
}


ICollideable* CClientEntityList::GetClientCollideable( int entnum )
{
	IClientUnknown *pEnt = GetListedEntity( entnum );
	return pEnt ? pEnt->GetClientCollideable() : 0;
}


IClientNetworkable* CClientEntityList::GetClientNetworkableFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetClientNetworkable() : 0;
}


IClientEntity* CClientEntityList::GetClientEntityFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetIClientEntity() : 0;
}


IClientRenderable* CClientEntityList::GetClientRenderableFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetClientRenderable() : 0;
}


C_BaseEntity* CClientEntityList::GetBaseEntityFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetBaseEntity() : 0;
}


ICollideable* CClientEntityList::GetClientCollideableFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetClientCollideable() : 0;
}


IClientThinkable* CClientEntityList::GetClientThinkableFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetClientThinkable() : 0;
}


void CClientEntityList::OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
	int entnum = handle.GetEntryIndex();
	EntityCacheInfo_t *pCache = &m_EntityCacheInfo[entnum];

	if ( entnum >= 0 && entnum < MAX_EDICTS )
	{
		// Update our counters.
		m_iNumServerEnts++;
		if ( entnum > m_iMaxUsedServerIndex )
		{
			m_iMaxUsedServerIndex = entnum;
		}


		// Cache its networkable pointer.
		Assert( dynamic_cast< IClientUnknown* >( pEnt ) );
		Assert( ((IClientUnknown*)pEnt)->GetClientNetworkable() ); // Server entities should all be networkable.
		pCache->m_pNetworkable = ((IClientUnknown*)pEnt)->GetClientNetworkable();
	}

	// Store it in a special list for fast iteration if it's a C_BaseEntity.
	IClientUnknown *pUnknown = (IClientUnknown*)pEnt;
	C_BaseEntity *pBaseEntity = pUnknown->GetBaseEntity();
	if ( pBaseEntity )
	{
		pCache->m_BaseEntitiesIndex = m_BaseEntities.AddToTail( pBaseEntity );
	}
	else
	{
		pCache->m_BaseEntitiesIndex = m_BaseEntities.InvalidIndex();
	}
}


void CClientEntityList::OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
	int entnum = handle.GetEntryIndex();
	EntityCacheInfo_t *pCache = &m_EntityCacheInfo[entnum];

	if ( entnum >= 0 && entnum < MAX_EDICTS )
	{
		// This is a networkable ent. Clear out our cache info for it.
		pCache->m_pNetworkable = NULL;
		m_iNumServerEnts--;

		if ( entnum >= m_iMaxUsedServerIndex )
		{
			RecomputeHighestEntityUsed();
		}
	}

	if ( pCache->m_BaseEntitiesIndex != m_BaseEntities.InvalidIndex() )
		m_BaseEntities.Remove( pCache->m_BaseEntitiesIndex );

	pCache->m_BaseEntitiesIndex = m_BaseEntities.InvalidIndex();
}



// -------------------------------------------------------------------------------------------------- //
// C_AllBaseEntityIterator
// -------------------------------------------------------------------------------------------------- //
C_AllBaseEntityIterator::C_AllBaseEntityIterator()
{
	Restart();
}


void C_AllBaseEntityIterator::Restart()
{
	m_CurBaseEntity = ClientEntityList().m_BaseEntities.Head();
}

	
C_BaseEntity* C_AllBaseEntityIterator::Next()
{
	if ( m_CurBaseEntity == ClientEntityList().m_BaseEntities.InvalidIndex() )
		return NULL;

	C_BaseEntity *pRet = ClientEntityList().m_BaseEntities[m_CurBaseEntity];
	m_CurBaseEntity = ClientEntityList().m_BaseEntities.Next( m_CurBaseEntity );
	return pRet;
}


// -------------------------------------------------------------------------------------------------- //
// C_BaseEntityIterator
// -------------------------------------------------------------------------------------------------- //
C_BaseEntityIterator::C_BaseEntityIterator()
{
	Restart();
}

void C_BaseEntityIterator::Restart()
{
	m_CurBaseEntity = ClientEntityList().m_BaseEntities.Head();
}

C_BaseEntity* C_BaseEntityIterator::Next()
{
	// Skip dormant entities
	while ( m_CurBaseEntity != ClientEntityList().m_BaseEntities.InvalidIndex() )
	{
		C_BaseEntity *pRet = ClientEntityList().m_BaseEntities[m_CurBaseEntity];
		m_CurBaseEntity = ClientEntityList().m_BaseEntities.Next( m_CurBaseEntity );

		if (!pRet->IsDormant())
			return pRet;
	}

	return NULL;
}

