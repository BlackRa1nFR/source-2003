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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include <stddef.h>
#include "vengineserver_impl.h"
#include "server.h"
#include "pr_edict.h"
#include "world.h"
#include "ispatialpartition.h"
#include "utllinkedlist.h"


// Edicts won't get reallocated for this many seconds after being freed.
#define EDICT_FREETIME	1.0



#ifdef _DEBUG
#include <malloc.h>
#endif // _DEBUG

/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict (edict_t *e)
{
	byte sn = e->serial_number;
	e->free = false;
	
	serverGameEnts->FreeContainingEntity(e);
	InitializeEntityDLLFields(e);

	e->serial_number = sn;	// Preserve the serial number.
	e->entity_created = 0;	// No player has seen this entity yet.
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/


edict_t *ED_Alloc (void)
{
	// Check the free list first.
	for ( int i=svs.maxclients+1; i < sv.num_edicts; i++ )
	{
		edict_t *pEdict = &sv.edicts[i];
		if ( pEdict->free && (pEdict->freetime < 2 || sv.gettime() - pEdict->freetime >= EDICT_FREETIME) )
		{
			ED_ClearEdict( pEdict );
			return pEdict;
		}
	}
	
	// Allocate a new edict.
	if ( sv.num_edicts >= sv.max_edicts )
	{
		if ( sv.max_edicts == 0 )
			Sys_Error( "ED_Alloc: No edicts yet" );
		Sys_Error ("ED_Alloc: no free edicts");
	}

	edict_t *e = sv.edicts + sv.num_edicts;
	ED_ClearEdict( e );
	
	sv.num_edicts++;
	return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free (edict_t *ed)
{
	if (ed->free)
	{
#ifdef _DEBUG
//		Con_DPrintf("duplicate free on '%s'\n", pr_strings + ed->classname );
#endif
		return;
	}

	SV_UnlinkEdict (ed);		// unlink from world bsp

	// don't free player edicts
	if ( (ed - sv.edicts) >= 1 && (ed - sv.edicts) <= svs.maxclients )
		return;

	// release the DLL entity that's attached to this edict, if any
	serverGameEnts->FreeContainingEntity( ed );

	ed->free = true;
	ed->freetime = sv.gettime();

	// Increment the serial number so it knows to send explicit deletes the clients.
	ed->serial_number++; 
}

//
// 	serverGameEnts->FreeContainingEntity( pEdict )  frees up memory associated with a DLL entity.
// InitializeEntityDLLFields clears out fields to NULL or UNKNOWN.
// Release is for terminating a DLL entity.  Initialize is for initializing one.
//
void InitializeEntityDLLFields( edict_t *pEdict )
{
	// clear all the game variables
	size_t sz = offsetof( edict_t, m_pEnt ) + sizeof( void* );
	memset( ((byte*)pEdict) + sz, 0, sizeof(edict_t) - sz );

	// No spatial partition
	pEdict->partition = PARTITION_INVALID_HANDLE;
}

edict_t *EDICT_NUM(int n)
{
	if (n < 0 ||
		n >= sv.max_edicts)
		Sys_Error ("EDICT_NUM: bad number %i", n);
	return &sv.edicts[n];
}

int NUM_FOR_EDICT(const edict_t *e)
{
	int		b;
	
	b = e - sv.edicts;
	
	if (b < 0 || b >= sv.num_edicts)
		Sys_Error ("NUM_FOR_EDICT: bad pointer");
	return b;
}


