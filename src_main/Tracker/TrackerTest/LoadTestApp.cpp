//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "LoadTestApp.h"
#include "UserDatabase.h"
#include "../TrackerNET/Threads.h"
#include "../common/DebugTimer.h"

#include <stdlib.h>
#include <time.h>

// instances
CLoadTestApp gApp;
IDebugConsole *g_pConsole;
ITrackerNET *g_pNet;
IThreads *g_pThreads;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CLoadTestApp::CLoadTestApp()
{
	g_pConsole = NULL;
	g_pNet = NULL;
	g_pThreads = NULL;
	m_iMaxTest = 512;
	m_flSleepTime = 0.02f;
	m_flNextRandomAct = 0.0f;
	m_iNameBase = 0;

	// generate our (hopefully) unique ID
	srand(time(NULL));
	m_szID[0] = ((rand() * 26) / RAND_MAX) + 'a';
	m_szID[1] = ((rand() * 26) / RAND_MAX) + 'a';
	m_szID[2] = ((rand() * 26) / RAND_MAX) + 'a';
	m_szID[3] = ((rand() * 26) / RAND_MAX) + 'a';
	m_szID[4] = 0;

	// reseed use a constant seed to make things more deterministic
	srand(100);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CLoadTestApp::~CLoadTestApp()
{
}

//-----------------------------------------------------------------------------
// Purpose: Runs the app
//-----------------------------------------------------------------------------
void CLoadTestApp::Run()
{
	Initialize();

	g_pConsole->Print(5, "Tracker Load Test: simulating %d clients.\n", m_iMaxTest);

	m_bRunning = true;
	while (m_bRunning)
	{
		FrameLoop();
	}

	Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the initial state
//-----------------------------------------------------------------------------
void CLoadTestApp::Initialize()
{
	// load interfaces
	m_hTrackerDBG = Sys_LoadModule("TrackerDBG.dll");
	m_hTrackerNET = Sys_LoadModule("TrackerNET.dll");

	CreateInterfaceFn factory = Sys_GetFactory(m_hTrackerDBG);
	g_pConsole = (IDebugConsole *)factory(DEBUGCONSOLE_INTERFACE_VERSION, NULL);

	factory = Sys_GetFactory(m_hTrackerNET);
	g_pNet = (ITrackerNET *)factory(TRACKERNET_INTERFACE_VERSION, NULL);
	g_pThreads = (IThreads *)factory(THREADS_INTERFACE_VERSION, NULL);

	g_pConsole->Initialize("Tracker LoadTest");
	g_pConsole->Print(0, "Tracker LoadTest initialization successful\n");
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down anything relevant
//-----------------------------------------------------------------------------
void CLoadTestApp::Shutdown()
{
//	Sys_FreeModule(m_hTrackerNET);
	Sys_UnloadModule(m_hTrackerDBG);
}

//-----------------------------------------------------------------------------
// Purpose: Runs the main tracker loop
//-----------------------------------------------------------------------------
void CLoadTestApp::FrameLoop()
{
	// check console commands
	ProcessConsoleInput();
	
	// process any networking input for the clients
	int index = -1;
	CUser *user;
	while ((user = gUserDatabase.GetNextActiveUser(index)) != NULL)
	{
		user->ProcessNetworkInput();
	}

//	if (g_pThreads->GetTime() < m_flNextRandomAct)
//		return;

	// pick a random Client and perform an random act on them
	index = (rand() * m_iMaxTest) / RAND_MAX;
	user = gUserDatabase.GetUser(index);
	if (user)
	{
		if (!user->IsActive())
		{
			// create the user with a unique name
			char buf[64];
			sprintf(buf, "t_%s%d", m_szID, m_iNameBase++);
			user->CreateUser(buf, Sys_GetFactory(m_hTrackerNET));

			if ((m_iNameBase % 64) == 0)
			{
				g_pConsole->Print(2, "users active: %d\n", m_iNameBase);
			}
		}
		else
		{
			// perform a random act
			user->PerformRandomAct();
		}
	}

	m_flNextRandomAct = g_pThreads->GetTime() + m_flSleepTime;
}

//-----------------------------------------------------------------------------
// Purpose: Reads in and acts upon any console/UI input
//-----------------------------------------------------------------------------
void CLoadTestApp::ProcessConsoleInput()
{
	char cmd[256];
	g_pConsole->GetInput(cmd, sizeof(cmd));

	if (!stricmp(cmd, "quit"))
	{
		m_bRunning = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of users being tested
// Output : int
//-----------------------------------------------------------------------------
int CLoadTestApp::GetNumUsers()
{
	return m_iMaxTest;
}

//-----------------------------------------------------------------------------
// Purpose: gets the id
// Output : const char
//-----------------------------------------------------------------------------
const char *CLoadTestApp::GetUserIdBase()
{
	return m_szID;
}




