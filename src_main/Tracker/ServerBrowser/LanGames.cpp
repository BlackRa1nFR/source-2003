//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "LanGames.h"

#include "LanBroadcastMsgHandler.h"
#include "proto_oob.h"
#include "ServerContextMenu.h"
#include "ServerListCompare.h"
#include "Socket.h"
#include "util.h"
#include "ServerBrowserDialog.h"
#include "InternetGames.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ListPanel.h>

using namespace vgui;

const float BROADCAST_LIST_TIMEOUT = 0.4f;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CLanGames::CLanGames(vgui::Panel *parent) : CBaseGamesPage(parent, "LanGames")
{
	m_iServerRefreshCount = 0;
	m_bRequesting = false;

	m_pBroadcastSocket = new CSocket( "lan broadcast", -1 );
	m_pLanBroadcastMsgHandler = new CLanBroadcastMsgHandler(this, CMsgHandler::MSGHANDLER_ALL);
	m_pBroadcastSocket->AddMessageHandler(m_pLanBroadcastMsgHandler);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CLanGames::~CLanGames()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLanGames::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Activates the page, starts refresh
//-----------------------------------------------------------------------------
void CLanGames::OnPageShow()
{
	StartRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: Called on page hide, stops any refresh
//-----------------------------------------------------------------------------
void CLanGames::OnPageHide()
{
	StopRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame
//-----------------------------------------------------------------------------
void CLanGames::OnTick()
{
	m_pBroadcastSocket->Frame();
	BaseClass::OnTick();
	CheckRetryRequest();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the game list supports the specified ui elements
//-----------------------------------------------------------------------------
bool CLanGames::SupportsItem(InterfaceItem_e item)
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
void CLanGames::StartRefresh()
{
	// Stop current refresh
	m_Servers.StopRefresh();

	// get new server list
	GetNewServerList();

	// display us as refreshing
	SetRefreshing(true);
}

//-----------------------------------------------------------------------------
// Purpose: gets a new server list
//-----------------------------------------------------------------------------
void CLanGames::GetNewServerList()
{
	// Clear the current list
	m_Servers.Clear();
	m_pGameList->DeleteAllItems();
	m_bRequesting = true;
	SetRefreshing(true);

	CMsgBuffer *buffer = m_pBroadcastSocket->GetSendBuffer();
	buffer->Clear();
	buffer->WriteLong(-1);
	buffer->WriteString("infostring");

	// broadcast message
	m_pBroadcastSocket->Broadcast(27015);
	m_pBroadcastSocket->Broadcast(27016);
	m_pBroadcastSocket->Broadcast(27017);
	m_pBroadcastSocket->Broadcast(27018);
	m_pBroadcastSocket->Broadcast(27019);
	m_pBroadcastSocket->Broadcast(27020);

	// get the time
	m_fRequestTime = CSocket::GetClock();
	m_pLanBroadcastMsgHandler->SetRequestTime(m_fRequestTime);
}

//-----------------------------------------------------------------------------
// Purpose: stops current refresh/GetNewServerList()
//-----------------------------------------------------------------------------
void CLanGames::StopRefresh()
{
	// Stop the server list refreshing
	m_Servers.StopRefresh();

	// clear update states
	m_iServerRefreshCount = 0;
	m_bRequesting = false;

	// update UI
	RefreshComplete();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the list is currently refreshing servers
//-----------------------------------------------------------------------------
bool CLanGames::IsRefreshing()
{
	return m_Servers.IsRefreshing() || m_bRequesting;
}

//-----------------------------------------------------------------------------
// Purpose: adds a new server to list
//-----------------------------------------------------------------------------
void CLanGames::AddNewServer(serveritem_t &newServer)
{
	// copy server into main server list
	unsigned int index = m_Servers.AddNewServer(newServer);

	// reget the server
	serveritem_t &server = m_Servers.GetServer(index);
	server.hadSuccessfulResponse = true;
	server.doNotRefresh = false;

	server.listEntryID = GetInvalidServerListID();
	server.serverID = index;

	// mark the server as ready to be refreshed
	m_Servers.AddServerToRefreshList(server.serverID);
}

//-----------------------------------------------------------------------------
// Purpose: Continues the refresh
//-----------------------------------------------------------------------------
void CLanGames::ListReceived(bool moreAvailable, int lastUnique)
{
	m_Servers.StartRefresh();
	m_bRequesting = false;
	m_iServerRefreshCount = 0;

	SetRefreshing(IsRefreshing());
}

//-----------------------------------------------------------------------------
// Purpose: called when Connect button is pressed
//-----------------------------------------------------------------------------
void CLanGames::OnBeginConnect()
{
	if (!m_pGameList->GetSelectedItemsCount())
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
void CLanGames::OnViewGameInfo()
{
	if (!m_pGameList->GetSelectedItemsCount())
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
void CLanGames::ApplyFilters()
{
	ApplyGameFilters();
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if we've finished looking for local servers
//-----------------------------------------------------------------------------
void CLanGames::CheckRetryRequest()
{
	if (!m_bRequesting)
		return;

	float curtime = CSocket::GetClock();
	if (curtime - m_fRequestTime <= BROADCAST_LIST_TIMEOUT)
	{
		return;
	}

	// time has elapsed, finish up
	m_bRequesting = false;
	ListReceived(false, 0);
}

//-----------------------------------------------------------------------------
// Purpose: called when a server response has timed out, remove it
//-----------------------------------------------------------------------------
void CLanGames::ServerFailedToRespond(serveritem_t &server)
{
	if ( m_pGameList->IsValidItemID(server.listEntryID) )
	{
		// find the row in the list and kill
		m_pGameList->RemoveItem(server.listEntryID);
		server.listEntryID = GetInvalidServerListID();
	}
}

//-----------------------------------------------------------------------------
// Purpose: called when the current refresh list is complete
//-----------------------------------------------------------------------------
void CLanGames::RefreshComplete()
{
	SetRefreshing(IsRefreshing());
	m_pGameList->SortList();
	m_iServerRefreshCount = 0;
	m_pGameList->SetEmptyListText("#ServerBrowser_NoLanServers");
}

//-----------------------------------------------------------------------------
// Purpose: opens context menu (user right clicked on a server)
//-----------------------------------------------------------------------------
void CLanGames::OnOpenContextMenu(int row)
{
	if (!m_pGameList->GetSelectedItemsCount())
		return;

	// get the server
	int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(0));
	serveritem_t &server = m_Servers.GetServer(serverID);

	// Activate context menu
	CServerContextMenu *menu = ServerBrowserDialog().GetContextMenu(m_pGameList);
	menu->ShowMenu(this, serverID, true, true, false);
}

//-----------------------------------------------------------------------------
// Purpose: refreshes a single server
//-----------------------------------------------------------------------------
void CLanGames::OnRefreshServer(int serverID)
{
	// walk the list of selected servers refreshing them
	for (int i = 0; i < m_pGameList->GetSelectedItemsCount(); i++)
	{
		int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(i));
			
		// refresh this server
		m_Servers.AddServerToRefreshList(serverID);
	}

	m_Servers.StartRefresh();
	SetRefreshing(IsRefreshing());
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CLanGames::m_MessageMap[] =
{
	MAP_MESSAGE( CLanGames, "ConnectToServer", OnBeginConnect ),
	MAP_MESSAGE( CLanGames, "ViewGameInfo", OnViewGameInfo ),
	MAP_MESSAGE_INT( CLanGames, "RefreshServer", OnRefreshServer, "serverID" ),
	MAP_MESSAGE_INT( CLanGames, "OpenContextMenu", OnOpenContextMenu, "itemID" ),
};
IMPLEMENT_PANELMAP(CLanGames, BaseClass);
