//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// hl2_eventlog.cpp: implementation of the CHL2EventLog class.
//
//////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "../eventlog.h"
#include <keyvalues.h>

class CHL2EventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	virtual ~CHL2EventLog() {};

public:
	bool PrintEvent( KeyValues * event )	// overwrite virual function
	{
		if ( BaseClass::PrintEvent( event ) )
		{
			return true;
		}
	
		if ( Q_strcmp(event->GetName(), "hl2_") == 0 )
		{
			return PrintHL2Event( event );
		}

		return false;
	}

protected:

	bool PrintHL2Event( KeyValues * event )	// print Mod specific logs
	{
	//	const char * name = event->GetName() + Q_strlen("hl2_"); // remove prefix

		return false;
	}

};

CHL2EventLog g_HL2EventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
	return &g_HL2EventLog;
}

