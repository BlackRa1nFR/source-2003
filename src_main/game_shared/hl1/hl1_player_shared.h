//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef HL1_PLAYER_SHARED_H
#define HL1_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// Shared header file for players
#if defined( CLIENT_DLL )
#define CHL1_Player C_BaseHLPlayer	//FIXME: Lovely naming job between server and client here...
#include "c_basehlplayer.h"
#else
#include "hl1_player.h"
#endif

#endif // HL1_PLAYER_SHARED_H
