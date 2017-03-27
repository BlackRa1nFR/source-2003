//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEENTITY_SHARED_H
#define BASEENTITY_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// Simple shared header file for common base entities
#if defined( CLIENT_DLL )
#include "c_baseentity.h"
#include "c_baseanimating.h"
#else
//#include "cbase.h"
#endif

// entity capabilities
// These are caps bits to indicate what an object's capabilities (currently used for +USE, save/restore and level transitions)
#define		FCAP_MUST_SPAWN				0x00000001		// Spawn after restore
#define		FCAP_ACROSS_TRANSITION		0x00000002		// should transfer between transitions 
// UNDONE: This will ignore transition volumes (trigger_transition), but not the PVS!!!
#define		FCAP_FORCE_TRANSITION		0x00000004		// ALWAYS goes across transitions

#define		FCAP_IMPULSE_USE			0x00000010		// can be used by the player
#define		FCAP_CONTINUOUS_USE			0x00000020		// can be used by the player
#define		FCAP_ONOFF_USE				0x00000040		// can be used by the player
#define		FCAP_DIRECTIONAL_USE		0x00000080		// Player sends +/- 1 when using (currently only tracktrains)
// NOTE: Normally +USE only works in direct line of sight.  Add these caps for additional searches
#define		FCAP_USE_ONGROUND			0x00000100
#define		FCAP_USE_IN_RADIUS			0x00000200

#define		FCAP_MASTER					0x10000000		// Can be used to "master" other entities (like multisource)
#define		FCAP_NO_EDICT_AT_SPAWN		0x40000000		// Doesn't grab an engine edict when spawned
#define		FCAP_DONT_SAVE				0x80000000		// Don't save this


#endif // BASEENTITY_SHARED_H
