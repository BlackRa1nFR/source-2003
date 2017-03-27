//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "IVGuiModule.h"

#include <VGUI.h>
#include <VGUI_Controls.h>
#include <VGUI_ILocalize.h>
#include <VGUI_KeyValues.h>
#include <VGUI_IVGui.h>

#include "Tracker.h"

#include "TrackerDoc.h"
#include "TrackerDialog.h"
#include "IRunGameEngine.h"
#include "ServerSession.h"
#include "ServerBrowser/IServerBrowser.h"

IRunGameEngine *g_pRunGameEngine = NULL;
extern IServerBrowser *g_pIServerBrowser;

//-----------------------------------------------------------------------------
// Purpose: exposes tracker as a vgui module
//-----------------------------------------------------------------------------
class CTrackerUIVGuiModule : public IVGuiModule
{
public:
	CTrackerUIVGuiModule();
	~CTrackerUIVGuiModule();

	virtual bool Initialize(CreateInterfaceFn *factories, int factoryCount);
	virtual bool PostInitialize(CreateInterfaceFn *modules, int factoryCount);
	virtual bool Activate();
	virtual void Deactivate();
	virtual void Reactivate();

	virtual bool IsValid();
	virtual void Shutdown();
	virtual vgui::VPanel *GetPanel();
};

EXPOSE_SINGLE_INTERFACE(CTrackerUIVGuiModule, IVGuiModule, "VGuiModuleTracker001");

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTrackerUIVGuiModule::CTrackerUIVGuiModule()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTrackerUIVGuiModule::~CTrackerUIVGuiModule()
{
}

//-----------------------------------------------------------------------------
// Purpose: called to setup the module with all the required factories
//-----------------------------------------------------------------------------
bool CTrackerUIVGuiModule::Initialize(CreateInterfaceFn *factories, int factoryCount)
{
	Tracker_SetStandaloneMode(true);

	// load the vgui interfaces
	if ( vgui::VGui_InitInterfacesList(factories, factoryCount) )
	{
		// load localization file
		vgui::localize()->AddFile(vgui::filesystem(), "friends/trackerui_english.txt");	
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: called after all the modules have been initialized
//			modules should use this time to link to all the other module interfaces	
//-----------------------------------------------------------------------------
bool CTrackerUIVGuiModule::PostInitialize(CreateInterfaceFn *modules, int factoryCount)
{
	// link to the server browser interface
	g_pIServerBrowser = NULL;
	g_pRunGameEngine = NULL;

	// find the interfaces we need
	for (int i = 0; i < factoryCount; i++)
	{
		if (!g_pRunGameEngine)
		{
			g_pRunGameEngine = (IRunGameEngine *)(modules[i])(RUNGAMEENGINE_INTERFACE_VERSION, NULL);
		}

		if (!g_pIServerBrowser)
		{
			g_pIServerBrowser = (IServerBrowser *)(modules[i])(SERVERBROWSER_INTERFACE_VERSION, NULL);
		}
	}

	// if we didn't find it elsewhere, use our own
	if (!g_pRunGameEngine)
	{
		g_pRunGameEngine = (IRunGameEngine *)Sys_GetFactoryThis()(RUNGAMEENGINE_INTERFACE_VERSION, NULL);
		if (!g_pRunGameEngine)
			return false;
	}

	// create our main dialog
	CTrackerDialog *trackerDialog = new CTrackerDialog;
	trackerDialog->SetTitle("#TrackerUI_Friends_Title", true);
	trackerDialog->MakePopup();
	
	if (trackerDialog->Start())
	{
		trackerDialog->SetVisible(false);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: called when the module is selected from the menu or otherwise activated
//-----------------------------------------------------------------------------
bool CTrackerUIVGuiModule::Activate()
{
	// bring the tracker dialog to the front
	CTrackerDialog::GetInstance()->Activate();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: requests that the UI is temporarily disabled and all data files saved
//-----------------------------------------------------------------------------
void CTrackerUIVGuiModule::Deactivate()
{
	CTrackerDialog::GetInstance()->ShutdownUI();
}

//-----------------------------------------------------------------------------
// Purpose: restart from a Deactivate()
//-----------------------------------------------------------------------------
void CTrackerUIVGuiModule::Reactivate()
{
	if (GetDoc()->GetUserID() > 0)
	{
		CTrackerDialog::GetInstance()->StartTrackerWithUser(GetDoc()->GetUserID());
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the module is successfully initialized and available
//-----------------------------------------------------------------------------
bool CTrackerUIVGuiModule::IsValid()
{

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: called when the module is about to be Shutdown
//-----------------------------------------------------------------------------
void CTrackerUIVGuiModule::Shutdown()
{
	CTrackerDialog::GetInstance()->Shutdown();
	CTrackerDialog::GetInstance()->MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: returns a handle to the main panel
//-----------------------------------------------------------------------------
vgui::VPanel *CTrackerUIVGuiModule::GetPanel()
{
	return CTrackerDialog::GetInstance() ? CTrackerDialog::GetInstance()->GetVPanel() : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to the rungameengine interface
//-----------------------------------------------------------------------------
IRunGameEngine *Tracker_GetRunGameEngineInterface()
{
	return g_pRunGameEngine;
}
