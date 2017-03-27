//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#define PROTECTED_THINGS_DISABLE

#include <stdlib.h>  // atoi
#include "IServerRefreshResponse.h"
#include "ServerList.h"
#include "ServerMsgHandlerDetails.h"
#include "Socket.h"
#include "proto_oob.h"

// for debugging
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Controls.h>

typedef enum
{
	NONE = 0,
	INFO_REQUESTED,
	INFO_RECEIVED
} QUERYSTATUS;

extern void v_strncpy(char *dest, const char *src, int bufsize);
#define min(a,b)    (((a) < (b)) ? (a) : (b))

//-----------------------------------------------------------------------------
// Purpose: Comparison function used in query redblack tree
//-----------------------------------------------------------------------------
bool QueryLessFunc( const query_t &item1, const query_t &item2 )
{
	// compare port then ip
	if (item1.addr.port < item2.addr.port)
		return true;
	else if (item1.addr.port > item2.addr.port)
		return false;

	int ip1 = *(int *)&item1.addr.ip;
	int ip2 = *(int *)&item2.addr.ip;

	return ip1 < ip2;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServerList::CServerList(IServerRefreshResponse *target) : m_Queries(0, MAX_QUERY_SOCKETS, QueryLessFunc)
{
	m_pResponseTarget = target;
	m_iUpdateSerialNumber = 1;

	// calculate max sockets based on users' rate
	char speedBuf[32];
	int internetSpeed;
	if (!vgui::system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Valve\\Steam\\Rate", speedBuf, sizeof(speedBuf)-1))
	{
		// default to DSL speed if no reg key found (an unlikely occurance)
		strcpy(speedBuf, "7500");
	}
	internetSpeed = atoi(speedBuf);
	int maxSockets = (MAX_QUERY_SOCKETS * internetSpeed) / 10000;
	if (internetSpeed < 6000)
	{
		// reduce the number of active queries again for slow internet speeds
		maxSockets /= 2;
	}
	m_nMaxActive = maxSockets;

	m_nRampUpSpeed = 1;
	m_bQuerying = false;
	m_nMaxRampUp = 1;

	m_nInvalidServers = 0;
	m_nRefreshedServers = 0;

	// setup sockets
	int bytecode = S2A_INFO_DETAILED;
	m_pQuery = new CSocket("internet dialog query", -1);
	m_pQuery->AddMessageHandler(new CServerMsgHandlerDetails(this, CMsgHandler::MSGHANDLER_ALL, &bytecode));
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServerList::~CServerList()
{
	delete m_pQuery;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerList::RunFrame()
{
	if (m_pQuery)
	{
		m_pQuery->Frame();
	}

	QueryFrame();
}

//-----------------------------------------------------------------------------
// Purpose: gets a server from the list by id, range [0, ServerCount)
//-----------------------------------------------------------------------------
serveritem_t &CServerList::GetServer(unsigned int serverID)
{
	if (m_Servers.IsValidIndex(serverID))
	{
		return m_Servers[serverID];
	}

	// return a dummy
	static serveritem_t dummyServer;
	memset(&dummyServer, 0, sizeof(dummyServer));
	return dummyServer;
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of servers
//-----------------------------------------------------------------------------
int CServerList::ServerCount()
{
	return m_Servers.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of servers not yet pinged
//-----------------------------------------------------------------------------
int CServerList::RefreshListRemaining()
{
	return m_RefreshList.Count();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the server list is still in the process of talking to servers
//-----------------------------------------------------------------------------
bool CServerList::IsRefreshing()
{
	return m_bQuerying;
}

//-----------------------------------------------------------------------------
// Purpose: adds a new server to the list
//-----------------------------------------------------------------------------
unsigned int CServerList::AddNewServer(serveritem_t &server)
{
	// make sure the server isn't already here
	// this is a bug in the master server, sending us servers we already have
	/*
	for (int i = 0; i < m_Servers.Count(); i++)
	{
		if (m_Servers[i].ip[0] == server.ip[0]
			&& m_Servers[i].ip[1] == server.ip[1]
			&& m_Servers[i].ip[2] == server.ip[2]
			&& m_Servers[i].ip[3] == server.ip[3]
			&& m_Servers[i].port == server.port)
		{
			// assert(!("ADDING DUPLICATE SERVER"));
			return i;
		}
	}
	*/

	unsigned int serverID = m_Servers.AddToTail(server);
	m_Servers[serverID].serverID = serverID;
	return serverID;
}

//-----------------------------------------------------------------------------
// Purpose: Clears all servers from the list
//-----------------------------------------------------------------------------
void CServerList::Clear()
{
	StopRefresh();

	m_Servers.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: stops all refreshing
//-----------------------------------------------------------------------------
void CServerList::StopRefresh()
{
	// Reset query context
	m_Queries.RemoveAll();

	// reset server received states
	int count = ServerCount();
	for (int i = 0; i < count; i++)
	{
		m_Servers[i].received = 0;
	}

	m_RefreshList.RemoveAll();

	// up the serial number so previous results don't interfere
	m_iUpdateSerialNumber++;

	m_nInvalidServers = 0;
	m_nRefreshedServers = 0;
	m_bQuerying = false;
	m_nMaxRampUp = m_nRampUpSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: marks a server to be refreshed
//-----------------------------------------------------------------------------
void CServerList::AddServerToRefreshList(unsigned int serverID)
{
	if (!m_Servers.IsValidIndex(serverID))
		return;

	serveritem_t &server = m_Servers[serverID];
	server.received = NONE;

	m_RefreshList.AddToTail(serverID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerList::StartRefresh()
{
	if (m_RefreshList.Count() > 0)
	{
		m_bQuerying = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles a refresh response from a server
//-----------------------------------------------------------------------------
void CServerList::UpdateServer(netadr_t *adr, bool proxy, const char *serverName, const char *map, const char *gamedir, const char *gameDescription, int players, int maxPlayers, float recvTime, bool password)
{
	// find the reply in the query list
	query_t finder;
	finder.addr = *adr;
	int queryIndex = m_Queries.Find(finder);
	if (queryIndex == m_Queries.InvalidIndex())
		return;

	query_t &query = m_Queries[queryIndex];
	int serverIndex = query.serverID;
	float sendTime = query.sendTime;

	// remove the query from the list
	m_Queries.RemoveAt(queryIndex);

	// update the server
	serveritem_t &server = m_Servers[serverIndex];
	if (server.received != INFO_RECEIVED)
	{
		m_nRefreshedServers++;
		server.received = INFO_RECEIVED;
	}

	server.hadSuccessfulResponse = true;

	// copy in data necessary for filters
	v_strncpy(server.gameDir, gamedir, sizeof(server.gameDir) - 1);
	v_strncpy(server.map, map, sizeof(server.map) - 1);
	v_strncpy(server.name, serverName, sizeof(server.name) - 1);
	v_strncpy(server.gameDescription, gameDescription, sizeof(server.gameDescription) - 1);
	server.players = players;
	server.maxPlayers = maxPlayers;
	server.proxy = proxy;
	server.password = password;

	int ping = (int)((recvTime - sendTime) * 1000);

	if (ping > 3000 || ping < 0)
	{
		// make sure ping is valid
		ping = 1200;
	}

	// add to ping times list 
	server.pings[0] = server.pings[1];
	server.pings[1] = server.pings[2];
	server.pings[2] = ping;

	// calculate ping
	ping = CalculateAveragePing(server);

	// make sure the ping changes each time so the user can see the server has updated
	if (server.ping == ping)
	{
		ping--;
	}
	server.ping = ping;
	server.received = INFO_RECEIVED;

	// notify the UI of the new server info
	m_pResponseTarget->ServerResponded(server);
}

//-----------------------------------------------------------------------------
// Purpose: recalculates a servers ping, from the last few ping times
//-----------------------------------------------------------------------------
int CServerList::CalculateAveragePing(serveritem_t &server)
{
	if (server.pings[0])
	{
		// three pings, throw away any the most extreme and average the other two
		int middlePing = 0, lowPing = 1, highPing = 2;
		if (server.pings[0] < server.pings[1])
		{
			if (server.pings[0] > server.pings[2])
			{
				lowPing = 2;
				middlePing = 0;
				highPing = 1;
			}
			else if (server.pings[1] < server.pings[2])
			{
				lowPing = 0;
				middlePing = 1;
				highPing = 2;
			}
			else
			{
				lowPing = 0;
				middlePing = 2;
				highPing = 1;
			}
		}
		else
		{
			if (server.pings[1] > server.pings[2])
			{
				lowPing = 2;
				middlePing = 1;
				highPing = 0;
			}
			else if (server.pings[0] < server.pings[2])
			{
				lowPing = 1;
				middlePing = 0;
				highPing = 2;
			}
			else
			{
				lowPing = 1;
				middlePing = 2;
				highPing = 0;
			}
		}

		// we have the middle ping, see which it's closest to
		if ((server.pings[middlePing] - server.pings[lowPing]) < (server.pings[highPing] - server.pings[middlePing]))
		{
			return (server.pings[middlePing] + server.pings[lowPing]) / 2;
		}
		else
		{
			return (server.pings[middlePing] + server.pings[highPing]) / 2;
		}
	}
	else if (server.pings[1])
	{
		// two pings received, average them
		return (server.pings[1] + server.pings[2]) / 2;
	}
	else
	{
		// only one ping received so far, use that
		return server.pings[2];
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame to check queries
//-----------------------------------------------------------------------------
void CServerList::QueryFrame()
{
	if (!m_bQuerying)
		return;

	float curtime = CSocket::GetClock();

	// walk the query list, looking for any server timeouts
	unsigned short idx = m_Queries.FirstInorder();
	while (m_Queries.IsValidIndex(idx))
	{
		query_t &query = m_Queries[idx];
		if ((curtime - query.sendTime) > 1.2f)
		{
			// server has timed out
			serveritem_t &item = m_Servers[query.serverID];

			// mark the server
			item.pings[0] = item.pings[1];
			item.pings[1] = item.pings[2];
			item.pings[2] = 1200;
			item.ping = CalculateAveragePing(item);
			if (!item.hadSuccessfulResponse)
			{
				// remove the server if it has never responded before
				item.doNotRefresh = true;
				m_nInvalidServers++;
			}
			// respond to the game list notifying of the lack of response
			m_pResponseTarget->ServerFailedToRespond(item);
			item.received = false;

			// get the next server now, since we're about to delete it from query list
			unsigned short nextidx = m_Queries.NextInorder(idx);
			
			// delete the query
			m_Queries.RemoveAt(idx);

			// move to next item
			idx = nextidx;
		}
		else
		{
			// still waiting for server result
			idx = m_Queries.NextInorder(idx);
		}
	}

	// increment the number of sockets to use
	m_nMaxRampUp = min(m_nMaxActive, m_nMaxRampUp + m_nRampUpSpeed);

	// see if we should send more queries
	while (m_RefreshList.Count() > 0 && (int)m_Queries.Count() < m_nMaxRampUp)
	{
		// get the first item from the list to refresh
		int currentServer = m_RefreshList[0];
		if (!m_Servers.IsValidIndex(currentServer))
			break;
		serveritem_t &item = m_Servers[currentServer];

		item.time = curtime;
		
		QueryServer(m_pQuery, currentServer);

		// remove the server from the refresh list
		m_RefreshList.Remove((int)0);
	}

	// Done querying?
	if (m_Queries.Count() < 1)
	{
		m_bQuerying = false;
		m_pResponseTarget->RefreshComplete();

		// up the serial number, so that we ignore any late results
		m_iUpdateSerialNumber++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: sends a status query packet to a single server
//-----------------------------------------------------------------------------
void CServerList::QueryServer(CSocket *socket, unsigned int serverID)
{
	CMsgBuffer *buffer = socket->GetSendBuffer();
	Assert( buffer );
	if ( !buffer )
		return;


	netadr_t adr;
	serveritem_t &server = m_Servers[serverID];

	adr.ip[0] = server.ip[0];
	adr.ip[1] = server.ip[1];
	adr.ip[2] = server.ip[2];
	adr.ip[3] = server.ip[3];
	adr.port = (server.port & 0xff) << 8 | (server.port & 0xff00) >> 8;
	adr.type = NA_IP;

	// add into query list
	query_t query;
	query.addr = adr;
	query.serverID = serverID;

	int duplicateIndex = m_Queries.Find(query);
	if (m_Queries.IsValidIndex(duplicateIndex))
	{
		// we're already trying to ping the server... break out
		// kill the server
		server.hadSuccessfulResponse = false;
		server.doNotRefresh = true;
		return;
	}

	// Set state
	server.received = (int)INFO_REQUESTED;

	// Create query message
	buffer->Clear();
	// Write control sequence
	buffer->WriteLong(0xffffffff);
	// Write query string
	buffer->WriteString("infostring");

	// Sendmessage
	socket->SendMessage( &adr );

	// insert the query into the list and set the time
	int idx = m_Queries.Insert(query);
	m_Queries[idx].sendTime = CSocket::GetClock();
}



