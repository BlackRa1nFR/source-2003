//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "MySqlDatabase.h"

#include "../public/ITrackerUserDatabase.h"
#include "TrackerMessageFlags.h"

#include "DebugConsole_Interface.h"
extern IDebugConsole *g_pConsole;

extern void v_strncpy(char *dest, const char *src, int bufsize);

#include "MemPool.h"

//-----------------------------------------------------------------------------
// Purpose: MySql interface to database
//-----------------------------------------------------------------------------
class CMySqlUserDatabase : public ITrackerUserDatabase
{
public:
	CMySqlUserDatabase();

	// initializes the server to use the specified database
	virtual bool Initialize(unsigned int serverID, const char *serverName, const char *catalogName);

	// Dispatches responses to SQLDB queries
	virtual bool RunFrame();

	// load info - returns the number of sql db queries waiting to be processed
	virtual int QueriesInOutQueue();

	// number of queries finished processing, waiting to be responded to
	virtual int QueriesInFinishedQueue();

	// cleanup
	virtual void deleteThis();

	////////////////////////////////////////////////////////////////////////////////
	// NON-BLOCKING SQLDB FUNCTIONS

	// values returned via callback interface ISQLDBReplyTarget

	// creates a new user; the userID must have been reserved with ReserveUserID()
	virtual void User_Create(ISQLDBReplyTarget *replyTarget, unsigned int userID, const char *email, const char *password, const char *userName, const char *firstName, const char *lastName, unsigned int IP, unsigned int port);

	// logs the user onto the server
	// returns 0 == success, 1 == bad password, 2 == no such user
	virtual void User_Login(ISQLDBReplyTarget *replyTarget, unsigned int userID, const char *password, unsigned int userIP, unsigned short userPort, unsigned int sessionID, int status, int buildNumber);
	
	// checks to see if the user exists; returns the userID if yes, -1 if the user doesn't exist, -2 if password is wrong
	virtual void User_Validate(ISQLDBReplyTarget *replyTarget, const char *email, const char *password, unsigned int IP, unsigned int port, unsigned int temporaryID);
	
	// finds a list of users based on the search criteria
	// < 1 failure, > 0 is the uid of the user found
	//!! needs to be moved to ITrackerFindDatabase
	virtual void Find_Users(ISQLDBReplyTarget *replyTarget, unsigned int userID, const char *email, const char *userName, const char *firstName, const char *lastName);

	// returns the details of a logged in session
	virtual void User_GetSessionInfo(ISQLDBReplyTarget *replyTarget, int returnState, unsigned int userID);

	// logs user out of database
	// returns 1 on success, 0 on failure
	// will only fail if the user has logged into a different server in the meantime
	virtual void User_Logoff(ISQLDBReplyTarget *replyTarget, unsigned int userID, bool forceLogoff);

	// checks to see if user 'userID' is authorized to see user 'targetID'
	virtual void User_IsAuthed(ISQLDBReplyTarget *replyTarget, int returnState, unsigned int userID, int targetID, const char *extraData = NULL);

	// gets information about the user
	virtual void User_GetInfo(ISQLDBReplyTarget *replyTarget, int returnState, unsigned int userID);

	// returns the number of watchers found
	virtual void User_GetWatchers(ISQLDBReplyTarget *replyTarget, unsigned int userID);

	// returns all our friends that we are authorized to see
	virtual void User_GetFriendList(ISQLDBReplyTarget *replyTarget, unsigned int userID);

	// returns the status of a set of friends
	// return value is the Start of the user_t list, returnVal is the number of friends returned
	virtual void User_GetFriendsStatus(ISQLDBReplyTarget *replyTarget, unsigned int userID, simpleuser_t *friends, int friendCount);

	// gets game information about friends
	virtual void User_GetFriendsGameStatus(ISQLDBReplyTarget *replyTarget, unsigned int userID, simpleuser_t *friends, int friendCount);

	// returns a single message - returns 0 when no more messages
	virtual void User_GetMessage(ISQLDBReplyTarget *replyTarget, unsigned int userID);

	// deletes a message
	virtual void User_DeleteMessage(ISQLDBReplyTarget *replyTarget, unsigned int userID, int iMsgID);

	// returns the number of users in the range still online
	virtual void Users_AreOnline(ISQLDBReplyTarget *replyTarget, unsigned int lowerUserIDRange, unsigned int upperUserIDRange);

	// returns the complete set of data for a set of users
	virtual void Users_GetCompleteUserData(ISQLDBReplyTarget *replyTarget, unsigned int lowerRange, unsigned int upperRange, completedata_t *data);

	// inserts a range of users into the database
	virtual void Users_InsertCompleteUserData(ISQLDBReplyTarget *replyTarget, unsigned int lowerRange, unsigned int upperRange, const completedata_t *data);

	////////////////////////////////////////////////////////////////////////////////
	// NON-REPLYING NON-BLOCKING FUNCTIONS

	virtual void Users_DeleteRange(unsigned int lowerUserIDRange, unsigned int upperUserIDRange);

	// updates the users status in the database
	virtual void User_Update( unsigned int userID, int newStatus, unsigned int gameIP, unsigned int gamePort, const char *gameType );

	// adds to a users auth list
	virtual void User_SetAuth( unsigned int userID, unsigned int targetID, int authLevel );

	// Breaks the authorization links between two users
	// needs to be Run on both users servers
	virtual void User_RemoveAuth( unsigned int userID, unsigned int targetID );

	// sets blocked status
	virtual void User_SetBlock( unsigned int userID, unsigned int blockedID, bool block );

	// sets info
	virtual void User_SetInfo(unsigned int userID, const char *userName, const char *firstName, const char *lastName);

	// sends a message
	virtual void User_PostMessage(unsigned int fromUID, unsigned int toUID, const char *messageType, const char *body, const char *fromName, int flags);

	// adds watches to a set of users on this server
	virtual void User_AddWatcher(unsigned int userID, unsigned int sessionID, unsigned int serverID, simpleuser_t *friends, int friendCount);

	// adds a single watch on a user
	virtual void User_AddSingleWatch(unsigned int userID, unsigned int sessionID, unsigned int serverID, unsigned int friendID);

	// removes a watch from the server.  userID is probably a user on another server.
	virtual void User_RemoveWatcher(unsigned int userID);

	////////////////////////////////////////////////////////////////////////////////
	// BLOCKING SQLDB FUNCTIONS 

	// don't don't return until the function completes

	// clears invalid users out of the database
	virtual void Users_Flush();

	// removes
	virtual void Users_KillAllUsersFromServer();

	// METHODS

private:
	CMySqlDatabase m_DB;

	// our own unique server identification number
	unsigned int m_iServerID;
};

EXPOSE_INTERFACE(CMySqlUserDatabase, ITrackerUserDatabase, TRACKERUSERDATABASE_INTERFACE_VERSION);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMySqlUserDatabase::CMySqlUserDatabase()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : serverID - 
//			*serverName - 
//			*catalogName - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMySqlUserDatabase::Initialize(unsigned int serverID, const char *serverName, const char *catalogName)
{
	m_iServerID = serverID;

	if (!m_DB.Initialize(serverName, catalogName))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches responses to SQLDB queries
//-----------------------------------------------------------------------------
bool CMySqlUserDatabase::RunFrame()
{
	return m_DB.RunFrame();
}

//-----------------------------------------------------------------------------
// Purpose: load info - returns the number of sql db queries waiting to be processed
// Output : int
//-----------------------------------------------------------------------------
int CMySqlUserDatabase::QueriesInOutQueue()
{
	return m_DB.QueriesInOutQueue();
}

//-----------------------------------------------------------------------------
// Purpose: number of queries finished processing, waiting to be responded to
// Output : int
//-----------------------------------------------------------------------------
int CMySqlUserDatabase::QueriesInFinishedQueue()
{
	return m_DB.QueriesInFinishedQueue();
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down the DB and deletes it
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::deleteThis()
{
	delete this;
}

static void FixString(const char *string, char *result, int size)
{
	// look for any quote characters
	char *dest = result;
	const char *src = string;

	while (*src != 0)
	{
		if (*src == '\'' || *src == '\"')
		{
			// write in an extra '\' character
			*dest = '\\';
			dest++;
		}

		*dest = *src;
		src++;
		dest++;
	}
	*dest = 0;
}

////////////////////////////////////////////////////////////////////////////////
// NON-BLOCKING SQLDB FUNCTIONS

// values returned via callback interface ISQLDBReplyTarget

//-----------------------------------------------------------------------------
// Purpose: Create user DB command
//			returns 0 == failure, > 0 new UID, -1 existing user but password wrong
//-----------------------------------------------------------------------------
class CCmdUserCreate : public ISQLDBCommand
{
private:
	ITrackerUserDatabase::UserCreate_t data;
	char m_szEmail[128];
	char m_szPassword[32];
	char m_szUserName[32];
	char m_szFirstName[32];
	char m_szLastName[32];
	unsigned int m_iUserID;

public:
	CCmdUserCreate()
	{
	}

	// set up the command
	void Init(unsigned int userID, const char *email, const char *password, const char *userName, const char *firstName, const char *lastName, unsigned int IP, unsigned int port)
	{
		// fill in the command
		m_iUserID = userID;
		FixString(email, m_szEmail, 127 );
		FixString(password, m_szPassword, 31);
		FixString(userName, m_szUserName, 31);
		FixString(firstName, m_szFirstName, 31);
		FixString(lastName, m_szLastName, 31);

		data.IP = IP;
		data.Port = port;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// check to see if the user already exists
		query << "select userID from User where emailAddress = " << m_szEmail;
		Result res = query.store(RESET_QUERY);
		if (res)
		{
			// user already exists, exit
			return -1;
		}

		// insert user into database
		query << "insert into User ( userId, firstName, lastName, nickName,	password, emailAddress ) values ( "
				<< m_iUserID << ", \""
				<< m_szFirstName << "\", \""
				<< m_szLastName << "\", \""
				<< m_szUserName << "\", \""
				<< m_szPassword << "\", \""
				<< m_szEmail << "\" )";

		query.execute();

		if (!query.success())
		{
			g_pConsole->Print(4, "** User_Create Error: %s\n", query.error().c_str());
			m_iUserID = -2;
		}

		return m_iUserID;
	}

	// return data
	virtual void *GetReturnData()
	{
		return &data;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_CREATE;
	}

	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserCreate);

////////////////////////////////////////////////////////////////////////////////
// NON-REPLYING NON-BLOCKING FUNCTIONS

//-----------------------------------------------------------------------------
// Purpose: Command to access above
//-----------------------------------------------------------------------------
class CCmdUserLogin : public ISQLDBCommand
{
private:
	ITrackerUserDatabase::UserLogin_t data;
	enum errorCodes
	{
		ERROR_NONE = 0,
		ERROR_BADPASSWORD = -1,
		ERROR_NOSUCHUSER = -2,
		ERROR_ALREADYLOGGEDON = -3
	};

	int m_iResult;	// 0 is success, any other value an error code
	unsigned int m_iServerID;
	unsigned int m_iUserID;
	char m_szPassword[32];
	unsigned int m_iUserIP;
	unsigned int m_iUserPort;
	unsigned int m_iSessionID;
	int m_iStatus;
	int m_iBuild;

public:
	CCmdUserLogin()
	{
	}

	// set up the command
	void Init(unsigned int serverID, unsigned int userID, const char *password, int userIP, int userPort, unsigned int sessionID, int status, int build)
	{
		m_iResult = ERROR_NONE;
		m_iServerID = serverID;
		m_iUserID = userID;
		v_strncpy(m_szPassword, password, 31);
		m_iUserIP = userIP;
		m_iUserPort = userPort;
		m_iSessionID = sessionID;
		m_iStatus = status;
		m_iBuild = build;

		data.acquiredStatus = status;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// find the user in the database
		query << "select password from User where userID = " << m_iUserID;
		Result res = query.store(RESET_QUERY);

		if (!res)
		{
			g_pConsole->Print(4, "** User_Login Error: %s\n", connection.error().c_str());
			return 0;
		}

		if (!res.size())
		{
			// no user found
			return ERROR_NOSUCHUSER;
		}

		// check password
		Row row = res[0];
		string password = row[0];
		if (strcmp(m_szPassword, password.c_str()))
		{
			// incorrect password
			return ERROR_BADPASSWORD;
		}

		// we have a successful user and password, log them in

		// see if they are already logged on
		query << "select status, userIP, userPort from UserStatus where userID = " << m_iUserID;
		res = query.store(RESET_QUERY);
		if (!res || res.size() < 1)
		{
			// we're not in the table, insert
			query << "insert into UserStatus ( userID, serverID, status, userIP, userPort, sessionID, lastModified, buildNumber )"
					<< " values ( " << m_iUserID 
					<< ", " << m_iServerID
					<< ", " << m_iStatus
					<< ", " << m_iUserIP
					<< ", " << m_iUserPort
					<< ", " << m_iSessionID
					<< ", NULL, " << m_iBuild
					<< " ) ";

			// Run the status insert query
			query.execute();

			if (!query.success())
			{
				g_pConsole->Print(4, "** User_Login (2) Error: %s\n", query.error().c_str());
				return 0;
			}
		}
		else
		{
			// we're already in the table, check to see if we're already logged in
			row = res[0];
			int status = row[0];
			unsigned int IP = row[1];
			unsigned int port = row[2];

			if (status > 0)
			{
				// we're already logged in

				// if we're at the same IP, just override connection
				if (IP != m_iUserIP)
				{
					return ERROR_ALREADYLOGGEDON;
				}
			}

			// update the table
			query << "update UserStatus set"
					<< " status = "	<< m_iStatus
					<< ", userIP = " << m_iUserIP
					<< ", userPort = " << m_iUserPort
					<< ", sessionID = " << m_iSessionID
					<< ", serverID = " << m_iServerID
					<< ", buildNumber = " << m_iBuild
					<< ", lastModified = NULL"
					<< " where UserStatus.userid = " << m_iUserID;

			query.execute();

			if (!query.success())
			{
				g_pConsole->Print(4, "** User_Login (3) Error: %s\n", query.error().c_str());
				return 0;
			}
		}

		return 1;
	}

	// return data
	virtual void *GetReturnData()
	{
		return &data;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_LOGIN;
	}

	virtual void GetDebugInfo(char *buf, int bufSize)
	{
		sprintf(buf, "User_Login Parameters:\n\t@serverID = %d\n\t@userId=%d\n\t@password = %s\n\t@userIp = %d\n\t@userPort = %d\n\t@sessionID = %d\n\t@status = %d\n\t@build=%d\n", m_iServerID, m_iUserID, m_szPassword, m_iUserIP, m_iUserPort, m_iSessionID, m_iStatus, m_iBuild);
	}

	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserLogin);

//-----------------------------------------------------------------------------
// Purpose: Command to access above
//-----------------------------------------------------------------------------
class CCmdUserValidate : public ISQLDBCommand
{
private:
	ITrackerUserDatabase::UserValidate_t data;
	char m_szEmail[128];
	char m_szPassword[32];

public:
	CCmdUserValidate()
	{
	}

	// set up the command
	void Init(const char *email, const char *password, unsigned int IP, unsigned int port, int temporaryID)
	{
		FixString(email, m_szEmail, 127);
		v_strncpy(m_szPassword, password, 31);

		data.IP = IP;
		data.Port = port;
		data.temporaryID = temporaryID;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		query << "select userID, password from User where emailAddress = '" << m_szEmail << "'";
		Result res = query.store(RESET_QUERY);

		if (!res || res.size() < 1)
		{
			// user not found, fail
			return -1;
		}

		Row row = res[0];

		unsigned int userID = row[0];
		string password = row[1];

		if (strcmp(password.c_str(), m_szPassword))
		{
			// password is incorrect
			return -2;
		}

		// user validated, return the correct user id
		return *((int *)(&userID));
	}

	// return data
	virtual void *GetReturnData()
	{
		return &data;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_VALIDATE;
	}

	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserValidate);

//-----------------------------------------------------------------------------
// Purpose: Logs the user off the server
//-----------------------------------------------------------------------------
class CCmdUserLogoff : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	unsigned int m_iServerID;
	bool m_bForceLogoff;

public:
	CCmdUserLogoff()
	{
	}

	// set up the command
	void Init(unsigned int userID, unsigned int serverID, bool forceLogoff)
	{
		m_iUserID = userID;
		m_iServerID = serverID;
		m_bForceLogoff = forceLogoff;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();
		Result res;

		// get our login state
		if (!m_bForceLogoff)
		{
			query << "select serverID from UserStatus where userID = " << m_iUserID;
			res = query.store(RESET_QUERY);
			if (!res)
			{
				g_pConsole->Print(4, "** User_Logoff Error: %s\n", query.error().c_str());
			}
			
			if (res.size() < 1)
			{
				// user not found
				return 0;
			}
			unsigned int serverID = (res[0])[0];

			// make sure the logoff is coming from the right server
			if (m_iServerID && serverID != m_iServerID)
			{
				// logoff from wrong server
				return 0;
			}
		}

		// mark user as offline
		query << "update UserStatus set status = 0, userIP = 0, userPort = 0, sessionID = 0 where userID = " << m_iUserID;
		query.execute();
		if (!query.success())
		{
			g_pConsole->Print(4, "** User_Logoff (2) Error: %s\n", query.error().c_str());
		}
		query.reset();

		return 2;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_LOGOFF;
	}

	virtual void GetDebugInfo(char *buf, int bufSize)
	{
		sprintf(buf, "User_Logoff Parameters: \n\t@userID = %d\n\t@serverID = %d\n", m_iUserID, m_iServerID);
	}

	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserLogoff);


//-----------------------------------------------------------------------------
// Purpose: Command to access above
//-----------------------------------------------------------------------------
class CCmdUserUpdate : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	int m_iStatus;
	unsigned int m_iGameIP;
	unsigned int m_iGamePort;
	char m_szGameType[32];

public:
	CCmdUserUpdate()
	{
	}

	// set up the command
	void Init(unsigned int userID, int status, int gameIP, int gamePort, const char *gameType)
	{
		m_iUserID = userID;
		m_iStatus = status;
		m_iGameIP = gameIP;
		if (gameIP)
		{
			m_iGamePort = gamePort;
			FixString(gameType, m_szGameType, sizeof(m_szGameType));
		}
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// update the users' status in the database
		if (m_iGameIP)
		{
			query << "update UserStatus set status = " << m_iStatus 
										<< ", gameIP = " << m_iGameIP
										<< ", gamePort = " << m_iGamePort
										<< ", gameType = '" << m_szGameType
										<< "' where userID = " << m_iUserID;
		}
		else
		{
			query << "update UserStatus set status = " << m_iStatus << " where userID = " << m_iUserID;
		}
		query.execute();

		if (!query.success())
		{
			g_pConsole->Print(4, "** User_Update Error: %s\n", query.error().c_str());
		}

		return 1;
	}

	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserUpdate);


//-----------------------------------------------------------------------------
// Purpose: Command that returns all our friends that we are authorized to see
//-----------------------------------------------------------------------------
class CCmdGetFriendsList : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	CUtlVector<ITrackerUserDatabase::simpleuser_t> data;

public:
	void Init(unsigned int userID)
	{
		m_iUserID = userID;
		data.RemoveAll();
	}
	
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		query << "select targetID from UserAuth where userID = "
			<< m_iUserID
			<< " and authLevel > 4";
		Result res = query.store();

		if (!query.success())
		{
			g_pConsole->Print(4, "** User_GetFriendList Error: %s\n", query.error().c_str());
			return 0;
		}

		// write out result into data
		int friendCount = res.size();
		if (friendCount < 1)
			return 0;

		data.AddMultipleToTail(friendCount);

		// copy out all the user id's
		for (int i = 0; i < friendCount; i++)
		{
			data[i].userID = (res[i])[0];
		}
	
		return friendCount;
	}

	// return data
	virtual void *GetReturnData()
	{
		return data.Base();
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_GETFRIENDLIST;
	}

	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdGetFriendsList);


//-----------------------------------------------------------------------------
// Purpose: Command that adds watches to a set of users on this server
//			Does not return
//-----------------------------------------------------------------------------
class CCmdUserAddWatcher : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	unsigned int m_iSessionID;
	unsigned int m_iServerID;
	CUtlVector<ITrackerUserDatabase::simpleuser_t> data;

public:
	void Init(unsigned int userID, unsigned int sessionID, unsigned int serverID, ITrackerUserDatabase::simpleuser_t *friends, int friendCount)
	{
		m_iUserID = userID;
		m_iSessionID = sessionID;
		m_iServerID = serverID;
		data.CopyArray(friends, friendCount);
	}
	
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		bool bSuccess = false;
		while (!bSuccess)
		{
			bSuccess = true;

			// insert the watcher
			query << "insert into Watcher ( watcherID, watcherSessionID, watcherServerID ) values ( "
					<< m_iUserID << ", "
					<< m_iSessionID << ", "
					<< m_iServerID << " )";

			query.execute(RESET_QUERY);
			if (!query.success())
			{
				bSuccess = false;
				// g_pConsole->Print(4, "** User_AddWatcher (3) Error: %s\n", query.error().c_str());
			}

			// add into watchmap
			if (bSuccess)
			{
				query << "insert into WatchMap ( watcherID, targetID ) values ";
				for (int i = 0; i < data.Size(); i++)
				{
					query << "( " << m_iUserID << ", " << data[i].userID << " )";
					
					if ((i + 1) < data.Size())
					{
						query << ", ";
					}
				}
				query.execute();
				if (!query.success())
				{
					bSuccess = false;
					// g_pConsole->Print(4, "** User_AddWatcher (4) Error: %s\n", query.error().c_str());
				}
			}

			if (!bSuccess)
			{
				g_pConsole->Print(4, "User_AddWatcher: cleaning dead entries out of sqldb for user '%d'\n", m_iUserID);

				// looks like we have some dead entries; clear us out of the database
				// make sure we're cleared out of the database already
				query << "delete from Watcher where watcherID = " << m_iUserID;
				query.execute(RESET_QUERY);
				if (!query.success())
				{
					g_pConsole->Print(4, "** User_AddWatcher Error: %s\n", query.error().c_str());
				}

				query << "delete from WatchMap where watcherID = " << m_iUserID;
				query.execute(RESET_QUERY);
				if (!query.success())
				{
					g_pConsole->Print(4, "** User_AddWatcher (2) Error: %s\n", query.error().c_str());
				}
			}
		}
			
		return 1;
	}

	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserAddWatcher);


//-----------------------------------------------------------------------------
// Purpose: adds a single watch to a user
//-----------------------------------------------------------------------------
class CCmdUserAddSingleWatch : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	unsigned int m_iSessionID;
	unsigned int m_iServerID;
	unsigned int m_iFriendID;

public:
	void Init(unsigned int userID, unsigned int sessionID, unsigned int serverID, unsigned int friendID)
	{
		m_iUserID = userID;
		m_iSessionID = sessionID;
		m_iServerID = serverID;
		m_iFriendID = friendID;
	}

	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// insert the watcher
		query << "insert ignore into Watcher ( watcherID, watcherSessionID, watcherServerID ) values ( "
				<< m_iUserID << ", "
				<< m_iSessionID << ", "
				<< m_iServerID << " )";

		query.execute(RESET_QUERY);
		if (!query.success())
		{
			g_pConsole->Print(4, "** User_AddSingleWatch Error: %s\n", query.error().c_str());
		}

		// add into watchmap
		query << "insert ignore into WatchMap ( watcherID, targetID ) values ( " << m_iUserID << ", " << m_iFriendID << " )";
		query.execute();
		if (!query.success())
		{
			g_pConsole->Print(4, "** User_AddSingleWatch (2) Error: %s\n", query.error().c_str());
		}

		return 1;
	}

	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserAddSingleWatch);


//-----------------------------------------------------------------------------
// Purpose: command to removes all watches from this server, from Watcher and WatchMap
//-----------------------------------------------------------------------------
class CCmdUserRemoveWatcher : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;

public:
	void Init(unsigned int userID)
	{
		m_iUserID = userID;
	}
	
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// clear the watch map first
		query << "delete from WatchMap where watcherID = " << m_iUserID;
		query.execute();
		// check for errors
		if (!query.success())
		{
			g_pConsole->Print(4, "** User_RemoveWatcher Error: %s\n", query.error().c_str());
		}
		query.reset();

		// delete from watch table
		query << "delete from Watcher where watcherID = " << m_iUserID;
		query.execute();
		if (!query.success())
		{
			g_pConsole->Print(4, "** User_RemoveWatcher (2) Error: %s\n", query.error().c_str());
		}
		query.reset();

		return 1;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserRemoveWatcher);


//-----------------------------------------------------------------------------
// Purpose: Command that gets a set of friends stati
//-----------------------------------------------------------------------------
class CCmdUserGetFriendsStatus : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	CUtlVector<ITrackerUserDatabase::user_t> data;

public:
	void Init(unsigned int userID, ITrackerUserDatabase::simpleuser_t *friends, int friendCount)
	{
		m_iUserID = userID;
		data.SetSize(friendCount);
		for (int i = 0; i < friendCount; i++)
		{
			data[i].userID = friends[i].userID;
			data[i].userStatus = 0;
			data[i].userIP = 0;
			data[i].userPort = 0;
			data[i].userSessionID = 0;
			data[i].userServerID = 0;
		}
	}
	
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// query all the specified users
		query << "select userID, status, userIP, userPort, sessionID, serverID "
				<< "from UserStatus where userID in (";

		// list all the friends
		int i = 0;
		while (1)
		{
			int id = data[i].userID;
			query << " " << id;

			i++;
			if (i >= data.Size())
				break;

			query << ",";
		}
		query << " ) order by userID";
		// return is ordered by userID to make the list merging faster
		// we need to merge the db list and local list since the db UserStatus list may not have all the users in it
		// the incoming list is assumed to be sorted

		Result res = query.store();
		if (!res)
		{
			g_pConsole->Print(4, "** User_GetFriendsStatus Error: %s\n", query.error().c_str());
			return -1;
		}

		// get the number of users
		int count = res.size();
			
		// parse out list
		int dataRow = 0;
		for (i = 0; i < count && dataRow < data.Size(); i++)
		{
			Row row = res[i];

			// find the db row in the local list
			unsigned int userID = row[0];
			while (data[dataRow].userID != userID && dataRow < data.Size())
			{
				// user not in userstatus, skip this item
				dataRow++;
			}

			if (dataRow >= data.Size())
				break;

			// user is there, copy out their details
			data[dataRow].userStatus = row[1];
			data[dataRow].userIP = row[2];
			data[dataRow].userPort = row[3];
			data[dataRow].userSessionID = row[4];
			data[dataRow].userServerID = row[5];

			dataRow++;
		}

		return data.Size();
	}

	// return data
	virtual void *GetReturnData()
	{
		return data.Base();
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_GETFRIENDSTATUS;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserGetFriendsStatus);


//-----------------------------------------------------------------------------
// Purpose: Command that gets a set of friends game information
//-----------------------------------------------------------------------------
class CCmdUserGetFriendsGameStatus : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	CUtlVector<ITrackerUserDatabase::gameuser_t> data;

public:
	void Init(unsigned int userID, ITrackerUserDatabase::simpleuser_t *friends, int friendCount)
	{
		m_iUserID = userID;
		data.SetSize(friendCount);
		for (int i = 0; i < friendCount; i++)
		{
			data[i].userID = friends[i].userID;
		}
	}
	
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		if (data.Size() == 1)
		{
			// query the single user
			query << "select userID, status, gameIP, gamePort, gameType "
					<< "from UserStatus where userID = " << data[0].userID;
		}
		else
		{
			// query all the specified users
			query << "select userID, status, gameIP, gamePort, gameType "
					<< "from UserStatus where userID in (";

			// list all the friends
			int i = 0;
			while (1)
			{
				int id = data[i].userID;
				query << " " << id;

				i++;
				if (i >= data.Size())
					break;

				query << ",";
			}
			query << " ) order by userID";
		}

		Result res = query.store();
		if (!res)
		{
			g_pConsole->Print(4, "** User_GetFriendsGameStatus Error: %s\n", query.error().c_str());
			return -1;
		}

		// get the number of users
		int count = res.size();
			
		// parse out list
		int dataRow = 0;
		for (int i = 0; i < count && dataRow < data.Size(); i++)
		{
			Row row = res[i];

			// find the db row in the local list
			unsigned int userID = row[0];
			while (data[dataRow].userID != userID && dataRow < data.Size())
			{
				// we've skipped this item; null it out and move to next one
				data[dataRow].userStatus = 0;
				data[dataRow].gameIP = 0;
				data[dataRow].gamePort = 0;
				data[dataRow].gameType[0] = 0;

				dataRow++;
			}

			if (dataRow >= data.Size())
				break;

			// user is there, copy out their details
			data[dataRow].userStatus = row[1];
			data[dataRow].gameIP = row[2];
			data[dataRow].gamePort = row[3];
			strncpy(data[dataRow].gameType, row[4].c_str(), 31);
			data[dataRow].gameType[31] = 0;

			dataRow++;
		}

		return data.Size();
	}

	// return data
	virtual void *GetReturnData()
	{
		return data.Base();
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_GETFRIENDSGAMESTATUS;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserGetFriendsGameStatus);


//-----------------------------------------------------------------------------
// Purpose: Finds a set of users from the database
//-----------------------------------------------------------------------------
class CCmdFindUsers : public ISQLDBCommand
{
private:
	ITrackerUserDatabase::FindUsers_t data;

	unsigned int m_iUserID;
	char m_szEmail[128];
	char m_szUserName[32];
	char m_szFirstName[32];
	char m_szLastName[32];

public:
	CCmdFindUsers()
	{
	}

	// set up the command
	void Init(unsigned int userID, const char *email, const char *userName, const char *firstName, const char *lastName)
	{
		// fill in the command
		m_iUserID = userID;
		v_strncpy(m_szEmail, email, 128);
		v_strncpy(m_szUserName, userName, 32);
		v_strncpy(m_szFirstName, firstName, 32);
		v_strncpy(m_szLastName, lastName, 32);
		data.usersReturned = 0;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// set up the base query, then add the search criteria
		query << "select userID, nickName, firstName, lastName from User where ";

		// scan first by search ID
		if (m_iUserID)
		{
			query << "userID = " << m_iUserID;			
		}
		else
		{
			bool needAnd = false;
			if (strlen(m_szEmail) > 0)
			{
				query << "emailAddress LIKE '%" << m_szEmail << "%'";
				needAnd = true;
			}

			if (strlen(m_szUserName) > 0)
			{
				if (needAnd)
				{
					query << " and ";
				}
				query << "nickName LIKE '%" << m_szUserName << "%'";
				needAnd = true;
			}

			if (strlen(m_szFirstName) > 0)
			{
				if (needAnd)
				{
					query << " and ";
				}
				query << "firstName LIKE '%" << m_szFirstName << "%'";
				needAnd = true;
			}

			if (strlen(m_szLastName) > 0)
			{
				if (needAnd)
				{
					query << " and ";
				}
				query << "lastName LIKE '%" << m_szLastName << "%'";
				needAnd = true;
			}
		}

		// add the limit
		query << " LIMIT " << ITrackerUserDatabase::MAX_FIND_USERS;

		// Run the query
		Result res = query.store();

		if (!res)
		{
			g_pConsole->Print(5, "Users_Find error: %s\n", query.error().c_str());
			return 0;
		}
		data.usersReturned = res.size();

		// walk the list
		for (int i = 0; i < data.usersReturned; i++)
		{
			Row row = res[i];
			if (!row)
				continue;

			// extract row data
			int id = row[0];
			string user = row[1];
			string first = row[2];
			string last = row[3];

			// copy data into return struct
			data.userID[i] = id;
			v_strncpy(data.userName[i], user.c_str(), 24);
			v_strncpy(data.firstName[i], first.c_str(), 24);
			v_strncpy(data.lastName[i], last.c_str(), 24);
		}

		return data.usersReturned;
	}

	// return data
	virtual void *GetReturnData()
	{
		return &data;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_FINDUSERS;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdFindUsers);


//-----------------------------------------------------------------------------
// Purpose: Command to access above
//-----------------------------------------------------------------------------
class CCmdGetSessionInfo : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	ITrackerUserDatabase::user_t data;

public:
	CCmdGetSessionInfo()
	{
	}

	// set up the command
	void Init(unsigned int userID)
	{
		// fill in the command
		m_iUserID = userID;
		memset(&data, 0, sizeof(data));
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();
		query << "select status, sessionID, userIP, userPort, serverID from UserStatus where userID = " << m_iUserID;

		Result res = query.store();
		if (!res)
		{
			g_pConsole->Print(4, "** User_GetSessionInfo Error: %s\n", query.error().c_str());
			return -1;
		}

		if (res.size() < 1)
		{
			// no such user
			return -1;
		}

		// parse out results
		Row row = res[0];

		data.userID = m_iUserID;
		data.userStatus = row[0];
		data.userSessionID = row[1];
		data.userIP = row[2];
		data.userPort = row[3];
		data.userServerID = row[4];

		return 1;
	}

	// return data
	virtual void *GetReturnData()
	{
		return &data;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_GETSESSIONINFO;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdGetSessionInfo);


//-----------------------------------------------------------------------------
// Purpose: Command to access above
//-----------------------------------------------------------------------------
class CCmdUserSetAuth : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	int m_iTargetID;
	int m_iNewAuthLevel;

public:
	CCmdUserSetAuth()
	{
	}

	// set up the command
	void Init(unsigned int userID, int targetID, int authLevel)
	{
		// fill in command
		m_iUserID = userID;
		m_iTargetID = targetID;
		m_iNewAuthLevel = authLevel;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// insert the entry into the table
		query << "insert into UserAuth ( userID, targetID, authLevel ) values ( " << m_iUserID << ", " << m_iTargetID << ", " << m_iNewAuthLevel << " )";
		query.execute(RESET_QUERY);

		if (!query.success())
		{
			// there is probably already an existing entry, update it
			query << "update UserAuth set authLevel = " << m_iNewAuthLevel << " where userID = " << m_iUserID << " and targetID = " << m_iTargetID;
			query.execute(RESET_QUERY);

			if (!query.success())
			{
				g_pConsole->Print(4, "** User_SetAuth Error: %s\n", query.error().c_str());
				return -1;
			}
		}

		// add to watch map
		query << "insert ignore into WatchMap ( watcherID, targetID ) values ( " << m_iUserID << ", " << m_iTargetID << " )";
		query.execute();
		if (!query.success())
		{
			g_pConsole->Print(4, "** User_SetAuth (2) Error: %s\n", query.error().c_str());
			return -1;
		}

		return 1;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserSetAuth);


//-----------------------------------------------------------------------------
// Purpose: Completely removes the authorization link between two users on one server
//			needs to be Run on both users servers
//-----------------------------------------------------------------------------
class CCmdUserRemoveAuth : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	int m_iTargetID;

public:
	CCmdUserRemoveAuth()
	{
	}

	// set up the command
	void Init(unsigned int userID, int targetID)
	{
		// fill in command
		m_iUserID = userID;
		m_iTargetID = targetID;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// we just need to delete a whole bunch of links

		// remove from watchmap, to ensure the deauthorization is effective immediately
		query << "delete from WatchMap where watcherID = " << m_iUserID << " and targetID = " << m_iTargetID;
		query.execute(RESET_QUERY);
		query << "delete from WatchMap where watcherID = " << m_iTargetID << " and targetID = " << m_iUserID;
		query.execute(RESET_QUERY);

		if (!query.success())
		{
			g_pConsole->Print(4, "** User_RemoveAuth Error: %s\n", query.error().c_str());
			return -1;
		}

		// remove userID -> targetID link
		query << "delete from UserAuth where userID = " << m_iUserID << " and targetID = " << m_iTargetID;
		query.execute(RESET_QUERY);
		// remove targetID -> userID link
		query << "delete from UserAuth where userID = " << m_iTargetID << " and targetID = " << m_iUserID;
		query.execute(RESET_QUERY);

		if (!query.success())
		{
			g_pConsole->Print(4, "** User_RemoveAuth (2) Error: %s\n", query.error().c_str());
			return -1;
		}

		return 1;
	}

	virtual void GetDebugInfo(char *buf, int bufSize)
	{
		sprintf(buf, "User_RemoveAuth Parameters: \n\t@userID = %d\n\t@targetID = %d\n", m_iUserID, m_iTargetID);
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserRemoveAuth);


//-----------------------------------------------------------------------------
// Purpose: Command to access above
//-----------------------------------------------------------------------------
class CCmdUserSetBlock : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	int m_iBlockedID;
	int m_iBlock;

public:
	CCmdUserSetBlock()
	{
	}

	// set up the command
	void Init(unsigned int userID, int blockedID, bool block)
	{
		// fill in command
		m_iUserID = userID;
		m_iBlockedID = blockedID;
		m_iBlock = block;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();
		if (m_iBlock > 0)
		{
			// add the user to the blocked list
			query << "insert ignore into UserBlocked ( userID, blockedID ) values ( " << m_iUserID << ", " << m_iBlockedID << " )";
			query.execute();

			// there are no error checks for this; a primary key violation would simply mean the user is already blocked
		}
		else
		{
			// clear the block
			query << "delete from UserBlocked where userID = " << m_iUserID << " and blockedID = " << m_iBlockedID;
			query.execute();
		}

		return 1;
	}

	virtual void GetDebugInfo(char *buf, int bufSize)
	{
		sprintf(buf, "User_SetBlock Parameters: \n\t@userID = %d\n\t@blockedID = %d\n\t@block = %d\n", m_iUserID, m_iBlockedID, m_iBlock);
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserSetBlock);


//-----------------------------------------------------------------------------
// Purpose: Command to access above
//-----------------------------------------------------------------------------
class CCmdUserSetInfo : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	char m_szUserName[64];
	char m_szFirstName[64];
	char m_szLastName[64];

public:
	CCmdUserSetInfo()
	{
	}

	// set up the command
	void Init(unsigned int userID, const char *userName, const char *firstName, const char *lastName)
	{
		// fill in command
		m_iUserID = userID;
		FixString(userName, m_szUserName, 63);
		FixString(firstName, m_szFirstName, 63);
		FixString(lastName, m_szLastName, 63);
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// prevent users from having names short than 3 chars
		if (strlen(m_szUserName) > 2)
		{
			query << "update User set nickName = '" << m_szUserName 
				<< "', firstName = '" << m_szFirstName 
				<< "', lastName = '" << m_szLastName 
				<< "' where userID = " << m_iUserID;
		}
		else
		{
			query << "update User set lastName = '" << m_szLastName 
				<< "', firstName = '" << m_szFirstName 
				<< "' where userID = " << m_iUserID;
		}

		query.execute();

		if (!query.success())
		{
			g_pConsole->Print(4, "** User_SetInfo Error: %s\n", query.error().c_str());
			return -1;
		}

		return 1;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserSetInfo);


//-----------------------------------------------------------------------------
// Purpose: Queries auth state
//-----------------------------------------------------------------------------
class CCmdUserIsAuthed : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	ITrackerUserDatabase::IsAuthed_t data;

public:
	CCmdUserIsAuthed()
	{
	}

	// set up the command
	void Init(unsigned int userID, int targetID, const char *extraData)
	{
		// fill in command
		m_iUserID = userID;
		data.targetID = targetID;
		data.authLevel = 0;
		data.blocked = false;

		if (extraData)
		{
			v_strncpy(data.extraData, extraData, sizeof(data.extraData));
		}
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		// users are always authorized to see themselves
		if (m_iUserID == data.targetID)
			return true;

		Query query = connection.query();

		query << "select authLevel from UserAuth where userID = " << m_iUserID << " and targetID = " << data.targetID;
		Result res = query.store();
		if (!res)
		{
			g_pConsole->Print(4, "** User_IsAuthed Error: %s\n", query.error().c_str());
			return false;
		}

		if (res.size() < 1)
		{
			// no entry, no auth
			return false;
		}

		Row row = res[0];
		data.authLevel = row[0];

		if (data.authLevel < 1)
		{
			// check to see if we're blocked


			return false;
		}

		// passed, user is authenticated to see targetID
		return true;
	}

	// return data
	virtual void *GetReturnData()
	{
		return &data;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_ISAUTHED;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserIsAuthed);


//-----------------------------------------------------------------------------
// Purpose: Queries auth state
//-----------------------------------------------------------------------------
class CCmdUserGetInfo : public ISQLDBCommand
{
private:
	ITrackerUserDatabase::GetInfo_t data;

public:
	CCmdUserGetInfo()
	{
	}

	// set up the command
	void Init(unsigned int userID)
	{
		// fill in command
		data.targetID = userID;
		data.userName[0] = 0;
		data.firstName[0] = 0;
		data.lastName[0] = 0;
		data.email[0] = 0;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		query << "select nickName, firstName, lastName, emailAddress from User where userID = " << data.targetID;
		Result res = query.store();
		if (!res)
		{
			g_pConsole->Print(4, "** User_GetInfo Error: %s\n", query.error().c_str());
			return -1;
		}
		if (res.size() != 1)
		{
			g_pConsole->Print(4, "** User_GetInfo (2) Error: Could not find user\n", query.error().c_str());
			return -1;
		}

		Row row = res[0];

		v_strncpy(data.userName, row[0], sizeof(data.userName));
		v_strncpy(data.firstName, row[1], sizeof(data.firstName));
		v_strncpy(data.lastName, row[2], sizeof(data.lastName));
		v_strncpy(data.email, row[3], sizeof(data.email));

		return 1;
	}

	// return data
	virtual void *GetReturnData()
	{
		return &data;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_GETINFO;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserGetInfo);


//-----------------------------------------------------------------------------
// Purpose: Queries auth state
//-----------------------------------------------------------------------------
class CCmdUserGetWatchers : public ISQLDBCommand
{
private:
//	ITrackerUserDatabase::GetInfo_t data;
	unsigned int m_iUserID;
	CUtlVector<ITrackerUserDatabase::watcher_t> m_Watchers;

public:
	CCmdUserGetWatchers()
	{
	}

	// set up the command
	void Init(unsigned int userID)
	{
		// fill in command
		m_iUserID = userID;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// get all details of the users watching us
		query << "select Watcher.watcherID, watcherSessionID, watcherServerID from Watcher inner join WatchMap on Watcher.watcherID = WatchMap.watcherID where WatchMap.targetID = " << m_iUserID;

		Result res = query.store();

		if (!res)
		{
			g_pConsole->Print(4, "** User_GetWatchers Error: %s\n", query.error().c_str());
			return -1;
		}

		// get the number of users
		int count = res.size();
		m_Watchers.SetSize(count);

		// copy out list
		for (int i = 0; i < count; i++)
		{
			Row row = res[i];

			m_Watchers[i].userID = row[0];
			m_Watchers[i].userSessionID = row[1];
			unsigned int serverID = row[2];
			m_Watchers[i].userServerID = serverID;
		}

		return count;
	}

	void *GetReturnData()
	{
		return m_Watchers.Base();
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_GETWATCHERS;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserGetWatchers);


//-----------------------------------------------------------------------------
// Purpose: Command to access above
//-----------------------------------------------------------------------------
class CCmdUserPostMessage : public ISQLDBCommand
{
private:
	int m_iFromUID;
	int m_iToUID;
	char m_szMsgType[32];
	char m_szBody[255];
	char m_szFromName[32];
	int m_iFlags;

public:
	CCmdUserPostMessage()
	{
	}

	// set up the command
	void Init(int fromUID, unsigned int toUID, const char *messageType, const char *body, const char *fromName, int flags)
	{
		m_iFromUID = fromUID;
		m_iToUID = toUID;
		m_iFlags = flags;

		FixString(messageType, m_szMsgType, 32);
		FixString(fromName, m_szFromName, 32);
		FixString(body, m_szBody, 255);
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// make sure the sender isn't in the blocked list
		query << "select userID from UserBlocked where userID = " << m_iToUID << " and blockedID = " << m_iFromUID;
		Result res = query.store(RESET_QUERY);
		if (!res)
		{
			g_pConsole->Print(4, "** User_PostMessage Error: %s\n", query.error().c_str());
			return 0;
		}

		if (res.size() > 0)
		{
			// user is blocked from sending, ignore
			return 0;
		}

		// insert the message into the table
		query << "insert into Message ( fromUserID, toUserID, dateSent, fromUserName, body,	flags, deleteDate ) values ( "
				<< m_iFromUID << ", "
				<< m_iToUID << ", "
				<< "NULL ,'"		// this just writes int the current time
				<< m_szFromName << "', '"
				<< m_szBody << "', "
				<< m_iFlags << ", "
				<< "CURRENT_TIMESTAMP + 30000000 )";		// set the message to be discarded in 30 days

		query.execute(RESET_QUERY);

		if (!query.success())
		{
			g_pConsole->Print(4, "** User_PostMessage (3) Error: %s\n", query.error().c_str());
			return 0;
		}

		return 0;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserPostMessage);


//-----------------------------------------------------------------------------
// Purpose: Queries auth state
//-----------------------------------------------------------------------------
class CCmdUserGetMessage : public ISQLDBCommand
{
private:
	ITrackerUserDatabase::UserMessage_t data;
	unsigned int m_iUserID;

public:
	CCmdUserGetMessage()
	{
	}

	// set up the command
	void Init(unsigned int userID)
	{
		// fill in command
		m_iUserID = userID;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// get one message from the database
		query << "select messageID, fromUserId, fromUserName, body, flags, minimumBuild, maximumBuild, deleteDate, CURRENT_TIMESTAMP from Message where toUserID = " << m_iUserID << " LIMIT 1";
		Result res = query.store();
		if (!res)
		{
			g_pConsole->Print(4, "** User_GetMessage Error: %s\n", query.error().c_str());
			return -1;
		}

		if (res.size() < 1)
		{
			// no messages, return
			return 0;
		}

		Row row = res[0];

		// copy out the message
		data.msgID = row[0];
		data.fromUserID = row[1];
		v_strncpy(data.name, row[2], sizeof(data.name));
		v_strncpy(data.text, row[3], sizeof(data.text));
		data.flags = row[4];
		data.mininumBuild = row[5];
		data.maximumBuild = row[6];

		MysqlDateTime currentTime, deleteTime;
		deleteTime = row[7];
		currentTime = row[8];

		if (currentTime > deleteTime)
		{
			// message is passed it's prime - but still let it through unless it's a system message
			// system messages really do expire
			if (data.flags & MESSAGEFLAG_SYSTEM)
			{
				// delete the message right now
				query << "delete from Message where messageID = " << data.msgID;
				query.execute();

				if (!query.success())
				{
					g_pConsole->Print(4, "** User_GetMessage (2) Error: %s\n", query.error().c_str());
					return -1;
				}

				// now we need to re-Run this command
				// there should not be too many of these messages however
				return RunCommand(connection);
			}
		}

		return 1;
	}

	// return data
	virtual void *GetReturnData()
	{
		return &data;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_GETMESSAGE;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserGetMessage);


//-----------------------------------------------------------------------------
// Purpose: Deletes message from server, returns whether or not there are more msgs remaining
//-----------------------------------------------------------------------------
class CCmdUserDeleteMessage : public ISQLDBCommand
{
private:
	unsigned int m_iUserID;
	int m_iMsgID;

public:
	CCmdUserDeleteMessage()
	{
	}

	// set up the command
	void Init(unsigned int userID, int msgID)
	{
		// fill in command
		m_iUserID = userID;
		m_iMsgID = msgID;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// delete the message
		query << "delete from Message where messageID = " << m_iMsgID << " and toUserID = " << m_iUserID;
		query.execute();

		if (!query.success())
		{
			g_pConsole->Print(4, "** User_DeleteMessage Error: %s\n", query.error().c_str());
			return -1;
		}

		return 1;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_DELETEMESSAGE;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUserDeleteMessage);


//-----------------------------------------------------------------------------
// Purpose: returns the number of users in the range still online
//-----------------------------------------------------------------------------
class CCmdUsersAreOnline : public ISQLDBCommand
{
private:
	int m_iLowerUserIDRange;
	int m_iUpperUserIDRange;

public:
	void Init(int lowerUserIDRange, int upperUserIDRange)
	{
		m_iLowerUserIDRange = lowerUserIDRange - 1;
		m_iUpperUserIDRange = upperUserIDRange + 1;
	}

	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		query << "select count(*) from UserStatus where userID > " << m_iLowerUserIDRange <<
													" and userID < " << m_iUpperUserIDRange <<
													" and status > 0";

		Result res = query.store();
		int onlineUsers = res[0][0];
		return onlineUsers;
	}

	virtual int GetID()
	{
		return CMD_USERSAREONLINE;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUsersAreOnline);


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCmdGetCompleteUserData : public ISQLDBCommand
{
private:
	ITrackerUserDatabase::completedata_t *m_pData;
	int m_iLowerUserIDRange;
	int m_iUpperUserIDRange;

public:
	void Init(int lowerUserIDRange, int upperUserIDRange, ITrackerUserDatabase::completedata_t *data)
	{
		m_iLowerUserIDRange = lowerUserIDRange - 1;
		m_iUpperUserIDRange = upperUserIDRange + 1;
		m_pData = data;
	}

	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// get user table
		query << "select userID, nickName, firstName, lastName, emailAddress, password, firstLogon, lastLogon, logonCount"
				<< " from User"
				<< " where userID > " << m_iLowerUserIDRange
				<< " and userID < " << m_iUpperUserIDRange;

		Result user = query.store(RESET_QUERY);

		if (!user)
		{
			g_pConsole->Print(6, "** Users_GetCompleteUserData (user) Error: %s\n", query.error().c_str());
			return -1;
		}

		if (user.size() < 1)
		{
			g_pConsole->Print(6, "** Users_GetCompleteUserData (user) Error: No users in range (%d, %d)\n", m_iLowerUserIDRange, m_iUpperUserIDRange);
			return 0;
		}

		// copy user data
		m_pData->user.RemoveAll();
		m_pData->user.AddMultipleToTail(user.size());
		for (int i = 0; i < user.size(); i++)
		{
			Row row = user[i];
			m_pData->user[i].userID = row[0];
			v_strncpy(m_pData->user[i].userName, row[1], 32);
			v_strncpy(m_pData->user[i].firstName, row[2], 32);
			v_strncpy(m_pData->user[i].lastName, row[3], 32);
			v_strncpy(m_pData->user[i].emailAddress, row[4], 64);
			v_strncpy(m_pData->user[i].password, row[5], 32);
			v_strncpy(m_pData->user[i].firstLogin, row[6], 15);
			v_strncpy(m_pData->user[i].lastLogin, row[7], 15);
			m_pData->user[i].loginCount = row[8];
		}

		// get userauth table
		query << "select userID, targetID, authLevel from UserAuth"
				<< " where (userID > " << m_iLowerUserIDRange
				<< " and userID < " << m_iUpperUserIDRange << ")"
				<< " or (targetID > " << m_iLowerUserIDRange
				<< " and targetID < " << m_iUpperUserIDRange << ")";

		Result userauth = query.store(RESET_QUERY);
		if (!userauth)
		{
			g_pConsole->Print(6, "** Users_GetCompleteUserData (userauth) Error: %s\n", query.error().c_str());
			return -1;
		}

		// copy auth data
		m_pData->userauth.RemoveAll();
		m_pData->userauth.AddMultipleToTail(userauth.size());
		for (i = 0; i < userauth.size(); i++)
		{
			Row row = userauth[i];

			m_pData->userauth[i].userID = row[0];
			m_pData->userauth[i].targetID = row[1];
			m_pData->userauth[i].authLevel = row[2];
		}

		// get userblocked table
		query << "select userID, blockedID from UserBlocked "
				<< " where userID > " << m_iLowerUserIDRange
				<< " and userID < " << m_iUpperUserIDRange;

		Result userblocked = query.store(RESET_QUERY);
		if (!userblocked)
		{
			g_pConsole->Print(6, "** Users_GetCompleteUserData (userblocked) Error: %s\n", query.error().c_str());
			return -1;
		}

		// copy blocked data
		m_pData->userblocked.RemoveAll();
		m_pData->userblocked.AddMultipleToTail(userblocked.size());
		for (i = 0; i < userblocked.size(); i++)
		{
			Row row = userblocked[i];
			m_pData->userblocked[i].userID = row[0];
			m_pData->userblocked[i].blockedID = row[1];
		}

		// get message table
		query << "select fromUserID, toUserID, dateSent, body, flags, fromUserName, deleteDate, minimumBuild, maximumBuild from Message "
				<< " where (toUserID > " << m_iLowerUserIDRange
				<< " AND toUserID < " << m_iUpperUserIDRange
				<< " )";

		Result message = query.store(RESET_QUERY);
		if (!message)
		{
			g_pConsole->Print(6, "** Users_GetCompleteUserData (message) Error: %s\n", query.error().c_str());
			return -1;
		}

		// copy message data
		m_pData->message.RemoveAll();
		m_pData->message.AddMultipleToTail(message.size());
		for (i = 0; i < message.size(); i++)
		{
			Row row = message[i];

			m_pData->message[i].fromUserID = row[0];
			m_pData->message[i].toUserID = row[1];
			v_strncpy(m_pData->message[i].dateSent, row[2], 15);
			v_strncpy(m_pData->message[i].body, row[3], 255);
			m_pData->message[i].flags = row[4];
			v_strncpy(m_pData->message[i].fromUserName, row[5], 32);
			v_strncpy(m_pData->message[i].deleteDate, row[6], 15);
			m_pData->message[i].minimumBuild = row[7];
			m_pData->message[i].minimumBuild = row[8];
		}

		return user.size();
	}

	virtual int GetID()
	{
		return CMD_GETCOMPLETEUSERDATA;
	}

	virtual void *GetReturnData()
	{
		return m_pData;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdGetCompleteUserData);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCmdInsertCompleteUserData : public ISQLDBCommand
{
private:
	ITrackerUserDatabase::completedata_t const *m_pData;
	int m_iLowerUserIDRange;
	int m_iUpperUserIDRange;

public:
	void Init(int lowerUserIDRange, int upperUserIDRange, ITrackerUserDatabase::completedata_t const *data)
	{
		m_iLowerUserIDRange = lowerUserIDRange - 1;
		m_iUpperUserIDRange = upperUserIDRange + 1;
		m_pData = data;
	}

	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// insert user data
//		query << "insert ignore into User (userID, nickName, firstName, lastName, emailAddress, password, firstLogon, lastLogon, logonCount) values ";
		for (int i = 0; i < m_pData->user.Size(); i++)
		{
			char userName[64];
			char firstName[64];
			char lastName[64];
			char emailAddress[128];
			char password[64];

			FixString(m_pData->user[i].userName, userName, 64);
			FixString(m_pData->user[i].firstName, firstName, 64);
			FixString(m_pData->user[i].lastName, lastName, 64);
			FixString(m_pData->user[i].emailAddress, emailAddress, 128);
			FixString(m_pData->user[i].password, password, 64);

			char buffer[2048];
			sprintf(buffer, "insert ignore into User (userID, nickName, firstName, lastName, emailAddress, password, firstLogon, lastLogon, logonCount) values ( %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d )", m_pData->user[i].userID, userName, firstName, lastName, emailAddress, password, m_pData->user[i].firstLogin, m_pData->user[i].lastLogin, m_pData->user[i].loginCount);
			query << buffer;
			query.execute(RESET_QUERY);
			if (!query.success())
			{
				g_pConsole->Print(6, "** Users_InsertCompleteUserData (user) Error: %s\n", query.error().c_str());
				return 0;
			}
	/*				
			query << "( " << m_pData->user[i].userID
				<< ", '" << m_pData->user[i].userName
				<< "', '" << m_pData->user[i].firstName
				<< "', '" << m_pData->user[i].lastName
				<< "', '" << m_pData->user[i].emailAddress
				<< "', '" << m_pData->user[i].password
				<< "', '" << m_pData->user[i].firstLogin
				<< "', '" << m_pData->user[i].lastLogin
				<< "', " << m_pData->user[i].loginCount
				<< " )";
			
			if ((i + 1) < m_pData->user.Size())
			{
				query << ", ";
			}
*/
		}

/*
		query.execute(RESET_QUERY);
		if (!query.success())
		{
			g_pConsole->Print(6, "** Users_InsertCompleteUserData (user) Error: %s\n", query.error().c_str());
			return 0;
		}
*/

		// insert auth information
		if (m_pData->userauth.Size())
		{
			query << "insert ignore into UserAuth (userID, targetID, authLevel) values ";
			for (i = 0; i < m_pData->userauth.Size(); i++)
			{
				query << "( " << m_pData->userauth[i].userID
					<< ", " << m_pData->userauth[i].targetID
					<< ", " << m_pData->userauth[i].authLevel
					<< " )";

				if ((i + 1) < m_pData->userauth.Size())
				{
					query << ", ";
				}
			}
			query.execute(RESET_QUERY);
			if (!query.success())
			{
				g_pConsole->Print(6, "** Users_InsertCompleteUserData (userauth) Error: %s\n", query.error().c_str());
				return 0;
			}
		}

		// insert blocked info
		if (m_pData->userblocked.Size())
		{
			query << "insert ignore into UserBlocked (userID, blockedID) values ";
			for (i = 0; i < m_pData->userblocked.Size(); i++)
			{
				query << "( " << m_pData->userblocked[i].userID
					<< ", " << m_pData->userblocked[i].blockedID
					<< " )";

				if ((i + 1) < m_pData->userblocked.Size())
				{
					query << ", ";
				}
			}
			query.execute(RESET_QUERY);
			if (!query.success())
			{
				g_pConsole->Print(6, "** Users_InsertCompleteUserData (userblocked) Error: %s\n", query.error().c_str());
				return 0;
			}
		}

		// insert messages
		if (m_pData->message.Size())
		{
			query << "insert into Message (fromUserID, toUserID, dateSent, body, flags, fromUserName, deleteDate, minimumBuild, maximumBuild) values ";
			for (i = 0; i < m_pData->message.Size(); i++)
			{
				char body[512];
				char fromUserName[64];
				FixString(m_pData->message[i].body, body, 512);
				FixString(m_pData->message[i].fromUserName, fromUserName, 64);

				query << "( " << m_pData->message[i].fromUserID
					<< ", " << m_pData->message[i].toUserID
					<< ", '" << m_pData->message[i].dateSent
					<< "', '" << body
					<< "', " << m_pData->message[i].flags
					<< ", '" << fromUserName
					<< "', '" << m_pData->message[i].deleteDate
					<< "', " << m_pData->message[i].minimumBuild
					<< ", " << m_pData->message[i].maximumBuild
					<< " )";

				if ((i + 1) < m_pData->message.Size())
				{
					query << ", ";
				}
			}
			query.execute(RESET_QUERY);
			if (!query.success())
			{
				g_pConsole->Print(6, "** Users_InsertCompleteUserData (message) Error: %s\n", query.error().c_str());
				return 0;
			}
		}

		return 1;
	}

	virtual int GetID()
	{
		return CMD_INSERTCOMPLETEUSERDATA;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdInsertCompleteUserData);


//-----------------------------------------------------------------------------
// Purpose: Command to delete a range of users from the DB
//-----------------------------------------------------------------------------
class CCmdDeleteUserRange : public ISQLDBCommand
{
private:
	int m_iLowerUserIDRange;
	int m_iUpperUserIDRange;

public:
	void Init(int lowerUserIDRange, int upperUserIDRange)
	{
		m_iLowerUserIDRange = lowerUserIDRange - 1;
		m_iUpperUserIDRange = upperUserIDRange + 1;
	}

	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// delete from user status
		query << "delete from UserStatus where userID > " << m_iLowerUserIDRange
			<< " and userID < " << m_iUpperUserIDRange;
		query.execute(RESET_QUERY);
		if (!query.success())
		{
			g_pConsole->Print(6, "** Users_DeleteUserRange (UserStatus) Error: %s\n", query.error().c_str());
			return 0;
		}

		// delete from user table
		query << "delete from User where userID > " << m_iLowerUserIDRange
			<< " and userID < " << m_iUpperUserIDRange;
		query.execute(RESET_QUERY);
		if (!query.success())
		{
			g_pConsole->Print(6, "** Users_DeleteUserRange (User) Error: %s\n", query.error().c_str());
			return 0;
		}

		// delete from userauth
		query << "delete from UserAuth"
				<< " where (userID > " << m_iLowerUserIDRange
				<< " and userID < " << m_iUpperUserIDRange << ")";
		query.execute(RESET_QUERY);
		if (!query.success())
		{
			g_pConsole->Print(6, "** Users_DeleteUserRange (UserAuth) Error: %s\n", query.error().c_str());
			return 0;
		}

		// delete from message
		query << "delete from Message where toUserID > " << m_iLowerUserIDRange
			<< " and toUserID < " << m_iUpperUserIDRange;
		query.execute(RESET_QUERY);
		if (!query.success())
		{
			g_pConsole->Print(6, "** Users_DeleteUserRange (Message) Error: %s\n", query.error().c_str());
			return 0;
		}

		// delete from blocked
		query << "delete from UserBlocked where userID > " << m_iLowerUserIDRange
			<< " and userID < " << m_iUpperUserIDRange;
		query.execute(RESET_QUERY);
		if (!query.success())
		{
			g_pConsole->Print(6, "** Users_DeleteUserRange (UserBlocked) Error: %s\n", query.error().c_str());
			return 0;
		}

		return 1;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdDeleteUserRange);


//-----------------------------------------------------------------------------
// Purpose: Kills any users associated with this server
//-----------------------------------------------------------------------------
class CCmdKillAllUsersFromServer : public ISQLDBCommand
{
private:
	unsigned int m_iServerID;

public:
	CCmdKillAllUsersFromServer()
	{
	}

	// set up the command
	void Init(unsigned int serverID)
	{
		m_iServerID = serverID;
	}

	// executes the command
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// clear the userstatus table
		query << "delete from UserStatus where serverID = " << m_iServerID;
		query.execute();
		if (!query.success())
		{
			g_pConsole->Print(4, "** User_KillAllUsersFromServer Error: %s\n", query.error().c_str());
		}
		query.reset();

		// clear the watchmap table
		//!! disabled, for some reason the syntax is wrong
		/*
		query << "delete from WatchMap inner join Watcher on Watcher.watcherID = WatchMap.watcherID where Watcher.watcherServerID = " << m_iServerID;
		query.execute();
		if (!query.success())
		{
			g_pConsole->Print(4, "** User_KillAllUsersFromServer (2) Error: %s\n", query.error().c_str());
		}
		query.reset();
		*/

		// clear the watcher table
		query << "delete from Watcher where watcherServerID = " << m_iServerID;
		query.execute();
		if (!query.success())
		{
			g_pConsole->Print(4, "** User_KillAllUsersFromServer (3) Error: %s\n", query.error().c_str());
		}
		query.reset();

		return 1;
	}
	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdKillAllUsersFromServer);


//-----------------------------------------------------------------------------
// Purpose: Logs the user on to the server
//			Maps to Tracker/Stored Procedures/User_Login
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_Login(ISQLDBReplyTarget *replyTarget, unsigned int userID, const char *password, unsigned int userIP, unsigned short userPort, unsigned int sessionID, int status, int build)
{
	// create the command and add it to the execute queue
	CCmdUserLogin *cmd = new CCmdUserLogin();
	cmd->Init(m_iServerID, userID, password, userIP, userPort, sessionID, status, build);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: Validates the user name and password
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_Validate(ISQLDBReplyTarget *replyTarget, const char *email, const char *password, unsigned int IP, unsigned int port, unsigned int temporaryID)
{
	CCmdUserValidate *cmd = new CCmdUserValidate();
	cmd->Init(email, password, IP, port, temporaryID);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: Logs the user off the server
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_Logoff(ISQLDBReplyTarget *replyTarget, unsigned int userID, bool forceLogoff)
{
	// create the command and add it to the execute queue
	CCmdUserLogoff *cmd = new CCmdUserLogoff();
	cmd->Init(userID, m_iServerID, forceLogoff);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_GetSessionInfo(ISQLDBReplyTarget *replyTarget, int returnState, unsigned int userID)
{
	CCmdGetSessionInfo *cmd  = new CCmdGetSessionInfo();
	cmd->Init(userID);
	m_DB.AddCommandToQueue(cmd, replyTarget, returnState);
}


//-----------------------------------------------------------------------------
// Purpose: Tries to create a new user
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_Create(ISQLDBReplyTarget *replyTarget, unsigned int userID, const char *email, const char *password, const char *userName, const char *firstName, const char *lastName, unsigned int IP, unsigned int port )
{
	CCmdUserCreate *cmd = new CCmdUserCreate();
	cmd->Init(userID, email, password, userName, firstName, lastName, IP, port);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: Updates the status of a user in the database
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_Update( unsigned int userID, int newStatus, unsigned int gameIP, unsigned int gamePort, const char *gameType )
{
	// fill in command
	CCmdUserUpdate *cmd = new CCmdUserUpdate();
	cmd->Init(userID, newStatus, gameIP, gamePort, gameType);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Returns a list of users based on the search criteria
//			//!! should be moved to the TrackerFind database eventually
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::Find_Users(ISQLDBReplyTarget *replyTarget, unsigned int userID, const char *email, const char *userName, const char *firstName, const char *lastName)
{
	CCmdFindUsers *cmd = new CCmdFindUsers();
	cmd->Init(userID, email, userName, firstName, lastName);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: returns all our friends that we are authorized to see
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_GetFriendList(ISQLDBReplyTarget *replyTarget, unsigned int userID)
{
	CCmdGetFriendsList *cmd = new CCmdGetFriendsList();
	cmd->Init(userID);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}


//-----------------------------------------------------------------------------
// Purpose: Authorizes a user to see another (to be able to 'watch' them)
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_SetAuth(unsigned int userID, unsigned int targetID, int authLevel)
{
	CCmdUserSetAuth *cmd = new CCmdUserSetAuth();
	cmd->Init(userID, targetID, authLevel);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_RemoveAuth(unsigned int userID, unsigned int targetID)
{
	CCmdUserRemoveAuth *cmd = new CCmdUserRemoveAuth();
	cmd->Init(userID, targetID);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the blocked status of a user in the database
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_SetBlock(unsigned int userID, unsigned int blockedID, bool block)
{
	CCmdUserSetBlock *cmd = new CCmdUserSetBlock();
	cmd->Init(userID, blockedID, block);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Updates the users information in the database
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_SetInfo(unsigned int userID, const char *userName, const char *firstName, const char *lastName)
{
	CCmdUserSetInfo *cmd = new CCmdUserSetInfo();
	cmd->Init(userID, userName, firstName, lastName);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Posts a message to a user
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_PostMessage(unsigned int fromUID, unsigned int toUID, const char *messageType, const char *body, const char *fromName, int flags)
{
	CCmdUserPostMessage *cmd = new CCmdUserPostMessage();
	cmd->Init(fromUID, toUID, messageType, body, fromName, flags);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: checks to see if user 'userID' is authorized to see user 'targetID'
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_IsAuthed(ISQLDBReplyTarget *replyTarget, int returnState, unsigned int userID, int targetID, const char *extraData)
{
	CCmdUserIsAuthed *cmd = new CCmdUserIsAuthed();
	cmd->Init(userID, targetID, extraData);
	m_DB.AddCommandToQueue(cmd, replyTarget, returnState);
}

//-----------------------------------------------------------------------------
// Purpose: Gets information about a user
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_GetInfo(ISQLDBReplyTarget *replyTarget, int returnState, unsigned int userID)
{
	CCmdUserGetInfo *cmd = new CCmdUserGetInfo();
	cmd->Init(userID);
	m_DB.AddCommandToQueue(cmd, replyTarget, returnState);
}

//-----------------------------------------------------------------------------
// Purpose: Gets the list of users watching this user
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_GetWatchers(ISQLDBReplyTarget *replyTarget, unsigned int userID)
{
	CCmdUserGetWatchers *cmd = new CCmdUserGetWatchers();
	cmd->Init(userID);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: Retrieves a message from the server, but does not delete it
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_GetMessage(ISQLDBReplyTarget *replyTarget, unsigned int userID)
{
	CCmdUserGetMessage *cmd = new CCmdUserGetMessage();
	cmd->Init(userID);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: Deletes a message from the server
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_DeleteMessage(ISQLDBReplyTarget *replyTarget, unsigned int userID, int iMsgID)
{
	CCmdUserDeleteMessage *cmd = new CCmdUserDeleteMessage();
	cmd->Init(userID, iMsgID);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of users in the range still online
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::Users_AreOnline(ISQLDBReplyTarget *replyTarget, unsigned int lowerUserIDRange, unsigned int upperUserIDRange)
{
	CCmdUsersAreOnline *cmd = new CCmdUsersAreOnline();
	cmd->Init(lowerUserIDRange, upperUserIDRange);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: returns the complete set of data for a set of users
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::Users_GetCompleteUserData(ISQLDBReplyTarget *replyTarget, unsigned int lowerUserIDRange, unsigned int upperUserIDRange, completedata_t *data)
{
	CCmdGetCompleteUserData *cmd = new CCmdGetCompleteUserData();
	cmd->Init(lowerUserIDRange, upperUserIDRange, data);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: inserts a range of users into the database
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::Users_InsertCompleteUserData(ISQLDBReplyTarget *replyTarget, unsigned int lowerUserIDRange, unsigned int upperUserIDRange, const completedata_t *data)
{
	CCmdInsertCompleteUserData *cmd = new CCmdInsertCompleteUserData();
	cmd->Init(lowerUserIDRange, upperUserIDRange, data);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: deletes a range of users
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::Users_DeleteRange(unsigned int lowerUserIDRange, unsigned int upperUserIDRange)
{
	CCmdDeleteUserRange *cmd = new CCmdDeleteUserRange();
	cmd->Init(lowerUserIDRange, upperUserIDRange);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: adds watches to a set of users on this server
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_AddWatcher(unsigned int userID, unsigned int sessionID, unsigned int serverID, simpleuser_t *friends, int friendCount)
{
	CCmdUserAddWatcher *cmd = new CCmdUserAddWatcher();
	cmd->Init(userID, sessionID, serverID, friends, friendCount);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: adds a single watch on a user
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_AddSingleWatch(unsigned int userID, unsigned int sessionID, unsigned int serverID, unsigned int friendID)
{
	CCmdUserAddSingleWatch *cmd = new CCmdUserAddSingleWatch();
	cmd->Init(userID, sessionID, serverID, friendID);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: removes a watch from the server.  userID is probably a user on another server.
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_RemoveWatcher(unsigned int userID)
{
	CCmdUserRemoveWatcher *cmd = new CCmdUserRemoveWatcher();
	cmd->Init(userID);
	m_DB.AddCommandToQueue(cmd, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: returns the status of a set of friends
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_GetFriendsStatus(ISQLDBReplyTarget *replyTarget, unsigned int userID, simpleuser_t *friends, int friendCount)
{
	CCmdUserGetFriendsStatus *cmd = new CCmdUserGetFriendsStatus();
	cmd->Init(userID, friends, friendCount);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: Gets game information about friends
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::User_GetFriendsGameStatus(ISQLDBReplyTarget *replyTarget, unsigned int userID, simpleuser_t *friends, int friendCount)
{
	CCmdUserGetFriendsGameStatus *cmd = new CCmdUserGetFriendsGameStatus();
	cmd->Init(userID, friends, friendCount);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: delete any users belonging to us
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::Users_KillAllUsersFromServer()
{
	CCmdKillAllUsersFromServer *cmd = new CCmdKillAllUsersFromServer();
	cmd->Init(m_iServerID);
	m_DB.AddCommandToQueue(cmd, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// BLOCKING SQLDB FUNCTIONS 

// don't don't return until the function completes

//-----------------------------------------------------------------------------
// Purpose: clears invalid users out of the database
//-----------------------------------------------------------------------------
void CMySqlUserDatabase::Users_Flush()
{
	Query query = m_DB.DB().query();

	// get the list of users logged on from our server
	query << "select userID from UserStatus where serverID = " << m_iServerID << " and status > 0";
	Result res = query.store(RESET_QUERY);
	if (!res)
	{
		g_pConsole->Print(4, "** Users_Flush Error: %s\n", query.error().c_str());
		return;
	}

	// walk the list, logging the users out
	int count = res.size();
	for (int i = 0; i < count; i++)
	{
		Row row = res[i];
		unsigned int userID = row[0];

		User_Logoff(NULL, userID, true);
	}

	// delete any users belonging to us
	Users_KillAllUsersFromServer();
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
