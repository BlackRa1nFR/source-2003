//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Entry point into app
//=============================================================================

#include "interface.h"
#include <VGUI_Controls.h>
#include <VGUI_Panel.h>
#include <VGUI_IScheme.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>
#include "FileSystem.h"

#include "..\..\tracker\common\winlite.h"

#include "LocalizationDialog.h"

#include <stdio.h>

//-----------------------------------------------------------------------------
// Purpose: Entry point
//			loads interfaces and initializes dialog
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// load vgui
	CSysModule *vguiModule = Sys_LoadModule("vgui2.dll");
	CSysModule *filesystem = Sys_LoadModule("FileSystem_Stdio.dll");

	CreateInterfaceFn factorylist[3];
	factorylist[0] = Sys_GetFactoryThis();
	factorylist[1] = Sys_GetFactory(vguiModule);
	factorylist[2] = Sys_GetFactory(filesystem);

	// initialize interfaces
	vgui::VGui_InitInterfacesList(factorylist, 3);

	vgui::filesystem()->AddSearchPath("../", "PLATFORM");

	// load the scheme
	if (!vgui::scheme()->LoadSchemeFromFile("Resource/TrackerScheme.res"))
		return 1;

	// Init the surface
	vgui::Panel *panel = new vgui::Panel(NULL, "TopPanel");
	vgui::surface()->Init(panel->GetVPanel());

	// Start vgui
	vgui::ivgui()->Start();

	// add our main window
	vgui::Panel *main = new CLocalizationDialog(lpCmdLine);

	// Run app frame loop
	while (vgui::ivgui()->IsRunning())
	{
		vgui::ivgui()->RunFrame();
	}

	// Shutdown
	vgui::surface()->Shutdown();
	Sys_UnloadModule(vguiModule);
	return 1;
}
