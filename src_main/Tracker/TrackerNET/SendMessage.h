//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef SENDMESSAGE_H
#define SENDMESSAGE_H
#pragma once

#include "TrackerNET_Interface.h"
#include "NetAddress.h"
#include "BinaryBuffer.h"

class IBinaryBuffer;
class ISockets;

//-----------------------------------------------------------------------------
// Purpose: SendMessage buffer implementation
//-----------------------------------------------------------------------------
class CSendMessage : public ISendMessage
{
public:
	CSendMessage(int msgID, IBinaryBuffer *pBuffer);
	CSendMessage(int msgID, const void *pBuf, int bufSize, IBinaryBuffer *pBuffer);
	~CSendMessage();

	void WriteInt(const char *name, int data);
	void WriteUInt(const char *name, unsigned int data);
	void WriteString(const char *name, const char *data);
	void WriteBlob(const char *name, const void *data, int dataSize);

	virtual int GetMsgID()
	{
		return m_iID;
	}

	IBinaryBuffer *GetBuffer() { return m_pBuf; }

	bool IsReliable()		{ return m_bReliable; }
	void SetReliable(bool state) { m_bReliable = state; }

	// encryption
	virtual bool IsEncrypted()				{ return m_bEncrypted; }
	virtual void SetEncrypted(bool state)	{ m_bEncrypted = state; }

	// data accessors
	CNetAddress& NetAddress( void )		{ return m_NetAddress; }
	unsigned int SessionID( void )		{ return m_iSessionID; }

	void SetNetAddress( CNetAddress netAddress )	{ m_NetAddress = netAddress; }
	void SetSessionID( unsigned int sessionID )				{ m_iSessionID = sessionID; }

	// reads out the whole message into a buffer
	virtual void ReadMsgIntoBuffer(void *dest, int destBufferSize);
	// returns the size needed to hold the whole message
	virtual int MsgSize();
	// use to delete
	virtual void deleteThis();

	// memory allocation (for using mem pool)
	void *operator new(unsigned int size);
	void operator delete(void *pMem);

private:
	// buffer allocation/freeing
	IBinaryBuffer *m_pBuf;

	// network message packet data 
	CNetAddress m_NetAddress;
	int m_iID;
	unsigned int m_iSessionID;
	bool m_bReliable;
	bool m_bEncrypted;
};

//-----------------------------------------------------------------------------
// Purpose: Creates a new message by ID (using ID's from common/TrackerProtocol.h probably)
//-----------------------------------------------------------------------------
extern ISendMessage *Create_SendMessage( int msgID, IBinaryBuffer *buffer );


#endif // SENDMESSAGE_H
