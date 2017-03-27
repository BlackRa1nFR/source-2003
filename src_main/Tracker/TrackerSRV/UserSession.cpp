//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "UserSession.h"
#include "Random.h"
#include "SessionManager.h"
#include "TrackerDatabaseManager.h"
#include "../public/ITrackerUserDatabase.h"

#include "../TrackerNET/TrackerNET_Interface.h"
#include "../TrackerNET/ReceiveMessage.h"
#include "../TrackerNET/SendMessage.h"
#include "DebugConsole_Interface.h"
#include "TrackerMessageFlags.h"
#include "TrackerProtocol.h"
#include "UtlMsgBuffer.h"
#include "TopologyManager.h"
#include "MemPool.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

extern IDebugConsole *g_pConsole;

#define ARRAYSIZE(p)	(sizeof(p)/sizeof(p[0]))

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

static const int STATUS_AWAY = 3;
static const int STATUS_INGAME = 4;
static const int STATUS_SNOOZE = 5;
static const int STATUS_OFFLINE = 0;

void v_strncpy(char *dest, const char *src, int bufsize);

// minimum build number of Client to be allowed to connect to server
static const int MINIMUM_BUILD_NUMBER = 1957;

// build numbers used for compatibility with old clients
static const int COMPATILIBITY_SERVERID_SUPPORT_VERSION_MIN = 1920;

// the number of seconds after missed heartbeat the user will be disconnected
static const float USER_TIMEOUT_SLOP = 30.0f;

// the minimum duration a firewall must be open to be an acceptable Client
static const float MINIMUM_FIRWALL_WINDOW_DURATION = 15.0f;

enum EAuthLevels
{
	AUTHLEVEL_NONE = 0,
	AUTHLEVEL_REQUESTINGAUTH = 3,
	AUTHLEVEL_FULLAUTH = 10,
};
//-----------------------------------------------------------------------------
// Purpose: State list
//-----------------------------------------------------------------------------
enum UserState_t
{
	STATE_NORMAL,
	STATE_REQUESTINGAUTH,
	STATE_GETFRIENDINFO,
	STATE_SENDINGSTATUS,
	STATE_EXCHANGESTATUS,
	STATE_DISCONNECTINGUSER,
	STATE_ADDWATCH,
	STATE_CHECKMESSAGES,
	STATE_SENDINGOFFLINESTATUS,
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CUserSession::CUserSession()
{
	m_iSessionID = 0;
	m_pMsgQueueFirst = NULL;
	m_pMsgQueueLast = NULL;

	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CUserSession::~CUserSession()
{
}

//-----------------------------------------------------------------------------
// Purpose: Clears the session between user connections
//-----------------------------------------------------------------------------
void CUserSession::Clear()
{ 
	m_iUserID = 0; 
	m_iStatus = 0;
	m_iUserSessionState = USERSESSION_FREE;
	m_pDB = NULL;
	m_iBuildNum = 0;
	m_iGameIP = 0;
	m_iGamePort = 0;
	m_szGameType[0] = 0;
	m_szPassword[0] = 0;
	m_szUserName[0] = 0;
	m_flLastAccess = 0.0f;
	m_flFirewallWindow = 9999999.0f;
	m_bUpdateFirewallWindowToClient = false;
	m_flTimeoutDelay = 0.0f;

	FlushMessageQueue();
}

//-----------------------------------------------------------------------------
// Purpose: Wakes up the main thread
//-----------------------------------------------------------------------------
void CUserSession::WakeUp()
{
	sessionmanager()->WakeUp();
}

//-----------------------------------------------------------------------------
// Purpose: called after a succesful login
//-----------------------------------------------------------------------------
void CUserSession::PostLogin()
{
	g_pConsole->Print(4, "Logged in: %d\n", m_iUserID);

	// set watches on all the servers that our friend resides
	// get a list of all the friends we need to watch - this will initiate setting watches
	m_pDB->User_GetFriendList(this, m_iUserID);

	// tell other users that the friend has logged on
	UpdateStatusToFriends();
	
	// check to see if we have any messages waiting
	CheckForMessages();
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the current user session should be checked for timeout
//-----------------------------------------------------------------------------
bool CUserSession::CheckForTimeout(float currentTime)
{
	if (m_iUserSessionState == USERSESSION_ACTIVE)
	{
		if ((m_flLastAccess + m_flTimeoutDelay) < currentTime && m_iStatus > 0)
		{
			// time elapsed between received notification from user too long, log them off
			Logoff(true, true, false);
			return true;
		}
	}
	else
	{
		float timeoutTime = 0.0f;
		if (m_iUserSessionState == USERSESSION_CONNECTING)
		{
			// timeout a connection after 20 seconds
			timeoutTime = 20.0f;
		}
		/* unnecessary:
		else if (m_iUserSessionState == USERSESSION_CONNECTFAILED)
		{
			timeoutTime = 0.0f;
		}
		*/

		if (m_iUserSessionState == USERSESSION_FREE)
		{
			g_pConsole->Print(8, "!! Timing out already freed session\n");
		}

		if ((m_flLastAccess + timeoutTime) < currentTime)
		{
			// kill the user, no need to log them off since they were never logged in
			sessionmanager()->FreeUserSession(this);
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Creates a message to be sent to the current user
//-----------------------------------------------------------------------------
ISendMessage *CUserSession::CreateUserMessage(int msgID)
{
	ISendMessage *msg = net()->CreateMessage(msgID);
	msg->SetSessionID(m_iSessionID);
	msg->SetNetAddress(m_NetAddress);
	msg->SetEncrypted(true);

	return msg;
}

// message dispatch table
typedef void (CUserSession::*funcPtr)( IReceiveMessage * );
struct UserMsgDispatch_t
{
	int msgName;
	funcPtr msgFunc;
};

// map of networking messages -> functions
// ordered from most called to least called
static UserMsgDispatch_t g_UserMsgDispatch[] =
{
	{ TCLS_HEARTBEAT, CUserSession::ReceivedMsg_Heartbeat },
	{ TCLS_ROUTETOFRIEND, CUserSession::ReceivedMsg_RouteToFriend },
	{ TCLS_MESSAGE, CUserSession::ReceivedMsg_Message },
	{ TCLS_RESPONSE, CUserSession::ReceivedMsg_Response },
	{ TCLS_FRIENDINFO, CUserSession::ReceivedMsg_FriendInfo },
	{ TCLS_AUTHUSER, CUserSession::ReceivedMsg_AuthUser },
	{ TCLS_REQAUTH, CUserSession::ReceivedMsg_ReqAuth },
	{ TCLS_FRIENDSEARCH, CUserSession::ReceivedMsg_FindUser },
	{ TCLS_SETINFO, CUserSession::ReceivedMsg_SetInfo },
};


// SQL DB Reply table
typedef void (CUserSession::*dbFuncPtr)(int , void *);
struct DBReplyMapItem_t
{
	int cmdID;
	int returnState;
	dbFuncPtr msgFunc;
};

static DBReplyMapItem_t g_DBMsgDispatch[] =
{
	{ CMD_LOGIN,			STATE_NORMAL,			CUserSession::DBMsg_Login },
	{ CMD_LOGOFF,			STATE_NORMAL,			CUserSession::DBMsg_Logoff },
	{ CMD_GETSESSIONINFO,	STATE_SENDINGSTATUS,	CUserSession::DBMsg_GetSessionInfo_SendingStatus },
	{ CMD_GETSESSIONINFO,	STATE_EXCHANGESTATUS,	CUserSession::DBMsg_GetSessionInfo_ExchangeStatus },
	{ CMD_GETSESSIONINFO,	STATE_SENDINGOFFLINESTATUS,	CUserSession::DBMsg_GetSessionInfo_SendingOfflineStatus },
	{ CMD_GETSESSIONINFO,	STATE_ADDWATCH,			CUserSession::DBMsg_GetSessionInfo_AddWatch },
	{ CMD_GETSESSIONINFO,	STATE_DISCONNECTINGUSER,CUserSession::DBMsg_GetSessionInfo_DisconnectingUser },
	{ CMD_GETSESSIONINFO,	STATE_CHECKMESSAGES,	CUserSession::DBMsg_GetSessionInfo_CheckMessages },
	{ CMD_FINDUSERS,		STATE_NORMAL,			CUserSession::DBMsg_FindUsers },
	{ CMD_GETINFO,			STATE_NORMAL,			CUserSession::DBMsg_GetInfo },
	{ CMD_ISAUTHED,			STATE_GETFRIENDINFO,	CUserSession::DBMsg_GetFriendInfo_IsAuthed },
	{ CMD_ISAUTHED,			STATE_REQUESTINGAUTH,	CUserSession::DBMsg_RequestAuth_IsAuthed },
	{ CMD_GETWATCHERS,		STATE_NORMAL,			CUserSession::DBMsg_GetWatchers },
	{ CMD_GETFRIENDLIST,	STATE_NORMAL,			CUserSession::DBMsg_GetFriendList },
	{ CMD_GETFRIENDSTATUS,	STATE_NORMAL,			CUserSession::DBMsg_GetFriendStatus },
	{ CMD_GETFRIENDSGAMESTATUS, STATE_NORMAL,		CUserSession::DBMsg_GetFriendsGameStatus },
	{ CMD_GETMESSAGE,		STATE_NORMAL,			CUserSession::DBMsg_GetMessage },
	{ CMD_DELETEMESSAGE,	STATE_NORMAL,			CUserSession::DBMsg_DeleteMessage },
};

//-----------------------------------------------------------------------------
// Purpose: Handles a user session receiving a network message
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMessage(IReceiveMessage *userMsg, float currentTime)
{
	// mark the new access
	m_flLastAccess = currentTime;

	// update the user port
	if (userMsg->NetAddress().Port() != m_NetAddress.Port())
	{
		g_pConsole->Print(9, "++++ User port changed, %s -> %d\n", m_NetAddress.ToStaticString(), userMsg->NetAddress().Port());
		m_NetAddress.SetPort(userMsg->NetAddress().Port());
	}

	// send all the queued messages
	SendQueuedMessages();

	// simple dispatch
	// loop through the array and find the right message
	int arraySize = ARRAYSIZE(g_UserMsgDispatch);
	int msgName = userMsg->GetMsgID();
	for (int i = 0; i < arraySize; i++)
	{
		if (msgName == g_UserMsgDispatch[i].msgName)
		{
			(this->*g_UserMsgDispatch[i].msgFunc)( userMsg );
			return;
		}
	}

	// invalid message
	g_pConsole->Print(5, "* Unhandled message '%d'\n", msgName);
}

//-----------------------------------------------------------------------------
// Purpose: Increments the sessionID stamp
// Input  : baseSessionID - basic session id that occupies the top 16 bits
//			of the sessionID number
//-----------------------------------------------------------------------------
void CUserSession::CreateNewSessionID(int baseSessionID)
{
	// mask off the base id
	unsigned int numericID = (m_iSessionID & 0x0000FFFF);

	// incremement
	numericID++;

	// mask again (to remove any overflow)
	numericID &= 0x0000FFFF;

	// reset the base id
	m_iSessionID = numericID | (baseSessionID << 16);
}


//-----------------------------------------------------------------------------
// LOGGING INTO A SESSION
/*
	Client -> sends a message requesting a log in
			+ 'login'
				- 'uid'
				- 'password'
				- 'email'

	server -> allocates an new session
	       -> generates new sessionID and random challenge key
		   -> sends Client a reply challenging them to respond
			+ 'challenge'
				- 'sessionID'
				- 'challenge key'

	Client -> replies to the challenge
			+ 'response'
				- 'sessionID'
				- 'challenge key'

	server -> validates the player in the database to see if they exist
	       -> sends back a login ack if successful
			+ 'LoginOK'
				- 'sessionID'
		   -> sends status of friends to Client
		   -> sends Client status to friends
	
	Client -> enters logged in mode
		   -> displays friends status & messages

*/
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Attempts to log a user into the server
//-----------------------------------------------------------------------------
bool CUserSession::ReceivedMsg_Login(IReceiveMessage *loginMsg)
{
	unsigned int userID;
	char szEmail[128];
	char szPassword[32];
	int iStatus;
	int iFirewallWindow = 0;

	loginMsg->ReadUInt("uid", userID);
	loginMsg->ReadString("email", szEmail, sizeof(szEmail));
	loginMsg->ReadString("password", szPassword, sizeof(szPassword));
	loginMsg->ReadInt("status", iStatus);
	loginMsg->ReadInt("FirewallWindow", iFirewallWindow);

	if (userID < 1)
	{
		g_pConsole->Print(4, "User (%s) with bad userID %d attempted to log into server\n", loginMsg->NetAddress().ToStaticString(), userID);
		return false;
	}

	// get our database handle
	m_pDB = g_pDataManager->TrackerUser(userID);

	// make sure this is a valid userID
	if (!m_pDB)
	{
		// bad user ID
		g_pConsole->Print(4, "User (%s) with bad userID %d attempted to log into server\n", loginMsg->NetAddress().ToStaticString(), m_iUserID);
		return false;
	}

	// if this user is already logged in to the server from this IP, override previous session
	CUserSession *oldUserSession = sessionmanager()->GetUserSessionByID(userID);
	if (oldUserSession)
	{
		if (oldUserSession->IP() == loginMsg->NetAddress().IP() || oldUserSession->State() != USERSESSION_ACTIVE)
		{
			// same IP address, simply kill the previous session and continue
			g_pConsole->Print(0, "Forcing off previous instance of user '%d'\n", m_iUserID);
			sessionmanager()->ForceDisconnectUser(userID);
		}
		else if (oldUserSession->State() == USERSESSION_ACTIVE)
		{
			// begin logging off the old user, quit ourselves
			oldUserSession->Logoff(true, false, true);
			return false;
		}
	}

	// save off data
	m_iUserID = userID;
	m_iStatus = iStatus;
	v_strncpy(m_szPassword, szPassword, sizeof(m_szPassword));
	m_iUserSessionState = USERSESSION_CONNECTING;
	m_NetAddress = loginMsg->NetAddress();
	m_flTimeoutDelay = (6 * 60.0f);	// 6 minute timeout by default
	if (iFirewallWindow)
	{
		m_flFirewallWindow = (float)iFirewallWindow;
	}
	else
	{
		// no firewall info sent, mark it as wide as possible
		m_flFirewallWindow = 999999.0f;
	}

	// set the current access time
	m_flLastAccess = sessionmanager()->GetCurrentTime();

	// build a reply
	ISendMessage *replyMsg = net()->CreateReply(TSVC_CHALLENGE, loginMsg );
	replyMsg->SetEncrypted(true);

	// session ID is 0 for this message, indicating a non-validated packet
	replyMsg->SetSessionID( 0 );

	// generate a random challenge key (that the Client will have to respond with correctly)
	m_iChallengeKey = RandomLong(1, MAX_RANDOM_RANGE);

	// write out the reply
	replyMsg->WriteUInt("sessionID", m_iSessionID);
	replyMsg->WriteInt("challenge", m_iChallengeKey);

	// this could be changed to unreliable for optimization
	SendMessage(replyMsg, NET_RELIABLE /*NET_UNRELIABLE*/);

//	g_pConsole->Print(4, "Sending challenge for login request from '%s'\n", szEmail);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Response to a login challenge
//			If this response to the challenge is correct, then fully log the user
//			into the database
//			Invalid 'response' messages receive no reply (since it's then a security problem)
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_Response(IReceiveMessage *pMsg)
{
	unsigned int msgSessionID = pMsg->SessionID();
	unsigned int sessionID; 
	int challenge, status, buildNum, heartBeatRateMillis;
	pMsg->ReadInt("challenge", challenge);
	pMsg->ReadUInt("sessionID", sessionID);
	pMsg->ReadInt("status", status);
	pMsg->ReadInt("build", buildNum);
	pMsg->ReadInt("hrate", heartBeatRateMillis);
	
//	g_pConsole->Print(4, "Received Response from user (%d) sessionID (%d)\n", m_iUserID, m_iSessionID);

	// Validate
	if (msgSessionID != sessionID || m_iChallengeKey != challenge)
	{
		g_pConsole->Print(6, "Response ignored, invalid challenge or sessionID\n");
		m_iUserSessionState = USERSESSION_CONNECTFAILED;
		sessionmanager()->FreeUserSession(this);
		return;
	}

	// check build number
	if (buildNum < MINIMUM_BUILD_NUMBER)
	{
		// send login failed message
		m_iStatus = 0;

		ISendMessage *reply = net()->CreateReply(TSVC_LOGINFAIL, pMsg);
		reply->WriteInt("reason", -4);
		SendMessage(reply, NET_RELIABLE);
		m_iUserSessionState = USERSESSION_CONNECTFAILED;
		sessionmanager()->FreeUserSession(this);
		return;
	}

	m_iUserSessionState = USERSESSION_ACTIVE;

	m_iBuildNum = buildNum;

	if (status < 1)
	{
		status = 1;
	}

	// record heartbeat rate
	m_flTimeoutDelay = (heartBeatRateMillis / 1000.0f) + USER_TIMEOUT_SLOP;

	// try and login
	m_pDB->User_Login(this, m_iUserID, m_szPassword, m_NetAddress.IP(), m_NetAddress.Port(), sessionID, status, buildNum);

	// now wait on reply, handled in SQLDBResponse()
}

//-----------------------------------------------------------------------------
// Purpose: Heartbeat message received from user
//			Performs keepalive actions for the user
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_Heartbeat(IReceiveMessage *pMsg)
{
	int status;
	pMsg->ReadInt("status", status);

	g_pConsole->Print(1, "Heartbeat userid: %d  status: %d\n", m_iUserID, status);

	// check to see if we have any game info
	bool updateGameInfo = false;
	bool updateStatusInfo = false;

	if (status != m_iStatus)
	{
		m_iStatus = status;
		updateStatusInfo = true;
	}

	if (status == STATUS_INGAME)
	{
		unsigned int ip, port;
		pMsg->ReadUInt("GameIP", ip);
		pMsg->ReadUInt("GamePort", port);

		if (ip != m_iGameIP && port != m_iGamePort)
		{
			updateGameInfo = true;
			pMsg->ReadString("Game", m_szGameType, sizeof(m_szGameType));
			m_iGameIP = ip;
			m_iGamePort = port;
		}
	}
	else
	{
		// clear the game info if they're not in a game
		m_iGameIP = 0;
		m_iGamePort = 0;
		m_szGameType[0] = 0;
	}

	// check for heartbeat rate update
	int heartBeatRateMillis;
	if (pMsg->ReadInt("hrate", heartBeatRateMillis))
	{
		m_flTimeoutDelay = (heartBeatRateMillis / 1000.0f) + USER_TIMEOUT_SLOP;
	}

	if (updateGameInfo || updateStatusInfo)
	{
		// 0 status is a log off message
		if (m_iStatus < 1)
		{
			// log the user off the server
			Logoff(false, false, false);
			return;
		}
		else
		{
			// update the status in the DB
			m_pDB->User_Update(m_iUserID, m_iStatus, m_iGameIP, m_iGamePort, m_szGameType);

			// tell the new status to friends
			UpdateStatusToFriends();
		}
	}

	// see if we need to update their status
	if (m_bUpdateFirewallWindowToClient)
	{
		// send them a heartbeat with their new firewall window
		ISendMessage *msg = CreateUserMessage(TSVC_HEARTBEAT);
		msg->WriteInt("FirewallWindow", (int)m_flFirewallWindow);
	}

	// check for messages
	CheckForMessages();
}

//-----------------------------------------------------------------------------
// Purpose: Generic message routing to a friend
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_RouteToFriend(IReceiveMessage *dataBlock)
{
	// parse out details
	int msgID;
	unsigned int userID, sessionID, serverID = 0;
	dataBlock->ReadInt("rID", msgID);
	dataBlock->ReadUInt("rUserID", userID);
	dataBlock->ReadUInt("rSessionID", sessionID);
	dataBlock->ReadUInt("rServerID", serverID);

	// parse out the message to be routed
	char buf[1300];
	int bytesRead = dataBlock->ReadBlob("rData", buf, sizeof(buf));

	// route the message through a different server
	topology()->SendNetworkMessageToUser(userID, sessionID, serverID, msgID, buf, bytesRead);
}

//-----------------------------------------------------------------------------
// Purpose: REDUNDANT: Message from user to be redirected to a friend
//			this function has been replaced by RouteToFriend and should be deleted
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_Message(IReceiveMessage *pMsg)
{
	// parse out redirection info
	unsigned int fsessionID, fip, fport;
	pMsg->ReadUInt("FSID", fsessionID);
	pMsg->ReadUInt("FIP", fip);
	pMsg->ReadUInt("FPort", fport);

	// parse out message
	unsigned int userID, targetID;
	int status, chatID;
	char text[512], userName[32];
	pMsg->ReadUInt("uid", userID);

	// check userID match
	if (userID != m_iUserID)
		return;

	pMsg->ReadUInt("targetID", targetID);
	pMsg->ReadString("UserName", userName, sizeof(userName));
	pMsg->ReadInt("status", status);
	pMsg->ReadString("Text", text, sizeof(text));
	pMsg->ReadInt("chatID", chatID);

	// write out redirected message
	ISendMessage *msg = net()->CreateMessage(TSVC_MESSAGE);
	CNetAddress addr;
	addr.SetIP(fip);
	addr.SetPort(fport);
	msg->SetNetAddress(addr);
	msg->SetEncrypted(true);
	msg->SetSessionID(fsessionID);
	
	msg->WriteUInt("uid", m_iUserID);
	msg->WriteUInt("targetID", targetID);
	msg->WriteString("UserName", userName);
	msg->WriteInt("status", status);
	msg->WriteString("Text", text);
	if (chatID)
	{
		msg->WriteInt("chatID", chatID);
	}
 
	SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Disconnects the user from the server
//-----------------------------------------------------------------------------
void CUserSession::Logoff(bool forced, bool fastReconnect, bool permanentDisconnect)
{
	if (!m_iUserID || !m_pDB)
		return;

	m_iStatus = 0;

	if (!forced)
	{
		g_pConsole->Print(4, "Logging off: %d (user requested)\n", m_iUserID);
	}
	else
	{
		if (permanentDisconnect)
		{
			g_pConsole->Print(4, "Logging off: %d (overriding login)\n", m_iUserID);
		}
		else if (fastReconnect)
		{
			g_pConsole->Print(4, "Logging off: %d (timed out)\n", m_iUserID);
		}
		else
		{
			g_pConsole->Print(4, "Logging off: %d (range lock or flush)\n", m_iUserID);
		}
	}

	// then log them off
	m_pDB->User_Logoff(this, m_iUserID, false);

	if (forced)
	{
		// send disconnect message to user
		ISendMessage *msg = CreateUserMessage(TSVC_DISCONNECT);

		// tell user to reconnect at a random time within the range (in seconds)
		if (permanentDisconnect)
		{
			msg->WriteInt("minTime", 0);
			msg->WriteInt("maxTime", 0);
			msg->WriteInt("logoffMsg", 1);
		}
		else if (fastReconnect)
		{
			msg->WriteInt("minTime", 1);
			msg->WriteInt("maxTime", 5);
		}
		else
		{
			msg->WriteInt("minTime", 5);
			msg->WriteInt("maxTime", 15);
		}

		// just send the message (don't go through the normal send path)
		net()->SendMessage(msg, NET_RELIABLE);
	}

	// empty the message queue
	FlushMessageQueue();

	// wait for result to return
}

//-----------------------------------------------------------------------------
// Purpose: Sends the users' current status to all their watchers
//-----------------------------------------------------------------------------
void CUserSession::UpdateStatusToFriends()
{
	m_pDB->User_GetWatchers(this, m_iUserID);
}

//-----------------------------------------------------------------------------
// Purpose: Checks for and sends the user and messages they having waiting for them
//-----------------------------------------------------------------------------
void CUserSession::CheckForMessages()
{
	// look to see if we have any messages
	m_pDB->User_GetMessage(this, m_iUserID);
}

//-----------------------------------------------------------------------------
// Purpose: Searches the database for the set of users that match the criteria
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_FindUser(IReceiveMessage *msg)
{
	// the message contains search criteria to find a friend
	unsigned int iUID;
	char szEmail[128];
	char szUserName[32];
	char szFirstName[32];
	char szLastName[32];

	msg->ReadUInt("uid", iUID);
	msg->ReadString("Email", szEmail, 128);
	msg->ReadString("UserName", szUserName, 32);
	msg->ReadString("FirstName", szFirstName, 32);
	msg->ReadString("LastName", szLastName, 32);

	// search for the users
	if (iUID)
	{
		// only one user
		g_pDataManager->TrackerUser(iUID)->Find_Users(this, iUID, szEmail, szUserName, szFirstName, szLastName);
	}
	else
	{
		// look in each database
		for (int i = 0; i < g_pDataManager->TrackerUserCount(); i++)
		{
			g_pDataManager->TrackerUserByIndex(i).db->Find_Users(this, iUID, szEmail, szUserName, szFirstName, szLastName);
		}
	}

	// await replies from db
}

//-----------------------------------------------------------------------------
// Purpose: Authorizes a user
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_AuthUser(IReceiveMessage *msg)
{
	unsigned int iUID;
	int auth;

	msg->ReadUInt("targetID", iUID);
	msg->ReadInt("auth", auth);

	if (auth)
	{
		// Run the auth user DB command
		AuthorizeUser(iUID);
	}
	else
	{
		// it's a deny message, break the link between users
		UnauthUser(iUID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: requesting authorization from a user
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_ReqAuth(IReceiveMessage *msg)
{
	unsigned int friendID;
	char szRequestReason[128];

	msg->ReadUInt("uid", friendID);
	msg->ReadString("ReqReason", szRequestReason, 128);

	// remove any block we have on that user
	m_pDB->User_SetBlock(m_iUserID, friendID, false);

	// get our current authorization state
	m_pDB->User_IsAuthed(this, STATE_REQUESTINGAUTH, m_iUserID, friendID, szRequestReason);
}

//-----------------------------------------------------------------------------
// Purpose: User is requesting an updated friends list
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_ReqFriends(IReceiveMessage *reqAuthMsg)
{
	// this is not supported, since the user should have always received the correct set of stati
}

//-----------------------------------------------------------------------------
// Purpose: Returns information about a single user to a user, but only
//			if they're authorized to see
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_FriendInfo(IReceiveMessage *reqFriendInfoMsg)
{
	unsigned int iUID;
	reqFriendInfoMsg->ReadUInt("uid", iUID);

	// check to see if we're authorized to get that info
	// always authorized to see our own info
	m_pDB->User_IsAuthed(this, STATE_GETFRIENDINFO, m_iUserID, iUID, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Updates the information regarding a user
//-----------------------------------------------------------------------------
void CUserSession::ReceivedMsg_SetInfo(IReceiveMessage *msg)
{
	unsigned int iUID;
	char szUser[32];
	char szFirst[32];
	char szLast[32];

	msg->ReadUInt("uid", iUID);
	msg->ReadString("UserName", szUser, 32);
	msg->ReadString("FirstName", szFirst, 32);
	msg->ReadString("LastName", szLast, 32);

	if (iUID != m_iUserID)
		return;

	// update information in database
	m_pDB->User_SetInfo(m_iUserID, szUser, szFirst, szLast);
}

//-----------------------------------------------------------------------------
// Purpose: Authorizes one use to be able to get the others IP address
//-----------------------------------------------------------------------------
void CUserSession::AuthorizeUser(unsigned int targetID)
{
	// remove any message blocks
	m_pDB->User_SetBlock(m_iUserID, targetID, false);

	// set up the two way link
	m_pDB->User_SetAuth(targetID, m_iUserID, AUTHLEVEL_FULLAUTH);
	m_pDB->User_SetAuth(m_iUserID, targetID, AUTHLEVEL_FULLAUTH);
	
	// add the link on both servers
	ITrackerUserDatabase *targetDB = g_pDataManager->TrackerUser(targetID);
	if (!targetDB)
		return;

	if (targetDB != m_pDB)
	{
		targetDB->User_SetAuth(targetID, m_iUserID, AUTHLEVEL_FULLAUTH);
		targetDB->User_SetAuth(m_iUserID, targetID, AUTHLEVEL_FULLAUTH);
	}

	// set the user up to watch us
	targetDB->User_GetSessionInfo(this, STATE_ADDWATCH, targetID);
	// put our watch on them
	targetDB->User_AddSingleWatch(m_iUserID, m_iSessionID, topology()->GetServerID(), targetID);

	// post a message to the user and have them check their messages right away
	PostMessageToUser(m_iUserID, targetID, "Authed", MESSAGEFLAG_AUTHED);
	targetDB->User_GetSessionInfo(this, STATE_CHECKMESSAGES, targetID);

	// send status both ways
	targetDB->User_GetSessionInfo(this, STATE_EXCHANGESTATUS, targetID);
}

//-----------------------------------------------------------------------------
// Purpose: Breaks the authorization links between two users, and blocks the users from rerequesting status
//-----------------------------------------------------------------------------
void CUserSession::UnauthUser(unsigned int targetID)
{
	// remove auth link
	// watchmaps are automatically modified
	m_pDB->User_RemoveAuth(targetID, m_iUserID);
	// remove the link on both servers if different
	if (g_pDataManager->TrackerUser(targetID) != m_pDB)
	{
		g_pDataManager->TrackerUser(targetID)->User_RemoveAuth(targetID, m_iUserID);
	}

	m_pDB->User_SetBlock(m_iUserID, targetID, true);

	// tell the user that we've gone offline
	g_pDataManager->TrackerUser(targetID)->User_GetSessionInfo(this, STATE_SENDINGOFFLINESTATUS, targetID);
}


//-----------------------------------------------------------------------------
// Purpose: Stores a message in the database for later collection by a user
//-----------------------------------------------------------------------------
void CUserSession::PostMessageToUser(unsigned int fromID, unsigned int targetID, const char *text, int flags)
{
	// store the message in the database and flag the user to collect it next time they log on / heartbeat
	const char *name = "";
	if (fromID == m_iUserID)
	{
		name = m_szUserName;
	}

	g_pDataManager->TrackerUser(targetID)->User_PostMessage(fromID, targetID, "Normal", text, name, flags);

	// tell the friend that they have a message waiting for them, so the auth request is instant
	g_pDataManager->TrackerUser(targetID)->User_GetSessionInfo(this, STATE_CHECKMESSAGES, targetID);
}

//-----------------------------------------------------------------------------
// Purpose: Wraps the TrackerNET send message
//-----------------------------------------------------------------------------
void CUserSession::SendMessage(ISendMessage *SendMessage, Reliability_e state)
{
	float currentTime = sessionmanager()->GetCurrentTime();
	// check to see if firewall window is open
	if (m_flLastAccess + m_flFirewallWindow > currentTime)
	{
		// just send the message
		net()->SendMessage(SendMessage, state);
	}
	else
	{
		// user is behind a firewall; queue the packet until they heartbeat us again
		int bufferSize = SendMessage->MsgSize();
		bufferedmsg_t *msg = AllocateMessageBuffer(bufferSize);
		msg->msgID = SendMessage->GetMsgID();
		SendMessage->ReadMsgIntoBuffer(msg->data, bufferSize);
		AddBufferToQueue(msg);

		// release the send message
		SendMessage->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: message to user has failed, either the user has crashed or the firewall is blocking
//-----------------------------------------------------------------------------
void CUserSession::OnFailedMessage(IReceiveMessage *failedMsg)
{
	float currentTime = sessionmanager()->GetCurrentTime();

	// make sure we shouldn't be timed out
	if (CheckForTimeout(currentTime))
		return;

	// calculate the time at which we know the firewall fails, with a little slop
	float timeSinceAccess = currentTime - m_flLastAccess;
	// subtract out the time it takes a message to fail
	timeSinceAccess -= 6.0f;

	if (m_flFirewallWindow > timeSinceAccess)
	{
		// the firewall window has shrunk
		m_flFirewallWindow = timeSinceAccess;
		m_bUpdateFirewallWindowToClient = true;
	}

	if (m_flFirewallWindow < MINIMUM_FIRWALL_WINDOW_DURATION)
	{
		// firewall window is too small, just kill the user
		Logoff(true, false, false);
		return;
	}

	// extract the message, add it to the message queue
	int bufferSize = failedMsg->MsgSize();
	bufferedmsg_t *msg = AllocateMessageBuffer(bufferSize);
	msg->msgID = failedMsg->GetMsgID() - TMSG_FAIL_OFFSET;
	failedMsg->ReadMsgIntoBuffer(msg->data, bufferSize);
	AddBufferToQueue(msg);
}

enum
{
	MEMPOOL_SIZE_SMALL = 128,
	MEMPOOL_SIZE_MEDIUM = 256,
	MEMPOOL_SIZE_LARGE = 512,
};

CMemoryPool g_SmallMsgMemPool(MEMPOOL_SIZE_SMALL, 256);
CMemoryPool g_MediumMsgMemPool(MEMPOOL_SIZE_MEDIUM, 256);
CMemoryPool g_LargeMsgMemPool(MEMPOOL_SIZE_LARGE, 256);

static int g_AllocCounts[4] = { 0 };

//-----------------------------------------------------------------------------
// Purpose: returns a block of memory from the appropriate mempool
//-----------------------------------------------------------------------------
CUserSession::bufferedmsg_t *CUserSession::AllocateMessageBuffer(int bufferSize)
{
	int allocSize = bufferSize + sizeof(bufferedmsg_t);

	// find the mempool of the appropriate size
	bufferedmsg_t *buffer = NULL;
	if (allocSize <= MEMPOOL_SIZE_SMALL)
	{
		buffer = (bufferedmsg_t *)g_SmallMsgMemPool.Alloc(allocSize);
		g_AllocCounts[0]++;
	}
	else if (allocSize <= MEMPOOL_SIZE_MEDIUM)
	{
		buffer = (bufferedmsg_t *)g_MediumMsgMemPool.Alloc(allocSize);
		g_AllocCounts[1]++;
	}
	else if (allocSize <= MEMPOOL_SIZE_LARGE)
	{
		buffer = (bufferedmsg_t *)g_LargeMsgMemPool.Alloc(allocSize);
		g_AllocCounts[2]++;
	}
	else
	{
		// too large, just allocate from heap
		buffer = (bufferedmsg_t *)malloc(allocSize);
		g_AllocCounts[3]++;
	}

	buffer->dataSize = bufferSize;
	return buffer;
}

//-----------------------------------------------------------------------------
// Purpose: free's the memory for a message buffer
//-----------------------------------------------------------------------------
void CUserSession::FreeMessageBuffer(bufferedmsg_t *msg)
{
	int allocSize = msg->dataSize + sizeof(bufferedmsg_t);

	// find the mempool of the appropriate size
	if (allocSize <= MEMPOOL_SIZE_SMALL)
	{
		g_SmallMsgMemPool.Free(msg);
	}
	else if (allocSize <= MEMPOOL_SIZE_MEDIUM)
	{
		g_MediumMsgMemPool.Free(msg);
	}
	else if (allocSize <= MEMPOOL_SIZE_LARGE)
	{
		g_LargeMsgMemPool.Free(msg);
	}
	else
	{
		// too large, will have used heap
		free(msg);
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds a message buffer to the end of the queue
//-----------------------------------------------------------------------------
void CUserSession::AddBufferToQueue(bufferedmsg_t *msg)
{
	g_pConsole->Print(6, "Queueing message '%d' for '%d' (size: %d)\n", msg->msgID, m_iUserID, msg->dataSize);

	if (!m_pMsgQueueLast)
	{
		assert(!m_pMsgQueueFirst);

		// nothing in queue yet, add to Start
		m_pMsgQueueFirst = msg;
		m_pMsgQueueLast = msg;
		msg->nextBuffer = NULL;
		return;
	}

	// add to the end of the list
	msg->nextBuffer = NULL;
	m_pMsgQueueLast->nextBuffer = msg;
	m_pMsgQueueLast = msg;
}

//-----------------------------------------------------------------------------
// Purpose: sends all the messages in the queue, then releases them
//-----------------------------------------------------------------------------
void CUserSession::SendQueuedMessages()
{
	if (m_pMsgQueueFirst)
	{
		g_pConsole->Print(6, "Sending queued messages to '%d'\n", m_iUserID);
	}

	// iterate through and send all the messages
	for (bufferedmsg_t *msg = m_pMsgQueueFirst; msg != NULL; msg = msg->nextBuffer)
	{
		ISendMessage *sendMsg = net()->CreateMessage(msg->msgID, msg->data, msg->dataSize);
		sendMsg->SetSessionID(m_iSessionID);
		sendMsg->SetNetAddress(m_NetAddress);
		sendMsg->SetEncrypted(true);

		net()->SendMessage(sendMsg, NET_RELIABLE);
	}
	
	// clear out the queue
	FlushMessageQueue();
}

//-----------------------------------------------------------------------------
// Purpose: throws away the current message queue
//-----------------------------------------------------------------------------
void CUserSession::FlushMessageQueue()
{
	// free all the messages
	bufferedmsg_t *msg = m_pMsgQueueFirst;
	while (msg)
	{
		bufferedmsg_t *nextMsg = msg->nextBuffer;
		FreeMessageBuffer(msg);
		msg = nextMsg;
	}

	m_pMsgQueueFirst = NULL;
	m_pMsgQueueLast = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
unsigned int CUserSession::IP( void )
{
	return m_NetAddress.IP();
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
unsigned int CUserSession::Port( void )
{
	return m_NetAddress.Port();
}

//-----------------------------------------------------------------------------
// Purpose: Constructs the data from specifying a users' details
//			returns the number of bytes written
//-----------------------------------------------------------------------------
int CUserSession::ConstructFriendsStatusMessageData(void const *usersArray, int userCount, unsigned int *destBuffer)
{
	ITrackerUserDatabase::user_t *users = (ITrackerUserDatabase::user_t *)usersArray;

	int intsWritten = 0;
	for (int i = 0; i < userCount; i++)
	{
		// write in ID first
		destBuffer[intsWritten++] = users[i].userID;

		// write status second
		destBuffer[intsWritten++] = users[i].userStatus;

		// write session id third
		destBuffer[intsWritten++] = users[i].userSessionID;

		// write IP address
		destBuffer[intsWritten++] = users[i].userIP;
		destBuffer[intsWritten++] = users[i].userPort;

		// write serverID
		// BACKWARDS COMPATIBILITY: don't send serverID to old clients
		if (m_iBuildNum >= COMPATILIBITY_SERVERID_SUPPORT_VERSION_MIN)
		{
			destBuffer[intsWritten++] = users[i].userServerID;
		}
	}
	return intsWritten * sizeof(int);
}

//-----------------------------------------------------------------------------
// Purpose: Handles results from the database
//-----------------------------------------------------------------------------
void CUserSession::SQLDBResponse(int cmdID, int returnState, int returnVal, void *dataBlock)
{
	// check to see if sesion is still valid
	if (!m_iUserID || !m_pDB)
		return;

	bool bHandled = false;
	int arraySize = ARRAYSIZE(g_DBMsgDispatch);
	for (int i = 0; i < arraySize; i++)
	{
		if (g_DBMsgDispatch[i].cmdID == cmdID && g_DBMsgDispatch[i].returnState == returnState)
		{
			// dispatch the function
			(this->*g_DBMsgDispatch[i].msgFunc)(returnVal, dataBlock);
			bHandled = true;
			break;
		}
	}

	if (!bHandled)
	{
		g_pConsole->Print(9, "**** Error: no handler for CUserSession::SQLDBResponse(%d, %d)\n", cmdID, returnState);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_Login(int returnVal, void *dataBlock)
{
	ITrackerUserDatabase::UserLogin_t *data = (ITrackerUserDatabase::UserLogin_t *)dataBlock;
	int loginState = returnVal;
	if (loginState > 0)
	{
		// successful login
		m_iUserSessionState = USERSESSION_ACTIVE;
		m_iStatus = data->acquiredStatus;

		// reply to the user with a login ok message
		ISendMessage *msg = CreateUserMessage(TSVC_LOGINOK);
		msg->WriteInt("status", m_iStatus);
		SendMessage(msg, NET_RELIABLE);

		g_pConsole->Print( 0, "Sending 'LoginOK' message to '%s'\n", m_NetAddress.ToStaticString() );

		PostLogin();
	}
	else
	{
		if (loginState == -3)
		{
			// get the old account and they will be told to log off
			g_pDataManager->TrackerUser(m_iUserID)->User_GetSessionInfo(this, STATE_DISCONNECTINGUSER, m_iUserID);
			m_iUserSessionState = USERSESSION_CONNECTING;
		}
		else
		{
			m_iUserSessionState = USERSESSION_CONNECTFAILED;
			sessionmanager()->FreeUserSession(this);
		}

		g_pConsole->Print(4, "User_Login(%d) failed: %d\n", m_iUserID, loginState);
		
		// send login failed message
		m_iStatus = 0;

		ISendMessage *reply = CreateUserMessage(TSVC_LOGINFAIL);
		reply->WriteInt("reason", loginState);
		SendMessage(reply, NET_RELIABLE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user has finished being logged off
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_Logoff(int returnVal, void *dataBlock)
{
	// tell the friends that this user is going offline if we are successful
	if (returnVal > 0)
	{
		// logoff sucessful
		g_pConsole->Print(0, "Finished logging off user '%d'\n", m_iUserID);

		// tell everyone about their status
		UpdateStatusToFriends();

		// remove all watches, from all databases
		int dbCount = g_pDataManager->TrackerUserCount();
		for (int i = 0; i < dbCount; i++)
		{
			g_pDataManager->TrackerUserByIndex(i).db->User_RemoveWatcher(m_iUserID);
		}
	}
	else if (returnVal < 0)
	{
		g_pConsole->Print(0, "** Error '%d' logging off user '%d'\n", returnVal, m_iUserID);

		// kill the session
		sessionmanager()->FreeUserSession(this);
	}
	else
	{
		// returnVal is 0, which means that while the user was not logged off, it was still successful
		// user is probably logged into a different server with a dead connection left here

		// kill the session
		sessionmanager()->FreeUserSession(this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Used when we're trying to log in, but we're already logged in elsewhere.
//			disconnects our other connection from the server
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetSessionInfo_DisconnectingUser(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;

	ITrackerUserDatabase::user_t *data = (ITrackerUserDatabase::user_t *)dataBlock;
	if (data->userStatus < 1)
		return;	// they're not online

	topology()->ForceDisconnectUser(data->userID, data->userSessionID, data->userServerID);

	// free this user account
	m_iUserSessionState = USERSESSION_CONNECTFAILED;
	sessionmanager()->FreeUserSession(this);
}

//-----------------------------------------------------------------------------
// Purpose: Tells the user that they should check their messages
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetSessionInfo_CheckMessages(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;

	ITrackerUserDatabase::user_t *data = (ITrackerUserDatabase::user_t *)dataBlock;
	if (data->userStatus < 1)
		return;	// they're not online

	// tell them, check their messages
	topology()->UserCheckMessages(data->userID, data->userSessionID, data->userServerID);
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new watcher to ourselves, someone we have probably just authed
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetSessionInfo_AddWatch(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;

	ITrackerUserDatabase::user_t *data = (ITrackerUserDatabase::user_t *)dataBlock;

	if (data->userStatus < 1)
		return;	// they're not online, don't add the watch

	if (m_pDB)
	{
		m_pDB->User_AddSingleWatch(data->userID, data->userSessionID, data->userServerID, m_iUserID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Notify two users of each other's status
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetSessionInfo_ExchangeStatus(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;

	// send our status to the friend
	DBMsg_GetSessionInfo_SendingStatus(returnVal, dataBlock);

	// send friend's status to us
	ITrackerUserDatabase::user_t *data = (ITrackerUserDatabase::user_t *)dataBlock;
	if (data->userStatus < 1)
		return;	// they're not online

	// tell the user our status
	ISendMessage *msg = CreateUserMessage(TSVC_FRIENDS);

	msg->WriteInt("count", 1);

	unsigned int msgBuf[6];
	int bytesWritten = ConstructFriendsStatusMessageData(data, 1, msgBuf);
	msg->WriteBlob("status", msgBuf, bytesWritten);

	SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Sends the status of a single friend
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetSessionInfo_SendingStatus(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;

	ITrackerUserDatabase::user_t *data = (ITrackerUserDatabase::user_t *)dataBlock;
	if (data->userStatus < 1)
		return;	// they're not online

	// tell the user our status
	CUtlMsgBuffer msgBuffer;
	msgBuffer.WriteInt("_id", TSVC_FRIENDS);
	msgBuffer.WriteInt("count", 1);

	int msgBuf[6];
	int intsWritten = 0;
	msgBuf[intsWritten++] = m_iUserID;
	// write status second
	msgBuf[intsWritten++] = m_iStatus;
	// write session id third
	msgBuf[intsWritten++] = data->userSessionID;
	// write IP address
	msgBuf[intsWritten++] = m_NetAddress.IP();
	msgBuf[intsWritten++] = m_NetAddress.Port();

	if (m_iBuildNum >= COMPATILIBITY_SERVERID_SUPPORT_VERSION_MIN)
	{
		// write serverID
		msgBuf[intsWritten++] = topology()->GetServerID();
	}

	msgBuffer.WriteBlob("status", msgBuf, intsWritten * sizeof(int));

	topology()->SendNetworkMessageToUser(data->userID, data->userSessionID, data->userServerID, TSVC_FRIENDS, msgBuffer.Base(), msgBuffer.DataSize());
}


//-----------------------------------------------------------------------------
// Purpose: Tells a user that we're offline (even if we're not)
//			Used when a user is blocked
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetSessionInfo_SendingOfflineStatus(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;

	ITrackerUserDatabase::user_t *data = (ITrackerUserDatabase::user_t *)dataBlock;
	if (data->userStatus < 1)
		return;	// they're not online

	// tell the user our status is offline
	CUtlMsgBuffer msgBuffer;
	msgBuffer.WriteInt("_id", TSVC_FRIENDS);
	msgBuffer.WriteInt("count", 1);
	int msgBuf[5];
	int intsWritten = 0;
	msgBuf[intsWritten++] = m_iUserID;
	// write status second
	msgBuf[intsWritten++] = 0;
	// write sessionID third
	msgBuf[intsWritten++] = 0;
	// write blank IP address
	msgBuf[intsWritten++] = 0;
	msgBuf[intsWritten++] = 0;
	if (m_iBuildNum >= COMPATILIBITY_SERVERID_SUPPORT_VERSION_MIN)
	{
		// write empty serverID
		msgBuf[intsWritten++] = 0;
	}

	msgBuffer.WriteBlob("status", msgBuf, intsWritten * sizeof(int));

	// route the message to the user
	topology()->SendNetworkMessageToUser(data->userID, data->userSessionID, data->userServerID, TSVC_FRIENDS, msgBuffer.Base(), msgBuffer.DataSize());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_FindUsers(int returnVal, void *dataBlock)
{
	ITrackerUserDatabase::FindUsers_t *data = (ITrackerUserDatabase::FindUsers_t *)dataBlock;

	if (data->usersReturned > 0)
	{
		// Success!

		//!! need to send these as one packet
		for (int i = 0; i < data->usersReturned; i++)
		{
			unsigned int friendID = data->userID[i];
			const char *userName = data->userName[i];
			const char *firstName = data->firstName[i];
			const char *lastName = data->lastName[i];

			ISendMessage *msg = CreateUserMessage(TSVC_FRIENDSFOUND);

			msg->WriteUInt("uid", friendID);
			msg->WriteString("UserName", userName);
			msg->WriteString("FirstName", firstName);
			msg->WriteString("LastName", lastName);
		
			SendMessage(msg, NET_RELIABLE);
		}
	}
	else
	{
		// failure, no friends found
		ISendMessage *msg = CreateUserMessage(TSVC_NOFRIENDS);
		SendMessage(msg, NET_RELIABLE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns information about ourself
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetInfo(int returnVal, void *dataBlock)
{
	ITrackerUserDatabase::GetInfo_t *data = (ITrackerUserDatabase::GetInfo_t *)dataBlock;

	// send back information about the user
	ISendMessage *msg = CreateUserMessage(TSVC_FRIENDINFO);

	msg->WriteUInt("uid", data->targetID);
	msg->WriteString("UserName", data->userName);
	msg->WriteString("FirstName", data->firstName);
	msg->WriteString("LastName", data->lastName);
	msg->WriteString("email", data->email);

	// if this is the current users details, remember them
	if (data->targetID == m_iUserID)
	{
		v_strncpy(m_szUserName, data->userName, sizeof(m_szUserName));
	}

	SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetFriendInfo_IsAuthed(int returnVal, void *dataBlock)
{
	ITrackerUserDatabase::IsAuthed_t *data = (ITrackerUserDatabase::IsAuthed_t *)dataBlock;
	unsigned int friendID = data->targetID;
	if (data->authLevel > 0 || data->targetID == m_iUserID)
	{
		// get information about the user
		g_pDataManager->TrackerUser(friendID)->User_GetInfo(this, 0, friendID);
	}
	else
	{
		// send a not-authed message
		ISendMessage *msg = CreateUserMessage(TSVC_FRIENDINFO);
		msg->WriteUInt("uid", friendID);
		msg->WriteInt("NotAuthed", 1);
		SendMessage(msg, NET_RELIABLE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles the second part of an auth request
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_RequestAuth_IsAuthed(int returnVal, void *dataBlock)
{
	ITrackerUserDatabase::IsAuthed_t *data = (ITrackerUserDatabase::IsAuthed_t *)dataBlock;
	int friendID = data->targetID;

	// if we're blocked, or already authed, we don't need to do any more
	if (data->authLevel > AUTHLEVEL_REQUESTINGAUTH || data->blocked)
		return;

	// grant rights to the friend to be able to get our status
	m_pDB->User_SetAuth(m_iUserID, friendID, AUTHLEVEL_REQUESTINGAUTH);
	ITrackerUserDatabase *friendDB = g_pDataManager->TrackerUser(friendID);
	if (friendDB && friendDB != m_pDB)
	{
		friendDB->User_SetAuth(m_iUserID, friendID, AUTHLEVEL_REQUESTINGAUTH);
	}

	// post an request authorization message
	PostMessageToUser(m_iUserID, friendID, data->extraData, MESSAGEFLAG_REQUESTAUTH | MESSAGEFLAG_AUTHED);
}

//-----------------------------------------------------------------------------
// Purpose: sends the users status to all the friends marked to watch them
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetWatchers(int returnVal, void *dataBlock)
{
	int count = returnVal;

	ITrackerUserDatabase::watcher_t *watcher = (ITrackerUserDatabase::watcher_t *)dataBlock;

	// construct the message to send to all the watchers
	CUtlMsgBuffer msgBuffer;
	msgBuffer.WriteInt("_id", TSVC_FRIENDUPDATE);
	msgBuffer.WriteUInt("userID", m_iUserID);
	msgBuffer.WriteInt("status", m_iStatus);
	msgBuffer.WriteUInt("sessionID", m_iSessionID);
	msgBuffer.WriteUInt("serverID", topology()->GetServerID());
	msgBuffer.WriteUInt("IP", m_NetAddress.IP());
	msgBuffer.WriteUInt("port", m_NetAddress.Port());
	if (m_iStatus == STATUS_INGAME && m_iGameIP && m_iGamePort)
	{
		// send down game info as well
		msgBuffer.WriteUInt("GameIP", m_iGameIP);
		msgBuffer.WriteUInt("GamePort", m_iGamePort);
		msgBuffer.WriteString("Game", m_szGameType);
	}

	// loop through all the watchers and send each of them the status update message
	for (int i = 0; i < count; i++)
	{
		if (!watcher[i].userServerID)
			continue;	//!! watcher has bogus info

		if (m_iUserID == watcher[i].userID)
			continue;

		// send the message, routed through the users' server
		topology()->SendNetworkMessageToUser(watcher[i].userID, watcher[i].userSessionID, watcher[i].userServerID, TSVC_FRIENDUPDATE, msgBuffer.Base(), msgBuffer.DataSize());
	}

	// if we're sending our offline status, then this will be the last message we send
	if (m_iStatus == STATUS_OFFLINE)
	{
		// kill the session
		sessionmanager()->FreeUserSession(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Basic sort function, for use in qsort
//-----------------------------------------------------------------------------
static int __cdecl IntegerSortFunc(const void *elem1, const void *elem2)
{
	ITrackerUserDatabase::simpleuser_t *e1 = (ITrackerUserDatabase::simpleuser_t *)elem1;
	ITrackerUserDatabase::simpleuser_t *e2 = (ITrackerUserDatabase::simpleuser_t *)elem2;

	if (e1->userID > e2->userID)
	{
		return 1;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Sends a list of friends' status to the friend
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetFriendStatus(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;	// no friends

	static const int MAX_USERS = 400;
	static const int MAX_USERS_PER_PACKET = 40;

	// get the data
	ITrackerUserDatabase::user_t *user = (ITrackerUserDatabase::user_t *)dataBlock;
	int count = returnVal;

	if (count >= MAX_USERS)
	{
		g_pConsole->Print(10, "SendStatusOfFriends: user %d has too many friends\n", m_iUserID);
	}

	int friendsUnsent = count;
	ITrackerUserDatabase::user_t *usersToSend = user;
	while (friendsUnsent > 0)
	{
		// create the packet
		ISendMessage *msg = CreateUserMessage(TSVC_FRIENDS);

		// work out how many friends we can fit in this packet
		int processAmount = MAX_USERS_PER_PACKET;
		if (processAmount > friendsUnsent)
		{
			processAmount = friendsUnsent;
		}

		msg->WriteInt("count", processAmount);

		unsigned int msgBuf[MAX_USERS_PER_PACKET * 6];
		int bytesWritten = ConstructFriendsStatusMessageData(usersToSend, processAmount, msgBuf);
		msg->WriteBlob("status", msgBuf, bytesWritten);

		// send the message
		SendMessage(msg, NET_RELIABLE);

		// advance the count
		friendsUnsent -= processAmount;
		usersToSend += processAmount;
	}

	// build a new list of our ingame friends and get their status
	{
		ITrackerUserDatabase::simpleuser_t gamers[MAX_USERS];
		int gamerCount = 0;

		// find all users who are in a game
		for (int i = 0; i < count; i++)
		{
			if (user[i].userStatus == STATUS_INGAME)
			{
				gamers[gamerCount++].userID = user[i].userID;
			}
		}

		if (gamerCount < 1)
			return;	// no gamers in our list

		// batch up users per database
		ITrackerUserDatabase *previousDatabase = g_pDataManager->BaseTrackerUser(gamers[0].userID);
		i = 0;
		int batchCount = 0;
		ITrackerUserDatabase::simpleuser_t *batchStart = gamers + 0;
		while (previousDatabase)
		{
			// get the database
			ITrackerUserDatabase *db;
			if (i == gamerCount)
			{
				db = NULL;
			}
			else
			{
				db = g_pDataManager->BaseTrackerUser(gamers[i].userID);
			}
			
			if (db == previousDatabase)
			{
				i++;
				batchCount++;
				continue;
			}

			if (db != previousDatabase)
			{
				// finish up with the previous server and the batch of users on it

				// get the state of friends in this set
				previousDatabase->User_GetFriendsGameStatus(this, m_iUserID, batchStart, batchCount);

				// move to the next db
				previousDatabase = db;
				batchCount = 0;
				batchStart = gamers + i;
			}

			// go to the next user
			i++;
			batchCount++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles the return of a bunch of friends' ingame status
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetFriendsGameStatus(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;	// no friends

	// get the data
	ITrackerUserDatabase::gameuser_t *user = (ITrackerUserDatabase::gameuser_t *)dataBlock;
	int count = returnVal;

	// send a packet per friend
	for (int i = 0; i < count; i++)
	{
		if (user[i].userStatus != STATUS_INGAME || user[i].gameIP == 0)
			continue;

		ISendMessage *msg = CreateUserMessage(TSVC_GAMEINFO);

		// write in the details
		// they should already know the sessionID/IP/port of the user
		msg->WriteUInt("userID", user[i].userID);
		msg->WriteInt("status", user[i].userStatus);
		msg->WriteUInt("GameIP", user[i].gameIP);
		msg->WriteUInt("GamePort", user[i].gamePort);
		msg->WriteString("Game", user[i].gameType);

		// send
		SendMessage(msg, NET_RELIABLE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Takes a list of users, and adds watches to them
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetFriendList(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;	// no friends

	ITrackerUserDatabase::simpleuser_t *user = (ITrackerUserDatabase::simpleuser_t *)dataBlock;
	int friendCount = returnVal;

	// sort the list of friends by their userID, so we ensure they are grouped correctly
	qsort(dataBlock, friendCount, sizeof(ITrackerUserDatabase::simpleuser_t), IntegerSortFunc);

	// batch up a set of users
	ITrackerUserDatabase *previousDatabase = g_pDataManager->BaseTrackerUser(user[0].userID);
	int i = 0;
	int batchCount = 0;
	ITrackerUserDatabase::simpleuser_t *batchStart = user + 0;
	while (previousDatabase)
	{
		// get the database
		ITrackerUserDatabase *db;
		if (i == friendCount)
		{
			db = NULL;
		}
		else
		{
			db = g_pDataManager->BaseTrackerUser(user[i].userID);
		}
		
		if (db == previousDatabase)
		{
			i++;
			batchCount++;
			continue;
		}

		if (db != previousDatabase)
		{
			// finish up with the previous server and the batch of users on it

			// add watches to these users (this call is non-blocking)
			previousDatabase->User_AddWatcher(m_iUserID, m_iSessionID, topology()->GetServerID(), batchStart, batchCount);

			// get the state of friends in this set
			previousDatabase->User_GetFriendsStatus(this, m_iUserID, batchStart, batchCount);

			// move to the next db
			previousDatabase = db;
			batchCount = 0;
			batchStart = user + i;
		}

		// go to the next user
		i++;
		batchCount++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: We have retrieved a message
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_GetMessage(int returnVal, void *dataBlock)
{
	if (returnVal < 1)
		return;  // no message, so bail
	
	ITrackerUserDatabase::UserMessage_t *data = (ITrackerUserDatabase::UserMessage_t *)dataBlock;

	// make sure the user is in the specified build range
	if ((!data->mininumBuild || m_iBuildNum >= data->mininumBuild) && (!data->maximumBuild || m_iBuildNum <= data->maximumBuild))
	{
		// send to the user the message
		ISendMessage *msg = CreateUserMessage(TSVC_MESSAGE);

		msg->WriteUInt("uid", data->fromUserID);
		msg->WriteString("UserName", data->name);
		msg->WriteString("Text", data->text);
		msg->WriteInt("flags", data->flags);

		SendMessage(msg, NET_RELIABLE);
	}

	// delete the message from the db
	m_pDB->User_DeleteMessage(this, m_iUserID, data->msgID);
}

//-----------------------------------------------------------------------------
// Purpose: Delete message has returned
//-----------------------------------------------------------------------------
void CUserSession::DBMsg_DeleteMessage(int returnVal, void *data)
{
	if (returnVal > 0)
	{
		// message has been deleted successfully, so check for more messages
		m_pDB->User_GetMessage(this, m_iUserID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Safe string copy
//-----------------------------------------------------------------------------
void v_strncpy(char *dest, const char *src, int bufsize)
{
	if (src == dest)
		return;

	strncpy(dest, src, bufsize - 1);
	dest[bufsize - 1] = 0;
}
