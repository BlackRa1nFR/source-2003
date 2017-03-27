//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "LocalDebugConsole.h"

#include <stdio.h>
#include <stdarg.h>

#pragma warning(disable: 4100) // warning C4100: unreferenced formal parameter

void CLocalDebugConsole::Initialize( const char *consoleName )
{
}

void CLocalDebugConsole::Print( int severity, const char *msgDescriptor, ... )
{
	static char szBuf[2048];
	va_list marker;
	va_start( marker, msgDescriptor );
	vsprintf( szBuf, msgDescriptor, marker );
	va_end( marker );

	printf("%s", szBuf);
}

// gets a line of command input
// returns true if anything returned, false otherwise
bool CLocalDebugConsole::GetInput(char *buffer, int bufferSize)
{
	return false;
}


