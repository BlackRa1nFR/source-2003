//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef TRACKERDATABASEMANAGER_H
#define TRACKERDATABASEMANAGER_H
#pragma once

#include "interface.h"

class ITrackerUserDatabase;
class ITrackerFindDatabase;
class ITrackerDistroDatabase;

//-----------------------------------------------------------------------------
// Purpose: Handles access to the various tracker backend databases
//-----------------------------------------------------------------------------
class ITrackerDatabaseManager
{
public:
	// setup
	virtual bool Initialize(int serverID, const char *sqldbName, CreateInterfaceFn debugFactory) = 0;
	virtual bool ReloadUserDistribution() = 0;

	// database retrieval pointers
	virtual ITrackerUserDatabase *TrackerUser(int userID) = 0;
	virtual ITrackerDistroDatabase *TrackerDistro() = 0;

	// returns the database, ignoring db access threads, that the user is on
	virtual ITrackerUserDatabase *BaseTrackerUser(int userID) = 0;

	struct db_t
	{
		// user range
		int rangeMin;
		int rangeMax;
		int rangeMod;
		int desiredMod;

		// database pointer
		ITrackerUserDatabase *db;

		// name
		char serverName[32];
		char catalogName[32];

		int nextValidUserID;
	};

	// iteration functions for accessing all known tracker user databases
	// returns the number of tracker user servers
	virtual int TrackerUserCount() = 0;
	// returns the user db by index, range [0, TrackerUserCount)
	virtual struct db_t &TrackerUserByIndex(int index) = 0;

	// frame
	virtual bool RunFrame() = 0;

	virtual void deleteThis() = 0;
};

// exported virtual constructor
extern ITrackerDatabaseManager *Create_TrackerDatabaseManager( void );

extern ITrackerDatabaseManager *g_pDataManager;

#endif // TRACKERDATABASEMANAGER_H
