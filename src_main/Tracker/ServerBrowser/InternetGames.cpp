//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "INetApi.h"
#include "InternetGames.h"
#include "MasterMsgHandler.h"
#include "proto_oob.h"
#include "ServerContextMenu.h"
#include "ServerListCompare.h"
#include "Socket.h"
#include "util.h"
#include "ServerBrowserDialog.h"
#include <GameUI/Random.h>

#include <stdio.h>

#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/MouseCode.h>
#include <KeyValues.h>

#include <vgui_controls/Menu.h>

using namespace vgui;

// Number of retry attempts to get batch of server list
const int NUMBER_OF_RETRIES = 3;
// Time between retries
const float MASTER_LIST_TIMEOUT = 3.0f;
// How often to re-sort the server list
const float MINIMUM_SORT_TIME = 1.5f;
// maximum number of servers to get from master
const int MAXIMUM_SERVERS = 10000;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//			NOTE:	m_Servers can not use more than 96 sockets, else it will
//					cause internet explorer to Stop working under win98 SE!
//-----------------------------------------------------------------------------
CInternetGames::CInternetGames(vgui::Panel *parent, const char *panelName) : CBaseGamesPage(parent, panelName)
{
	// Init server query data
	m_nStartPoint = 0;
	m_fLastSort = 0.0f;
	m_bDirty = false;
	m_nRetriesRemaining = NUMBER_OF_RETRIES;
	m_fRequestTime = 0.0f;
	m_bRequesting = false;
	m_nLastRequest = 0;
	m_bRequireUpdate = true;
	m_bMoreUpdatesAvailable = false;
	m_iServerRefreshCount = 0;

	m_pMaster = new CSocket( "internet dialog master", -1 );
	// The socket will delete the handler
	int bytecode = M2A_SERVER_BATCH;
	m_pMaster->AddMessageHandler(new CMasterMsgHandler(this, CMsgHandler::MSGHANDLER_BYTECODE, &bytecode));

	// load masters from config file
	KeyValues *kv = new KeyValues("MasterServers");
	if (kv->LoadFromFile((IBaseFileSystem*)filesystem(), "servers/MasterServers.vdf" ))
	{
		// iterate the list loading all the servers
		for (KeyValues *srv = kv->GetFirstSubKey(); srv != NULL; srv = srv->GetNextKey())
		{
			m_MasterServerNames.AddToTail(srv->GetString("addr"));
		}
	}
	else
	{
		Assert(!("Could not load file servers/MasterServers.vdf; server browser will not function."));
	}

	// make sure we have at least one master listed
	if (m_MasterServerNames.Count() < 1)
	{
		// add the default master
		m_MasterServerNames.AddToTail("half-life.west.won.net:27010");
	}

	// choose a server at random
	int serverIndex = RandomLong(0, m_MasterServerNames.Count() - 1);
	net->StringToAdr(m_MasterServerNames[serverIndex].String(), &m_MasterAddress);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CInternetGames::~CInternetGames()
{
	delete m_pMaster;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInternetGames::PerformLayout()
{
	if (m_bRequireUpdate)
	{
		GetNewServerList();
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Activates the page, starts refresh if needed
//-----------------------------------------------------------------------------
void CInternetGames::OnPageShow()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called on page hide, stops any refresh
//-----------------------------------------------------------------------------
void CInternetGames::OnPageHide()
{
	StopRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame, maintains sockets and runs refreshes
//-----------------------------------------------------------------------------
void CInternetGames::OnTick()
{
	if (m_pMaster)
	{
		m_pMaster->Frame();
	}

	BaseClass::OnTick();

	// check to see if it's time to get the next master server list
	if (m_Servers.IsRefreshing() && m_Servers.RefreshListRemaining() < 1)
	{
		// server list has sent out all the pings and is now just waiting for replies;
		// the perfect time to ask for the next server list
		if (m_bMoreUpdatesAvailable && !m_bRequesting)
		{
			RequestServers(m_nLastUnique, GetFilterString());
		}
	}

	CheckRedoSort();
	CheckRetryRequest();
}

//-----------------------------------------------------------------------------
// Purpose: Handles incoming server refresh data
//			updates the server browser with the refreshed information from the server itself
//-----------------------------------------------------------------------------
void CInternetGames::ServerResponded(serveritem_t &server)
{
	m_bDirty = true;
	BaseClass::ServerResponded(server);

	m_iServerRefreshCount++;
	if (!m_bMoreUpdatesAvailable && m_pGameList->GetItemCount() > 0)
	{
		wchar_t unicode[128], unicodePercent[6];
		char tempPercent[6];

		int percentDone = (m_iServerRefreshCount * 100) / m_pGameList->GetItemCount();
		if (percentDone < 0)
		{
			percentDone = 0;
		}
		else if (percentDone > 99)
		{
			percentDone = 99;
		}

		itoa( percentDone, tempPercent, 10 );
		localize()->ConvertANSIToUnicode(tempPercent, unicodePercent, sizeof( unicodePercent ));
		localize()->ConstructString(unicode, sizeof( unicode ), localize()->Find("#ServerBrowser_RefreshingPercentDone"), 1, unicodePercent);
		ServerBrowserDialog().UpdateStatusText(unicode);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInternetGames::ServerFailedToRespond(serveritem_t &server)
{
	m_bDirty = true;

	if (server.hadSuccessfulResponse)
	{
		// if it's had a successful response in the past, leave it on
		ServerResponded(server);
	}
	else
	{
		// we've never had a good response from this server, remove it from the list
		RemoveServer(server);
		m_iServerRefreshCount++;
	}

}

//-----------------------------------------------------------------------------
// Purpose: Called when server refresh has been completed
//-----------------------------------------------------------------------------
void CInternetGames::RefreshComplete()
{
	if (!m_bMoreUpdatesAvailable)
	{
		SetRefreshing(false);
	}

	// get the next batch of servers if we're not already getting them
	if (!m_bRequesting)
	{
		if (m_bMoreUpdatesAvailable)
		{
			RequestServers(m_nLastUnique, GetFilterString());
		}
		else
		{
			// finished getting servers, reset Start point
			m_nStartPoint = 0;
			m_bRequesting = false;
			m_iServerRefreshCount = 0;
			m_pGameList->SetEmptyListText("#ServerBrowser_NoInternetGames");

			// perform last sort
			m_bDirty = false;
			m_fLastSort = m_pMaster->GetClock();
			if (IsVisible())
			{
				m_pGameList->SortList();
			}
		}
	}

	UpdateStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInternetGames::GetNewServerList()
{
	// No master socket?
	Assert( m_pMaster );
	if ( !m_pMaster )
	{
		return;
	}

	// Clear the current list
	m_Servers.Clear();
	UpdateStatus();

	m_pGameList->DeleteAllItems();

	m_bRequireUpdate = false;
	m_iServerRefreshCount = 0;

	// Start requesting servers at batch 0
	RequestServers(0, GetFilterString());
}

//-----------------------------------------------------------------------------
// Purpose: Handles details on new server received from master
//			Adds to list and marks it to be pinged/refreshed
//-----------------------------------------------------------------------------
void CInternetGames::AddNewServer(serveritem_t &server)
{
	// add to main server list
	unsigned int index = m_Servers.AddNewServer(server);

	// add to refresh list
	m_Servers.AddServerToRefreshList(index);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the game list supports the specified ui elements
//-----------------------------------------------------------------------------
bool CInternetGames::SupportsItem(IGameList::InterfaceItem_e item)
{
	switch (item)
	{
	case FILTERS:
	case GETNEWLIST:
		return true;

	default:
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInternetGames::CheckRedoSort( void )
{
	float curtime;

	// No changes detected
	if ( !m_bDirty )
		return;

	curtime = m_pMaster->GetClock();
	// Not time yet
	if (curtime - m_fLastSort < MINIMUM_SORT_TIME)
		return;

	// postpown sort if mouse button is down
	if (input()->IsMouseDown(MOUSE_LEFT) || input()->IsMouseDown(MOUSE_RIGHT))
	{
		// don't sort for at least another second
		m_fLastSort = curtime - MINIMUM_SORT_TIME + 1.0f;
		return;
	}

	// Reset timer
	m_bDirty	= false;
	m_fLastSort = curtime;

	// Force sort to occur now!
	m_pGameList->SortList();
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if master server didn't respond to request for servers
//-----------------------------------------------------------------------------
void CInternetGames::CheckRetryRequest( void )
{
	if (!m_bRequesting)
		return;

	float curtime = m_pMaster->GetClock();
	if (curtime - m_fRequestTime <= MASTER_LIST_TIMEOUT)
	{
		return;
	}

	if (--m_nRetriesRemaining <= 0)
	{
		// couldn't connect
		StopRefresh();
		return;
	}

	// Re-send last request
	RequestServers(m_nLastRequest, GetFilterString());
}

//-----------------------------------------------------------------------------
// Purpose: removes the server from the UI list
//-----------------------------------------------------------------------------
void CInternetGames::RemoveServer(serveritem_t &server)
{
	if ( m_pGameList->IsValidItemID(server.listEntryID) )
	{
		// don't remove the server from list, just hide since this is a lot faster
		m_pGameList->SetItemVisible(server.listEntryID, false);

		// find the row in the list and kill
	//	m_pGameList->RemoveItem(server.listEntryID);
	//	server.listEntryID = GetInvalidServerListID();
	}

	UpdateStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInternetGames::StartRefresh()
{
	// if the game list is empty, we need to update
	if (m_pGameList->GetItemCount() < 1 || m_bRequireUpdate)
	{
		GetNewServerList();
	}
	else
	{
		// Stop current refresh
		m_Servers.StopRefresh();

		// build the refresh list from the game list
		for (int i = 0; i < m_pGameList->GetItemCount(); i++)
		{
			int serverID = m_pGameList->GetItemUserData(m_pGameList->GetItemIDFromRow(i));

			m_Servers.AddServerToRefreshList(serverID);
		}

		ContinueRefresh();
	}

	m_iServerRefreshCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Continues current refresh
//-----------------------------------------------------------------------------
void CInternetGames::ContinueRefresh()
{
	m_Servers.StartRefresh();
	SetRefreshing(true);
	if (m_bMoreUpdatesAvailable)
	{
		ServerBrowserDialog().UpdateStatusText("#ServerBrowser_GettingNewServerList");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called after a list of servers has been received from the master
//-----------------------------------------------------------------------------
void CInternetGames::ListReceived(bool moreAvailable, int lastUnique)
{
	if (!m_bRequesting)
		return;

	m_bRequesting = false;
	m_nRetriesRemaining = NUMBER_OF_RETRIES;

	if (m_Servers.ServerCount() < MAXIMUM_SERVERS)
	{
		m_bMoreUpdatesAvailable = moreAvailable;
		m_nLastUnique = lastUnique;
	}
	else
	{
		// we're over the max, so don't get any more servers
		m_bMoreUpdatesAvailable = false;
		m_nLastUnique = 0;
	}

	ContinueRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if getting server list or refreshing servers
//-----------------------------------------------------------------------------
bool CInternetGames::IsRefreshing()
{
	return (m_Servers.IsRefreshing() || m_bRequesting);
}

//-----------------------------------------------------------------------------
// Purpose: stops current refresh, does last sort on list
//-----------------------------------------------------------------------------
void CInternetGames::StopRefresh()
{
	// Stop the server list refreshing
	m_Servers.StopRefresh();

	// clear update states
	m_nStartPoint = 0;
	m_bRequesting = false;
	m_bRequireUpdate = false;
	m_bMoreUpdatesAvailable = false;
	m_nLastUnique = 0;
	m_iServerRefreshCount = 0;

	// update UI
	RefreshComplete();
}

//-----------------------------------------------------------------------------
// Purpose: initiates server connection
//-----------------------------------------------------------------------------
void CInternetGames::OnBeginConnect()
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
void CInternetGames::OnViewGameInfo()
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
// Purpose: applies filters to the current list
//-----------------------------------------------------------------------------
void CInternetGames::ApplyFilters()
{
	ApplyGameFilters();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a set of servers from the master
//-----------------------------------------------------------------------------
void CInternetGames::RequestServers(int Start, const char *filterString)
{
	CMsgBuffer *buffer = m_pMaster->GetSendBuffer();
	Assert( buffer );
	if ( !buffer )
		return;

	m_bRequesting = true;

	SetRefreshing(true);
	ServerBrowserDialog().UpdateStatusText("#ServerBrowser_GettingNewServerList");

	m_fRequestTime = m_pMaster->GetClock();
	m_nLastRequest = Start;

	buffer->Clear();
	// Write query string
	buffer->WriteByte( A2M_GET_SERVERS_BATCH2 );
	// Write first requested id
	buffer->WriteLong( Start );
	// write filter string
	buffer->WriteString(filterString);

	m_pMaster->SendMessage(&m_MasterAddress);
}

//-----------------------------------------------------------------------------
// Purpose: opens context menu (user right clicked on a server)
//-----------------------------------------------------------------------------
void CInternetGames::OnOpenContextMenu(int itemID)
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
//-----------------------------------------------------------------------------
void CInternetGames::OnRefreshServer(int serverID)
{
	// walk the list of selected servers refreshing them
	for (int i = 0; i < m_pGameList->GetSelectedItemsCount(); i++)
	{
		serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(i));

		// refresh this server
		m_Servers.AddServerToRefreshList(serverID);
	}

	ContinueRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: adds a server to the favorites
//-----------------------------------------------------------------------------
void CInternetGames::OnAddToFavorites()
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
MessageMapItem_t CInternetGames::m_MessageMap[] =
{
	MAP_MESSAGE( CInternetGames, "ConnectToServer", OnBeginConnect ),
	MAP_MESSAGE( CInternetGames, "ViewGameInfo", OnViewGameInfo ),
	MAP_MESSAGE( CInternetGames, "AddToFavorites", OnAddToFavorites ),
	MAP_MESSAGE_INT( CInternetGames, "RefreshServer", OnRefreshServer, "serverID" ),
	MAP_MESSAGE_INT( CInternetGames, "OpenContextMenu", OnOpenContextMenu, "itemID" ),
};
IMPLEMENT_PANELMAP(CInternetGames, BaseClass);