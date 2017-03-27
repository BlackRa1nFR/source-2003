//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

// set the max sockets usable to 1024 (normally 64)
// this is for the TrackerTest app
#define FD_SETSIZE 1024

#include "Sockets.h"
#include "random.h"
#include "winlite.h"

#include <winsock.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>

#undef SetPort

// enable this define if you want all packets to be logged
//#define LOG_PACKETS_TO_DISK

void Socket_OutputDebugString(const char *string);
void Socket_WritePacketToDisk(void const *pBuf, int bufSize);

#pragma warning(disable: 4127) // warning C4127: conditional expression is constant

//-----------------------------------------------------------------------------
// Purpose: WinSock implementation of the ISockets interface
//-----------------------------------------------------------------------------
class CSockets : public ISockets
{
public:
	CSockets();
	~CSockets();

	bool InitializeSockets( unsigned short portMin, unsigned short portMax );
	bool StringToAddress( const char *s, int *ip, unsigned short *port );
	bool SendData( CNetAddress netAddress, void *data, int dataSize );
	bool BroadcastData(int port, void *data, int dataSize);
	bool IsDataAvailable( void );
	int ReadData( void *buffer, int bufferSize, CNetAddress *source );
	CNetAddress GetLocalAddress();
	
	unsigned short GetListenPort( void ) { return m_iListenPort; }

	void deleteThis()
	{
		delete this;
	}

private:
	SOCKET CreateUDPSocket( void );
	int    BindSocket( SOCKET socket );
	
	bool m_bInitialized;
	SOCKET m_ReceiveSocket;
	unsigned short m_iListenPort;
};

EXPOSE_INTERFACE(CSockets, ISockets, SOCKETS_INTERFACE_VERSION);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSockets::CSockets() : m_bInitialized(false), m_iListenPort(0)
{
	// the main initialization happens in InitializeSockets()
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSockets::~CSockets()
{
	// completely shut down the sockets
	// i couldn't find the #define for the Shutdown parameters
	::shutdown(m_ReceiveSocket, 0x01);
	::shutdown(m_ReceiveSocket, 0x02);
	::closesocket(m_ReceiveSocket);
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the sockets for use, creating 2 sockets, send and receive
//			Receive socket will listen on one of the ports in the range portMin->portMax
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSockets::InitializeSockets( unsigned short portMin, unsigned short portMax )
{
	if ( m_bInitialized )
		return true;

	// create the send/receiving socket
	m_ReceiveSocket = CreateUDPSocket();

	// make sure the created sockets are valid
	if (m_ReceiveSocket == INVALID_SOCKET)
	{
		return false;
	}

	// bind the receiving socket listen on our port range
	sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;

	// bind ourselves to the first available port
	bool bFirst = false;
	for ( unsigned short port = (unsigned short)portMin; port <= portMax; port++ )
	{
		if (bFirst)
		{
			port = (unsigned short)RandomLong(portMin, portMax);
		}

		socketAddress.sin_port = ::htons(port);
		int result = ::bind( m_ReceiveSocket, (sockaddr *)&socketAddress, sizeof(socketAddress) );

		BOOL val = true;
		result = ::setsockopt( m_ReceiveSocket, SOL_SOCKET, SO_BROADCAST, (const char *)&val, sizeof(val));

		if ( !result )
		{
			// success!
			m_iListenPort = port;
			break; 
		}

		if (bFirst)
		{
			port = (unsigned short)(portMin - 1);
			bFirst = false;
		}
	}

	if ( port > portMax )
	{
		// couldn't bind a port
		return false;
	}

	// get our max packet size
	unsigned int maxMsgSize = 0;
	int maxMsgSizeSize = sizeof(maxMsgSize);
	::getsockopt( m_ReceiveSocket, SOL_SOCKET, SO_SNDBUF, (char*)&maxMsgSize, &maxMsgSizeSize );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the current machines IP address
//-----------------------------------------------------------------------------
CNetAddress CSockets::GetLocalAddress()
{
	sockaddr_in addr;
	int size = sizeof(addr);
	if (getsockname(m_ReceiveSocket, (sockaddr *)&addr, &size))
	{
		// error getting sockname, just null out
		memset(&addr, 0, sizeof(addr));
	}

	CNetAddress localAddress;
	localAddress.SetIP(addr.sin_addr.s_addr);
	localAddress.SetPort(m_iListenPort);

	return localAddress;	
}

//-----------------------------------------------------------------------------
// Purpose: Creates a new non-blocking UDP socket
// Output : SOCKET - the newly created socket handle
//-----------------------------------------------------------------------------
SOCKET CSockets::CreateUDPSocket( void )
{
	// create the base socket
	SOCKET socket = ::socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( socket == INVALID_SOCKET )
	{
		return INVALID_SOCKET;
	}

	// make it non-blocking
	unsigned long valueTrue = true;
	int result = ::ioctlsocket( socket, FIONBIO, &valueTrue );
	if ( result == SOCKET_ERROR )
	{
		return INVALID_SOCKET;
	}

	return socket;
}


//-----------------------------------------------------------------------------
// Purpose: Writes to the network stream
// Input  : netAddress - destination address
//			*data - pointer to data block
//			dataSize - total sizeof data
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSockets::SendData( CNetAddress netAddress, void *data, int dataSize )
{
	/*
	static int dumped = 0, sent = 0;
	//!! debug: randomly throw away packets
	float chance = (float)RandomFloat(0.0f, 1.0f);
	if (chance < 0.2f)	// 20% packet loss
	{
		dumped++;
		return true;
	}
	sent++;
	*/
#ifdef LOG_PACKETS_TO_DISK
	Socket_WritePacketToDisk(data, dataSize);
#endif // LOG_PACKETS_TO_DISK

	// construct the send to address
	sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = netAddress.IP();
	socketAddress.sin_port = ::htons( netAddress.Port() );

	// write the data to the stream
	int result = ::sendto( m_ReceiveSocket, (char *)data, dataSize, 0, (sockaddr*)&socketAddress, sizeof(socketAddress) );

	// make sure all the data was written
	if ( result != dataSize )
	{
#ifdef _DEBUG
		int error = ::WSAGetLastError();
		assert(error != 0);
#endif
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Broadcasts the data across the network
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSockets::BroadcastData(int port, void *data, int dataSize)
{
	// construct the send to address
	sockaddr_in socketAddress;
	memset(&socketAddress, 0, sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_BROADCAST;
	socketAddress.sin_port = ::htons((unsigned short)port);

	// write the data to the stream
	int result = ::sendto( m_ReceiveSocket, (char *)data, dataSize, 0, (sockaddr*)&socketAddress, sizeof(socketAddress) );

	// make sure all the data was written
	if ( result != dataSize )
	{
		//!! debug on the error
		assert(::WSAGetLastError() != 0);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: reads data from the stream; returns the number of bytes read.
// Input  : *buffer - 
//			bufferSize - 
//			*source - [out] address of sender... optional and may be NULL
// Output : int - number of bytes read
//-----------------------------------------------------------------------------
int CSockets::ReadData( void *buffer, int bufferSize, CNetAddress *sourceAddr )
{
	sockaddr_in fromAddr;
	int fromLen = sizeof(fromAddr);

	// read the data out of the network into the buffer
	int bytesRead = ::recvfrom( m_ReceiveSocket, (char *)buffer, bufferSize, 0, (SOCKADDR*)&fromAddr, &fromLen );
	if ( bytesRead == SOCKET_ERROR )
	{
		assert(::WSAGetLastError() != 0);
		return 0;
	}

#ifdef LOG_PACKETS_TO_DISK
	Socket_WritePacketToDisk(buffer, bytesRead);
#endif // LOG_PACKETS_TO_DISK

	// fillout the source address
	if ( sourceAddr )
	{
		sourceAddr->SetIP( fromAddr.sin_addr.S_un.S_addr );
		sourceAddr->SetPort( ::ntohs(fromAddr.sin_port) );
	}

	return bytesRead;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if there is data available in the network
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSockets::IsDataAvailable( void )
{
	fd_set socketSet;
	timeval timeout = { 0, 0 };
	FD_ZERO( &socketSet );
	FD_SET( m_ReceiveSocket, &socketSet );

	if ( ::select(0, &socketSet, NULL, NULL, &timeout) == 1 )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Extracts the IP address and port number from a string IP
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSockets::StringToAddress( const char *s, int *ip, unsigned short *port )
{
	char copy[128];
	*ip = 0;
	*port = 0;
	strcpy( copy, s );

	// strip off a trailing :port if present
	for ( char *colon = copy; *colon; colon++ )
	{
		if (*colon == ':')
		{
			*colon = 0;
			*port = (short)atoi(colon+1);
		}
	}
	
	if ( copy[0] >= '0' && copy[0] <= '9' && strstr(copy, ".") )
	{
		*ip = ::inet_addr( copy );
	}
	else
	{
		hostent	*h = ::gethostbyname(copy);
		if (!h)
			return false;

		*ip = *(int *)h->h_addr_list[0];
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper for debugging
// Input  : *string - 
//-----------------------------------------------------------------------------
void Socket_OutputDebugString(const char *string)
{
	::OutputDebugString(string);
}

//-----------------------------------------------------------------------------
// Purpose: For debugging, logs a packet out to disk
//-----------------------------------------------------------------------------
void Socket_WritePacketToDisk(void const *pBuf, int bufSize)
{
	static int packetID = 0;

	char filename[256];
	sprintf(filename, "packet%d.bin", packetID);
	FILE *f = fopen(filename, "wb");
	if (!f)
		return;

	fwrite(pBuf, bufSize, 1, f);
	fclose(f);

	packetID++;
}


