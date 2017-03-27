//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Nasty headers!
#include "MySqlDatabase.h"
#include "vstdlib/strtools.h"
#include "vmpi.h"
#include "vmpi_dispatch.h"
#include "mpi_stats.h"
#include "cmdlib.h"
#include "mysql_wrapper.h"
#include "threadhelpers.h"
#include "vmpi_tools_shared.h"


/*

-- SQL code to (re)create the DB.

-- Master generates a unique job ID (in job_master_start) and sends it to workers.
-- Each worker (and the master) make a job_worker_start, link it to the primary job ID, 
--     get their own unique ID, which represents that process in that job.
-- All JobWorkerID fields link to the JobWorkerID field in job_worker_start. 

-- NOTE: do a "use vrad" or "use vvis" first, depending on the DB you want to create.


drop table job_master_start;
create table job_master_start ( 
	JobID					INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,	index id( JobID, MachineName(5) ),
	BSPFilename				TINYTEXT NOT NULL,
	StartTime				TIMESTAMP NOT NULL,
	MachineName				TEXT NOT NULL,
	RunningTimeMS			INTEGER UNSIGNED NOT NULL
	);

drop table job_master_end;
create table job_master_end (
	JobID					INTEGER UNSIGNED NOT NULL,					PRIMARY KEY ( JobID ),
	NumWorkersConnected		SMALLINT UNSIGNED NOT NULL,
	NumWorkersDisconnected	SMALLINT UNSIGNED NOT NULL,
	ErrorText				TEXT NOT NULL
	);

drop table job_worker_start;
create table job_worker_start (
	JobWorkerID				INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,	index id( JobWorkerID, JobID, IsMaster ),
	JobID					INTEGER UNSIGNED NOT NULL,				-- links to job_master_start::JobID
	IsMaster				BOOL NOT NULL,							-- Set to 1 if this "worker" is the master process.
	MachineName				TEXT NOT NULL
	);

drop table job_worker_end;
create table job_worker_end (
	JobWorkerID				INTEGER UNSIGNED NOT NULL,					PRIMARY KEY ( JobWorkerID ),
	ErrorText				TEXT NOT NULL,
	RunningTimeMS			INTEGER UNSIGNED NOT NULL
	);

drop table text_messages;
create table text_messages (
	JobWorkerID				INTEGER UNSIGNED NOT NULL,					index id( JobWorkerID, MessageIndex ),
	MessageIndex			INTEGER UNSIGNED NOT NULL,
	Text					TEXT NOT NULL
	);

drop table graph_entry;
create table graph_entry (
	JobWorkerID				INTEGER UNSIGNED NOT NULL,					index id( JobWorkerID ),
	MSSinceJobStart			INTEGER UNSIGNED NOT NULL,
	BytesSent				INTEGER UNSIGNED NOT NULL,
	BytesReceived			INTEGER UNSIGNED NOT NULL,
	WorkUnitsCompleted		INTEGER UNSIGNED NOT NULL
	);

drop table events;
create table events (
	JobWorkerID				INTEGER UNSIGNED NOT NULL,					index id( JobWorkerID ),
	Text					TEXT NOT NULL
	);
*/


/*

-- This code updates the access tables for VRAD.

use mysql;


-- This updates the "user" table, which is checked when someone tries to connect to the database.
delete from user where user="vrad_worker";
delete from user where user="vmpi_browser";

insert into user (host, user) values ("%", "vrad_worker");
insert into user (host, user) values ("%", "vmpi_browser");


-- This updates the "db" table, which controls who can access which tables in each database.
delete from db where Db="vrad";
insert into db (host, db, user, select_priv, insert_priv, update_priv) values ("%", "vrad", "vrad_worker", "y", "y", "y");
insert into db (host, db, user, select_priv)              values ("%", "vrad", "vmpi_browser", "y");


flush privileges;

*/

/*

-- This code updates the access tables for VVIS.

use mysql;


-- This updates the "user" table, which is checked when someone tries to connect to the database.
delete from user where user="vvis_worker";
delete from user where user="vmpi_browser";

insert into user (host, user) values ("%", "vvis_worker");
insert into user (host, user) values ("%", "vmpi_browser");


-- This updates the "db" table, which controls who can access which tables in each database.
delete from db where Db="vvis";
insert into db (host, db, user, select_priv, insert_priv, update_priv) values ("%", "vvis", "vvis_worker", "y", "y", "y");
insert into db (host, db, user, select_priv)              values ("%", "vvis", "vmpi_browser", "y");


flush privileges;

*/


// Stats set by the app.
int		g_nWorkersConnected = 0;
int		g_nWorkersDisconnected = 0;


DWORD	g_StatsStartTime;

CMySqlDatabase	*g_pDB = NULL;
IMySQL			*g_pSQL = NULL;

char	g_BSPFilename[256];

bool			g_bMaster = false;
unsigned long	g_JobPrimaryID = 0;	// This represents this job, but doesn't link to a particular machine.
unsigned long	g_JobWorkerID = 0;	// A unique key in the DB that represents this machine in this job.
char			g_MachineName[MAX_COMPUTERNAME_LENGTH+1] = {0};

unsigned long	g_CurrentMessageIndex = 0;


HANDLE	g_hPerfThread = NULL;
DWORD	g_PerfThreadID = 0xFEFEFEFE;
HANDLE	g_hPerfThreadExitEvent = NULL;

DWORD g_RunningTimeMS = 0;


// This inserts the necessary backslashes in front of backslashes or quote characters.
char* FormatStringForSQL( const char *pText )
{
	// First, count the quotes in the string. We need to put a backslash in front of each one.
	int nChars = 0;
	const char *pCur = pText;
	while ( *pCur != 0 )
	{
		if ( *pCur == '\"' || *pCur == '\\' )
			++nChars;
		
		++pCur;
		++nChars;
	}

	pCur = pText;
	char *pRetVal = new char[nChars+1];
	for ( int i=0; i < nChars; )
	{
		if ( *pCur == '\"' || *pCur == '\\' )
			pRetVal[i++] = '\\';
		
		pRetVal[i++] = *pCur;
		++pCur;
	}
	pRetVal[nChars] = 0;

	return pRetVal;
}



// -------------------------------------------------------------------------------- //
// Commands to add data to the database.
// -------------------------------------------------------------------------------- //
class CSQLDBCommandBase : public ISQLDBCommand
{
public:
	virtual ~CSQLDBCommandBase()
	{
	}

	virtual void deleteThis()
	{
		delete this;
	}
};

class CSQLDBCommand_JobMasterEnd : public CSQLDBCommandBase
{
public:
	
	virtual int RunCommand()
	{
		CMySQLQuery query;
		query.Format( "insert into job_master_end values ( %lu, %d, %d, \"no errors\" )", g_JobPrimaryID, g_nWorkersConnected, g_nWorkersDisconnected ); 
		g_pSQL->Execute( query );

		// Now set RunningTimeMS.
		query.Format( "update job_master_start set RunningTimeMS=%lu where JobID=%lu", g_RunningTimeMS, g_JobPrimaryID );
		g_pSQL->Execute( query );
		return 1;
	}
};


class CSQLDBCommand_JobWorkerEnd : public CSQLDBCommandBase
{
public:
	
	virtual int RunCommand()
	{
		CMySQLQuery query;
		query.Format( "insert into job_worker_end ( JobWorkerID, ErrorText, RunningTimeMS ) values ( %lu, \"%s\", %lu )", g_JobWorkerID, "No Errors", g_RunningTimeMS );
		g_pSQL->Execute( query );
		return 1;
	}
};


class CSQLDBCommand_GraphEntry : public CSQLDBCommandBase
{
public:
	
				CSQLDBCommand_GraphEntry( DWORD msTime, DWORD nBytesSent, DWORD nBytesReceived )
				{
					m_msTime = msTime;
					m_nBytesSent = nBytesSent;
					m_nBytesReceived = nBytesReceived;
				}

	virtual int RunCommand()
	{
		CMySQLQuery query;
		query.Format(	"insert into graph_entry (JobWorkerID, MSSinceJobStart, BytesSent, BytesReceived) "
						"values ( %lu, %lu, %lu, %lu )", 
			g_JobWorkerID, 
			m_msTime, 
			m_nBytesSent, 
			m_nBytesReceived );
		
		g_pSQL->Execute( query );

		++g_CurrentMessageIndex;
		return 1;
	}

	DWORD m_nBytesSent;
	DWORD m_nBytesReceived;
	DWORD m_msTime;
};



class CSQLDBCommand_TextMessage : public CSQLDBCommandBase
{
public:
	
				CSQLDBCommand_TextMessage( const char *pText )
				{
					m_pText = FormatStringForSQL( pText );
				}

	virtual		~CSQLDBCommand_TextMessage()
	{
		delete [] m_pText;
	}

	virtual int RunCommand()
	{
		CMySQLQuery query;
		query.Format( "insert into text_messages (JobWorkerID, MessageIndex, Text) values ( %lu, %lu, \"%s\" )", g_JobWorkerID, g_CurrentMessageIndex, m_pText );
		g_pSQL->Execute( query );

		++g_CurrentMessageIndex;
		return 1;
	}

	char		*m_pText;
};


// -------------------------------------------------------------------------------- //
// Internal helpers.
// -------------------------------------------------------------------------------- //

// This is the spew output before it has connected to the MySQL database.
CCriticalSection g_SpewTextCS;
CUtlVector<char> g_SpewText( 1024 );


void VMPI_Stats_SpewHook( const char *pMsg )
{
	CCriticalSectionLock csLock( &g_SpewTextCS );
	csLock.Lock();

		// Queue the text up so we can send it to the DB right away when we connect.
		g_SpewText.AddMultipleToTail( strlen( pMsg ), pMsg );
}


void PerfThread_SendSpewText()
{
	// Send the spew text to the database.
	CCriticalSectionLock csLock( &g_SpewTextCS );
	csLock.Lock();
		
		if ( g_SpewText.Count() > 0 )
		{
			g_SpewText.AddToTail( 0 );
			g_pDB->AddCommandToQueue( new CSQLDBCommand_TextMessage( g_SpewText.Base() ), NULL );
			g_SpewText.RemoveAll();
		}

	csLock.Unlock();
}


// This function adds a graph_entry into the database every few seconds.
DWORD WINAPI PerfThreadFn( LPVOID pParameter )
{
	DWORD lastSent = 0;
	DWORD lastReceived = 0;
	DWORD startTicks = GetTickCount();

	while ( WaitForSingleObject( g_hPerfThreadExitEvent, 1000 ) != WAIT_OBJECT_0 )
	{
		DWORD curSent = g_nBytesSent + g_nMulticastBytesSent;
		DWORD curReceived = g_nBytesReceived + g_nMulticastBytesReceived;

		g_pDB->AddCommandToQueue( 
			new CSQLDBCommand_GraphEntry( 
				GetTickCount() - startTicks,
				curSent - lastSent, 
				curReceived - lastReceived ), 
			NULL );

		PerfThread_SendSpewText();

		lastSent = curSent;
		lastReceived = curReceived;
	}

	PerfThread_SendSpewText();
	SetEvent( g_hPerfThreadExitEvent );
	return 0;
}


// -------------------------------------------------------------------------------- //
// VMPI_Stats interface.
// -------------------------------------------------------------------------------- //

void VMPI_Stats_InstallSpewHook()
{
	InstallExtraSpewHook( VMPI_Stats_SpewHook );
}


bool VMPI_Stats_Init_Master( 
	const char *pHostName, 
	const char *pDBName, 
	const char *pUserName,
	const char *pBSPFilename, 
	unsigned long *pDBJobID )
{
	Assert( !g_pDB );

	g_bMaster = true;
	
	// Connect the database.
	g_pDB = new CMySqlDatabase;
	if ( !g_pDB || !g_pDB->Initialize() || !( g_pSQL = InitMySQL( pDBName, pHostName, pUserName ) ) )
	{
		delete g_pDB;
		g_pDB = NULL;
		return false;
	}

	DWORD size = sizeof( g_MachineName );
	GetComputerName( g_MachineName, &size );

	// Create the job_master_start row.
	ExtractFileBase( pBSPFilename, g_BSPFilename, sizeof( g_BSPFilename ) );

	g_JobPrimaryID = 0;
	CMySQLQuery query;
	query.Format( "insert into job_master_start ( BSPFilename, StartTime, MachineName, RunningTimeMS ) values ( \"%s\", null, \"%s\", %lu )", g_BSPFilename, g_MachineName, RUNNINGTIME_MS_SENTINEL ); 
	g_pSQL->Execute( query );

	g_JobPrimaryID = g_pSQL->InsertID();
	if ( g_JobPrimaryID == 0 )
	{
		delete g_pDB;
		g_pDB = NULL;
		return false;
	}


	// Now init the worker portion.
	*pDBJobID = g_JobPrimaryID;
	return VMPI_Stats_Init_Worker( NULL, NULL, NULL, g_JobPrimaryID );
}



bool VMPI_Stats_Init_Worker( const char *pHostName, const char *pDBName, const char *pUserName, unsigned long DBJobID )
{
	g_StatsStartTime = GetTickCount();
	
	// If pDBServerName is null, then we're the master and we just want to make the job_worker_start entry.
	if ( pHostName )
	{
		Assert( !g_pDB );
		
		// Connect the database.
		g_pDB = new CMySqlDatabase;
		if ( !g_pDB || !g_pDB->Initialize() || !( g_pSQL = InitMySQL( pDBName, pHostName, pUserName ) ) )
		{
			delete g_pDB;
			g_pDB = NULL;
			return false;
		}
		
		// Get our machine name to store in the database.
		DWORD size = sizeof( g_MachineName );
		GetComputerName( g_MachineName, &size );
	}


	g_JobPrimaryID = DBJobID;
	g_JobWorkerID = 0;

	CMySQLQuery query;
	query.Format( "insert into job_worker_start ( JobID, IsMaster, MachineName ) values ( %lu, %d, \"%s\" )",
		g_JobPrimaryID, g_bMaster, g_MachineName );
	g_pSQL->Execute( query );
			
	g_JobWorkerID = g_pSQL->InsertID();
	if ( g_JobWorkerID == 0 )
	{
		delete g_pDB;
		g_pDB = NULL;
		return false;
	}

	// Now create a thread that samples perf data and stores it in the database.
	g_hPerfThreadExitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	g_hPerfThread = CreateThread(
		NULL,
		0,
		PerfThreadFn,
		NULL,
		0,
		&g_PerfThreadID );

	return true;	
}


void VMPI_Stats_Term()
{
	if ( !g_pDB )
		return;

	// Stop the thread.
	SetEvent( g_hPerfThreadExitEvent );
	WaitForSingleObject( g_hPerfThread, INFINITE );
	
	CloseHandle( g_hPerfThreadExitEvent );
	g_hPerfThreadExitEvent = NULL;

	CloseHandle( g_hPerfThread );
	g_hPerfThread = NULL;

	g_RunningTimeMS = GetTickCount() - g_StatsStartTime;

	// Write a job_worker_end.
	g_pDB->AddCommandToQueue( new CSQLDBCommand_JobWorkerEnd, NULL );
	
	if ( g_bMaster )
	{
		// (Write a job_master_end entry here).
		g_pDB->AddCommandToQueue( new CSQLDBCommand_JobMasterEnd, NULL );
	}

	// Wait for up to a second for the DB to finish writing its data.
	DWORD startTime = GetTickCount();
	while ( GetTickCount() - startTime < 1000 )
	{
		if ( g_pDB->QueriesInOutQueue() == 0 )
			break;
	}

	delete g_pDB;
	g_pDB = NULL;

	g_pSQL->Release();
	g_pSQL = NULL;
}


static bool ReadStringFromFile( FileHandle_t fp, char *pStr, int strSize )
{
	for ( int i=0; i < strSize-2; i++ )
	{
		if ( g_pFileSystem->Read( &pStr[i], 1, fp ) != 1 ||
			pStr[i] == '\n' )
		{
			break;
		}
	}

	pStr[i] = 0;
	return i != 0;
}


// This looks for pDBInfoFilename in the same path as pBaseExeFilename.
// The file has 3 lines: machine name (with database), database name, username
void GetDBInfo( const char *pDBInfoFilename, CDBInfo *pInfo )
{
	char baseExeFilename[512];
	if ( !GetModuleFileName( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
		Error( "GetModuleFileName failed." );
	
	// Look for the info file in the same directory as the exe.
	char dbInfoFilename[512];
	Q_strncpy( dbInfoFilename, baseExeFilename, sizeof( dbInfoFilename ) );
	StripFilename( dbInfoFilename );

	if ( dbInfoFilename[0] == 0 )
		Q_strncpy( dbInfoFilename, ".", sizeof( dbInfoFilename ) );

	Q_strncat( dbInfoFilename, "/", sizeof( dbInfoFilename ) );
	Q_strncat( dbInfoFilename, pDBInfoFilename, sizeof( dbInfoFilename ) );

	FileHandle_t fp = g_pFileSystem->Open( dbInfoFilename, "rt" );
	if ( !fp )
	{
		Error( "Can't open %s for database info.\n", dbInfoFilename );
	}

	if ( !ReadStringFromFile( fp, pInfo->m_HostName, sizeof( pInfo->m_HostName ) ) ||
		 !ReadStringFromFile( fp, pInfo->m_DBName, sizeof( pInfo->m_DBName ) ) || 
		 !ReadStringFromFile( fp, pInfo->m_UserName, sizeof( pInfo->m_UserName ) ) 
		 )
	{
		Error( "%s is not a valid database info file.\n", dbInfoFilename );
	}

	g_pFileSystem->Close( fp );
}


void StatsDB_InitStatsDatabase( 
	int argc, 
	char **argv, 
	const char *pDBInfoFilename )
{
	unsigned long jobPrimaryID;

	// Now open the DB.
	if ( g_bMPIMaster )
	{
		CDBInfo dbInfo;
		GetDBInfo( pDBInfoFilename, &dbInfo );

		if ( !VMPI_Stats_Init_Master( dbInfo.m_HostName, dbInfo.m_DBName, dbInfo.m_UserName, argv[argc-1], &jobPrimaryID ) )
		{
			Warning( "VMPI_Stats_Init_Master( %s, %s, %s ) failed.\n", dbInfo.m_HostName, dbInfo.m_DBName, dbInfo.m_UserName );

			// Tell the workers not to use stats.
			dbInfo.m_HostName[0] = 0; 
		}

		// Send the database info to all the workers.
		SendDBInfo( &dbInfo, jobPrimaryID );
	}
	else
	{
		// Wait to get DB info so we can connect to the MySQL database.
		CDBInfo dbInfo;
		unsigned long jobPrimaryID;
		RecvDBInfo( &dbInfo, &jobPrimaryID );
		
		if ( dbInfo.m_HostName[0] != 0 )
		{
			if ( !VMPI_Stats_Init_Worker( dbInfo.m_HostName, dbInfo.m_DBName, dbInfo.m_UserName, jobPrimaryID ) )
				Error( "VMPI_Stats_Init_Worker( %s, %s, %d ) failed.\n", dbInfo.m_HostName, dbInfo.m_DBName, dbInfo.m_UserName, jobPrimaryID );
		}
	}
}


unsigned long StatsDB_GetUniqueJobID()
{
	return g_JobPrimaryID;
}


