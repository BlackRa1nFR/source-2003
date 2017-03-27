//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: An application framework 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#include <windows.h>
#include "AppSystemGroup.h"
#include "tier0/dbg.h"
#include "vstdlib/icommandline.h"
#include "interface.h"


//-----------------------------------------------------------------------------
// Globals...
//-----------------------------------------------------------------------------
HINSTANCE s_HInstance;
extern IApplication *__g_pApplicationObject;

//HWND s_HWnd;


SpewRetval_t AppDefaultSpewFunc( SpewType_t spewType, char const *pMsg )
{
	OutputDebugString( pMsg );
	switch( spewType )
	{
	case SPEW_MESSAGE:
	case SPEW_WARNING:
	case SPEW_LOG:
		return SPEW_CONTINUE;

	case SPEW_ASSERT:
	case SPEW_ERROR:
	default:
		return SPEW_DEBUGGER;
	}
}


//-----------------------------------------------------------------------------
// HACK: Since I don't want to refit vgui yet...
//-----------------------------------------------------------------------------
void *GetAppInstance()
{
	return s_HInstance;
}

//-----------------------------------------------------------------------------
// Method to initialize/shutdown all systems
//-----------------------------------------------------------------------------
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	s_HInstance = hInstance;
	CommandLine()->CreateCmdLine( lpCmdLine );

	// By default, direct dbg reporting...
	SpewOutputFunc( AppDefaultSpewFunc );

	// A bunch of systems that all have the same scope
	CAppSystemGroup appSystems;

	// Do I create a window here?

	// Call an installed application creation function
	if (!__g_pApplicationObject->Create( &appSystems ))
		return -1;

	// Let all systems know about each other
	if (!appSystems.ConnectSystems())
		return -1;

	// Allow the application to do some work before init
	if (!__g_pApplicationObject->PreInit( &appSystems ))
		return -1;

	// Call Init on all App Systems
	if (!appSystems.InitSystems())
		return -1;

	// Main loop implemented by the application
	// FIXME: HACK workaround to avoid vgui porting
	__g_pApplicationObject->Main();

	// Cal Shutdown on all App Systems
	appSystems.ShutdownSystems();

	// Allow the application to do some work after shutdown
	__g_pApplicationObject->PostShutdown();

	// Systems should disconnect from each other
	appSystems.DisconnectSystems();

	// Unload all DLLs loaded in the AppCreate block
	appSystems.RemoveAllSystems();
	appSystems.UnloadAllModules();

	// Call an installed application destroy function
	__g_pApplicationObject->Destroy();

	return 0;
}

