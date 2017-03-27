//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TOPOLOGYMANAGER_H
#define TOPOLOGYMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "UtlVector.h"
#include "ISQLDBReplyTarget.h"
#include "../public/ITrackerUserDatabase.h"

#include "../TrackerNET/NetAddress.h"

class ITrackerNET;
class CNetAddress;

//-----------------------------------------------------------------------------
// Purpose: Handles the connection and maintenance of this server
//			and its relationship to other trackerSRV's
//-----------------------------------------------------------------------------
class CTopologyManager : public ISQLDBReplyTarget
{
public:
	CTopologyManager();
	~CTopologyManager();

	// startup, returns false if initialization is unsucessful
	bool Initialize(unsigned int serverID, CreateInterfaceFn netFactory, CNetAddress server);

	// begins us shutting down server
	void StartShutdown();

	// Runs a networkframe
	// returns true if there is any activity, false otherwise
	bool RunFrame(int sleepFrame);

	// routes the message to the specified user, buffering and rerouting through other server if necessary
	void SendNetworkMessageToUser(unsigned int userID, unsigned int sessionID, unsigned int serverID, int msgID, void const *data, int dataSize);

	// disconnects the specified user
	void ForceDisconnectUser(unsigned int userID, unsigned int sessionID, unsigned int serverID);

	// makes the specified user check their messages
	void UserCheckMessages(unsigned int userID, unsigned int sessionID, unsigned int serverID);

	// message handling
	void ReceivedData(IReceiveMessage *dataBlock);
	void ReceivedMsg_WhoIsPrimary(IReceiveMessage *dataBlock);
	void ReceivedMsg_PrimarySrv(IReceiveMessage *dataBlock);
	void ReceivedMsg_RequestInfo(IReceiveMessage *dataBlock);
	void ReceivedMsg_SrvInfo(IReceiveMessage *dataBlock);
	void ReceivedMsg_ReqSrvInfo(IReceiveMessage *dataBlock);
	void ReceivedMsg_ServerPing(IReceiveMessage *dataBlock);
	void ReceivedMsg_LockUserRange(IReceiveMessage *dataBlock);
	void ReceivedMsg_UnlockUserRange(IReceiveMessage *dataBlock);
	void ReceivedMsg_RedirectToUser(IReceiveMessage *dataBlock);
	void ReceivedMsg_ForceDisconnectUser(IReceiveMessage *dataBlock);
	void ReceivedMsg_UserCheckMessages(IReceiveMessage *dataBlock);
	void OnMessageFailed(IReceiveMessage *dataBlock);

	// data from primary server
	const char *GetSQLDB();
	void SetSQLDB(const char *dbname);

	unsigned int GetServerID();

	// status
	bool IsPrimary() { return m_bPrimary; }
	CNetAddress PrimaryServerAddress() { return m_PrimaryServerAddress; }

private:
	void SendServerInfo(CNetAddress &addr);
	void SendTopologyInfo(CNetAddress &addr);
	void BecomePrimaryServer();

	// user range locking
	void LockUserRange(unsigned int lowerRange, unsigned int upperRange);
	void UnlockUserRange();
	
	// handles a response from the database
	void SQLDBResponse(int cmdID, int returnState, int returnVal, void *data);
	void WakeUp();

	// primary server states
	bool State_BasePrimaryServerFrame();
	bool State_WaitForUserRangeLock();
	bool State_MakeSureUserRangeIsOffline();
	bool State_CopyUsers();
	bool State_UserDataCopied();
	bool State_UserDistributionUpdated();

	bool State_AbortMoveUsers();

	// data
	bool m_bFindingPrimaryServer;	// true if we are currently looking for primary servers
	bool m_bPrimary;				// true if this is the primary server
	char m_dbsName[256];			// name of the main sqldb to connect to
	bool m_bShuttingDown;
	CNetAddress m_PrimaryServerAddress;
	ITrackerNET *m_pLocalNET;	// networking layer only used for local server->server traffic
	unsigned int m_iServerID;
	bool m_bResendServerInfo;	// this is true if we are the primary server and need to send new server lists to everyone

	double m_flLastFrameTime;	// time to complete last frame
	int m_iFPS;					// frames per second

	float m_flRepingTime;		// the time at which to reping all the servers

	// information regarding a single server
	struct server_t
	{
		CNetAddress addr;
		unsigned int serverID;
		bool primary;
	};
	CUtlVector<server_t> m_Servers;

	// Finds a specified server
	server_t &FindServer(CNetAddress &addr, unsigned int serverID);

	// list of clients watching our server information (usually just ServerMonitor.exe)
	CUtlVector<CNetAddress> m_Watchers;

	// current state function pointer
	typedef bool (CTopologyManager::*StateFunc_t)();
	StateFunc_t m_pStateFunc;

	// time at which to call the state function
	float m_flStateThinkTime;

	// moving users state
	unsigned int m_iMoveRangeLower;
	unsigned int m_iMoveRangeUpper;

	int m_iDestDatabaseIndex;
	int m_iSrcDatabaseIndex;

	ITrackerUserDatabase::completedata_t m_UserData;
};

inline CTopologyManager *topology()
{
	extern CTopologyManager *g_pTopologyManager;
	return g_pTopologyManager;
}


#endif // TOPOLOGYMANAGER_H
