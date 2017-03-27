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
#include <VGUI_Controls.h>
#include <VGUI_IInput.h>
#include <VGUI_ISurface.h>
#include <VGUI_IScheme.h>
#include <VGUI_IVGui.h>
#include <VGUI_MouseCode.h>
#include "FileSystem.h"

// vgui controls
#include <VGUI_Button.h>
#include <VGUI_CheckButton.h>
#include <VGUI_ComboBox.h>
#include <VGUI_FocusNavGroup.h>
#include <VGUI_Frame.h>
#include <VGUI_KeyValues.h>
#include <VGUI_ListPanel.h>
#include <VGUI_MessageBox.h>
#include <VGUI_Panel.h>
#include <VGUI_PropertySheet.h>
#include <VGUI_QueryBox.h>


// serverbrowser headers
#include "inetapi.h"
#include "msgbuffer.h"
#include "proto_oob.h"
#include "ServerContextMenu.h"
#include "socket.h"
#include "util.h"
#include "vinternetdlg.h"
#include "ModList.h"
#include "DialogGameInfo.h"

// game list
#include "InternetGames.h"
#include "FavoriteGames.h"
#include "SpectateGames.h"
#include "LanGames.h"
#include "FriendsGames.h"

// interface to game engine / tracker
#include "IRunGameEngine.h"

using namespace vgui;

static VInternetDlg *s_InternetDlg = NULL;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
VInternetDlg::VInternetDlg( void ) : Frame(NULL, "VInternetDlg")
{
	s_InternetDlg = this;
	MakePopup();

	m_szGameName[0] = 0;

	m_pSavedData = NULL;

	// load filters
	LoadFilters();

	m_pInternetGames = new CInternetGames();
	m_pSpectateGames = new CSpectateGames();
	m_pFavorites = new CFavoriteGames();

	m_pLanGames = new CLanGames();
	m_pFriendsGames = new CFriendsGames();

	SetMinimumSize(512, 384);

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

	LoadControlSettings("Servers/DialogServerBrowser.res");

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

	// load window settings
	LoadDialogState(this, "ServerBrowser");

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
VInternetDlg::~VInternetDlg()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called once to set up
//-----------------------------------------------------------------------------
void VInternetDlg::Initialize()
{
	SetTitle("Servers", true);
	SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : serverID - 
// Output : serveritem_t
//-----------------------------------------------------------------------------
serveritem_t &VInternetDlg::GetServer(unsigned int serverID)
{
	return m_pGameList->GetServer(serverID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VInternetDlg::Open( void )
{	
	m_pTabPanel->RequestFocus();
	// if serverbrowser file is not there we will try to transfer the favorites list.
	FileHandle_t f = filesystem()->Open("ServerBrowser.vdf", "rb");
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
void VInternetDlg::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	
	// game list in middle
	m_pTabPanel->SetBounds(8, y + 8, GetWide() - 16, tall - (28));
	x += 4;

	// status text along bottom
	m_pStatusLabel->SetBounds(x + 2, (tall - y) + 40, wide - 6, 20);
	m_pStatusLabel->SetContentAlignment(Label::a_northwest);

	Repaint();

}

//-----------------------------------------------------------------------------
// Purpose: This makes it so the menu will always be in front in GameUI
//-----------------------------------------------------------------------------
void VInternetDlg::PaintBackground()
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
void VInternetDlg::OnClose()
{
	// bug here if you exit before logging in.
	SaveDialogState(this, "ServerBrowser");
	SaveFilters();
	Frame::OnClose();	
}

//-----------------------------------------------------------------------------
// Purpose: Loads filter settings from disk
//-----------------------------------------------------------------------------
void VInternetDlg::LoadFilters()
{
  	// free any old filters
  	if (m_pSavedData)
  	{
  		m_pSavedData->deleteThis();
  	}

	m_pSavedData = new KeyValues ("Filters");
	if (!m_pSavedData->LoadFromFile(filesystem(), "ServerBrowser.vdf", true, "CONFIG"))
	{
		// doesn't matter if the file is not found, defaults will work successfully and file will be created on exit
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VInternetDlg::SaveFilters()
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
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void VInternetDlg::UpdateStatusText(const char *fmt, ...)
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
void VInternetDlg::UpdateStatusText(wchar_t *unicode)
{
	if ( !m_pStatusLabel )
		return;

	m_pStatusLabel->SetText( unicode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VInternetDlg::OnGameListChanged()
{
	m_pGameList = dynamic_cast<IGameList *>(m_pTabPanel->GetActivePage());

	UpdateStatusText("");

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a static instance of this dialog
// Output : VInternetDlg
//-----------------------------------------------------------------------------
VInternetDlg *VInternetDlg::GetInstance()
{
	return s_InternetDlg;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a server to the list of favorites
// Input  : &server - 
//-----------------------------------------------------------------------------
void VInternetDlg::AddServerToFavorites(serveritem_t &server)
{
	m_pFavorites->AddNewServer(server);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CServerContextMenu
//-----------------------------------------------------------------------------
CServerContextMenu *VInternetDlg::GetContextMenu()
{
	// create a drop down for this object's states
	if (m_pContextMenu)
		delete m_pContextMenu;
	m_pContextMenu = new CServerContextMenu(this);
	m_pContextMenu->SetParent(this);
	m_pContextMenu->SetVisible(false);
	return m_pContextMenu;
}

//-----------------------------------------------------------------------------
// Purpose: begins the process of joining a server from a game list
//			the game info dialog it opens will also update the game list
//-----------------------------------------------------------------------------
CDialogGameInfo *VInternetDlg::JoinGame(IGameList *gameList, unsigned int serverIndex)
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
CDialogGameInfo *VInternetDlg::JoinGame(int serverIP, int serverPort, const char *titleName)
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
CDialogGameInfo *VInternetDlg::OpenGameInfoDialog(IGameList *gameList, unsigned int serverIndex)
{
	CDialogGameInfo *gameDialog = new CDialogGameInfo(gameList, serverIndex);
	serveritem_t &server = gameList->GetServer(serverIndex);
	gameDialog->Run(server.name);
	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog by a specified IP, not attached to any game list
//-----------------------------------------------------------------------------
CDialogGameInfo *VInternetDlg::OpenGameInfoDialog(int serverIP, int serverPort, const char *titleName)
{
	CDialogGameInfo *gameDialog = new CDialogGameInfo(NULL, 0, serverIP, serverPort);
	gameDialog->Run(titleName);
	return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: Save position and window size of a dialog from the .vdf file.
// Input  : *dialog - panel we are setting position and size 
//			*dialogName -  name of dialog in the .vdf file
//-----------------------------------------------------------------------------
void VInternetDlg::SaveDialogState(Panel *dialog, const char *dialogName)
{
	// write the size and position to the document
	int x, y, wide, tall;
	dialog->GetBounds(x, y, wide, tall);

	KeyValues *data;
	data = m_pSavedData->FindKey(dialogName, true);

	data->SetInt("x", x);
	data->SetInt("y", y);
	data->SetInt("w", wide);
	data->SetInt("t", tall);
}

//-----------------------------------------------------------------------------
// Purpose: Load position and window size of a dialog from the .vdf file.
// Input  : *dialog - panel we are setting position and size 
//			*dialogName -  name of dialog in the .vdf file
//-----------------------------------------------------------------------------
void VInternetDlg::LoadDialogState(Panel *dialog, const char *dialogName)
{						   
	// read the size and position from the document
	KeyValues *data;
	data = m_pSavedData->FindKey(dialogName, true);

	// calculate defaults, center of the screen
	int x, y, wide, tall, dwide, dtall;
	int nx, ny, nwide, ntall;
	vgui::surface()->GetScreenSize(wide, tall);
	dialog->GetSize(dwide, dtall);
	x = (int)((wide - dwide) * 0.5);
	y = (int)((tall - dtall) * 0.5);

	// set dialog
	nx = data->GetInt("x", x);
	ny = data->GetInt("y", y);
	nwide = data->GetInt("w", dwide);
	ntall = data->GetInt("t", dtall);

	// make sure it's on the screen. If it isn't, move it over so it is.
	if (nx + nwide > wide)
	{
		nx = wide - nwide;
	}
	if (ny + ntall > tall)
	{
		ny = tall - ntall;
	}
	if (nx < 0)
	{
		nx = 0;
	}
	if (ny < 0)
	{
		ny = 0;
	}

	dialog->SetBounds(nx, ny, nwide, ntall);
}

//-----------------------------------------------------------------------------
// Purpose: accessor to the filter save data
//-----------------------------------------------------------------------------
vgui::KeyValues *VInternetDlg::GetFilterSaveData(const char *filterSet)
{
	vgui::KeyValues *filterList = m_pSavedData->FindKey("FilterList", true);
	return filterList->FindKey(filterSet, true);
}

//-----------------------------------------------------------------------------
// Purpose: gets the name of the mod directory we're restricted to accessing, NULL if none
//-----------------------------------------------------------------------------
const char *VInternetDlg::GetActiveModName()
{
	return m_szGameName[0] ? m_szGameName : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: receives a specified game is active, so no other game types can be displayed in server list
//-----------------------------------------------------------------------------
void VInternetDlg::OnActiveGameName(const char *gameName)
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
MessageMapItem_t VInternetDlg::m_MessageMap[] =
{
	MAP_MESSAGE( VInternetDlg, "PageChanged", OnGameListChanged ),
	MAP_MESSAGE_CONSTCHARPTR( VInternetDlg, "ActiveGameName", OnActiveGameName, "name" ),
};
IMPLEMENT_PANELMAP(VInternetDlg, vgui::Frame);
