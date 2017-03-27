//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef USER_H
#define USER_H
#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"
#include "interface.h"
#include "../TrackerNET/NetAddress.h"

class ITrackerNET;
class IReceiveMessage;

//-----------------------------------------------------------------------------
// Purpose: A single simulated user
//-----------------------------------------------------------------------------
class CUser
{
public:
	CUser();
	~CUser();

	// returns true if the user has a network connection established to the server
	bool IsActive();

	// returns pointer to user data
	KeyValues *Data();

	// makes a user perform a random act
	void PerformRandomAct();

	// makes the user check, read and respond to any networking messages it receives
	void ProcessNetworkInput();

	// networking messages
	void CreateUser(const char *nameBase, CreateInterfaceFn netFactory);
	void LoginUser();
	void LogoffUser();

	// random acts
	void Act_ChangeStatus();
	void Act_Logoff();
	void Act_SearchForFriends();

	// network message sending
	void SendMsg_CreateUser(const char *email, const char *userName, const char *firstName, const char *lastName, const char *password);
	void SendMsg_Login(int uid, const char *email, const char *password);
	void SendMsg_Logoff();
	void SendMsg_HeartBeat();
	void SendMsg_FriendSearch(int uid, const char *email, const char *username, const char *firstname, const char *lastname);
	void SendMsg_ReqAuth(int userID);
	void SendMsg_AuthUser(int targetID, bool allow);

	// network message receiving
	void RecvMsg_Challenge(KeyValues *msg);
	void RecvMsg_LoginOK(KeyValues *msg);
	void RecvMsg_LoginFail(KeyValues *msg);
	void RecvMsg_Friends(KeyValues *msg);
	void RecvMsg_FriendUpdate(KeyValues *msg);
	void RecvMsg_Heartbeat(KeyValues *msg);
	void RecvMsg_PingAck(KeyValues *msg);
	void RecvMsg_UserCreated(KeyValues *msg);
	void RecvMsg_UserCreateDenied(KeyValues *msg);
	void RecvMsg_FriendFound(KeyValues *msg);
	void RecvMsg_Message(KeyValues *msg);

private:
	// helper methods
	class ISendMessage *CreateServerMessage(const char *msgName);
	CNetAddress GetServerAddress();
	void WriteOut(KeyValues *dat, class ISendMessage *outFile);
	void ReadIn(KeyValues *dat, class IReceiveMessage *inFile);
	void UpdateHeartbeatTime();

	bool m_bActive;
	bool m_bLoggedIn;
	char m_szNameBase[32];
	KeyValues *m_pData;
	int m_iUID;
	int m_iStatus;
	int m_iSessionID;
	int m_iChallengeKey;
	int m_iBuildNum;

	enum { SEARCH_NONE, SEARCH_USERNAME, SEARCH_FIRSTNAME, SEARCH_LASTNAME, SEARCH_EMAIL };
	int m_iSearchType;

	double m_flHeartbeatTime;
	CNetAddress m_ServerAddress;

	ITrackerNET *m_pNet;
};


#endif // USER_H
