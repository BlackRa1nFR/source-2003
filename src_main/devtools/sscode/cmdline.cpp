//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "cmdline.h"

static int g_ArgCount;
static char **g_Args;

extern const char *GetUsageString( void );

//-----------------------------------------------------------------------------
// Purpose: simple args API
// Input  : argc - 
//			*argv[] - 
//-----------------------------------------------------------------------------
void ArgsInit( int argc, char *argv[] )
{
	g_ArgCount = argc;
	g_Args = argv;
}


//-----------------------------------------------------------------------------
// Purpose: tests for the presence of arg "pName"
// Input  : *pName - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ArgsExist( const char *pName )
{
	int i;

	if ( pName && pName[0] == '-' )
	{
		for ( i = 0; i < g_ArgCount; i++ )
		{
			// Is this a switch?
			if ( g_Args[i][0] != '-' )
				continue;

			if ( !stricmp( pName, g_Args[i] ) )
			{
				return true;
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: looks for the arg "pName" and returns it's parameter or pDefault otherwise
// Input  : *pName - 
//			*pDefault - 
// Output : const char *
//-----------------------------------------------------------------------------
const char *ArgsGet( const char *pName, const char *pDefault )
{
	int i;

	// don't bother with the last arg, it can't be a switch with a parameter
	for ( i = 0; i < g_ArgCount-1; i++ )
	{
		// Is this a switch?
		if ( g_Args[i][0] != '-' )
			continue;

		if ( !stricmp( pName, g_Args[i] ) )
		{
			// If the next arg is not a switch, return it
			if ( g_Args[i+1][0] != '-' )
				return g_Args[i+1];
		}
	}

	return pDefault;
}


void Error( const char *pString, ... )
{
	va_list list;
	va_start( list, pString );
	vprintf( pString, list );
	printf("Usage: %s\n", GetUsageString() );
	exit( 1 );
}

