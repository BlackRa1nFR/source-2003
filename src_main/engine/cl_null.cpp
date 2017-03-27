//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: replaces the cl_*.cpp files with stubs
//
//=============================================================================

#ifdef SWDS

#include "quakedef.h"
#include "client.h"
#include "enginestats.h"
#include "convar.h"

client_static_t	cls;
CClientState	cl;
CEngineStats g_EngineStats;

bool Demo_IsPlayingBack()
{
	return false;
}

bool Demo_IsPlayingBack_TimeDemo()
{
	return false;
}


#endif
