// vmpi_service.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "vmpi.h"
#include "iphelpers.h"
#include "bitbuf.h"
#include "vstdlib/strtools.h"
#include "interface.h"
#include "ivraddll.h"
#include "ivvisdll.h"
#include <io.h>
#include "utllinkedlist.h"
#include "service_helpers.h"
#include "vmpi_filesystem.h"
#include "service_conn_mgr.h"


// If this is true, then we are running under the SCM. We can also run as a regular 
// Windows app for debugging.
bool g_bRunningAsService = false;

double g_flLastKillProcessTime = 0;

const char *g_pExeFilename = "vmpi_service.exe";

char *g_pPassword = NULL;	// Set if this service is using a pw.
ISocket *g_pSocket = NULL;
int g_SocketPort = -1;		// Which port we were able to bind the port on.

HANDLE g_hRunningProcess = NULL;
HANDLE g_hRunningThread = NULL;

// When this is true, it will launch new processes invisibly.
bool g_bHideNewProcessWindows = true;

HINSTANCE g_hInstance = NULL;
int g_iBoundPort = -1;

bool g_bSkipCSXJobs = false;


// GetTickCount() at the time the app was started.
DWORD g_AppStartTime = 0;


// Different modes this app can run in.
#define RUNMODE_INSTALL		0
#define RUNMODE_CONSOLE		1
#define RUNMODE_SERVICE		2

int g_RunMode = RUNMODE_CONSOLE;

bool g_bMinimized = false;				// true if they run with -minimized.
int g_iCurState = VMPI_SERVICE_STATE_IDLE;
char g_CurMasterName[512] = {0};


void KillRunningProcess( const char *pReason );
void LoadStateFromRegistry();
void SaveStateToRegistry();



// ------------------------------------------------------------------------------------------ //
// This handles connection to clients.
// ------------------------------------------------------------------------------------------ //

class CVMPIServiceConnMgr : public CServiceConnMgr
{
public:

	virtual void	OnNewConnection( int id );
	virtual void	HandlePacket( const char *pData, int len );
	void			SendCurStateTo( int id );


public:

	void	AddConsoleOutput( const char *pMsg );

	void	SetAppState( int iState );
};


void CVMPIServiceConnMgr::AddConsoleOutput( const char *pMsg )
{
	// Tell clients of the new text string.
	CUtlVector<char> data;
	data.AddToTail( VMPI_SERVICE_CONSOLE_TEXT );
	data.AddMultipleToTail( strlen( pMsg ) + 1, pMsg );
	SendPacket( -1, data.Base(), data.Count() );
}


void CVMPIServiceConnMgr::SetAppState( int iState )
{
	// Update our state and send it to the clients.
	g_iCurState = iState;
	SendCurStateTo( -1 );
}


void CVMPIServiceConnMgr::OnNewConnection( int id )
{
	// Send our current state to the new connection.
	Msg( "(debug) Made a new connection!\n" );
	SendCurStateTo( id );
}


void CVMPIServiceConnMgr::SendCurStateTo( int id )
{
	CUtlVector<char> data;
	data.AddToTail( VMPI_SERVICE_STATE );
	data.AddMultipleToTail( sizeof( g_iCurState ), (char*)&g_iCurState );

	data.AddToTail( (char)g_bSkipCSXJobs );
	
	if ( g_pPassword )
		data.AddMultipleToTail( strlen( g_pPassword ) + 1, g_pPassword );
	else
		data.AddToTail( 0 );

	SendPacket( -1, data.Base(), data.Count() );
}


void CVMPIServiceConnMgr::HandlePacket( const char *pData, int len )
{
	switch( pData[0] )
	{
		case VMPI_SERVICE_DISABLE:
		{
			KillRunningProcess( "Got a VMPI_SERVICE_DISABLE packet" );
			SetAppState( VMPI_SERVICE_STATE_DISABLED );
			SaveStateToRegistry();
		}
		break;

		case VMPI_SERVICE_ENABLE:
		{
			if ( g_iCurState == VMPI_SERVICE_STATE_DISABLED )
			{
				SetAppState( VMPI_SERVICE_STATE_IDLE );
			}
			SaveStateToRegistry();
		}
		break;

		case VMPI_SERVICE_UPDATE_PASSWORD:
		{
			const char *pStr = pData + 1;
//			if ( g_pPassword )
//				delete [] g_pPassword;

			g_pPassword = new char[ strlen( pStr ) + 1 ];
			strcpy( g_pPassword, pStr );
			
			// Send out the new state.
			SendCurStateTo( -1 );
		}
		break;

		case VMPI_SERVICE_SKIP_CSX_JOBS:
		{
			g_bSkipCSXJobs = (pData[1] != 0);
			SendCurStateTo( -1 );
			SaveStateToRegistry();
		}
		break;

		case VMPI_SERVICE_EXIT:
		{
			ServiceHelpers_ExitEarly();
		}
		break;
	}
}

CVMPIServiceConnMgr g_ConnMgr;


// ------------------------------------------------------------------------------------------ //
// Persistent state stuff.
// ------------------------------------------------------------------------------------------ //

void LoadStateFromRegistry()
{
	HKEY hKey = NULL;
	RegCreateKey( HKEY_LOCAL_MACHINE, VMPI_SERVICE_KEY, &hKey );
	if ( hKey )
	{
		DWORD val = 0;
		DWORD type = REG_DWORD;
		DWORD size = sizeof( val );
		if ( RegQueryValueEx( 
			hKey,
			"SkipCSXJobs",
			0,
			&type,
			(unsigned char*)&val,
			&size ) == ERROR_SUCCESS && 
			type == REG_DWORD && 
			size == sizeof( val ) )
		{
			g_bSkipCSXJobs = (val != 0);
		}

		if ( RegQueryValueEx( 
			hKey,
			"Disabled",
			0,
			&type,
			(unsigned char*)&val,
			&size ) == ERROR_SUCCESS && 
			type == REG_DWORD && 
			size == sizeof( val ) &&
			val != 0 )
		{
			g_ConnMgr.SetAppState( VMPI_SERVICE_STATE_DISABLED );
		}
	}
}

void SaveStateToRegistry()
{
	HKEY hKey = NULL;
	RegCreateKey( HKEY_LOCAL_MACHINE, VMPI_SERVICE_KEY, &hKey );
	if ( hKey )
	{
		DWORD val = g_bSkipCSXJobs;
		RegSetValueEx( 
			hKey,
			"SkipCSXJobs",
			0,
			REG_DWORD,
			(unsigned char*)&val,
			sizeof( val ) );

		val = (g_iCurState == VMPI_SERVICE_STATE_DISABLED);
		RegSetValueEx( 
			hKey,
			"Disabled",
			0,
			REG_DWORD,
			(unsigned char*)&val,
			sizeof( val ) );
	}
}
 

// ------------------------------------------------------------------------------------------ //
// Helper functions.
// ------------------------------------------------------------------------------------------ //

char* FindArg( int argc, char **argv, const char *pArgName, char *pDefaultValue="" )
{
	for ( int i=0; i < argc; i++ )
	{
		if ( stricmp( argv[i], pArgName ) == 0 )
		{
			if ( (i+1) >= argc )
				return pDefaultValue;
			else
				return argv[i+1];
		}
	}
	return NULL;
}


SpewRetval_t MySpewOutputFunc( SpewType_t spewType, const char *pMsg )
{
	// Put the message in status.txt.
	static bool bFirst = true;
	static FILE *fp = NULL;
	if ( bFirst )
	{
		fp = fopen( "c:\\vmpi\\status.txt", "wt" );
		bFirst = false;
	}

	if ( fp )
	{
		fprintf( fp, "%s", pMsg );
		fflush( fp );
	}


	// Print it to the console.
	g_ConnMgr.AddConsoleOutput( pMsg );
	
	// Output to the debug console.
	OutputDebugString( pMsg );

	if ( spewType == SPEW_ASSERT )
		return SPEW_DEBUGGER;
	else if( spewType == SPEW_ERROR )
		return SPEW_ABORT;
	else
		return SPEW_CONTINUE;
}


void AppendArg( CUtlVector<char*> &newArgv, const char *pIn )
{
	// Add it to newArgv.
	char *pNewArg = new char[ strlen( pIn ) + 1 ];
	strcpy( pNewArg, pIn );
	newArgv.AddToTail( pNewArg );
}


void KillRunningProcess( const char *pReason )
{
	if ( !g_hRunningProcess )
		return;

	Msg( pReason );

	TerminateProcess( g_hRunningProcess, 1 );

	// Yep. Now we can start a new one.
	CloseHandle( g_hRunningProcess );
	g_hRunningProcess = NULL;

	CloseHandle( g_hRunningThread );
	g_hRunningThread = NULL;

	g_ConnMgr.SetAppState( VMPI_SERVICE_STATE_IDLE );
}


// ------------------------------------------------------------------------------------------ //
// Job memory stuff.
// ------------------------------------------------------------------------------------------ //

// CJobMemory is used to track which jobs we ran (or tried to run).
// We remember which jobs we did because Winsock likes to queue up the job packets on
// our socket, so if we don't remember which jobs we ran, we'd run the job a bunch of times.
class CJobMemory
{
public:
	int		m_ID[4];		// Random ID that comes from the server.
};

CUtlLinkedList<CJobMemory, int> g_JobMemories;


bool FindJobMemory( int id[4] )
{
	int iNext;
	for ( int i=g_JobMemories.Head(); i != g_JobMemories.InvalidIndex(); i=iNext )
	{
		iNext = g_JobMemories.Next( i );
		
		CJobMemory *pJob = &g_JobMemories[i];
		if ( memcmp( pJob->m_ID, id, sizeof( pJob->m_ID ) ) == 0 )
			return true;
	}
	return false;
}


void AddJobMemory( int id[4] )
{
	CJobMemory job;
	memcpy( job.m_ID, id, sizeof( job.m_ID ) );
	g_JobMemories.AddToTail( job );
}


bool CheckJobID( bf_read &buf )
{
	int jobID[4];
	jobID[0] = buf.ReadLong();
	jobID[1] = buf.ReadLong();
	jobID[2] = buf.ReadLong();
	jobID[3] = buf.ReadLong();
	if ( FindJobMemory( jobID ) || buf.IsOverflowed() )
	{
		return false;
	}
	
	AddJobMemory( jobID );
	return true;
}




// ------------------------------------------------------------------------------------------ //
// The main VMPI code.
// ------------------------------------------------------------------------------------------ //

void VMPI_Waiter_Term()
{
	if ( g_pSocket )
	{
		g_pSocket->Release();
		g_pSocket = NULL;
	}

	if ( g_hRunningProcess )
	{
		// Force the app to exit.
		TerminateProcess( g_hRunningProcess, 1 );

		CloseHandle( g_hRunningProcess );
		g_hRunningProcess = NULL;

		CloseHandle( g_hRunningThread );
		g_hRunningThread = NULL;
	}

	g_ConnMgr.Term();
}


bool VMPI_Waiter_Init()
{
	// Run as idle priority.
	HKEY hKey = NULL;
	RegCreateKey( HKEY_LOCAL_MACHINE, VMPI_SERVICE_KEY, &hKey );
	DWORD dwVal = 0;
	DWORD dummyType = REG_DWORD;
	DWORD dwValLen = sizeof( dwVal );
	if ( RegQueryValueEx( hKey, "LowPriority", NULL, &dummyType, (LPBYTE)&dwVal, &dwValLen ) == ERROR_SUCCESS )
	{
		if ( dwVal )
		{
			SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS );
		}
	}
	else
	{
		RegSetValueEx( hKey, "LowPriority", 0, REG_DWORD, (unsigned char*)&dwVal, sizeof( dwVal ) );
	}

	if ( !g_ConnMgr.InitServer() )
		Msg( "ERROR INITIALIZING CONNMGR\n" );

	g_pSocket = CreateIPSocket();
	if ( !g_pSocket )
	{
		Msg( "Error creating a socket.\n" );
		return false;
	}
	
	// Bind to the first port we find in the range [VMPI_SERVICE_PORT, VMPI_LAST_SERVICE_PORT].
	int iTest;
	for ( iTest=VMPI_SERVICE_PORT; iTest <= VMPI_LAST_SERVICE_PORT; iTest++ )
	{
		g_SocketPort = iTest;
		if ( g_pSocket->BindToAny( iTest ) )
			break;
	}
	if ( iTest == VMPI_LAST_SERVICE_PORT )
	{	
		Msg( "Error binding a socket to port %d.\n", VMPI_SERVICE_PORT );
		VMPI_Waiter_Term();
		return false;
	}
	
	g_iBoundPort = iTest;
	return true;
}


void RunInDLL( const char *pFilename, CUtlVector<char*> &newArgv, bool bVRAD )
{
	g_ConnMgr.SetAppState( VMPI_SERVICE_STATE_BUSY );

	bool bSuccess = false;
	CSysModule *pModule = Sys_LoadModule( pFilename );
	if ( pModule )
	{
		CreateInterfaceFn fn = Sys_GetFactory( pModule );
		if ( fn )
		{
			if ( bVRAD )
			{
				IVRadDLL *pDLL = (IVRadDLL*)fn( VRAD_INTERFACE_VERSION, NULL );
				if( pDLL )
				{
					pDLL->main( newArgv.Count(), newArgv.Base() );
					bSuccess = true;
					SpewOutputFunc( MySpewOutputFunc );
				}
			}
			else
			{
				IVVisDLL *pDLL = (IVVisDLL*)fn( VVIS_INTERFACE_VERSION, NULL );
				if( pDLL )
				{
					pDLL->main( newArgv.Count(), newArgv.Base() );
					bSuccess = true;
					SpewOutputFunc( MySpewOutputFunc );
				}
			}
		}

		Sys_UnloadModule( pModule );
	}
	
	if ( !bSuccess )
	{
		Msg( "Error running VRAD (or VVIS) out of DLL '%s'\n", pFilename );
	}

	g_ConnMgr.SetAppState( VMPI_SERVICE_STATE_IDLE );
}


void GetArgsFromBuffer( 
	bf_read &buf, 
	CUtlVector<char*> &newArgv, 
	const char *pIPString,
	bool *bShowAppWindow )
{
	int nArgs = buf.ReadWord();

	bool bSpewArgs = false;
	
	for ( int iArg=0; iArg < nArgs; iArg++ )
	{
		char argStr[512];
		buf.ReadString( argStr, sizeof( argStr ) );

		AppendArg( newArgv, argStr );
		if ( stricmp( argStr, "-mpi_verbose" ) == 0 )
			bSpewArgs = true;

		if ( stricmp( argStr, "-mpi_ShowAppWindow" ) == 0 )
			*bShowAppWindow = true;

		// Add these arguments after the executable filename to tell the program
		// that it's an MPI worker and who to connect to. 
		if ( iArg == 0 )
		{
			// (-mpi is already on the command line of whoever ran the app).
			// AppendArg( commandLine, sizeof( commandLine ), "-mpi" );
			AppendArg( newArgv, "-mpi_worker" );
			AppendArg( newArgv, pIPString );
		}
	}

	if ( bSpewArgs )
	{
		Msg( "nArgs: %d\n", newArgv.Count() );
		for ( int i=0; i < newArgv.Count(); i++ )
			Msg( "Arg %d: %s\n", i, newArgv[i] );
	}
}


bool GetDLLFilename( CUtlVector<char*> &newArgv, char pDLLFilename[512], bool &bVRAD )
{
	char *argStr = newArgv[0];

	bVRAD = true;
	int lookLen = 4;

	// First, look for the exe name.
	char *pVRADPos = Q_stristr( argStr, "vrad.exe" );
	if ( !pVRADPos )
	{
		pVRADPos = Q_stristr( argStr, "vrad" );
		
		// Make sure vrad is at the end of the string.
		if ( (pVRADPos - argStr) != (int)(strlen( argStr ) - lookLen) )
			pVRADPos = NULL;
	}

	// Not vrad? Look for vvis.
	if ( !pVRADPos )
	{
		lookLen = 4;
		pVRADPos = Q_stristr( argStr, "vvis.exe" );
		if ( !pVRADPos )
		{
			pVRADPos = Q_stristr( argStr, "vvis" );
			
			// Make sure vrad is at the end of the string.
			if ( (pVRADPos - argStr) != (int)(strlen( argStr ) - lookLen) )
				pVRADPos = NULL;
		}

		if ( pVRADPos )
			bVRAD = false;
	}

	// HL1...
	if ( !pVRADPos )
	{
		lookLen = 5;
		pVRADPos = Q_stristr( argStr, "hlrad.exe" );
		if ( !pVRADPos )
		{
			pVRADPos = Q_stristr( argStr, "hlrad" );
			
			// Make sure vrad is at the end of the string.
			if ( (pVRADPos - argStr) != (int)(strlen( argStr ) - lookLen) )
				pVRADPos = NULL;
		}
	}

	if ( pVRADPos )
	{
		Q_strncpy( pDLLFilename, argStr, 512 );
		pDLLFilename[ pVRADPos - argStr + lookLen ] = 0;
		Q_strncat( pDLLFilename, ".dll", 512 );

		return true;
	}
	else
	{
		return false;
	}
}


void RunProcessAtCommandLine( CUtlVector<char*> &newArgv, const char *pIPString, bool bShowAppWindow )
{
	char commandLine[512];
	commandLine[0] = 0;

	for ( int i=0; i < newArgv.Count(); i++ )
	{
		char argStr[512];
		Q_snprintf( argStr, sizeof( argStr ), "\"%s\" ", newArgv[i] );
		Q_strncat( commandLine, argStr, sizeof( commandLine ) );
	}

	Msg( "Running '%s'\n", commandLine );

	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	DWORD dwFlags = 0;//IDLE_PRIORITY_CLASS;
	if ( bShowAppWindow )
		dwFlags |= CREATE_NEW_CONSOLE;
	else
		dwFlags |= CREATE_NO_WINDOW;

	UINT oldMode = SetErrorMode( SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS );

	if ( CreateProcess( 
		NULL, 
		commandLine, 
		NULL,							// security
		NULL,
		TRUE,
		dwFlags | IDLE_PRIORITY_CLASS,	// flags
		NULL,							// environment
		"C:\\",							// current directory (use c:\\ because we don't want it to accidentally share
										// DLLs like vstdlib with us).
		&si,
		&pi ) )
	{
		g_ConnMgr.SetAppState( VMPI_SERVICE_STATE_BUSY );
		g_hRunningProcess = pi.hProcess;
		g_hRunningThread = pi.hThread;
	}
	else
	{
		Msg( " - ERROR in CreateProcess (%s)!\n", GetLastErrorString() );
	}

	SetErrorMode( oldMode );
}


bool WaitForProcessToExit()
{
	if ( g_hRunningProcess )
	{
		// Did the process complete yet?
		if ( WaitForSingleObject( g_hRunningProcess, 0 ) == WAIT_TIMEOUT )
		{
			// Nope.. keep waiting.
			return true;
		}
		else
		{
			Msg( "Finished!\n ");

			// Change back to the 'waiting' icon.
			g_ConnMgr.SetAppState( VMPI_SERVICE_STATE_IDLE );

			// Yep. Now we can start a new one.
			CloseHandle( g_hRunningProcess );
			g_hRunningProcess = NULL;

			CloseHandle( g_hRunningThread );
			g_hRunningThread = NULL;
		}
	}

	return false;
}


void HandleWindowMessages()
{
	MSG msg;
	while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
}


int GetCurrentState()
{
	if ( g_iCurState == VMPI_SERVICE_STATE_DISABLED )
		return VMPI_STATE_DISABLED;
	else
		return g_hRunningProcess ? VMPI_STATE_BUSY : VMPI_STATE_IDLE;
}


void BuildPingHeader( CUtlVector<char> &data, char packetID, int iState )
{
	// Figure out the computer's name.
	char computerName[128];
	DWORD computerNameLen = sizeof( computerName );
	GetComputerName( computerName, &computerNameLen );	

	// Ping back at them.
	data.AddToTail( VMPI_PROTOCOL_VERSION );
	data.AddToTail( packetID );
	data.AddToTail( (char)iState );
	
	DWORD liveTime = GetTickCount() - g_AppStartTime;
	data.AddMultipleToTail( sizeof( liveTime ), (char*)&liveTime );

	data.AddMultipleToTail( sizeof( g_SocketPort ), (char*)&g_SocketPort );
	data.AddMultipleToTail( strlen( computerName ) + 1, computerName );

	if ( g_hRunningProcess )
		data.AddMultipleToTail( strlen( g_CurMasterName ) + 1, g_CurMasterName );
	else
		data.AddMultipleToTail( 1, "" );
}


void SendPingResponseTo( const CIPAddr &addr )
{
	CUtlVector<char> data;
	BuildPingHeader( data, VMPI_PING_RESPONSE, GetCurrentState() );
	g_pSocket->SendTo( &addr, data.Base(), data.Count() );
}


bool RegisterRestart( int timeout )
{
	// Run WaitAndRun to have it restart us in 60 seconds.
	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	// First add the exe to run.
	char commandLine[1024];
	Q_snprintf( commandLine, sizeof( commandLine ), "WaitAndRestart %d ", timeout );

	// Now add on our own command line.
	if ( g_RunMode == RUNMODE_SERVICE )
	{
		// If we were run as a service, then use vmpi_service_install to launch us again.
		Q_strncat( commandLine, "vmpi_service_install -start", sizeof( commandLine ) );
	}
	else
	{
		Q_strncat( commandLine, "vmpi_service_install -start -winapp", sizeof( commandLine ) );
	}

	if ( CreateProcess( 
		NULL, 
		commandLine, 
		NULL,						// security
		NULL,
		FALSE,
		DETACHED_PROCESS,			// flags
		NULL,						// environment
		NULL,						// current directory
		&si,
		&pi ) )
	{
		CloseHandle( pi.hThread );	// We don't care what the process does.
		CloseHandle( pi.hProcess );
		return true;
	}
	else
	{
		Msg( "RegisterRestart->CreateProcess failed (%s).\n", GetLastErrorString() );
		return false;
	}
}


void StopUI()
{
	char cPacket = VMPI_SERVICE_EXIT;
	g_ConnMgr.SendPacket( -1, &cPacket, 1 );

	// Wait for a bit for the connection to go away.
	DWORD startTime = GetTickCount();
	while ( GetTickCount()-startTime < 2000 )
	{
		g_ConnMgr.Update();
		if ( !g_ConnMgr.IsConnected() )
			break;
		else
			Sleep( 10 );
	}							
}


void VMPI_Waiter_Update()
{
	HandleWindowMessages();
	
	while ( 1 )
	{
		WaitForProcessToExit();

		// Recv off the socket first so it clears the queue while we're waiting for the process to exit.
		char data[4096];
		CIPAddr ipFrom;
		int len = g_pSocket->RecvFrom( data, sizeof( data ), &ipFrom );

		// Any incoming packets?
		if ( len <= 0 )
			break;

		bf_read buf( data, len );
		if ( buf.ReadByte() != VMPI_PROTOCOL_VERSION )
			continue;

		// Only handle packets with the right password.
		char pwString[256];
		buf.ReadString( pwString, sizeof( pwString ) );
		
		if ( pwString[0] == VMPI_PASSWORD_OVERRIDE )
		{
			// Always process these packets regardless of the password (these usually come from
			// the installer when it is trying to stop a previously-running instance).
		}
		else if ( pwString[0] == 0 )
		{
			if ( g_pPassword && g_pPassword[0] != 0 )
				continue;
		}
		else
		{
			if ( !g_pPassword || stricmp( g_pPassword, pwString ) != 0 )
				continue;
		}

		int packetID = buf.ReadByte();

		// VMPI_KILL_PROCESS is checked before everything.
		if ( packetID == VMPI_KILL_PROCESS )
		{
			if ( Plat_FloatTime() - g_flLastKillProcessTime > 5 )
			{
				KillRunningProcess( "Got a KILL_PROCESS packet. Stopping the worker executable.\n" );
				SendPingResponseTo( ipFrom );
				g_flLastKillProcessTime = Plat_FloatTime();
			}
		}
		else if ( packetID == VMPI_PING_REQUEST )
		{
			SendPingResponseTo( ipFrom );
		}
		else if ( packetID == VMPI_STOP_SERVICE )
		{
			Msg( "Got a STOP_SERVICE packet. Shutting down...\n" );
			
			CWaitTimer timer( 1 );
			while ( 1 )
			{
				SendPingResponseTo( ipFrom );

				if ( timer.ShouldKeepWaiting() )
					Sleep( 200 );
				else
					break;
			}

			StopUI();
			ServiceHelpers_ExitEarly();
			return;
		}
		else if ( packetID == VMPI_SERVICE_PATCH )
		{
			RegisterRestart( buf.ReadLong() );

			CUtlVector<char> data;
			BuildPingHeader( data, VMPI_PING_RESPONSE, VMPI_STATE_PATCHING );
			g_pSocket->SendTo( &ipFrom, data.Base(), data.Count() );

			StopUI();
			ServiceHelpers_ExitEarly();
		}

		// If they've told us not to wait for jobs, then ignore the packet.
		if ( g_iCurState == VMPI_SERVICE_STATE_DISABLED )
			continue;

		// If we're waiting for the process to exit, just ignore all packets.
		if ( WaitForProcessToExit() )
			continue;

		if ( packetID == VMPI_LOOKING_FOR_WORKERS )
		{
			int iPort = buf.ReadLong();

			// Make sure we don't run the same job more than once.
			if ( !CheckJobID( buf ) )
				continue;

			char ipString[128];
			_snprintf( ipString, sizeof( ipString ), "%d.%d.%d.%d:%d", ipFrom.ip[0], ipFrom.ip[1], ipFrom.ip[2], ipFrom.ip[3], iPort );
			
			CUtlVector<char*> newArgv;
			bool bShowAppWindow = false;
			GetArgsFromBuffer( buf, newArgv, ipString, &bShowAppWindow );

			// Figure out the name of the master machine.
			for ( int iArg=0; iArg < newArgv.Count()-1; iArg++ )
			{
				if ( stricmp( newArgv[iArg], "-mpi_MasterName" ) == 0 )
				{
					Q_strncpy( g_CurMasterName, newArgv[iArg+1], sizeof( g_CurMasterName ) );
				}
			}

			bool bVRAD;
			char DLLFilename[512];
			
			char *argStr = newArgv[0];
			if ( g_bSkipCSXJobs && (Q_stristr( argStr, "hlrad" ) || Q_stristr( argStr, "hlvis" )) )
			{
				Msg( "*** SKIPPING hlrad or hlvis ***\n" );
			}
			else
			{			
				if ( FindArg( __argc, __argv, "-TryDLLMode" ) && 
					!g_bRunningAsService && 
					GetDLLFilename( newArgv, DLLFilename, bVRAD ) )
				{
					// This is just a helper for debugging. If it's VRAD, we can run it
					// in-process as a DLL instead of running it as a separate EXE.
					RunInDLL( DLLFilename, newArgv, bVRAD );
				}
				else
				{
					// Run the (hopefully!) MPI app they specified.
					RunProcessAtCommandLine( newArgv, ipString, bShowAppWindow );
				}
			}

			newArgv.PurgeAndDeleteElements();
		}
	}
}


// ------------------------------------------------------------------------------------------------ //
// Startup and service code.
// ------------------------------------------------------------------------------------------------ //

void RunMainLoop()
{
	// This is the service's main loop.
	while ( 1 )
	{
		// If the service has been told to exit, then just exit.
		if ( ServiceHelpers_ShouldExit() )
			break;

		VMPI_Waiter_Update();
		g_ConnMgr.Update();
		
		Sleep( 50 );
	}
}


// This function runs us as a console app. Useful for debugging or if you want to run more
// than one instance of VRAD on the same machine.
void RunAsNonServiceApp()
{
	g_bRunningAsService = false;

	if ( !VMPI_Waiter_Init() )
		return;

	RunMainLoop();
	VMPI_Waiter_Term();
}


// This function runs inside the service thread.
void ServiceThreadFn( void *pParam )
{
	// Initialization code goes here. 
    if ( VMPI_Waiter_Init() )
	{
		RunMainLoop();	
		VMPI_Waiter_Term();
	}
}


// This function works with the service manager and runs as a system service.
void RunService()
{
	if( !ServiceHelpers_StartService( VMPI_SERVICE_NAME_INTERNAL, ServiceThreadFn, NULL ) )
	{
		Msg( "Service manager not started. Running as console app.\n" );
		g_RunMode = RUNMODE_CONSOLE;
		RunAsNonServiceApp();
	}
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	g_pPassword = FindArg( __argc, __argv, "-mpi_pw", NULL );
	
	if ( FindArg( __argc, __argv, "-console" ) )
	{					
		g_RunMode = RUNMODE_CONSOLE;
	}
	else
	{
		g_RunMode = RUNMODE_SERVICE;
	}

	g_AppStartTime = GetTickCount();
	g_bMinimized = FindArg( __argc, __argv, "-minimized" ) != NULL;

	ServiceHelpers_Init(); 	
	g_hInstance = hInstance;

	// Hook spew output.
	SpewOutputFunc( MySpewOutputFunc );

	LoadStateFromRegistry();

	// Install the service?
	if ( g_RunMode == RUNMODE_CONSOLE )
	{					
		RunAsNonServiceApp();
	}
	else
	{
		RunService();
	}

	return 0;
}


