//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef NETWORKSTRINGTABLE_CLIENTDLL_H
#define NETWORKSTRINGTABLE_CLIENTDLL_H
#ifdef _WIN32
#pragma once
#endif

#include "INetworkStringTableClient.h"
extern INetworkStringTableClient *networkstringtable;

// String tables used by the client DLL	
// (see InstallStringTableCallback for where they're initialized)
extern TABLEID g_StringTableVguiScreen;
extern TABLEID g_StringTableEffectDispatch;

#endif // NETWORKSTRINGTABLE_CLIENTDLL_H
