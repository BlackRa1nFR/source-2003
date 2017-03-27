//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERLIST_H
#define SERVERLIST_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlVector.h"
#include "../TrackerNET/NetAddress.h"

#include <VGUI_Panel.h>

class IReceiveMessage;

enum ServerState_t
{
	SERVER_UNKNOWN,				// server has unknown state
	SERVER_ACTIVE,				// server is running normally
	SERVER_DOWN,				// server is deliberately down
	SERVER_NOTRESPONDING,		// server is crashed/unresponsive
	SERVER_SHUTTINGDOWN,		// server is in the process of being manually Shutdown
};

//-----------------------------------------------------------------------------
// Purpose: information about a tracker server
//-----------------------------------------------------------------------------
struct server_t
{
	CNetAddress addr;
	char name[128];	// dns name of the server

	int state;

	double refreshTime;		// time to refresh this server at, in seconds

	double lastSend;		// last time a refresh was sent to this server
	double lastReceive;		// last time a refresh was received from this server

	// data about the server
	int users;
	int maxUsers;
	int fps;
	int networkBuffers;
	int peakNetworkBuffers;
	int dbInBufs;	// number of buffers in outgoing sqldb queue
	int dbOutBufs; // number of buffers in incoming (completed) sqldb queue
	int bytesReceivedPerSecond;
	int bytesSentPerSecond;

	int primary;

	bool underHeavyLoad;
};

//-----------------------------------------------------------------------------
// Purpose: Holds the list of tracker servers to watch
//-----------------------------------------------------------------------------
class CServerList
{
public:
	CServerList(vgui::Panel *notifyPanel);
	~CServerList();

	// must be called every frame to handle networking I/O
	void RunFrame();

	// sends out a broadcast ping looking for servers
	void BroadcastForServers();

	// adds/finds a server to/from the list
	server_t &FindServer(CNetAddress &addr);

	// refreshes the details of a server
	void RefreshServer(server_t &server);

	// returns the number of servers
	int ServerCount();	

	// returns a server by index
	server_t &GetServer(int index);

private:
	void ReceivedMsg_MonitorInfo(IReceiveMessage *msg);
	void ReceivedMsg_SrvInfo(IReceiveMessage *msg);

	void CheckOnServer(server_t &server);

	vgui::Panel *m_pNotifyPanel;
	void ReceivedData(IReceiveMessage *msg);
	CUtlVector<server_t> m_Servers;

	bool m_bHasDBInfo;

	double m_flRebroadcastTime;
};


#endif // SERVERLIST_H
