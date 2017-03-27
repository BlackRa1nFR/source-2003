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

#define	USED

#include <windows.h>
#include "cmdlib.h"
#define NO_THREAD_NAMES
#include "threads.h"
#include "pacifier.h"

#define	MAX_THREADS	16


class CRunThreadsData
{
public:
	int m_iThread;
	void *m_pUserData;
	RunThreadsFn m_Fn;
};

CRunThreadsData g_RunThreadsData[MAX_THREADS];


int		dispatch;
int		workcount;
qboolean		pacifier;

qboolean	threaded;
bool g_bLowPriorityThreads = false;

HANDLE g_ThreadHandles[MAX_THREADS];



/*
=============
GetThreadWork

=============
*/
int	GetThreadWork (void)
{
	int	r;

	ThreadLock ();

	if (dispatch == workcount)
	{
		ThreadUnlock ();
		return -1;
	}

	UpdatePacifier( (float)dispatch / workcount );

	r = dispatch;
	dispatch++;
	ThreadUnlock ();

	return r;
}


ThreadWorkerFn workfunction;

void ThreadWorkerFunction( int iThread, void *pUserData )
{
	int		work;

	while (1)
	{
		work = GetThreadWork ();
		if (work == -1)
			break;
		 
		workfunction( iThread, work );
	}
}

void RunThreadsOnIndividual (int workcnt, qboolean showpacifier, ThreadWorkerFn func)
{
	if (numthreads == -1)
		ThreadSetDefault ();
	
	workfunction = func;
	RunThreadsOn (workcnt, showpacifier, ThreadWorkerFunction);
}


/*
===================================================================

WIN32

===================================================================
*/

int		numthreads = -1;
CRITICAL_SECTION		crit;
static int enter;


class CCritInit
{
public:
	CCritInit()
	{
		InitializeCriticalSection (&crit);
	}
} g_CritInit;



void SetLowPriority()
{
	SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS );
}


void ThreadSetDefault (void)
{
	SYSTEM_INFO info;

	if (numthreads == -1)	// not set manually
	{
		GetSystemInfo (&info);
		numthreads = info.dwNumberOfProcessors;
		if (numthreads < 1 || numthreads > 32)
			numthreads = 1;
	}

	qprintf ("%i threads\n", numthreads);
}


void ThreadLock (void)
{
	if (!threaded)
		return;
	EnterCriticalSection (&crit);
	if (enter)
		Error ("Recursive ThreadLock\n");
	enter = 1;
}

void ThreadUnlock (void)
{
	if (!threaded)
		return;
	if (!enter)
		Error ("ThreadUnlock without lock\n");
	enter = 0;
	LeaveCriticalSection (&crit);
}


// This runs in the thread and dispatches a RunThreadsFn call.
DWORD WINAPI InternalRunThreadsFn( LPVOID pParameter )
{
	CRunThreadsData *pData = (CRunThreadsData*)pParameter;
	pData->m_Fn( pData->m_iThread, pData->m_pUserData );
	return 0;
}


void RunThreads_Start( RunThreadsFn fn, void *pUserData )
{
	threaded = true;

	if ( numthreads > MAX_TOOL_THREADS )
		numthreads = MAX_TOOL_THREADS;

	for ( int i=0; i < numthreads ;i++ )
	{
		g_RunThreadsData[i].m_iThread = i;
		g_RunThreadsData[i].m_pUserData = pUserData;
		g_RunThreadsData[i].m_Fn = fn;

		DWORD dwDummy;
		g_ThreadHandles[i] = CreateThread(
		   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
		   0,		// DWORD cbStack,
		   InternalRunThreadsFn,	// LPTHREAD_START_ROUTINE lpStartAddr,
		   &g_RunThreadsData[i],	// LPVOID lpvThreadParm,
		   0,			// DWORD fdwCreate,
		   &dwDummy );

		if( g_bLowPriorityThreads )
			SetThreadPriority( g_ThreadHandles[i], THREAD_PRIORITY_LOWEST );
	}
}


void RunThreads_End()
{
	WaitForMultipleObjects( numthreads, g_ThreadHandles, TRUE, INFINITE );
	for ( int i=0; i < numthreads; i++ )
		CloseHandle( g_ThreadHandles[i] );

	threaded = false;
}
	

/*
=============
RunThreadsOn
=============
*/
void RunThreadsOn( int workcnt, qboolean showpacifier, RunThreadsFn fn, void *pUserData )
{
	int		start, end;

	start = I_FloatTime ();
	dispatch = 0;
	workcount = workcnt;
	StartPacifier("");
	pacifier = showpacifier;

#ifdef _PROFILE
	threaded = false;
	(*func)( 0 );
	return;
#endif

	
	RunThreads_Start( fn, pUserData );
	RunThreads_End();


	end = I_FloatTime ();
	if (pacifier)
	{
		EndPacifier(false);
		printf (" (%i)\n", end-start);
	}
}


