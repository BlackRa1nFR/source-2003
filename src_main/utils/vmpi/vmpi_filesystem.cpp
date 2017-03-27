
#include <winsock2.h>
#include "filesystem.h"
#include "vmpi_filesystem.h"
#include "tier0/dbg.h"
#include "vstdlib/strtools.h"
#include "vstdlib/random.h"
#include "messbuf.h"
#include "vmpi_dispatch.h"
#include "utllinkedlist.h"
#include <io.h>
#include <sys/stat.h>
#include "vmpi.h"
#include "utlvector.h"
#include "filesystem_tools.h"
#include "zlib.h"
#include "iphelpers.h"
#include "vmpi_tools_shared.h"


#define MULTICAST_CHUNK_PAYLOAD_SIZE	(1024*1)
#define MULTICAST_TRANSMIT_RATE			(1024*1024*6)	// N megs per second
#define MINIMUM_SLEEP_MS				1

#define NUM_BUFFERED_CHUNK_ACKS	512
#define ACK_FLUSH_INTERVAL		500	// Flush the ack queue twice per second.
												  

#define VMPI_PACKETID_FILESYSTEM	0	// The file system reserves this packet ID.
										// All application traffic must set its first byte to something other
										// than this value.

// Sub packet IDs specific to the VMPI file system.
#define VMPI_FSPACKETID_FILE_REQUEST	1	// Sent by the worker to request a file.
#define VMPI_FSPACKETID_FILE_RESPONSE	2	// Master's response to a file request.
#define VMPI_FSPACKETID_CHUNK_RECEIVED	3	// Sent by workers to tell the master they received a chunk.
#define VMPI_FSPACKETID_FILE_RECEIVED	4	// Sent by workers to tell the master they received the whole file.


char g_cPacketID_FileSystem = VMPI_PACKETID_FILESYSTEM;


extern void IPAddrToSockAddr( const CIPAddr *pIn, sockaddr_in *pOut );


bool g_bDisableFileAccess = false;
static IVMPIFileSystemHelpers *g_pHelpers = NULL;



// This does a fast zlib compression of the source data into the 'out' buffer.
bool ZLibCompress( const void *pData, int len, CUtlVector<char> &out )
{
	int outStartLen = len;
RETRY:;

	// Prepare the compression stream.
	z_stream zs;
	memset( &zs, 0, sizeof( zs ) );
	
	if ( deflateInit( &zs, 1 ) != Z_OK )
		return false;
	

	// Now compress it into the output buffer.
	out.SetSize( outStartLen );

	zs.next_in = (unsigned char*)pData;
	zs.avail_in = len;

	zs.next_out = (unsigned char*)out.Base();
	zs.avail_out = out.Count();

	int ret = deflate( &zs, Z_FINISH );
	deflateEnd( &zs );

	if ( ret == Z_STREAM_END )
	{
		// Get rid of whatever was left over.
		out.RemoveMultiple( zs.total_out, out.Count() - zs.total_out );
		return true;
	}
	else if ( ret == Z_OK )
	{
		// Need more space in the output buffer.
		outStartLen += 1024 * 128;
		goto RETRY;
	}
	else
	{
		return false;
	}
}


bool ZLibDecompress( const void *pInput, int inputLen, void *pOut, int outLen )
{
	z_stream decompressStream;
	
	// Initialize the decompression stream.
	memset( &decompressStream, 0, sizeof( decompressStream ) );
	if ( inflateInit( &decompressStream ) != Z_OK )
		return false;

	// Decompress all this stuff and write it to the file.
	decompressStream.next_in = (unsigned char*)pInput;
	decompressStream.avail_in = inputLen;

	char *pOutChar = (char*)pOut;
	while ( decompressStream.avail_in )
	{
		decompressStream.total_out = 0;
		decompressStream.next_out = (unsigned char*)pOutChar;
		decompressStream.avail_out = outLen - (pOutChar - (char*)pOut);

		int ret = inflate( &decompressStream, Z_NO_FLUSH );
		if ( ret != Z_OK && ret != Z_STREAM_END )
			return false;


		pOutChar += decompressStream.total_out;

		if ( ret == Z_STREAM_END )
		{
			if ( (pOutChar - (char*)pOut) == outLen )
			{
				return true;
			}
			else
			{
				Assert( false );
				return false;
			}
		}
	}

	Assert( false ); // Should have gotten to Z_STREAM_END.
	return false;
}



// ----------------------------------------------------------------------------- //
// This is the thread that sits and multicasts any chunks of files that need to
// be sent out to clients.
// ----------------------------------------------------------------------------- //

class CMulticastFileInfo
{
public:
	unsigned long m_CompressedSize;
	unsigned long m_UncompressedSize;
	unsigned short m_FileID;
	unsigned short m_nChunks;
};


class CMasterMulticastThread
{
public:

					CMasterMulticastThread();
					~CMasterMulticastThread();

	// This creates the socket and starts the thread (initially in an idle state since it doesn't
	// know of any files anyone wants).
	bool			Init( unsigned short localPort, const CIPAddr *pAddr );
	void			Term();

	// Returns -1 if there is an error.
	int				FindOrAddFile( const char *pFilename );	
	const CUtlVector<char>& GetFileData( int iFile ) const;
	
	// When a client requests a files, this is called to tell the thread to start
	// adding chunks from the specified file into the queue it's multicasting.
	//
	// Returns -1 if the file isn't there. Otherwise, it returns the file ID
	// that will be sent along with the file's chunks in the multicast packets.
	int				AddFileRequest( const char *pFilename, int clientID );
	
	// As each client receives multicasted chunks, they ack them so the master can
	// stop transmitting any chunks it knows nobody wants.
	void			OnChunkReceived( int fileID, int clientID, int iChunk );
	void			OnFileReceived( int fileID, int clientID );

	// Call this if a client disconnects so it can stop sending anything this client wants.
	void			OnClientDisconnect( int clientID );

	void CreateVirtualFile( const char *pFilename, const void *pData, unsigned long fileLength );

private:

	class CChunkInfo
	{
	public:
		unsigned short	m_iChunk;
		unsigned short	m_RefCount;				// How many clients want this chunk.
		unsigned short	m_iActiveChunksIndex;	// Index into m_ActiveChunks.
	};


	// This stores a client's reference to a file so it knows which pieces of the file the client needs.
	class CClientFileInfo
	{
	public:
		bool NeedsChunk( int i ) const { return (m_ChunksToSend[i>>3] & (1 << (i&7))) != 0; }	
	
	public:
		int							m_ClientID;
		CUtlVector<unsigned char>	m_ChunksToSend;	// One bit for each chunk that this client still wants.
		int m_nChunksLeft;
	};


	class CMulticastFile
	{
	public:
		~CMulticastFile()
		{
			m_Clients.PurgeAndDeleteElements();
		}

		const char* GetFilename() { return m_Filename.Base(); }


	public:
		int m_nCycles; // How many times has the multicast thread visited this file?

		// This is sent along with every packet. If a client gets a chunk and doesn't have that file's
		// info, the client will receive that file too.
		CUtlVector<char> m_Filename;
		CMulticastFileInfo m_Info;

		// This is stored so the app can read out the uncompressed data.
		CUtlVector<char>				m_UncompressedData;

		// zlib-compressed file data
		CUtlVector<char>				m_Data;	

		// m_Chunks holds the chunks by index.
		// m_ActiveChunks holds them sorted by whether they're active or not.
		// 
		// Each chunk has a refcount. While the refcount is > 0, the chunk is in the first
		// half of m_ActiveChunks. When the refcount gets to 0, the chunk is moved to the end of 
		// m_ActiveChunks. That way, we can iterate through the chunks that need to be sent and 
		// stop iterating the first time we hit one with a refcount of 0.
		CUtlVector<CChunkInfo>			m_Chunks;
		CUtlLinkedList<CChunkInfo*,int>	m_ActiveChunks;

		// This tells which clients want pieces of this file.
		CUtlLinkedList<CClientFileInfo*,int>	m_Clients;
	};


private:

	static DWORD WINAPI StaticMulticastThread( LPVOID pParameter );
	DWORD MulticastThread();

	// Called after pFile->m_UncompressedData has been setup. This compresses the data, prepares the header,
	// copies the filename, and adds it into the queue for the multicast thread.
	int FinishFileSetup( CMulticastFile *pFile, const char *pFilename );

	void IncrementChunkRefCount( CMasterMulticastThread::CMulticastFile *pFile, int iChunk );
	void DecrementChunkRefCount( int iFile, int iChunk );
	
	int FindFile( const char *pName );

	bool FindWarningSuppression( const char *pFilename );
	void AddWarningSuppression( const char *pFilename );

private:
	
	CUtlLinkedList<CMulticastFile*,int>		m_Files;

	// This tracks how many chunks we have that want to be sent.
	int m_nTotalActiveChunks;

	SOCKET m_Socket;
	sockaddr_in m_MulticastAddr;
	
	HANDLE m_hThread;
	CRITICAL_SECTION m_CS;

	// Events used to communicate with our thread.
	HANDLE m_hTermEvent;

	// The thread walks through this as it spews chunks of data.
	volatile int m_iCurFile;			// Index into m_Files.
	volatile int m_iCurActiveChunk;		// Current index into CMulticastFile::m_ActiveChunks.

	CUtlLinkedList<char*,int> m_WarningSuppressions;
};


CMasterMulticastThread::CMasterMulticastThread()
{
	m_hThread = NULL;
	m_Socket = INVALID_SOCKET;
	m_nTotalActiveChunks = 0;
	m_iCurFile = m_iCurActiveChunk = -1;
	
	m_hTermEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	InitializeCriticalSection( &m_CS );
}


CMasterMulticastThread::~CMasterMulticastThread()
{
	Term();
	
	CloseHandle( m_hTermEvent );

	DeleteCriticalSection( &m_CS );
}


bool CMasterMulticastThread::Init( unsigned short localPort, const CIPAddr *pAddr )
{
	Term();

	// First, create our socket.
	m_Socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
	if ( m_Socket == INVALID_SOCKET )
	{
		Warning( "CMasterMulticastThread::Init - socket() failed\n" );
		return false;
	}

		
	// Bind to INADDR_ANY.
	CIPAddr localAddr( 0, 0, 0, 0, localPort );
	
	sockaddr_in addr;
	IPAddrToSockAddr( &localAddr, &addr );

	int status = bind( m_Socket, (sockaddr*)&addr, sizeof(addr) );
	if ( status != 0 )
	{
		Term();
		Warning( "CMasterMulticastThread::Init - bind( %d.%d.%d.%d:%d ) failed\n", EXPAND_ADDR( *pAddr ) );
		return false;
	}

	
	// Remember the address we want to send to.
	IPAddrToSockAddr( pAddr, &m_MulticastAddr );
	

	// Now create our thread.
	DWORD dwThreadID = 0;
	m_hThread = CreateThread( NULL, 0, &CMasterMulticastThread::StaticMulticastThread, this, 0, &dwThreadID );
	if ( !m_hThread )
	{
		Term();
		Warning( "CMasterMulticastThread::Init - CreateThread failed\n", EXPAND_ADDR( *pAddr ) );
		return false;
	}

	SetThreadPriority( m_hThread, THREAD_PRIORITY_LOWEST );
	return true;
}


void CMasterMulticastThread::Term()
{
	// Stop the thread if it is running.
	if ( m_hThread )
	{
		SetEvent( m_hTermEvent );
		WaitForSingleObject( m_hThread, INFINITE );
		CloseHandle( m_hThread );

		m_hThread = NULL;
	}

	// Close the socket.
	closesocket( m_Socket );
	m_Socket = INVALID_SOCKET;

	// Free up other data.
	m_Files.PurgeAndDeleteElements();
}


int CMasterMulticastThread::AddFileRequest( const char *pFilename, int clientID )
{
	// Firstly, do we already have this file?
	int iFile = FindOrAddFile( pFilename );
	if ( iFile == -1 )
		return -1;

	CMulticastFile *pFile = m_Files[iFile];

	// Now that we have a file setup, merge in this client's info.
	EnterCriticalSection( &m_CS );

		CClientFileInfo *pClient = new CClientFileInfo;
		pClient->m_ClientID = clientID;
		pClient->m_nChunksLeft = pFile->m_Info.m_nChunks;
		pClient->m_ChunksToSend.SetSize( PAD_NUMBER( pFile->m_Info.m_nChunks, 8 ) / 8 );
		memset( pClient->m_ChunksToSend.Base(), 0xFF, pClient->m_ChunksToSend.Count() );
		pFile->m_Clients.AddToTail( pClient );

		for ( int i=0; i < pFile->m_Chunks.Count(); i++ )
		{
			IncrementChunkRefCount( pFile, i );
		}
	
	LeaveCriticalSection( &m_CS );

	return iFile;
}


void CMasterMulticastThread::OnChunkReceived( int fileID, int clientID, int iChunk )
{
	if ( !m_Files.IsValidIndex( fileID ) )
	{
		Warning( "CMasterMulticastThread::OnChunkReceived: invalid file (%d) from client %d\n", fileID, clientID );
		return;
	}

	CMulticastFile *pFile = m_Files[fileID];
	CClientFileInfo *pClient = NULL;
	FOR_EACH_LL( pFile->m_Clients, iClient )
	{
		if ( pFile->m_Clients[iClient]->m_ClientID == clientID )
		{
			pClient = pFile->m_Clients[iClient];
			break;
		}
	}
	if ( !pClient )
	{
		Warning( "CMasterMulticastThread::OnChunkReceived: invalid client ID (%d) for file %s\n", clientID, pFile->GetFilename() );
		return;
	}

	if ( !pFile->m_Chunks.IsValidIndex( iChunk ) )
	{
		Warning( "CMasterMulticastThread::OnChunkReceived: invalid chunk index (%d) for file %s\n", iChunk, pFile->GetFilename() );
		return;
	}

	// Mark that this client doesn't need this chunk anymore.
	pClient->m_ChunksToSend[iChunk >> 3] &= ~(1 << (iChunk & 7));
	pClient->m_nChunksLeft--;

	if ( pClient->m_nChunksLeft == 0 && g_iVMPIVerboseLevel >= 2 )
		Warning( "Client %d got file %s\n", clientID, pFile->GetFilename() );
	
	EnterCriticalSection( &m_CS );
		DecrementChunkRefCount( fileID, iChunk );
	LeaveCriticalSection( &m_CS );
}


void CMasterMulticastThread::OnFileReceived( int fileID, int clientID )
{
	if ( !m_Files.IsValidIndex( fileID ) )
	{
		Warning( "CMasterMulticastThread::OnChunkReceived: invalid file (%d) from client %d\n", fileID, clientID );
		return;
	}

	CMulticastFile *pFile = m_Files[fileID];
	for ( int i=0; i < pFile->m_Info.m_nChunks; i++ )
		OnChunkReceived( fileID, clientID, i );
}


void CMasterMulticastThread::OnClientDisconnect( int clientID )
{
	EnterCriticalSection( &m_CS );

	// Remove all references from this client.
	FOR_EACH_LL( m_Files, iFile )
	{
		CMulticastFile *pFile = m_Files[iFile];

		FOR_EACH_LL( pFile->m_Clients, iClient )
		{
			CClientFileInfo *pClient = pFile->m_Clients[iClient];
			
			if ( pClient->m_ClientID != clientID )
				continue;

			// Ok, this is our man. Decrement the refcount of any chunks this client wanted.
			for ( int iChunk=0; iChunk < pFile->m_Info.m_nChunks; iChunk++ )
			{
				if ( pClient->NeedsChunk( iChunk ) )
				{
					DecrementChunkRefCount( iFile, iChunk );
				}
			}

			delete pClient;
			pFile->m_Clients.Remove( iClient );

			break;
		}
	}

	LeaveCriticalSection( &m_CS );
}


void CMasterMulticastThread::CreateVirtualFile( const char *pFilename, const void *pData, unsigned long fileLength )
{
	int iFile = FindFile( pFilename );
	if ( iFile != -1 )
		Error( "CMasterMulticastThread::CreateVirtualFile( %s ) - file already exists!", pFilename );

	CMulticastFile *pFile = new CMulticastFile;
	pFile->m_UncompressedData.CopyArray( (const char*)pData, fileLength );

	FinishFileSetup( pFile, pFilename );
}


DWORD WINAPI CMasterMulticastThread::StaticMulticastThread( LPVOID pParameter )
{
	return ((CMasterMulticastThread*)pParameter)->MulticastThread();
}


DWORD CMasterMulticastThread::MulticastThread()
{
	double nMicrosecondsPerByte = 1000000.0 / (double)MULTICAST_TRANSMIT_RATE;
	unsigned long waitAccumulator = 0;

	DWORD msToWait = 0;
	while ( WaitForSingleObject( m_hTermEvent, msToWait ) != WAIT_OBJECT_0 )
	{
		EnterCriticalSection( &m_CS );
			
			// If we have nothing to send then kick back for a while.
			if ( m_nTotalActiveChunks == 0 )
			{
				LeaveCriticalSection( &m_CS );
				msToWait = 50;
				continue;
			}

			
			// We're going to time how long this chunk took to send.
			CFastTimer timer;
			timer.Start();

			// Make sure we're on a valid chunk.
			if ( m_iCurFile == -1 )
			{
				Assert( m_Files.Count() > 0 );
				m_iCurFile = m_Files.Head();
				m_iCurActiveChunk = m_Files[m_iCurFile]->m_ActiveChunks.Head();
			}

			int iStartFile = m_iCurFile;
			while ( 1 )
			{
				if ( m_iCurActiveChunk == m_Files[m_iCurFile]->m_ActiveChunks.InvalidIndex() ||
					m_Files[m_iCurFile]->m_ActiveChunks[m_iCurActiveChunk]->m_RefCount == 0 )
				{
					// Finished with that file. Send the next one.
					m_iCurFile = m_Files.Next( m_iCurFile );
					if ( m_iCurFile == m_Files.InvalidIndex() )
						m_iCurFile = m_Files.Head();

					m_iCurActiveChunk = m_Files[m_iCurFile]->m_ActiveChunks.Head();
				}

				if ( m_iCurActiveChunk != m_Files[m_iCurFile]->m_ActiveChunks.InvalidIndex() )
				{
					// Only break if we're on an active chunk.
					if ( m_Files[m_iCurFile]->m_ActiveChunks[m_iCurActiveChunk]->m_RefCount != 0 )
					{
						break;
					}

					m_iCurActiveChunk = m_Files[m_iCurFile]->m_ActiveChunks.Next( m_iCurActiveChunk );
				}
			}

			// Send the next chunk (file, size, time, chunk data).
			CMulticastFile *pFile = m_Files[m_iCurFile];
			
			int iStartByte = m_iCurActiveChunk * MULTICAST_CHUNK_PAYLOAD_SIZE;
			int iEndByte = min( iStartByte + MULTICAST_CHUNK_PAYLOAD_SIZE, pFile->m_Data.Count() );

			WSABUF bufs[4];
			bufs[0].buf = (char*)&pFile->m_Info;
			bufs[0].len = sizeof( pFile->m_Info );

			bufs[1].buf = (char*)&m_iCurActiveChunk;
			bufs[1].len = sizeof( m_iCurActiveChunk );

			bufs[2].buf = (char*)pFile->GetFilename();
			bufs[2].len = strlen( pFile->GetFilename() ) + 1;

			bufs[3].buf = &pFile->m_Data[iStartByte];
			bufs[3].len = iEndByte - iStartByte;

			DWORD nBytesSent = 0;
			int ret = WSASendTo( 
				m_Socket, 
				bufs, 
				ARRAYSIZE( bufs ), 
				&nBytesSent, 
				0, 
				(sockaddr*)&m_MulticastAddr, 
				sizeof( m_MulticastAddr ),
				NULL,
				NULL );			

			g_nMulticastBytesSent += (int)nBytesSent;

			// Move to the next chunk.
			m_iCurActiveChunk = m_Files[m_iCurFile]->m_ActiveChunks.Next( m_iCurActiveChunk );

		LeaveCriticalSection( &m_CS );


		// Measure how long it took to send this.
		timer.End();
		unsigned long timeTaken = timer.GetDuration().GetMicroseconds();

		
		// Measure how long it should have taken.
		unsigned long optimalTimeTaken = (unsigned long)( nMicrosecondsPerByte * nBytesSent );

		
		// If we went faster than we should have, then wait for the difference in time.
		if ( timeTaken < optimalTimeTaken )
		{
			waitAccumulator += optimalTimeTaken - timeTaken;
		}

		// Since we can only wait in milliseconds, it accumulates the wait time in microseconds until 
		// we've got a couple seconds of wait time.
		unsigned long testWait = waitAccumulator / 1000;
		if ( testWait >= MINIMUM_SLEEP_MS )
		{
			msToWait = testWait;
			waitAccumulator -= testWait * 1000;
		}
		else
		{
			msToWait = 0;
		}
	}

	return 0;
}


void CMasterMulticastThread::IncrementChunkRefCount( CMasterMulticastThread::CMulticastFile *pFile, int iChunk )
{
	CChunkInfo *pChunk = &pFile->m_Chunks[iChunk];

	if ( pChunk->m_RefCount == 0 )
	{
		++m_nTotalActiveChunks;
		
		// Move the chunk to the head of the list since it is now active.
		pFile->m_ActiveChunks.Remove( pChunk->m_iActiveChunksIndex );
		pChunk->m_iActiveChunksIndex = pFile->m_ActiveChunks.AddToHead( pChunk );
	}

	pChunk->m_RefCount++;
}


void CMasterMulticastThread::DecrementChunkRefCount( int iFile, int iChunk )
{
	CMulticastFile *pFile = m_Files[iFile];
	CChunkInfo *pChunk = &pFile->m_Chunks[iChunk];

	if ( pChunk->m_RefCount == 0 )
	{
		Error( "CMasterMulticastThread::DecrementChunkRefCount - refcount already zero!\n" );
	}

	pChunk->m_RefCount--;
	if ( pChunk->m_RefCount == 0 )
	{
		--m_nTotalActiveChunks;
		
		// If this is the current chunk the thread is reading on, seek up to the next chunk so
		// the thread doesn't spin off into the next file and skip its current file's contents.
		if ( iFile == m_iCurFile && pChunk->m_iActiveChunksIndex == m_iCurActiveChunk )
		{
			m_iCurActiveChunk = pFile->m_ActiveChunks.Next( pChunk->m_iActiveChunksIndex );
		}

		// Move the chunk to the end of the list since it is now inactive.
		pFile->m_ActiveChunks.Remove( pChunk->m_iActiveChunksIndex );
		pChunk->m_iActiveChunksIndex = pFile->m_ActiveChunks.AddToTail( pChunk );
	}
}


int CMasterMulticastThread::FindFile( const char *pName )
{
	FOR_EACH_LL( m_Files, i )
	{
		if ( stricmp( m_Files[i]->GetFilename(), pName ) == 0 )
			return i;
	}
	return -1;
}


bool CMasterMulticastThread::FindWarningSuppression( const char *pFilename )
{
	FOR_EACH_LL( m_WarningSuppressions, i )
	{
		if ( Q_stricmp( m_WarningSuppressions[i], pFilename ) == 0 )
			return true;
	}
	return false;
}


void CMasterMulticastThread::AddWarningSuppression( const char *pFilename )
{
	char *pBlah = new char[ strlen( pFilename ) + 1 ];
	strcpy( pBlah, pFilename );
	m_WarningSuppressions.AddToTail( pBlah );
}


int CMasterMulticastThread::FindOrAddFile( const char *pFilename )
{
	// See if we've already opened this file.
	int iFile = FindFile( pFilename );
	if ( iFile != -1 )
		return iFile;

	// Read in the file's data.
	FILE *fp = fopen( pFilename, "rb" );
	if ( !fp )
	{
		// It tends to show this once per worker, so only let it show a warning once.
		//if ( !FindWarningSuppression( pFilename ) )
		//{
		//	AddWarningSuppression( pFilename );
		//	Warning( "CMasterMulticastThread::AddFileRequest( %s ): fopen() failed\n", pFilename );
		//}

		return -1;
	}

	CMulticastFile *pFile = new CMulticastFile;

	fseek( fp, 0, SEEK_END );
	pFile->m_UncompressedData.SetSize( ftell( fp ) );
	fseek( fp, 0, SEEK_SET );
	fread( pFile->m_UncompressedData.Base(), 1, pFile->m_UncompressedData.Count(), fp );

	fclose( fp );

	return FinishFileSetup( pFile, pFilename );	
}


int CMasterMulticastThread::FinishFileSetup( CMulticastFile *pFile, const char *pFilename )
{
	// Compress the file's contents.
	if ( !ZLibCompress( pFile->m_UncompressedData.Base(), pFile->m_UncompressedData.Count(), pFile->m_Data ) )
	{
		delete pFile;
		return -1;
	}

	pFile->m_Filename.SetSize( strlen( pFilename ) + 1 );
	strcpy( pFile->m_Filename.Base(), pFilename );
	pFile->m_nCycles = 0;

	pFile->m_Info.m_CompressedSize = pFile->m_Data.Count();
	pFile->m_Info.m_UncompressedSize = pFile->m_UncompressedData.Count();

	pFile->m_Info.m_nChunks = PAD_NUMBER( pFile->m_Info.m_CompressedSize, MULTICAST_CHUNK_PAYLOAD_SIZE ) / MULTICAST_CHUNK_PAYLOAD_SIZE;

	// Initialize the chunks.
	pFile->m_Chunks.SetSize( pFile->m_Info.m_nChunks );
	for ( int i=0; i < pFile->m_Chunks.Count(); i++ )
	{
		CChunkInfo *pChunk = &pFile->m_Chunks[i];

		pChunk->m_iChunk = (unsigned short)i;
		pChunk->m_RefCount = 0;
		pChunk->m_iActiveChunksIndex = pFile->m_ActiveChunks.AddToTail( pChunk );
	}

	// Get this file in the queue.
	EnterCriticalSection( &m_CS );
		pFile->m_Info.m_FileID = m_Files.AddToTail( pFile );
	LeaveCriticalSection( &m_CS );

	return pFile->m_Info.m_FileID;
}


const CUtlVector<char>& CMasterMulticastThread::GetFileData( int iFile ) const
{
	return m_Files[iFile]->m_UncompressedData;
}



// -------------------------------------------------------------------------------------------------------------- //
// The VMPI file system. The master runs a thread that multicasts the contents of all the 
// files being used in the session. The workers read whatever files they need out of the stream.
// -------------------------------------------------------------------------------------------------------------- //

class CWorkerFile
{
public:
	const char* GetFilename() { return m_Filename.Base(); }
	bool IsReadyToRead() const { return m_nChunksToReceive == 0; }


public:
	CFastTimer m_Timer; // To see how long it takes to download the file.

	// This is false until we get any packets about the file. In the packets,
	// we find out what the size is supposed to be.
	bool m_bGotCompressedSize;

	// The ID the master uses to refer to this file.
	int m_FileID;

	CUtlVector<char> m_Filename;

	// First data comes in here, then when it's all there, it is inflated into m_UncompressedData.
	CUtlVector<char> m_CompressedData;
	
	// 1 bit for each chunk.
	CUtlVector<unsigned char> m_ChunksReceived;

	// When this is zero, the file is done being received and m_UncompressedData is valid.
	int m_nChunksToReceive;
	CUtlVector<char> m_UncompressedData;
};


// -------------------------------------------------------------------------------------------------------------- //
// The VMPI file system deals with in-memory and on-disk files.
// -------------------------------------------------------------------------------------------------------------- //

class IVMPIFile
{
public:
	virtual void Close() = 0;
	virtual void Seek( int pos, FileSystemSeek_t seekType ) = 0;
	virtual unsigned int Tell() = 0;
	virtual unsigned int Size() = 0;
	virtual void Flush() = 0;
	virtual int Read( void* pOutput, int size ) = 0;
	virtual int Write( void const* pInput, int size ) = 0;
};

class CVMPIFile_Stdio : public IVMPIFile
{
public:
	void Init( FILE *fp )
	{
		fseek( fp, 0, SEEK_END );
		m_Len = ftell( fp );
		fseek( fp, 0, SEEK_SET );
		m_fp = fp;
	}

	virtual void Close()
	{
		fclose( m_fp );
		delete this;
	}

	virtual void Seek( int pos, FileSystemSeek_t seekType )
	{
		if ( seekType == FILESYSTEM_SEEK_HEAD )
			fseek( m_fp, pos, SEEK_SET );
		else if ( seekType == FILESYSTEM_SEEK_CURRENT )
			fseek( m_fp, pos, SEEK_CUR );
		else
			fseek( m_fp, pos, SEEK_END );
	}

	virtual unsigned int Tell() { return ftell( m_fp ); }
	virtual unsigned int Size() { return m_Len; }
	virtual void Flush() { fflush( m_fp ); }
	virtual int Read( void* pOutput, int size ) { return fread( pOutput, 1, size, m_fp ); }
	virtual int Write( void const* pInput, int size ) { return fwrite( pInput, 1, size, m_fp ); }

	FILE *m_fp;
	long m_Len;
};

class CVMPIFile_Memory : public IVMPIFile
{
public:
	void Init( const char *pData, long len )
	{
		m_pData = pData;
		m_DataLen = len;
		m_iCurPos = 0;
	}

	virtual void Close()
	{
		delete this;
	}

	virtual void Seek( int pos, FileSystemSeek_t seekType )
	{
		if ( seekType == FILESYSTEM_SEEK_HEAD )
			m_iCurPos = pos;
		else if ( seekType == FILESYSTEM_SEEK_CURRENT )
			m_iCurPos += pos;
		else
			m_iCurPos = m_DataLen - pos;
	}

	virtual unsigned int Tell() { return m_iCurPos; }
	virtual unsigned int Size() { return m_DataLen; }
	virtual void Flush() {  }
	virtual int Read( void* pOutput, int size ) 
	{ 
		Assert( m_iCurPos >= 0 );
		int nToRead = min( m_DataLen - m_iCurPos, size );
		memcpy( pOutput, &m_pData[m_iCurPos], nToRead );
		m_iCurPos += nToRead;
		return nToRead;
	}

	virtual int Write( void const* pInput, int size ) { Assert( false ); return 0; }

	const char *m_pData;
	long m_DataLen;
	int m_iCurPos;
};


class CWorkerMulticastListener
{
public:
	CWorkerMulticastListener()
	{	   
		m_nUnfinishedFiles = 0;
	}
	
	~CWorkerMulticastListener()
	{
		Term();
	}

	bool Init( const CIPAddr &mcAddr )
	{
		m_MulticastAddr = mcAddr;
		return true;
	}

	void Term()
	{
		m_WorkerFiles.PurgeAndDeleteElements();
	}

	
	CWorkerFile* RequestFileFromServer( const char *pFilename )
	{
		Assert( FindWorkerFile( pFilename ) == NULL );

		// Send a request to the master to find out if this file even exists.
		unsigned char packetID[2] = { VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_REQUEST };
		void *pChunks[2] = { packetID, (void*)pFilename };
		int chunkLengths[2]  = { sizeof( packetID ), strlen( pFilename ) + 1 };
		VMPI_SendChunks( pChunks, chunkLengths, ARRAYSIZE( pChunks ), 0 );

		// Wait for the file ID to come back.
		MessageBuffer mb;
		int iSource;
		VMPI_DispatchUntil( &mb, &iSource, VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_RESPONSE, true );
	
		// If we get -1 back, it means the file doesn't exist.
		int fileID = *((int*)&mb.data[2]);
		if ( fileID == -1 )
			return NULL;

		CWorkerFile *pTestFile = new CWorkerFile;
		pTestFile->m_Filename.SetSize( strlen( pFilename ) + 1 );
		strcpy( pTestFile->m_Filename.Base(), pFilename );
		pTestFile->m_FileID = fileID;
		pTestFile->m_bGotCompressedSize = false;
		pTestFile->m_nChunksToReceive = 9999;
		pTestFile->m_Timer.Start();
		m_WorkerFiles.AddToTail( pTestFile );

		++m_nUnfinishedFiles;

		return pTestFile;
	}

	void FlushAckChunks( unsigned short chunksToAck[NUM_BUFFERED_CHUNK_ACKS][2], int &nChunksToAck, DWORD &lastAckTime )
	{
		if ( nChunksToAck )
		{
			// Tell the master we received this chunk.
			unsigned char packetID[2] = { VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_CHUNK_RECEIVED };
			void *pChunks[2] = { packetID, chunksToAck };
			int chunkLengths[2] = { sizeof( packetID ), nChunksToAck * 4 };
			VMPI_SendChunks( pChunks, chunkLengths, 2, 0 );
			nChunksToAck = 0;
		}

		lastAckTime = GetTickCount();
	}

	void MaybeFlushAckChunks( unsigned short chunksToAck[NUM_BUFFERED_CHUNK_ACKS][2], int &nChunksToAck, DWORD &lastAckTime )
	{
		if ( nChunksToAck && GetTickCount() - lastAckTime > ACK_FLUSH_INTERVAL )
			FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
	}

	void AddAckChunk( 
		unsigned short chunksToAck[NUM_BUFFERED_CHUNK_ACKS][2], 
		int &nChunksToAck,
		DWORD &lastAckTime,
		int fileID,
		int iChunk )
	{
		chunksToAck[nChunksToAck][0] = (unsigned short)fileID;
		chunksToAck[nChunksToAck][1] = (unsigned short)iChunk;

		++nChunksToAck;
		if ( nChunksToAck == NUM_BUFFERED_CHUNK_ACKS )
		{
			FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
		}
	}


	// This is the main function the workers use to pick files out of the multicast stream.
	// The app is waiting for a specific file, but we receive and ack any files we can until 
	// we get the file they're looking for, then we return.
	//
	// NOTE: ideally, this would be in a thread, but it adds lots of complications and may
	// not be worth it.
	CWorkerFile* ListenFor( const char *pFilename )
	{
		CWorkerFile *pFile = FindWorkerFile( pFilename );
		if ( !pFile )
		{
			// Ok, we haven't requested this file yet. Create an entry for it and
			// tell the master we'd like this file.
			pFile = RequestFileFromServer( pFilename );
			if ( !pFile )
				return NULL;
		}

		
		// Now start listening to the multicast stream.
		ISocket *pSocket = CreateMulticastListenSocket( m_MulticastAddr );
		if ( !pSocket )
		{
			char str[512];
			IP_GetLastErrorString( str, sizeof( str ) );
			Warning( "CreateMulticastListenSocket (%d.%d.%d.%d:%d) failed\n%s\n", EXPAND_ADDR( m_MulticastAddr ), str );
			return NULL;
		}
		
		unsigned short chunksToAck[NUM_BUFFERED_CHUNK_ACKS][2];
		int nChunksToAck = 0;
		DWORD lastAckTime = GetTickCount();

		// Now just receive multicast data until this file has been received.
		while ( m_nUnfinishedFiles > 0 )
		{
			CIPAddr ipFrom;
			char data[MULTICAST_CHUNK_PAYLOAD_SIZE*2];
			int len = pSocket->RecvFrom( data, sizeof( data ), &ipFrom );
			if ( len == -1 )
			{
				// Sleep for 10 seconds and also handle socket errors.
				VMPI_HandleSocketErrors( 10 );
				continue;
			}

			g_nMulticastBytesReceived += len;

			// Alrighty. Figure out what the deal is with this file.
			CMulticastFileInfo *pInfo = (CMulticastFileInfo*)data;
			int *piChunk = (int*)( pInfo + 1 );
			const char *pTestFilename = (const char*)( piChunk + 1 );
			const char *pPayload = pTestFilename + strlen( pFilename ) + 1;
			int payloadLen = len - ( pPayload - data );
			if ( payloadLen < 0 )
			{
				Warning( "CWorkerMulticastListener::ListenFor: invalid packet received on multicast group\n" );
				continue;
			}


			if ( pInfo->m_FileID != pFile->m_FileID )
				continue;

			CWorkerFile *pTestFile = FindWorkerFile( pInfo->m_FileID );
			if ( !pTestFile )
				Error( "FindWorkerFile( %s ) failed\n", pTestFilename );

			// TODO: reenable this code and disable the if right above here.
			// We always get "invalid payload length" errors on the workers when using this, but
			// I haven't been able to figure out why yet.
			/*
			// Put the data into whatever file it belongs in.
			if ( !pTestFile )
			{
				pTestFile = RequestFileFromServer( pTestFilename );
				if ( !pTestFile )
					continue;
			}
			*/

			// Is this the first packet about this file?
			if ( !pTestFile->m_bGotCompressedSize )
			{
				pTestFile->m_bGotCompressedSize = true;
				pTestFile->m_CompressedData.SetSize( pInfo->m_CompressedSize );
				pTestFile->m_UncompressedData.SetSize( pInfo->m_UncompressedSize );
				pTestFile->m_ChunksReceived.SetSize( PAD_NUMBER( pInfo->m_nChunks, 8 ) / 8 );
				pTestFile->m_nChunksToReceive = pInfo->m_nChunks;
				memset( pTestFile->m_ChunksReceived.Base(), 0, pTestFile->m_ChunksReceived.Count() );
			}

			// Validate the chunk index and uncompressed size.
			int iChunk = *piChunk;
			if ( iChunk < 0 || iChunk >= pInfo->m_nChunks )
			{
				Error( "ListenFor(): invalid chunk index (%d) for file '%s'\n", iChunk, pTestFilename );
			}

			// Only handle this if we didn't already received the chunk.
			if ( !(pTestFile->m_ChunksReceived[iChunk >> 3] & (1 << (iChunk & 7))) )
			{
				// Make sure the file is properly setup to receive the data into.
				if ( (int)pInfo->m_UncompressedSize != pTestFile->m_UncompressedData.Count() ||
					(int)pInfo->m_CompressedSize != pTestFile->m_CompressedData.Count() )
				{
					Error( "ListenFor(): invalid compressed or uncompressed size.\n"
						"pInfo = '%s', pTestFile = '%s'\n"
						"Compressed   (pInfo = %d, pTestFile = %d)\n"
						"Uncompressed (pInfo = %d, pTestFile = %d)\n", 
						pTestFilename,
						pTestFile->GetFilename(),
						pInfo->m_CompressedSize,
						pTestFile->m_CompressedData.Count(),
						pInfo->m_UncompressedSize,
						pTestFile->m_UncompressedData.Count()
						);
				}
				
				int iChunkStart = iChunk * MULTICAST_CHUNK_PAYLOAD_SIZE;
				int iChunkEnd = min( iChunkStart + MULTICAST_CHUNK_PAYLOAD_SIZE, pTestFile->m_CompressedData.Count() );
				int chunkLen = iChunkEnd - iChunkStart;

				if ( chunkLen != payloadLen )
				{
					Error( "ListenFor(): invalid payload length for '%s' (%d should be %d)\n"
						"pInfo = '%s', pTestFile = '%s'\n"
						"Chunk %d out of %d. Compressed size: %d\n", 
						pTestFile->GetFilename(),
						payloadLen, 
						chunkLen,
						pTestFilename,
						pTestFile->GetFilename(),
						iChunk,
						pInfo->m_nChunks,
						pInfo->m_CompressedSize
						);
				}

				memcpy( &pTestFile->m_CompressedData[iChunkStart], pPayload, chunkLen );
				pTestFile->m_ChunksReceived[iChunk >> 3] |= (1 << (iChunk & 7));

				--pTestFile->m_nChunksToReceive;

				// Remember to ack what we received.
				AddAckChunk( chunksToAck, nChunksToAck, lastAckTime, pInfo->m_FileID, iChunk );
				
				// If we're done receiving the data, unpack it.
				if ( pTestFile->m_nChunksToReceive == 0 )
				{
					// Ack the file.
					FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );

					pTestFile->m_Timer.End();

					CFastTimer decompressTimer;
					decompressTimer.Start();

						pTestFile->m_UncompressedData.SetSize( pInfo->m_UncompressedSize );
						--m_nUnfinishedFiles;

						if ( !ZLibDecompress( 
							pTestFile->m_CompressedData.Base(), 
							pTestFile->m_CompressedData.Count(),
							pTestFile->m_UncompressedData.Base(),
							pTestFile->m_UncompressedData.Count() ) )
						{
							pSocket->Release();
							FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
							Error( "ZLibDecompress failed.\n" );
							return NULL;
						}

					decompressTimer.End();
					
					Warning( "Got '%s' (%dk) in %.2f seconds (decompress time: %.2f seconds)\n", 
						pTestFile->GetFilename(), 
						(pTestFile->m_UncompressedData.Count() + 511) / 1024,
						pTestFile->m_Timer.GetDuration().GetSeconds(),
						decompressTimer.GetDuration().GetSeconds()
						);

					// Won't be needing this anymore.
					pTestFile->m_CompressedData.Purge();
				}
			}

			MaybeFlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
		}

		Assert( pFile->IsReadyToRead() );
		FlushAckChunks( chunksToAck, nChunksToAck, lastAckTime );
		pSocket->Release();

		return pFile;
	}

	CWorkerFile* FindWorkerFile( const char *pFilename ) 
	{
		FOR_EACH_LL( m_WorkerFiles, i )
		{
			if ( stricmp( m_WorkerFiles[i]->GetFilename(), pFilename ) == 0 )
				return m_WorkerFiles[i];
		}
		return NULL;
	}

	CWorkerFile* FindWorkerFile( int fileID ) 
	{
		FOR_EACH_LL( m_WorkerFiles, i )
		{
			if ( m_WorkerFiles[i]->m_FileID == fileID )
				return m_WorkerFiles[i];
		}
		return NULL;
	}


private:
	CIPAddr m_MulticastAddr;
	CUtlLinkedList<CWorkerFile*, int> m_WorkerFiles;

	// How many files do we have open that we haven't finished receiving from the server yet?
	// We always keep waiting for data until this is zero.
	int m_nUnfinishedFiles;
};


class CVMPIFileSystem : public IBaseFileSystem
{
public:
	CMasterMulticastThread m_MasterThread;
	CWorkerMulticastListener m_Listener;

	CIPAddr m_MulticastIP;



	
	~CVMPIFileSystem()
	{
	}


	static void OnClientDisconnect( int procID, const char *pReason )
	{
		extern CVMPIFileSystem g_VMPIFileSystem;
		g_VMPIFileSystem.m_MasterThread.OnClientDisconnect( procID );
	}
	

	bool Init()
	{
		// Pick a random IP in the multicast range (224.0.0.2 to 239.255.255.255);
		if ( g_bUseMPI )
		{
			if ( g_bMPIMaster )
			{
				CCycleCount cnt;
				cnt.Sample();
				RandomSeed( (int)cnt.GetMicroseconds() );

				int localPort = 23412; // This can be anything.

				m_MulticastIP.port = RandomInt( 22000, 25000 ); // Pulled out of something else.
				m_MulticastIP.ip[0] = (unsigned char)RandomInt( 225, 238 );
				m_MulticastIP.ip[1] = (unsigned char)RandomInt( 0, 255 );
				m_MulticastIP.ip[2] = (unsigned char)RandomInt( 0, 255 );
				m_MulticastIP.ip[3] = (unsigned char)RandomInt( 3, 255 );

				if ( !m_MasterThread.Init( localPort, &m_MulticastIP ) )
					return false;

				// Send out the multicast addr to all the clients.
				SendMulticastIP( &m_MulticastIP );

				// Make sure we're notified when a client disconnects so we can unlink them from the 
				// multicast thread's structures.
				VMPI_AddDisconnectHandler( &CVMPIFileSystem::OnClientDisconnect );
				return true;
			}
			else
			{
				// Get the multicast addr to listen on.
				CIPAddr mcAddr;
				RecvMulticastIP( &mcAddr );

				return m_Listener.Init( mcAddr );
			}
		}
		else
		{
			return true;
		}
	}

	void Term()
	{
		m_MasterThread.Term();
		m_Listener.Term();
	}


	FileHandle_t Open_Master( const char *pFilename, const char *pOptions )
	{
		// Use a stdio file if they want to write to it.
		bool bWriteAccess = (Q_stristr( pOptions, "w" ) != 0);
		if ( bWriteAccess )
		{
			FILE *fp = fopen( pFilename, pOptions );
			if ( !fp )
				return FILESYSTEM_INVALID_HANDLE;

			CVMPIFile_Stdio *pFile = new CVMPIFile_Stdio;
			pFile->Init( fp );
			return (FileHandle_t)pFile;
		}

		// Have our multicast thread load all the data so it's there when workers want it.
		int iFile = m_MasterThread.FindOrAddFile( pFilename );
		if ( iFile == -1 )
			return FILESYSTEM_INVALID_HANDLE;

		const CUtlVector<char> &data = m_MasterThread.GetFileData( iFile );

		CVMPIFile_Memory *pFile = new CVMPIFile_Memory;
		pFile->Init( data.Base(), data.Count() );
		return (FileHandle_t)pFile;
	}


	FileHandle_t Open_Worker( const char *pFilename, const char *pOptions )
	{
		// Workers can't open anything for write access.
		bool bWriteAccess = (Q_stristr( pOptions, "w" ) != 0);
		if ( bWriteAccess )
			return FILESYSTEM_INVALID_HANDLE;

		// Do we have this file's data already?
		CWorkerFile *pFile = m_Listener.FindWorkerFile( pFilename );
		if ( !pFile || !pFile->IsReadyToRead() )
		{
			// Ok, start listening to the multicast stream until we get the file we want.
			
			// NOTE: it might make sense here to have the client ask for a list of ALL the files that
			// the master currently has and wait to receive all of them (so we don't come back a bunch
			// of times and listen 

			// NOTE NOTE: really, the best way to do this is to have a thread on the workers that sits there
			// and listens to the multicast stream. Any time the master opens a new file up, it assumes
			// all the workers need the file, and it starts to send it on the multicast stream until
			// the worker threads respond that they all have it.
			//
			// (NOTE: this probably means that the clients would have to ack the chunks on a UDP socket that
			// the thread owns).
			//
			// This would simplify all the worries about a client missing half the stream and having to 
			// wait for another cycle through it.
			pFile = m_Listener.ListenFor( pFilename );

			if ( !pFile )
			{
				return FILESYSTEM_INVALID_HANDLE;
			}
		}

		// Ok! Got the file. now setup a memory stream they can read out of it with.
		CVMPIFile_Memory *pOut = new CVMPIFile_Memory;
		pOut->Init( pFile->m_UncompressedData.Base(), pFile->m_UncompressedData.Count() );
		return (FileHandle_t)pOut;
	}


	virtual FileHandle_t	Open( const char *pFilename, const char *pOptions, const char *pathID = 0 )
	{
		// Warning( "(%d) Open( %s, %s )\n", (int)Plat_FloatTime(), pFilename, pOptions );

		if ( g_bDisableFileAccess )
			Error( "Open( %s, %s ) - file access has been disabled.", pFilename, pOptions );

		// If not using VMPI, then use regular file IO.
		if ( !g_bUseMPI )
		{
			FILE *fp = fopen( pFilename, pOptions );
			if ( !fp )
				return FILESYSTEM_INVALID_HANDLE;

			CVMPIFile_Stdio *pFile = new CVMPIFile_Stdio;
			pFile->Init( fp );
			return (FileHandle_t)pFile;
		}

		
		// If MPI is on and we're a worker and we're opening a file for reading, then we use the cache.
		if ( g_bMPIMaster )
		{
			return Open_Master( pFilename, pOptions );
		}
		else
		{ 
			return Open_Worker( pFilename, pOptions );
		}
	}

	virtual void			Close( FileHandle_t file )
	{
		if ( file )
			((IVMPIFile*)file)->Close();
	}

	virtual int				Read( void* pOutput, int size, FileHandle_t file )
	{
		return ((IVMPIFile*)file)->Read( pOutput, size );
	}

	virtual int				Write( void const* pInput, int size, FileHandle_t file )
	{
		return ((IVMPIFile*)file)->Write( pInput, size );
	}

	virtual void			Seek( FileHandle_t file, int pos, FileSystemSeek_t seekType )
	{
		((IVMPIFile*)file)->Seek( pos, seekType );
	}

	virtual unsigned int	Tell( FileHandle_t file )
	{
		return ((IVMPIFile*)file)->Tell();
	}

	virtual unsigned int	Size( FileHandle_t file )
	{
		return ((IVMPIFile*)file)->Size();
	}

	virtual unsigned int	Size( const char *pFilename, const char *pathID = 0 )
	{
		FileHandle_t hFile = Open( pFilename, "rb" );
		if ( hFile == FILESYSTEM_INVALID_HANDLE )
		{
			return 0;
		}
		else
		{
			unsigned int ret = Size( hFile );
			Close( hFile );
			return ret;
		}
	}

	virtual long			GetFileTime( const char *pFileName, const char *pathID = 0 )
	{
		struct	_stat buf;
		int sr = _stat( pFileName, &buf );
		if ( sr == -1 )
		{
			return 0;
		}

		return buf.st_mtime;
	}

	virtual void			Flush( FileHandle_t file )
	{
		((IVMPIFile*)file)->Flush();
	}	

	virtual bool			Precache( const char* pFileName, const char *pPathID )
	{
		return false;
	} 

	virtual bool			FileExists( const char *pFileName, const char *pPathID )
	{
		FileHandle_t hFile = Open( pFileName, "rb" );
		if ( hFile )
		{
			Close( hFile );
			return true;
		}
		else
		{
			return false;
		}
	}
	virtual bool			IsFileWritable( const char *pFileName, const char *pPathID )
	{
		struct	_stat buf;
		int sr = _stat( pFileName, &buf );
		if ( sr == -1 )
		{
			return false;
		}

		if( buf.st_mode & _S_IWRITE )
		{
			return true;
		}

		return false;
	}
};

CVMPIFileSystem g_VMPIFileSystem;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CVMPIFileSystem, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION, g_VMPIFileSystem );


IBaseFileSystem* VMPI_FileSystem_Init( IVMPIFileSystemHelpers *pHelpers )
{
	g_pHelpers = pHelpers;
	if ( !g_VMPIFileSystem.Init() )
	{
		Error( "Error in CVMPIFileSystem::Init()\n" );
	}

	return &g_VMPIFileSystem;
}


void VMPI_FileSystem_Term()
{
	if ( g_bUseMPI )
	{
		g_VMPIFileSystem.Term();

		if ( g_bMPIMaster )
			Msg( "Multicast send: %dk\n", (g_nMulticastBytesSent + 511) / 1024 );
		else
			Msg( "Multicast recv: %dk\n", (g_nMulticastBytesReceived + 511) / 1024 );
	}
}


void VMPI_FileSystem_DisableFileAccess()
{
	g_bDisableFileAccess = true;
}


CreateInterfaceFn VMPI_FileSystem_GetFactory()
{
	return Sys_GetFactoryThis();
}


void VMPI_FileSystem_CreateVirtualFile( const char *pFilename, const void *pData, unsigned long fileLength )
{
	g_VMPIFileSystem.m_MasterThread.CreateVirtualFile( pFilename, pData, fileLength );
}


// Register our packet ID.
bool FileSystemRecv( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	// Handle this packet.
	int subPacketID = pBuf->data[1];
	switch( subPacketID )
	{
		case VMPI_FSPACKETID_FILE_REQUEST:
		{
			const char *pFilename = (const char*)&pBuf->data[2];
			
			if ( g_iVMPIVerboseLevel >= 2 )
				Msg( "Client %d requested '%s'\n", iSource, pFilename );

			int fileID = g_VMPIFileSystem.m_MasterThread.AddFileRequest( pFilename, iSource );
			
			// Send back the file ID.
			unsigned char cPacket[2] = { VMPI_PACKETID_FILESYSTEM, VMPI_FSPACKETID_FILE_RESPONSE };
			void *pChunks[2] = { cPacket, &fileID };
			int chunkLen[2] = { sizeof( cPacket ), sizeof( fileID ) };

			VMPI_SendChunks( pChunks, chunkLen, ARRAYSIZE( pChunks ), iSource );
			return true;
		}
		break;

		case VMPI_FSPACKETID_CHUNK_RECEIVED:
		{
			unsigned short *pFileID = (unsigned short*)&pBuf->data[2];
			unsigned short *pChunkID = pFileID+1;
			
			int nChunks = (pBuf->getLen() - 2) / 4;
			for ( int i=0; i < nChunks; i++ )
			{
				g_VMPIFileSystem.m_MasterThread.OnChunkReceived( *pFileID, iSource, *pChunkID );
				pFileID += 2;
				pChunkID += 2;
			}
			return true;
		}
		break;

		case VMPI_FSPACKETID_FILE_RECEIVED:
		{
			unsigned short *pFileID = (unsigned short*)&pBuf->data[2];
			g_VMPIFileSystem.m_MasterThread.OnFileReceived( *pFileID, iSource );
			return true;
		}
		
		default:
		{
			return false;
		}
	}
}


CDispatchReg g_DispatchReg_FileSystem( VMPI_PACKETID_FILESYSTEM, FileSystemRecv );
