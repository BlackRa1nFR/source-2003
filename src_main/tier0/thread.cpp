//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Thread management routines
//
// $NoKeywords: $
//=============================================================================

#ifdef _WIN32
#define WIN_32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#endif

#include "tier0/platform.h"
#include "tier0/dbg.h"

// Private Thread local ID:
__declspec(thread) unsigned long Plat_CurrentThreadID = 0;

unsigned long Plat_PrimaryThreadID = 0;

// See the app note here, for a description of Plat_SetThreadName:
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vsdebug/html/vxtsksettingthreadname.asp

void Plat_SetThreadName( unsigned long dwThreadID, const char *pName )
{
	const unsigned long MS_VC_EXCEPTION = 0x406d1388;

	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType;        // must be 0x1000
		LPCSTR szName;       // pointer to name (in same addr space)
		DWORD dwThreadID;    // thread ID (-1 caller thread)
		DWORD dwFlags;       // reserved for future use, most be zero
	} THREADNAME_INFO;

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = pName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (DWORD *)&info);
    }
    __except( EXCEPTION_CONTINUE_EXECUTION )
    {
    }
}

unsigned long Plat_RegisterThread( const char *pName )
{
	// Initialize the current thread ID.
	Plat_CurrentThreadID = GetCurrentThreadId();

	//Assert( pName );
	//if( pName )
	//{
	//	Plat_SetThreadName( -1, pName );
	//}

	return Plat_CurrentThreadID;
}

// Registers the primary thread.
unsigned long Plat_RegisterPrimaryThread()
{
	Plat_PrimaryThreadID = Plat_RegisterThread( "Primary Thread" );
	return Plat_PrimaryThreadID;
}

unsigned long Plat_GetCurrentThreadID()
{
	return Plat_CurrentThreadID;
}
