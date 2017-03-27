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
#include "quakedef.h"
#include "eiface.h"
#include "server.h"
#include "sv_main.h"
#include "tier0/vprof.h"
#include "host.h"

bool SV_IsSimulating( void );

float CServerState::gettime() const
{
	return tickcount * TICK_RATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool SV_HasActivePlayers()
{
	int i;
	for ( i = 0; i < svs.maxclients; i++ )
	{
		if ( svs.clients[ i ].active )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Run physics code (simulating == false means we're paused, but we'll still
//  allow player usercmds to be processed
//-----------------------------------------------------------------------------
void SV_Physics( bool simulating )
{
	VPROF( "SV_Physics" );
	
	if ( !SV_HasActivePlayers() )
		return;
	
	// Set time
	// Increment clock unless paused
	if ( simulating )
	{
		sv.tickcount++;
		g_ServerGlobalVariables.tickcount = sv.tickcount;
	}
	
	g_ServerGlobalVariables.curtime		= sv.gettime();
	g_ServerGlobalVariables.frametime	= simulating ? TICK_RATE : 0;

	serverGameDLL->GameFrame( simulating );

	// Update gpGlobals->time to SendProxy stuff can reference timestamp
	g_ServerGlobalVariables.tickcount = sv.tickcount;
	g_ServerGlobalVariables.curtime = sv.gettime();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : simulating - 
//-----------------------------------------------------------------------------
void SV_GameRenderDebugOverlays()
{
	if ( !serverGameDLL )
		return;

	if ( !sv.active )
		return;

	serverGameDLL->GameRenderDebugOverlays( SV_IsSimulating() );
}
