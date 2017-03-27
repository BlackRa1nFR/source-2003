//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "../TrackerNET/TrackerNET_Interface.h"
#include "../TrackerNET/NetAddress.h"
#include "DebugConsole_Interface.h"
#include "../TrackerNET/Threads.h"

#include "SessionManager.h"
#include "TopologyManager.h"
#include "UserSession.h"
#include "TrackerDatabaseManager.h"
#include "../public/ITrackerDistroDatabase.h"
#include "../public/ITrackerUserDatabase.h"
#include "TrackerProtocol.h"
#include "UtlMsgBuffer.h"

#include "DebugTimer.h"

extern IDebugConsole *g_pConsole;
CTopologyManager *g_pTopologyManager;

#define ARRAYSIZE(p)	(sizeof(p)/sizeof(p[0]))

#define QUERY_TIMEOUT (8.0f)

// network message dispatch table
typedef void (CTopologyManager::*funcPtr)( IReceiveMessage * );
struct MsgDispatch_t
{
	int msgName;
	funcPtr msgFunc;
};

static MsgDispatch_t g_UserMsgDispatch[] =
{
	{ TSV_WHOISPRIMARY,	CTopologyManager::ReceivedMsg_WhoIsPrimary },
	{ TSV_PRIMARYSRV,	CTopologyManager::ReceivedMsg_PrimarySrv },
	{ TSV_REQUESTINFO,	CTopologyManager::ReceivedMsg_RequestInfo },
	{ TSV_SRVINFO,		CTopologyManager::ReceivedMsg_SrvInfo },
	{ TSV_REQSRVINFO,	CTopologyManager::ReceivedMsg_ReqSrvInfo },
	{ TSV_SERVERPING,   CTopologyManager::ReceivedMsg_ServerPing },
	{ TSV_LOCKUSERRANGE, CTopologyManager::ReceivedMsg_LockUserRange },
	{ TSV_UNLOCKUSERRANGE, CTopologyManager::ReceivedMsg_UnlockUserRange },
	{ TSV_REDIRECTTOUSER, CTopologyManager::ReceivedMsg_RedirectToUser },
	{ TSV_FORCEDISCONNECTUSER, CTopologyManager::ReceivedMsg_ForceDisconnectUser },
	{ TSV_USERCHECKMESSAGES, CTopologyManager::ReceivedMsg_UserCheckMessages },
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTopologyManager::CTopologyManager()
{
	m_dbsName[0] = 0;
	m_bShuttingDown = false;
	m_bResendServerInfo = false;
	m_flLastFrameTime = 0.0;
	m_flRepingTime = 0.0f;
	m_bPrimary = false;
	m_bFindingPrimaryServer = true;
	m_flStateThinkTime = 0.0f;
	m_pStateFunc = NULL;
	m_iMoveRangeLower = 0;
	m_iMoveRangeUpper = 0;
	g_pTopologyManager = this;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTopologyManager::~CTopologyManager()
{
	g_pTopologyManager = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTopologyManager::Initialize(unsigned int serverID, CreateInterfaceFn netFactory, CNetAddress server)
{
	m_iServerID = serverID;

	m_pLocalNET = (ITrackerNET *)netFactory(TRACKERNET_INTERFACE_VERSION, NULL);
	if (!m_pLocalNET->Initialize(1201, 1201))
	{
		g_pConsole->Print(10, "Could not reserve port 1201\n");
		return false;
	}

	// fix the port to send to at 1201
	server.SetPort(1201);
	m_pLocalNET->SetWindowsEvent(sessionmanager()->GetWindowsEvent());

	if (server.IP() != 0)
	{
		// connect to the specified address
		g_pConsole->Print(3, "Sending cluster info request to server '%s'...\n", server.ToStaticString());
		
		ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_WHOISPRIMARY);
		msg->SetNetAddress(server);
		msg->SetEncrypted(false);
		msg->WriteUInt("serverID", m_iServerID);
		m_pLocalNET->SendMessage(msg, NET_RELIABLE);
	}
	else
	{
		// broadcast to all local servers asking for the server
		g_pConsole->Print(3, "Searching for primary tracker server...\n");

		ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_WHOISPRIMARY);
		msg->SetNetAddress(server);
		msg->SetEncrypted(false);
		msg->WriteUInt("serverID", m_iServerID);
		m_pLocalNET->SendMessage(msg, NET_BROADCAST);
	}

	m_bFindingPrimaryServer = true;

	// keep running frames until we time out, or we get the primary back
	float timeout = sessionmanager()->GetCurrentTime() + 6.2f;
	float primaryTime = sessionmanager()->GetCurrentTime() + 3.0f;

	while (m_bFindingPrimaryServer)
	{
		m_pLocalNET->GetThreadAPI()->Sleep(5);

		if (sessionmanager()->GetCurrentTime() >= timeout)
			break;

		if (primaryTime > 0 && sessionmanager()->GetCurrentTime() >= primaryTime)
		{
			// if we don't no about an sqldb, don't try and be primary
			if (!m_dbsName[0])
			{
				primaryTime = 0;
				continue;
			}

			// check to see if we should be the primary server
			unsigned int bestServer = (unsigned int)-1;
			unsigned int bestID = m_iServerID;
			for (int i = 0; i < m_Servers.Size(); i++)
			{
				if (m_Servers[i].serverID <= bestID)
				{
					bestServer = i;
					bestID = m_Servers[i].serverID;
				}
			}

			if (bestID == m_iServerID)
			{
				// the best server is us, engage!
				BecomePrimaryServer();
			}
			else
			{
				// we should not be the primary, there is a better candidate
			}

			primaryTime = 0;
		}

		RunFrame(0);
	}

	if (m_bFindingPrimaryServer && !m_bPrimary)
	{
		g_pConsole->Print(10, "Could not find primary server.\nIf this is to be the primary server, Run with -sqldb <db name>\n");
		return false;
	}

	// Start the frame timer
	Timer_Start();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: begins us shutting down server
//-----------------------------------------------------------------------------
void CTopologyManager::StartShutdown()
{
	m_bShuttingDown = true;

	// send info to all our watchers to let them know the server is going down
	for (int i = 0; i < m_Watchers.Size(); i++)
	{
		SendServerInfo(m_Watchers[i]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Runs a frame
//			returns true if there is any activity, false otherwise
//-----------------------------------------------------------------------------
bool CTopologyManager::RunFrame(int sleepFrame)
{
	bool activity = false;

	IReceiveMessage *recv;
	while ((recv = m_pLocalNET->GetIncomingData()) != NULL)
	{
		activity = true;

		// handle message
		ReceivedData(recv);

		// free the message
		m_pLocalNET->ReleaseMessage(recv);
	}

	// check for failed messages
	while ((recv = m_pLocalNET->GetFailedMessage()) != NULL)
	{
		activity = true;

		// use bad message handler, target server must have gone down
		OnMessageFailed(recv);

		m_pLocalNET->ReleaseMessage(recv);
	}

	m_pLocalNET->RunFrame();

	// record how long that last frame took
	m_flLastFrameTime = Timer_End();

	if (sleepFrame > 0)
	{
		// calculate frame rate
		m_iFPS = (int)(1000.0 / m_flLastFrameTime);
	}
	else
	{
		// we're sleeping, frame rate is a non-issue
		m_iFPS = 0;
	}

	// restart the timer
	Timer_Start();

	// check for reping
	if (sessionmanager()->GetCurrentTime() > m_flRepingTime)
	{
		// reping everyone
		// we will get a fail message if the packet could not be delivered, indicating that the server has gone down
		for (int i = 0; i < m_Servers.Size(); i++)
		{
			if (m_Servers[i].serverID == m_iServerID)
				continue;

			if (m_bResendServerInfo && m_bPrimary)
			{
				// send out server info to everyone
				SendTopologyInfo(m_Servers[i].addr);
			}
			else
			{
				// send a ping message to server to make sure they know about us
				ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_SERVERPING);
				msg->SetNetAddress(m_Servers[i].addr);
				msg->SetEncrypted(false);
				msg->WriteUInt("serverID", m_iServerID);
				m_pLocalNET->SendMessage(msg, NET_RELIABLE);
			}
		}
		
		m_flRepingTime = sessionmanager()->GetCurrentTime() + 10.0f;
		m_bResendServerInfo = false;
	}

	// call the state function if necessary
	if (m_pStateFunc && m_flStateThinkTime < sessionmanager()->GetCurrentTime())
	{
		if ((this->*m_pStateFunc)())
		{
			activity = true;
		}
	}

	return activity;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CTopologyManager::GetSQLDB()
{
	return m_dbsName;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
unsigned int CTopologyManager::GetServerID()
{
	return m_iServerID;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CTopologyManager::SetSQLDB(const char *dbname)
{
	strcpy(m_dbsName, dbname);
}

//-----------------------------------------------------------------------------
// Purpose: routes the message to the specified user, 
//			buffering and rerouting through other server if necessary
//-----------------------------------------------------------------------------
void CTopologyManager::SendNetworkMessageToUser(unsigned int userID, unsigned int sessionID, unsigned int serverID, int msgID, void const *data, int dataSize)
{
//	g_pConsole->Print(6, "Sending msg (%d) to user (%d, %d) on server (%d)\n", msgID, userID, sessionID, serverID);

	if (serverID == m_iServerID)
	{
		// user is logged on to this server, so pass off to SessionManager to handle
		sessionmanager()->SendMessageToUser(userID, sessionID, msgID, data, dataSize);
	}
	else
	{
		// look for the server this user is on, and route the message through that server
		CNetAddress addr;
		addr.SetIP(0);
		for (int i = 0; i < m_Servers.Size(); i++)
		{
			if (m_Servers[i].serverID == serverID)
			{
				addr = m_Servers[i].addr;
				break;
			}
		}
		if (!addr.IP())
		{
			g_pConsole->Print(4, "**** Watcher (%d) has bad serverID (%d)\n", userID, serverID);
			return;
		}

		// send the message, routed through the users' server
		ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_REDIRECTTOUSER);
		msg->SetNetAddress(addr);
		msg->SetEncrypted(false);

		// write in all the routing info
		msg->WriteInt("rID", msgID);
		msg->WriteUInt("rUserID", userID);
		msg->WriteUInt("rSessionID", sessionID);
		msg->WriteBlob("rData", data, dataSize);

		m_pLocalNET->SendMessage(msg, NET_RELIABLE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: finds the server the user is connected to and forces the user to 
//			be disconnected, and ensures the are logged out of sqldb
//-----------------------------------------------------------------------------
void CTopologyManager::ForceDisconnectUser(unsigned int userID, unsigned int sessionID, unsigned int serverID)
{
	// check to see if the user is on this server
	if (serverID == m_iServerID || serverID == 0)
	{
		sessionmanager()->ForceDisconnectUser(userID);
		return;
	}

	// find which server the user is connected to
	CNetAddress addr;
	addr.SetIP(0);
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		if (m_Servers[i].serverID == serverID)
		{
			addr = m_Servers[i].addr;
			break;
		}
	}
	if (!addr.IP())
	{
		// couldn't find the server; just force them off
		sessionmanager()->ForceDisconnectUser(userID);
		return;
	}

	// send the message, routed through the users' server
	ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_FORCEDISCONNECTUSER);
	msg->SetNetAddress(addr);
	msg->SetEncrypted(false);

	// write in all the routing info
	msg->WriteUInt("userID", userID);
	msg->WriteUInt("sessionID", sessionID);

	m_pLocalNET->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: tells the user, wherever they are on the network, to check their messages
//-----------------------------------------------------------------------------
void CTopologyManager::UserCheckMessages(unsigned int userID, unsigned int sessionID, unsigned int serverID)
{
	// check to see if the user is on this server
	if (serverID == m_iServerID || serverID == 0)
	{
		sessionmanager()->UserCheckMessages(userID);
		return;
	}

	// find which server the user is connected to
	CNetAddress addr;
	addr.SetIP(0);
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		if (m_Servers[i].serverID == serverID)
		{
			addr = m_Servers[i].addr;
			break;
		}
	}
	if (!addr.IP())
	{
		// couldn't find the server; just ignore
		return;
	}

	// send the message, routed through the users' server
	ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_USERCHECKMESSAGES);
	msg->SetNetAddress(addr);
	msg->SetEncrypted(false);

	// write in all the routing info
	msg->WriteUInt("userID", userID);
	msg->WriteUInt("sessionID", sessionID);

	m_pLocalNET->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Handles receiving a network message
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedData(IReceiveMessage *dataBlock)
{
	// simple dispatch
	// loop through the array and find the right message
	//!! use a hash table or the like eventually
	int arraySize = ARRAYSIZE(g_UserMsgDispatch);
	int msgName = dataBlock->GetMsgID();
	for (int i = 0; i < arraySize; i++)
	{
		if (msgName == g_UserMsgDispatch[i].msgName)
		{
			(this->*g_UserMsgDispatch[i].msgFunc)(dataBlock);
			return;
		}
	}

	// invalid message
	g_pConsole->Print(2, "Unhandled message '%d'\n", msgName);
}

//-----------------------------------------------------------------------------
// Purpose: Handles another server asking for the primary
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_WhoIsPrimary(IReceiveMessage *dataBlock)
{
	// read in the serverID
	unsigned int serverID;
	dataBlock->ReadUInt("serverID", serverID);
	
	// add the server to our known list
	server_t &server = FindServer(dataBlock->NetAddress(), serverID);
	
	// don't reply to ourselves
	if (server.serverID != m_iServerID)
	{
		// send back our details to make sure the other server knows about us
		ISendMessage *msg = m_pLocalNET->CreateReply(TSV_SERVERPING, dataBlock);
		msg->WriteUInt("serverID", m_iServerID);
		m_pLocalNET->SendMessage(msg, NET_RELIABLE);
	}

	if (!m_bFindingPrimaryServer)
	{
		// we know who the primary server is, so let the other server know

		// send back the details
		ISendMessage *msg = m_pLocalNET->CreateReply(TSV_PRIMARYSRV, dataBlock);
		
		msg->WriteUInt("PrimaryIP", m_PrimaryServerAddress.IP());
		msg->WriteUInt("PrimaryPort", m_PrimaryServerAddress.Port());
		msg->WriteString("SQLDB", m_dbsName);

		m_pLocalNET->SendMessage(msg, NET_RELIABLE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: A ping from another server, to establish contact
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_ServerPing(IReceiveMessage *dataBlock)
{
	unsigned int serverID;

	// read in the server detail
	dataBlock->ReadUInt("serverID", serverID);

	FindServer(dataBlock->NetAddress(), serverID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_PrimarySrv(IReceiveMessage *dataBlock)
{
	if (m_bPrimary)
		return;		// we are the primary server

	m_bFindingPrimaryServer = false;

	// we got a response about the primary server, extract details
	unsigned int IP;
	unsigned int Port;
	char dbname[128];

	dataBlock->ReadUInt("PrimaryIP", IP);
	dataBlock->ReadUInt("PrimaryPort", Port);
	dataBlock->ReadString("SQLDB", dbname, sizeof(dbname));

	if (IP == 0)
	{
		// primary server didn't know it's own IP, fix up
		IP = dataBlock->NetAddress().IP();
		Port = dataBlock->NetAddress().Port();
	}

	if (Port == 0)
	{
		Port = 1201;
	}

	if (m_PrimaryServerAddress.IP() == (unsigned int)IP && m_PrimaryServerAddress.Port() == Port)
		return;	// we are already using this as our primary server, ignore

	if (dbname[0])
	{
		SetSQLDB(dbname);
	}

	m_PrimaryServerAddress.SetIP(IP);
	m_PrimaryServerAddress.SetPort(Port);

	g_pConsole->Print(4, "Using primary server %s\n", m_PrimaryServerAddress.ToStaticString());

	// send whois message to server to make sure they know about us
	ISendMessage *msg2 = m_pLocalNET->CreateMessage(TSV_SERVERPING);
	msg2->SetNetAddress(m_PrimaryServerAddress);
	msg2->SetEncrypted(false);
	msg2->WriteUInt("serverID", m_iServerID);
	m_pLocalNET->SendMessage(msg2, NET_RELIABLE);

	// get the topology info from the server
	ISendMessage *reply = m_pLocalNET->CreateMessage(TSV_REQSRVINFO);
	reply->SetNetAddress(m_PrimaryServerAddress);
	reply->WriteUInt("serverID", m_iServerID);
	m_pLocalNET->SendMessage(reply, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: sends a set of info about this server to the requester (servermonitor most likely)
//-----------------------------------------------------------------------------
void CTopologyManager::SendServerInfo(CNetAddress &addr)
{
	// send the watcher back all the information about ourselves we can muster
	ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_MONITORINFO);
	msg->SetEncrypted(true);
	msg->SetNetAddress(addr);

	// get the data
	int users = sessionmanager()->ActiveUserCount();
	int maxUsers = sessionmanager()->MaxUserCount();
	int networkBuffers = net()->BuffersInQueue();
	int peakNetworkBuffers = net()->PeakBuffersInQueue();
	int dataSentPerSecond = net()->BytesSentPerSecond();
	int dataReceivedPerSecond = net()->BytesReceivedPerSecond();

	int dbOutBuffers = 0;
	int dbInBuffers = 0;
	if (g_pDataManager)
	{
		dbOutBuffers = g_pDataManager->TrackerUser(0)->QueriesInOutQueue();
		dbInBuffers = g_pDataManager->TrackerUser(0)->QueriesInFinishedQueue();
	}

	int state = 1;
	if (m_bShuttingDown)
	{
		state = 4;	// indicates we're shutting down
	}

	int iPrimary = (int)m_bPrimary;

	// write out the message
	msg->WriteInt("users", users);
	msg->WriteInt("maxusers", maxUsers);
	msg->WriteInt("state", state);
	msg->WriteInt("fps", m_iFPS);
	msg->WriteInt("netbufs", networkBuffers);
	msg->WriteInt("peaknetbufs", peakNetworkBuffers);
	msg->WriteInt("dboutbufs", dbOutBuffers);
	msg->WriteInt("dbinbufs", dbInBuffers);
	msg->WriteInt("primary", iPrimary);
	msg->WriteInt("rbps", dataReceivedPerSecond);
	msg->WriteInt("sbps", dataSentPerSecond);

	m_pLocalNET->SendMessage(msg, NET_RELIABLE);

}
//-----------------------------------------------------------------------------
// Purpose: Message from a watcher, probably the ServerMonitor
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_RequestInfo(IReceiveMessage *dataBlock)
{
	// add the sender to the watchers list, so we can update them if our status changes
	CNetAddress &addr = dataBlock->NetAddress();

	// check to see if the watcher is already in the list
	bool bFound = false;
	for (int i = 0; i < m_Watchers.Size(); i++)
	{
		if (m_Watchers[i] == addr)
		{
			bFound = true;
		}
	}
	if (!bFound)
	{
		m_Watchers.AddToTail(addr);
	}

	// send the server info
	SendServerInfo(addr);
}

#pragma pack(push)
#pragma pack(4)
struct serv_t
{
	int ip;
	int port;
	unsigned int serverID;
};
#pragma pack(pop)

//-----------------------------------------------------------------------------
// Purpose: Request from a Client to the primary server detailing the network of servers
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_ReqSrvInfo(IReceiveMessage *dataBlock)
{
	if (m_bPrimary)
	{
		// send a message to that server
		SendTopologyInfo(dataBlock->NetAddress());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Response from the master detailing network topology
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_SrvInfo(IReceiveMessage *dataBlock)
{
	if (m_bPrimary)
	{
		if (!(dataBlock->NetAddress() == m_PrimaryServerAddress))
		{
			// conflict
			g_pConsole->Print(8, "Primary server conflict with %s\n", dataBlock->NetAddress().ToStaticString());

			// check to see if we should yield to the other server
			unsigned int serverID;
			dataBlock->ReadUInt("serverID", serverID);

			// if we have a lower server id, and are not involved in any complex state, then yield control
			if (serverID < m_iServerID && m_pStateFunc == State_BasePrimaryServerFrame)
			{
				// yield
				g_pConsole->Print(8, "Yielding primary server status\n", dataBlock->NetAddress().ToStaticString());

				m_PrimaryServerAddress = dataBlock->NetAddress();
				m_bFindingPrimaryServer = false;
				m_bPrimary = false;
				m_pStateFunc = NULL;
			}
			else
			{
				// just ignore
				return;
			}
		}

		if (m_bPrimary)
		{
			// we are the primary server, ignore
			return;
		}
	}

	// parse out the server info and store it off
	int serverCount;
	serv_t bin[400];

	dataBlock->ReadInt("servercount", serverCount);
	dataBlock->ReadBlob("servers", bin, sizeof(bin));

	// clear out the current servers list
	m_Servers.RemoveAll();

	g_pConsole->Print(3, "Removing all servers: adding %d\n", serverCount);

	// replace with new data
	m_Servers.AddMultipleToTail(serverCount);
	for (int i = 0; i < serverCount; i++)
	{
		server_t &server = m_Servers[i];

		server.addr.SetIP(bin[i].ip);
		server.addr.SetPort(bin[i].port);
		server.serverID = bin[i].serverID;

		// check to see if this server is the primary
		if (server.addr == m_PrimaryServerAddress)
		{
			server.primary = true;
		}
		else
		{
			server.primary = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message was received from the master to lock a user range
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_LockUserRange(IReceiveMessage *dataBlock)
{
	unsigned int lowerRange, upperRange;
	dataBlock->ReadUInt("lowerRange", lowerRange);
	dataBlock->ReadUInt("upperRange", upperRange);
	sessionmanager()->LockUserRange(lowerRange, upperRange);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_UnlockUserRange(IReceiveMessage *dataBlock)
{
	sessionmanager()->UnlockUserRange();
}

//-----------------------------------------------------------------------------
// Purpose: Handles a redirected message
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_RedirectToUser(IReceiveMessage *dataBlock)
{
	// parse out routing info
	int msgID;
	unsigned int userID, sessionID;
	dataBlock->ReadInt("rID", msgID);
	dataBlock->ReadUInt("rUserID", userID);
	dataBlock->ReadUInt("rSessionID", sessionID);

	// parse out the message to be routed
	char buf[1300];
	int bytesRead = dataBlock->ReadBlob("rData", buf, sizeof(buf));

//	g_pConsole->Print(6, "Redirecting msg (%d) to user (%d, %d)\n", msgID, userID, sessionID);

	// get the session manager to find the user session then pass on the message
	sessionmanager()->SendMessageToUser(userID, sessionID, msgID, buf, bytesRead);
}

//-----------------------------------------------------------------------------
// Purpose: network message indicating to kill user
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_ForceDisconnectUser(IReceiveMessage *dataBlock)
{
	unsigned int userID = 0;
	dataBlock->ReadUInt("userID", userID);

	if (userID)
	{
		sessionmanager()->ForceDisconnectUser(userID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: network message indicating to user should check their messages
//-----------------------------------------------------------------------------
void CTopologyManager::ReceivedMsg_UserCheckMessages(IReceiveMessage *dataBlock)
{
	unsigned int userID = 0;
	dataBlock->ReadUInt("userID", userID);

	if (userID)
	{
		sessionmanager()->UserCheckMessages(userID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles an undeliverable packet
//-----------------------------------------------------------------------------
void CTopologyManager::OnMessageFailed(IReceiveMessage *dataBlock)
{
	// find the server that matches the target
	bool bFound = false;
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		server_t &server = m_Servers[i];
		CNetAddress addr = dataBlock->NetAddress();
		if (dataBlock->NetAddress() == server.addr)
		{
			// this server must be down

			// is it primary?
			if (server.primary)
			{
				// argh primary server down!
				g_pConsole->Print(8, "Lost connection to server: %s (primary)\n", server.addr.ToStaticString());
			}
			else
			{
				// just a normal server - but since it could be the server that was supposed to take
				// over from the primary and has instead crashed
				g_pConsole->Print(8, "Lost connection to server: %s\n", server.addr.ToStaticString());
			}

			// drop server from list
			m_Servers.FastRemove(i);
			bFound = true;
		}
	}

	if (!bFound)
	{
		// the message wasn't to one of the servers, maybe a watcher
		for (int i = 0; i < m_Watchers.Size(); i++)
		{
			if (m_Watchers[i] == dataBlock->NetAddress())
			{
				// remove this watcher, it is probably down
				m_Watchers.FastRemove(i);
				return;
			}
		}

		// couldn't find target, just ignore
		return;	
	}

	if (m_bPrimary)
		return;		// we're the primary server already

	// check to see if we should try and become primary
	int primaryIndex = -1;
	unsigned int lowestID = m_iServerID;
	for (i = 0; i < m_Servers.Size(); i++)
	{
		if (m_Servers[i].primary)
		{
			primaryIndex = i;
		}

		if (m_Servers[i].serverID < m_iServerID)
		{
			lowestID = m_Servers[i].serverID;
		}
	}

	if (primaryIndex < 0)
	{
		// we don't have a primary server anymore; maybe we should inherit the title
		if (lowestID == m_iServerID)
		{
			// it's us!  announce to the other known servers that we are now the primary server
			BecomePrimaryServer();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes this server assume the role of primary
//-----------------------------------------------------------------------------
void CTopologyManager::BecomePrimaryServer()
{
	g_pConsole->Print(4, "Designating self as primary server\n");
	m_bPrimary = true;
	m_pStateFunc = State_BasePrimaryServerFrame;
	m_bFindingPrimaryServer = false;
	m_PrimaryServerAddress = m_pLocalNET->GetLocalAddress();

	// announce to everyone we know about that we're the primary server
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		if (m_Servers[i].serverID == m_iServerID)
		{
			// use the address we got via loopback
			m_PrimaryServerAddress = m_Servers[i].addr;
			m_Servers[i].primary = true;

			// don't send the message to ourselves
			continue;	
		}

		m_Servers[i].primary = false;

		ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_PRIMARYSRV);
		msg->SetNetAddress(m_Servers[i].addr);

		msg->WriteUInt("PrimaryIP", m_PrimaryServerAddress.IP());
		msg->WriteUInt("PrimaryPort", m_PrimaryServerAddress.Port());
		msg->WriteString("SQLDB", m_dbsName);

		m_pLocalNET->SendMessage(msg, NET_RELIABLE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sends the list of all the servers to a target address
//-----------------------------------------------------------------------------
void CTopologyManager::SendTopologyInfo(CNetAddress &addr)
{
	ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_SRVINFO);
	msg->SetNetAddress(addr);
	msg->SetEncrypted(false);
	msg->WriteUInt("serverID", m_iServerID);

	int serverCount = m_Servers.Size();
	msg->WriteInt("servercount", serverCount);

	CUtlVector<serv_t> servers;
	servers.AddMultipleToTail(serverCount);
	
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		servers[i].ip = m_Servers[i].addr.IP();
		servers[i].port = m_Servers[i].addr.Port();
		servers[i].serverID = m_Servers[i].serverID;
	}

	msg->WriteBlob("servers", servers.Base(), sizeof(serv_t) * serverCount);
	msg->WriteString("sqldb", m_dbsName);
	m_pLocalNET->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Ensures we know about this server
//-----------------------------------------------------------------------------
CTopologyManager::server_t &CTopologyManager::FindServer(CNetAddress &addr, unsigned int serverID)
{
	// try and find this server
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		if (m_Servers[i].addr == addr)
		{
			// ensure the serverID is valid
			if (serverID)
			{
				m_Servers[i].serverID = serverID;
			}

			return m_Servers[i];
		}
		else if (serverID && m_Servers[i].serverID == serverID)
		{
			// ip doesn't match, but the serverID does.
			// probably a bounced broadbast packet
			// Pretend this message is from the original server
			return m_Servers[i];
		}
	}

	// add the server to the list
	server_t server;
	server.addr = addr;
	server.primary = false;
	server.serverID = serverID;

	int serverIndex = m_Servers.AddToTail(server);
	
	if (m_bPrimary)
	{
		// we're the primary and we've just added a new server; so make sure the other servers know
		m_bResendServerInfo = true;
	}

	g_pConsole->Print(3, "Adding server %s (%d)\n", server.addr.ToStaticString(), server.serverID);

	return m_Servers[serverIndex];
}


//-----------------------------------------------------------------------------
// Purpose: locks a range of users across all servers
//-----------------------------------------------------------------------------
void CTopologyManager::LockUserRange(unsigned int lowerRange, unsigned int upperRange)
{
	// broadcast the message to all connected servers to lock the specified user range
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		if (m_Servers[i].serverID == m_iServerID)
			continue;

		ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_LOCKUSERRANGE);
		msg->SetNetAddress(m_Servers[i].addr);

		msg->WriteUInt("lowerRange", lowerRange);
		msg->WriteUInt("upperRange", upperRange);
		
		m_pLocalNET->SendMessage(msg, NET_RELIABLE);
	}

	// lock locally
	sessionmanager()->LockUserRange(lowerRange, upperRange);
}

//-----------------------------------------------------------------------------
// Purpose: unlocks all locked users on all servers
//-----------------------------------------------------------------------------
void CTopologyManager::UnlockUserRange()
{
	// broadcast to all connected servers that they can unlock any currently locked user range
	// broadcast the message to all connected servers to lock the specified user range
	for (int i = 0; i < m_Servers.Size(); i++)
	{
		if (m_Servers[i].serverID == m_iServerID)
			continue;

		ISendMessage *msg = m_pLocalNET->CreateMessage(TSV_UNLOCKUSERRANGE);
		msg->SetNetAddress(m_Servers[i].addr);

		m_pLocalNET->SendMessage(msg, NET_RELIABLE);
	}

	// unlock locally
	sessionmanager()->UnlockUserRange();
}

//-----------------------------------------------------------------------------
// Purpose: a signal to Start this thread
//-----------------------------------------------------------------------------
void CTopologyManager::WakeUp()
{
	sessionmanager()->WakeUp();
}

//-----------------------------------------------------------------------------
// Purpose: Handles a response from the database
//-----------------------------------------------------------------------------
void CTopologyManager::SQLDBResponse(int cmdID, int returnState, int returnVal, void *data)
{
	// make sure we're still in the wait state
	if (m_pStateFunc != State_AbortMoveUsers)
		return;

	if (cmdID == CMD_USERSAREONLINE)
	{
		if (returnVal > 0)
		{
			// we need to wait a bit longer, some users still logged in
			m_pStateFunc = State_MakeSureUserRangeIsOffline;
			m_flStateThinkTime = 0.2f;

			//!! need an error case for if the users never all go offline
		}
		else
		{
			// all the users in the range have been logged out, proceed
			m_pStateFunc = State_CopyUsers;
			m_flStateThinkTime = 0.0f;
		}
	}
	else if (cmdID == CMD_GETCOMPLETEUSERDATA)
	{
		if (returnVal > 0)
		{
			// allow a timeout on the copying to make sure we don't get stuck
			m_pStateFunc = State_AbortMoveUsers;
			m_flStateThinkTime = sessionmanager()->GetCurrentTime() + QUERY_TIMEOUT;

			// we've gotten the complete user data; now stuff it back into the previous database
			ITrackerDatabaseManager::db_t &database = g_pDataManager->TrackerUserByIndex(m_iDestDatabaseIndex);
			ITrackerUserDatabase *db = database.db;
			db->Users_InsertCompleteUserData(this, m_iMoveRangeLower, m_iMoveRangeUpper, &m_UserData);
		}
		else if (returnVal == 0)
		{
			// no users moved, but no error
			// proceed straight to success state
			m_pStateFunc = State_UserDataCopied;
		}
		else
		{
			// no users copied; abort
			m_pStateFunc = State_AbortMoveUsers;
			m_flStateThinkTime = 0.0f;
		}
	}
	else if (cmdID == CMD_INSERTCOMPLETEUSERDATA)
	{
		if (returnVal > 0)
		{
			// finished copying
			m_pStateFunc = State_UserDataCopied;
			m_flStateThinkTime = 0.0f;
		}
		else
		{
			// error, abort
			m_pStateFunc = State_AbortMoveUsers;
			m_flStateThinkTime = 0.0f;
		}
	}
	else if (cmdID == CMD_UPDATEUSERDISTRIBUTION)
	{
		// finished updating user distribution
		m_pStateFunc = State_UserDistributionUpdated;
		m_flStateThinkTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the db needs to be balanced
// Output : Returns true on work done, false nothing
//-----------------------------------------------------------------------------
bool CTopologyManager::State_BasePrimaryServerFrame()
{
	if (!g_pDataManager)
		return false;

	if (m_bShuttingDown)
		return false;

	m_flStateThinkTime = sessionmanager()->GetCurrentTime() + 1.5f;
	const int USERS_TO_MOVE = 5;

	int srcServerIndex = -1;
	int highestUserCount = 0;
	int totalRange = 0;
	for (int i = 0; i < g_pDataManager->TrackerUserCount(); i++)
	{
		ITrackerDatabaseManager::db_t &db = g_pDataManager->TrackerUserByIndex(i);

		int rangeSize;
		if (db.nextValidUserID > db.rangeMin)
		{
			// this is the allocation server, use from where it has allocated, not to it's max range
			rangeSize = db.nextValidUserID - db.rangeMin;
		}
		else
		{
			rangeSize = db.rangeMax - db.rangeMin;
		}
		
		// sum the total number of users
		totalRange += rangeSize;

		// choose the server with the highest user count to move users away from
		if (rangeSize > highestUserCount)
		{
			highestUserCount = rangeSize;
			srcServerIndex = i;
		}
	}

	// make sure we have found a src server
	if (srcServerIndex < 0)
		return false;

	// find which server lies before it (it will be where we move the users to)
	int destServerIndex = -1;
	for (i = 0; i < g_pDataManager->TrackerUserCount(); i++)
	{
		ITrackerDatabaseManager::db_t &db = g_pDataManager->TrackerUserByIndex(i);
		ITrackerDatabaseManager::db_t &srcDB = g_pDataManager->TrackerUserByIndex(srcServerIndex);

		if (db.rangeMax == srcDB.rangeMin - 1)
		{
			// server found
			destServerIndex = i;
			break;
		}
	}

	// make sure we found a destination server
	if (destServerIndex < 0)
		return false;

	// calculate the average number of users on each server, to see if it's worth doing any user moving
	int averageUsers = totalRange / g_pDataManager->TrackerUserCount();
	if (averageUsers < 100)
		return false;
	
	// if the highest range is within 100 users of the average, don't bother load balancing
	if (highestUserCount < averageUsers + 100)
		return false;

	// this is a lowpoint in users; we can Start moving users on to it
	
	// decide which users to move
	int lowRange, highRange;
	ITrackerDatabaseManager::db_t &db = g_pDataManager->TrackerUserByIndex(destServerIndex);

	m_iDestDatabaseIndex = destServerIndex;
	m_iSrcDatabaseIndex = srcServerIndex;

	lowRange = db.rangeMax + 1;
	highRange = db.rangeMax + USERS_TO_MOVE;

	m_iMoveRangeLower = lowRange;
	m_iMoveRangeUpper = highRange;

	g_pConsole->Print(3, "Attempting to move user range [%d, %d]\n", lowRange, highRange);

	LockUserRange(lowRange, highRange);
	m_pStateFunc = State_WaitForUserRangeLock;
	m_flStateThinkTime = sessionmanager()->GetCurrentTime() + 0.2f;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Server state - Waits until all servers have been locked in the specified user range
// Output : Returns true on work done, false nothing
//-----------------------------------------------------------------------------
bool CTopologyManager::State_WaitForUserRangeLock()
{
	//!! check to see if everyone is locked
	// theoretically everything should be fine; but if something slips through it could be bad

	// move into the next state, which is making sure that all the users we are about to move are offline
	m_pStateFunc = State_MakeSureUserRangeIsOffline;
	m_flStateThinkTime = 0.0f;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks the database and makes sure the users are all logged off before preceding
//-----------------------------------------------------------------------------
bool CTopologyManager::State_MakeSureUserRangeIsOffline()
{
	// set the state to be a timeout
	m_pStateFunc = State_AbortMoveUsers;
	m_flStateThinkTime = sessionmanager()->GetCurrentTime() + QUERY_TIMEOUT;

	// check the database, make sure all the users are offline
	ITrackerUserDatabase *db = g_pDataManager->TrackerUserByIndex(m_iSrcDatabaseIndex).db;

	if (db)
	{
		db->Users_AreOnline(this, m_iMoveRangeLower, m_iMoveRangeUpper);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Copies the user data to the new server
//-----------------------------------------------------------------------------
bool CTopologyManager::State_CopyUsers()
{
	// set the state to be a timeout
	m_pStateFunc = State_AbortMoveUsers;
	m_flStateThinkTime = sessionmanager()->GetCurrentTime() + QUERY_TIMEOUT;

	if (m_bShuttingDown)
		return false;

	// we're fully ready to copy users, send off all the database requests
	ITrackerUserDatabase *db = g_pDataManager->TrackerUserByIndex(m_iSrcDatabaseIndex).db;
	db->Users_GetCompleteUserData(this, m_iMoveRangeLower, m_iMoveRangeUpper, &m_UserData);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Cleans up after users copied
//-----------------------------------------------------------------------------
bool CTopologyManager::State_UserDataCopied()
{
	// find the databases that we're using
	ITrackerDatabaseManager::db_t &destDB = g_pDataManager->TrackerUserByIndex(m_iDestDatabaseIndex);
	ITrackerDatabaseManager::db_t &srcDB = g_pDataManager->TrackerUserByIndex(m_iSrcDatabaseIndex);
	
	// reset the state to be a timeout
	m_pStateFunc = State_AbortMoveUsers;
	m_flStateThinkTime = sessionmanager()->GetCurrentTime() + QUERY_TIMEOUT;

	// don't continue if we're shutting down
	if (m_bShuttingDown)
		return false;

	// update the user distribution
	g_pDataManager->TrackerDistro()->UpdateUserDistribution(this, srcDB.serverName, srcDB.catalogName, destDB.serverName, destDB.catalogName, m_iMoveRangeLower, m_iMoveRangeUpper);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Notifies other servers that the user distribution has been updated
//-----------------------------------------------------------------------------
bool CTopologyManager::State_UserDistributionUpdated()
{
	g_pConsole->Print(4, "Successfully moved user range [%d, %d]\n", m_iMoveRangeLower, m_iMoveRangeUpper);

	// set us to be in an abort state
	m_pStateFunc = State_AbortMoveUsers;
	m_flStateThinkTime = sessionmanager()->GetCurrentTime() + QUERY_TIMEOUT;

	// unlock user range & reload user distribution
	UnlockUserRange();

	// delete the old users
	g_pDataManager->TrackerUserByIndex(m_iSrcDatabaseIndex).db->Users_DeleteRange(m_iMoveRangeLower, m_iMoveRangeUpper);

	// return to base state
	m_iMoveRangeLower = 0;
	m_iMoveRangeUpper = 0;
	m_pStateFunc = State_BasePrimaryServerFrame;
	m_flStateThinkTime = sessionmanager()->GetCurrentTime() + QUERY_TIMEOUT;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Stops the user range move state machine, and unlocks the range on all servers
//-----------------------------------------------------------------------------
bool CTopologyManager::State_AbortMoveUsers()
{
	g_pConsole->Print(3, "Aborted moving user range [%d, %d]\n", m_iMoveRangeLower, m_iMoveRangeUpper);

	// reset our state
	UnlockUserRange();
	m_iMoveRangeLower = 0;
	m_iMoveRangeUpper = 0;
	m_pStateFunc = State_BasePrimaryServerFrame;
	m_flStateThinkTime = sessionmanager()->GetCurrentTime() + QUERY_TIMEOUT;

	return true;
}

