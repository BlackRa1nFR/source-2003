//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <VGUI_Controls.h>
#include <VGUI_IScheme.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>

#include <VGUI_Panel.h>

#include "interface.h"
#include "FileSystem.h"

#include "MainDialog.h"

#include <stdio.h>

#include "winlite.h"

#include "../TrackerNET/TrackerNET_Interface.h"
#include "../TrackerNET/Threads.h"

#include "ServerList.h"

// networking
ITrackerNET *g_pTrackerNET = NULL;

// server list
CServerList *g_pServerList = NULL;

//-----------------------------------------------------------------------------
// Purpose: Entry point
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// load vgui
	CSysModule *vguiModule = Sys_LoadModule("vgui2.dll");
	CSysModule *fileSystemModule = Sys_LoadModule("filesystem_stdio.dll");
	CSysModule *netModule = Sys_LoadModule("../friends/trackerNET.dll");
	CreateInterfaceFn vguiFactory = Sys_GetFactory(vguiModule);
	CreateInterfaceFn netFactory = Sys_GetFactory(netModule);
	CreateInterfaceFn fileSystemFactory = Sys_GetFactory(fileSystemModule);
	if (!vguiFactory)
	{
		printf("Fatal error: Could not load vgui2.dll\n");
		return 2;
	}
	if (!netFactory)
	{
		printf("Fatal error: Could not load trackerNET.dll\n");
		return 3;
	}

	// initialize interfaces
	CreateInterfaceFn factories[2] = { fileSystemFactory, vguiFactory };
	assert(vgui::VGui_InitInterfacesList(factories, 2));
	g_pTrackerNET = (ITrackerNET *)netFactory(TRACKERNET_INTERFACE_VERSION, NULL);
	g_pTrackerNET->Initialize(1300, 1400);

	vgui::filesystem()->AddSearchPath("../", "");

	// Init the surface
	vgui::Panel *panel = new vgui::Panel(NULL, "TopPanel");
	vgui::surface()->Init(panel->GetVPanel());

	// load the scheme
	if (!vgui::scheme()->LoadSchemeFromFile("resource/TrackerScheme.res"))
	{
		return false;
	}

	// Start vgui
	vgui::ivgui()->Start();

	// add our main window
	vgui::Panel *main = new CMainDialog(panel);

	// server list
	g_pServerList = new CServerList(main);

	// Run app frame loop
	while (vgui::ivgui()->IsRunning())
	{
		vgui::ivgui()->RunFrame();

		// networking
		g_pServerList->RunFrame();
	}

	delete g_pServerList;

	// Shutdown
	vgui::surface()->Shutdown();
	Sys_UnloadModule(vguiModule);
	Sys_UnloadModule(netModule);
	return 1;
}

