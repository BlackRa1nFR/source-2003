//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface to the TrackerNET dll
//
//			Handles connecting and validation to the server
//			Can establish once-off (UDP) or continual (TCP) connections to other users
//
//=============================================================================

#ifndef TRACKERNET_INTERFACE_H
#define TRACKERNET_INTERFACE_H
#pragma once

class IReceiveMessage;
class ISendMessage;
class IThreads;
class CNetAddress;
class IBinaryBuffer;

enum Reliability_e { NET_UNRELIABLE, NET_RELIABLE, NET_BROADCAST };

#include "interface.h"

//-----------------------------------------------------------------------------
// Purpose: Initialization and use of the tracker Client networking
//-----------------------------------------------------------------------------
class ITrackerNET : public IBaseInterface
{
public:
	// sets up the networking thread - tries to get a udp port in the specified range
	// return true on success, false on failure
	virtual bool Initialize(unsigned short portMin, unsigned short portMax) = 0;

	// shutsdown the networking
	virtual void Shutdown(bool sendRemainingMessages) = 0;

	// gets the local machine IP address
	virtual CNetAddress GetLocalAddress() = 0;

	// creates a new empty network message
	virtual ISendMessage *CreateMessage(int msgID) = 0;

	// creates a new network message from existing data
	virtual ISendMessage *CreateMessage(int msgID, void const *pBuf, int bufSize) = 0;

	// as CreateMessage() but sets up sequences and network info for reply
	virtual ISendMessage *CreateReply(int msgID, IReceiveMessage *msgToReplyTo ) = 0;

	// sends a message across the wire
	virtual void SendMessage(ISendMessage *pMsg, Reliability_e state) = 0;

	// Gets any info has been received 
	virtual IReceiveMessage *GetIncomingData() = 0;

	// Gets any failed sends - returns any packet that could not be delivered
	virtual IReceiveMessage *GetFailedMessage() = 0;

	// Call to free the buffer of any message received
	virtual void ReleaseMessage(IReceiveMessage *) = 0;

	// raw message handling - messages that don't use the normal tracker packet structure
	// used for communication with servers etc.
	virtual IBinaryBuffer *CreateRawMessage() = 0;

	// sends a raw message, and frees (releases) the buffer
	virtual void SendRawMessage(IBinaryBuffer *message, CNetAddress &address) = 0;

	// gets any info that has been received from out-of-band engine packets
	// call IBinaryBuffer::Release() to free the packet
	virtual IBinaryBuffer *GetIncomingRawData(CNetAddress &address) = 0;

	// Runs a networking frame, handling reliable packet delivery
	// should be called after GetIncomingData()/ReleaseMessage() loop
	virtual void RunFrame() = 0;

	// returns a pointer to the threading api
	virtual IThreads *GetThreadAPI( void ) = 0;

	// converts a string to a net address
	virtual CNetAddress GetNetAddress( const char * ) = 0;

	// returns the port number the socket is listening on
	virtual int GetPort() = 0;

	// safe deletion
	virtual void deleteThis() = 0;

	// returns the number of buffers current in the input and output queues
	virtual int BuffersInQueue() = 0;

	// returns the peak number of buffers in the queues
	virtual int PeakBuffersInQueue() = 0;

	// bandwidth usage
	virtual int BytesSent() = 0;
	virtual int BytesReceived() = 0;
	virtual int BytesSentPerSecond() = 0;
	virtual int BytesReceivedPerSecond() = 0;

	// sets the windows event to signal when we get a new message
	virtual void SetWindowsEvent(unsigned long event) = 0;

	enum
	{ 
		// maximum size of a msg name
		MSG_NAME_LENGTH = 32,
	};
};

#define TRACKERNET_INTERFACE_VERSION "TrackerNET009"

//-----------------------------------------------------------------------------
// Purpose: Interface to constructing an outgoing message
//-----------------------------------------------------------------------------
class ISendMessage
{
public:
	// returns the message id number
	virtual int GetMsgID() = 0;

	// basic writes
	// it is fastest if fields are written in the same order as they are to be read
	virtual void WriteInt(const char *name, int data) = 0;
	virtual void WriteUInt(const char *name, unsigned int data) = 0;
	virtual void WriteString(const char *name, const char *data) = 0;
	virtual void WriteBlob(const char *name, const void *data, int dataSize) = 0;

	// reliability flag
	virtual bool IsReliable() = 0;
	virtual void SetReliable(bool state) = 0;

	// encryption
	virtual bool IsEncrypted() = 0;
	virtual void SetEncrypted(bool state) = 0;

	// net address accessors
	virtual CNetAddress& NetAddress( void ) = 0;
	virtual unsigned int SessionID( void ) = 0;

	virtual void SetNetAddress( CNetAddress netAddress ) = 0;
	virtual void SetSessionID( unsigned int sessionID ) = 0;

	// reads out the whole message into a buffer
	virtual void ReadMsgIntoBuffer(void *dest, int destBufferSize) = 0;
	
	// returns the size needed to hold the whole message
	virtual int MsgSize() = 0;

	// use to delete
	virtual void deleteThis() = 0;

protected:
	// virtual destructor, can't be called directly
	virtual ~ISendMessage() {}
};

//-----------------------------------------------------------------------------
// Purpose: Interface to an incoming network message
//-----------------------------------------------------------------------------
class IReceiveMessage
{
public:
	// returns the message id name
	virtual int GetMsgID() = 0;

	// net address accessors
	virtual CNetAddress& NetAddress( void ) = 0;
	virtual unsigned int SessionID( void ) = 0;

	// returns true if the packet was encrypted
	virtual bool WasEncrypted() = 0;

	// data reads - find the named variable in the packet and writes it into the buffer
	// reads are more optimal if they are done in the same order that they were written
	// returns number of bytes read
	// the data buffer is nulled out on failure
	virtual int ReadInt(const char *name, int &data) = 0;
	virtual int ReadUInt(const char *name, unsigned int &data) = 0;
	virtual int ReadString(const char *name, char *data, int dataBufferSize) = 0;
	virtual int ReadBlob(const char *name, void *data, int dataBufferSize) = 0;

	// these functions are all for iterating through all the field in the packet
	// reads in the current field, without moving the read point
	virtual bool ReadField() = 0;
	// advance to the next field
	virtual bool AdvanceField() = 0;
	// type of the field
	virtual int GetFieldType() = 0;
	// name of the field
	virtual const char *GetFieldName() = 0;
	// returns the number of bytes in the field data block
	virtual int GetFieldSize() = 0;
	// copies out the field data, returning the number of bytes read
	virtual bool GetFieldData(void *pBuf, int bufferSize) = 0;

	// reads out the whole message into a buffer
	virtual void ReadMsgIntoBuffer(void *dest, int destBufferSize) = 0;
	
	// returns the size needed to hold the whole message
	virtual int MsgSize() = 0;

	// use to delete
	virtual void deleteThis() = 0;

protected:
	// virtual destructor, can't be called directly
	virtual ~IReceiveMessage() {}
};

#endif // TRACKERNET_INTERFACE_H
