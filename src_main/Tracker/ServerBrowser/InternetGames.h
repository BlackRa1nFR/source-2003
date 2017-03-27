//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef INTERNETGAMES_H
#define INTERNETGAMES_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseGamesPage.h"

#include "IGameList.h"
#include "IServerRefreshResponse.h"
#include "netadr.h"
#include "server.h"
#include "UtlVector.h"
#include "UtlSymbol.h"

class CSocket;

//-----------------------------------------------------------------------------
// Purpose: Internet games list
//-----------------------------------------------------------------------------
class CInternetGames : public CBaseGamesPage
{
public:
	CInternetGames(vgui::Panel *parent, const char *panelName = "InternetGames");
	~CInternetGames();

	// property page handlers
	virtual void OnPageShow();
	virtual void OnPageHide();

	// returns true if the game list supports the specified ui elements
	virtual bool SupportsItem(IGameList::InterfaceItem_e item);

	// starts the servers refreshing
	virtual void StartRefresh();

	// gets a new server list
	virtual void GetNewServerList();

	// stops current refresh/GetNewServerList()
	virtual void StopRefresh();

	// returns true if getting server list or refreshing servers
	virtual bool IsRefreshing();

	// adds a new server to list
	virtual void AddNewServer(serveritem_t &server);

	// marks that server list has been fully received
	virtual void ListReceived(bool moreAvailable, int lastUnique);

	// initiates server connection
	virtual void OnBeginConnect();

	// called to look at game info
	virtual void OnViewGameInfo();

	// reapplies filters
	virtual void ApplyFilters();

	// serverlist refresh responses
	virtual void ServerResponded(serveritem_t &server);
	virtual void ServerFailedToRespond(serveritem_t &server);
	virtual void RefreshComplete();

protected:
	// vgui overrides
	virtual void PerformLayout();
	virtual void OnTick();

	DECLARE_PANELMAP();

	// Request server batch, starting at server # Start
	virtual void RequestServers(int Start, const char *filterString);

private:
	// Called once per frame to see if sorting needs to occur again
	void CheckRedoSort();
	// Called once per frame to check re-send request to master server
	void CheckRetryRequest();
	// Removes server from list
	void RemoveServer(serveritem_t &server);
	// continues current refresh
	void ContinueRefresh();
	// opens context menu (user right clicked on a server)
	void OnOpenContextMenu(int row);
	// refreshes a single server
	void OnRefreshServer(int serverID);
	// adds a server to the favorites
	void OnAddToFavorites();

	// server query data
	CSocket				*m_pMaster;	// Master server socket
	CUtlVector<CUtlSymbol> m_MasterServerNames;	// full names of master servers
	netadr_t			m_MasterAddress;	// Address of master server

	int					m_nStartPoint;	// What element to Start at
	float				m_fLastSort;	// Time of last re-sort
	bool				m_bDirty;	// Has the list been modified, thereby needing re-sort

	int					m_nRetriesRemaining;	// When retrieving list of server IPs, # of times remaining to request a batch from master
	float				m_fRequestTime;		// Time of last request
	bool				m_bRequesting;	// Whether we have started requesting servers from the master
	int					m_nLastRequest;	// Starting point of last attempted request
	bool				m_bRequireUpdate;	// checks whether we need an update upon opening
	bool				m_bMoreUpdatesAvailable; // save off last request
	int					m_nLastUnique;

	int m_iServerRefreshCount;	// number of servers refreshed

	typedef CBaseGamesPage BaseClass;
};


#endif // INTERNETGAMES_H
