//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "MySqlDatabase.h"
#include "../public/ITrackerDistroDatabase.h"
#include "MemPool.h"

#include "DebugConsole_Interface.h"
extern IDebugConsole *g_pConsole;

extern void v_strncpy(char *dest, const char *src, int bufsize);

//-----------------------------------------------------------------------------
// Purpose: MySQL implementation of the tracker distro database
//-----------------------------------------------------------------------------
class CMySqlDistroDatabase : public ITrackerDistroDatabase
{
public:
	CMySqlDistroDatabase() {}

	virtual bool Initialize(int serverID, const char *serverName, const char *catalogName, CreateInterfaceFn debugFactory);
	virtual bool RunFrame();
	virtual bool GetUserDistribution(CUtlVector<trackerserver_t> &serverList);
	virtual void UpdateUserDistribution(ISQLDBReplyTarget *replyTarget, const char *srcServer, const char *srcCatalog, const char *destServer, const char *destCatalog, int lowerRange, int upperRange);
	virtual void ReserveUserID(ISQLDBReplyTarget *replyTarget, const char *email, const char *password, const char *userName, const char *firstName, const char *lastName, int IP, int port);
	virtual void deleteThis();

private:
	int m_iServerID;
	CMySqlDatabase m_DB;
};

EXPOSE_INTERFACE(CMySqlDistroDatabase, ITrackerDistroDatabase, TRACKERDISTRODATABASE_INTERFACE_VERSION);

//-----------------------------------------------------------------------------
// Purpose: sets up the database
//-----------------------------------------------------------------------------
bool CMySqlDistroDatabase::Initialize(int serverID, const char *serverName, const char *catalogName, CreateInterfaceFn debugFactory)
{
	g_pConsole = (IDebugConsole *)debugFactory(DEBUGCONSOLE_INTERFACE_VERSION, NULL);
	if (!g_pConsole)
		return false;

	m_iServerID = serverID;

	return m_DB.Initialize(serverName, catalogName);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMySqlDistroDatabase::RunFrame()
{
	return m_DB.RunFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Cleanup
//-----------------------------------------------------------------------------
void CMySqlDistroDatabase::deleteThis()
{
	delete this;
}

//-----------------------------------------------------------------------------
// Purpose: returns a list of user servers, filling in the serverList vector
// Input  : &serverList - in/out
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMySqlDistroDatabase::GetUserDistribution(CUtlVector<trackerserver_t> &serverList)
{
	Query query = m_DB.DB().query();

	// get the new userID
	query << "select serverName, catalogName, backupServerName, backupCatalogName, useridminimum, useridmaximum, nextValidUserId "
			<< "from UserDistribution where dbtype = 0";

	Result res = query.store();
	if (!query.success())
	{
		g_pConsole->Print(4, "** GetUserDistribution Error: %s", query.error().c_str());
		return false;
	}

	if (!res)
	{
		g_pConsole->Print(4, "** GetUserDistribution Error: %s", query.error().c_str());
		return false;
	}

	for (int i = 0; i < res.size(); i++)
	{
		Row row = res[i];

		// make the server, push it back
		trackerserver_t server;

		v_strncpy(server.serverName, row[0].c_str(), 64);
		v_strncpy(server.catalogName, row[1].c_str(), 64);
		v_strncpy(server.backupServerName, row[2].c_str(), 64);
		v_strncpy(server.backupCatalogName, row[3].c_str(), 64);

		server.userRangeMin = row[4];
		server.userRangeMax = row[5];
		server.nextValidUserID = row[6];

		serverList.AddToTail(server);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: DB command that reserves a userID number for use in a create user command
//-----------------------------------------------------------------------------
class CCmdUpdateUserDistribution : public ISQLDBCommand
{
private:
	char m_szSrcServer[32];
	char m_szSrcCatalog[32];
	char m_szDestServer[32];
	char m_szDestCatalog[32];
	int m_iLowerUserRange;
	int m_iUpperUserRange;

public:
	void Init(const char *srcServer, const char *srcCatalog, const char *destServer, const char *destCatalog, int lowerRange, int upperRange)
	{
		v_strncpy(m_szSrcServer, srcServer, sizeof(m_szSrcServer));
		v_strncpy(m_szSrcCatalog, srcCatalog, sizeof(m_szSrcCatalog));
		v_strncpy(m_szDestServer, destServer, sizeof(m_szDestServer));
		v_strncpy(m_szDestCatalog, destCatalog, sizeof(m_szDestCatalog));
		m_iLowerUserRange = lowerRange;
		m_iUpperUserRange = upperRange;
	}

	// makes the command Run (blocking), returning the success code
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// lock the database
		// no one else will be able to access the table until this is complete
		query << "lock tables UserDistribution WRITE";
		query.execute(RESET_QUERY);
		int result = 0;

		try
		{
			// get the new userID
			query << "update UserDistribution set userIDMaximum = "
				<< m_iUpperUserRange
				<< " where serverName = '" << m_szDestServer << "'"
				<< " and catalogName = '" << m_szDestCatalog << "' LIMIT 1";

			query.execute(RESET_QUERY);
			if (query.success())
			{
				query << "update UserDistribution set userIDMinimum = "
					<< (m_iUpperUserRange + 1)
					<< " where serverName = '" << m_szSrcServer << "'"
					<< " and catalogName = '" << m_szSrcCatalog << "' LIMIT 1";

				query.execute(RESET_QUERY);
				result = 1;

				if (!query.success())
				{
					g_pConsole->Print(4, "** UpdateUserDistribution Error: %s", query.error().c_str());
				}
			}
			else
			{
				g_pConsole->Print(4, "** UpdateUserDistribution Error: %s", query.error().c_str());
			}
		}
		catch (...)
		{
			// catch all exceptions, to make absolutely sure we don't leave the table locked
		}

		// unlock the table
		query << "unlock tables";
		query.execute();

		return result;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_UPDATEUSERDISTRIBUTION;
	}

	DECLARE_MEMPOOL();
};

IMPLEMENT_MEMPOOL(CCmdUpdateUserDistribution);

//-----------------------------------------------------------------------------
// Purpose: DB command that moves a set of users from one server to another
//			in the user distribution table
//-----------------------------------------------------------------------------
void CMySqlDistroDatabase::UpdateUserDistribution(ISQLDBReplyTarget *replyTarget, const char *srcServer, const char *srcCatalog, const char *destServer, const char *destCatalog, int lowerRange, int upperRange)
{
	CCmdUpdateUserDistribution *cmd = new CCmdUpdateUserDistribution();
	cmd->Init(srcServer, srcCatalog, destServer, destCatalog, lowerRange, upperRange);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}

//-----------------------------------------------------------------------------
// Purpose: DB command that reserves a userID number for use in a create user command
//-----------------------------------------------------------------------------
class CCmdDistroReserveUserID : public ISQLDBCommand
{
public:
	void Init(const char *email, const char *password, const char *userName, const char *firstName, const char *lastName, int IP, int port)
	{
		// store off the user details
		v_strncpy(data.email, email, sizeof(data.email));
		v_strncpy(data.password, password, sizeof(data.password));
		v_strncpy(data.username, userName, sizeof(data.username));
		v_strncpy(data.firstname, firstName, sizeof(data.firstname));
		v_strncpy(data.lastname, lastName, sizeof(data.lastname));
		data.ip = IP;
		data.port = port;
	}

	// makes the command Run (blocking), returning the success code
	virtual int RunCommand(Connection &connection)
	{
		Query query = connection.query();

		// lock the database
		// no one else will be able to access the table until this is complete
		query << "lock tables UserDistribution WRITE";
		query.execute(RESET_QUERY);

		int userID = -2;	// flag as error by default
		try
		{
			// get the new userID
			query << "select nextValidUserId from UserDistribution where nextValidUserID > 0 LIMIT 1";
			Result res = query.store(RESET_QUERY);
			if (res)
			{
				Row row = res[0];

				// parse out the new userID
				userID = row[0];

				// update database
				query << "update UserDistribution set nextValidUserId = nextValidUserId + 1, userIDMaximum = userIDMaximum + 1 where nextValidUserID > 0 and dbtype = 0 LIMIT 1";
				query.execute(RESET_QUERY);
				if (!query.success())
				{
					g_pConsole->Print(5, "** ReserveUserID Error: %s", query.error().c_str());
				}
			}
		}
		catch (...)
		{
			// catch all exceptions, to make absolutely sure we don't leave the table locked
		}

		// unlock the table
		query << "unlock tables";
		query.execute();

		return userID;
	}

	// return data
	virtual void *GetReturnData()
	{
		return &data;
	}

	// returns the command ID
	virtual int GetID()
	{
		return CMD_RESERVEUSERID;
	}

	DECLARE_MEMPOOL();

private:
	ITrackerDistroDatabase::ReserveUserID_t data;
};

IMPLEMENT_MEMPOOL(CCmdDistroReserveUserID);

//-----------------------------------------------------------------------------
// Purpose: Reserves a single userID
// Input  : &userID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CMySqlDistroDatabase::ReserveUserID(ISQLDBReplyTarget *replyTarget, const char *email, const char *password, const char *userName, const char *firstName, const char *lastName, int IP, int port)
{
	CCmdDistroReserveUserID *cmd = new CCmdDistroReserveUserID();
	cmd->Init(email, password, userName, firstName, lastName, IP, port);
	m_DB.AddCommandToQueue(cmd, replyTarget);
}



