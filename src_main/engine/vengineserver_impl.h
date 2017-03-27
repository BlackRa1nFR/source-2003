//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Expose the IVEngineServer implementation to engine code.
//
// $NoKeywords: $
//=============================================================================

#ifndef VENGINESERVER_IMPL_H
#define VENGINESERVER_IMPL_H

#include "eiface.h"

// Subdirectory of basedir that we scan for extension DLLs.
#define EXTENSION_DLL_SUBDIR "dlls"

// Name of game listing file
#define GAME_LIST_FILE       "liblist.gam"

// The engine can call its own exposed functions in here rather than 
// splitting them into naked functions and sharing.
extern IVEngineServer *g_pVEngineServer;

// Used to seed the random # stream
void SeedRandomNumberGenerator( bool random_invariant );


#endif // VENGINESERVER_IMPL_H

