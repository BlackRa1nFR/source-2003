//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: This module implements the subset of MPI that VRAD and VVIS use.
//
// $NoKeywords: $
//=============================================================================

#include <windows.h>
#include <io.h>
#include <conio.h>
#include <stdio.h>
#include <direct.h>
#include "iphelpers.h"
#include "utlvector.h"
#include "utllinkedlist.h"
#include "vmpi.h"
#include "bitbuf.h"
#include "vstdlib/strtools.h"
#include "threadhelpers.h"
#include "IThreadedTCPSocket.h"
#include "vstdlib/random.h"
#include "vmpi_distribute_work.h"
#include "filesystem.h"
#include "checksum_md5.h"


#define DEFAULT_MAX_WORKERS	32	// Unless they specify -mpi_MaxWorkers, it will stop accepting workers after it gets this many.


typedef CUtlVector<char> PersistentPacket;

CCriticalSection g_PersistentPacketsCS;
CUtlLinkedList<PersistentPacket*> g_PersistentPackets;


// ---------------------------------------------------------------------------------------- //
// Globals.
// ---------------------------------------------------------------------------------------- //

class CVMPIConnection;

// This queues up all the incoming VMPI messages.
CCriticalSection g_VMPIMessagesCS;
CUtlLinkedList< CTCPPacket*, int > g_VMPIMessages;
CEvent g_VMPIMessagesEvent;	// This is set when there are messages in the queue.

// These are used to notify the main thread when some socket had OnError() called on it.
CUtlLinkedList<CVMPIConnection*,int> g_ErrorSockets;
CEvent g_ErrorSocketsEvent;
CCriticalSection g_ErrorSocketsCS;

#define MAX_VMPI_CONNECTIONS 256
CVMPIConnection *g_Connections[MAX_VMPI_CONNECTIONS];
int g_nConnections = 0;
CCriticalSection g_ConnectionsCS;


VMPIDispatchFn g_VMPIDispatch[MAX_VMPI_PACKET_IDS];
MessageBuffer g_DispatchBuffer;

int g_nBytesSent = 0;
int g_nMessagesSent = 0;
int g_nBytesReceived = 0;
int g_nMessagesReceived = 0;

int g_nMulticastBytesSent = 0;
int g_nMulticastBytesReceived = 0;


CUtlLinkedList<VMPI_Disconnect_Handler,int> g_DisconnectHandlers;

bool g_bUseMPI = false;
int g_iVMPIVerboseLevel = 0;
bool g_bMPIMaster = false;
bool g_bMPI_NoStats = false;

bool g_bMPIAllowDropIn = false;


// ---------------------------------------------------------------------------------------- //
// Classes.
// ---------------------------------------------------------------------------------------- //

// This class is used while discovering what files the workers need.
class CDependencyInfo
{
public:
	class CDependencyFile
	{
	public:
		char m_Name[256];
		long m_Time;
		unsigned long m_Size;
	};


	// This is the root directory on the network where we'll stick the dependency files.
	char m_BaseDirName[256];

	// This is m_BaseDirName + the hash code for the special directory for the clients to get the exes out of.
	char m_HashedDirName[512];

	// This is the directory where the version of VRAD that the MPI master is running uses for its exes.
	// This is where we look for the exes if 
	char m_OriginalExeDir[256];

	// "vrad.exe", "vvis.exe", etc.
	char m_OriginalExeFilename[256];

	// MD5 hash of the dependency filenames, their lengths, and their sizes.
	// This determines what directory we'll direct the workers to.
	unsigned char m_Digest[MD5_DIGEST_LENGTH];

	CUtlVector<CDependencyFile*> m_Files;


public:

	CDependencyFile* FindFile( const char *pFilename )
	{
		for ( int i=0; i < m_Files.Count(); i++ )
		{
			if ( stricmp( pFilename, m_Files[i]->m_Name ) == 0 )
				return m_Files[i];
		}
		return NULL;
	}
};


class CVMPIConnection : public ITCPSocketHandler
{
public:
	CVMPIConnection( int iConnection )
	{
		m_iConnection = iConnection;
		m_pSocket = NULL;

		char str[512];
		Q_snprintf( str, sizeof( str ), "%d", iConnection );
		SetMachineName( str );
	}
	
	~CVMPIConnection()
	{
		if ( m_pSocket )
			m_pSocket->Release();
	}


public:
	
	void HandleDisconnect()
	{
		if ( m_pSocket )
		{
			// Copy out the error string.
			CCriticalSectionLock csLock( &g_ErrorSocketsCS );
			csLock.Lock();
				char str[512];
				Q_strncpy( str, m_ErrorString.Base(), sizeof( str ) );
			csLock.Unlock();

			// Tell the app.
			FOR_EACH_LL( g_DisconnectHandlers, i )
				g_DisconnectHandlers[i]( m_iConnection, str );
			
			// Free our socket.
			m_pSocket->Release();
			m_pSocket = NULL;
		}
	}

	
	IThreadedTCPSocket* GetSocket()
	{
		return m_pSocket;
	}

	
	void SetMachineName( const char *pName )
	{
		m_MachineName.CopyArray( pName, strlen( pName ) + 1 );
	}
	
	const char* GetMachineName()
	{
		return m_MachineName.Base();
	}


// ITCPSocketHandler implementation (thread-safe stuff).
public:

	virtual void Init( IThreadedTCPSocket *pSocket )
	{
		m_pSocket = pSocket;
	}

	virtual void OnPacketReceived( CTCPPacket *pPacket )
	{
		// Set who this message came from.
		pPacket->SetUserData( m_iConnection );
		Assert( m_iConnection >= 0 && m_iConnection < 2048 );

		// Store it in the global list.
		CCriticalSectionLock csLock( &g_VMPIMessagesCS );
		csLock.Lock();
			
			g_VMPIMessages.AddToTail( pPacket );

			if ( g_VMPIMessages.Count() == 1 )
				g_VMPIMessagesEvent.SetEvent();
	}

	virtual void OnError( int errorCode, const char *pErrorString )
	{
		if ( !g_bMPIMaster )
		{
			Msg( "%s - CVMPIConnection::OnError( %s )\n", GetMachineName(), pErrorString );
		}

		CCriticalSectionLock csLock( &g_ErrorSocketsCS );
		csLock.Lock();

			m_ErrorString.CopyArray( pErrorString, strlen( pErrorString ) + 1 );
			
			g_ErrorSockets.AddToTail( this );
			
			// Notify the main thread that a socket is in trouble!
			g_ErrorSocketsEvent.SetEvent();

		// Make sure the main thread picks up this error soon.
		InterlockedIncrement( &m_ErrorSignal );	
	}


private:
	
	CUtlVector<char> m_MachineName;
	CUtlVector<char> m_ErrorString;
	long m_ErrorSignal;
	int m_iConnection;
	IThreadedTCPSocket *m_pSocket;
};


class CVMPIConnectionCreator : public IHandlerCreator
{
public:
	virtual ITCPSocketHandler* CreateNewHandler()
	{
		Assert( g_nConnections < MAX_VMPI_CONNECTIONS );
		CVMPIConnection *pRet = new CVMPIConnection( g_nConnections );
		g_Connections[g_nConnections++] = pRet;
		return pRet;
	}
};



// ---------------------------------------------------------------------------------------- //
// Helpers.
// ---------------------------------------------------------------------------------------- //

const char* VMPI_FindArg( int argc, char **argv, const char *pName, const char *pDefault )
{
	for ( int i=0; i < argc; i++ )
	{
		if ( stricmp( argv[i], pName ) == 0 )
		{
			if ( (i+1) < argc )
				return argv[i+1];
			else
				return pDefault;
		}
	}
	return NULL;
}


void ParseOptions( int argc, char **argv )
{
	if ( VMPI_FindArg( argc, argv, "-mpi_NoTimeout" ) )
		ThreadedTCP_EnableTimeouts( false );
	
	const char *pVerbose = VMPI_FindArg( argc, argv, "-mpi_Verbose", "1" );
	if ( pVerbose )
	{
		if ( pVerbose[0] == '1' )
			g_iVMPIVerboseLevel = 1;
		else if ( pVerbose[0] == '2' )
			g_iVMPIVerboseLevel = 2;
	}

	if ( VMPI_FindArg( argc, argv, "-mpi_DisableStats" ) )
		g_bMPI_NoStats = true;
}


void SetupDependencyFilename( CDependencyInfo *pInfo )
{
	char baseExeFilename[512];
	if ( !GetModuleFileName( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
		Error( "GetModuleFileName failed." );
	
	Q_strncpy( pInfo->m_OriginalExeDir, baseExeFilename, sizeof( pInfo->m_OriginalExeDir ) );
	char *pCurChar = pInfo->m_OriginalExeDir;
	char *pLast = 0;
	while ( *pCurChar )
	{
		if ( *pCurChar == '/' || *pCurChar == '\\' )
			pLast = pCurChar;
	
		++pCurChar;
	}
	
	if ( pLast )
	{
		Q_strncpy( pInfo->m_OriginalExeFilename, pLast+1, sizeof( pInfo->m_OriginalExeFilename ) );
		*pLast = 0;
	}
	else
	{
		// Assume there is no directory.
		Q_strncpy( pInfo->m_OriginalExeFilename, pInfo->m_OriginalExeDir, sizeof( pInfo->m_OriginalExeFilename ) );
		strcpy( pInfo->m_OriginalExeDir, "." );
	}

	strupr( pInfo->m_OriginalExeDir );
}


bool ReadString( char *pOut, int maxLen, FILE *fp )
{
	if ( !fgets( pOut, maxLen, fp ) || pOut[0] == 0 )
		return false;

	int len = strlen( pOut );
	if ( pOut[len - 1] == '\n' )
		pOut[len - 1] = 0;

	return true;
}


void ParseDependencyFile( CDependencyInfo *pInfo, const char *pDepFilename )
{
	MD5Context_t context;
	MD5Init( &context );


	FILE *fp = fopen( pDepFilename, "rt" );
	if ( !fp )
		Error( "Can't find %s.", pDepFilename );

	if ( !ReadString( pInfo->m_BaseDirName, sizeof( pInfo->m_BaseDirName ), fp ) )
		Error( "First string (base directory) is missing from %s.", pDepFilename );

	char tempStr[256];
	while ( ReadString( tempStr, sizeof( tempStr ), fp ) )
	{
		CDependencyInfo::CDependencyFile *pFile = new CDependencyInfo::CDependencyFile;
		Q_strncpy( pFile->m_Name, tempStr, sizeof( pFile->m_Name ) );

		// Now get the file info.
		char fullFilename[256];
		Q_snprintf( fullFilename, sizeof( fullFilename ), "%s/%s", pInfo->m_OriginalExeDir, pFile->m_Name );
		_finddata_t data;
		long handle = _findfirst( fullFilename, &data );
		if ( handle == -1 )
			Error( "Can't find %s (listed in %s).", fullFilename, pDepFilename );

		pFile->m_Time = data.time_write;
		pFile->m_Size = data.size;

		
		// Update our MD5 hash.
		MD5Update( &context, (const unsigned char*)tempStr, strlen( tempStr ) );
		MD5Update( &context, (const unsigned char*)&pFile->m_Time, sizeof( pFile->m_Time ) );
		MD5Update( &context, (const unsigned char*)&pFile->m_Size, sizeof( pFile->m_Size ) );


		_findclose( handle );

		// Add it to our list of files.
		pInfo->m_Files.AddToTail( pFile );
	}

	fclose( fp );

	MD5Final( pInfo->m_Digest, &context );
}


// This beautiful little snippet comes from the MSDN docs.
void UnixTimeToFileTime( time_t t, LPFILETIME pft )
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}


void SetupDependencyInfo( CDependencyInfo *pInfo, const char *pDependencyFilename )
{
	SetupDependencyFilename( pInfo );
	
	// Parse the dependency file.
	char depFilename[256];
	Q_snprintf( depFilename, sizeof( depFilename ), "%s/%s", pInfo->m_OriginalExeDir, pDependencyFilename );
	ParseDependencyFile( pInfo, depFilename );


	// Now look for a directory with the MD5
	Q_snprintf( pInfo->m_HashedDirName, sizeof( pInfo->m_HashedDirName ), "%s/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		pInfo->m_BaseDirName,
		pInfo->m_Digest[0], pInfo->m_Digest[1], pInfo->m_Digest[2], pInfo->m_Digest[3], 
		pInfo->m_Digest[4], pInfo->m_Digest[5], pInfo->m_Digest[6], pInfo->m_Digest[7], 
		pInfo->m_Digest[8], pInfo->m_Digest[9], pInfo->m_Digest[10], pInfo->m_Digest[11], 
		pInfo->m_Digest[12], pInfo->m_Digest[13], pInfo->m_Digest[14], pInfo->m_Digest[15] );

	if ( _access( pInfo->m_HashedDirName, 0 ) == 0 )
	{
		// Yay! The files are already up there.
		// Verify that the files are actually the ones we want.
		char searchString[512];
		Q_snprintf( searchString, sizeof( searchString ), "%s/*.*", pInfo->m_HashedDirName );
		_finddata_t data;
		long handle = _findfirst( searchString, &data );
		long val = handle;
		while ( val != -1 )
		{
			if ( data.name[0] != '.' )
			{
				CDependencyInfo::CDependencyFile *pFile = pInfo->FindFile( data.name );
				if ( !pFile )
				{
					Error( "%s has extra file %s.", pInfo->m_HashedDirName, data.name );
				}
				
				if ( pFile->m_Size != data.size || 
					( pFile->m_Time != data.time_write && abs( pFile->m_Time - data.time_write ) != 3600 ) )
				{
					Error( "File size or date for %s doesn't match local version.", data.name );
				}
			}

			val = _findnext( handle, &data );
		}
		_findclose( handle );
	}
	else
	{
		Msg("---------------------------------------------------\n"
			"Dependency directory '%s' does not exist.\n"
			"Copying relevant files up...\n"
			"---------------------------------------------------\n", 
			pInfo->m_HashedDirName );

		// Damn. We have to do work.
		if ( _mkdir( pInfo->m_HashedDirName ) == -1 )
			Error( "_mkdir( %s ) failed.", pInfo->m_HashedDirName );

		// Now copy all the files up.
		for ( int i=0; i < pInfo->m_Files.Count(); i++ )
		{
			CDependencyInfo::CDependencyFile *pFile = pInfo->m_Files[i];

			char srcFilename[512], destFilename[512];
			Q_snprintf( srcFilename, sizeof( srcFilename ), "%s/%s", pInfo->m_OriginalExeDir, pFile->m_Name );
			Q_snprintf( destFilename, sizeof( destFilename ), "%s/%s", pInfo->m_HashedDirName, pFile->m_Name );

			if ( !CopyFile( srcFilename, destFilename, TRUE ) )
				Error( "CopyFile( %s, %s ) failed.", srcFilename, destFilename );

			if ( !SetFileAttributes( destFilename, FILE_ATTRIBUTE_NORMAL ) )
				Error( "SetFileAttributes( %s ) failed.", destFilename );
			
			HANDLE hFile = CreateFile(
				destFilename,
				GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL );

			FILETIME newFileTime;
			UnixTimeToFileTime( pFile->m_Time, &newFileTime );
			if ( !SetFileTime( hFile, &newFileTime, &newFileTime, &newFileTime ) )
				Error( "SetFileTime( %s ) failed.", destFilename );
		
			CloseHandle( hFile );
		}
	}
}


int GetCurMicrosecondsAndSleep( int sleepLen )
{
	Sleep( sleepLen );

	CCycleCount cnt;
	cnt.Sample();
	return cnt.GetMicroseconds();
}


// ---------------------------------------------------------------------------------------- //
// CMasterBroadcaster
// This class broadcasts messages looking for workers. The app updates it as often as possible
// and it'll add workers as necessary.
// ---------------------------------------------------------------------------------------- //

#define MASTER_BROADCAST_INTERVAL 600 // Send every N milliseconds.

class CMasterBroadcaster
{
public:
	
	CMasterBroadcaster();
	~CMasterBroadcaster();

	bool Init( int argc, char **argv, const char *pDependencyFilename, int nMaxWorkers );
	void Term();


private:


	void ThreadFn();
	static DWORD WINAPI StaticThreadFn( LPVOID lpParameter );

	bool Update();


private:

	ITCPConnectSocket *m_pListenSocket;
	ISocket *m_pSocket;

	char m_PacketData[512];
	bf_write m_PacketBuf;

	DWORD m_LastSendTime;

	CVMPIConnectionCreator m_ConnectionCreator;
	int m_nMaxWorkers;

	HANDLE m_hThread;
	CEvent m_hShutdownEvent;
	CEvent m_hShutdownReply;
};


CMasterBroadcaster::CMasterBroadcaster()
{
	m_pListenSocket = NULL;
	m_pSocket = NULL;
}

CMasterBroadcaster::~CMasterBroadcaster()
{
	Term();
}

bool CMasterBroadcaster::Init( int argc, char **argv, const char *pDependencyFilename, int nMaxWorkers )
{
	g_bMPIMaster = true;
	m_nMaxWorkers = nMaxWorkers;


	if ( argc <= 0 )
		Error( "MPI_Init_Master: argc <= 0!" );


	ParseOptions( argc, argv );


	// Open the file that tells us where to stick our executables.

	// Make sure they used UNC paths. It usually won't work if they don't.
	// They can override this behavior by adding -NoUNC on the command line.
	char workerExeFilename[512];
	if ( VMPI_FindArg( argc, argv, "-mpi_UseUNC" ) )
	{
		const char *pExeFilename = argv[0];
		if ( pExeFilename[0] != '\\' || pExeFilename[1] != '\\' )
			Error( "MPI_Init_Master: executable filename on the command line must be a UNC path (\\\\computer\\hl2\\bin\\vrad)." );
	
		Q_strncpy( workerExeFilename, argv[0], sizeof( workerExeFilename ) );
	}
	else
	{
		CDependencyInfo dependencyInfo;
		SetupDependencyInfo( &dependencyInfo, pDependencyFilename );
		Q_snprintf( workerExeFilename, sizeof( workerExeFilename ), "%s/%s", dependencyInfo.m_HashedDirName, dependencyInfo.m_OriginalExeFilename );
	}


	m_pListenSocket = NULL;
	int iListenPort;

	const char *pPortStr = VMPI_FindArg( argc, argv, "-mpi_Port" );
	if ( pPortStr )
	{
		iListenPort = atoi( pPortStr );
		m_pListenSocket = ThreadedTCP_CreateListener( &m_ConnectionCreator, iListenPort );
	}
	else
	{
		// Create a socket to listen on.
		CCycleCount cnt;
		cnt.Sample();
		int iTime = (int)cnt.GetMicroseconds();
		srand( (unsigned)iTime );

		for ( int iTest=0; iTest < 20; iTest++ )
		{
			iListenPort = 20000 + rand() % 10000;
			
			//CUniformRandomStream randomStream;
			//randomStream.SetSeed( -iTime ); // Bug in CUniformRandomStream - only generates random #'s if it gets a negative seed.
			//iListenPort = randomStream.RandomInt( 20000, 30000 );

			m_pListenSocket = ThreadedTCP_CreateListener( &m_ConnectionCreator, iListenPort );
			if ( m_pListenSocket )
				break;
		}
	}

	if ( !m_pListenSocket )
	{
		Error( "Can't bind a listen socket in port range [%d, %d].", VMPI_MASTER_PORT_FIRST, VMPI_MASTER_PORT_LAST );
	}


	// Create a socket to broadcast from.
	m_pSocket = CreateIPSocket();
	if ( !m_pSocket->BindToAny( 0 ) )
		Error( "MPI_Init_Master: can't bind a socket" );

	// Come up with a unique job ID.
	int jobID[4];
	jobID[0] = GetCurMicrosecondsAndSleep( 1 );
	jobID[1] = GetCurMicrosecondsAndSleep( 1 );
	jobID[2] = GetCurMicrosecondsAndSleep( 1 );
	jobID[3] = GetCurMicrosecondsAndSleep( 1 );

	
	// Broadcast out to tell all the machines we want workers.
	m_PacketBuf.StartWriting( m_PacketData, sizeof( m_PacketData ) );
	m_PacketBuf.WriteByte( VMPI_PROTOCOL_VERSION );

	const char *pPassword = VMPI_FindArg( argc, argv, "-mpi_pw", "" );
	m_PacketBuf.WriteString( pPassword );
	
	m_PacketBuf.WriteByte( VMPI_LOOKING_FOR_WORKERS );
	m_PacketBuf.WriteLong( iListenPort );	// Tell the port that we're listening on.
	m_PacketBuf.WriteLong( jobID[0] );
	m_PacketBuf.WriteLong( jobID[1] );
	m_PacketBuf.WriteLong( jobID[2] );
	m_PacketBuf.WriteLong( jobID[3] );
	m_PacketBuf.WriteWord( argc+2 );

	// Write the alternate exe name.
	m_PacketBuf.WriteString( workerExeFilename );

	// Write the machine name of the master into the command line. It's ignored by the code, but it's useful
	// if a job crashes the workers - by looking at the command line in vmpi_service, you can see who ran the job.
	m_PacketBuf.WriteString( "-mpi_MasterName" );
	m_PacketBuf.WriteString( VMPI_GetLocalMachineName() );
	
	for ( int i=1; i < argc; i++ )
		m_PacketBuf.WriteString( argv[i] );


	// Add ourselves as the first process (rank 0).
	m_ConnectionCreator.CreateNewHandler();

	// Initiate as many connections as we can for a few seconds.
	m_LastSendTime = Plat_MSTime() - MASTER_BROADCAST_INTERVAL*2;
	
	
	m_hShutdownEvent.Init( false, false );
	m_hShutdownReply.Init( false, false );

	DWORD dwThreadID = 0;
	m_hThread = CreateThread( 
		NULL,
		0,
		&CMasterBroadcaster::StaticThreadFn,
		this,
		0,
		&dwThreadID );

	if ( m_hThread )
	{
		SetThreadPriority( m_hThread, THREAD_PRIORITY_HIGHEST );
		return true;
	}
	else
	{
		return false;
	}
}


bool CMasterBroadcaster::Update()
{
	CCriticalSectionLock connectionsLock( &g_ConnectionsCS );
	connectionsLock.Lock();

	// Don't accept any more connections when we've hit the limit.
	if ( g_nConnections > m_nMaxWorkers )
		return false;

	// Only broadcast our presence so often.
	DWORD curTime = Plat_MSTime();
	if ( curTime - m_LastSendTime >= MASTER_BROADCAST_INTERVAL )
	{
		for ( int iBroadcastPort=VMPI_SERVICE_PORT; iBroadcastPort <= VMPI_LAST_SERVICE_PORT; iBroadcastPort++ )
		{
			m_pSocket->Broadcast( m_PacketBuf.GetBasePointer(), m_PacketBuf.GetNumBytesWritten(), iBroadcastPort );
		}
		m_LastSendTime = curTime;
	}	
	
	IThreadedTCPSocket *pNewConn = NULL;
	if ( m_pListenSocket->Update( &pNewConn, 50 ) )
	{
		if ( pNewConn )
		{
			CIPAddr workerAddr = pNewConn->GetRemoteAddr();
			//Warning( "Connection %d: %d.%d.%d.%d\n", g_nConnections-1, workerAddr.ip[0], workerAddr.ip[1], workerAddr.ip[2], workerAddr.ip[3] );
			Msg( "%d.. ", g_nConnections-1 );

			// Send this guy all the persistent packets.
			CCriticalSectionLock csLock( &g_PersistentPacketsCS );
			csLock.Lock();

			FOR_EACH_LL( g_PersistentPackets, iPacket )
			{
				PersistentPacket *pPacket = g_PersistentPackets[iPacket];
				VMPI_SendData( pPacket->Base(), pPacket->Count(), g_nConnections-1 );
			}

			// Limit the # of workers they can have.
			if ( g_nConnections > m_nMaxWorkers )
			{
				Msg( "\n* Hit -mpi_WorkerCount limit of %d. No more connections accepted.\n\n", m_nMaxWorkers );
			}
			return true;
		}
	}

	return false;
}


void CMasterBroadcaster::ThreadFn()
{
	// Update every 100ms or until the main thread tells us to go away.
	while ( WaitForSingleObject( m_hShutdownEvent.GetEventHandle(), 10 ) == WAIT_TIMEOUT )
	{
		while ( Update() )
		{
		}
	}
	m_hShutdownReply.SetEvent();
}


DWORD CMasterBroadcaster::StaticThreadFn( LPVOID lpParameter )
{
	((CMasterBroadcaster*)lpParameter)->ThreadFn();
	return 0;
}


void CMasterBroadcaster::Term()
{
	// Shutdown the update thread.
	if ( m_hThread )
	{
		m_hShutdownEvent.SetEvent();
		WaitForSingleObject( m_hThread, INFINITE );
		CloseHandle( m_hThread );
		m_hThread = 0;
	}
	
	if ( m_pSocket )
	{
		m_pSocket->Release();
		m_pSocket = NULL;
	}

	if ( m_pListenSocket )
	{
		m_pListenSocket->Release();
		m_pListenSocket = NULL;
	}
}


CMasterBroadcaster g_MasterBroadcaster;


// ---------------------------------------------------------------------------------------- //
// CDispatchReg.
// ---------------------------------------------------------------------------------------- //

CDispatchReg::CDispatchReg( int iPacketID, VMPIDispatchFn fn )
{
	Assert( iPacketID >= 0 && iPacketID < MAX_VMPI_PACKET_IDS );
	Assert( !g_VMPIDispatch[iPacketID] );
	g_VMPIDispatch[iPacketID] = fn;
}


// ---------------------------------------------------------------------------------------- //
// Helpers.
// ---------------------------------------------------------------------------------------- //

bool MPI_Init_Worker( int argc, char **argv, const int masterIP[4], const int masterPort )
{
	g_bMPIMaster = false;

	for ( int i=0; i < argc; i++ )
	{
		Msg( "arg %d: %s\n", i, argv[i] );
	}

	ParseOptions( argc, argv );

	// Make a connector to try connect to the master.
	CIPAddr masterAddr( masterIP, masterPort );
	CVMPIConnectionCreator connectionCreator;

	ITCPConnectSocket *pConnectSocket = NULL;
	int iPort;
	for ( iPort=VMPI_WORKER_PORT_FIRST; iPort <= VMPI_WORKER_PORT_LAST; iPort++ )
	{
		pConnectSocket = ThreadedTCP_CreateConnector(
			masterAddr, 
			CIPAddr( 0, 0, 0, 0, iPort ),
			&connectionCreator );

		if ( pConnectSocket )
			break;
	}
	if ( !pConnectSocket )
	{
		Error( "Can't bind a port in range [%d, %d].", VMPI_WORKER_PORT_FIRST, VMPI_WORKER_PORT_LAST );
	}


	// Now wait for a connection.
	CWaitTimer wait( 5 );
	while ( 1 )
	{
		IThreadedTCPSocket *pSocket = NULL;
		if ( pConnectSocket->Update( &pSocket, 100 ) )
		{
			if ( pSocket )
			{
				return true;
			}
		}
		else
		{
			pConnectSocket->Release();
			Error( "ITCPConnectSocket::Update() errored out" );
		}
		
		if( wait.ShouldKeepWaiting() )
			Sleep( 100 );
		else
			break;
	};

	// Never made a connection, shucks.
	pConnectSocket->Release();
	return false;
}


bool InitMasterModal( int argc, char **argv, const char *pDependencyFilename, int nMinWorkers, int nMaxWorkers )
{
	if ( !g_MasterBroadcaster.Init( argc, argv, pDependencyFilename, nMaxWorkers ) )
		return false;

	Msg( "Waiting for connections... press spacebar to start early.\n" );

	// Now wait for the wait time or until there are enough workers.
	float flWaitTime = 12.0f;
	const char *pWaitTimeStr = VMPI_FindArg( argc, argv, "-mpi_WaitTime" );
	if ( pWaitTimeStr )
		flWaitTime = atof( pWaitTimeStr );

	float startTime = Plat_FloatTime();
	while ( (Plat_FloatTime()-startTime) < flWaitTime || 
		g_nConnections <= nMinWorkers )
	{
		// If we exceed the max # of workers, then we're done.
		if ( g_nConnections > nMaxWorkers )
			break;

		if ( kbhit() && getch() == ' ' )
		{
			Msg( "Spacebar pressed.. starting early.\n" );
			break;
		}

		Sleep( 100 );
	}

	g_MasterBroadcaster.Term();

	return g_nConnections > 1;
}


bool InitMaster( int argc, char **argv, const char *pDependencyFilename )
{
	// Figure out the min and max worker counts.
	int nMinWorkers = 1;
	const char *pMinWorkers = VMPI_FindArg( argc, argv, "-mpi_MinWorkers" );
	if ( pMinWorkers )
	{
		nMinWorkers = atoi( pMinWorkers );
		nMinWorkers = clamp( nMinWorkers, 1, 256 );
		Warning( "-mpi_MinWorkers: %d\n", nMinWorkers );
	}

	int nMaxWorkers = -1;
	const char *pProcCount = VMPI_FindArg( argc, argv, "-mpi_WorkerCount" );
	if ( pProcCount )
	{
		nMaxWorkers = atoi( pProcCount );
		Warning( "-mpi_WorkerCount: waiting for %d processes to join.\n", nMaxWorkers );
	}
	else
	{
		nMaxWorkers = DEFAULT_MAX_WORKERS;
	}
	nMaxWorkers = clamp( nMaxWorkers, 2, MAX_VMPI_CONNECTIONS );
	nMaxWorkers = max( nMaxWorkers, nMinWorkers );


	// Startup..
	if ( VMPI_FindArg( argc, argv, "-mpi_AllowDropIn", NULL ) )
	{
		// If we want to allow workers to drop in, just leave the broadcaster initialized
		// and its thread will continue to accept connections.
		return g_MasterBroadcaster.Init( argc, argv, pDependencyFilename, nMaxWorkers );
	}
	else
	{
		return InitMasterModal( argc, argv, pDependencyFilename, nMinWorkers, nMaxWorkers );
	}
}


bool VMPI_Init( int argc, char **argv, const char *pDependencyFilename )
{
	// Init event objects.
	g_VMPIMessagesEvent.Init( false, false );
	g_ErrorSocketsEvent.Init( false, false );


	#if defined( _DEBUG )
		
		for ( int iArg=0; iArg < argc; iArg++ )
		{
			Warning( "%s\n", argv[iArg] );
		}
		
		Warning( "\n" );
	
	#endif


	// Were we launched by the vmpi service as a worker?
	bool bWorker = false;
	int ip[4], port = 0;
	const char *pMasterIP = VMPI_FindArg( argc, argv, "-mpi_worker", NULL );
	if ( pMasterIP )
	{
		sscanf( pMasterIP, "%d.%d.%d.%d:%d", &ip[0], &ip[1], &ip[2], &ip[3], &port );
		bWorker = true;
	}
	
	if ( bWorker )
	{
		return MPI_Init_Worker( argc, argv, ip, port );
	}
	else
	{
		return InitMaster( argc, argv, pDependencyFilename );
	}
}


void VMPI_Finalize()
{
#ifdef ALLOW_DROP_IN
	g_MasterBroadcaster.Term();
#endif

	DistributeWork_Cancel();

	// Get rid of all the sockets.
	for ( int iConn=0; iConn < g_nConnections; iConn++ )
		delete g_Connections[iConn];

	g_nConnections = 0;

	// Get rid of all the packets.
	FOR_EACH_LL( g_VMPIMessages, i )
	{
		g_VMPIMessages[i]->Release();
	}
	g_VMPIMessages.Purge();

	g_PersistentPackets.PurgeAndDeleteElements();
}


int VMPI_GetCurrentNumberOfConnections()
{
	return g_nConnections;
}


void InternalHandleSocketErrors()
{
	// Copy the list of sockets with errors into a local array so we can handle all the errors outside
	// the mutex, thus avoiding potential deadlock if any error handlers call Error().
	CUtlVector<CVMPIConnection*> errorSockets;
	
	CCriticalSectionLock csLock( &g_ErrorSocketsCS );
	csLock.Lock();

		errorSockets.SetSize( g_ErrorSockets.Count() );
		int iCur = 0;
		FOR_EACH_LL( g_ErrorSockets, i )
		{
			errorSockets[iCur++] = g_ErrorSockets[i];
		}

		g_ErrorSockets.Purge();

	csLock.Unlock();

	// Handle the errors.
	for ( i=0; i < errorSockets.Count(); i++ )
	{
		errorSockets[i]->HandleDisconnect();
	}
}


void VMPI_HandleSocketErrors( unsigned long timeout )
{
	DWORD ret = WaitForSingleObject( g_ErrorSocketsEvent.GetEventHandle(), timeout );
	if ( ret == WAIT_OBJECT_0 )
	{
		InternalHandleSocketErrors();
	}
}


// If bWait is false, then this function returns false immediately if there are no messages waiting.
bool VMPI_GetNextMessage( MessageBuffer *pBuf, int *pSource, unsigned long startTimeout )
{
	HANDLE handles[2] = { g_ErrorSocketsEvent.GetEventHandle(), g_VMPIMessagesEvent.GetEventHandle() };
	
	DWORD startTime = Plat_MSTime();
	DWORD timeout = startTimeout;

	while ( 1 )
	{
		DWORD ret = WaitForMultipleObjects( ARRAYSIZE( handles ), handles, FALSE, timeout );
		if ( ret == WAIT_TIMEOUT )
		{
			return false;
		}
		else if ( ret == WAIT_OBJECT_0 )
		{
			// A socket had an error. Handle all socket errors.
			InternalHandleSocketErrors();

			// Update the timeout.
			DWORD delta = Plat_MSTime() - startTime;
			if ( delta >= startTimeout )
				return false;

			timeout = startTimeout - delta;
			continue;
		}
		else if ( ret == (WAIT_OBJECT_0 + 1) )
		{
			// Read out the next message.
			CCriticalSectionLock csLock( &g_VMPIMessagesCS );
			csLock.Lock();

				int iHead = g_VMPIMessages.Head();
				CTCPPacket *pPacket = g_VMPIMessages[iHead];
				g_VMPIMessages.Remove( iHead );

				// Set the event again if there are more messages waiting.
				if ( g_VMPIMessages.Count() > 0 )
					g_VMPIMessagesEvent.SetEvent();

			csLock.Unlock();

			// Copy it into their message buffer.
			pBuf->setLen( pPacket->GetLen() );
			memcpy( pBuf->data, pPacket->GetData(), pPacket->GetLen() );

			*pSource = pPacket->GetUserData();
			Assert( *pSource >= 0 && *pSource < g_nConnections );
			
			// Update global stats about how much data we've received.
			++g_nMessagesReceived;
			g_nBytesReceived += pPacket->GetLen() + 4;	// (4 bytes extra for the packet length)

			/*
			if ( !g_bMPIMaster )
			{
				static int s_nReceived = 0;
				++s_nReceived;
				Msg( "(%d): got (%d, %d)\n", s_nReceived, pBuf->data[0], pBuf->data[1] );
			}*/
			
			// Free the memory associated with the packet.
			pPacket->Release();
			return true;
		}
		else
		{
			Error( "VMPI_GetNextMessage: WaitForSingleObject returned %lu", ret );
			return false;
		}
	}
}


bool VMPI_InternalDispatch( MessageBuffer *pBuf, int iSource )
{
	if ( pBuf->getLen() >= 1 &&
		pBuf->data[0] >= 0 && pBuf->data[0] < MAX_VMPI_PACKET_IDS &&
		g_VMPIDispatch[pBuf->data[0]] )
	{
		return g_VMPIDispatch[ pBuf->data[0] ]( pBuf, iSource, pBuf->data[0] );
		
	}
	else
	{
		return false;
	}
}


bool VMPI_DispatchNextMessage( unsigned long timeout )
{
	MessageBuffer *pBuf = &g_DispatchBuffer;

	while ( 1 )
	{
		int iSource;
		if ( !VMPI_GetNextMessage( pBuf, &iSource, timeout ) )
			return false;
		
		if ( VMPI_InternalDispatch( pBuf, iSource ) )
		{
			break;
		}
		else
		{
			// Oops! What is this packet?
			Assert( false );
		}
	}

	return true;
}


bool VMPI_DispatchUntil( MessageBuffer *pBuf, int *pSource, int packetID, int subPacketID, bool bWait )
{
	while ( 1 )
	{
		if ( !VMPI_GetNextMessage( pBuf, pSource, bWait ? VMPI_TIMEOUT_INFINITE : 0 ) )
			return false;
		
		if ( !VMPI_InternalDispatch( pBuf, *pSource ) )
		{		
			if ( pBuf->getLen() >= 1 && pBuf->data[0] == packetID )
			{
				if ( subPacketID == -1 )
					return true;

				if ( pBuf->getLen() >= 2 && pBuf->data[1] == subPacketID )
					return true;
			}
		
			// Oops! What is this packet?
			// Note: the most common case where this happens is if it finishes a BuildFaceLights run
			// and is in an AppBarrier and one of the workers is still finishing up some work given to it.
			// It'll be waiting for a barrier packet, and it'll get results. In that case, the packet should
			// be discarded like we do here, so maybe this assert won't be necessary.
			//Assert( false );
		}
	}
}


bool VMPI_SendData( void *pData, int nBytes, int iDest )
{
	return VMPI_SendChunks( &pData, &nBytes, 1, iDest );
}


bool VMPI_SendChunks( void const * const *pChunks, const int *pChunkLengths, int nChunks, int iDest )
{
	if ( iDest == VMPI_SEND_TO_ALL )
	{
		// Don't want new connections while in here!
		CCriticalSectionLock connectionsLock( &g_ConnectionsCS );
		connectionsLock.Lock();

		for ( int i=0; i < g_nConnections; i++ )
			VMPI_SendChunks( pChunks, pChunkLengths, nChunks, i );

		return true;
	}
	else if ( iDest == VMPI_PERSISTENT )
	{
		// Don't want new connections while in here!
		CCriticalSectionLock connectionsLock( &g_ConnectionsCS );
		connectionsLock.Lock();

		CCriticalSectionLock csLock( &g_PersistentPacketsCS );
		csLock.Lock();

		// Send the packet to everyone.
		for ( int i=0; i < g_nConnections; i++ )
			VMPI_SendChunks( pChunks, pChunkLengths, nChunks, i );

		// Remember to send it to the new workers.
		if ( iDest == VMPI_PERSISTENT )
		{
			PersistentPacket *pNew = new PersistentPacket;
			for ( i=0; i < nChunks; i++ )
				pNew->AddMultipleToTail( pChunkLengths[i], (const char*)pChunks[i] );

			g_PersistentPackets.AddToTail( pNew );
		}
	
		return true;
	}
	else
	{
		g_nMessagesSent++;
		g_nBytesSent += 4; // for message tag.
		for ( int i=0; i < nChunks; i++ )
			g_nBytesSent += pChunkLengths[i];

		CVMPIConnection *pConnection = g_Connections[iDest];
		IThreadedTCPSocket *pSocket = pConnection->GetSocket();
		if ( !pSocket )
			return false;

		return pSocket->SendChunks( pChunks, pChunkLengths, nChunks );
	}
}


bool VMPI_Send2Chunks( const void *pChunk1, int chunk1Len, const void *pChunk2, int chunk2Len, int iDest )
{
	const void *pChunks[2] = { pChunk1, pChunk2 };
	int len[2] = { chunk1Len, chunk2Len };
	return VMPI_SendChunks( pChunks, len, ARRAYSIZE( pChunks ), iDest );
}


bool VMPI_Send3Chunks( const void *pChunk1, int chunk1Len, const void *pChunk2, int chunk2Len, const void *pChunk3, int chunk3Len, int iDest )
{
	const void *pChunks[3] = { pChunk1, pChunk2, pChunk3 };
	int len[3] = { chunk1Len, chunk2Len, chunk3Len };
	return VMPI_SendChunks( pChunks, len, ARRAYSIZE( pChunks ), iDest );
}


void VMPI_AddDisconnectHandler( VMPI_Disconnect_Handler handler )
{
	g_DisconnectHandlers.AddToTail( handler );
}


bool VMPI_IsProcConnected( int procID )
{
	if ( procID < 0 || procID >= g_nConnections )
	{
		Assert( false );
		return false;
	}

	return g_Connections[procID]->GetSocket() != NULL;
}

void VMPI_Sleep( unsigned long ms )
{
	Sleep( ms );
}


const char* VMPI_GetMachineName( int iProc )
{
	if ( iProc < 0 || iProc >= g_nConnections )
	{
		Assert( false );
		return "invalid index";
	}

	return g_Connections[iProc]->GetMachineName();
}


void VMPI_SetMachineName( int iProc, const char *pName )
{
	if ( iProc < 0 || iProc >= g_nConnections )
	{
		Assert( false );
		return;
	}

	g_Connections[iProc]->SetMachineName( pName );
}


const char* VMPI_GetLocalMachineName()
{
	static char cName[MAX_COMPUTERNAME_LENGTH+1];
	DWORD len = sizeof( cName );
	if ( GetComputerName( cName, &len ) )
		return cName;
	else
		return "(error in GetComputerName)";
}


	
