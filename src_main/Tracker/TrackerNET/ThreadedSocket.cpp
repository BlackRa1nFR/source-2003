//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "BinaryBuffer.h"
#include "Sockets.h"
#include "PacketHeader.h"
#include "Threads.h"
#include "ThreadedSocket.h"
#include "IceKey.h"
#include "CompletionEvent.h"
#include "UtlLinkedList.h"

extern void Socket_OutputDebugString(const char *outputString);
extern void EncryptPacketToBuffer(unsigned char *bufData, unsigned char *output, int bufferSize);
extern void EncryptPacket(unsigned char *buffer, int bufferSize);
extern void DecryptPacket(unsigned char *buffer, int bufferSize);

#define UDP_HEADER_SIZE 28

// encryption object
IceKey g_Cipher(1); /* medium encryption level */

unsigned char g_EncryptionKey[8] = { 13, 85, 243, 211, 173, 6, 87, 71 };

//-----------------------------------------------------------------------------
// Purpose: Sits over socket layer, provides multithreaded socket I/O
//-----------------------------------------------------------------------------
class CThreadedSocket : public IThreadedSocket
{
public:
	CThreadedSocket();
	~CThreadedSocket();

	void Shutdown(bool flush);

	void deleteThis()
	{
		delete this;
	}

	// queues a buffer for sending across the network
	virtual void SendMessage(IBinaryBuffer *sendMsg, CNetAddress &address, int sessionID, int targetID, int sequenceNumber, int sequenceReply, bool encrypt);

	// sends a message without the normal packet header
	virtual void SendRawMessage(IBinaryBuffer *message, CNetAddress &address);

	// queues a buffer for broadcasting
	virtual void BroadcastMessage(IBinaryBuffer *sendMsg, int port);

	// gets a message from the network stream, non-blocking, returns true if message returned
	virtual bool GetNewMessage(IBinaryBuffer **BufPtrPtr, CNetAddress *address, bool &encrypted);

	// gets a non-tracker message from the network stream, non-blocking, returns true if message returned
	virtual bool GetNewNonTrackerMessage(IBinaryBuffer **BufPtrPtr, CNetAddress *address);

	virtual ISockets *GetSocket()
	{
		return m_pSocket;
	}

	// functions for walking the socket list
	CThreadedSocket *GetNextSocket()
	{
		return m_pNextSocket;
	}
	void SetNextSocket(CThreadedSocket *socket)
	{
		m_pNextSocket = socket;
	}
	bool RunFrame();

	// returns the number of buffers current in the input and output queues
	int BuffersInQueue()
	{
		return m_iBuffersInQueue;
	}

	int PeakBuffersInQueue()
	{
		return m_iPeakBuffersInQueue;
	}

	int BytesSent()
	{
		return m_iBytesSent;
	}

	int BytesReceived()
	{
		return m_iBytesReceived;
	}

	int BytesSentPerSecond()
	{
		return m_iBytesSentPerSecond;
	}

	int BytesReceivedPerSecond()
	{
		return m_iBytesReceivedPerSecond;
	}

	// sets the windows event to signal when we get a new message
	void SetWindowsEvent(unsigned long event);

private:
	IThreads *m_pThreads;
	ISockets *m_pSocket;

	CThreadedSocket *m_pNextSocket;

	// data on our buffer state
	int m_iBuffersInQueue;
	int m_iPeakBuffersInQueue;
	int m_iSendQueueSize;
	int m_iReceiveQueueSize;
	
	// bandwidth usage
	int m_iBytesSent;
	int m_iBytesReceived;
	int m_iBytesSentPerSecond;
	int m_iBytesReceivedPerSecond;
	int m_iBytesSentThisCount;
	int m_iBytesReceivedThisCount;
	double m_flLastBandwidthCalcTime;

	// time after which the next packet can be sent
	int m_iNextSendTime;

	// event to signal when we receive a packet
	unsigned int m_hEvent;

	// thread main loop
	bool ServiceSendQueue();

	// send buffers
	struct send_buffer_t
	{
		IBinaryBuffer *buffer;
		CNetAddress address;
		int sessionID;
		int targetID;
		int sequenceNumber;
		int sequenceReply;
		bool usePacketHeader;
		bool broadcast;
		bool encrypt;
	};
	bool WriteBufferToNetwork(send_buffer_t *sendBuf);
	void ReadPacket();

	CUtlLinkedList<send_buffer_t, int> m_SendQueue;
	IThreads::CCriticalSection *m_pCSReceiveQueue;
	IThreads::CCriticalSection *m_pCSSendQueue;

	// receive buffers
	struct receive_buffer_t
	{
		IBinaryBuffer *buffer;
		CNetAddress address;
		bool encrypted;
	};

	CUtlLinkedList<receive_buffer_t, int> m_ReceiveQueue;
	CUtlLinkedList<receive_buffer_t, int> m_NonTrackerReceiveQueue;
};

EXPOSE_INTERFACE(CThreadedSocket, IThreadedSocket, THREADEDSOCKET_INTERFACE_VERSION);

//-----------------------------------------------------------------------------
// Purpose: Holds a set of threaded sockets, so that they can share the same thread
//			Only one thread is used, no matter how many sockets are created, for efficiency
//-----------------------------------------------------------------------------
class CThreadedSocketManager : public IThreadRun
{
public:
	virtual int ThreadRun( void )
	{
		m_bThreadRunning = true;
		while (m_bRunning)
		{
			m_pThreads->EnterCriticalSection(m_pListCriticalSection);

			// break out if we're not running
			if (!m_bRunning)
			{
				m_pThreads->LeaveCriticalSection(m_pListCriticalSection);
				break;
			}

			// loop through all the sockets and Run their loop
			bool shouldSleep = true;
			for (CThreadedSocket *socket = m_pListHead; socket != NULL; socket = socket->GetNextSocket())
			{
				if (socket->RunFrame())
				{
					// don't sleep if any work is done
					shouldSleep = false;
				}
			}

			if (m_bRunning && shouldSleep)
			{
				m_pThreads->Sleep(2);
			}

			m_pThreads->LeaveCriticalSection(m_pListCriticalSection);
		}

		return false;
	}

	virtual void ThreadFinished( void ) 
	{
		m_bThreadRunning = false;
	}

	CThreadedSocketManager()
	{
		m_pListHead = NULL;
		m_pThreads = NULL;
		m_pListCriticalSection = NULL;
		m_bRunning = false;
		m_bThreadRunning = false;

		// initialize the cipher
		g_Cipher.set(g_EncryptionKey);
	}

	~CThreadedSocketManager()
	{
		// make sure the thread has finished running
		m_bRunning = false;
		int count = 0;
		while (m_bThreadRunning && ++count < 100)
		{
			m_pThreads->Sleep(10);
		}
		m_pThreads->Sleep(10);

		if (m_pThreads)
		{
			m_pThreads->DeleteCriticalSection(m_pListCriticalSection);
		}
	}

	void RegisterSocket(CThreadedSocket *socket)
	{
		// create the threads
		if (!m_bRunning)
		{
			if (!m_pThreads)
			{
				CreateInterfaceFn factory = Sys_GetFactoryThis();
				m_pThreads = (IThreads *)factory(THREADS_INTERFACE_VERSION, NULL);
				m_pListCriticalSection = m_pThreads->CreateCriticalSection();
			}

			m_pThreads->EnterCriticalSection(m_pListCriticalSection);
			
			// Start a thread running with us
			m_bRunning = true;
			m_pThreads->CreateThread(this);

			m_pThreads->LeaveCriticalSection(m_pListCriticalSection);
		}

		m_pThreads->EnterCriticalSection(m_pListCriticalSection);

		// throw socket in Start of list
		socket->SetNextSocket(m_pListHead);
		m_pListHead = socket;

		m_pThreads->LeaveCriticalSection(m_pListCriticalSection);
	}

	void UnregisterSocket(CThreadedSocket *socket)
	{
		m_pThreads->EnterCriticalSection(m_pListCriticalSection);

		if (socket == m_pListHead)
		{
			// remove the head of the list
			m_pListHead = m_pListHead->GetNextSocket();
			m_pThreads->LeaveCriticalSection(m_pListCriticalSection);

			// Shutdown thread
			m_bRunning = false;
			while (m_bThreadRunning)
			{
				m_pThreads->Sleep(10);
			}
			m_pThreads->Sleep(10);
			return;
		}

		// look for the socket in the list
		CThreadedSocket *prevSocket = NULL;
		for (CThreadedSocket *search = m_pListHead; search != NULL; search = search->GetNextSocket())
		{
			if (search == socket)
			{
				// we've found the socket to remove
				// point the previous socket past the socket to remove
				prevSocket->SetNextSocket(socket->GetNextSocket());
				return;
			}

			prevSocket = socket;
		}

		m_pThreads->LeaveCriticalSection(m_pListCriticalSection);
	}

private:
	bool m_bRunning;
	bool m_bThreadRunning;
	CThreadedSocket *m_pListHead;
	IThreads *m_pThreads;
	IThreads::CCriticalSection *m_pListCriticalSection;
};

//-----------------------------------------------------------------------------
// Purpose: Returns a singleton socket manager
// Output : static CThreadedSocketManager
//-----------------------------------------------------------------------------
static CThreadedSocketManager &GetThreadedSocketManager()
{
	static CThreadedSocketManager singleton;
	return singleton;
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the object and creates a new thread
//-----------------------------------------------------------------------------
CThreadedSocket::CThreadedSocket()
{
	m_iNextSendTime = 0;

	// get interfaces
	CreateInterfaceFn factory = Sys_GetFactoryThis();
	m_pThreads = (IThreads *)factory(THREADS_INTERFACE_VERSION, NULL);
	m_pSocket = (ISockets *)factory(SOCKETS_INTERFACE_VERSION, NULL);

	// setup critical sections for threaded buffer list access
	m_pCSReceiveQueue = m_pThreads->CreateCriticalSection();
	m_pCSSendQueue = m_pThreads->CreateCriticalSection();

	// register self with socket manager
	GetThreadedSocketManager().RegisterSocket(this);

	m_iBuffersInQueue = 0;
	m_iPeakBuffersInQueue = 0;
	m_iSendQueueSize = 0;
	m_iReceiveQueueSize = 0;
	m_hEvent = 0;
	m_iBytesSent = 0;
	m_iBytesReceived = 0;
	m_iBytesSentPerSecond = 0;
	m_iBytesReceivedPerSecond = 0;
	m_iBytesSentThisCount = 0;
	m_iBytesReceivedThisCount = 0;
	m_flLastBandwidthCalcTime = m_pThreads->GetTime();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CThreadedSocket::~CThreadedSocket()
{
	GetThreadedSocketManager().UnregisterSocket(this);

	m_pThreads->DeleteCriticalSection( m_pCSReceiveQueue );
	m_pThreads->DeleteCriticalSection( m_pCSSendQueue );

	m_pSocket->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Runs a frame
//			Returns true if any real work was done by the socket
//-----------------------------------------------------------------------------
bool CThreadedSocket::RunFrame()
{
	bool shouldSleep = true;

	// check for incoming data
	if (m_pSocket->IsDataAvailable())
	{
		ReadPacket();
		shouldSleep = false;
	}

	// send any outgoing data
	if (ServiceSendQueue())
	{
		shouldSleep = false;
	}

	// calculate our buffer sizes
	m_iBuffersInQueue = m_iReceiveQueueSize + m_iSendQueueSize;
	if (m_iBuffersInQueue > m_iPeakBuffersInQueue)
	{
		m_iPeakBuffersInQueue = m_iBuffersInQueue;
	}
	
	if (shouldSleep)
	{
		// recalculate our bandwidth usage rate every 30 seconds or so
		double time = m_pThreads->GetTime();
		if (m_flLastBandwidthCalcTime + 30.0f < time)
		{
			double delta = time - m_flLastBandwidthCalcTime;

			m_iBytesReceivedPerSecond = (int)(m_iBytesReceivedThisCount / delta);
			m_iBytesSentPerSecond = (int)(m_iBytesSentThisCount / delta);
			
			// reset counters
			m_iBytesReceivedThisCount = 0;
			m_iBytesSentThisCount = 0;
			m_flLastBandwidthCalcTime = time;
		}
	}

	// if the received queue is getting long, then we need to make sure 
	// that we give up cycles for them to be processed
	if (m_iReceiveQueueSize > 40)
	{
		shouldSleep = true;
	}

	return !shouldSleep;
}

//-----------------------------------------------------------------------------
// Purpose: sets the windows event to signal when we get a new message
//-----------------------------------------------------------------------------
void CThreadedSocket::SetWindowsEvent(unsigned long event)
{
	m_hEvent = event;
}

//-----------------------------------------------------------------------------
// Purpose: Called externally to deactivate threads
//-----------------------------------------------------------------------------
void CThreadedSocket::Shutdown(bool wait)
{
	// remove ourselves from the socket list
	GetThreadedSocketManager().UnregisterSocket(this);

	if (wait)
	{
		// send any remaining packets
		while (ServiceSendQueue())
		{
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks the send queue, and if any buffers are waiting, writes 
//			them to the network
//-----------------------------------------------------------------------------
bool CThreadedSocket::ServiceSendQueue( void )
{
	if (m_SendQueue.Count())
	{
		/* disable lagged sending, need to do this on a per-user level
		int timeMillis = (int)(m_pThreads->GetTime() * 1000);
		if (m_iNextSendTime > timeMillis)
		{
			// check for clock errors
			if (m_iNextSendTime - timeMillis > 5)
			{
				m_iNextSendTime = timeMillis;
			}

			return false;
		}
		*/

		// take the first item in the queue and remove it from the list
		m_pThreads->EnterCriticalSection( m_pCSSendQueue );

		int headIndex = m_SendQueue.Head();
		send_buffer_t sendBuf = m_SendQueue[headIndex];
		m_SendQueue.Remove(headIndex);

		m_iSendQueueSize = m_SendQueue.Count();
		m_pThreads->LeaveCriticalSection( m_pCSSendQueue );

		// write the network buffer to the socket
		WriteBufferToNetwork( &sendBuf );

		// m_iNextSendTime = timeMillis + 5;
	}

	// returns true if there is more work to do, false otherwise
	return (m_iSendQueueSize > 0);
}

//-----------------------------------------------------------------------------
// Purpose: Hands off a buffer to be sent into the network stream
// Input  : *netBuffer - buffer to send
//-----------------------------------------------------------------------------
void CThreadedSocket::SendMessage(IBinaryBuffer *sendMsg, CNetAddress &address, int sessionID, int targetID, int sequenceNumber, int sequenceReply, bool encrypt)
{
	// add the buffer to the end of the queue
	send_buffer_t sendBuf;
	sendBuf.buffer = sendMsg;
	sendBuf.address = address;
	sendBuf.sessionID = sessionID;
	sendBuf.targetID = targetID;
	sendBuf.sequenceNumber = sequenceNumber;
	sendBuf.sequenceReply = sequenceReply;
	sendBuf.usePacketHeader = true;
	sendBuf.broadcast = false;
	sendBuf.encrypt = encrypt;

	// store a reference to hold on to it
	sendMsg->AddReference();

	m_pThreads->EnterCriticalSection( m_pCSSendQueue );
	m_SendQueue.AddToTail( sendBuf );
	m_iSendQueueSize = m_SendQueue.Count();
	m_pThreads->LeaveCriticalSection( m_pCSSendQueue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CThreadedSocket::SendRawMessage(IBinaryBuffer *sendMsg, CNetAddress &address)
{
	// add the buffer to the end of the queue
	send_buffer_t sendBuf;
	sendBuf.buffer = sendMsg;
	sendBuf.address = address;
	sendBuf.usePacketHeader = false;
	sendBuf.broadcast = false;
	sendBuf.encrypt = false;

	// store a reference to hold on to it
	sendMsg->AddReference();

	m_pThreads->EnterCriticalSection( m_pCSSendQueue );
	m_SendQueue.AddToTail( sendBuf );
	m_iSendQueueSize = m_SendQueue.Count();
	m_pThreads->LeaveCriticalSection( m_pCSSendQueue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CThreadedSocket::BroadcastMessage(IBinaryBuffer *sendMsg, int port)
{
	// add the buffer to the end of the queue
	send_buffer_t sendBuf;
	sendBuf.buffer = sendMsg;

	CNetAddress addr;
	addr.SetPort((unsigned short)port);
	sendBuf.address = addr;
	sendBuf.sessionID = 0;
	sendBuf.targetID = 0;
	sendBuf.sequenceNumber = 0;
	sendBuf.sequenceReply = 0;
	sendBuf.usePacketHeader = true;
	sendBuf.broadcast = true;
	sendBuf.encrypt = false;

	// store a reference to hold on to it
	sendMsg->AddReference();

	m_pThreads->EnterCriticalSection( m_pCSSendQueue );
	m_SendQueue.AddToTail( sendBuf );
	m_iSendQueueSize = m_SendQueue.Count();
	m_pThreads->LeaveCriticalSection( m_pCSSendQueue );
}


//-----------------------------------------------------------------------------
// Purpose: Reads a packet from the network stream
//			Handles reassembly of fragmented packets
//			Puts completely assembled packets into the received queue
//-----------------------------------------------------------------------------
void CThreadedSocket::ReadPacket( void )
{
	if (m_ReceiveQueue.Count() > 1000)
	{
		// we've got 1000 packets queued up, just throw the packet away
		char buf[PACKET_DATA_MAX];
		m_pSocket->ReadData( buf, PACKET_DATA_MAX, NULL );
		return;
	}

	// get a buffer to read into
	IBinaryBuffer *buf = Create_BinaryBuffer(PACKET_DATA_MAX, 0);

	// read the data into the buffer
	CNetAddress netAddress;
	void *bufData = buf->GetBufferData();
	int bytesRead = m_pSocket->ReadData( bufData, buf->GetBufferMaxSize(), &netAddress );

	// record bandwidth usage
	m_iBytesReceived += (bytesRead + UDP_HEADER_SIZE);
	if (m_iBytesReceived < 0)
	{
		m_iBytesReceived = 0;
	}
	m_iBytesReceivedThisCount += (bytesRead + UDP_HEADER_SIZE);

	// set the buffer size
	buf->SetSize(bytesRead);

	if ( bytesRead )
	{
		// add to available packets list
		receive_buffer_t receiveBuf;
		receiveBuf.buffer = buf;
		receiveBuf.address = netAddress;

		// check for decryption
		int flag;
		memcpy(&flag, bufData, 4);
		if (flag == -2)
		{
			// make sure the buffersize is a multiple of 8 + 4, otherwise it's not a valid encrypted packet
			if (bytesRead % 8 != 4)
			{
				buf->Release();
				return;
			}

			DecryptPacket((unsigned char *)bufData, bytesRead);
			receiveBuf.encrypted = true;
		}
		else
		{
			receiveBuf.encrypted = false;
		}

		m_pThreads->EnterCriticalSection( m_pCSReceiveQueue );

		// check for packet type
		if (*((int *)bufData) == -1) 
		{
			m_NonTrackerReceiveQueue.AddToTail(receiveBuf);
		}
		else 
		{
			// need to append them in order if possible
			m_ReceiveQueue.AddToTail(receiveBuf);
			m_iReceiveQueueSize = m_ReceiveQueue.Count();
		}
		
		m_pThreads->LeaveCriticalSection( m_pCSReceiveQueue );

		// signal that we've received a packet
		if (m_hEvent)
		{
			Event_SignalEvent(m_hEvent);
		}
	}
	else
	{
		// no data available; free the buffer we were going to read into
		buf->Release();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets a message from from the network
//			returns NULL if no buffers available
//-----------------------------------------------------------------------------
bool CThreadedSocket::GetNewMessage(IBinaryBuffer **BufPtrPtr, CNetAddress *address, bool &encrypted)
{
	if (m_ReceiveQueue.Count())
	{
		m_pThreads->EnterCriticalSection(m_pCSReceiveQueue);
		int headIndex = m_ReceiveQueue.Head();
		receive_buffer_t receiveBuf = m_ReceiveQueue[headIndex];
		m_ReceiveQueue.Remove(headIndex);
		m_iReceiveQueueSize = m_ReceiveQueue.Count();

		// fill out the return data
		*BufPtrPtr = receiveBuf.buffer; 
		*address = receiveBuf.address;
		encrypted = receiveBuf.encrypted;
		
		m_pThreads->LeaveCriticalSection(m_pCSReceiveQueue);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CThreadedSocket::GetNewNonTrackerMessage(IBinaryBuffer **BufPtrPtr, CNetAddress *address)
{
	if (m_NonTrackerReceiveQueue.Count())
	{
		m_pThreads->EnterCriticalSection(m_pCSReceiveQueue);

		int headIndex = m_NonTrackerReceiveQueue.Head();
		receive_buffer_t receiveBuf = m_NonTrackerReceiveQueue[headIndex];
		m_NonTrackerReceiveQueue.Remove(headIndex);

		// fill out the return data
		*BufPtrPtr = receiveBuf.buffer; 
		*address = receiveBuf.address;
		
		m_pThreads->LeaveCriticalSection(m_pCSReceiveQueue);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Writes a buffer to a network socket
//-----------------------------------------------------------------------------
bool CThreadedSocket::WriteBufferToNetwork( send_buffer_t *sendBuf )
{
	IBinaryBuffer *binaryBuffer = sendBuf->buffer;
	bool success = true;

	// loop through all the buffers
	for ( int i = 0; i < 1; i++ )
	{
		void *bufferData = binaryBuffer->GetBufferData();
		int bufferSize = binaryBuffer->GetBufferSize();
		int reservedSize = binaryBuffer->GetReservedSize();

		// prepare packet for encryption
		if (sendBuf->encrypt)
		{
			// pad the buffer to be the right size for encryption
			int pad = (8 - (bufferSize % 8));	// pad to a 64bit boundary
			binaryBuffer->Write("\0\0\0\0\0\0\0\0\0\0\0\0", pad);

			bufferSize = binaryBuffer->GetBufferSize();
			assert(bufferSize % 8 == 0);
		}

		// add packet header if needed
		if (sendBuf->usePacketHeader)
		{
			// build and copy in the header
			packet_header_t packetHeader = {	PROTOCOL_VERSION,
												(unsigned char)reservedSize,
												(unsigned short)bufferSize,
												sendBuf->targetID,
												sendBuf->sessionID,
												sendBuf->sequenceNumber,
												sendBuf->sequenceReply,
												1, /* packet number */
												1, /* total packets */ };

			::memcpy( bufferData, &packetHeader, sizeof(packetHeader) );
		}

		// encrypt packet
		if (sendBuf->encrypt)
		{
			unsigned char *bufData = (unsigned char *)bufferData;

			// create a copy to encrypt into
			unsigned char buffer[PACKET_DATA_MAX];

			// set encryption marker
			((int *)buffer)[0] = -2;
			EncryptPacketToBuffer(bufData, buffer + 4, bufferSize);

			// mark the encrypted copy as the buffer to send
			bufferData = (void *)buffer;
			bufferSize += 4;
		}

		// record
		m_iBytesSent += (bufferSize + UDP_HEADER_SIZE);
		if (m_iBytesSent < 0)
		{
			m_iBytesSent = 0;
		}
		m_iBytesSentThisCount += (bufferSize + UDP_HEADER_SIZE);

		if (sendBuf->broadcast)
		{
			// broadcast each buffer to the network stream
			success = m_pSocket->BroadcastData( sendBuf->address.Port(), bufferData, bufferSize );
		}
		else
		{
			// write each buffer to the network stream
			success = m_pSocket->SendData( sendBuf->address, bufferData, bufferSize );
		}
		if ( !success )
			break;
	}

	// free the resource
	binaryBuffer->Release();

	return success;
}

//-----------------------------------------------------------------------------
// Purpose: Encrypts a block of data to a seperate block of data
//-----------------------------------------------------------------------------
void EncryptPacketToBuffer(unsigned char *bufData, unsigned char *output, int bufferSize)
{
	assert(bufferSize % 8 == 0);

	unsigned char *cipherText = output;
	unsigned char *plainText = bufData;
	int bytesEncrypted = 0;

	while (bytesEncrypted < bufferSize)
	{
		// encrypt 8 byte section into the out buffer
		g_Cipher.encrypt(plainText, cipherText);

		bytesEncrypted += 8;
		cipherText += 8;
		plainText += 8;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void EncryptPacket(unsigned char *bufData, int bufferSize)
{
	assert(bufferSize % 8 == 0);

	unsigned char *cipherText = bufData + 4;
	unsigned char *plainText = bufData;
	int bytesEncrypted = 0;

	unsigned char plainbuf[8];
	unsigned char cipherbuf[8];
	// copy the first chunk to be encrypted
	memcpy(plainbuf, plainText, 8);

	while (bytesEncrypted < bufferSize)
	{
		// encrypt 8 byte section
		g_Cipher.encrypt(plainbuf, cipherbuf);

		// copy the next chunk in to be encrypted
		memcpy(plainbuf, plainText + 8, 8);

		// copy the encrypted chunk into the cipher buffer
		memcpy(cipherText, cipherbuf, 8);

		bytesEncrypted += 8;
		cipherText += 8;
		plainText += 8;
	}

	// marked the packet as encrypted
	int encryptionMarker = -2;
	memcpy(bufData, &encryptionMarker, 4);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DecryptPacket(unsigned char *bufData, int bufferSize)
{
	assert(bufferSize % 8 == 4);

	unsigned char *cipherText = ((unsigned char *)bufData) + 4;
	unsigned char *plainText = (unsigned char *)bufData;
	int bytesDecrypted = 0;
	unsigned char buf[8];
	while (bytesDecrypted < bufferSize)
	{
		// decrypt 8 byte section
		g_Cipher.decrypt(cipherText, buf);

		// copy into out buffer
		memcpy(plainText, buf, 8);

		bytesDecrypted += 8;
		cipherText += 8;
		plainText += 8;
	}
}
