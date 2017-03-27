//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "ServerList.h"
#include "TrackerProtocol.h"

#include "../TrackerNET/TrackerNET_Interface.h"
#include "../TrackerNET/Threads.h"
#include "../TrackerNET/NetAddress.h"

#include <VGUI_Panel.h>
#include <VGUI_KeyValues.h>

#include <winsock2.h>
#undef SetPort
#undef SendMessage
#undef PostMessage

extern ITrackerNET *g_pTrackerNET;
IThreads *g_pThreads = NULL;

// broadcast for new servers every xx seconds
static const float BROADCAST_INTERVAL = 40.0f;

// refresh server info every xx seconds
static const float REFRESH_INTERVAL = 5.0f;

// interval before server is marked as crashed
static const float TIMEOUT_INTERVAL = 7.0f;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServerList::CServerList(vgui::Panel *notifyPanel) : m_pNotifyPanel(notifyPanel)
{
	g_pThreads = g_pTrackerNET->GetThreadAPI();
	m_bHasDBInfo = false;

	// Start out looking for servers
	BroadcastForServers();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServerList::~CServerList()
{
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of servers
// Output : int
//-----------------------------------------------------------------------------
int CServerList::ServerCount()
{
	return m_Servers.Size();
}

//-----------------------------------------------------------------------------
// Purpose: returns a server by index
// Input  : index - 
// Output : server_t
//-----------------------------------------------------------------------------
server_t &CServerList::GetServer(int index)
{
	return m_Servers[index];
}

//-----------------------------------------------------------------------------
// Purpose: Runs a frame of networking
//-----------------------------------------------------------------------------
void CServerList::RunFrame()
{
	// check networking messages
	IReceiveMessage *recv;
	while ((recv = g_pTrackerNET->GetIncomingData()) != NULL)
	{
		// handle message
		ReceivedData(recv);

		// free the message
		g_pTrackerNET->ReleaseMessage(recv);
	}

	g_pTrackerNET->RunFrame();

	// check for timeouts
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		server_t &server = m_Servers[i];
		CheckOnServer(server);
	}

	// check to see if we need to do any refreshes
	double time = g_pThreads->GetTime();
	if (time > m_flRebroadcastTime)
	{
		BroadcastForServers();

		// force the database info to update
		m_bHasDBInfo = false;
		
		// only do one request per frame
		return;	
	}

	// look to see if any servers need to be updated
	for (i = 0; i < m_Servers.Size(); i++)
	{
		server_t &server = m_Servers[i];
		if (time > server.refreshTime)
		{
			RefreshServer(server);

			// only do one request per frame
			break;		
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a server to the list to monitor, making sure it's not a duplicate
// Input  : &addr - 
//-----------------------------------------------------------------------------
server_t &CServerList::FindServer(CNetAddress &addr)
{
	// check to see if server is already in the list
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		server_t &server = m_Servers[i];
		
		if (server.addr == addr && server.addr.Port() == addr.Port())
		{
			// record the time we received from this server
			server.lastReceive = g_pThreads->GetTime();
			return server;
		}
	}

	// add server to end of list
	server_t srv;
	memset(&srv, 0, sizeof(srv));
	srv.addr = addr;
	strcpy(srv.name, addr.ToStaticString());
	srv.state = 0;
	srv.refreshTime = 0.0;
	srv.state = SERVER_UNKNOWN;

	// try to resolve the IP
	int ip = srv.addr.IP();
	hostent *host = ::gethostbyaddr((const char *)&ip, 4, IPPROTO_UDP);
	if (host)
	{
		strcpy(srv.name, host->h_name);
	}

	// record the time we received from this server
	srv.lastReceive = g_pThreads->GetTime();
	srv.lastSend = srv.lastReceive;

	int id = m_Servers.AddToTail(srv);

	return m_Servers[id];
}

//-----------------------------------------------------------------------------
// Purpose: Handles incoming networking traffic
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CServerList::ReceivedData(IReceiveMessage *msg)
{
	int msgName = msg->GetMsgID();
	bool refreshUI = false;

	if (msgName == TSV_MONITORINFO)
	{
		// add the server to the list
		ReceivedMsg_MonitorInfo(msg);
		refreshUI = true;
	}
	else if (msgName == TSV_SRVINFO)
	{
		ReceivedMsg_SrvInfo(msg);
		refreshUI = true;
	}

	if (refreshUI)
	{
		m_pNotifyPanel->PostMessage(m_pNotifyPanel, new vgui::KeyValues("RefreshServers"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sends out a broadcast packet looking for a server
//-----------------------------------------------------------------------------
void CServerList::BroadcastForServers()
{
	ISendMessage *msg = g_pTrackerNET->CreateMessage(TSV_REQUESTINFO);
	CNetAddress addr;
	addr.SetPort(1201);
	msg->SetNetAddress(addr);
	msg->SetEncrypted(false);
	g_pTrackerNET->SendMessage(msg, NET_BROADCAST);

	msg = g_pTrackerNET->CreateMessage(TSV_REQUESTINFO);
	addr = g_pTrackerNET->GetNetAddress("205.158.143.202:1201");
	msg->SetNetAddress(addr);
	msg->SetEncrypted(false);
	g_pTrackerNET->SendMessage(msg, NET_UNRELIABLE);
	
	msg = g_pTrackerNET->CreateMessage(TSV_REQUESTINFO);
	addr = g_pTrackerNET->GetNetAddress("205.158.143.200:1201");
	msg->SetNetAddress(addr);
	msg->SetEncrypted(false);
	g_pTrackerNET->SendMessage(msg, NET_UNRELIABLE);

	m_flRebroadcastTime = g_pThreads->GetTime() + BROADCAST_INTERVAL;
}

//-----------------------------------------------------------------------------
// Purpose: Requests basic information from a server
// Input  : &server - 
//-----------------------------------------------------------------------------
void CServerList::RefreshServer(server_t &server)
{
	server.lastSend = g_pThreads->GetTime();

	// send off request info packet
	ISendMessage *msg = g_pTrackerNET->CreateMessage(TSV_REQUESTINFO);
	msg->SetNetAddress(server.addr);
	msg->SetEncrypted(false);
	g_pTrackerNET->SendMessage(msg, NET_RELIABLE);

	server.refreshTime = g_pThreads->GetTime() + REFRESH_INTERVAL;
	server.lastSend = g_pThreads->GetTime();
}

//-----------------------------------------------------------------------------
// Purpose: Checks for server timeouts
// Input  : &server - 
//-----------------------------------------------------------------------------
void CServerList::CheckOnServer(server_t &server)
{
	// if server is active, check for timeout
	if (server.state == SERVER_ACTIVE || server.state == SERVER_SHUTTINGDOWN || server.state == SERVER_UNKNOWN)
	{
		if ((server.lastSend - server.lastReceive) > TIMEOUT_INTERVAL)
		{
			// server has timed out - progress into timeout state
			switch (server.state)
			{
			case SERVER_SHUTTINGDOWN:
				server.state = SERVER_DOWN;
				break;

			case SERVER_UNKNOWN:
			case SERVER_ACTIVE:
			default:
				server.state = SERVER_NOTRESPONDING;
				break;
			};

			// refresh the UI
			m_pNotifyPanel->PostMessage(m_pNotifyPanel, new vgui::KeyValues("RefreshServers"));
		}
	}

	// if server is down, it will automatically be marked as active again when a srvinfo packet is received
}

//-----------------------------------------------------------------------------
// Purpose: Handles a server info message, which is a reply from a RequestInfo msg
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CServerList::ReceivedMsg_MonitorInfo(IReceiveMessage *msg)
{
	server_t &server = FindServer(msg->NetAddress());

	// parse out information about server
	msg->ReadInt("users", server.users);
	msg->ReadInt("maxusers", server.maxUsers);
	msg->ReadInt("state", server.state);
	msg->ReadInt("fps", server.fps);
	msg->ReadInt("netbufs", server.networkBuffers);
	msg->ReadInt("peaknetbufs", server.peakNetworkBuffers);
	msg->ReadInt("dboutbufs", server.dbOutBufs);
	msg->ReadInt("dbinbufs", server.dbInBufs);
	msg->ReadInt("primary", server.primary);
	msg->ReadInt("rbps", server.bytesReceivedPerSecond);
	msg->ReadInt("sbps", server.bytesSentPerSecond);

	// calculate some info
	if ((server.fps > 0 && server.fps < 500) || server.dbInBufs > 100 || server.dbOutBufs > 100 || server.networkBuffers > 100)
	{
		server.underHeavyLoad = true;
	}
	else
	{
		server.underHeavyLoad = false;
	}

	// if we don't have the sqldb info yet, ask for it
	if (!m_bHasDBInfo)
	{
		g_pTrackerNET->SendMessage(g_pTrackerNET->CreateReply(TSV_REQSRVINFO, msg), NET_RELIABLE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Information about the server network
//-----------------------------------------------------------------------------
void CServerList::ReceivedMsg_SrvInfo(IReceiveMessage *msg)
{
	// only read if we don't already have this info
	if (m_bHasDBInfo)
		return;

	char sqldb[32];
	if (msg->ReadString("sqldb", sqldb, sizeof(sqldb)))
	{
		m_bHasDBInfo = true;
		
		// access the database
	}
}

