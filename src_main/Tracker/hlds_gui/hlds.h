//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef HLDS_H
#define HLDS_H
#ifdef _WIN32
#pragma once
#endif

#define snprintf _snprintf
#define vsnprintf _vsnprintf

//#define FRAMERATE // define me to have hlds print out what it thinks the framerate is

#include <stdio.h>
#include <stdlib.h>
#include <direct.h>

#include "dll_state.h" // for DLL_CLOSE, DLL_NORMAL defines

#include "engine_hlds_api.h"
#include "interface.h"
#include "vinternetdlg.h"



//-----------------------------------------------------------------------------
// Purpose: runs the actual game engine
//-----------------------------------------------------------------------------
class CHLDSServer
{
public:
	CHLDSServer() { window = NULL; engineAPI=NULL; memset(cvars,0x0,1024);}
	~CHLDSServer() {}
	
	void UpdateStatus(int force);
	IDedicatedServerAPI *GetEngineAPI() { return engineAPI; }
	void SetEngineAPI(IDedicatedServerAPI *API) { engineAPI=API; }
	void SetInstance(VInternetDlg *w) { window=w; }
	void *GetShutDownHandle() { return m_hShutdown; }
	char *GetCvars() { return cvars; }
	void ResetCvars() {memset(cvars,0x0,1024);}

	void Sys_Printf(char *fmt, ...);

	char *GetCmdline() { return cmdline; }


	void Start(const char *txt,const char *cvIn);
	void Stop();

private:	
    VInternetDlg *window; // the controlling window
	IDedicatedServerAPI *engineAPI;

	void *					m_hThread;
	// Thread id
	unsigned long					m_nThreadId;
	// Event that lets the thread tell the main window it shutdown
	void *					m_hShutdown;

	char cmdline[1024];
	char cvars[1024];

};


#endif // HLDS_H
