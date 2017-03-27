//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// cs_eventlog.cpp: implementation of the CCSEventLog class.
//
//////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "../eventlog.h"
#include <keyvalues.h>

class CCSEventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	virtual ~CCSEventLog() {};

public:
	bool PrintEvent( KeyValues * event )	// overwrite virual function
	{
		if ( BaseClass::PrintEvent( event ) )
		{
			return true;
		}
	
		if ( Q_strcmp(event->GetName(), "cstrike_") == 0 )
		{
			return PrintCStrikeEvent( event );
		}

		return false;
	}

protected:

	bool PrintCStrikeEvent( KeyValues * event )	// print Mod specific logs
	{
	//	const char * name = event->GetName() + Q_strlen("cstrike_"); // remove prefix

		return false;
	}

};

CCSEventLog g_CSEventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
	return &g_CSEventLog;
}

