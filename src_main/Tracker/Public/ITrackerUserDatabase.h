//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef TRACKERUSERDATABASE_H
#define TRACKERUSERDATABASE_H
#pragma once

#include "interface.h"
#include "TrackerCmdID.h"
#include "UtlVector.h"

class ISQLDBReplyTarget;

//-----------------------------------------------------------------------------
// Purpose: Defines the interface to the main tracker user database
//			These function map to remote SQL stored procedures of the same name
//-----------------------------------------------------------------------------
class ITrackerUserDatabase : public IBaseInterface
{
public:
	// server initialization
	virtual bool Initialize(unsigned int serverID, const char *serverName, const char *catalogName) = 0;

	// Dispatches responses to SQLDB queries
	virtual bool RunFrame() = 0;

	// load info - returns the number of sql db queries waiting to be processed
	virtual int QueriesInOutQueue() = 0;

	// number of queries finished processing, waiting to be responded to
	virtual int QueriesInFinishedQueue() = 0;

	// cleanup
	virtual void deleteThis() = 0;

	////////////////////////////////////////////////////////////////////////////////
	// NON-BLOCKING SQLDB FUNCTIONS

	// values returned via callback interface ISQLDBReplyTarget

	// creates a new user; userID must have been reserved by ReserveUserID
	virtual void User_Create(ISQLDBReplyTarget *replyTarget, unsigned int userID, const char *email, const char *password, const char *userName, const char *firstName, const char *lastName, unsigned int IP, unsigned int port) = 0;
	struct UserCreate_t
	{
		unsigned int IP;
		unsigned int Port;
	};

	// logs the user onto the server
	// returns 0 == success, 1 == bad password, 2 == no such user
	virtual void User_Login(ISQLDBReplyTarget *replyTarget, unsigned int userID, const char *password, unsigned int userIP, unsigned short userPort, unsigned int sessionID, int status, int buildNumber) = 0;
	struct UserLogin_t
	{
		int acquiredStatus;
	};
	
	// checks to see if the user exists; returns the userID if yes, -1 if the user doesn't exist, -2 if password is wrong
	virtual void User_Validate(ISQLDBReplyTarget *replyTarget, const char *email, const char *password, unsigned int IP, unsigned int port, unsigned int temporaryID) = 0;
	struct UserValidate_t
	{
		unsigned int IP;
		unsigned int Port;
		unsigned int temporaryID;
	};
	
	// finds a list of users based on the search criteria
	// < 1 failure, > 0 is the uid of the user found
	//!! needs to be moved to ITrackerFindDatabase
	virtual void Find_Users(ISQLDBReplyTarget *replyTarget, unsigned int iUID, const char *email, const char *userName, const char *firstName, const char *lastName) = 0;
	enum { MAX_FIND_USERS = 32 };
	struct FindUsers_t
	{
		int usersReturned;
		unsigned int userID[MAX_FIND_USERS];
		char userName[MAX_FIND_USERS][24];
		char firstName[MAX_FIND_USERS][24];
		char lastName[MAX_FIND_USERS][24];
	};

	// checks to see if user 'iUID' is authorized to see user 'targetID'
	struct IsAuthed_t
	{
		unsigned int targetID;
		int authLevel;
		bool blocked;
		char extraData[128];
	};
	virtual void User_IsAuthed(ISQLDBReplyTarget *replyTarget, int returnState, unsigned int iUID, int targetID, const char *extraData) = 0;

	// gets information about the user
	struct GetInfo_t
	{
		unsigned int targetID;
		char userName[32];
		char firstName[32];
		char lastName[32];
		char email[128];
	};
	virtual void User_GetInfo(ISQLDBReplyTarget *replyTarget, int returnState, unsigned int iUID) = 0;

	// logs user out of database
	// returns 1 on success, 0 on failure
	// will only fail if the user has logged into a different server in the meantime
	virtual void User_Logoff(ISQLDBReplyTarget *replyTarget, unsigned int userID, bool forceLogoff) = 0;

	// holds a single watcher
	struct watcher_t
	{
		unsigned int userID;
		unsigned int userSessionID;
		unsigned int userServerID;
	};
	// returns the number of watchers found
	// the data pointer points to the Start of a watcher_t list
	virtual void User_GetWatchers(ISQLDBReplyTarget *replyTarget, unsigned int iUID) = 0;

	// holds a single friend
	struct user_t
	{
		unsigned int userID;
		unsigned int userSessionID;
		unsigned int userIP;
		unsigned int userPort;
		int userStatus;
		unsigned int userServerID;
	};
	// returns the details of a logged in session
	// return value is a user_t struct
	virtual void User_GetSessionInfo(ISQLDBReplyTarget *replyTarget, int returnState, unsigned int iUID) = 0;

	// returns all our friends that we are authorized to see
	struct simpleuser_t
	{
		unsigned int userID;
	};
	virtual void User_GetFriendList(ISQLDBReplyTarget *replyTarget, unsigned int iUID) = 0;
	
	// returns the status of a set of friends
	// return value is the Start of the user_t list, returnVal is the number of friends returned
	virtual void User_GetFriendsStatus(ISQLDBReplyTarget *replyTarget, unsigned int userID, simpleuser_t *friends, int friendCount) = 0;

	struct gameuser_t
	{
		unsigned int userID;
		int userStatus;
		unsigned int gameIP;
		unsigned int gamePort;
		char gameType[32];
	};
	virtual void User_GetFriendsGameStatus(ISQLDBReplyTarget *replyTarget, unsigned int userID, simpleuser_t *friends, int friendCount) = 0;
	// returns the game status of a set of friends (whom we already know to be in a game)
	// this could be merged with User_GetFriendsStatus(), but i Think this is more optimal since it's a lot rarer

	// returns a single message - returns 0 when no more messages
	struct UserMessage_t
	{
		int msgID;
		unsigned int fromUserID;
		int flags;
		char name[32];
		char text[256];
		int mininumBuild;
		int maximumBuild;
	};
	virtual void User_GetMessage(ISQLDBReplyTarget *replyTarget, unsigned int userID) = 0;

	// deletes a message, return true if more messages remain
	virtual void User_DeleteMessage(ISQLDBReplyTarget *replyTarget, unsigned int iUID, int iMsgID) = 0;

	// returns the number of users in the range still online
	virtual void Users_AreOnline(ISQLDBReplyTarget *replyTarget, unsigned int lowerUserIDRange, unsigned int upperUserIDRange) = 0;

	// gets complete information about a set of users
	struct completeuser_t
	{
		unsigned int userID;
		char userName[32];
		char firstName[32];
		char lastName[32];
		char emailAddress[64];
		char password[32];
		char firstLogin[15];	// timestamp
		char lastLogin[15];		// timestamp
		int loginCount;
	};
	struct completeauth_t
	{
		unsigned int userID;
		unsigned int targetID;
		int authLevel;
	};
	struct completeblocked_t
	{
		unsigned int userID;
		unsigned int blockedID;
	};
	struct completemessage_t
	{
		// int messageID;  auto-increment variable, not copied
		unsigned int fromUserID;
		unsigned int toUserID;
		char dateSent[15];	// timestamp
		char body[255];
		int flags;
		char fromUserName[32];
		char deleteDate[15];
		int minimumBuild;
		int maximumBuild;
	};
	// holds the complete data for a set of users
	struct completedata_t
	{
		CUtlVector<completeuser_t> user;
		CUtlVector<completeauth_t> userauth;
		CUtlVector<completeblocked_t> userblocked;
		CUtlVector<completemessage_t> message;
	};

	// returns the complete set of data for a set of users
	virtual void Users_GetCompleteUserData(ISQLDBReplyTarget *replyTarget, unsigned int lowerRange, unsigned int upperRange, completedata_t *data) = 0;

	// inserts a range of users into the database
	virtual void Users_InsertCompleteUserData(ISQLDBReplyTarget *replyTarget, unsigned int lowerRange, unsigned int upperRange, const completedata_t *data) = 0;

	////////////////////////////////////////////////////////////////////////////////
	// NON-REPLYING NON-BLOCKING FUNCTIONS

	// deletes a range of users
	virtual void Users_DeleteRange(unsigned int lowerRange, unsigned int upperRange) = 0;

	// updates the users status in the database
	virtual void User_Update( unsigned int userID, int newStatus, unsigned int gameIP, unsigned int gamePort, const char *gameType ) = 0;

	// sets the auth state between two users
	// needs to be Run on both users servers
	virtual void User_SetAuth( unsigned int iUID, unsigned int targetID, int authLevel ) = 0;

	// Breaks the authorization links between two users, including watch tables
	// needs to be Run on both users servers
	virtual void User_RemoveAuth( unsigned int iUID, unsigned int targetID ) = 0;

	// sets blocked status
	virtual void User_SetBlock( unsigned int iUID, unsigned int blockedID, bool block ) = 0;

	// sets info
	virtual void User_SetInfo(unsigned int userID, const char *userName, const char *firstName, const char *lastName) = 0;

	// sends a message
	virtual void User_PostMessage(unsigned int fromUID, unsigned int toUID, const char *messageType, const char *body, const char *fromName, int flags) = 0;

	// adds watches to a set of users on this server
	virtual void User_AddWatcher(unsigned int userID, unsigned int sessionID, unsigned int serverID, simpleuser_t *friends, int friendCount) = 0;

	// adds a single watch on a user
	virtual void User_AddSingleWatch(unsigned int userID, unsigned int sessionID, unsigned int serverID, unsigned int friendID) = 0;

	// removes a watch from the server.  userID is probably a user on another server.
	virtual void User_RemoveWatcher(unsigned int userID) = 0;

	////////////////////////////////////////////////////////////////////////////////
	// BLOCKING SQLDB FUNCTIONS 

	// don't don't return until the function completes

	// clears invalid users out of the database
	virtual void Users_Flush() = 0;
};

#define TRACKERUSERDATABASE_INTERFACE_VERSION "TrackerUserDatabase001"

#endif // TRACKERUSERDATABASE_H
