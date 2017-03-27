//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef C_SOUNDSCAPE_H
#define C_SOUNDSCAPE_H
#ifdef _WIN32
#pragma once
#endif


class IGameSystem;
struct audioparams_t;

extern IGameSystem *ClientSoundscapeSystem();

// call when audio params may have changed
extern void Soundscape_Update( audioparams_t &audio );

#endif // C_SOUNDSCAPE_H
