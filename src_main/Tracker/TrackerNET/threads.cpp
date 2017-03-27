//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
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

#include "threads.h"

#include "winlite.h"
#include "time.h"

#pragma warning(disable: 4127) // warning C4127: conditional expression is constant

// critical thread usage
class IThreads::CCriticalSection
{
public:
	CCriticalSection() : bValid(false), pNext(NULL) {}
	bool bValid;
	CRITICAL_SECTION criticalSection;
	CCriticalSection *pNext;
};

//-----------------------------------------------------------------------------
// Purpose: Threads implementation
//-----------------------------------------------------------------------------
class CThreads : public IThreads
{
	CCriticalSection *m_pCriticalSections;

public:
	CThreads()
	{
		m_pCriticalSections = new CCriticalSection;
	}

	~CThreads()
	{
		// delete all the critical sections
		CCriticalSection *pNext = m_pCriticalSections;
		while ( pNext )
		{
			CCriticalSection *die = pNext;
			pNext = die->pNext;
			delete die;
		}
	}

private:
	struct thread_details_t
	{
		unsigned long threadID;
		HANDLE threadHandle;
		IThreadRun *threadFunc;
	};

	// wraps the thread calling function
	static DWORD WINAPI CloseThreadFunc( void *threadInfo )
	{
		// call the thread interface
		thread_details_t *threadData = (thread_details_t *)threadInfo;

		// this function
		IThreadRun *threadRun = threadData->threadFunc;
		int ret = threadRun ->ThreadRun();

		// close the thread handle
		::CloseHandle( threadData->threadHandle );

		// free the associated data block
		delete threadData;

		threadRun->ThreadFinished();
		return ret;
	}

	// returns a handle to a newly created thread
	void CreateThread( IThreadRun *threadFunc )
	{
		thread_details_t *threadData = new thread_details_t;
		threadData->threadID = 0;
		threadData->threadHandle = NULL;
		threadData->threadFunc = threadFunc;
		threadData->threadHandle = ::CreateThread( NULL, 0, CloseThreadFunc, threadData, 0, &threadData->threadID );
	}

	// returns a pointer to a new critical section
	CCriticalSection *CreateCriticalSection( void )
	{
		// search for a free critical section
		CCriticalSection *section = m_pCriticalSections;
		while ( 1 )
		{
			if ( !section->bValid )
			{
				// found a free section
				section->bValid = true;
				::InitializeCriticalSection( &section->criticalSection );
				return section;
			}

			if ( section->pNext == NULL )
			{
				// allocate a new critical section
				section->pNext = new CCriticalSection;
			}

			section = section->pNext;
		}
	}

	// invalidates and deletes an existing critical section
	void DeleteCriticalSection( CCriticalSection *criticalSection )
	{
		criticalSection->bValid = false;
		::DeleteCriticalSection( &criticalSection->criticalSection );
	}

	// causes the thread to sleep
	void Sleep( int milliseconds )
	{
		::Sleep( milliseconds );
	}

	// returns the time, in seconds
	float GetTime()
	{
		static double baseTime = ::GetTickCount() * 0.001;		// this keeps a nice 0 base
		static double timeAdd = 0.0;
		static double prevTime = 0.0;

		// get the new time
		double newTime = ::GetTickCount() * 0.001;

		// GetTickCount() loops every 49.7 days, so detect resets and take them into account
		if (newTime < prevTime)
		{
			timeAdd += prevTime;
		}

		prevTime = newTime;
		return (float)(newTime + timeAdd - baseTime);
	}

	// enters a critical section;  thread sleeps until the critical section becomes available
	void EnterCriticalSection( CCriticalSection *criticalSection )
	{
		::EnterCriticalSection( &criticalSection->criticalSection );
	}

	// tries to enter a critical section;  returns true if suceeded, false otherwise
/*  can't use this, since it won't work in first release of win95
	bool TryEnterCriticalSection( CCriticalSection *criticalSection )
	{
		return ::TryEnterCriticalSection( &criticalSection->criticalSection );
	}
*/

	// leaves a critical section, freeing it up for other threads to enter
	void LeaveCriticalSection( CCriticalSection *criticalSection )
	{
		::LeaveCriticalSection( &criticalSection->criticalSection );
	}
};

EXPOSE_SINGLE_INTERFACE(CThreads, IThreads, THREADS_INTERFACE_VERSION);