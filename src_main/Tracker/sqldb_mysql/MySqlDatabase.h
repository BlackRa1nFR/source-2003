//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MYSQLDATABASE_H
#define MYSQLDATABASE_H
#ifdef _WIN32
#pragma once
#endif

#pragma warning(disable: 4800)	// forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4355)  // 'this' : used in base member initializer list
#pragma warning(disable: 4786)  // identifier was truncated to '255' characters in the debug information

#include <iostream> 
#include <iomanip> 
#include <sqlplus.hh> 

#include "CompletionEvent.h"
#include "ISQLDBReplyTarget.h"
#include "UtlVector.h"
#include "UtlLinkedList.h"

class ISQLDBCommand;

//-----------------------------------------------------------------------------
// Purpose: Generic MySQL accessing database
//			Provides threaded I/O queue functionality for accessing a mysql db
//-----------------------------------------------------------------------------
class CMySqlDatabase
{
public:
	// constructor
	CMySqlDatabase();
	~CMySqlDatabase();

	// initialization - must be called before this object can be used
	bool Initialize(const char *serverName, const char *catalogName);

	// Dispatches responses to SQLDB queries
	bool RunFrame();

	// load info - returns the number of sql db queries waiting to be processed
	virtual int QueriesInOutQueue();

	// number of queries finished processing, waiting to be responded to
	virtual int QueriesInFinishedQueue();

	// activates the thread
	void RunThread();

	// command queues
	void AddCommandToQueue(ISQLDBCommand *cmd, ISQLDBReplyTarget *replyTarget, int returnState = 0);

	// returns access to the db
	Connection &DB() { return m_Connection; }

private:
	// connect to SQL server
	Connection m_Connection;

	// threading data
	bool m_bRunThread;
	CRITICAL_SECTION m_csThread;
	CRITICAL_SECTION m_csInQueue;
	CRITICAL_SECTION m_csOutQueue;
	CRITICAL_SECTION m_csDBAccess;

	// wait event
	EventHandle_t m_hEvent;

	struct msg_t
	{
		ISQLDBCommand *cmd;
		ISQLDBReplyTarget *replyTarget;
		int result;
		int returnState;
	};

	// command queues
	CUtlLinkedList<msg_t, int> m_InQueue;
	CUtlLinkedList<msg_t, int> m_OutQueue;
};

class Connection;

//-----------------------------------------------------------------------------
// Purpose: Interface to a command
//-----------------------------------------------------------------------------
class ISQLDBCommand
{
public:
	// makes the command Run (blocking), returning the success code
	virtual int RunCommand(Connection &connection) = 0;

	// return data
	virtual void *GetReturnData() { return NULL; }

	// returns the command ID
	virtual int GetID() { return 0; }

	// gets information about the command for if it failed
	virtual void GetDebugInfo(char *buf, int bufSize) { buf[0] = 0; }

	// use to delete
	virtual void deleteThis() = 0;

protected:
	// protected destructor, so that it has to be deleted through deleteThis()
	virtual ~ISQLDBCommand() {}
};

//-----------------------------------------------------------------------------
// Purpose: critical section object that auto initializes and destructs
//-----------------------------------------------------------------------------
class CCriticalSection
{
public:
	CCriticalSection();
	~CCriticalSection();

	EnterCriticalSection();
	LeaveCriticalSection();

private:
	CRITICAL_SECTION m_CriticalSection;
};

// macro used to store commands in memory pools
#define DECLARE_MEMPOOL() \
	void *operator new(unsigned int size); \
	void operator delete(void *pMem); \
	static CMemoryPool s_MemPool;\
	static CCriticalSection s_MemPoolCriticalSection; \
	virtual void deleteThis() \
	{ delete this; }

#define IMPLEMENT_MEMPOOL(classname) \
	CCriticalSection classname::s_MemPoolCriticalSection; \
	CMemoryPool classname::s_MemPool(sizeof(classname), 32); \
	void *classname::operator new(unsigned int size)	\
	{ \
		s_MemPoolCriticalSection.EnterCriticalSection(); \
		void *pMem = s_MemPool.Alloc(size); \
		s_MemPoolCriticalSection.LeaveCriticalSection(); \
		return pMem; \
	} \
	void classname::operator delete(void *pMem) \
	{ \
		s_MemPoolCriticalSection.EnterCriticalSection(); \
		s_MemPool.Free(pMem); \
		s_MemPoolCriticalSection.LeaveCriticalSection(); \
	}

/*
	void *classname::operator new(unsigned int size)	{ s_MemPoolCriticalSection.EnterCriticalSection(); void *pMem = malloc(size); return pMem; s_MemPoolCriticalSection.LeaveCriticalSection(); } \
	void classname::operator delete(void *pMem) { s_MemPoolCriticalSection.EnterCriticalSection(); free(pMem); s_MemPoolCriticalSection.LeaveCriticalSection(); }

	*/


#endif // MYSQLDATABASE_H
