//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LANGAMES_H
#define LANGAMES_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseGamesPage.h"

#include "IGameList.h"
#include "server.h"
#include "UtlVector.h"

class CLanBroadcastMsgHandler;

//-----------------------------------------------------------------------------
// Purpose: Favorite games list
//-----------------------------------------------------------------------------
class CLanGames : public CBaseGamesPage
{
public:
	CLanGames(vgui::Panel *parent);
	~CLanGames();

	// property page handlers
	virtual void OnPageShow();
	virtual void OnPageHide();

	// IGameList handlers
	// returns true if the game list supports the specified ui elements
	virtual bool SupportsItem(InterfaceItem_e item);

	// starts the servers refreshing
	virtual void StartRefresh();

	// gets a new server list
	virtual void GetNewServerList();

	// stops current refresh/GetNewServerList()
	virtual void StopRefresh();

	// returns true if the list is currently refreshing servers
	virtual bool IsRefreshing();

	// adds a new server to list
	virtual void AddNewServer(serveritem_t &server);

	// marks that server list has been fully received
	virtual void ListReceived(bool moreAvailable, int lastUnique);

	// called when Connect button is pressed
	virtual void OnBeginConnect();

	// called to look at game info
	virtual void OnViewGameInfo();

	// reapplies filters (does nothing with this)
	virtual void ApplyFilters();

	// IServerRefreshResponse handlers
	// called when a server response has timed out
	virtual void ServerFailedToRespond(serveritem_t &server);

	// called when the current refresh list is complete
	virtual void RefreshComplete();

private:
	// vgui message handlers
	virtual void PerformLayout();
	virtual void OnTick();

	// lan timeout checking
	virtual void CheckRetryRequest();

	// context menu message handlers
	void OnOpenContextMenu(int row);
	void OnRefreshServer(int serverID);

	// number of servers refreshed
	int m_iServerRefreshCount;	

	// true if we're broadcasting for servers
	bool m_bRequesting;

	// time at which we last broadcasted
	float m_fRequestTime;

	// broadcast socket
	CSocket	*m_pBroadcastSocket;

	DECLARE_PANELMAP();
	typedef CBaseGamesPage BaseClass;

	CLanBroadcastMsgHandler *m_pLanBroadcastMsgHandler;
};



#endif // LANGAMES_H
