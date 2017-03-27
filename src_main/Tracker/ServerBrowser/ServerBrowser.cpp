//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "ServerBrowser.h"
#include "ServerBrowserDialog.h"
#include "IRunGameEngine.h"
#include "Friends/IFriendsUser.h"
#include "DialogGameInfo.h"

#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>

// expose the server browser interfaces
CServerBrowser g_ServerBrowserSingleton;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerBrowser, IServerBrowser, SERVERBROWSER_INTERFACE_VERSION, g_ServerBrowserSingleton);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerBrowser, IVGuiModule, "VGuiModuleServerBrowser001", g_ServerBrowserSingleton);

IRunGameEngine *g_pRunGameEngine = NULL;
IFriendsUser *g_pFriendsUser = NULL;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServerBrowser::CServerBrowser()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServerBrowser::~CServerBrowser()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowser::CreateDialog()
{
	if (!m_hInternetDlg.Get())
	{
		m_hInternetDlg = new CServerBrowserDialog(NULL); // SetParent() call below fills this in
		m_hInternetDlg->Initialize();
	}
}

//-----------------------------------------------------------------------------
// Purpose: links to vgui and engine interfaces
//-----------------------------------------------------------------------------
bool CServerBrowser::Initialize(CreateInterfaceFn *factorylist, int factoryCount)
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
	if ( vgui::VGui_InitInterfacesList("ServerBrowser", factorylist, factoryCount) )
	{
		// load localization file
		vgui::localize()->AddFile(vgui::filesystem(), "servers/serverbrowser_%language%.txt");
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: links to other modules interfaces (tracker)
//-----------------------------------------------------------------------------
bool CServerBrowser::PostInitialize(CreateInterfaceFn *modules, int factoryCount)
{
	g_pFriendsUser = NULL;

	// find the interfaces we need
	for (int i = 0; i < factoryCount; i++)
	{
		if (!g_pRunGameEngine)
		{
			g_pRunGameEngine = (IRunGameEngine *)(modules[i])(RUNGAMEENGINE_INTERFACE_VERSION, NULL);
		}

		if (!g_pFriendsUser)
		{
			g_pFriendsUser = (IFriendsUser *)(modules[i])(FRIENDSUSER_INTERFACE_VERSION, NULL);
		}
	}

	CreateDialog();
	m_hInternetDlg->SetVisible(false);

	return (g_pRunGameEngine /*&& g_pFriendsUser*/);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CServerBrowser::IsValid()
{
	return (g_pRunGameEngine /*&& g_pFriendsUser*/);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CServerBrowser::Activate()
{
	Open();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowser::Deactivate()
{
	if (m_hInternetDlg.Get())
	{
		m_hInternetDlg->SaveFilters();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowser::Reactivate()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowser::Open()
{
	m_hInternetDlg->Open();
}

//-----------------------------------------------------------------------------
// Purpose: returns direct handle to main server browser dialog
//-----------------------------------------------------------------------------
vgui::VPANEL CServerBrowser::GetPanel()
{
	return m_hInternetDlg.Get() ? m_hInternetDlg->GetVPanel() : NULL;
}


//-----------------------------------------------------------------------------
// Purpose: sets the parent panel of the main module panel
//-----------------------------------------------------------------------------
void CServerBrowser::SetParent(vgui::VPANEL parent)
{
	if(m_hInternetDlg.Get())
	{
		m_hInternetDlg->SetParent(parent);
	}
}




//-----------------------------------------------------------------------------
// Purpose: Closes down the server browser for good
//-----------------------------------------------------------------------------
void CServerBrowser::Shutdown()
{
	if (m_hInternetDlg.Get())
	{
		m_hInternetDlg->Close();
		m_hInternetDlg->MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog to watch the specified server; associated with the friend 'userName'
//-----------------------------------------------------------------------------
GameHandle_t CServerBrowser::OpenGameInfoDialog(unsigned int gameIP, unsigned int gamePort, const char *userName)
{
	// opens the game info dialog, but does not connect
	if (m_hInternetDlg.Get())
	{
		vgui::Panel *dlg = m_hInternetDlg->OpenGameInfoDialog(gameIP, gamePort, userName);

		// return safe handle to the panel
		return (GameHandle_t)vgui::ivgui()->PanelToHandle(dlg->GetVPanel());
	}

	return (GameHandle_t)0;
}

//-----------------------------------------------------------------------------
// Purpose: joins a specified game - game info dialog will only be opened if the server is fully or passworded
//-----------------------------------------------------------------------------
GameHandle_t CServerBrowser::JoinGame(unsigned int gameIP, unsigned int gamePort, const char *userName)
{
	// open the game info dialog in connect-right-away-mode
	if (m_hInternetDlg.Get())
	{
		vgui::Panel *dlg = m_hInternetDlg->JoinGame(gameIP, gamePort, userName);

		// return safe handle to the panel
		return (GameHandle_t)vgui::ivgui()->PanelToHandle(dlg->GetVPanel());
	}

	return (GameHandle_t)0;
}

//-----------------------------------------------------------------------------
// Purpose: changes the game info dialog to watch a new server; normally used when a friend changes games
//-----------------------------------------------------------------------------
void CServerBrowser::UpdateGameInfoDialog(GameHandle_t gameDialog, unsigned int gameIP, unsigned int gamePort)
{
	// updates an existing dialog with a new game (for if a friend changes games)
	vgui::VPANEL VPanel = vgui::ivgui()->HandleToPanel(gameDialog);
	if (VPanel)
	{
		CDialogGameInfo *gameDialog = dynamic_cast<CDialogGameInfo *>(vgui::ipanel()->GetPanel(VPanel, vgui::GetControlsModuleName()));
		if (gameDialog)
		{
			gameDialog->ChangeGame(gameIP, gamePort);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: forces the game info dialog closed
//-----------------------------------------------------------------------------
void CServerBrowser::CloseGameInfoDialog(GameHandle_t gameDialog)
{
	// forces the dialog closed
	vgui::VPANEL VPanel = vgui::ivgui()->HandleToPanel(gameDialog);
	if (VPanel)
	{
		vgui::ivgui()->PostMessage(VPanel, new KeyValues("Close"), NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: closes all the game info dialogs
//-----------------------------------------------------------------------------
void CServerBrowser::CloseAllGameInfoDialogs()
{
	if (m_hInternetDlg.Get())
	{
		m_hInternetDlg->CloseAllGameInfoDialogs();
	}
}
