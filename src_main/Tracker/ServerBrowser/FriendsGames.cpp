//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "FriendsGames.h"

#include "proto_oob.h"
#include "ServerContextMenu.h"
#include "ServerListCompare.h"
#include "Socket.h"
#include "util.h"
#include "ServerBrowserDialog.h"
#include "Friends/IFriendsUser.h"
#include "InternetGames.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>

#include <vgui_controls/ListPanel.h>
#include <vgui_controls/ImagePanel.h>

using namespace vgui;

extern IFriendsUser *g_pFriendsUser;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFriendsGames::CFriendsGames(vgui::Panel *parent) : CBaseGamesPage(parent, "FriendsGames")
{
	m_iServerRefreshCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CFriendsGames::~CFriendsGames()
{
}

//-----------------------------------------------------------------------------
// Purpose: Activates the page, starts refresh
//-----------------------------------------------------------------------------
void CFriendsGames::OnPageShow()
{
	StartRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: Called on page hide, stops any refresh
//-----------------------------------------------------------------------------
void CFriendsGames::OnPageHide()
{
	StopRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the game list supports the specified ui elements
//-----------------------------------------------------------------------------
bool CFriendsGames::SupportsItem(InterfaceItem_e item)
{
	switch (item)
	{
	case FILTERS:
		return true;

	case GETNEWLIST:
	default:
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: starts the servers refreshing
//-----------------------------------------------------------------------------
void CFriendsGames::StartRefresh()
{
	// Stop current refresh
	m_Servers.StopRefresh();

	// rebuild list
	GetNewServerList();
}

//-----------------------------------------------------------------------------
// Purpose: generates new server list from the tracker friends list
//-----------------------------------------------------------------------------
void CFriendsGames::GetNewServerList()
{
	if (!g_pFriendsUser)
		return;

	// Stop any refresh
	StopRefresh();

	// clear list
	m_Servers.Clear();
	m_pGameList->DeleteAllItems();

	// iterate through the friends looking for games
	int friendCount = g_pFriendsUser->GetNumberOfBuddies();
	for (int i = 0; i < friendCount; i++)
	{
		int friendID = g_pFriendsUser->GetBuddyFriendsID(i);
		int ip = 0, port = 0;
		if (g_pFriendsUser->GetBuddyGameAddress(friendID, &ip, &port))
		{
			// add server to refresh list
			serveritem_t server;
			memset(&server, 0, sizeof(server));

			*((int *)server.ip) = ip;
			server.port = port;

			AddNewServer(server);
		}
	}

	ListReceived(false, 0);
}

//-----------------------------------------------------------------------------
// Purpose: stops current refresh/GetNewServerList()
//-----------------------------------------------------------------------------
void CFriendsGames::StopRefresh()
{
	// Stop the server list refreshing
	m_Servers.StopRefresh();

	// clear update states
	m_iServerRefreshCount = 0;

	// update UI
	RefreshComplete();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the list is currently refreshing servers
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFriendsGames::IsRefreshing()
{
	return m_Servers.IsRefreshing();
}

//-----------------------------------------------------------------------------
// Purpose: adds a new server to list
// Input  : &server - 
//-----------------------------------------------------------------------------
void CFriendsGames::AddNewServer(serveritem_t &newServer)
{
	// walk the server list, making sure this server is unique
	for (int i = 0; i < m_Servers.ServerCount(); i++)
	{
		serveritem_t &comp = m_Servers.GetServer(i);

		if (comp.ip[0] == newServer.ip[0] && comp.ip[1] == newServer.ip[1] 
			&& comp.ip[2] == newServer.ip[2] && comp.ip[3] == newServer.ip[3] 
			&& comp.port == newServer.port)
		{
			// we have a match, just bail
			return;
		}
	}

	// copy server into main server list
	unsigned int index = m_Servers.AddNewServer(newServer);

	// reget the server
	serveritem_t &server = m_Servers.GetServer(index);
	server.hadSuccessfulResponse = true;
	server.doNotRefresh = false;
	server.listEntryID = GetInvalidServerListID();
	server.serverID = index;

	// add to refresh list
	m_Servers.AddServerToRefreshList(index);
}

//-----------------------------------------------------------------------------
// Purpose: Continues the refresh
// Input  : moreAvailable - 
//			lastUnique - 
//-----------------------------------------------------------------------------
void CFriendsGames::ListReceived(bool moreAvailable, int lastUnique)
{
	m_Servers.StartRefresh();

	// setup UI
	SetRefreshing(IsRefreshing());
}

//-----------------------------------------------------------------------------
// Purpose: called when Connect button is pressed
//-----------------------------------------------------------------------------
void CFriendsGames::OnBeginConnect()
{
	if ( !m_pGameList->GetSelectedItemsCount() )
		return;
	
	// get the server
	int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(0));

	// Stop the current refresh
	StopRefresh();
	
	// join the game
	ServerBrowserDialog().JoinGame(this, serverID);
}

//-----------------------------------------------------------------------------
// Purpose: Displays the current game info without connecting
//-----------------------------------------------------------------------------
void CFriendsGames::OnViewGameInfo()
{
	if ( !m_pGameList->GetSelectedItemsCount() )
		return;

	// get the server
	int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(0));

	// Stop the current refresh
	StopRefresh();

	// join the game
	ServerBrowserDialog().OpenGameInfoDialog(this, serverID);
}

//-----------------------------------------------------------------------------
// Purpose: reapplies filters (does nothing with this)
//-----------------------------------------------------------------------------
void CFriendsGames::ApplyFilters()
{
	ApplyGameFilters();
}

//-----------------------------------------------------------------------------
// Purpose: called when a server response has timed out, treated just like a normal server
//-----------------------------------------------------------------------------
void CFriendsGames::ServerFailedToRespond(serveritem_t &server)
{
	ServerResponded(server);
}

//-----------------------------------------------------------------------------
// Purpose: called when the current refresh list is complete
//-----------------------------------------------------------------------------
void CFriendsGames::RefreshComplete()
{
	SetRefreshing(false);
	m_pGameList->SortList();
	m_iServerRefreshCount = 0;

	// set empty message
	m_pGameList->SetEmptyListText("#ServerBrowser_NoFriendsServers");
}

//-----------------------------------------------------------------------------
// Purpose: opens context menu (user right clicked on a server)
//-----------------------------------------------------------------------------
void CFriendsGames::OnOpenContextMenu(int itemID)
{
	if (!m_pGameList->GetSelectedItemsCount())
		return;

	// get the server
	int serverID = m_pGameList->GetItemData(m_pGameList->GetSelectedItem(0))->userData;
	serveritem_t &server = m_Servers.GetServer(serverID);

	// Activate context menu
	CServerContextMenu *menu = ServerBrowserDialog().GetContextMenu(m_pGameList);
	menu->ShowMenu(this, serverID, true, true, true);
}

//-----------------------------------------------------------------------------
// Purpose: refreshes a single server
// Input  : serverID - 
//-----------------------------------------------------------------------------
void CFriendsGames::OnRefreshServer(int serverID)
{
	// walk the list of selected servers refreshing them
	for (int i = 0; i < m_pGameList->GetSelectedItemsCount(); i++)
	{
		serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(i));
		
		// refresh this server
		m_Servers.AddServerToRefreshList(serverID);
	}

	m_Servers.StartRefresh();
	SetRefreshing(IsRefreshing());
}

//-----------------------------------------------------------------------------
// Purpose: adds a server to the favorites
//-----------------------------------------------------------------------------
void CFriendsGames::OnAddToFavorites()
{
	// loop through all the selected favorites
	for (int i = 0; i < m_pGameList->GetSelectedItemsCount(); i++)
	{
		int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(i));
		if (serverID >= m_Servers.ServerCount())
			continue;
		
		serveritem_t &server = m_Servers.GetServer(serverID);

		// add to favorites list
		ServerBrowserDialog().AddServerToFavorites(server);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CFriendsGames::m_MessageMap[] =
{
	MAP_MESSAGE( CFriendsGames, "ConnectToServer", OnBeginConnect ),
	MAP_MESSAGE( CFriendsGames, "ViewGameInfo", OnViewGameInfo ),
	MAP_MESSAGE( CFriendsGames, "AddToFavorites", OnAddToFavorites ),
	MAP_MESSAGE_INT( CFriendsGames, "RefreshServer", OnRefreshServer, "serverID" ),
	MAP_MESSAGE_INT( CFriendsGames, "OpenContextMenu", OnOpenContextMenu, "itemID" ),
};
IMPLEMENT_PANELMAP(CFriendsGames, BaseClass);

