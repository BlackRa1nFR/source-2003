//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef SERVERSESSION_H
#define SERVERSESSION_H
#pragma once

#include <VGUI.h>
#include <VGUI_PHandle.h>

#include "../TrackerNET/TrackerNET_Interface.h"
class CNetAddress;
class ISendMessage;
class CUtlMsgBuffer;
class KeyValues;

namespace vgui
{
class Panel;
};

using vgui::KeyValues;

#include "UtlVector.h"

//-----------------------------------------------------------------------------
// Purpose: Handles Client<->server network communications
//-----------------------------------------------------------------------------
class CServerSession
{
public:
	CServerSession();
	~CServerSession();

	void RunFrame();

	// shuts down the networking, killing the threads and ensuring all remaing messages are sent
	void Shutdown();

	// gets the current user status
	int GetStatus();

	// returns true if we're currently connected to the server
	bool IsConnectedToServer();

	// returns true if we're currently in a state of attempting to connect to the server
	bool IsConnectingToServer();

	// cancels any current attempt to connect to the server
	void CancelConnect();

	// initiates a connection to the server	
	void SendInitialLogin(int desiredStatus);

	// callbacks
	void AddNetworkMessageWatch(vgui::Panel *panel, int msgID);
	void RemoveNetworkMessageWatch(vgui::Panel *panel);

	// server message sending
	void UserSelectNewStatus(int requestedStatus);

	void SearchForFriend(unsigned int uid, const char *email, const char *username, const char *firstname, const char *lastname);
	void RequestAuthorizationFromUser(unsigned int uid, const char *requestReason);

	void SendAuthUserMessage(unsigned int targetID, bool allow);
	void RequestUserInfoFromServer(unsigned int userID);
	void UpdateUserInfo(unsigned int userID, const char *userName, const char *firstName, const char *lastName);

	// starts looking for a valid server without actually logging into it (for initial CreateUser requests)
	void UnconnectedSearchForServer();

	// sets our game state so it can be uploaded to the server in a heartbeat
	void SetGameInfo(const char *gameDir, unsigned int gameIP, unsigned int gamePort);

	// creates a user
	void CreateNewUser(const char *email, const char *userName, const char *firstName, const char *lastName, const char *password);

	// checks to see if it's a valid user (need to remove?)
	void ValidateUserWithServer(const char *email, const char *password);

	// user message sending
	void SendTextMessageToUser(unsigned int userID, const char *text, int chatID);
	void RequestGameInfo(unsigned int uid);
	void SetUserBlock(unsigned int uid, int block, int fakeStatus);
	void SendKeyValuesMessage(unsigned int userID, int msgID, KeyValues *data);

	// message handlers
	void ReceivedMsg_Challenge(IReceiveMessage *challengeMsg);
	void ReceivedMsg_LoginOK(IReceiveMessage *loginMsg);
	void ReceivedMsg_LoginFail(IReceiveMessage *loginMsg);
	void ReceivedMsg_Friends(IReceiveMessage *friendMsg);
	void ReceivedMsg_FriendUpdate(IReceiveMessage *friendMsg);
	void ReceivedMsg_Heartbeat(IReceiveMessage *friendMsg);
	void ReceivedMsg_PingAck(IReceiveMessage *nullMsg);
	void ReceivedMsg_Disconnect(IReceiveMessage *msg);
	void ReceivedMsg_GameInfo(IReceiveMessage *msg);
	bool ReceivedMsg_DefaultHandler(IReceiveMessage *friendsMsg);

	bool ReceivedData(IReceiveMessage *dataBlock);
	void ReceivedRawData(IBinaryBuffer *data, CNetAddress &address);
	void MessageFailed(IReceiveMessage *dataBlock);
	bool CheckMessageValidity(IReceiveMessage *dataBlock);

private:
	// connection state change inputs
	void OnConnectionTimeout();
	void OnUserChangeStatus(int newStatus);
	void OnReceivedChallenge();
	void OnReceivedLoginSuccess(int newStatus);
	void OnReceivedDisconnectMessage(int minDisconnectTime, int maxDisconnectTime);
	void OnLoginFail(int reason);

	// checks to see if any states should change due to time
	void CheckForConnectionStateChange();

	// sends a message to any who requested to watch network data
	bool DispatchMessageToWatchers(KeyValues *msg, bool postFast = false);

	// tries a different server IP
	bool FallbackToAlternateServer();

	// communicates via heartbeat to the server our status
	void SendStatusToServer(int status);

	// returns the time, in milliseconds, between heartbeats
	int GetHeartBeatRate();

	CNetAddress GetServerAddress(void);
	ISendMessage *CreateServerMessage(int msgID);
	ISendMessage *CreateUserMessage(unsigned int userID, int msgID);
	void SendRoutedUserMessage(unsigned int userID, int msgID, CUtlMsgBuffer &msgBuffer);
	void SendUserMessage(unsigned int userID, int msgID, CUtlMsgBuffer &msgBuffer);

	ITrackerNET *m_pNet;
	KeyValues *m_pUserData;

	unsigned int m_iUID;		// current user id
	unsigned int m_iSessionID;	// server session identifier
	int m_iChallengeKey;	// challenge key

	int m_iTime;			// current time
	int m_iLastHeartbeat;	// time of last heartbeat being sent
	int m_iLastReceivedTime;	// time of last received packet from server
	int m_iFirewallWindow;
	
	enum ELoginState
	{
		LOGINSTATE_DISCONNECTED,
		LOGINSTATE_WAITINGTORECONNECT,
		LOGINSTATE_AWAITINGCHALLENGE,
		LOGINSTATE_AWAITINGLOGINSUCCESS,
		LOGINSTATE_CONNECTED,
	};

	ELoginState m_iLoginState;

	int m_iReconnectTime;		// time at which game will try to reconnect to server
	int m_iLoginTimeout;		// time at which a login attempt will fail


	int m_iBuildNum;
	int m_iDesiredStatus;	// status to switch to after login

	bool m_bServerSearch;	// true if we're in unconnected server search mode

	// watch list
	struct watchitem_t
	{
		vgui::PHandle hPanel;
		int messageID;
	};
	enum { MAX_WATCHITEMS = 128 };
	int m_iWatchCount;
	watchitem_t m_WatchList[MAX_WATCHITEMS];

	typedef CUtlVector<CNetAddress> netaddresslist_t;
	netaddresslist_t m_ServerList;
	int m_iCurrentServer;	// the index of the current master server we're trying

	int m_iReconnectRetries;	// number of times we're tried to connect since last connecting

	int m_iPreviousHeartBeatRateSentToServer;	// the heartbeat rate we had previously told the server

	int m_iStatus;
	unsigned int m_iGameIP;
	unsigned int m_iGamePort;
	char m_szGameDir[64];
};

//-----------------------------------------------------------------------------
// Purpose: Provides access to global instance of the server session
//-----------------------------------------------------------------------------
extern CServerSession &ServerSession();


#endif // SERVERSESSION_H
