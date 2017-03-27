//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implements TrackerSRV interface
//=============================================================================

#include "PrecompHeader.h"

#include "TrackerSRV_Interface.h"
#include "SessionManager.h"
#include "TopologyManager.h"
#include "LocalDebugConsole.h"
#include "../TrackerNET/TrackerNET_Interface.h"
#include "../TrackerNET/Threads.h"
#include "../TrackerNET/NetAddress.h"

#include "interface.h"
#include "Random.h"

#include "DebugTimer.h"

#include <stdio.h>

// global debug console pointer
CLocalDebugConsole g_LocalConsole;
IDebugConsole *g_pConsole = &g_LocalConsole;
ITrackerNET *g_pTrackerSRVNET = NULL;

//-----------------------------------------------------------------------------
// Purpose: TrackerSRV implementation
//			Handles initialization and running of the server
//-----------------------------------------------------------------------------
class CTrackerSRV : public ITrackerSRV
{
public:
	virtual void RunTrackerServer(const char *lpCmdLine);

private:
	IThreads *m_pThreads;
	IDebugConsole *m_pConsole;

	CSessionManager m_SessionManager;
	CTopologyManager m_TopologyManager;

	bool Initialize(const char *serverName, const char *dbsName);
	bool RegisterServerInNetwork(const char *serverName);
	void HandleTextCommand(const char *command);
	void LoadSettings();

	int m_iServerID;
	CSysModule *m_hDebugModule;
	CSysModule *m_hNetModule;
};

EXPOSE_SINGLE_INTERFACE(CTrackerSRV, ITrackerSRV, TRACKERSERVER_INTERFACE_VERSION);

//-----------------------------------------------------------------------------
// Purpose: Initializes the tracker server
//-----------------------------------------------------------------------------
bool CTrackerSRV::Initialize(const char *serverName, const char *dbsName)
{
	m_hDebugModule = Sys_LoadModule("TrackerDBG.dll");
	m_hNetModule = Sys_LoadModule("TrackerNET.dll");

	CreateInterfaceFn debugFactory = Sys_GetFactory(m_hDebugModule);
	CreateInterfaceFn netFactory = Sys_GetFactory(m_hNetModule);

	// load debugging console
	g_pConsole = m_pConsole = (IDebugConsole *)debugFactory(DEBUGCONSOLE_INTERFACE_VERSION, NULL);
	m_pConsole->Initialize("Tracker Server Debug Console");

	// initialize the networking to listen on port 1200, and only on port 1200
	g_pTrackerSRVNET = (ITrackerNET *)netFactory(TRACKERNET_INTERFACE_VERSION, NULL);
	if ( !g_pTrackerSRVNET->Initialize(1200, 1200) )
	{
		m_pConsole->Print(3, "Couldn't get port 1200 for server.\n");
		return false;
	}

	m_pThreads = net()->GetThreadAPI();

	if (dbsName)
	{
		m_TopologyManager.SetSQLDB(dbsName);
	}

	// load our details
	LoadSettings();

	// initialize the server registration module
	if (!RegisterServerInNetwork(serverName))
	{
		return false;
	}

	// initialize the session manager
	if (!m_SessionManager.Initialize(m_iServerID, 10000, m_TopologyManager.GetSQLDB(), debugFactory))
	{
		return false;
	}

	m_pConsole->Print(3, "TrackerSRV initialized and running\n");
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Main execution loop for the server
//-----------------------------------------------------------------------------
void CTrackerSRV::RunTrackerServer(const char *lpCmdLine) 
{
	char szServerName[256];
	szServerName[0] = 0;
	char szDBSName[256];
	szDBSName[0] = 0;

	char seps[]   = " ,\t\n";
	char *token = strtok((char *)lpCmdLine, seps);
	while (token != NULL)
	{
		if (!stricmp(token, "-connect"))
		{
			// server to connect to has been specified
			token = strtok(NULL, seps);
			if (token)
			{
				strcpy(szServerName, token);
			}
		}
		else if (!stricmp(token, "-sqldb"))
		{
			token = strtok(NULL, seps);
			if (token)
			{
				strcpy(szDBSName, token);
			}
		}
		
		token = strtok(NULL, seps);
	}

	// setup the server
	if (!Initialize(szServerName, szDBSName))
	{
		m_pConsole->Print(4, "Initialization failed, shutting down\n");
		if (m_pThreads)
		{
			m_pThreads->Sleep(6000);
		}
		return;
	}

	// main loop, handling all incoming data
	bool running = true;
	bool goodClose = false;
	float closeTime = 0.0f;		// number of frames to Run before closing
	float currentTime = m_SessionManager.GetCurrentTime();
	while (running || currentTime < closeTime )
	{
		currentTime = m_SessionManager.GetCurrentTime();

		// decrement the close frames if we shouldn't be running
		if (!running)
		{
			if (m_SessionManager.ActiveUserCount() < 1)
			{
				goodClose = true;

				// expediate the Shutdown, since all the users are logged off
				if (closeTime > (currentTime + 0.1f))
				{
					closeTime = currentTime + 0.1f;
				}
			}
		}

		bool shouldSleep = true;

		// call into the networking and see if we have any data waiting
		// this calls into the session manager
		IReceiveMessage *recv;
		if ((recv = net()->GetIncomingData()) != NULL)
		{
			m_SessionManager.ReceivedData(recv);

			// free the message
			net()->ReleaseMessage(recv);
			shouldSleep = false;
		}

		// check for failed messages
		if ((recv = net()->GetFailedMessage()) != NULL)
		{
			// pass on any failed messages
			m_SessionManager.OnFailedMessage(recv);
			net()->ReleaseMessage(recv);
		}

		net()->RunFrame();

		// returns true if there was any activity
		if (m_TopologyManager.RunFrame(0))
		{
			shouldSleep = false;
		}

		// returns true if there was any activity
		if (m_SessionManager.RunFrame(shouldSleep))
		{
			shouldSleep = false;
		}

		// poll the console for input
		char buf[128];
		if (g_pConsole->GetInput(buf, 127))
		{
			if (!stricmp(buf, "Quit"))
			{
				g_pConsole->Print(3, "Initiating Shutdown...\n");

				// initiate Shutdown sequence
				running = false;
				m_TopologyManager.StartShutdown();

				// make sure we close within 5 seconds - should be plenty of time to log everyone off
				closeTime = currentTime + 5.0f;

				// don't accept any new connections
				m_SessionManager.LockConnections();

				// logoff all the users, safely
				m_SessionManager.FlushUsers(false);
			}
			else
			{
				HandleTextCommand(buf);
			}
			shouldSleep = false;
		}

		if (shouldSleep)
		{
			// no work has been done, so sleep until we're signalled
			// we have to wake up routinely simply to manage the networking thread
			sessionmanager()->WaitForEvent(20);
		}
	}

	if (!goodClose)
	{
		g_pConsole->Print(3, "Shutdown not successful - some users may still be active in SQLDB\n");

		// not all the users were logged off in the time; force the remaining users off
		m_SessionManager.FlushUsers(true);

		// make sure the db has shut down
		m_pThreads->Sleep(2000);
	}
	else
	{
		g_pConsole->Print(3, "Shutdown completed successfully.\n");
	}

	// networking Shutdown
	net()->Shutdown(true);
}

//-----------------------------------------------------------------------------
// Purpose: Registers the server on the local network of tracker servers
// Input  : *serverName - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTrackerSRV::RegisterServerInNetwork(const char *serverName)
{
	CNetAddress addr;
	if (serverName && serverName[0])
	{
		addr = net()->GetNetAddress(serverName);
		addr.SetPort(1201);
	}

	CreateInterfaceFn netFactory = Sys_GetFactory(m_hNetModule);
	return m_TopologyManager.Initialize(m_iServerID, netFactory, addr);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CTrackerSRV::HandleTextCommand(const char *command)
{
	if (!stricmp(command, "status"))
	{
		// print server details
		g_pConsole->Print(9, "Server status\n");
		g_pConsole->Print(9, "Local IP: %s\n", net()->GetLocalAddress().ToStaticString());

		if (m_TopologyManager.IsPrimary())
		{
			g_pConsole->Print(9, "Primary server: self\n");
		}
		else
		{
			CNetAddress srv = m_TopologyManager.PrimaryServerAddress();
			g_pConsole->Print(9, "Primary server: %s\n", srv.ToStaticString());
		}

		// user counts
		g_pConsole->Print(9, "Users: %d / %d\n", m_SessionManager.ActiveUserCount(), m_SessionManager.MaxUserCount());
		g_pConsole->Print(9, "Bandwidth usage: %d b/s in    %d b/s out\n", net()->BytesReceivedPerSecond(), net()->BytesSentPerSecond());
		g_pConsole->Print(9, "Total sent: %d bytes\n", net()->BytesSent());
		g_pConsole->Print(9, "Total received: %d bytes\n", net()->BytesReceived());
	}
	else if (!stricmp(command, "users"))
	{
		m_SessionManager.PrintUserList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Loads any settings from disk
//-----------------------------------------------------------------------------
void CTrackerSRV::LoadSettings()
{
	FILE *f = fopen("id.txt", "rt");

	// only read in serverID for now
	if (f)
	{
		// load ID from file
		fscanf(f, "%d", &m_iServerID);
	}
	else
	{
		// just generate serverID from current time
		m_iServerID = RandomLong(1, MAX_RANDOM_RANGE);

		// write out the ID
		f = fopen("id.txt", "wt");
		fprintf(f, "%d", m_iServerID);
	}

	fclose(f);
}
