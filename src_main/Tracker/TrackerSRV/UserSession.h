//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Defines a single user session object
//=============================================================================

#ifndef USERSESSION_H
#define USERSESSION_H
#pragma once

//#pragma warning(disable: 4786)

#include "ISQLDBReplyTarget.h"
#include "../TrackerNet/NetAddress.h"

class IReceiveMessage;
class ISendMessage;
class ITrackerNET;
class ITrackerDatabaseManager;
class ITrackerUserDatabase;

enum EUserSessionState
{
	USERSESSION_FREE,
	USERSESSION_ACTIVE,
	USERSESSION_CONNECTING,
	USERSESSION_CONNECTFAILED,
};

//-----------------------------------------------------------------------------
// Purpose: A single user session
//-----------------------------------------------------------------------------
class CUserSession : public ISQLDBReplyTarget
{
public:
	// construction
	CUserSession();
	~CUserSession();

	// initialization/reinitialization
	void Clear();

	// data access
	EUserSessionState State() { return m_iUserSessionState; }
	unsigned int UID()			{ return m_iUserID; }
	unsigned int SessionID()	{ return m_iSessionID; }
	int Status()			{ return m_iStatus; }
	const char *UserName()	{ return m_szUserName; }
	unsigned int IP();
	unsigned int Port();

	// reply handler for sql queries
	virtual void SQLDBResponse(int cmdID, int returnState, int success, void *data);
	virtual void WakeUp();

	// methods
	void CreateNewSessionID(int baseSessionID);
	bool CheckForTimeout(float currentTime);
	void CheckForMessages();

	ISendMessage *CreateUserMessage(int msgID);
	void ReceivedMessage(IReceiveMessage *userMsg, float currentTime);

	// called to disconnect/drop
	void Logoff(bool forced, bool fastReconnect, bool permanentDisconnect);
	
	// need to call through here to send a message to a user, to ensure it gets queued correctly
	void SendMessage(ISendMessage *SendMessage, enum Reliability_e state);

	// posts a message to the database for a user
	void PostMessageToUser(unsigned int fromID, unsigned int targetID, const char *text, int flags);

	//////////////////////////////////////////
	// network message handlers
	bool ReceivedMsg_Login(IReceiveMessage *loginMsg);
	void ReceivedMsg_Response(IReceiveMessage *pMsg);
	void ReceivedMsg_Heartbeat(IReceiveMessage *pMsg);
	void ReceivedMsg_FindUser(IReceiveMessage *findUserMsg);
	void ReceivedMsg_AuthUser(IReceiveMessage *authUserMsg);
	void ReceivedMsg_ReqAuth(IReceiveMessage *reqAuthMsg);
	void ReceivedMsg_ReqFriends(IReceiveMessage *reqAuthMsg);
	void ReceivedMsg_FriendInfo(IReceiveMessage *reqFriendInfoMsg);
	void ReceivedMsg_SetInfo(IReceiveMessage *setInfoMsg);
	void ReceivedMsg_Message(IReceiveMessage *msg);
	void ReceivedMsg_RouteToFriend(IReceiveMessage *msg);

	// failed message handler
	void OnFailedMessage(IReceiveMessage *failedMsg);

	//////////////////////////////////////////
	// SQL DB message handlers
	void DBMsg_Login(int returnVal, void *data);
	void DBMsg_Logoff(int returnVal, void *data);
	void DBMsg_GetSessionInfo_SendingStatus(int returnVal, void *data);
	void DBMsg_GetSessionInfo_ExchangeStatus(int returnVal, void *data);
	void DBMsg_GetSessionInfo_SendingOfflineStatus(int returnVal, void *dataBlock);
	void DBMsg_GetSessionInfo_DisconnectingUser(int returnVal, void *data);
	void DBMsg_GetSessionInfo_AddWatch(int returnVal, void *dataBlock);
	void DBMsg_GetSessionInfo_CheckMessages(int returnVal, void *dataBlock);
	void DBMsg_FindUsers(int returnVal, void *data);
	void DBMsg_GetInfo(int returnVal, void *data);
	void DBMsg_GetFriendInfo_IsAuthed(int returnVal, void *data);
	void DBMsg_RequestAuth_IsAuthed(int returnVal, void *data);
	void DBMsg_GetWatchers(int returnVal, void *data);
	void DBMsg_GetFriendList(int returnVal, void *data);
	void DBMsg_GetFriendStatus(int returnVal, void *dataBlock);
	void DBMsg_GetFriendsGameStatus(int returnVal, void *dataBlock);
	void DBMsg_GetMessage(int returnVal, void *data);
	void DBMsg_DeleteMessage(int returnVal, void *data);

private:
	void UpdateStatusToFriends();
	void AuthorizeUser(unsigned int targetID);
	void UnauthUser(unsigned int targetID);
	int ConstructFriendsStatusMessageData(void const *users, int userCount, unsigned int *destBuffer);

	// called after a succesful login
	void PostLogin();

	//////////////////////////////////////////
	// data
	EUserSessionState m_iUserSessionState;
	unsigned int m_iUserID;		// user id
	unsigned int m_iSessionID;	// session id (this stays across multiple users on this session)
	int m_iChallengeKey;	// secret session key
	CNetAddress m_NetAddress; // address of user
	float m_flLastAccess;	// time at which this session last received a packet from the Client
	float m_flTimeoutDelay;	// duration of time with no received packets after which user will be timed out
	float m_flFirewallWindow;	// duration of time for which the users' firewall is expected to be open after they send out a packet
	bool m_bUpdateFirewallWindowToClient;	// true if we need to send down the firewall window to the Client
	int m_iStatus;
	int m_iBuildNum;		// build number of the Client
	ITrackerUserDatabase *m_pDB;

	// game info
	unsigned int m_iGameIP;
	unsigned int m_iGamePort;
	char m_szGameType[64];

	char m_szPassword[32];
	char m_szUserName[32];

	// message queueing
	struct bufferedmsg_t
	{
		bufferedmsg_t *nextBuffer;
		int dataSize;
		int msgID;
		unsigned char data[1];
	};
	bufferedmsg_t *m_pMsgQueueFirst;
	bufferedmsg_t *m_pMsgQueueLast;

	// message queueing methods
	bufferedmsg_t *AllocateMessageBuffer(int bufferSize);
	void FreeMessageBuffer(bufferedmsg_t *msg);
	void AddBufferToQueue(bufferedmsg_t *msg);
	void FlushMessageQueue();
	void SendQueuedMessages();
};


#endif // USERSESSION_H
