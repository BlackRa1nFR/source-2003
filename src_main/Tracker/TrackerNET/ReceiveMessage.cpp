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


#include "ReceiveMessage.h"
#include "BinaryBuffer.h"
#include "PacketHeader.h"
#include "TrackerProtocol.h"
#include "MemPool.h"

#include <string.h>
#include <assert.h>

#define min(x,y) (((x) < (y)) ? (x) : (y))

#pragma warning(disable: 4127) // warning C4127: conditional expression is constant

//-----------------------------------------------------------------------------
// Purpose: Constructs a receive message from a network buffer
//-----------------------------------------------------------------------------
CReceiveMessage::CReceiveMessage(IBinaryBuffer *buf, bool encrypted) : m_iSize(0), m_cType(0), m_pBuf(buf)
{
	m_bValid = false;
	m_pBuf->AddReference();
	m_bEncrypted = encrypted;

	// get the Start position of the buffer
	m_pStartPos = m_pBuf->GetPosition();

	// supporting linear reads
	m_pNextVarPos = m_pStartPos;
	m_pFieldDataPos = m_pStartPos;

	// find our name
	m_iID = 0;

	// read our id
	if (!ReadInt("_id", m_iID))
	{
		// failure, no identifier for packet
		return;
	}

	// packet successfully parsed
	m_bValid = true;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CReceiveMessage::~CReceiveMessage()
{
	m_pBuf->Release();
}

//-----------------------------------------------------------------------------
// Purpose: reads out the whole message into a buffer
//-----------------------------------------------------------------------------
void CReceiveMessage::ReadMsgIntoBuffer(void *dest, int destBufferSize)
{
	if (destBufferSize > MsgSize())
	{
		destBufferSize = MsgSize();
	}
	memcpy(dest, m_pBuf->GetBufferData(), destBufferSize);
}

//-----------------------------------------------------------------------------
// Purpose: returns the size needed to hold the whole message
//-----------------------------------------------------------------------------
int CReceiveMessage::MsgSize()
{
	return m_pBuf->GetBufferSize();
}

//-----------------------------------------------------------------------------
// Purpose: use to delete
//-----------------------------------------------------------------------------
void CReceiveMessage::deleteThis()
{
	delete this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CReceiveMessage::IsValid( void )
{
	return (m_bValid);
}

//-----------------------------------------------------------------------------
// Purpose: sets the message as had being failed
//			overwrites the msgID with the failed version
//-----------------------------------------------------------------------------
void CReceiveMessage::SetFailed()
{
	m_iID = m_iID + TMSG_FAIL_OFFSET;

	// reset the buffer position
	m_pBuf->SetPosition(m_pStartPos);

	// write out the variable type
	unsigned char vtype = PACKBIT_CONTROLBIT | PACKBIT_BINARYDATA;	// stringname var, binary data
	m_pBuf->Write(&vtype, 1);
	
	// write out the variable name
	m_pBuf->Write("_id", 4);

	// write out the size of the data
	unsigned short size = (unsigned short)4;
	m_pBuf->Write(&size, 2);

	// write out the data itself
	m_pBuf->Write(&m_iID, size);
}

//-----------------------------------------------------------------------------
// Purpose: reads in the current field, without moving the read point
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CReceiveMessage::ReadField()
{
	// set read to be at the current field pos
	m_pBuf->SetPosition(m_pFieldDataPos);

	// read in the field info

	// read the type
	unsigned char vtype = 1;
	if (!m_pBuf->Read(&vtype, 1))
		return false;

	// check for null-termination type
	if (vtype == 0)
		return false;

	// get the type (1 is binary data, 0 is string data)
	m_iFieldType = (vtype & PACKBIT_BINARYDATA) ? 1 : 0;

	// read the variable name
	m_pBuf->ReadString(m_szFieldName, sizeof(m_szFieldName) - 1);

	// read the data size
	unsigned short size = 0;
	if (!m_pBuf->Read(&size, 2))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: advance the read handle to the next field
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CReceiveMessage::AdvanceField( void )
{
	// set read to be at the current field pos
	m_pBuf->SetPosition(m_pFieldDataPos);

	// read in the field info

	// read the type
	unsigned char vtype = 1;
	if (!m_pBuf->Read(&vtype, 1))
		return false;

	// check for null-termination type
	if (vtype == 0)
		return false;

	// get the type (1 is binary data, 0 is string data)
	m_iFieldType = (vtype & PACKBIT_BINARYDATA);

	// read the variable name
	m_pBuf->ReadString(m_szFieldName, sizeof(m_szFieldName) - 1);

	// read the data size
	unsigned short size = 0;
	if (!m_pBuf->Read(&size, 2))
		return false;

	// just skip over the data block to the next variable
	if (!m_pBuf->Advance(size))
		return false;

	// record the next position
	m_pFieldDataPos = m_pBuf->GetPosition();
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the current data set
//-----------------------------------------------------------------------------
bool CReceiveMessage::GetFieldData(void *pBuf, int bufferSize)
{
	// set read to be at the current field pos
	m_pStartPos = m_pFieldDataPos;
	if (!ReadBlob(m_szFieldName, pBuf, bufferSize))
		return 0;

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CReceiveMessage::ReadInt(const char *name, int &data)
{
	return ReadBlob(name, &data, 4);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CReceiveMessage::ReadUInt(const char *name, unsigned int &data)
{
	return ReadBlob(name, &data, 4);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CReceiveMessage::ReadString(const char *name, char *data, int dataBufferSize)
{
	return ReadBlob(name, data, dataBufferSize);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CReceiveMessage::ReadBlob(const char *name, void *data, int dataBufferSize)
{
	// reset to where we Think the next var will be read from
	m_pBuf->SetPosition(m_pNextVarPos);
	int loopCount = 2;

	// loop through looking for the specified variable
	while (loopCount--)
	{
		while (1)
		{
			// read the type
			unsigned char vtype = 1;
			if (!m_pBuf->Read(&vtype, 1))
				break;

			// check for null-termination type
			if (vtype == 0)
				break;

			// read the variable name
			char varname[32];
			m_pBuf->ReadString(varname, 31);

			// read the data size
			unsigned short size = 0;
			if (!m_pBuf->Read(&size, 2))
				break;

			// is this our variable?
			if (!stricmp(varname, name))
			{
				// read the data itself and return that
				if (dataBufferSize < size)
				{
					// result buffer is smaller than the data
					m_pBuf->Read(data, dataBufferSize);

					// advance the rest of the way past the data
					m_pBuf->Advance(size - dataBufferSize);
				}
				else
				{
					m_pBuf->Read(data, size);
				}

				// store of the next position, since that is probably where the next read needs to occur
				m_pNextVarPos = m_pBuf->GetPosition();
				return size;
			}

			// skip over the data block to the next variable
			if (!m_pBuf->Advance(size))
				break;
		}

		// we haven't found the data yet, Start again
		m_pBuf->SetPosition(m_pStartPos);
	}

	// field not found, null out data
	memset(data, 0, dataBufferSize);
	return 0;
}

static CMemoryPool g_MemPool(sizeof(CReceiveMessage), 64);

//-----------------------------------------------------------------------------
// Purpose: memory allocation (for using mem pool)
//-----------------------------------------------------------------------------
void *CReceiveMessage::operator new(unsigned int size)
{
	return g_MemPool.Alloc(size);
}

//-----------------------------------------------------------------------------
// Purpose: memory allocation (for using mem pool)
//-----------------------------------------------------------------------------
void CReceiveMessage::operator delete(void *pMem)
{
	g_MemPool.Free(pMem);
}
