//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H
#pragma once

class CUserSession;
class IReceiveMessage;
class CNetAddress;
class ITrackerDatabaseManager;

#include "../TrackerNET/TrackerNET_Interface.h"

#include "ISQLDBReplyTarget.h"
#include "UtlVector.h"
#include "UtlLinkedList.h"
#include "UtlRBTree.h"
#include "UserSession.h"
#include "CompletionEvent.h"

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

//-----------------------------------------------------------------------------
// Purpose: Manages and dispatches to all the sessions on the server
//			Handles all incoming messages
//-----------------------------------------------------------------------------
class CSessionManager : public ISQLDBReplyTarget
{
public:
	// construction
	CSessionManager();
	~CSessionManager();
	bool Initialize(unsigned int serverID, int maxSessions, const char *dbsName, CreateInterfaceFn debugFactory);

	// returns true if any work was done, false otherwise
	bool RunFrame(bool doIdleWork);

	// makes session manager not accept any new logins or requests
	void LockConnections();

	// locks a specified user range; disconects and prevents connection from
	// any users in that range
	void LockUserRange(unsigned int lowerRange, unsigned int upperRange);

	// unlocks any previous locked
	void UnlockUserRange();

	// queues a network message to be sent to a user
	void SendMessageToUser(unsigned int userID, unsigned int sessionID, int msgID, void const *message, int messageSize);

	// saves a message into the DB that could not be routed to the target user
	void SaveUnroutableNetworkMessage(unsigned int userID, int msgID, void const *message, int messageSize);

	// status
	int ActiveUserCount();
	int MaxUserCount();
	void PrintUserList();

	// handles responses from the database
	virtual void SQLDBResponse(int cmdID, int returnState, int returnVal, void *data);
	virtual void WakeUp();

	// causes the main thread to sleep until the session manager is signalled
	void WaitForEvent(unsigned long timeoutMillis);

	// returns the sleep event
	unsigned long GetWindowsEvent();

	// methods
	void LoginNewUser( IReceiveMessage *loginMsg );
	void CreateNewUser( IReceiveMessage *loginMsg );
	void DispatchMessageToUserSession( IReceiveMessage *userMsg );
	CUserSession *GetNewUserSession( void );
	void FreeUserSession( CUserSession *userSession );
	void FlushUsers(bool force);
	void ValidateUser(IReceiveMessage *validateMsg);
	void AcknowledgeUserPing(IReceiveMessage *pingMsg);

	// forces the user to be disconnected, and ensures the are logged out of sqldb
	void ForceDisconnectUser(unsigned int userID);

	// makes a user check their messages
	void UserCheckMessages(unsigned int userID);

	float GetCurrentTime();

	CUserSession *GetUserSessionByID(unsigned int userID);

	virtual bool ReceivedData( IReceiveMessage *dataBlock );
	virtual void OnFailedMessage( IReceiveMessage *dataBlock );

private:
	void MapUserIDToUserSession(unsigned int userID, CUserSession *userSession);
	void UnmapUserID(unsigned int userID);

	// MemPool/HashTable of user sessions
	// the high word of sessionID is used as the array index into m_pUserSessions
	CUtlVector<CUserSession> m_UserSessions;	// array of user sessions
	CUtlLinkedList<unsigned int, int> m_FreeUserSessions;
	unsigned int m_iServerID;				// server identifier
	bool m_bLockedDown;

	// wait event
	EventHandle_t m_hEvent;

	// inclusive range of locked users
	unsigned int m_iLockedUserLowerRange;
	unsigned int m_iLockedUserUpperRange;

	struct CreateUser_t
	{
		char email[128];
		char password[32];
		char userName[32];
		char firstName[32];
		char lastName[32];
		int resultsNeeded;
		unsigned int ip;
		unsigned int port;
		float requestTime;
		bool failed;
	};
	CUtlLinkedList<CreateUser_t, int> m_CreateUsers;	// list of users in the process of being created

	// mapping of userID's to session pointers
	struct SessionMapItem_t
	{
		unsigned int userID;
		CUserSession *session;
	};
	CUtlRBTree<SessionMapItem_t, int> m_IDMap;

	// sort function for idmap rbtree
	static bool SessionMapLessFunc(SessionMapItem_t const &, SessionMapItem_t const &);
};


//-----------------------------------------------------------------------------
// Purpose: accessor to session manager
//-----------------------------------------------------------------------------
inline CSessionManager *sessionmanager()
{
	extern CSessionManager *g_pSessionManager;
	return g_pSessionManager;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to networking interface
//-----------------------------------------------------------------------------
inline ITrackerNET *net()
{
	extern ITrackerNET *g_pTrackerSRVNET;
	return g_pTrackerSRVNET;
};

#endif // SESSIONMANAGER_H
