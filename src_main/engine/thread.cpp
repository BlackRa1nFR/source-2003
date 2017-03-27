#include <windows.h>
#include "tier0/dbg.h"
#include "cd.h"
#include "cd_internal.h"
#include "ithread.h"

//-----------------------------------------------------------------------------
// Purpose: Implements cd audio thread processing
//-----------------------------------------------------------------------------
class CThread : public IThread
{
public:
	// Construction
				CThread( void );
	virtual		~CThread( void );

	// Implement IThread
	bool		Init( void );
	bool		Shutdown( void );

	bool		AddThreadItem( vfunc pfn, int param1, int param2 );

public:
	enum
	{ 
		MAX_COMMANDS = 10
	};

	typedef struct 
	{
		vfunc	pfn;
		int		param1;
		int		param2;
	} threadcmds;

	HANDLE				m_hThreadEvent;
	CRITICAL_SECTION	m_cs;

	int					m_nCmds;
	int					_m_nCmds;
	threadcmds			m_rgCmds[MAX_COMMANDS + 1];
	threadcmds			_m_rgCmds[MAX_COMMANDS + 1];

private:
	HANDLE				m_hThread;
	DWORD				m_dwThreadID;
};

// Construct interface and singleton
static CThread g_Thread;
IThread *thread = ( IThread * )&g_Thread;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CThread::CThread( void )
{
	m_hThreadEvent		= NULL;
	m_hThread			= NULL;
	m_dwThreadID		= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CThread::~CThread( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Thread processing function
// Input  : pv - 
// Output : static DWORD WINAPI
//-----------------------------------------------------------------------------
static DWORD WINAPI ThreadFunc( LPVOID pv )
{
	//Plat_RegisterThread("ThreadFunc");

	CThread *thread = ( CThread * )pv;
	Assert( thread );

	cdaudio->Init();

	for (;;)
	{
		WaitForSingleObject( thread->m_hThreadEvent, INFINITE );
		
		EnterCriticalSection( &thread->m_cs );
		{
			memcpy(&thread->_m_rgCmds, &thread->m_rgCmds, thread->m_nCmds * sizeof(CThread::threadcmds));
			thread->_m_nCmds = thread->m_nCmds;

			thread->m_nCmds = 0;
			ResetEvent(thread->m_hThreadEvent);
		}
		LeaveCriticalSection( &thread->m_cs );

		for (int i = 0; i < thread->_m_nCmds; i++)
		{
			CThread::threadcmds *pt = &thread->_m_rgCmds[i];

			if ( pt->pfn )
			{
				( GetInteralCDAudio()->*pt->pfn)( pt->param1, pt->param2 );
			}
		}
	}

	cdaudio->Shutdown();
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CThread::Shutdown( void )
{
	bool bReturn;

	if (!m_hThreadEvent)
		return true;

	bReturn = TerminateThread( m_hThread, 1 ) ? true : false;
	CloseHandle(m_hThreadEvent);
	DeleteCriticalSection(&m_cs);
	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CThread::Init( void )
{
	InitializeCriticalSection(&m_cs);
	m_hThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = CreateThread(0, 0, ThreadFunc, this, 0, &m_dwThreadID);
	return m_hThread ? true : false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pfn - 
//			param1 - 
//			param2 - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CThread::AddThreadItem( vfunc pfn, int param1, int param2 )
{
	bool fret	= false;
	bool bFound = false;

	EnterCriticalSection( &m_cs );

	// Check for duplicates
	for ( int i = 0; i < m_nCmds; i++ )
	{
		if ( m_rgCmds[i].pfn != pfn )
			continue;

		bFound = true;
		break;
	}

	if ( !bFound )
	{
		if ( m_nCmds < MAX_COMMANDS )
		{
			m_rgCmds[m_nCmds].pfn = pfn;
			m_rgCmds[m_nCmds].param1 = param1;
			m_rgCmds[m_nCmds].param2 = param2;
			m_nCmds++;
			fret = true;
		}
	}

	LeaveCriticalSection( &m_cs );

	// Actually copied in a new command?
	if ( fret )
	{
		SetEvent(m_hThreadEvent);
	}
	return fret;
}