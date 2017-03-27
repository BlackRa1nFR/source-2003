//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "PrecompHeader.h"

#include "TrackerDatabaseManager.h"

// specific databases
#include "../public/ITrackerUserDatabase.h"
#include "../public/ITrackerDistroDatabase.h"

#include "UtlVector.h"
#include "interface.h"

#include "DebugConsole_Interface.h"
#include <io.h>

extern IDebugConsole *g_pConsole;
ITrackerDatabaseManager *g_pDataManager = NULL;

//-----------------------------------------------------------------------------
// Purpose: Initializes, connects to, and stores all the databases used by tracker
//-----------------------------------------------------------------------------
class CTrackerDatabaseManager : public ITrackerDatabaseManager
{
public:
	CTrackerDatabaseManager();
	~CTrackerDatabaseManager();

	virtual bool Initialize(int serverID, const char *dbsName, CreateInterfaceFn debugFactory);

	// reloads the user distribution from the server - needs to be called after any load balancing work
	virtual bool ReloadUserDistribution();

	virtual ITrackerUserDatabase *TrackerUser(int userID);
	virtual ITrackerDistroDatabase *TrackerDistro() { return m_pDistroDatabase; }

	// returns the database, ignoring db access threads, that the user is on
	virtual ITrackerUserDatabase *BaseTrackerUser(int userID);

	// iteration functions for accessing all known tracker user databases
	// returns the number of tracker user servers
	virtual int TrackerUserCount();
	// returns the user db by index, range [0, TrackerUserCount)
	virtual struct db_t &TrackerUserByIndex(int index);

	virtual bool RunFrame();

	virtual void deleteThis()
	{
		delete this;
	}

private:
	bool LoadSQLDBDll();

	CUtlVector<db_t> m_UserDatabase;
	CUtlVector<int> m_UniqueUserDatabases;

	ITrackerDistroDatabase *m_pDistroDatabase;

	CSysModule *m_hSQLDBDLL;
	CreateInterfaceFn m_SQLDBFactory;

	int m_iServerID;
};


//-----------------------------------------------------------------------------
// Purpose: virtual constructor
// Output : ITrackerDatabaseManager
//-----------------------------------------------------------------------------
ITrackerDatabaseManager *Create_TrackerDatabaseManager( void )
{
	return new CTrackerDatabaseManager;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTrackerDatabaseManager::CTrackerDatabaseManager()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTrackerDatabaseManager::~CTrackerDatabaseManager()
{
	for (int i = 0; i < m_UserDatabase.Size(); i++)
	{
		m_UserDatabase[i].db->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Loads the databases
//-----------------------------------------------------------------------------
bool CTrackerDatabaseManager::Initialize(int serverID, const char *dbsName, CreateInterfaceFn debugFactory)
{
	m_iServerID = serverID;

	if (!LoadSQLDBDll())
		return false;

	// create the distro database
	m_pDistroDatabase = (ITrackerDistroDatabase *)m_SQLDBFactory(TRACKERDISTRODATABASE_INTERFACE_VERSION, NULL);
	if (!m_pDistroDatabase)
		return false;

	m_pDistroDatabase->Initialize(serverID, dbsName, "Tracker", debugFactory);

	// get the distribution map
	if (!ReloadUserDistribution())
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Reloads the user distribution mapping
//-----------------------------------------------------------------------------
bool CTrackerDatabaseManager::ReloadUserDistribution()
{
	// get the distribution map
	CUtlVector<ITrackerDistroDatabase::trackerserver_t> serverList(0, 64);
	if (!m_pDistroDatabase->GetUserDistribution(serverList))
		return false;

	// create the set of user databases
	const int threadsPerDB = 4;
	bool bSuccess = true;

	for (int serverIndex = 0; serverIndex < serverList.Size(); serverIndex++)
	{
		ITrackerDistroDatabase::trackerserver_t &server = serverList[serverIndex];

		// try and find the server in the existing list
		bool bFound = false;
		for (int i = 0; i < m_UserDatabase.Size(); i++)
		{
			db_t &database = m_UserDatabase[i];
			if (!stricmp(database.serverName, server.serverName) && !stricmp(database.catalogName, server.catalogName))
			{
				// the server and catalog names match, so instead just update this server
				database.rangeMin = server.userRangeMin;
				database.rangeMax = server.userRangeMax;
				database.nextValidUserID = server.nextValidUserID;

				bFound = true;
			}
		}

		// if we've already found the server, no need to create it
		if (bFound)
			continue;

		ITrackerUserDatabase *db = NULL;
		// initialize multiple threads per database
		for (i = 0; i < threadsPerDB; i++)
		{
			db_t database;

			db = (ITrackerUserDatabase *)m_SQLDBFactory(TRACKERUSERDATABASE_INTERFACE_VERSION, NULL);
			if (db->Initialize(m_iServerID, server.serverName, server.catalogName))
			{
				database.db = db;
				database.rangeMin = server.userRangeMin;
				database.rangeMax = server.userRangeMax;
				database.rangeMod = threadsPerDB;
				database.desiredMod = i;
				database.nextValidUserID = server.nextValidUserID;
				strncpy(database.serverName, server.serverName, sizeof(database.serverName)-1);
				database.serverName[sizeof(database.serverName)-1] = 0;
				strncpy(database.catalogName, server.catalogName, sizeof(database.catalogName)-1);
				database.catalogName[sizeof(database.catalogName)-1] = 0;
				
				m_UserDatabase.AddToTail(database);
			}
			else
			{
				bSuccess = false;
			}
		}

		// add a single db to the unique list
		m_UniqueUserDatabases.AddToTail(m_UserDatabase.Size() - 1);
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Runs a frame
//-----------------------------------------------------------------------------
bool CTrackerDatabaseManager::RunFrame()
{
	// keep track of any work done during the frame
	bool doneWork = false;

	// check the userdistribution queries
	if (m_pDistroDatabase->RunFrame())
	{
		doneWork = true;
	}

	// Run the user database threads
	for (int i = 0; i < m_UserDatabase.Size(); i++)
	{
		if (m_UserDatabase[i].db->RunFrame())
		{
			doneWork = true;
		}
	}

	return doneWork;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the correct database for the specified userID
//-----------------------------------------------------------------------------
ITrackerUserDatabase *CTrackerDatabaseManager::TrackerUser(int userID)
{
	if (userID == 0)
		return m_UserDatabase[0].db;

	// find the userID in the list
	for (int i = 0; i < m_UserDatabase.Size(); i++)
	{
		db_t &db = m_UserDatabase[i];

		if ((userID >= db.rangeMin) && (userID <= db.rangeMax) && (userID % db.rangeMod == db.desiredMod))
		{
			return db.db;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the base database for the specified userID, ignoring the thread dbs
//-----------------------------------------------------------------------------
ITrackerUserDatabase *CTrackerDatabaseManager::BaseTrackerUser(int userID)
{
	if (userID == 0)
		return m_UserDatabase[0].db;

	// find the userID in the list
	for (int i = 0; i < m_UserDatabase.Size(); i++)
	{
		db_t &db = m_UserDatabase[i];

		if ((userID >= db.rangeMin) && (userID <= db.rangeMax))
		{
			return db.db;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: returns the user db by index, range [0, TrackerUserCount)
//-----------------------------------------------------------------------------
int CTrackerDatabaseManager::TrackerUserCount()
{
	return m_UniqueUserDatabases.Size();
}

//-----------------------------------------------------------------------------
// Purpose: returns the user db by index, range [0, TrackerUserCount)
//-----------------------------------------------------------------------------
ITrackerDatabaseManager::db_t &CTrackerDatabaseManager::TrackerUserByIndex(int index)
{
	return m_UserDatabase[m_UniqueUserDatabases[index]];
}

//-----------------------------------------------------------------------------
// Purpose: loads the dll to interface to the sqldb database
//-----------------------------------------------------------------------------
bool CTrackerDatabaseManager::LoadSQLDBDll()
{
	_finddata_t findData;
	long findHandle = _findfirst("sqldb_*.dll", &findData);
	bool bSuccess = false;
	if (findHandle != -1)
	{
		m_hSQLDBDLL = Sys_LoadModule(findData.name);
		if (m_hSQLDBDLL)
		{
			m_SQLDBFactory = Sys_GetFactory(m_hSQLDBDLL);
			if (m_SQLDBFactory)
			{
				bSuccess = true;
			}
		}
		_findclose(findHandle);
	}

	if (!bSuccess)
	{
		// error
		g_pConsole->Print(9, "fatal error: could not load sqldb_*.dll\n");
	}

	return bSuccess;
}
