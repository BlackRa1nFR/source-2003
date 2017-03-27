//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "SendMessage.h"
#include "BinaryBuffer.h"

#include "Sockets.h"
#include "NetAddress.h"
#include "PacketHeader.h"
#include "MemPool.h"

#include <string.h>
#include <assert.h>

// field sizes of msgs
extern int g_MsgFieldSizes[];

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSendMessage::CSendMessage(int msgID, IBinaryBuffer *pBuffer)
{ 
	m_pBuf = pBuffer;
	m_pBuf->AddReference();
	m_bReliable = false;
	m_bEncrypted = false;
	m_iID = msgID;

	// first thing written to the buffer is the name of the buffer
	WriteInt("_id", msgID);
}

//-----------------------------------------------------------------------------
// Purpose: Constructs a network message from existing data
//-----------------------------------------------------------------------------
CSendMessage::CSendMessage(int msgID, const void *pBuf, int bufSize, IBinaryBuffer *pBuffer)
{
	m_pBuf = pBuffer;
	m_pBuf->AddReference();
	m_bReliable = false;
	m_bEncrypted = false;
	m_iID = msgID;

	// copy in the existing data
	m_pBuf->Write(pBuf, bufSize);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSendMessage::~CSendMessage() 
{ 
	// free the buffer
	m_pBuf->Release();
}

//-----------------------------------------------------------------------------
// Purpose: reads out the whole message into a buffer
//-----------------------------------------------------------------------------
void CSendMessage::ReadMsgIntoBuffer(void *dest, int destBufferSize)
{
	if (destBufferSize > MsgSize())
	{
		destBufferSize = MsgSize();
	}

	m_pBuf->SetPosition(m_pBuf->GetBufferData());
	m_pBuf->Advance(m_pBuf->GetReservedSize());
	m_pBuf->Read(dest, destBufferSize);
}

//-----------------------------------------------------------------------------
// Purpose: returns the size needed to hold the whole message
//-----------------------------------------------------------------------------
int CSendMessage::MsgSize()
{
	return m_pBuf->GetBufferSize() - m_pBuf->GetReservedSize();
}

//-----------------------------------------------------------------------------
// Purpose: use to delete
//-----------------------------------------------------------------------------
void CSendMessage::deleteThis()
{
	delete this;
}

//-----------------------------------------------------------------------------
// Purpose: Creates a new message by ID (using ID's from common/TrackerProtocol.h probably)
//-----------------------------------------------------------------------------
ISendMessage *Create_SendMessage( int msgID, IBinaryBuffer *buffer )
{
	return new CSendMessage(msgID, buffer);
}

//-----------------------------------------------------------------------------
// Purpose: Writes out an integer to the message
//-----------------------------------------------------------------------------
void CSendMessage::WriteInt(const char *name, int data)
{
	WriteBlob(name, &data, 4);
}

//-----------------------------------------------------------------------------
// Purpose: Writes out a named unsigned integer to the message
//-----------------------------------------------------------------------------
void CSendMessage::WriteUInt(const char *name, unsigned int data)
{
	WriteBlob(name, &data, 4);
}

//-----------------------------------------------------------------------------
// Purpose: Writes string data to the message
// Input  : *name - name of the variable
//			*data - pointer to the string data to write
//-----------------------------------------------------------------------------
void CSendMessage::WriteString(const char *name, const char *data)
{
	// write out the variable type
	unsigned char vtype = PACKBIT_CONTROLBIT;	// stringname var, string data
	m_pBuf->Write(&vtype, 1);
	
	// write out the variable name
	m_pBuf->Write(name, strlen(name) + 1);

	// write out the size of the data
	unsigned short size = (unsigned short)(strlen(data) + 1);
	m_pBuf->Write(&size, 2);

	// write out the data itself
	m_pBuf->Write(data, size);
}

//-----------------------------------------------------------------------------
// Purpose: Writes out a block of data in keyvalue form
//-----------------------------------------------------------------------------
void CSendMessage::WriteBlob(const char *name, const void *data, int dataSize)
{
	// write out the variable type
	unsigned char vtype = PACKBIT_CONTROLBIT | PACKBIT_BINARYDATA;	// stringname var, binary data
	m_pBuf->Write(&vtype, 1);
	
	// write out the variable name
	m_pBuf->Write(name, strlen(name) + 1);

	// write out the size of the data
	unsigned short size = (unsigned short)dataSize;
	m_pBuf->Write(&size, 2);

	// write out the data itself
	m_pBuf->Write(data, dataSize);
}

static CMemoryPool g_MemPool(sizeof(CSendMessage), 64);

//-----------------------------------------------------------------------------
// Purpose: memory allocation (for using mem pool)
//-----------------------------------------------------------------------------
void *CSendMessage::operator new(unsigned int size)
{
	return g_MemPool.Alloc(size);
}

//-----------------------------------------------------------------------------
// Purpose: memory allocation (for using mem pool)
//-----------------------------------------------------------------------------
void CSendMessage::operator delete(void *pMem)
{
	g_MemPool.Free(pMem);
}
