//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

// base vgui interfaces
#include <vgui/IInput.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>
#include "FileSystem.h"

// vgui controls
#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/FocusNavGroup.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/MessageBox.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/QueryBox.h>

// serverbrowser headers
#include "inetapi.h"
#include "msgbuffer.h"
#include "proto_oob.h"
#include "ServerContextMenu.h"
#include "socket.h"
#include "util.h"
#include "ServerBrowserDialog.h"
#include "ModList.h"
#include "DialogGameInfo.h"

// game list
#include "InternetGames.h"
#include "FavoriteGames.h"
#include "SpectateGames.h"
#include "LanGames.h"
#include "FriendsGames.h"

using namespace vgui;

static CServerBrowserDialog *s_InternetDlg = NULL;

CServerBrowserDialog &ServerBrowserDialog()
{
	return *CServerBrowserDialog::GetInstance();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServerBrowserDialog::CServerBrowserDialog(vgui::Panel *parent) : Frame(parent, "CServerBrowserDialog")
{
	s_InternetDlg = this;

	m_szGameName[0] = 0;
	m_pSavedData = NULL;

	LoadFilters();

	m_pInternetGames = new CInternetGames(this);
	m_pSpectateGames = new CSpectateGames(this);
	m_pFavorites = new CFavoriteGames(this);
	m_pLanGames = new CLanGames(this);
	m_pFriendsGames = new CFriendsGames(this);

	SetMinimumSize(564, 384);

	m_pGameList = m_pInternetGames;

	m_pContextMenu =  new CServerContextMenu(this);;

	// property sheet
	m_pTabPanel = new PropertySheet(this, "GameTabs");
	m_pTabPanel->SetTabWidth(72);
	m_pTabPanel->AddPage(m_pInternetGames, "#ServerBrowser_InternetTab");
	m_pTabPanel->AddPage(m_pFavorites, "#ServerBrowser_FavoritesTab");
	m_pTabPanel->AddPage(m_pSpectateGames, "#ServerBrowser_SpectateTab");
	m_pTabPanel->AddPage(m_pLanGames, "#ServerBrowser_LanTab");
	m_pTabPanel->AddPage(m_pFriendsGames, "#ServerBrowser_FriendsTab");
	m_pTabPanel->AddActionSignalTarget(this);

	m_pStatusLabel = new Label(this, "StatusLabel", "");

	LoadControlSettingsAndUserConfig("Servers/DialogServerBrowser.res");

	m_pStatusLabel->SetText("");

	// load favorite servers
	KeyValues *favorites = m_pSavedData->FindKey("Favorites", true);
	m_pFavorites->LoadFavoritesList(favorites);

	// load current tab
	const char *gameList = m_pSavedData->GetString("GameList");

	if (!stricmp(gameList, "spectate"))
	{
		m_pTabPanel->SetActivePage(m_pSpectateGames);
	}
	else if (!stricmp(gameList, "favorites"))
	{
		m_pTabPanel->SetActivePage(m_pFavorites);
	}
	else if (!stricmp(gameList, "lan"))
	{
		m_pTabPanel->SetActivePage(m_pLanGames);
	}
	else if (!stricmp(gameList, "friends"))
	{
		m_pTabPanel->SetActivePage(m_pFriendsGames);
	}
	else
	{
		m_pTabPanel->SetActivePage(m_pInternetGames);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServerBrowserDialog::~CServerBrowserDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called once to set up
//-----------------------------------------------------------------------------
void CServerBrowserDialog::Initialize()
{
	SetTitle("Servers", true);
	SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
serveritem_t &CServerBrowserDialog::GetServer(unsigned int serverID)
{
	return m_pGameList->GetServer(serverID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::Open( void )
{	
	m_pTabPanel->RequestFocus();
	// if serverbrowser file is not there we will try to transfer the favorites list.
	FileHandle_t f = filesystem()->Open("ServerBrowser.vdf", "rb", "CONFIG");
	if (f)
	{
		filesystem()->Close( f );
	}
	else
	{		
		m_pFavorites->ImportFavorites(); // import old favorites from old server browser
	}	
	
	surface()->SetMinimized(GetVPanel(), false);
	SetVisible(true);
	RequestFocus();
	m_pTabPanel->RequestFocus();
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: relayouts the dialogs controls
//-----------------------------------------------------------------------------
void CServerBrowserDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	
	// game list in middle
	m_pTabPanel->SetBounds(8, y + 8, GetWide() - 16, tall - (28));
	x += 4;

	// status text along bottom
	m_pStatusLabel->SetBounds(x + 2, (tall - y) + 40, wide - 24, 20);
	m_pStatusLabel->SetContentAlignment(Label::a_northwest);

	Repaint();

}

//-----------------------------------------------------------------------------
// Purpose: This makes it so the menu will always be in front in GameUI
//-----------------------------------------------------------------------------
void CServerBrowserDialog::PaintBackground()
{
	BaseClass::PaintBackground();
	if (m_pContextMenu->IsVisible())
	{
		m_pContextMenu->MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::OnClose()
{
	// bug here if you exit before logging in.
	SaveFilters();
	BaseClass::OnClose();	
}

//-----------------------------------------------------------------------------
// Purpose: Loads filter settings from disk
//-----------------------------------------------------------------------------
void CServerBrowserDialog::LoadFilters()
{
  	// free any old filters
  	if (m_pSavedData)
  	{
  		m_pSavedData->deleteThis();
  	}

	m_pSavedData = new KeyValues ("Filters");
	if (!m_pSavedData->LoadFromFile(filesystem(), "ServerBrowser.vdf", "CONFIG"))
	{
		// doesn't matter if the file is not found, defaults will work successfully and file will be created on exit
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::SaveFilters()
{
	// set the current tab
	if (m_pGameList == m_pSpectateGames)
	{
		m_pSavedData->SetString("GameList", "spectate");
	}
	else if (m_pGameList == m_pFavorites)
	{
		m_pSavedData->SetString("GameList", "favorites");
	}
	else if (m_pGameList == m_pLanGames)
	{
		m_pSavedData->SetString("GameList", "lan");
	}
	else if (m_pGameList == m_pFriendsGames)
	{
		m_pSavedData->SetString("GameList", "friends");
	}
	else
	{
		m_pSavedData->SetString("GameList", "internet");
	}

	// get the favorites list
	KeyValues *favorites = m_pSavedData->FindKey("Favorites", true);
	m_pFavorites->SaveFavoritesList(favorites);
	m_pSavedData->SaveToFile(filesystem(), "ServerBrowser.vdf", "CONFIG");
}

//-----------------------------------------------------------------------------
// Purpose: Updates status test at bottom of window
//-----------------------------------------------------------------------------
void CServerBrowserDialog::UpdateStatusText(const char *fmt, ...)
{
	if ( !m_pStatusLabel )
		return;

	char str[ 1024 ];
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( str, fmt, argptr );
	va_end( argptr );

	m_pStatusLabel->SetText( str );
}

//-----------------------------------------------------------------------------
// Purpose: Updates status test at bottom of window
// Input  : wchar_t* (unicode string) - 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::UpdateStatusText(wchar_t *unicode)
{
	if ( !m_pStatusLabel )
		return;

	m_pStatusLabel->SetText( unicode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::OnGameListChanged()
{
	m_pGameList = dynamic_cast<IGameList *>(m_pTabPanel->GetActivePage());

	UpdateStatusText("");

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a static instance of this dialog
//-----------------------------------------------------------------------------
CServerBrowserDialog *CServerBrowserDialog::GetInstance()
{
	return s_InternetDlg;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a server to the list of favorites
// Input  : &server - 
//-----------------------------------------------------------------------------
void CServerBrowserDialog::AddServerToFavorites(serveritem_t &server)
{
	m_pFavorites->AddNewServer(server);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CServerContextMenu *CServerBrowserDialog::GetContextMenu(vgui::Panel *pPanel)
{
	// create a drop down for this object's states
	if (m_pContextMenu)
		delete m_pContextMenu;
	m_pContextMenu = new CServerContextMenu(this);

    if (!pPanel)
    {
	    m_pContextMenu->SetParent(this);
    }
    else
    {
        m_pContextMenu->SetParent(pPanel);
    }

	m_pContextMenu->SetVisible(false);
	return m_pContextMenu;
}

//-----------------------------------------------------------------------------
// Purpose: begins the process of joining a server from a game list
//			the game info dialog it opens will also update the game list
//-----------------------------------------------------------------------------
CDialogGameInfo *CServerBrowserDialog::JoinGame(IGameList *gameList, unsigned int serverIndex)
{
	// open the game info dialog, then mark it to attempt to connect right away
	CDialogGameInfo *gameDialog = OpenGameInfoDialog(gameList, serverIndex);

	// set the dialog name to be the server name
	gameDialog->Connect();

	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: joins a game by a specified IP, not attached to any game list
//-----------------------------------------------------------------------------
CDialogGameInfo *CServerBrowserDialog::JoinGame(int serverIP, int serverPort, const char *titleName)
{
	// open the game info dialog, then mark it to attempt to connect right away
	CDialogGameInfo *gameDialog = OpenGameInfoDialog(serverIP, serverPort, titleName);

	// set the dialog name to be the server name
	gameDialog->Connect();

	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog from a game list
//-----------------------------------------------------------------------------
CDialogGameInfo *CServerBrowserDialog::OpenGameInfoDialog(IGameList *gameList, unsigned int serverIndex)
{
	CDialogGameInfo *gameDialog = new CDialogGameInfo(gameList, serverIndex);
	serveritem_t &server = gameList->GetServer(serverIndex);
	gameDialog->Run(server.name);
	int i = m_GameInfoDialogs.AddToTail();
	m_GameInfoDialogs[i] = gameDialog;
	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog by a specified IP, not attached to any game list
//-----------------------------------------------------------------------------
CDialogGameInfo *CServerBrowserDialog::OpenGameInfoDialog(int serverIP, int serverPort, const char *titleName)
{
	CDialogGameInfo *gameDialog = new CDialogGameInfo(NULL, 0, serverIP, serverPort);
	gameDialog->Run(titleName);
	int i = m_GameInfoDialogs.AddToTail();
	m_GameInfoDialogs[i] = gameDialog;
	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: closes all the game info dialogs
//-----------------------------------------------------------------------------
void CServerBrowserDialog::CloseAllGameInfoDialogs()
{
	for (int i = 0; i < m_GameInfoDialogs.Count(); i++)
	{
		vgui::Panel *dlg = m_GameInfoDialogs[i];
		if (dlg)
		{
			vgui::ivgui()->PostMessage(dlg->GetVPanel(), new KeyValues("Close"), NULL);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: accessor to the filter save data
//-----------------------------------------------------------------------------
KeyValues *CServerBrowserDialog::GetFilterSaveData(const char *filterSet)
{
	KeyValues *filterList = m_pSavedData->FindKey("FilterList", true);
	return filterList->FindKey(filterSet, true);
}

//-----------------------------------------------------------------------------
// Purpose: gets the name of the mod directory we're restricted to accessing, NULL if none
//-----------------------------------------------------------------------------
const char *CServerBrowserDialog::GetActiveModName()
{
	return m_szGameName[0] ? m_szGameName : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: receives a specified game is active, so no other game types can be displayed in server list
//-----------------------------------------------------------------------------
void CServerBrowserDialog::OnActiveGameName(const char *gameName)
{
	v_strncpy(m_szGameName, gameName, sizeof(m_szGameName));

	// reload filter settings (since they are no forced to be game specific)
	m_pInternetGames->LoadFilterSettings();
	m_pSpectateGames->LoadFilterSettings();
	m_pFavorites->LoadFilterSettings();
	m_pLanGames->LoadFilterSettings();
	m_pFriendsGames->LoadFilterSettings();
}

//-----------------------------------------------------------------------------
// Purpose: Passes build mode activation down into the pages
//-----------------------------------------------------------------------------
void CServerBrowserDialog::ActivateBuildMode()
{
	// no subpanel, no build mode
	EditablePanel *panel = dynamic_cast<EditablePanel *>(m_pTabPanel->GetActivePage());
	if (!panel)
		return;

	panel->ActivateBuildMode();
}

//-----------------------------------------------------------------------------
// Purpose: gets the default position and size on the screen to appear the first time
//-----------------------------------------------------------------------------
bool CServerBrowserDialog::GetDefaultScreenPosition(int &x, int &y, int &wide, int &tall)
{
	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds(wx, wy, ww, wt);
	x = wx + (int)(ww * 0.05);
	y = wy + (int)(wt * 0.4);
	wide = (int)(ww * 0.5);
	tall = (int)(wt * 0.55);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: safe string copy
//-----------------------------------------------------------------------------
void v_strncpy(char *dest, const char *src, int bufsize)
{
	if (src == dest)
		return;

	strncpy(dest, src, bufsize - 1);
	dest[bufsize - 1] = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CServerBrowserDialog::m_MessageMap[] =
{
	MAP_MESSAGE( CServerBrowserDialog, "PageChanged", OnGameListChanged ),
	MAP_MESSAGE_CONSTCHARPTR( CServerBrowserDialog, "ActiveGameName", OnActiveGameName, "name" ),
};
IMPLEMENT_PANELMAP(CServerBrowserDialog, vgui::Frame);
