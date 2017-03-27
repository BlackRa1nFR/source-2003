//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TRACKERDISTRODATABASE_H
#define TRACKERDISTRODATABASE_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlVector.h"
#include "TrackerCmdID.h"
#include "Interface.h"

class ISQLDBReplyTarget;

//-----------------------------------------------------------------------------
// Purpose: Defines the interface to the tracker UserDistribution database
//			These function map to a set of sql queries, ran in a thread
//-----------------------------------------------------------------------------
class ITrackerDistroDatabase : public IBaseInterface
{
public:
	// initializes the server to use the specified database
	// all other database info is gotten through this server
	virtual bool Initialize(int serverID, const char *serverName, const char *catalogName, CreateInterfaceFn debugFactory) = 0;

	virtual bool RunFrame() = 0;

	// cleanup
	virtual void deleteThis() = 0;

	// gets a list of servers
	struct trackerserver_t
	{
		char serverName[64];
		char catalogName[64];
		char backupServerName[64];
		char backupCatalogName[64];
		int userRangeMin;
		int userRangeMax;
		int nextValidUserID;
	};

	// returns a list of user servers, filling in the serverList vector
	// this is a blocking sql call
	// returns true on success
	virtual bool GetUserDistribution(CUtlVector<trackerserver_t> &serverList) = 0;

	// non-blocking call to update the user distribution - moves a set of users from one server to another
	virtual void UpdateUserDistribution(ISQLDBReplyTarget *replyTarget, const char *srcServer, const char *srcCatalog, const char *destServer, const char *destCatalog, int lowerRange, int upperRange) = 0;

	// reserves a userID for use in CreateUser()
	// non-blocking, use CMD_RESERVEUSERID to get result
	virtual void ReserveUserID(ISQLDBReplyTarget *replyTarget, const char *email, const char *password, const char *userName, const char *firstName, const char *lastName, int IP, int port) = 0;
	struct ReserveUserID_t
	{
		char email[128];
		char password[32];
		char username[32];
		char firstname[32];
		char lastname[32];
		int ip;
		int port;
	};
};

#define TRACKERDISTRODATABASE_INTERFACE_VERSION "TrackerDistroDatabase001"

#endif // TRACKERDISTRODATABASE_H
