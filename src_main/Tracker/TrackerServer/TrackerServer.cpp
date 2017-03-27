//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Simple executable to Run TrackerSRV.dll
//=============================================================================

#include "../TrackerSRV/TrackerSRV_Interface.h"
#include "../common/winlite.h"
#include <winsock2.h>

#include "interface.h"

//-----------------------------------------------------------------------------
// Purpose: Application entry point
//			Loads and runs the TrackerSRV
//-----------------------------------------------------------------------------
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	// winsock aware
	WSAData wsaData;
	int nReturnCode = ::WSAStartup(MAKEWORD(2,0), &wsaData);

	// load the Tracker Server DLL
	CSysModule *serverDll = Sys_LoadModule("TrackerSRV.dll");
	CreateInterfaceFn serverFactory = Sys_GetFactory(serverDll);
	if (!serverFactory)
	{
		::MessageBox(NULL, "Could not load 'TrackerSRV.dll'", "Fatal Error - TrackerServer", MB_OK | MB_ICONERROR);
		return 1;
	}

	ITrackerSRV *trackerInterface = (ITrackerSRV *)serverFactory(TRACKERSERVER_INTERFACE_VERSION, NULL);

	// Activate server
	trackerInterface->RunTrackerServer(lpCmdLine);

	Sys_UnloadModule(serverDll);

	::WSACleanup();

	return 0;
}




