//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// EventLog.h: interface for the CEventLog class.
//			
//			
//
//			
//
//=============================================================================

#if !defined EVENTLOG_H
#define EVENTLOG_H

#ifdef _WIN32
#pragma once
#endif

#include <igameevents.h>
#include <igamesystem.h>

class CEventLog : public CBaseGameSystem, public IGameEventListener
{
	
public:
	CEventLog();
	virtual ~CEventLog();

public: // IGameEventListener Interface
	
	virtual void FireGameEvent( KeyValues * event );
	virtual void GameEventsUpdated();

	bool IsLocalListener() { return true; };

public: // CBaseGameSystem overwrites

	virtual bool Init();
	virtual void Shutdown();

protected:

	virtual bool PrintEvent( KeyValues * event );
	virtual bool PrintGameEvent( KeyValues * event );
	virtual bool PrintPlayerEvent( KeyValues * event );
	virtual bool PrintTeamEvent( KeyValues * event );
};

IGameSystem* GameLogSystem();

#endif // EVENTLOG_H
