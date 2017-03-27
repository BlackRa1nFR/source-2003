//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef THREADEDSOCKET_H
#define THREADEDSOCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "NetAddress.h"

class IBinaryBuffer;
class ISockets;

//-----------------------------------------------------------------------------
// Purpose: Sits over socket layer, provides multithreaded socket I/O
//-----------------------------------------------------------------------------
class IThreadedSocket : public IBaseInterface
{
public:
	// shuts down the networking
	// if wait is true, doesn't return until the thread has stopped
	virtual void Shutdown( bool wait ) = 0;

	// call to delete object safely
	virtual void deleteThis() = 0;
	
	// queues a buffer for sending across the network
	virtual void SendMessage(IBinaryBuffer *sendMsg, CNetAddress &address, int sessionID, int targetID, int sequenceNumber, int sequenceReply, bool encrypt) = 0;

	// queues a buffer for broadcasting
	virtual void BroadcastMessage(IBinaryBuffer *sendMsg, int port) = 0;

	// sends a message without the normal packet header
	virtual void SendRawMessage(IBinaryBuffer *message, CNetAddress &address) = 0;

	// gets a message from the network stream, non-blocking, returns true if message returned
	virtual bool GetNewMessage(IBinaryBuffer **BufPtrPtr, CNetAddress *address, bool &encrypted) = 0;

	// gets a non-tracker message from the network stream, non-blocking, returns true if message returned
	virtual bool GetNewNonTrackerMessage(IBinaryBuffer **BufPtrPtr, CNetAddress *address) = 0;

	// returns the number of buffers current in the input and output queues
	virtual int BuffersInQueue() = 0;

	// returns the peak number of buffers in the queues
	virtual int PeakBuffersInQueue() = 0;

	// bandwidth usage
	virtual int BytesSent() = 0;
	virtual int BytesReceived() = 0;
	virtual int BytesSentPerSecond() = 0;
	virtual int BytesReceivedPerSecond() = 0;

	// returns a pointer to the base socket
	virtual ISockets *GetSocket() = 0;

	// sets the windows event to signal when we get a new message
	virtual void SetWindowsEvent(unsigned long event) = 0;
};

#define THREADEDSOCKET_INTERFACE_VERSION "ThreadedSocket002"


#endif // THREADEDSOCKET_H
