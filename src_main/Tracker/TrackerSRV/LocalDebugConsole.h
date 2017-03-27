//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LOCALDEBUGCONSOLE_H
#define LOCALDEBUGCONSOLE_H
#ifdef _WIN32
#pragma once
#endif

#include "DebugConsole_Interface.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLocalDebugConsole : public IDebugConsole
{
	virtual void Initialize( const char *consoleName );
	virtual void Print( int severity, const char *msgDescriptor, ... );

	// gets a line of command input
	// returns true if anything returned, false otherwise
	virtual bool GetInput(char *buffer, int bufferSize);
};


#endif // LOCALDEBUGCONSOLE_H
