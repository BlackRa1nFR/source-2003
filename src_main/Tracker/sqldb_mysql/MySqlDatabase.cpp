//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "MySqlDatabase.h"
#include "DebugConsole_Interface.h"

IDebugConsole *g_pConsole = NULL;

extern IDebugConsole *g_pConsole;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMySqlDatabase::CMySqlDatabase() : m_Connection(false)
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//			blocks until db process thread has stopped
//-----------------------------------------------------------------------------
CMySqlDatabase::~CMySqlDatabase()
{
	// flag the thread to Stop
	m_bRunThread = false;

	// pulse the thread to make it Run
	Event_SignalEvent(m_hEvent);

	// make sure it's done
	::EnterCriticalSection(&m_csThread);
	::LeaveCriticalSection(&m_csThread);
}

//-----------------------------------------------------------------------------
// Purpose: Thread access function
//-----------------------------------------------------------------------------
static DWORD WINAPI staticThreadFunc(void *param)
{
	((CMySqlDatabase *)param)->RunThread();
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Establishes connection to the database and sets up this object to handle db command
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMySqlDatabase::Initialize(const char *serverName, const char *catalogName)
{
	// attempt to connect to the database via named pipes
	m_Connection.connect(catalogName, serverName, "", "", true);

	// check the connection
	if (!m_Connection.connected())
	{
		// try again, using tcp/ip to connect instead
		m_Connection.connect(catalogName, serverName, "", "", false);

		if (!m_Connection.connected())
		{
			g_pConsole->Print(5, "Failed to connect to MySqlDB %s : %s\n", serverName, m_Connection.error().c_str());
			return false;
		}
	}
	
	// print out database info
	g_pConsole->Print(5, "Server info: %s, %s\n", m_Connection.server_info().c_str(), m_Connection.host_info().c_str());

	// prepare critical sections
	//!! need to download SDK and replace these with InitializeCriticalSectionAndSpinCount() calls
	::InitializeCriticalSection(&m_csThread);
	::InitializeCriticalSection(&m_csInQueue);
	::InitializeCriticalSection(&m_csOutQueue);
	::InitializeCriticalSection(&m_csDBAccess);

	// initialize wait calls
	m_hEvent = Event_CreateEvent();

	// Start the DB-access thread
	m_bRunThread = true;

	unsigned long threadID;
	::CreateThread(NULL, 0, staticThreadFunc, this, 0, &threadID);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Main thread loop
//-----------------------------------------------------------------------------
void CMySqlDatabase::RunThread()
{
	::EnterCriticalSection(&m_csThread);
	while (m_bRunThread)
	{
		if (m_InQueue.Count() > 0)
		{
			// get a dispatched DB request
			::EnterCriticalSection(&m_csInQueue);

			// pop the front of the queue
			int headIndex = m_InQueue.Head();
			msg_t msg = m_InQueue[headIndex];
			m_InQueue.Remove(headIndex);

			::LeaveCriticalSection(&m_csInQueue);

			::EnterCriticalSection(&m_csDBAccess);
			
			// Run sqldb command
			try
			{
				msg.result = msg.cmd->RunCommand(m_Connection);
			}
			catch (...)
			{
				g_pConsole->Print(10, "**** RunCommand() failed, exception caught\n");
				msg.result = 0;
			}

			::LeaveCriticalSection(&m_csDBAccess);

			if (msg.replyTarget)
			{
				// put the results in the outgoing queue
				::EnterCriticalSection(&m_csOutQueue);
				m_OutQueue.AddToTail(msg);
				::LeaveCriticalSection(&m_csOutQueue);

				// wake up out queue
				msg.replyTarget->WakeUp();
			}
			else
			{
				// there is no return data from the call, so kill the object now
				msg.cmd->deleteThis();
			}
		}
		else
		{
			// nothing in incoming queue, so wait until we get the signal
			Event_WaitForEvent(m_hEvent, TIMEOUT_INFINITE);
		}

		// check the size of the outqueue; if it's getting too big, sleep to let the main thread catch up
		if (m_OutQueue.Count() > 50)
		{
			::Sleep(2);
		}
	}
	::LeaveCriticalSection(&m_csThread);
}

//-----------------------------------------------------------------------------
// Purpose: Adds a database command to the queue, and wakes the db thread
//-----------------------------------------------------------------------------
void CMySqlDatabase::AddCommandToQueue(ISQLDBCommand *cmd, ISQLDBReplyTarget *replyTarget, int returnState)
{
	::EnterCriticalSection(&m_csInQueue);

	// add to the queue
	msg_t msg = { cmd, replyTarget, 0, returnState };
	m_InQueue.AddToTail(msg);

	::LeaveCriticalSection(&m_csInQueue);

	// signal the thread to Start running
	Event_SignalEvent(m_hEvent);
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches responses to SQLDB queries
//-----------------------------------------------------------------------------
bool CMySqlDatabase::RunFrame()
{
	bool doneWork = false;

	if (m_OutQueue.Count() > 0)
	{
		::EnterCriticalSection(&m_csOutQueue);

		// pop the first item in the queue
		int headIndex = m_OutQueue.Head();
		msg_t msg = m_OutQueue[headIndex];
		m_OutQueue.Remove(headIndex);

		::LeaveCriticalSection(&m_csOutQueue);

		// Run result
		if (msg.replyTarget)
		{
			msg.replyTarget->SQLDBResponse(msg.cmd->GetID(), msg.returnState, msg.result, msg.cmd->GetReturnData());

			// kill command
			// it would be a good optimization to be able to reuse these
			msg.cmd->deleteThis();
		}

		doneWork = true;
	}

	return doneWork;
}

//-----------------------------------------------------------------------------
// Purpose: load info - returns the number of sql db queries waiting to be processed
//-----------------------------------------------------------------------------
int CMySqlDatabase::QueriesInOutQueue()
{
	// the queue names are from the DB point of view, not the server - thus the reversal
	return m_InQueue.Count();
}

//-----------------------------------------------------------------------------
// Purpose: number of queries finished processing, waiting to be responded to
//-----------------------------------------------------------------------------
int CMySqlDatabase::QueriesInFinishedQueue()
{
	return m_OutQueue.Count();
}

//-----------------------------------------------------------------------------
// Purpose: CCriticalSection object implementation
//-----------------------------------------------------------------------------
CCriticalSection::CCriticalSection()
{
	::InitializeCriticalSection(&m_CriticalSection);
}

//-----------------------------------------------------------------------------
// Purpose: destructor, cleans up
//-----------------------------------------------------------------------------
CCriticalSection::~CCriticalSection()
{
	::DeleteCriticalSection(&m_CriticalSection);
}

//-----------------------------------------------------------------------------
// Purpose: wrapper
//-----------------------------------------------------------------------------
CCriticalSection::EnterCriticalSection()
{
	::EnterCriticalSection(&m_CriticalSection);
}

//-----------------------------------------------------------------------------
// Purpose: wrapper
//-----------------------------------------------------------------------------
CCriticalSection::LeaveCriticalSection()
{
	::LeaveCriticalSection(&m_CriticalSection);
}
