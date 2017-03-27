//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef NETWORKSTRINGTABLE_GAMEDLL_H
#define NETWORKSTRINGTABLE_GAMEDLL_H
#ifdef _WIN32
#pragma once
#endif

#include "inetworkstringtableserver.h"

extern INetworkStringTableServer *networkstringtable;

// String tables used by the game DLL
#define MAX_VGUI_SCREEN_STRING_BITS		8
#define MAX_VGUI_SCREEN_STRINGS			( 1 << MAX_VGUI_SCREEN_STRING_BITS )
#define VGUI_SCREEN_INVALID_STRING		( MAX_VGUI_SCREEN_STRINGS - 1 )

#define MAX_MATERIAL_STRING_BITS		10
#define MAX_MATERIAL_STRINGS			( 1 << MAX_MATERIAL_STRING_BITS )
#define OVERLAY_MATERIAL_INVALID_STRING	( MAX_MATERIAL_STRINGS - 1 )

extern TABLEID g_StringTableVguiScreen;


extern TABLEID g_StringTableEffectDispatch;


#endif // NETWORKSTRINGTABLE_GAMEDLL_H
