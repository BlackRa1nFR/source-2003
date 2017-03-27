//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef EDICT_H
#define EDICT_H
#ifdef _WIN32
#pragma once
#endif


#include "vector.h"
#include "cmodel.h"
#include "const.h"
#include "iserverentity.h"
#include "globalvars_base.h"
#include "engine/ICollideable.h"
#include "engine/ivmodelinfo.h"

struct edict_t;
extern IVModelInfo* modelinfo;


//-----------------------------------------------------------------------------
// Purpose: Defines the ways that a map can be loaded.
//-----------------------------------------------------------------------------
enum MapLoadType_t
{
	MapLoad_NewGame = 0,
	MapLoad_LoadGame,
	MapLoad_Transition,
};


//-----------------------------------------------------------------------------
// Purpose: Global variables shared between the engine and the game .dll
//-----------------------------------------------------------------------------
class CGlobalVars : public CGlobalVarsBase
{	
public:

	CGlobalVars( bool bIsClient );

public:
	
	// Current map
	string_t		mapname;
	int				mapversion;
	string_t		startspot;
	MapLoadType_t	eLoadType;	// How the current map was loaded

	// game specific flags
	bool			deathmatch;
	bool			coop;
	bool			teamplay;
	// current cd autio track
	int				cdAudioTrack;
	// current maxentities
	int				maxEntities;
};

inline CGlobalVars::CGlobalVars( bool bIsClient ) : 
	CGlobalVarsBase( bIsClient )
{
}



class CPlayerState;
class IServerNetworkable;

// Entities can span this many clusters before we revert to a slower area checking algorithm
#define	MAX_ENT_CLUSTERS	24


class IServerEntity;


class CBaseEdict
{
public:

	// Returns an IServerEntity if FL_FULLEDICT is set or NULL if this 
	// is a lightweight networking entity.
	IServerEntity*			GetIServerEntity();
	const IServerEntity*	GetIServerEntity() const;

	// Set when initting an entity. If it's only a networkable, this is false.
	void				SetFullEdict( bool bFullEdict );
	

public:
	// this should never be referenced by engine code
	// Alloced and freed by game DLL
	IServerNetworkable	*m_pEnt;		

protected:
	bool				m_bFullEdict;
};


//-----------------------------------------------------------------------------
// CBaseEdict inlines.
//-----------------------------------------------------------------------------
inline IServerEntity* CBaseEdict::GetIServerEntity()
{
	if ( m_bFullEdict )
		return (IServerEntity*)m_pEnt;
	else
		return 0;
}

inline const IServerEntity* CBaseEdict::GetIServerEntity() const
{
	if ( m_bFullEdict )
		return (IServerEntity*)m_pEnt;
	else
		return 0;
}

inline void CBaseEdict::SetFullEdict( bool bFullEdict )
{
	m_bFullEdict = bFullEdict;
}


//-----------------------------------------------------------------------------
// Purpose: The engine's internal representation of an entity, including some
//  basic collision and position info and a pointer to the class wrapped on top
//  of the structure
//-----------------------------------------------------------------------------
struct edict_t : public CBaseEdict
{
public:
	ICollideable *GetCollideable();

	// all edict variables
	string_t	classname;

	// Is the edict available for use
	bool		free;
	float		freetime;	// The sv.time at which the edict was freed.

	// For determining that an entity has been freed/reallocated in a slot
	byte		serial_number;

	// One bit for each client, signals that we did create the entity for this player at some point
	int			entity_created;

	// Spatial partition handle
	unsigned short partition;

	// number of clusters or -1 if too many
	int			clusterCount;		
	// cluster indices
	int			clusters[ MAX_ENT_CLUSTERS ];	

	// headnode for the entity's bounding box
	int			headnode;			
	// For dynamic "area portals"
	int			areanum, areanum2;	
};

inline ICollideable *edict_t::GetCollideable()
{
	IServerEntity *pEnt = GetIServerEntity();
	if ( pEnt )
		return pEnt->GetCollideable();
	else
		return NULL;
}


#endif // EDICT_H
