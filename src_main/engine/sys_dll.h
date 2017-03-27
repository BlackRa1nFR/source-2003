
//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Expose functions from sys_dll.cpp.
//
// $NoKeywords: $
//=============================================================================

#ifndef SYS_DLL_H
#define SYS_DLL_H

#include "interface.h"
extern CreateInterfaceFn physicsFactory;

// This factory gets to many of the major app-single systems,
// including the material system, vgui, vgui surface, the file system.
extern CreateInterfaceFn g_AppSystemFactory;



void LoadEntityDLLs( char *szBaseDir );
void UnloadEntityDLLs( void );

// This returns true if someone called Error() or Sys_Error() and we're exiting.
// Since we call exit() from inside those, some destructors need to be safe and not crash.
bool IsInErrorExit();

#endif


