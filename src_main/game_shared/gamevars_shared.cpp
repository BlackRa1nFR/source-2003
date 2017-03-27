//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "gamevars_shared.h"

// some shared cvars used by game rules
ConVar deadspecmode( 
	"mp_deadspecmode", 
	"0", 
	FCVAR_REPLICATED,
	"Restricts spectator modes for dead players" );
	
ConVar allow_spectators(
	"allow_spectators", 
	"1.0", 
	FCVAR_REPLICATED,
	"toggles whether the server allows spectator mode or not" );