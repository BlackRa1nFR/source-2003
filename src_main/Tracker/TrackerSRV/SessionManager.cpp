//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Manages and dispatches to all the sessions on the server
//=============================================================================

#include "SessionManager.h"
#include "UserSession.h"
#include "TrackerDatabaseManager.h"
#include "Random.h"
#include "TrackerProtocol.h"
#include "UtlMsgBuffer.h"

#include "../public/ITrackerUserDatabase.h"
#include "../public/ITrackerDistroDatabase.h"

#include "DebugConsole_Interface.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

extern IDebugConsole *g_pConsole;
CSessionManager *g_pSessionManager = NULL;

#define SESSION_INDEX(sessionID) ((sessionID) >> 16)

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSessionManager::CSessionManager() : m_UserSessions(NULL)
{
	g_pSessionManager = this;
	m_iLockedUserLowerRange = 0;
	m_iLockedUserUpperRange = 0;
	m_IDMap.SetLessFunc(SessionMapLessFunc);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSessionManager::~CSessionManager()
{
	if (g_pDataManager)
	{
		g_pDataManager->deleteThis();
	}

	g_pSessionManager = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Static function used in session map
//-----------------------------------------------------------------------------
bool CSessionManager::SessionMapLessFunc(SessionMapItem_t const &r1, SessionMapItem_t const &r2)
{
	return (r1.userID < r2.userID);
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the session manager
// Input  : maxSessions - number of sessions to allocate
//-----------------------------------------------------------------------------
bool CSessionManager::Initialize(unsigned int serverID, int maxSessions, const char *dbsName, CreateInterfaceFn debugFactory)
{
	// initialize sleep manager
	m_hEvent = Event_CreateEvent();
	net()->SetWindowsEvent(m_hEvent);

	// allocate space for user data
	m_UserSessions.AddMultipleToTail(maxSessions);
	m_FreeUserSessions.EnsureCapacity(maxSessions);

	// initialize the free list
	for (int i = 0; i < m_UserSessions.Size(); i++)
	{
		m_FreeUserSessions.AddToTail(i);
	}

	m_iServerID = serverID;

	// connect to the data server
	g_pDataManager = Create_TrackerDatabaseManager();
	if (!g_pDataManager->Initialize(m_iServerID, dbsName, debugFactory))
	{
		g_pConsole->Print(3, "Could not initialize DataManager\n");
		return false;
	}

	// flush bad users off the server
	FlushUsers(true);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: makes session manager not accept any new logins or info requests
//-----------------------------------------------------------------------------
void CSessionManager::LockConnections()
{
	m_bLockedDown = true;
}

//-----------------------------------------------------------------------------
// Purpose: Called every 'frame', as often as possible
//-----------------------------------------------------------------------------
bool CSessionManager::RunFrame(bool doIdleWork)
{
	// check to see if any of the clients have been connected too long
	float time = GetCurrentTime();
	bool doneWork = false;

	if (doIdleWork)
	{
		// walk the session map
		for (int i = 0; i < m_IDMap.MaxElement(); i++)
		{
			if (!m_IDMap.IsValidIndex(i))
				continue;

			CUserSession *session = m_IDMap[i].session;
			if (session->State() == USERSESSION_FREE)
			{
				// they shouldn't be in the mapping, remove
				g_pConsole->Print(5, "!! Removing invalid entry from the IDMap (user %d)\n", m_IDMap[i].userID);
				m_IDMap.RemoveAt(i);
			}
			else if (session->CheckForTimeout(time))
			{
				// we've sent a heartbeat to a Client; Stop checking for now and just continue later
				doneWork = true;
				break;
			}
		}
	}

	if (g_pDataManager->RunFrame())
	{
		doneWork = true;		
	}

	return doneWork;
}

//-----------------------------------------------------------------------------
// Purpose: Looks up the user session by the userID
//-----------------------------------------------------------------------------
CUserSession *CSessionManager::GetUserSessionByID(unsigned int userID)
{
	// search for the item in the mapping
	SessionMapItem_t searchItem = { userID, NULL };
	int index = m_IDMap.Find(searchItem);
	if (m_IDMap.IsValidIndex(index))
	{
		return m_IDMap[index].session;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Looks the user up in the session list and dispatch the message to them
//-----------------------------------------------------------------------------
void CSessionManager::DispatchMessageToUserSession(IReceiveMessage *userMsg)
{
	if (m_bLockedDown)
	{
		// don't accept any more messages from users when locked down
		return;
	}

	// session index into the session array is the high word of the sessionID
	unsigned short sessionIndex = SESSION_INDEX(userMsg->SessionID());

	bool bBadPacket = false;
	if (sessionIndex >= m_UserSessions.Size())
	{
		// bad packet
		bBadPacket = true;
	}
	else if (m_UserSessions[sessionIndex].SessionID() == userMsg->SessionID())
	{
		// sessionID is correct
		float timer = GetCurrentTime();
		if (m_UserSessions[sessionIndex].State() == USERSESSION_ACTIVE)
		{
			m_UserSessions[sessionIndex].ReceivedMessage(userMsg, timer);
		}
		else if (m_UserSessions[sessionIndex].State() == USERSESSION_CONNECTING && userMsg->GetMsgID() == TCLS_RESPONSE)
		{
			m_UserSessions[sessionIndex].ReceivedMessage(userMsg, timer);
		}
		else
		{
			bBadPacket = true;
		}

		if (m_UserSessions[sessionIndex].State() == USERSESSION_CONNECTFAILED)
		{
			FreeUserSession(&m_UserSessions[sessionIndex]);
		}
	}
	else
	{
		bBadPacket = true;
		g_pConsole->Print(6, "**** !! Received mismatched sessionID (%d, should be %d!\n", userMsg->SessionID(), m_UserSessions[sessionIndex].SessionID());
	}

	if (bBadPacket)
	{
		if (userMsg->GetMsgID() == TCLS_HEARTBEAT || userMsg->GetMsgID() == TCLS_MESSAGE || userMsg->GetMsgID() == TCLS_ROUTETOFRIEND)
		{
			// send back a message telling this Client that they're just not logged in
			g_pConsole->Print(6, "Bad pack, sending disconnect msg\n");

			ISendMessage *msg = net()->CreateReply(TSVC_DISCONNECT, userMsg);
			msg->SetSessionID(userMsg->SessionID());
			msg->WriteInt("minTime", 1);
			msg->WriteInt("maxTime", 4);
			net()->SendMessage(msg, NET_RELIABLE);
		}

		g_pConsole->Print(5, "Dropped msg '%d': Bad sessionID %d (%s)\n", userMsg->GetMsgID(), userMsg->SessionID(), userMsg->NetAddress().ToStaticString());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allocates a new user session from the free list, and sets up
//			a new session ID for it
//-----------------------------------------------------------------------------
CUserSession *CSessionManager::GetNewUserSession( void )
{
	// take a new session from the front of the free list
	int freeListIndex = m_FreeUserSessions.Head();
	int arrayIndex = m_FreeUserSessions[freeListIndex];
	m_FreeUserSessions.Remove(freeListIndex);
	CUserSession *newSession = &m_UserSessions[arrayIndex];

	// increment the new sessions sessionID so that no old messages will be valid for it
	newSession->CreateNewSessionID(arrayIndex);

	// return the new session
	return newSession;
}

//-----------------------------------------------------------------------------
// Purpose: Frees a user session and moves it into the free list
//-----------------------------------------------------------------------------
void CSessionManager::FreeUserSession(CUserSession *userSession)
{
	if (userSession->UID() > 0)
	{
		UnmapUserID(userSession->UID());
	}

	// clear the freed session
	userSession->Clear();

	// add it to the free list
	unsigned short sessionIndex = SESSION_INDEX(userSession->SessionID());
	m_FreeUserSessions.AddToTail((unsigned int)sessionIndex);
}

//-----------------------------------------------------------------------------
// Purpose: locks a specified user range; disconects and prevents connection from any users in that range
//-----------------------------------------------------------------------------
void CSessionManager::LockUserRange(unsigned int lowerRange, unsigned int upperRange)
{
	m_iLockedUserLowerRange = lowerRange;
	m_iLockedUserUpperRange = upperRange;

	// disconnect all users in that range
	for (unsigned int userID = m_iLockedUserLowerRange; userID <= m_iLockedUserUpperRange; userID++)
	{
		SessionMapItem_t searchItem = { userID, NULL };
		int index = m_IDMap.Find(searchItem);

		if (m_IDMap.IsValidIndex(index))
		{
			m_IDMap[index].session->Logoff(true, false, false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: unlocks any previous locked
//-----------------------------------------------------------------------------
void CSessionManager::UnlockUserRange()
{
	m_iLockedUserLowerRange = 0;
	m_iLockedUserUpperRange = 0;

	// reload the user distribution table
	g_pDataManager->ReloadUserDistribution();
}

//-----------------------------------------------------------------------------
// Purpose: forces the user to be disconnected, and ensures the are logged out of sqldb
//-----------------------------------------------------------------------------
void CSessionManager::ForceDisconnectUser(unsigned int userID)
{
	// find the session
	CUserSession *userSession = sessionmanager()->GetUserSessionByID(userID);

	if (userSession)
	{
		// we have the user, log them out
		userSession->Logoff(true, false, true);
	}
	else
	{
		// user not found; kill them out of database
		ITrackerUserDatabase *db = g_pDataManager->TrackerUser(userID);
		if (db)
		{
			db->User_Logoff(NULL, userID, true);

			// walk through, removing this user as a watcher
			int dbCount = g_pDataManager->TrackerUserCount();
			for (int i = 0; i < dbCount; i++)
			{
				g_pDataManager->TrackerUserByIndex(i).db->User_RemoveWatcher(userID);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: makes a user check their messages
//-----------------------------------------------------------------------------
void CSessionManager::UserCheckMessages(unsigned int userID)
{
	CUserSession *userSession = sessionmanager()->GetUserSessionByID(userID);
	if (userSession)
	{
		// we have the user, make them check messages
		userSession->CheckForMessages();
	}
}

//-----------------------------------------------------------------------------
// Purpose: queues a network message to be sent to a user
//-----------------------------------------------------------------------------
void CSessionManager::SendMessageToUser(unsigned int userID, unsigned int sessionID, int msgID, void const *message, int messageSize)
{
	// find the session
	unsigned short sessionIndex = SESSION_INDEX(sessionID);

	// check for valid sessionIndex
	if (sessionIndex >= m_UserSessions.Size())
	{
		g_pConsole->Print(5, "SendMessageToUser() : out-of-range sessionID(%d) sending msg (%d) to user (%d)\n", sessionID, msgID, userID);
		SaveUnroutableNetworkMessage(userID, msgID, message, messageSize);
		return;
	}

	CUserSession &session = m_UserSessions[sessionIndex];
	// check the session is valid
	if (!session.State() == USERSESSION_ACTIVE)
	{
		g_pConsole->Print(5, "SendMessageToUser() : invalid sessionID(%d) sending msg (%d) to user (%d)\n", sessionID, msgID, userID);
		SaveUnroutableNetworkMessage(userID, msgID, message, messageSize);
		return;
	}

	// check the userID matches the sessionID
	if (session.UID() != userID)
	{
		g_pConsole->Print(5, "SendMessageToUser() : invalid target user (%d) with valid sessionID(%d) sending msg (%d)\n", userID, sessionID, msgID);
		SaveUnroutableNetworkMessage(userID, msgID, message, messageSize);
		return;
	}

	// get their network address
	CNetAddress addr(session.IP(), session.Port());

	// convert the data block into a message
	ISendMessage *msg = net()->CreateMessage(msgID, message, messageSize);
	msg->SetNetAddress(addr);
	msg->SetEncrypted(true);
	msg->SetSessionID(sessionID);

	// send the message along to the user
	session.SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: saves a message that could not be routed to the target user
//-----------------------------------------------------------------------------
void CSessionManager::SaveUnroutableNetworkMessage(unsigned int userID, int msgID, void const *message, int messageSize)
{
	// only save text chat messages
	if (msgID == TCL_MESSAGE)
	{
		// get enough info to save the message
		bool bSaveSuccessful = false;
		CUserSession *targetUser = GetUserSessionByID(userID);
		if (targetUser)
		{
			// post the message into the database
			unsigned int fromID;
			char messageText[512];
			CUtlMsgBuffer msg(message, messageSize);
			msg.ReadUInt("uid", fromID);
			msg.ReadString("Text", messageText, sizeof(messageText));
			targetUser->PostMessageToUser(fromID, userID, messageText, 0);
			bSaveSuccessful = true;
		}
		
		if (!bSaveSuccessful)
		{
			g_pConsole->Print(6, "** Unroutable chat message to (%d) successfully saved in db\n", userID);
		}
		else
		{
			g_pConsole->Print(8, "!!!! Throwing away chat message to (%d)\n", userID);
		}
	}
	else
	{
		g_pConsole->Print(8, "!!!! Throwing away unroutable non-chat message to (%d)\n", userID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Callback for receiving data blocks from the network
//-----------------------------------------------------------------------------
bool CSessionManager::ReceivedData(IReceiveMessage *dataBlock)
{
	if (m_bLockedDown)
	{
		// don't accept any more messages when locked down
		return true;
	}

	// if it's a login message, allocate a new session
	switch (dataBlock->GetMsgID())
	{
		case TCLS_LOGIN:
		{
			// if it's a login message, allocate a new session
			LoginNewUser(dataBlock);
			return true;
		}

		case TCLS_CREATEUSER:
		{
			CreateNewUser(dataBlock);
			return true;
		}

		case TCLS_VALIDATEUSER:
		{
			ValidateUser(dataBlock);
			return true;
		}

		case TCLS_PING:
		{
			AcknowledgeUserPing(dataBlock);
			return true;
		}
		default:
		{
			// try and match the message with an existing session
			DispatchMessageToUserSession(dataBlock);
			break;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: We have had a network message to a user fail, force them off
//-----------------------------------------------------------------------------
void CSessionManager::OnFailedMessage(IReceiveMessage *failedMsg)
{
	// try and find the user
	unsigned int sessionID = failedMsg->SessionID();
	unsigned short sessionIndex = SESSION_INDEX(sessionID);

	if (sessionIndex >= m_UserSessions.Size())
	{
		// bad packet, throw away
		return;
	}

	if (m_UserSessions[sessionIndex].State() == USERSESSION_ACTIVE && m_UserSessions[sessionIndex].Status())
	{
		// they failed a general message, either a firewall issue or the user has crashed
		m_UserSessions[sessionIndex].OnFailedMessage(failedMsg);
	}
	else if (m_UserSessions[sessionIndex].State() == USERSESSION_CONNECTING)
	{
		// challenge message failed, disconnect user
		FreeUserSession(&m_UserSessions[sessionIndex]);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Flushes all logged in users out of the database
//-----------------------------------------------------------------------------
void CSessionManager::FlushUsers(bool force)
{
	if (force)
	{
		// issue general flush of any user logged on to this machine
		for (int i = 0; i < g_pDataManager->TrackerUserCount(); i++)
		{
			g_pDataManager->TrackerUserByIndex(i).db->Users_Flush();
		}
	}
	else
	{
		// log out all users
		for (int i = 0; i < m_UserSessions.Size(); i++)
		{
			if (m_UserSessions[i].State() == USERSESSION_ACTIVE)
			{
				m_UserSessions[i].Logoff(true, false, false);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to log a user onto the server
// Input  : *loginMsg - network message 
//-----------------------------------------------------------------------------
void CSessionManager::LoginNewUser(IReceiveMessage *loginMsg)
{
	// get the userID
	unsigned int userID;
	loginMsg->ReadUInt("uid", userID);

	if (userID >= m_iLockedUserLowerRange && userID <= m_iLockedUserUpperRange)
	{
		// we can't log the user in since they're in the locked range - tell them to stay disconnected
		g_pConsole->Print(6, "Locked range, sending disconnected message to %d\n", userID);

		ISendMessage *msg = net()->CreateReply(TSVC_DISCONNECT, loginMsg);
		msg->WriteInt("minTime", 1);
		msg->WriteInt("maxTime", 4);
		net()->SendMessage(msg, NET_RELIABLE);
	}
	else
	{
		// allocate a new user session
		CUserSession *session = GetNewUserSession();
		
		if (session->ReceivedMsg_Login(loginMsg))
		{
			// user login success; map in their user id
			MapUserIDToUserSession(session->UID(), session);
		}
		else 
		{
			// bad login attempt, clear user session
			FreeUserSession(session);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: inserts a mapping of userID -> CUserSession
//-----------------------------------------------------------------------------
void CSessionManager::MapUserIDToUserSession(unsigned int userID, CUserSession *userSession)
{
	SessionMapItem_t searchItem = { userID, userSession };
	m_IDMap.Insert(searchItem);
}

//-----------------------------------------------------------------------------
// Purpose: removes a userID -> CUserSession mapping
//-----------------------------------------------------------------------------
void CSessionManager::UnmapUserID(unsigned int userID)
{
	SessionMapItem_t searchItem = { userID, NULL };
	int index = m_IDMap.Find(searchItem);
	if (m_IDMap.IsValidIndex(index))
	{
		m_IDMap.RemoveAt(index);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tries to create a new user
//			Does not log them on
//-----------------------------------------------------------------------------
void CSessionManager::CreateNewUser(IReceiveMessage *msg)
{
	// temporarily store the new new user
	int newIndex = m_CreateUsers.AddToTail();
	CreateUser_t &user = m_CreateUsers[newIndex];

	msg->ReadString("username", user.userName, 32);
	msg->ReadString("firstname", user.firstName, 32);
	msg->ReadString("lastname", user.lastName, 32);
	msg->ReadString("email", user.email, 128);
	msg->ReadString("password", user.password, 32);

	// make sure the user doesn't exist on any of the servers
	user.resultsNeeded = g_pDataManager->TrackerUserCount();
	user.failed = false;

	user.ip = msg->NetAddress().IP();
	user.port = msg->NetAddress().Port();
	user.requestTime = GetCurrentTime();

	// ask all the servers whether they've seen this user
	for (int i = 0; i < g_pDataManager->TrackerUserCount(); i++)
	{
		ITrackerDatabaseManager::db_t &db = g_pDataManager->TrackerUserByIndex(i);
		db.db->User_Validate(this, user.email, user.password, user.ip, user.port, newIndex);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the specified user exists, and replies to the message
//			with the users userID
// Input  : *validateMsg - 
//-----------------------------------------------------------------------------
void CSessionManager::ValidateUser(IReceiveMessage *msg)
{
	char email[128];
	char password[32];

	// parse out the createUserMsg
	msg->ReadString("email", email, 128);
	msg->ReadString("password", password, 32);

	// check to see if the user exists on any server
	for (int i = 0; i < g_pDataManager->TrackerUserCount(); i++)
	{
		g_pDataManager->TrackerUserByIndex(i).db->User_Validate(this, email, password, msg->NetAddress().IP(), msg->NetAddress().Port(), (unsigned int)~0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles a simple ping message from a user, and replies with an 'Ack'
//-----------------------------------------------------------------------------
void CSessionManager::AcknowledgeUserPing(IReceiveMessage *pingMsg)
{
	ISendMessage *ackMsg = net()->CreateReply(TSVC_PINGACK, pingMsg);
	net()->SendMessage(ackMsg, NET_UNRELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Returns the current time, in seconds
//-----------------------------------------------------------------------------
float CSessionManager::GetCurrentTime()
{
	//!! need to use a higher res timer
	static long timeBase = 0;
	time_t timer;
	::time(&timer);

	// hack: normalize the timebase so the float isn't so inaccurate
	if (!timeBase)
	{
		timeBase = (long)timer;
	}
	return (float)((long)timer - timeBase);
}

//-----------------------------------------------------------------------------
// Purpose: causes the main thread to sleep until it's signalled
//-----------------------------------------------------------------------------
void CSessionManager::WaitForEvent(unsigned long timeoutMillis)
{
	Event_WaitForEvent(m_hEvent, timeoutMillis);
}

//-----------------------------------------------------------------------------
// Purpose: returns the sleep event
//-----------------------------------------------------------------------------
unsigned long CSessionManager::GetWindowsEvent()
{
	return m_hEvent;
}

//-----------------------------------------------------------------------------
// Purpose: Wakes up the main thread
//-----------------------------------------------------------------------------
void CSessionManager::WakeUp()
{
	Event_SignalEvent(m_hEvent);
}

//-----------------------------------------------------------------------------
// Purpose: handles a response from the sql database
//-----------------------------------------------------------------------------
void CSessionManager::SQLDBResponse(int cmdID, int returnState, int returnVal, void *data)
{
	if (cmdID == CMD_VALIDATE)
	{
		// response to the Validate command is in
		ITrackerUserDatabase::UserValidate_t *response = (ITrackerUserDatabase::UserValidate_t *)data;

		if (response->temporaryID == -1)
		{
			// reply with the returned state
			ISendMessage *reply = net()->CreateMessage(TSVC_USERVALID);
			CNetAddress addr;
			addr.SetIP(response->IP);
			addr.SetPort(response->Port);
			reply->SetNetAddress(addr);
			reply->SetEncrypted(true);

			unsigned int userID = *((unsigned int *)(&returnVal));
			reply->WriteUInt("userID", userID);

			net()->SendMessage(reply, NET_RELIABLE);
		}
		else
		{
			// user is attempting to be created
			if (!m_CreateUsers.IsValidIndex(response->temporaryID))
				return;

			CreateUser_t &user = m_CreateUsers[response->temporaryID];

			if (returnVal != -1)
			{
				// user already exists; send back a fail message
				user.failed = true;

				// bad ID, write out an error string
				ISendMessage *reply = net()->CreateMessage(TSVC_USERCREATED);
				CNetAddress addr;
				addr.SetIP(user.ip);
				addr.SetPort(user.port);
				reply->SetNetAddress(addr);
				reply->SetEncrypted(true);
				char *msg = "Could not create new account.\nUser with that email address already exists.";
				reply->WriteString("error", msg);

				net()->SendMessage(reply, NET_RELIABLE);
			}

			// check to see if we've received a response from each DB
			user.resultsNeeded--;
			if (user.resultsNeeded == 0)
			{
				if (!user.failed)
				{
					// user is ok, reserve a userID for them
					g_pDataManager->TrackerDistro()->ReserveUserID(this, user.email, user.password, user.userName, user.firstName, user.lastName, user.ip, user.port);
				}

				// remove the user
				m_CreateUsers.Remove(response->temporaryID);
			}
		}
	}
	else if (cmdID == CMD_RESERVEUSERID)
	{
		// we've got a userID back 
		ITrackerDistroDatabase::ReserveUserID_t *user = (ITrackerDistroDatabase::ReserveUserID_t *)data;
		int userID = returnVal;
		if (userID < 1)
		{
			// send back an error message
			ISendMessage *reply = net()->CreateMessage(TSVC_USERCREATED);
			CNetAddress addr;
			addr.SetIP(user->ip);
			addr.SetPort(user->port);
			reply->SetNetAddress(addr);
			reply->SetEncrypted(true);

			reply->WriteUInt("newUID", userID);
			if (returnVal == -1)
			{
				// bad ID, write out an error string
				char *msg = "Could not create new account.\nUser with that email address already exists.";
				reply->WriteString("error", msg);
			}
			else if (returnVal < 1)
			{
				char *msg = "Server could not create user.\nPlease try again at another time.";
				reply->WriteString("error", msg);
			}
			
			net()->SendMessage(reply, NET_RELIABLE);
			return;	// don't continue
		}

		// success, create the user entry in the database in the reserved slot
		g_pDataManager->TrackerUser(userID)->User_Create(this, userID, user->email, user->password, user->username, user->firstname, user->lastname, user->ip, user->port);
	}
	else if (cmdID == CMD_CREATE)
	{
		ITrackerUserDatabase::UserCreate_t *response = (ITrackerUserDatabase::UserCreate_t *)data;

		// build and reply with a create user success message
		ISendMessage *reply = net()->CreateMessage(TSVC_USERCREATED);
		CNetAddress addr;
		addr.SetIP(response->IP);
		addr.SetPort(response->Port);
		reply->SetNetAddress(addr);
		reply->SetEncrypted(true);

		unsigned int newUID = returnVal;
		reply->WriteUInt("newUID", newUID);
		if (returnVal < 1)
		{
			char *msg = "Server could not create user.\nPlease try again at another time.";
			reply->WriteString("error", msg);
		}
		
		net()->SendMessage(reply, NET_RELIABLE);

		g_pConsole->Print( 1, "Sending UserCreated(%d) message to '%s'\n", returnVal, reply->NetAddress().ToStaticString() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns number of active users on server
//-----------------------------------------------------------------------------
int CSessionManager::ActiveUserCount()
{
	return m_IDMap.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Returns maximum number of users holdable
//-----------------------------------------------------------------------------
int CSessionManager::MaxUserCount()
{
	return m_UserSessions.Size();
}

//-----------------------------------------------------------------------------
// Purpose: Displays current list of users on the server
//-----------------------------------------------------------------------------
void CSessionManager::PrintUserList()
{
	g_pConsole->Print(9, "UID\tSTATUS\tUSERNAME\t\t\tIP ADDRESS\n");

	for (int i = 0; i < m_UserSessions.Size(); i++)
	{
		CUserSession &session = m_UserSessions[i];
		if (session.State() == USERSESSION_ACTIVE)
		{
			CNetAddress addr;
			addr.SetIP(session.IP());
			addr.SetPort(session.Port());
			g_pConsole->Print(9, "%d\t%d\t%s\t\t\t%s\n", session.UID(), session.Status(), session.UserName(), addr.ToStaticString());
		}
	}
}


