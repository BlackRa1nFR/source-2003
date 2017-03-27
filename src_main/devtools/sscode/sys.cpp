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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "sys.h"

bool Sys_Exec( const char *pProgName, const char *pCmdLine )
{
#if 0
	int count = 0;
	char cmdLine[1024];
	STARTUPINFO si;

	memset( &si, 0, sizeof(si) );
	si.cb = sizeof(si);
	GetStartupInfo( &si );

	sprintf( cmdLine, "%s %s", pProgName, pCmdLine );

	PROCESS_INFORMATION pi;
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;

	if ( CreateProcess( pProgName, cmdLine, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi ) )
	{
		do 
		{
			WaitForInputIdle( pi.hProcess, 100 );
			count++;
		} while ( count < 100 );
		
		CloseHandle( pi.hProcess );
		return true;
	}

	return false;
#else
	char tmp[1024];
	sprintf( tmp, "%s %s\n", pProgName, pCmdLine );
	system( tmp );
	
	return true;
#endif
}

