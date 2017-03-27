//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "../TrackerNET/TrackerNET_Interface.h"
#include "../TrackerNET/Threads.h"
#include "../TrackerNET/NetAddress.h"
#include "../TrackerNET/BinaryBuffer.h"

// common files
#include "proto_oob.h"
#include "random.h"

#include "Buddy.h"
#include "interface.h"
#include "OnlineStatus.h"
#include "ServerSession.h"
#include "Tracker.h"
#include "TrackerDoc.h"
#include "TrackerProtocol.h"
#include "TrackerDialog.h"

#include "UtlMsgBuffer.h"

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <time.h>

#include <VGUI_KeyValues.h>
#include <VGUI_Panel.h>

#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_IVGui.h>
#include "FileSystem.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

using namespace vgui;

CSysModule * g_hTrackerNetModule = NULL;

//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
CServerSession &ServerSession()
{
	static CServerSession s_ServerSession;
	return s_ServerSession;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServerSession::CServerSession() 
{
	m_iDesiredStatus = 0;
	m_iUID = 0;
	m_iGameIP = 0;
	m_iGamePort = 0;
	m_iStatus = 0;
	m_iSessionID = 0;
	m_szGameDir[0] = 0;
	m_iPreviousHeartBeatRateSentToServer = 0;
	m_iFirewallWindow = 0;

	// load networking dll
	char szDLL[_MAX_PATH];
	vgui::filesystem()->GetLocalPath("Friends/TrackerNET.dll", szDLL);
	vgui::filesystem()->GetLocalCopy(szDLL);
	g_hTrackerNetModule = Sys_LoadModule(szDLL);

	CreateInterfaceFn netFactory = Sys_GetFactory(g_hTrackerNetModule);
	m_pNet = (ITrackerNET *)netFactory(TRACKERNET_INTERFACE_VERSION, NULL);

	// listen on any port between 27000-27100 (make this user configurable eventually)
	if (Tracker_StandaloneMode())
	{
		m_pNet->Initialize(27000, 27100);
	}
	else
	{
		// use a different port in the game
		m_pNet->Initialize(27001, 27100);
	}

	m_iWatchCount = 0;
	m_iReconnectRetries = 0;

	// get a pointer to the user data
	m_pUserData = ::GetDoc()->Data()->FindKey("User");

	m_iUID = ::GetDoc()->GetUserID();

	// load the server info file

	KeyValues *masterServers = ::GetDoc()->Data()->FindKey("MasterServers", true);
	masterServers->LoadFromFile(filesystem(), "Friends/servers.dat", "PLATFORM");

	// iterate through the servers adding them to the list
	for (KeyValues *kv = masterServers->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		const char *addr = kv->GetString("address", NULL);

		if (addr)
		{
			m_ServerList.AddToTail(m_pNet->GetNetAddress(addr));
		}
	}

	// make sure we have at least one server
	if (m_ServerList.Size() < 1)
	{
		m_ServerList.AddToTail(m_pNet->GetNetAddress("tracker.valvesoftware.com:1200"));
	}
	
	// check to see if we have a server saved
	m_iCurrentServer = ::GetDoc()->Data()->GetInt("App/Server", -1);
	if (m_iCurrentServer < 0 || m_iCurrentServer >= m_ServerList.Size())
	{
		// choose a server at random
		m_iCurrentServer = RandomLong(0, m_ServerList.Size() - 1);
	}

	m_iLastHeartbeat = 0;
	m_iTime = 0;
	m_iLastReceivedTime = 0;
	m_iReconnectTime = 0;
	m_iLoginState = LOGINSTATE_DISCONNECTED;
	m_bServerSearch = false;

	// calculate build time
	extern int build_number( void );
	m_iBuildNum = build_number();

	// log some basic build information
	WriteToLog("**** Tracker Networking Initialized ****\n");
	WriteToLog("Build number: %d\n", m_iBuildNum);

	const char *serverAddr = "<unknown>";
	kv = masterServers->GetFirstSubKey();
	for (int i = 0; i < m_ServerList.Size() && kv != NULL; i++, kv = kv->GetNextKey())
	{
		if (i == m_iCurrentServer)
		{
			serverAddr = kv->GetString("address", "<unknown>");
			break;
		}
	}

	WriteToLog("Server IP: %s\n", serverAddr);
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServerSession::~CServerSession()
{
}

//-----------------------------------------------------------------------------
// Purpose: Runs a single frame of the networking
//-----------------------------------------------------------------------------
void CServerSession::RunFrame()
{
	if (!m_pNet)
		return;

	unsigned int timeMillis = system()->GetTimeMillis();
	m_iTime = timeMillis;

	// get the latest raw messages
	IBinaryBuffer *buf;
	CNetAddress address;
	while ((buf = m_pNet->GetIncomingRawData(address)) != NULL)
	{
		ReceivedRawData(buf, address);
		buf->Release();
	}

	// get all the latest messages
	IReceiveMessage *recv;
	while ((recv = m_pNet->GetIncomingData()) != NULL)
	{
		ReceivedData(recv);
		m_pNet->ReleaseMessage(recv);
	}
	// get the latest fails
	while ((recv = m_pNet->GetFailedMessage()) != NULL)
	{
		MessageFailed(recv);
		m_pNet->ReleaseMessage(recv);
	}
	m_pNet->RunFrame();

	// check for any timeouts/etc.
	CheckForConnectionStateChange();
}


//-----------------------------------------------------------------------------
// Purpose: shuts down the networking, killing the threads and ensuring all remaing messages are sent
//-----------------------------------------------------------------------------
void CServerSession::Shutdown()
{
	// set a flag indicating the threads should kill themselves
	m_pNet->Shutdown(true);
	m_pNet->deleteThis();
	m_pNet = NULL;

	Sys_UnloadModule(g_hTrackerNetModule);
}

//-----------------------------------------------------------------------------
// Purpose: Returns the time, in milliseconds, between heartbeats
//-----------------------------------------------------------------------------
int CServerSession::GetHeartBeatRate()
{
	// scale heartbeats by our status
	int heartBeatTime = COnlineStatus::HEARTBEAT_TIME;
	int status = GetStatus();
	if (status == COnlineStatus::AWAY)
	{
		heartBeatTime *= 2;
	}
	else if (status == COnlineStatus::SNOOZE)
	{
		heartBeatTime *= 6;
	}
	else if (vgui::system()->GetTimeSinceLastUse() < 20.0f)
	{
		// reduce the time if the user is actively using the pc
		heartBeatTime /= 3;
	}

	return heartBeatTime;
}

//-----------------------------------------------------------------------------
// Purpose: Tries to connect to a different master server
// Output : Returns true on if we should try to reconnect immediately, false otherwise
//-----------------------------------------------------------------------------
bool CServerSession::FallbackToAlternateServer()
{
	m_iCurrentServer += 1;
	if (m_iCurrentServer >= m_ServerList.Size())
	{
		// loop back to the first server
		m_iCurrentServer = 0;	
	}

	if (m_iCurrentServer == 0)
	{
		// we've cycled the server list, so wait some time before trying to reconnect
		return false;
	}
	else
	{
		// try to connect to the next server immediately
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: starts looking for a valid server without actually logging 
//			into it (for initial CreateUser requests)
//-----------------------------------------------------------------------------
void CServerSession::UnconnectedSearchForServer()
{
	m_bServerSearch = true;

	// ping all the servers in the list
	for (int i = 0; i < m_ServerList.Size(); i++)
	{
		ISendMessage *msg = m_pNet->CreateMessage(TCLS_PING);
		msg->SetNetAddress(m_ServerList[i]);
		msg->SetSessionID(0);
		msg->SetEncrypted(true);

		m_pNet->SendMessage(msg, NET_RELIABLE);
	}

	WriteToLog("Sent: ping()\n");
}


//-----------------------------------------------------------------------------
// Purpose: Sends the first pack in the login sequence
//-----------------------------------------------------------------------------
void CServerSession::SendInitialLogin(int desiredStatus)
{
	assert(m_iLoginState == LOGINSTATE_WAITINGTORECONNECT || m_iLoginState == LOGINSTATE_DISCONNECTED);

	m_iUID = ::GetDoc()->GetUserID();
	if (!m_iUID)
		return;

	m_iSessionID = 0;

	// Stop searching for alternate servers
	m_bServerSearch = false;

	// remember our desired status
	if (desiredStatus < COnlineStatus::ONLINE)
	{
		desiredStatus = COnlineStatus::ONLINE;
	}

	::GetDoc()->Data()->SetInt("Login/DesiredStatus", desiredStatus);

	// setup the login message
	ISendMessage *loginMsg = m_pNet->CreateMessage(TCLS_LOGIN);
	loginMsg->SetNetAddress(GetServerAddress());
	loginMsg->SetEncrypted(true);
	loginMsg->SetSessionID(0);

	loginMsg->WriteUInt("uid", m_iUID);
	loginMsg->WriteString("email", m_pUserData->GetString("email"));
	loginMsg->WriteString("password", ::GetDoc()->Data()->GetString("App/password"));
	loginMsg->WriteInt("status", desiredStatus);
	m_iFirewallWindow = ::GetDoc()->Data()->GetInt("App/FirewallWindow", 0);
	if (m_iFirewallWindow)
	{
		loginMsg->WriteInt("FirewallWindow", m_iFirewallWindow);
	}

	m_pNet->SendMessage(loginMsg, NET_RELIABLE);

	WriteToLog("Sent: Login(userID: %d, %s)\n", m_iUID, m_pUserData->GetString("email", ""));

	// set the current status to be a connecting message
	m_iStatus = COnlineStatus::CONNECTING;
	CTrackerDialog::GetInstance()->PostMessage(CTrackerDialog::GetInstance(), new KeyValues("OnlineStatus", "value", COnlineStatus::CONNECTING));

	m_iLoginState = LOGINSTATE_AWAITINGCHALLENGE;
	// record the time (for timeouts)
	m_iLoginTimeout = system()->GetTimeMillis() + COnlineStatus::SERVERCONNECT_TIMEOUT;

	if (m_iGameIP)
	{
		// we're in a game, set us to heartbeat real soon so our game status gets updated
		m_iLastHeartbeat = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: cancels any current attempt to connect to the server
//-----------------------------------------------------------------------------
void CServerSession::CancelConnect()
{
	// stay offline
	m_iLoginState = LOGINSTATE_DISCONNECTED;
	m_iLoginTimeout = 0;
	m_iReconnectTime = 0;
	m_iStatus = COnlineStatus::OFFLINE;
}

//-----------------------------------------------------------------------------
// Purpose: changes status to user's choice
//-----------------------------------------------------------------------------
void CServerSession::UserSelectNewStatus(int requestedStatus)
{
	OnUserChangeStatus(requestedStatus);
}

// message dispatch table
typedef void (CServerSession::*funcPtr)( IReceiveMessage * );
struct ServerMsgDispatch_t
{
	int msgName;
	funcPtr msgFunc;
};

static ServerMsgDispatch_t g_ServerMsgDispatch[] =
{
	{ TSVC_CHALLENGE,	CServerSession::ReceivedMsg_Challenge },
	{ TSVC_LOGINOK,		CServerSession::ReceivedMsg_LoginOK },
	{ TSVC_LOGINFAIL,	CServerSession::ReceivedMsg_LoginFail },
	{ TSVC_DISCONNECT,	CServerSession::ReceivedMsg_Disconnect },
	{ TSVC_FRIENDS,		CServerSession::ReceivedMsg_Friends },
	{ TSVC_FRIENDUPDATE, CServerSession::ReceivedMsg_FriendUpdate },
	{ TSVC_GAMEINFO,	CServerSession::ReceivedMsg_GameInfo },
	{ TSVC_HEARTBEAT,	CServerSession::ReceivedMsg_Heartbeat },
	{ TSVC_PINGACK,		CServerSession::ReceivedMsg_PingAck },
};

// optimizer problem with the dispatch code
#pragma optimize("g", off)

//-----------------------------------------------------------------------------
// Purpose: Base received data handler
//-----------------------------------------------------------------------------
bool CServerSession::ReceivedData(IReceiveMessage *dataBlock)
{
	// make sure the message is valid
	if (!CheckMessageValidity(dataBlock))
		return false;

	// record the reception
	m_iLastReceivedTime = m_iTime;

	// find the message id in the dispatch table
	int dataName = dataBlock->GetMsgID();
	int arraySize = ARRAYSIZE(g_ServerMsgDispatch);
	for ( int i = 0; i < arraySize; i++ )
	{
		if (dataName == g_ServerMsgDispatch[i].msgName)
		{
			(this->*g_ServerMsgDispatch[i].msgFunc)( dataBlock );
			return true;
		}
	}

	// use the default handler
	return ReceivedMsg_DefaultHandler(dataBlock);
}
#pragma optimize("", on)

//-----------------------------------------------------------------------------
// Purpose: Raw data, outside normal tracker network traffic
//-----------------------------------------------------------------------------
void CServerSession::ReceivedRawData(IBinaryBuffer *data, CNetAddress &address)
{
	// make sure it's an engine-style out of band message
	int header = data->ReadInt();
	if (header != -1)
		return;

	// all raw data handling has been removed, ignore the packet

	return;
}

//-----------------------------------------------------------------------------
// Purpose: Handles stayalive messages from the server
//-----------------------------------------------------------------------------
void CServerSession::ReceivedMsg_Heartbeat(IReceiveMessage *serverMsg)
{
	// see if our firewall window time has been updated
	serverMsg->ReadInt("FirewallWindow", m_iFirewallWindow);
	if (m_iFirewallWindow)
	{
		// remember our firewall window so we can send it to the server next time
		::GetDoc()->Data()->SetInt("App/FirewallWindow", m_iFirewallWindow);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Basic acknowledge message
//-----------------------------------------------------------------------------
void CServerSession::ReceivedMsg_PingAck(IReceiveMessage *nullMsg)
{
	WriteToLog("Received: PingAck()\n");

	if (m_bServerSearch)
	{
		// we can Stop searching for a server, we have found one that responds
		m_bServerSearch = false;

		// look up which server it is that replied in the list
		for (int i = 0; i < m_ServerList.Size(); i++)
		{
			if (m_ServerList[i] == nullMsg->NetAddress())
			{
				m_iCurrentServer = i;
				break;
			}
		}

		// dispatch message to the UI
		ReceivedMsg_DefaultHandler(nullMsg);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Received a login challenge from the server
//-----------------------------------------------------------------------------
void CServerSession::ReceivedMsg_Challenge( IReceiveMessage *challengeMsg )
{
	if (m_iLoginState != LOGINSTATE_AWAITINGCHALLENGE)
		return;

	challengeMsg->ReadInt("challenge", m_iChallengeKey);
	challengeMsg->ReadUInt("sessionID", m_iSessionID);

	WriteToLog("Received: Challenge()\n");

	OnReceivedChallenge();
}

//-----------------------------------------------------------------------------
// Purpose: Handles a LoginOK message from the server
//-----------------------------------------------------------------------------
void CServerSession::ReceivedMsg_LoginOK(IReceiveMessage *pMsg)
{
	if (m_iLoginState != LOGINSTATE_AWAITINGLOGINSUCCESS)
		return;

	// read new status
	int newStatus;
	pMsg->ReadInt("status", newStatus);

	// login is OK
	ReceivedMsg_DefaultHandler(pMsg);
	
	OnReceivedLoginSuccess(newStatus);
}

//-----------------------------------------------------------------------------
// Purpose: Handles a bad login
//-----------------------------------------------------------------------------
void CServerSession::ReceivedMsg_LoginFail(IReceiveMessage *pMsg)
{
	int failReason;
	pMsg->ReadInt("reason", failReason);

	if (failReason != -3 && failReason < 0)
	{
		ReceivedMsg_DefaultHandler(pMsg);
	}

	OnLoginFail(failReason);
}

//-----------------------------------------------------------------------------
// Purpose: Network message handler for state information of a single user
//-----------------------------------------------------------------------------
void CServerSession::ReceivedMsg_FriendUpdate(IReceiveMessage *friendMsg)
{
	unsigned int userID, serverID, ip, port, sessionID, gameIP, gamePort;
	int status; 
	char gameType[64];
	friendMsg->ReadUInt("userID", userID);
	friendMsg->ReadInt("status", status);
	friendMsg->ReadUInt("sessionID", sessionID);
	friendMsg->ReadUInt("serverID", serverID);
	friendMsg->ReadUInt("IP", ip);
	friendMsg->ReadUInt("port", port);
	friendMsg->ReadUInt("GameIP", gameIP);
	friendMsg->ReadUInt("GamePort", gamePort);
	friendMsg->ReadString("Game", gameType, sizeof(gameType));

	// update document
	GetDoc()->UpdateBuddyStatus(userID, status, sessionID, serverID, &ip, &port, &gameIP, &gamePort, gameType);

	// send the update message to the panels
	DispatchMessageToWatchers(new KeyValues("Friends", "_id", TSVC_FRIENDS, "count", 1));
}

//-----------------------------------------------------------------------------
// Purpose: Handles game information for a user, sent from the server
//-----------------------------------------------------------------------------
void CServerSession::ReceivedMsg_GameInfo(IReceiveMessage *msg)
{
	unsigned int userID, gameIP, gamePort;
	int status;
	char gameType[64];
	msg->ReadUInt("userID", userID);
	msg->ReadInt("status", status);
	msg->ReadUInt("GameIP", gameIP);
	msg->ReadUInt("GamePort", gamePort);
	msg->ReadString("Game", gameType, sizeof(gameType));

	// update document
	GetDoc()->UpdateBuddyStatus(userID, status, 0, 0, NULL, NULL, &gameIP, &gamePort, gameType);

	// send the update message to the panels
	DispatchMessageToWatchers(new KeyValues("GameInfo", "_id", TSVC_GAMEINFO, "userID", userID));
}

//-----------------------------------------------------------------------------
// Purpose: Handles updating the status of a set of friends
//-----------------------------------------------------------------------------
void CServerSession::ReceivedMsg_Friends(IReceiveMessage *friendMsg)
{
	int count;
	int buf[240];
	friendMsg->ReadInt("count", count);
	int bytesRead = friendMsg->ReadBlob("status", buf, sizeof(buf));

	WriteToLog("Received: Friends(count: %d)\n", count);

	int bytesPerUser = bytesRead / count;
	bool readServerID = (bytesPerUser > 20);

	int pos = 0;
	for (int i = 0; i < count; i++)
	{
		unsigned int friendID = buf[pos++];
		int status = buf[pos++];
		unsigned int sessionID = buf[pos++];
		unsigned int IP = buf[pos++];
		unsigned int port = buf[pos++];
		unsigned int serverID = 0;
		if (readServerID)
		{
			serverID = buf[pos++];
		}

		GetDoc()->UpdateBuddyStatus(friendID, status, sessionID, serverID, &IP, &port, NULL, NULL, NULL);
	}

	// do the default dispatch also
	DispatchMessageToWatchers(new KeyValues("Friends", "_id", TSVC_FRIENDS, "count", count));
}

//-----------------------------------------------------------------------------
// Purpose: Disconnects the user from the server for a set amount of time
//-----------------------------------------------------------------------------
void CServerSession::ReceivedMsg_Disconnect(IReceiveMessage *msg)
{
	int minTime, maxTime;
	msg->ReadInt("minTime", minTime);
	msg->ReadInt("maxTime", maxTime);

	WriteToLog("Received: Disconnect(time: %d, %d)\n", minTime, maxTime);

	OnReceivedDisconnectMessage(minTime, maxTime);
}


//-----------------------------------------------------------------------------
// Purpose: Server request for searching for users
//-----------------------------------------------------------------------------
void CServerSession::SearchForFriend(unsigned int uid, const char *email, const char *username, const char *firstname, const char *lastname)
{
	ISendMessage *msg = CreateServerMessage(TCLS_FRIENDSEARCH);
	msg->WriteUInt("uid", uid);
	msg->WriteString("Email", email);
	msg->WriteString("UserName", username);
	msg->WriteString("FirstName", firstname);
	msg->WriteString("LastName", lastname);

	m_pNet->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Sends new status to server
//-----------------------------------------------------------------------------
void CServerSession::SendStatusToServer(int newStatus)
{
	if (!m_pNet)
		return;

	if (m_iLoginState != LOGINSTATE_CONNECTED)
		return;

	assert(m_iStatus > COnlineStatus::OFFLINE);

	ISendMessage *msg = CreateServerMessage(TCLS_HEARTBEAT);
	msg->WriteInt("status", newStatus);

	// if we're in a game, send that as well
	if (m_iGameIP && m_iGamePort && newStatus == COnlineStatus::INGAME)
	{
		msg->WriteUInt("GameIP", m_iGameIP);
		msg->WriteUInt("GamePort", m_iGamePort);
		msg->WriteString("Game", m_szGameDir);
	}

	// tell the server our new heartbeat rate if necessary
	int heartBeatRate = GetHeartBeatRate();
	if (m_iPreviousHeartBeatRateSentToServer != heartBeatRate)
	{
		msg->WriteInt("hrate", heartBeatRate);
		m_iPreviousHeartBeatRateSentToServer = heartBeatRate;
	}
	
	m_pNet->SendMessage(msg, NET_RELIABLE);

	if (newStatus == COnlineStatus::INGAME)
	{
		WriteToLog("Sent: Heartbeat(status: %d, %d, %d, %s)\n", newStatus, m_iGameIP, m_iGamePort, m_szGameDir);
	}
	else
	{
		WriteToLog("Sent: Heartbeat(status: %d)\n", newStatus);
	}

	// mark the heartbeat
	m_iLastHeartbeat = m_iTime;
}

//-----------------------------------------------------------------------------
// Purpose: sets our game state so it can be uploaded to the server in a heartbeat
//-----------------------------------------------------------------------------
void CServerSession::SetGameInfo(const char *gameDir, unsigned int gameIP, unsigned int gamePort)
{
	// store the game info
	strncpy(m_szGameDir, gameDir, 63);
	m_iGameIP = gameIP;
	m_iGamePort = gamePort;

	// set it so we heartbeat real soon to force the game info to be uploaded
	m_iLastHeartbeat = 0;
}

//-----------------------------------------------------------------------------
// Purpose: gets the current user status
//-----------------------------------------------------------------------------
int CServerSession::GetStatus()
{
	return m_iStatus;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CServerSession::IsConnectedToServer()
{
	return m_iStatus >= COnlineStatus::ONLINE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CServerSession::IsConnectingToServer()
{
	return m_iStatus == COnlineStatus::CONNECTING;
}

//-----------------------------------------------------------------------------
// Purpose: Request to see if the specified email/password are a valid login for a user
//-----------------------------------------------------------------------------
void CServerSession::ValidateUserWithServer(const char *email, const char *password)
{
	ISendMessage *msg = CreateServerMessage(TCLS_VALIDATEUSER);
	msg->WriteString("email", email);
	msg->WriteString("password", password);

	WriteToLog("Sent: ValidateUser(email: %s)\n", email);

	m_pNet->SendMessage(msg, NET_RELIABLE);
}


//-----------------------------------------------------------------------------
// Purpose: Responds to a user
//-----------------------------------------------------------------------------
void CServerSession::SendAuthUserMessage(unsigned int targetID, bool allow)
{
	ISendMessage *msg = CreateServerMessage(TCLS_AUTHUSER);
	msg->WriteUInt("targetID", targetID);
	msg->WriteInt("auth", allow);

	m_pNet->SendMessage(msg, NET_RELIABLE);
}


//-----------------------------------------------------------------------------
// Purpose: Sends a message to the server requesting authorization for the user
//-----------------------------------------------------------------------------
void CServerSession::RequestAuthorizationFromUser(unsigned int uid, const char *requestReason)
{
	// Validate
	if (uid < 1)
		return;

	ISendMessage *authUserMsg = CreateServerMessage(TCLS_REQAUTH);

	authUserMsg->WriteUInt("uid", uid);
	authUserMsg->WriteString("ReqReason", requestReason);

	m_pNet->SendMessage(authUserMsg, NET_RELIABLE);
}


//-----------------------------------------------------------------------------
// Purpose: Passes the message up to the default handler to be dispatched
//-----------------------------------------------------------------------------
bool CServerSession::ReceivedMsg_DefaultHandler(IReceiveMessage *msg)
{
	// unpack the message
	// need to turn the msg id into a name
	KeyValues *dat = new KeyValues("msg");
	dat->SetInt("_id", msg->GetMsgID());
	::GetDoc()->ReadIn(dat, msg);
	dat->SetInt("IP", msg->NetAddress().IP());
	dat->SetInt("Port", msg->NetAddress().Port());

	return DispatchMessageToWatchers(dat);
}

//-----------------------------------------------------------------------------
// Purpose: Handles the return of a network message that could not be delivered
//-----------------------------------------------------------------------------
void CServerSession::MessageFailed(IReceiveMessage *msg)
{
	// check to see if the failed message was to the server
	if (msg->NetAddress() == GetServerAddress() && GetStatus() >= COnlineStatus::ONLINE)
	{
		// one of the Client->server messages failed; disconnect from server
		// record the desired status, and go offline since the server isn't responding
		::GetDoc()->Data()->SetInt("Login/DesiredStatus", GetStatus());

		OnConnectionTimeout();
	}

	// unpack the message, marked as a fail
	KeyValues *dat = new KeyValues("fail_msg");
	dat->SetInt("_id", msg->GetMsgID());
	::GetDoc()->ReadIn(dat, msg);
	dat->SetInt("IP", msg->NetAddress().IP());
	dat->SetInt("Port", msg->NetAddress().Port());

	DispatchMessageToWatchers(dat);
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the current message is valid
//			replies with a message telling the sender if it's not
//-----------------------------------------------------------------------------
bool CServerSession::CheckMessageValidity(IReceiveMessage *dataBlock)
{
	int msgID = dataBlock->GetMsgID();
	if (msgID == TSVC_FRIENDS || msgID == TSVC_GAMEINFO || msgID == TSVC_HEARTBEAT || msgID == TSVC_FRIENDUPDATE)
	{
		// see if the server really knows us
		if (m_iStatus < COnlineStatus::ONLINE || m_iSessionID != dataBlock->SessionID())
		{
			// the server thinks we're still logged on to it
			// tell the server we're actually logged off from it
			ISendMessage *msg = m_pNet->CreateReply(TCLS_HEARTBEAT, dataBlock);
			// tell it we're the sessionID it thinks we are
			msg->SetSessionID(dataBlock->SessionID());
			msg->WriteInt("status", 0);
			m_pNet->SendMessage(msg, NET_RELIABLE);
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: takes a network message and sends it to any vgui panel that registered
//			a watch for it
//-----------------------------------------------------------------------------
bool CServerSession::DispatchMessageToWatchers(KeyValues *msg, bool postFast)
{
	// look for any handlers in the watch list
	bool bHandled = false;
	for (int i = 0; i < m_iWatchCount; i++)
	{
		watchitem_t *it = m_WatchList + i;

		if (it->messageID == msg->GetInt("_id") && it->hPanel.Get())
		{
			// found, send it off
			if (postFast)
			{
				// post fast means the message is handled at the panel immediately, instead of waiting until the end of the frame
				ivgui()->PostMessage(it->hPanel->GetVPanel(), msg->MakeCopy(), NULL);
			}
			else
			{
				ivgui()->PostMessage(it->hPanel->GetVPanel(), msg->MakeCopy(), NULL);
			}
			bHandled = true;
		}
	}

	if (!bHandled)
	{
		ivgui()->DPrintf2("Warning: Network message '%s' not handled\n", msg->GetName());
	}

	if (msg)
	{
		msg->deleteThis();
	}

	return bHandled;
}

//-----------------------------------------------------------------------------
// Purpose: adds a panel to the list of vgui panels watching for a network message
//-----------------------------------------------------------------------------
void CServerSession::AddNetworkMessageWatch(Panel *panel, int msgID)
{
	watchitem_t newItem;
	newItem.hPanel = panel;
	newItem.messageID = msgID;
	m_WatchList[m_iWatchCount++] = newItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerSession::RemoveNetworkMessageWatch(Panel *panel)
{
	// look through the list for panel and remove them
	for (int i = 0; i < m_iWatchCount; i++)
	{
		watchitem_t *it = m_WatchList + i;

		if (it->hPanel.Get() == NULL || it->hPanel.Get() == panel)
		{
			// remove item
			memmove(m_WatchList + i, m_WatchList + (i + 1), (m_iWatchCount - i) * sizeof(watchitem_t));
			m_iWatchCount--;
			i--;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the address of the currently active server
//-----------------------------------------------------------------------------
CNetAddress CServerSession::GetServerAddress(void)
{
	if (!m_ServerList.IsValidIndex(m_iCurrentServer))
		return m_ServerList[0];

	return m_ServerList[m_iCurrentServer];
}

//-----------------------------------------------------------------------------
// Purpose: Creates and sets up the a message to be sent to the server
//-----------------------------------------------------------------------------
ISendMessage *CServerSession::CreateServerMessage(int msgID)
{
	ISendMessage *msg = m_pNet->CreateMessage(msgID);
	msg->SetNetAddress(GetServerAddress());
	msg->SetSessionID(m_iSessionID);
	msg->SetEncrypted(true);

	return msg;
}

//-----------------------------------------------------------------------------
// Purpose: Sends a buffer-constructed message to a user directly
//-----------------------------------------------------------------------------
void CServerSession::SendUserMessage(unsigned int userID, int msgID, CUtlMsgBuffer &msgBuffer)
{
	CBuddy *bud = GetDoc()->GetBuddy(userID);
	if (!bud)
		return;
	KeyValues *buddy = bud->Data();
	if (!buddy)
		return;

	ISendMessage *msg = m_pNet->CreateMessage(msgID, msgBuffer.Base(), msgBuffer.DataSize());
	
	// set address
	CNetAddress addr(buddy->GetInt("IP"), buddy->GetInt("Port"));
	msg->SetNetAddress(addr);
	
	// set the session ID to be the session ID of the target
	msg->SetSessionID(buddy->GetInt("SessionID"));

	// enable encryption
	msg->SetEncrypted(true);

	// transmit the message
	m_pNet->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Creates a message to a user which is to be routed through the server
//			use for firewall circumvention
//-----------------------------------------------------------------------------
void CServerSession::SendRoutedUserMessage(unsigned int targetID, int msgID, CUtlMsgBuffer &msgBuffer)
{
	// get the friend
	CBuddy *buddy = ::GetDoc()->GetBuddy(targetID);
	assert(buddy != NULL);

	// create the message to the server
	ISendMessage *msg = CreateServerMessage(TCLS_ROUTETOFRIEND);

	// write in the redirection info
	msg->WriteInt("rID", msgID);
	msg->WriteUInt("rUserID", targetID);
	msg->WriteUInt("rSessionID", buddy->Data()->GetInt("sessionID"));
	int serverID = buddy->Data()->GetInt("serverID");
	msg->WriteUInt("rServerID", serverID);
	msg->WriteBlob("rData", msgBuffer.Base(), msgBuffer.DataSize());

	m_pNet->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Creates a message to send to a friend - returns NULL if user unknown
//-----------------------------------------------------------------------------
ISendMessage *CServerSession::CreateUserMessage(unsigned int userID, int msgID)
{
	CBuddy *bud = GetDoc()->GetBuddy(userID);
	if (!bud)
		return NULL;

	KeyValues *buddy = bud->Data();
	if (!buddy)
		return NULL;

	ISendMessage *msg = m_pNet->CreateMessage(msgID);

	// get address
	CNetAddress addr;
	addr.SetIP(buddy->GetInt("IP"));
	addr.SetPort(buddy->GetInt("Port"));
	msg->SetNetAddress(addr);

	msg->SetEncrypted(true);

	// set the session ID to be the session ID of the target
	msg->SetSessionID(buddy->GetInt("SessionID"));

	return msg;
}


//-----------------------------------------------------------------------------
// Purpose: Requests information about a user from the server
//			this is only basic information, not the extended peer-peer only info
//-----------------------------------------------------------------------------
void CServerSession::RequestUserInfoFromServer(unsigned int userID)
{
	ISendMessage *msg = CreateServerMessage(TCLS_FRIENDINFO);
	msg->WriteUInt("uid", userID);
	m_pNet->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Updates the server with a new set of information about ourself
//-----------------------------------------------------------------------------
void CServerSession::UpdateUserInfo(unsigned int userID, const char *userName, const char *firstName, const char *lastName)
{
	ISendMessage *msg = CreateServerMessage(TCLS_SETINFO);
	
	msg->WriteUInt("uid", userID);
	msg->WriteString("UserName", userName);
	msg->WriteString("FirstName", firstName);
	msg->WriteString("LastName", lastName);

	m_pNet->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to a friend, sending through the server if necessary
//-----------------------------------------------------------------------------
void CServerSession::SendTextMessageToUser(unsigned int targetID, const char *text, int chatID)
{
	// get the friend
	CBuddy *buddy = ::GetDoc()->GetBuddy(targetID);
	if (!buddy)
		return;

	// build our message
	CUtlMsgBuffer msgBuffer;
	msgBuffer.WriteInt("_id", TCL_MESSAGE);
	msgBuffer.WriteUInt("uid", m_iUID);
	msgBuffer.WriteUInt("targetID", targetID);
	msgBuffer.WriteString("UserName", GetDoc()->GetUserName());
	msgBuffer.WriteInt("status", GetStatus());
	msgBuffer.WriteString("Text", text);
	if (chatID)
	{
		msgBuffer.WriteInt("chatID", chatID);
	}
	
	// see how it should be sent
//	if (buddy->SendViaServer())
	{
		// send the message via the server
		SendRoutedUserMessage(targetID, TCL_MESSAGE, msgBuffer);
	}
//	else
//	{
//		SendUserMessage(targetID, TCL_MESSAGE, msgBuffer);
//	}
}

//-----------------------------------------------------------------------------
// Purpose: Tells the other user to block out info from this one
//-----------------------------------------------------------------------------
void CServerSession::SetUserBlock(unsigned int userID, int block, int fakeStatus)
{
	ISendMessage *msg = CreateUserMessage(userID, TCL_USERBLOCK);
	if (!msg)
		return;

	msg->WriteUInt("uid", m_iUID);
	msg->WriteUInt("Block", block);
	msg->WriteInt("FakeStatus", fakeStatus);

	m_pNet->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerSession::SendKeyValuesMessage(unsigned int userID, int msgID, KeyValues *data)
{
	ISendMessage *msg = CreateUserMessage(userID, msgID);
	if (!msg)
		return;

	for (KeyValues *subKey = data->GetFirstSubKey(); subKey != NULL; subKey = subKey->GetNextKey())
	{
		if (subKey->GetDataType() == KeyValues::TYPE_INT)
		{
			msg->WriteInt(subKey->GetName(), subKey->GetInt());
		}
		else if (subKey->GetDataType() == KeyValues::TYPE_STRING)
		{
			msg->WriteString(subKey->GetName(), subKey->GetString(NULL, ""));
		}
	}

	m_pNet->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Sends a create new user message to the server
//-----------------------------------------------------------------------------
void CServerSession::CreateNewUser(const char *email, const char *userName, const char *firstName, const char *lastName, const char *password)
{
	// send the message, register ourselves as the reply handler
	ISendMessage *msg = CreateServerMessage(TCLS_CREATEUSER);

	msg->WriteString("username", userName);
	msg->WriteString("firstname", firstName);
	msg->WriteString("lastname", lastName);
	msg->WriteString("email", email);
	msg->WriteString("password", password);

	m_pNet->SendMessage(msg, NET_RELIABLE);

	WriteToLog("Sent: CreateUser(%s)\n", email);
}


//-----------------------------------------------------------------------------
// Purpose: changes the state of the user to reconnect
//-----------------------------------------------------------------------------
void CServerSession::OnConnectionTimeout()
{
	int oldLoginState = m_iLoginState;

	// set our status to be offline
	OnUserChangeStatus(COnlineStatus::OFFLINE);

	// send a message tell us to try reconnect in X seconds
	m_iLoginState = LOGINSTATE_WAITINGTORECONNECT;
	m_iReconnectTime = m_iTime + COnlineStatus::RECONNECT_TIME;
	m_iLoginTimeout = 0;

	if (oldLoginState == LOGINSTATE_AWAITINGCHALLENGE || oldLoginState == LOGINSTATE_AWAITINGLOGINSUCCESS)
	{
		// login failed

		// mark us as falling back to a different server
		if (FallbackToAlternateServer())
		{
			// try reconnect almost immediately
			m_iReconnectTime = system()->GetTimeMillis() + 100;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the state to get the user to their desired status
//-----------------------------------------------------------------------------
void CServerSession::OnUserChangeStatus(int requestedStatus)
{
	m_iUID = ::GetDoc()->GetUserID();
	if (!m_iUID)
		return;

	// make sure the requested status is valid
	if (requestedStatus == COnlineStatus::INGAME && !m_iGameIP)
	{
		requestedStatus = COnlineStatus::ONLINE;
	}

	unsigned int timeMillis = system()->GetTimeMillis();
	int currentStatus = GetStatus();

	switch (m_iLoginState)
	{
	case LOGINSTATE_WAITINGTORECONNECT:
		if (requestedStatus == COnlineStatus::OFFLINE)
		{
			CancelConnect();
			break;
		}
		// fallthrough to below

	case LOGINSTATE_DISCONNECTED:
		if (requestedStatus >= COnlineStatus::ONLINE)
		{
			// initiate login sequence
			SendInitialLogin(requestedStatus);
		}
		break;

	case LOGINSTATE_AWAITINGCHALLENGE:
	case LOGINSTATE_AWAITINGLOGINSUCCESS:
		if (requestedStatus >= COnlineStatus::ONLINE)
		{
			// we're still logging in, so save off the status request
			m_iDesiredStatus = requestedStatus;
		}
		else
		{
			// user wants to break connection
			m_iLoginState = LOGINSTATE_DISCONNECTED;
		}
		// notify the dialog that we are offline
		CTrackerDialog::GetInstance()->PostMessage(CTrackerDialog::GetInstance(), new KeyValues("OnlineStatus", "value", COnlineStatus::OFFLINE));

		break;

	case LOGINSTATE_CONNECTED:
		assert(m_iStatus > COnlineStatus::OFFLINE);

		// update the status to the server
		SendStatusToServer(requestedStatus);

		if (requestedStatus < COnlineStatus::ONLINE)
		{
			// user has requested to logout
			m_iLoginState = LOGINSTATE_DISCONNECTED;
			m_iLoginTimeout = 0;
			m_iReconnectTime = 0;
		}

		// adopt the new status
		m_iStatus = requestedStatus;

		// notify the dialog that we've changed
		CTrackerDialog::GetInstance()->PostMessage(CTrackerDialog::GetInstance(), new KeyValues("OnlineStatus", "value", requestedStatus));
		break;

	default:
		assert(!("Invalid login state"));
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: responds to the login challenge
//-----------------------------------------------------------------------------
void CServerSession::OnReceivedChallenge()
{
	// respond to the challenge
	ISendMessage *reply = CreateServerMessage(TCLS_RESPONSE);
	reply->SetSessionID( m_iSessionID );
	int status = ::GetDoc()->Data()->GetInt("Login/DesiredStatus", 1);
	if (status < COnlineStatus::ONLINE)
	{
		status = COnlineStatus::ONLINE;
	}

	int heartbeatRate = GetHeartBeatRate();

	reply->WriteInt("challenge", m_iChallengeKey);
	reply->WriteUInt("sessionID", m_iSessionID);
	reply->WriteInt("status", status);
	reply->WriteInt("build", m_iBuildNum);
	reply->WriteInt("hrate", heartbeatRate);	// heartbeat rate to expect

	m_iPreviousHeartBeatRateSentToServer = heartbeatRate;

	// reset the login timeout
	m_iLoginTimeout = system()->GetTimeMillis() + COnlineStatus::SERVERCONNECT_TIMEOUT;

	WriteToLog("Sent: Response()\n");
	m_pNet->SendMessage(reply, NET_RELIABLE);

	// next step is to wait for a success message
	m_iLoginState = LOGINSTATE_AWAITINGLOGINSUCCESS;
}

//-----------------------------------------------------------------------------
// Purpose: moves the user to full connected state
//-----------------------------------------------------------------------------
void CServerSession::OnReceivedLoginSuccess(int newStatus)
{
	m_iLoginState = LOGINSTATE_CONNECTED;

	// if we're logged on we must have at least an online status
	if (newStatus < COnlineStatus::ONLINE)
	{
		newStatus = COnlineStatus::ONLINE;
	}

	WriteToLog("Received: LoginOK(status: %d)\n", newStatus);

	// record it in the doc
	m_iStatus = newStatus;
	::GetDoc()->Data()->SetInt("App/Server", m_iCurrentServer);
	m_iReconnectRetries = 0;

	// put into logged-in state
	KeyValues *res = new KeyValues("OnlineStatus");
	res->SetInt("value", newStatus);
	CTrackerDialog::GetInstance()->PostMessage(CTrackerDialog::GetInstance(), res);
}

//-----------------------------------------------------------------------------
// Purpose: moves the user into a disconnect state for a certain time
//-----------------------------------------------------------------------------
void CServerSession::OnReceivedDisconnectMessage(int minDisconnectTime, int maxDisconnectTime)
{
	// ignore the disconnect message if we're already offline
	if (m_iLoginState == LOGINSTATE_DISCONNECTED)
		return;

	WriteToLog("Received: Disconnected message (%d, %d)\n", minDisconnectTime, maxDisconnectTime);

	// record the previous status for when we reconnect
	int status = GetStatus();
	if (status < COnlineStatus::ONLINE)
	{
		status = COnlineStatus::ONLINE;
	}
	::GetDoc()->Data()->SetInt("Login/DesiredStatus", status);
	
	// a maximum disconnect time of 0 indicates we should not try and reconnect
	bool stayDisconnected = (maxDisconnectTime < 1);

	// go offline
	OnUserChangeStatus(COnlineStatus::OFFLINE);

	// change what server we're using (since it's probably only one server that's gone down)
	FallbackToAlternateServer();

	if (stayDisconnected)
	{
		// don't try and reconnect
		// probably being kicked offline by the same login from a different pc
		m_iLoginState = LOGINSTATE_DISCONNECTED;
	}
	else
	{
		// reconnect at a random time in the range
		// server has probably gone down and doesn't want everyone to reconnect at the same time
		m_iLoginState = LOGINSTATE_WAITINGTORECONNECT;
		int disconnectTime = RandomLong(minDisconnectTime * 1000, maxDisconnectTime * 1000);
		m_iReconnectTime = system()->GetTimeMillis() + disconnectTime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: moves the user to reconnecting state
//-----------------------------------------------------------------------------
void CServerSession::OnLoginFail(int failReason)
{
	WriteToLog("Received: LoginFail(reason: %d)\n", failReason);

	switch (m_iLoginState)
	{
	case LOGINSTATE_DISCONNECTED:
		// user must have specified to stay disconnected, just ignore
		return;

	case LOGINSTATE_CONNECTED:
		assert(!("Received LoginFail message when already connected"));
		// fall through to reconnecting

	case LOGINSTATE_WAITINGTORECONNECT:
	case LOGINSTATE_AWAITINGCHALLENGE:
	case LOGINSTATE_AWAITINGLOGINSUCCESS:
	default:
		// disconnect, reset the time at which we should reconnect
		m_iLoginState = LOGINSTATE_WAITINGTORECONNECT;

		// notify main panel, but only if we get a valid reason
		if (failReason == -3)
		{
			// we're already logged in elsewhere
			// just set the reconnect time to a bit more rapid than normal and try again in 2 seconds
			// server will have requested the other Client to logout
			// if other Client cannot respond, will have to wait for them to be timed out (up to 20 seconds)
			m_iReconnectRetries = min(10, m_iReconnectRetries + 1);
			m_iReconnectTime = system()->GetTimeMillis() + (2000 * m_iReconnectRetries);
			m_iLoginTimeout = 0;
		}
		else if (failReason < 0)
		{
			// don't try and relogin
			m_iLoginState = LOGINSTATE_DISCONNECTED;
			m_iReconnectTime = 0;
		}
		else
		{
			// server has had login fail for no reason, so just ignore the message so that we try another server
		}

		break;
	}

	// set us as offline
	m_iStatus = COnlineStatus::OFFLINE;
}

//-----------------------------------------------------------------------------
// Purpose: checks any timeouts to see if the connection state should change
//-----------------------------------------------------------------------------
void CServerSession::CheckForConnectionStateChange()
{
	int status = GetStatus();

	switch (m_iLoginState)
	{
	case LOGINSTATE_CONNECTED:
		assert(status > 0);

		// check for auto-switch status
		if (m_iDesiredStatus > 0)
		{
			OnUserChangeStatus(m_iDesiredStatus);
			m_iDesiredStatus = 0;
		}

		// we're online, so check to see if we need to send a heartbeat
		if ((m_iLastHeartbeat + m_iPreviousHeartBeatRateSentToServer) < system()->GetTimeMillis())
		{
			SendStatusToServer(status);
		}
		break;

	case LOGINSTATE_AWAITINGCHALLENGE:
	case LOGINSTATE_AWAITINGLOGINSUCCESS:
		// we're in the middle of the connection process
		// check for connection timeout
		if (m_iLoginTimeout < system()->GetTimeMillis())
		{
			OnConnectionTimeout();
		}
	
		break;

	case LOGINSTATE_WAITINGTORECONNECT:
		// check for reconnection
		//assert(m_iReconnectTime);
		assert(status < 1);

		if (m_iReconnectTime < system()->GetTimeMillis())
		{
			// initiate reconnect
			int desiredStatus = GetDoc()->Data()->GetInt("Login/DesiredStatus", COnlineStatus::ONLINE);
			if (desiredStatus < COnlineStatus::ONLINE)
			{
				desiredStatus = COnlineStatus::ONLINE;
			}

			OnUserChangeStatus(desiredStatus);
			m_iReconnectTime = 0;
		}
		break;

	case LOGINSTATE_DISCONNECTED:
	default:
		// nothing could possibly change here
		break;
	}
}
