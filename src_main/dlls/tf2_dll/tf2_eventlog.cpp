//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// hl2_eventlog.cpp: implementation of the CTF2EventLog class.
//
//////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "../eventlog.h"
#include <keyvalues.h>

class CTF2EventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	virtual ~CTF2EventLog() {};

public:
	bool PrintEvent( KeyValues * event )	// overwrite virual function
	{
		if ( BaseClass::PrintEvent( event ) )
		{
			return true;
		}
	
		if ( Q_strcmp(event->GetName(), "tf2_") == 0 )
		{
			return PrintTF2Event( event );
		}

		return false;
	}

protected:

	bool PrintTF2Event( KeyValues * event )	// print Mod specific logs
	{
	//	const char * name = event->GetName() + Q_strlen("tf2_"); // remove prefix

		return false;
	}

};

CTF2EventLog g_HL1EventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
	return &g_HL1EventLog;
}

