//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LOADTESTAPP_H
#define LOADTESTAPP_H
#ifdef _WIN32
#pragma once
#endif

#include "../common/DebugConsole_Interface.h"
#include "../TrackerNET/TrackerNET_Interface.h"
#include <stdio.h>
#include <string.h>

extern IDebugConsole *g_pConsole;
extern class IThreads *g_pThreads;

//-----------------------------------------------------------------------------
// Purpose: Holds main frame loop for app
//-----------------------------------------------------------------------------
class CLoadTestApp
{
public:
	CLoadTestApp();
	~CLoadTestApp();

	// initializes, runs the frame loop, and shuts down the app
	void Run();

	// returns the number of users being tested
	int GetNumUsers();

	// gets the base id for user names
	const char *GetUserIdBase();

private:
	void Initialize();
	void Shutdown();
	void FrameLoop();

	// frame loop functions
	void ProcessConsoleInput();

	// data
	bool m_bRunning;
	CSysModule *m_hTrackerDBG;
	CSysModule *m_hTrackerNET;

	// number of users to test with, change be changed dynamically to increase load
	int m_iMaxTest;

	// number of seconds to sleep between each frame;  lower to increase load
	float m_flSleepTime;

	// time at which to perform the next random act
	double m_flNextRandomAct;

	// identifier
	char m_szID[5];

	// name base
	int m_iNameBase;

};

extern CLoadTestApp gApp;
extern IDebugConsole *g_pConsole;
extern ITrackerNET *g_pNet;

#endif // LOADTESTAPP_H
