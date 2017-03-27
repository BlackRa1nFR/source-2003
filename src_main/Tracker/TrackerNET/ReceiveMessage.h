//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef RECEIVEMESSAGE_H
#define RECEIVEMESSAGE_H
#pragma once

#include "TrackerNET_Interface.h"
#include "NetAddress.h"

class IBinaryBuffer;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CReceiveMessage : public IReceiveMessage
{
public:
	CReceiveMessage(IBinaryBuffer *buf, bool encrypted);
	~CReceiveMessage();

	// basic read functions
	int ReadInt(const char *name, int &data);
	int ReadUInt(const char *name, unsigned int &data);
	int ReadString(const char *name, char *data, int dataBufferSize);
	int ReadBlob(const char *name, void *data, int dataBufferSize);

	bool IsValid( void );
	virtual int GetMsgID()
	{
		return m_iID;
	}

	// sets the message as had being failed
	void SetFailed();

	// reads in the current field, without moving the read point
	bool ReadField( void );

	// advance to the next field
	bool AdvanceField( void );

	// data access
	int GetFieldType( void )		 { return m_iFieldType; }
	const char *GetFieldName( void ) { return m_szFieldName; }
	virtual int GetFieldSize( void ) { return m_iFieldDataSize; }

	bool GetFieldData(void *pBuf, int bufferSize);

	virtual bool WasEncrypted()	{ return m_bEncrypted; }

	CNetAddress& NetAddress()	{ return m_NetAddress; }
	unsigned int SessionID()	{ return m_iSessionID; }

	void SetNetAddress(CNetAddress netAddress)	{ m_NetAddress = netAddress; }
	void SetSessionID(unsigned int sessionID)	{ m_iSessionID = sessionID; }

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
	CNetAddress m_NetAddress;
	unsigned int m_iSessionID;
	int m_iID;

	unsigned short m_iSize;
	unsigned char m_cType;
	IBinaryBuffer *m_pBuf;
	bool m_bEncrypted;
	bool m_bValid;

	void *m_pStartPos;	// Start position in the buffer
	void *m_pNextVarPos;	// a guess at which variable is most likely to be read next

	// current field data
	char m_szFieldName[ITrackerNET::MSG_NAME_LENGTH];	// name of the current field
	void *m_pFieldDataPos;		// buffer handle to position of the current data field
	int m_iFieldType;		// type of data in current field
	int m_iFieldDataSize;	// size of the data in the current field (1 is binary data, 0 is string data)
};

#endif // RECEIVEMESSAGE_H
