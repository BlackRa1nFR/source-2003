//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// hl2_eventlog.cpp: implementation of the CHL1EventLog class.
//
//////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "../eventlog.h"
#include <keyvalues.h>

class CHL1EventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	virtual ~CHL1EventLog() {};

public:
	bool PrintEvent( KeyValues * event )	// overwrite virual function
	{
		if ( BaseClass::PrintEvent( event ) )
		{
			return true;
		}
	
		if ( Q_strcmp(event->GetName(), "hl1_") == 0 )
		{
			return PrintHL1Event( event );
		}

		return false;
	}

protected:

	bool PrintHL1Event( KeyValues * event )	// print Mod specific logs
	{
	//	const char * name = event->GetName() + Q_strlen("hl1_"); // remove prefix

		return false;
	}

};

CHL1EventLog g_HL1EventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
	return &g_HL1EventLog;
}

