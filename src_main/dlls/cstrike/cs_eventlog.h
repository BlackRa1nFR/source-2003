//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// CSEventLog.h: interface for the CCSEventLog class.
//
//=============================================================================

#if !defined CSEVENTLOG_H
#define CSEVENTLOG_H

#ifdef _WIN32
#pragma once
#endif

#include "../eventlog.h"

class CCSEventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	CCSEventLog();
	virtual ~CCSEventLog();

public:
	bool PrintEvent( KeyValues * event );	// overwrite virual function

protected:

	bool PrintCStrikeEvent( KeyValues * event );

};

#endif 
