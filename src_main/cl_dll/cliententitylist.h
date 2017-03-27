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
// $Date:         $
// $NoKeywords: $
//=============================================================================
#if !defined( CLIENTENTITYLIST_H )
#define CLIENTENTITYLIST_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "icliententitylist.h"
#include "iclientunknown.h"
#include "UtlLinkedList.h"
#include "UtlVector.h"
#include "icliententityinternal.h"
#include "ispatialpartition.h"
#include "cdll_int.h"
#include "cdll_util.h"
#include "entitylist_base.h"

class C_Beam;
class C_BaseViewModel;
class C_BaseEntity;
							   


// Maximum size of entity list
#define INVALID_CLIENTENTITY_HANDLE CBaseHandle( INVALID_EHANDLE_INDEX )


//
// This is the IClientEntityList implemenation. It serves two functions:
//
// 1. It converts server entity indices into IClientNetworkables for the engine.
//
// 2. It provides a place to store IClientUnknowns and gives out ClientEntityHandle_t's
//    so they can be indexed and retreived. For example, this is how static props are referenced
//    by the spatial partition manager - it doesn't know what is being inserted, so it's 
//	  given ClientEntityHandle_t's, and the handlers for spatial partition callbacks can
//    use the client entity list to look them up and check for supported interfaces.
//
class CClientEntityList : public CBaseEntityList, public IClientEntityList
{
friend class C_BaseEntityIterator;
friend class C_AllBaseEntityIterator;

public:
	// Constructor, destructor
								CClientEntityList( void );
	virtual 					~CClientEntityList( void );

	void						Release();		// clears everything and releases entities


// Implement IClientEntityList
public:

	virtual IClientNetworkable*	GetClientNetworkable( int entnum );
	virtual IClientEntity*		GetClientEntity( int entnum );

	virtual int					NumberOfEntities( void );

	virtual IClientUnknown*		GetClientUnknownFromHandle( ClientEntityHandle_t hEnt );
	virtual IClientNetworkable*	GetClientNetworkableFromHandle( ClientEntityHandle_t hEnt );
	virtual IClientEntity*		GetClientEntityFromHandle( ClientEntityHandle_t hEnt );

	virtual int					GetHighestEntityIndex( void );

	virtual void				SetMaxEntities( int maxents );
	virtual int					GetMaxEntities( );


// CBaseEntityList overrides.
protected:

	virtual void OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle );
	virtual void OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle );


// Internal to client DLL.
public:

	// All methods of accessing specialized IClientUnknown's go through here.
	IClientUnknown*			GetListedEntity( int entnum );
	
	// Simple wrappers for convenience..
	C_BaseEntity*			GetBaseEntity( int entnum );
	ICollideable*			GetClientCollideable( int entnum );

	IClientRenderable*		GetClientRenderableFromHandle( ClientEntityHandle_t hEnt );
	C_BaseEntity*			GetBaseEntityFromHandle( ClientEntityHandle_t hEnt );
	ICollideable*			GetClientCollideableFromHandle( ClientEntityHandle_t hEnt );
	IClientThinkable*		GetClientThinkableFromHandle( ClientEntityHandle_t hEnt );

	// Is a handle valid?
	bool					IsHandleValid( ClientEntityHandle_t handle ) const;

	// For backwards compatibility...
	C_BaseEntity*			GetEnt( int entnum ) { return GetBaseEntity( entnum ); }

	void					RecomputeHighestEntityUsed( void );


	// Use this to iterate over all the C_BaseEntities.
	C_BaseEntity* FirstBaseEntity() const;
	C_BaseEntity* NextBaseEntity( C_BaseEntity *pEnt ) const;


private:

	// Cached info for networked entities.
	struct EntityCacheInfo_t
	{
		// Cached off because GetClientNetworkable is called a *lot*
		IClientNetworkable *m_pNetworkable;
		unsigned short m_BaseEntitiesIndex;	// Index into m_BaseEntities (or m_BaseEntities.InvalidIndex() if none).
	};

	// Current count
	int					m_iNumServerEnts;
	// Max allowed
	int					m_iMaxServerEnts;

	// Current last used slot
	int					m_iMaxUsedServerIndex;

	// This holds fast lookups for special edicts.
	EntityCacheInfo_t	m_EntityCacheInfo[NUM_ENT_ENTRIES];

	// For fast iteration.
	CUtlLinkedList<C_BaseEntity*, unsigned short> m_BaseEntities;
};


// Use this to iterate over *all* (even dormant) the C_BaseEntities in the client entity list.
class C_AllBaseEntityIterator
{
public:
	C_AllBaseEntityIterator();

	void Restart();
	C_BaseEntity* Next();	// keep calling this until it returns null.

private:
	unsigned short m_CurBaseEntity;
};

class C_BaseEntityIterator
{
public:
	C_BaseEntityIterator();

	void Restart();
	C_BaseEntity* Next();	// keep calling this until it returns null.

private:
	unsigned short m_CurBaseEntity;
};

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------

inline bool	CClientEntityList::IsHandleValid( ClientEntityHandle_t handle ) const
{
	return handle.Get() != 0;
}

inline IClientUnknown* CClientEntityList::GetListedEntity( int entnum )
{
	return (IClientUnknown*)LookupEntityByNetworkIndex( entnum );
}

inline IClientUnknown* CClientEntityList::GetClientUnknownFromHandle( ClientEntityHandle_t hEnt )
{
	return (IClientUnknown*)LookupEntity( hEnt );
}


//-----------------------------------------------------------------------------
// Returns the client entity list
//-----------------------------------------------------------------------------
extern CClientEntityList *cl_entitylist;

inline CClientEntityList& ClientEntityList()
{
	return *cl_entitylist;
}


#endif // CLIENTENTITYLIST_H

