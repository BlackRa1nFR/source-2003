//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "AdminServer.h"
#include "serverpage.h"
#include "IRunGameEngine.h"
#include "ITrackerUser.h"
#include "../TrackerNET/TrackerNET_Interface.h"
#include "DialogGameInfo.h"

#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_IPanel.h>
#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include "FileSystem.h"



// expose the server browser interfaces
CAdminServer g_AdminServerSingleton;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CAdminServer, IAdminServer, ADMINSERVER_INTERFACE_VERSION, g_AdminServerSingleton);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CAdminServer, IVGuiModule, "VGuiModuleAdminServer001", g_AdminServerSingleton);

IRunGameEngine *g_pRunGameEngine = NULL;
ITrackerUser *g_pTrackerUser = NULL;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CAdminServer::CAdminServer()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CAdminServer::~CAdminServer()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAdminServer::CreateDialog()
{
	if (!m_hServerPage.Get())
	{
		m_hServerPage = new CServerPage( g_pTrackerUser->GetTrackerID());
		m_hServerPage->Initialize();
	}
}

//-----------------------------------------------------------------------------
// Purpose: links to vgui and engine interfaces
//-----------------------------------------------------------------------------
bool CAdminServer::Initialize(CreateInterfaceFn *factorylist, int factoryCount)
{
	g_pRunGameEngine = NULL;

	for (int i = 0; i < factoryCount; i++)
	{
		if (!g_pRunGameEngine)
		{
			g_pRunGameEngine = (IRunGameEngine *)(factorylist[i])(RUNGAMEENGINE_INTERFACE_VERSION, NULL);
		}
	}

	// load the vgui interfaces
	return vgui::VGui_InitInterfacesList(factorylist, factoryCount);
}

//-----------------------------------------------------------------------------
// Purpose: links to other modules interfaces (tracker)
//-----------------------------------------------------------------------------
bool CAdminServer::PostInitialize(CreateInterfaceFn *modules, int factoryCount)
{
	g_pTrackerUser = NULL;

	// find the interfaces we need
	for (int i = 0; i < factoryCount; i++)
	{
		if (!g_pRunGameEngine)
		{
			g_pRunGameEngine = (IRunGameEngine *)(modules[i])(RUNGAMEENGINE_INTERFACE_VERSION, NULL);
		}

		if (!g_pTrackerUser)
		{
			g_pTrackerUser = (ITrackerUser *)(modules[i])(TRACKERUSER_INTERFACE_VERSION, NULL);
		}
	}

	CreateDialog();
	m_hServerPage->SetVisible(false);

	return (g_pRunGameEngine && g_pTrackerUser);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAdminServer::IsValid()
{
	return (g_pRunGameEngine && g_pTrackerUser);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAdminServer::Activate()
{
	Open();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAdminServer::Open()
{
	m_hServerPage->Open();
}

//-----------------------------------------------------------------------------
// Purpose: returns direct handle to main server browser dialog
//-----------------------------------------------------------------------------
vgui::VPanel *CAdminServer::GetPanel()
{
	return m_hServerPage.Get() ? m_hServerPage->GetVPanel() : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Closes down the server browser for good
//-----------------------------------------------------------------------------
void CAdminServer::Shutdown()
{
	if (m_hServerPage.Get())
	{
		vgui::ivgui()->PostMessage(m_hServerPage->GetVPanel(), new KeyValues("Close"), NULL);
		m_hServerPage->MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAdminServer::Deactivate()
{
	if (m_hServerPage.Get())
	{
		vgui::ivgui()->PostMessage(m_hServerPage->GetVPanel(), new KeyValues("Close"), NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAdminServer::Reactivate()
{
}

//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog to watch the specified server; associated with the friend 'userName'
//-----------------------------------------------------------------------------
GameHandle_t CAdminServer::OpenGameInfoDialog(unsigned int gameIP, unsigned int gamePort, const char *userName)
{
	// opens the game info dialog, but does not connect
	if (m_hServerPage.Get())
	{
		vgui::Panel *dlg = m_hServerPage->OpenGameInfoDialog(gameIP, gamePort, userName);

		// return safe handle to the panel
		return (GameHandle_t)vgui::ivgui()->PanelToHandle(dlg->GetVPanel());
	}

	return (GameHandle_t)0;
}

//-----------------------------------------------------------------------------
// Purpose: joins a specified game - game info dialog will only be opened if the server is fully or passworded
//-----------------------------------------------------------------------------
GameHandle_t CAdminServer::JoinGame(unsigned int gameIP, unsigned int gamePort, const char *userName)
{
	// open the game info dialog in connect-right-away-mode
	if (m_hServerPage.Get())
	{
		vgui::Panel *dlg = m_hServerPage->JoinGame(gameIP, gamePort, userName);

		// return safe handle to the panel
		return (GameHandle_t)vgui::ivgui()->PanelToHandle(dlg->GetVPanel());
	}

	return (GameHandle_t)0;
}

//-----------------------------------------------------------------------------
// Purpose: changes the game info dialog to watch a new server; normally used when a friend changes games
//-----------------------------------------------------------------------------
void CAdminServer::UpdateGameInfoDialog(GameHandle_t gameDialog, unsigned int gameIP, unsigned int gamePort)
{
	// updates an existing dialog with a new game (for if a friend changes games)
	vgui::VPanel *vpanel = vgui::ivgui()->HandleToPanel(gameDialog);
	if (vpanel)
	{
		CDialogGameInfo *gameDialog = dynamic_cast<CDialogGameInfo *>(vgui::ipanel()->Client(vpanel)->GetPanel());
		if (gameDialog)
		{
			gameDialog->ChangeGame(gameIP, gamePort);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: forces the game info dialog closed
//-----------------------------------------------------------------------------
void CAdminServer::CloseGameInfoDialog(GameHandle_t gameDialog)
{
	// forces the dialog closed
	vgui::VPanel *vpanel = vgui::ivgui()->HandleToPanel(gameDialog);
	if (vpanel)
	{
		vgui::ivgui()->PostMessage(vpanel, new KeyValues("Close"), NULL);
	}
}
