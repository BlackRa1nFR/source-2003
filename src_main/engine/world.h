//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Implementation of the main material system interface
//=============================================================================
	   
// world.h
#ifndef WORLD_H
#define WORLD_H

#ifdef _WIN32
#pragma once
#endif


struct edict_t;


void SV_ClearWorld (void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEdict (edict_t *ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
// flags ent->v.modified

void SV_LinkEdict (edict_t *ent, qboolean touch_triggers, const Vector *pPrevAbsOrigin = 0);
// Needs to be called any time an entity changes origin, mins, maxs, or solid
// flags ent->v.modified
// sets ent->v.absmin and ent->v.absmax
// if touchtriggers, calls prog functions for the intersected triggers

// This is to temporarily remove an object from the collision tree.
// Unlink returns a handle we have to use to relink
int SV_FastUnlink( edict_t *ent );
void SV_FastRelink( edict_t *ent, int tempHandle );


#endif // WORLD_H
