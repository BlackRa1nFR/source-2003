//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entitylist.h"
#include "utlvector.h"
#include "igamesystem.h"

extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
static CUtlVector<IServerNetworkable*> g_DeleteList;

CGlobalEntityList gEntList;
CBaseEntityList *g_pEntityList = &gEntList;

class CAimTargetManager : public IEntityListener
{
public:
	// Called by CEntityListSystem
	void LevelInitPreEntity() 
	{ 
		gEntList.AddListenerEntity( this );
		Clear(); 
	}
	void LevelShutdownPostEntity()
	{
		gEntList.RemoveListenerEntity( this );
		Clear();
	}

	void Clear() 
	{ 
		m_targetList.Purge(); 
	}

	// IEntityListener
	virtual void OnEntityCreated( CBaseEntity *pEntity ) {}
	virtual void OnEntityDeleted( CBaseEntity *pEntity )
	{
		if ( !(pEntity->GetFlags() & FL_AIMTARGET) )
			return;
		RemoveEntity(pEntity);
	}
	void AddEntity( CBaseEntity *pEntity )
	{
		if ( pEntity->IsMarkedForDeletion() )
			return;
		m_targetList.AddToTail( pEntity );
	}
	void RemoveEntity( CBaseEntity *pEntity )
	{
		int index = m_targetList.Find( pEntity );
		if ( m_targetList.IsValidIndex(index) )
		{
			m_targetList.FastRemove( index );
		}
	}
	int ListCount() { return m_targetList.Count(); }
	int ListCopy( CBaseEntity *pList[], int listMax )
	{
		int count = min(listMax, ListCount() );
		memcpy( pList, m_targetList.Base(), sizeof(CBaseEntity *) * count );
		return count;
	}

private:
	CUtlVector<CBaseEntity *>	m_targetList;
};

static CAimTargetManager g_AimManager;

int AimTarget_ListCount()
{
	return g_AimManager.ListCount();
}
int AimTarget_ListCopy( CBaseEntity *pList[], int listMax )
{
	return g_AimManager.ListCopy( pList, listMax );
}

CGlobalEntityList::CGlobalEntityList()
{
	m_iHighestEnt = m_iNumEnts = 0;
	m_bClearingEntities = false;
}


// removes the entity from the global list
// only called from with the CBaseEntity destructor
static bool g_fInCleanupDelete;


// mark an entity as deleted
void CGlobalEntityList::AddToDeleteList( IServerNetworkable *ent )
{
	if ( ent && ent->GetRefEHandle() != INVALID_EHANDLE_INDEX )
	{
		g_DeleteList.AddToTail( ent );
	}
}

extern bool g_bDisableEhandleAccess;
// call this before and after each frame to delete all of the marked entities.
void CGlobalEntityList::CleanupDeleteList( void )
{
	g_fInCleanupDelete = true;

	// clean up the vphysics delete list as well
	if ( physenv )
	{
		physenv->CleanupDeleteList();
	}
	g_bDisableEhandleAccess = true;
	for ( int i = 0; i < g_DeleteList.Count(); i++ )
	{
		delete g_DeleteList[i];
	}
	g_bDisableEhandleAccess = false;
	g_DeleteList.RemoveAll();

	g_fInCleanupDelete = false;
}

int CGlobalEntityList::ResetDeleteList( void )
{
	int result = g_DeleteList.Count();
	g_DeleteList.RemoveAll();
	return result;
}


	// add a class that gets notified of entity events
void CGlobalEntityList::AddListenerEntity( IEntityListener *pListener )
{
	if ( m_entityListeners.Find( pListener ) >= 0 )
	{
		AssertMsg( 0, "Can't add listeners multiple times\n" );
		return;
	}
	m_entityListeners.AddToTail( pListener );
}

void CGlobalEntityList::RemoveListenerEntity( IEntityListener *pListener )
{
	m_entityListeners.FindAndRemove( pListener );
}

void CGlobalEntityList::Clear( void )
{
	m_bClearingEntities = true;

	// Add all remaining entities in the game to the delete list and call appropriate UpdateOnRemove
	CBaseHandle hCur = FirstHandle();
	while ( hCur != InvalidHandle() )
	{
		IServerNetworkable *ent = GetServerNetworkable( hCur );
		if ( !ent )
			continue;

		// Force UpdateOnRemove to be called
		UTIL_Remove( ent );
		hCur = NextHandle( hCur );
	}
		
	CleanupDeleteList();
	// free the memory
	g_DeleteList.Purge();

	CBaseEntity::m_nDebugPlayer = -1;
	CBaseEntity::m_bInDebugSelect = false; 
	m_iHighestEnt = 0;
	m_iNumEnts = 0;

	m_bClearingEntities = false;
}


int CGlobalEntityList::NumberOfEntities( void )
{
	return m_iNumEnts;
}

CBaseEntity *CGlobalEntityList::NextEnt( CBaseEntity *pCurrentEnt ) 
{ 
	if ( !pCurrentEnt )
		return GetBaseEntity( FirstHandle() );

	// Run through the list until we get a CBaseEntity.
	CBaseHandle hCur = pCurrentEnt->GetRefEHandle();
	while ( hCur != INVALID_EHANDLE_INDEX )
	{
		hCur = NextHandle( hCur );

		CBaseEntity *pRet = GetBaseEntity( hCur );
		if ( pRet )
			return pRet;
	}
	
	return NULL; 

}

void CGlobalEntityList::ReportEntityFlagsChanged( CBaseEntity *pEntity, unsigned int flagsOld, unsigned int flagsNow )
{
	if ( pEntity->IsMarkedForDeletion() )
		return;
	// UNDONE: Move this into IEntityListener instead?
	unsigned int flagsChanged = flagsOld ^ flagsNow;
	if ( flagsChanged & FL_AIMTARGET )
	{
		unsigned int flagsAdded = flagsNow & flagsChanged;
		unsigned int flagsRemoved = flagsOld & flagsChanged;

		if ( flagsAdded & FL_AIMTARGET )
		{
			g_AimManager.AddEntity( pEntity );
		}
		if ( flagsRemoved & FL_AIMTARGET )
		{
			g_AimManager.RemoveEntity( pEntity );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used to confirm a pointer is a pointer to an entity, useful for
//			asserts.
//-----------------------------------------------------------------------------

bool CGlobalEntityList::IsEntityPtr( void *pTest )
{
	CBaseEntity *ent = NextEnt( NULL );

	for ( ; ent != NULL; ent = NextEnt(ent) )
	{
		if ( ent == pTest )
			return true;
	}

	return false; 
}

//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given classname.
// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
//			szName - Classname to search for.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassname( CBaseEntity *pStartEntity, const char *szName )
{
	CBaseEntity *e = pStartEntity;
	while ( (e = NextEnt(e)) != NULL )
	{
		if ( FStrEq( STRING(e->m_iClassname), szName ) )
			return e;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds an entity given a procedural name.
// Input  : szName - The procedural name to search for, should start with '!'.
//			pSearchingEntity - 
//			pActivator - The activator entity if this was called from an input
//				or Use handler.
//-----------------------------------------------------------------------------
CBaseEntity *FindEntityProcedural( const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator )
{
	//
	// Check for the name escape character.
	//
	if ( szName[0] == '!' )
	{
		const char *pName = szName + 1;

		//
		// It is a procedural name, look for the ones we understand.
		//
		if ( FStrEq( pName, "player" ) )
		{
			return (CBaseEntity *)UTIL_PlayerByIndex( 1 );
		}
		else if ( FStrEq( pName, "pvsplayer" ) )
		{
			if ( pSearchingEntity )
			{
				return CBaseEntity::Instance( UTIL_FindClientInPVS( pSearchingEntity->edict() ) );
			}
			else if ( pActivator )
			{
				// FIXME: error condition?
				return CBaseEntity::Instance( UTIL_FindClientInPVS( pActivator->edict() ) );
			}
			else
			{
				// FIXME: error condition?
				return (CBaseEntity *)UTIL_PlayerByIndex( 1 );
			}

		}
		else if ( FStrEq( pName, "activator" ) )
		{
			return pActivator;
		}
		else if ( FStrEq( pName, "picker" ) )
		{
			return FindPickerEntity( UTIL_PlayerByIndex(1) );
		}
		else 
		{
			Warning( "Invalid entity search name %s\n", szName );
			Assert(0);
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given name.
// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
//			szName - Name to search for.
//			pActivator - Activator entity if this was called from an input
//				handler or Use handler.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByName( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pActivator )
{
	if ( !szName || szName[0] == 0 )
		return NULL;

	if ( szName[0] == '!' )
	{
		//
		// Avoid an infinite loop, only find one match per procedural search!
		//
		if (pStartEntity == NULL)
		{
			// FIXME: no one ever calls with pSearchingEntity
			return FindEntityProcedural( szName, NULL, pActivator );
		}

		return NULL;
	}
	
	// @TODO (toml 03-18-03): Support real wildcards. Right now, only thing supported is trailing *
	int len = strlen( szName );
	if ( !len )
		return NULL;
	
	bool wildcard;
	if ( szName[ len - 1 ] == '*' )
	{
		wildcard = true;
		len--;
	}
	else
		wildcard = false;

	CBaseEntity *e = pStartEntity;
	while ( (e = NextEnt(e)) != NULL )
	{
		if ( !e->m_iName )
			continue;

		if ( !wildcard )
		{
			if ( stricmp( STRING(e->m_iName), szName ) == 0 )
				return e;
		}
		else
		{
			if ( _strnicmp( STRING(e->m_iName), szName, len ) == 0 )
				return e;
		}

	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pStartEntity - 
//			szModelName - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByModel( CBaseEntity *pStartEntity, const char *szModelName )
{
	CBaseEntity *e = pStartEntity;
	while ( (e = NextEnt(e)) != NULL )
	{
		if ( !e->edict() || !e->GetModelName() )
			continue;

		if ( FStrEq( STRING(e->GetModelName()), szModelName ) )
			return e;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given target.
// Input  : pStartEntity - 
//			szName - 
//-----------------------------------------------------------------------------
// FIXME: obsolete, remove
CBaseEntity	*CGlobalEntityList::FindEntityByTarget( CBaseEntity *pStartEntity, const char *szName )
{
	CBaseEntity *e = pStartEntity;
	while ( (e = NextEnt(e)) != NULL )
	{
		if ( !e->m_target )
			continue;

		if ( FStrEq( STRING(e->m_target), szName ) )
			return e;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Used to iterate all the entities within a sphere.
// Input  : pStartEntity - 
//			vecCenter - 
//			flRadius - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius )
{
	CBaseEntity *ent = NextEnt( pStartEntity );
	float	eorg;
	float	distSquared;

	flRadius *= flRadius;

	// iterate through all objects, returning those whos bounding boxes are within the radius
	for ( ; ent != NULL; ent = NextEnt(ent) )
	{
		if ( !ent->edict() )
			continue;
		CBaseEntity *v = ent;

		distSquared = 0;
		for ( int j = 0; j < 3 && distSquared <= flRadius; j++ )
		{
			if ( vecCenter[j] < v->GetAbsMins()[j] )
				eorg = vecCenter[j] - v->GetAbsMins()[j];
			else if ( vecCenter[j] > v->GetAbsMaxs()[j] )
				eorg = vecCenter[j] - v->GetAbsMaxs()[j];
			else
				eorg = 0;

			distSquared += eorg * eorg;
		}
		if (distSquared > flRadius)
			continue;

		return ent;
	}

	// nothing found
	return NULL; 
}


//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by name within a radius
// Input  : szName - Entity name to search for.
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator )
{
	CBaseEntity *pEntity = NULL;

	//
	// Check for matching class names within the search radius.
	//
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	}

	CBaseEntity *pSearch = NULL;
	while ((pSearch = gEntList.FindEntityByName( pSearch, szName, pActivator )) != NULL)
	{
		if ( !pSearch->edict() )
			continue;

		float flDist2 = (pSearch->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}

	return pEntity;
}



//-----------------------------------------------------------------------------
// Purpose: Finds the first entity by name within a radius
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for.
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByNameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator )
{
	//
	// Check for matching class names within the search radius.
	//
	CBaseEntity *pEntity = pStartEntity;
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		return gEntList.FindEntityByName( pEntity, szName, pActivator );
	}

	while ((pEntity = gEntList.FindEntityByName( pEntity, szName, pActivator )) != NULL)
	{
		if ( !pEntity->edict() )
			continue;

		float flDist2 = (pEntity->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			return pEntity;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by class name withing given search radius.
// Input  : szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius )
{
	CBaseEntity *pEntity = NULL;

	//
	// Check for matching class names within the search radius.
	//
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	}

	CBaseEntity *pSearch = NULL;
	while ((pSearch = gEntList.FindEntityByClassname( pSearch, szName )) != NULL)
	{
		if ( !pSearch->edict() )
			continue;

		float flDist2 = (pSearch->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}

	return pEntity;
}



//-----------------------------------------------------------------------------
// Purpose: Finds the first entity within radius distance by class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius )
{
	//
	// Check for matching class names within the search radius.
	//
	CBaseEntity *pEntity = pStartEntity;
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		return gEntList.FindEntityByClassname( pEntity, szName );
	}

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, szName )) != NULL)
	{
		if ( !pEntity->edict() )
			continue;

		float flDist2 = (pEntity->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			return pEntity;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds an entity by target name or class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator )
{
	CBaseEntity *pEntity = NULL;

	pEntity = gEntList.FindEntityByName( pStartEntity, szName, pActivator );
	if (!pEntity)
	{
		pEntity = gEntList.FindEntityByClassname( pStartEntity, szName );
	}

	return pEntity;
} 


//-----------------------------------------------------------------------------
// Purpose: Finds the first entity by target name or class name within a radius
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityGenericWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator )
{
	CBaseEntity *pEntity = NULL;

	pEntity = gEntList.FindEntityByNameWithin( pStartEntity, szName, vecSrc, flRadius, pActivator );
	if (!pEntity)
	{
		pEntity = gEntList.FindEntityByClassnameWithin( pStartEntity, szName, vecSrc, flRadius );
	}

	return pEntity;
} 

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by target name or class name within a radius.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator )
{
	CBaseEntity *pEntity = NULL;

	pEntity = gEntList.FindEntityByNameNearest( szName, vecSrc, flRadius, pActivator );
	if (!pEntity)
	{
		pEntity = gEntList.FindEntityByClassnameNearest( szName, vecSrc, flRadius );
	}

	return pEntity;
} 


//-----------------------------------------------------------------------------
// Purpose: Find the nearest entity along the facing direction from the given origin
//			within the angular threshold (ignores worldspawn) with the
//			given classname.
// Input  : origin - 
//			facing - 
//			threshold - 
//			classname - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, char *classname)
{
	float bestDot = threshold;
	CBaseEntity *ent = NULL;
	CBaseEntity *best_ent = NULL;

	while ( (ent = NextEnt(ent)) != NULL )
	{
		// FIXME: why is this skipping pointsize entities?
		if (ent->IsPointSized() )
			continue;

		// Make vector to entity
		Vector	to_ent = (ent->GetAbsOrigin() - origin);

		VectorNormalize( to_ent );
		float dot = DotProduct (facing , to_ent );
		if (dot > bestDot) 
		{
			if (FClassnameIs(ent,classname))
			{
				// Ignore if worldspawn
				if (!FClassnameIs( ent, "worldspawn" )  && !FClassnameIs( ent, "soundent")) 
				{
					bestDot	= dot;
					best_ent = ent;
				}
			}
		}
	}
	return best_ent;
}


//-----------------------------------------------------------------------------
// Purpose: Find the nearest entity along the facing direction from the given origin
//			within the angular threshold (ignores worldspawn)
// Input  : origin - 
//			facing - 
//			threshold - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold)
{
	float bestDot = threshold;
	CBaseEntity *ent = NULL;
	CBaseEntity *best_ent = NULL;

	while ( (ent = NextEnt(ent)) != NULL )
	{
		// Ignore logical entities
		if (!ent->pev)
			continue;

		// Make vector to entity
		Vector	to_ent = ent->WorldSpaceCenter() - origin;
		VectorNormalize(to_ent);

		float dot = DotProduct( facing, to_ent );
		if (dot <= bestDot) 
			continue;

		// Ignore if worldspawn
		if (!FStrEq( STRING(ent->m_iClassname), "worldspawn")  && !FStrEq( STRING(ent->m_iClassname), "soundent")) 
		{
			bestDot	= dot;
			best_ent = ent;
		}
	}
	return best_ent;
}


void CGlobalEntityList::OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
	int i = handle.GetEntryIndex();

	// record current list details
	m_iNumEnts++;
	if ( i > m_iHighestEnt )
		m_iHighestEnt = i;

	// If it's a CBaseEntity, notify the listeners.
	IServerNetworkable *pNet = (IServerNetworkable*)pEnt;
	Assert( pNet == dynamic_cast< IServerNetworkable* >( pEnt ) );

	CBaseEntity *pBaseEnt = pNet->GetBaseEntity();
	for ( i = m_entityListeners.Count()-1; i >= 0; i-- )
	{
		m_entityListeners[i]->OnEntityCreated( pBaseEnt );
	}
}


void CGlobalEntityList::OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
#ifdef DEBUG
	if ( !g_fInCleanupDelete )
	{
		int i;
		for ( i = 0; i < g_DeleteList.Count(); i++ )
		{
			if ( g_DeleteList[i] == pEnt )
			{
				g_DeleteList.FastRemove( i );
				Msg( "ERROR: Entity being destroyed but previously threaded on g_DeleteList\n" );
				break;
			}
		}
	}
#endif

	m_iNumEnts--;
}

// NOTE: This doesn't happen in OnRemoveEntity() specifically because 
// listeners may want to reference the object as it's being deleted
// OnRemoveEntity isn't called until the destructor and all data is invalid.
void CGlobalEntityList::NotifyRemoveEntity( CBaseHandle hEnt )
{
	CBaseEntity *pBaseEnt = GetBaseEntity( hEnt );
	if ( !pBaseEnt )
		return;

	for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
	{
		m_entityListeners[i]->OnEntityDeleted( pBaseEnt );
	}
}


//-----------------------------------------------------------------------------
// NOTIFY LIST
// 
// Allows entities to get events fired when another entity changes
//-----------------------------------------------------------------------------
struct entitynotify_t
{
	CBaseEntity	*pNotify;
	CBaseEntity	*pWatched;
};
class CNotifyList : public INotify, public IEntityListener
{
public:
	// INotify
	void AddEntity( CBaseEntity *pNotify, CBaseEntity *pWatched );
	void RemoveEntity( CBaseEntity *pNotify, CBaseEntity *pWatched );
	void ReportNamedEvent( CBaseEntity *pEntity, const char *pEventName );
	void ClearEntity( CBaseEntity *pNotify );
	void ReportSystemEvent( CBaseEntity *pEntity, notify_system_event_t eventType, const notify_system_event_params_t &params );

	// IEntityListener
	virtual void OnEntityCreated( CBaseEntity *pEntity );
	virtual void OnEntityDeleted( CBaseEntity *pEntity );

	// Called from CEntityListSystem
	void LevelInitPreEntity();
	void LevelShutdownPreEntity();

private:
	CUtlVector<entitynotify_t>	m_notifyList;
};

void CNotifyList::AddEntity( CBaseEntity *pNotify, CBaseEntity *pWatched )
{
	// OPTIMIZE: Also flag pNotify for faster "RemoveAllNotify" ?
	pWatched->AddEFlags( EFL_NOTIFY );
	int index = m_notifyList.AddToTail();
	entitynotify_t &notify = m_notifyList[index];
	notify.pNotify = pNotify;
	notify.pWatched = pWatched;
}

// Remove noitfication for an entity
void CNotifyList::RemoveEntity( CBaseEntity *pNotify, CBaseEntity *pWatched )
{
	for ( int i = m_notifyList.Count(); --i >= 0; )
	{
		if ( m_notifyList[i].pNotify == pNotify && m_notifyList[i].pWatched == pWatched)
		{
			m_notifyList.FastRemove(i);
		}
	}
}


void CNotifyList::ReportNamedEvent( CBaseEntity *pEntity, const char *pInputName )
{
	variant_t emptyVariant;

	if ( !pEntity->IsEFlagSet(EFL_NOTIFY) )
		return;

	for ( int i = 0; i < m_notifyList.Count(); i++ )
	{
		if ( m_notifyList[i].pWatched == pEntity )
		{
			m_notifyList[i].pNotify->AcceptInput( pInputName, pEntity, pEntity, emptyVariant, 0 );
		}
	}
}

void CNotifyList::LevelInitPreEntity()
{
	gEntList.AddListenerEntity( this );
}

void CNotifyList::LevelShutdownPreEntity( void )
{
	gEntList.RemoveListenerEntity( this );
	m_notifyList.Purge();
}

void CNotifyList::OnEntityCreated( CBaseEntity *pEntity )
{
}

void CNotifyList::OnEntityDeleted( CBaseEntity *pEntity )
{
	ReportDestroyEvent( pEntity );
	ClearEntity( pEntity );
}


// UNDONE: Slow linear search?
void CNotifyList::ClearEntity( CBaseEntity *pNotify )
{
	for ( int i = m_notifyList.Count(); --i >= 0; )
	{
		if ( m_notifyList[i].pNotify == pNotify || m_notifyList[i].pWatched == pNotify)
		{
			m_notifyList.FastRemove(i);
		}
	}
}

void CNotifyList::ReportSystemEvent( CBaseEntity *pEntity, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
	if ( !pEntity->IsEFlagSet(EFL_NOTIFY) )
		return;

	for ( int i = 0; i < m_notifyList.Count(); i++ )
	{
		if ( m_notifyList[i].pWatched == pEntity )
		{
			m_notifyList[i].pNotify->NotifySystemEvent( pEntity, eventType, params );
		}
	}
}

static CNotifyList g_NotifyList;
INotify *g_pNotify = &g_NotifyList;

class CEntityTouchManager : public IEntityListener
{
public:
	// called by CEntityListSystem
	void LevelInitPreEntity() 
	{ 
		gEntList.AddListenerEntity( this );
		Clear(); 
	}
	void LevelShutdownPostEntity() 
	{ 
		gEntList.RemoveListenerEntity( this );
		Clear(); 
	}
	void FrameUpdatePostEntityThink();

	void Clear()
	{
		m_updateList.Purge();
	}
	
	// IEntityListener
	virtual void OnEntityCreated( CBaseEntity *pEntity ) {}
	virtual void OnEntityDeleted( CBaseEntity *pEntity )
	{
		if ( !pEntity->GetCheckUntouch() )
			return;
		int index = m_updateList.Find( pEntity );
		if ( m_updateList.IsValidIndex(index) )
		{
			m_updateList.FastRemove( index );
		}
	}
	void AddEntity( CBaseEntity *pEntity )
	{
		if ( pEntity->IsMarkedForDeletion() )
			return;
		m_updateList.AddToTail( pEntity );
	}

private:
	CUtlVector<CBaseEntity *>	m_updateList;
};

static CEntityTouchManager g_TouchManager;

void EntityTouch_Add( CBaseEntity *pEntity )
{
	g_TouchManager.AddEntity( pEntity );
}


void CEntityTouchManager::FrameUpdatePostEntityThink()
{
	// Loop through all entities again, checking their untouch if flagged to do so
	
	int count = m_updateList.Count();
	if ( count )
	{
		// copy off the list
		CBaseEntity **ents = (CBaseEntity **)stackalloc( sizeof(CBaseEntity *) * count );
		memcpy( ents, m_updateList.Base(), sizeof(CBaseEntity *) * count );
		// clear it
		m_updateList.RemoveAll();
		
		// now update those ents
		for ( int i = 0; i < count; i++ )
		{
			//Assert( ents[i]->GetCheckUntouch() );
			if ( ents[i]->GetCheckUntouch() )
			{
				ents[i]->PhysicsCheckForEntityUntouch();
			}
		}
		stackfree( ents );
	}
}

// On hook to rule them all...
class CEntityListSystem : public CAutoGameSystem
{
public:
	void LevelInitPreEntity()
	{
		g_NotifyList.LevelInitPreEntity();
		g_TouchManager.LevelInitPreEntity();
		g_AimManager.LevelInitPreEntity();
	}
	void LevelShutdownPreEntity()
	{
		g_NotifyList.LevelShutdownPreEntity();
	}
	void LevelShutdownPostEntity()
	{
		g_TouchManager.LevelShutdownPostEntity();
		g_AimManager.LevelShutdownPostEntity();
	}

	void FrameUpdatePostEntityThink()
	{
		g_TouchManager.FrameUpdatePostEntityThink();
	}
};

static CEntityListSystem g_EntityListSystem;
