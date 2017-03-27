//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef THREADS_H
#define THREADS_H
#pragma once

class IThreadRun;

#include "interface.h"

//-----------------------------------------------------------------------------
// Purpose: Interface to the threading API
//-----------------------------------------------------------------------------
class IThreads : public IBaseInterface
{
public:
	// creation
	virtual void CreateThread( IThreadRun * ) = 0;

	// critical section usage
	class CCriticalSection;

	// returns a pointer to a new critical section
	virtual CCriticalSection *CreateCriticalSection( void ) = 0;

	// invalidates and deletes an existing critical section
	virtual void DeleteCriticalSection( CCriticalSection * ) = 0;

	// enters a critical section;  thread sleeps until the critical section becomes available
	virtual void EnterCriticalSection( CCriticalSection * ) = 0;

	// tries to enter a critical section;  returns true if suceeded, false otherwise
	// !! not implemented in first release of win95s
//	virtual bool TryEnterCriticalSection( CCriticalSection * ) = 0;

	// leaves a critical section, freeing it up for other threads to enter
	virtual void LeaveCriticalSection( CCriticalSection * ) = 0;

	////// sleeping and events //////

	// virtual CEvent *CreateEvent( void );
	// virtual void SignalEvent( CEvent * );

	// inactivity
	virtual void Sleep( int milliseconds ) = 0;

	// returns the time, in seconds, since IThreads was created
	virtual float GetTime() = 0;
};

#define THREADS_INTERFACE_VERSION "Threads001"

//-----------------------------------------------------------------------------
// Purpose: callback interface for the thread API
//			clients of the thread API implement this in objects they want threads created in
//-----------------------------------------------------------------------------
class IThreadRun
{
public:
	virtual int ThreadRun( void ) = 0;
	virtual void ThreadFinished( void ) = 0;
};




#endif // THREADS_H
