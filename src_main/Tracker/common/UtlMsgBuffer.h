//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Generic named data buffer, declaration and implementation
//
//=============================================================================

#ifndef UTLMSGBUFFER_H
#define UTLMSGBUFFER_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlMemory.h"
#include <string.h>

#pragma warning(disable: 4244) // warning C4244: '+=' : conversion from 'int' to 'short', possible loss of data

//-----------------------------------------------------------------------------
// Purpose: Generic named data buffer
//-----------------------------------------------------------------------------
class CUtlMsgBuffer
{
public:
	CUtlMsgBuffer(int initialSize = 0);
	CUtlMsgBuffer(void const *data, int dataSize);
	~CUtlMsgBuffer();

	// read functions
	bool ReadInt(const char *name, int &data);
	bool ReadUInt(const char *name, unsigned int &data);
	bool ReadString(const char *name, char *data, int dataBufferSize);
	bool ReadBlob(const char *name, void *data, int dataBufferSize);
	bool ReadBuffer(const char *name, CUtlMsgBuffer &buffer);

	// reads out the next variable available in the buffer
	// fills out parameters with var details and data
	// returns false if no more vars available
	bool ReadNextVar(char name[32], bool &stringData, void *data, int &dataSize);

	// write functions
	void WriteInt(const char *name, int data);
	void WriteUInt(const char *name, unsigned int data);
	void WriteString(const char *name, const char *data);
	void WriteBlob(const char *name, const void *data, int dataSize);
	void WriteBuffer(const char *name, CUtlMsgBuffer *buffer);

	// returns a pointer to the data buffer, and its size, of the specified variable
	void *FindVar(const char *name, int &dataSizeOut);

	// pads the buffer to the specified boundary (in bytes)
	void PadBuffer(int boundary);

	// returns the number of bytes used by the message
	int DataSize();

	// returns a pointer to the base data
	void *Base();

	// copy constructor
	CUtlMsgBuffer(CUtlMsgBuffer &rhs)
	{
		m_iWritePos = rhs.m_iWritePos;
		m_iReadPos = rhs.m_iReadPos;
		m_iNextVarPos = rhs.m_iNextVarPos;

		m_Memory.EnsureCapacity(rhs.m_Memory.NumAllocated());
		memcpy(Base(), rhs.Base(), rhs.m_Memory.NumAllocated());
	}

private:

	bool Read(void *buffer, int readAmount);
	bool ReadUntilNull(void *buffer, int bufferSize);
	void Write(void const *data, int size);

	CUtlMemory<char> m_Memory;

	short m_iWritePos;		// position in buffer we are currently writing to
	short m_iReadPos;			// current reading position
	short m_iNextVarPos;		// a guess at which variable is most likely to be read next
};

//-----------------------------------------------------------------------------
// Purpose: bitfields for use in variable descriptors
//-----------------------------------------------------------------------------
enum
{
	PACKBIT_CONTROLBIT	= 0x01,		// this must always be set
	PACKBIT_INTNAME		= 0x02,		// if this is set then it's an int named variable, instead of a string
	PACKBIT_BINARYDATA  = 0x04,		// signifies the data in this variable is binary, it's not a string
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
inline CUtlMsgBuffer::CUtlMsgBuffer(int initialSize) : m_Memory(0, initialSize)
{
	m_iWritePos = 0;
	m_iReadPos = 0;
	m_iNextVarPos = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor, takes initial data
//-----------------------------------------------------------------------------
inline CUtlMsgBuffer::CUtlMsgBuffer(void const *data, int dataSize) : m_Memory(0, dataSize)
{
	m_iWritePos = (short)dataSize;
	m_iReadPos = 0;
	m_iNextVarPos = 0;

	memcpy(Base(), data, dataSize);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
inline CUtlMsgBuffer::~CUtlMsgBuffer()
{
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of bytes used by the message
//-----------------------------------------------------------------------------
inline int CUtlMsgBuffer::DataSize()
{
	// return the highest read/write mark
	if (m_iWritePos > m_iReadPos)
		return m_iWritePos;

	return m_iReadPos;
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to the base data
//-----------------------------------------------------------------------------
inline void *CUtlMsgBuffer::Base()
{
	return &m_Memory[0];
}

//-----------------------------------------------------------------------------
// Purpose: pads the buffer to the specified boundary (in bytes)
//-----------------------------------------------------------------------------
inline void CUtlMsgBuffer::PadBuffer(int boundary)
{
	// pad the buffer to be the right size for encryption
	int pad = (boundary - (DataSize() % boundary));	
	Write("\0\0\0\0\0\0\0\0\0\0\0\0", pad);
}

//-----------------------------------------------------------------------------
// Purpose: Reads in a named 4-byte int, returns true on sucess, false on failure
//-----------------------------------------------------------------------------
inline bool CUtlMsgBuffer::ReadInt(const char *name, int &data)
{
	return ReadBlob(name, &data, 4);
}

//-----------------------------------------------------------------------------
// Purpose: Reads in a named 4-byte unsigned int, returns true on sucess, false on failure
//-----------------------------------------------------------------------------
inline bool CUtlMsgBuffer::ReadUInt(const char *name, unsigned int &data)
{
	return ReadBlob(name, &data, 4);
}

//-----------------------------------------------------------------------------
// Purpose: Reads in a named variable length character string
//			returns true on sucess, false on failure
//-----------------------------------------------------------------------------
inline bool CUtlMsgBuffer::ReadString(const char *name, char *data, int dataBufferSize)
{
	return ReadBlob(name, data, dataBufferSize);
}

//-----------------------------------------------------------------------------
// Purpose: Reads in a named variable length data blob
//			returns true on sucess, false on failure
//-----------------------------------------------------------------------------
inline bool CUtlMsgBuffer::ReadBlob(const char *name, void *data, int dataBufferSize)
{
	int dataSize = 0;
	char *readData = (char *)FindVar(name, dataSize);
	if (!readData)
	{
		memset(data, 0, dataBufferSize);
		return false;
	}

	// ensure against buffer overflow
	if (dataSize > dataBufferSize)
		dataSize = dataBufferSize;

	// copy out data
	memcpy(data, readData, dataSize);
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Reads a blob of binary data into it's own buffer
//-----------------------------------------------------------------------------
inline bool CUtlMsgBuffer::ReadBuffer(const char *name, CUtlMsgBuffer &buffer)
{
	int dataSize = 0;
	char *readData = (char *)FindVar(name, dataSize);
	if (!readData)
	{
		return false;
	}

	buffer.m_Memory.EnsureCapacity(dataSize);
	memcpy(&buffer.m_Memory[0], readData, dataSize);
	buffer.m_iReadPos = 0;
	buffer.m_iWritePos = (short)dataSize;
}

//-----------------------------------------------------------------------------
// Purpose: reads out the next variable available in the buffer
//			fills out parameters with var details and data
//			returns false if no more vars available
//-----------------------------------------------------------------------------
inline bool CUtlMsgBuffer::ReadNextVar(char varname[32], bool &stringData, void *data, int &dataSize)
{
	// read the type
	unsigned char vtype = 1;
	if (!Read(&vtype, 1))
		return false;

	// check for null-termination type
	if (vtype == 0)
		return false;

	stringData = !(vtype & PACKBIT_BINARYDATA);

	// read the variable name
	if (!ReadUntilNull(varname, 31))
		return false;

	// read the data size
	unsigned short size = 0;
	if (!Read(&size, 2))
		return false;

	// ensure against buffer overflows
	if (dataSize > size)
		dataSize = size;

	// copy data
	memcpy(data, &m_Memory[m_iReadPos], dataSize);

	// store of the next position, since that is probably where the next read needs to occur
	m_iReadPos += size;
	m_iNextVarPos = m_iReadPos;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: sets the read/write position to be at the specified variable
//			returns pointer to buffer position on success, NULL if not found
//-----------------------------------------------------------------------------
inline void *CUtlMsgBuffer::FindVar(const char *name, int &dataSize)
{
	// reset to where we Think the next var will be read from
	m_iReadPos = m_iNextVarPos;
	int loopCount = 2;

	// loop through looking for the specified variable
	while (loopCount--)
	{
		unsigned char vtype = 1;
		while (Read(&vtype, 1))
		{
			// check for null-termination type
			if (vtype == 0)
				break;

			// read the variable name
			char varname[32];
			if (!ReadUntilNull(varname, 31))
				break;

			// read the data size
			unsigned short size = 0;
			if (!Read(&size, 2))
				break;

			// is this our variable?
			if (!stricmp(varname, name))
			{
				dataSize = size;
				void *data = &m_Memory[m_iReadPos];

				// store of the next position, since that is probably where the next read needs to occur
				m_iReadPos += size;
				m_iNextVarPos = m_iReadPos;
				return data;
			}

			// skip over the data block to the next variable
			m_iReadPos += size;
			if (m_iReadPos >= m_Memory.NumAllocated())
				break;
		}

		// we haven't found the data yet, Start again
		m_iReadPos = 0;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Writes out an integer to the message
//-----------------------------------------------------------------------------
inline void CUtlMsgBuffer::WriteInt(const char *name, int data)
{
	WriteBlob(name, &data, 4);
}

//-----------------------------------------------------------------------------
// Purpose: Writes out an unsigned integer to the message
//-----------------------------------------------------------------------------
inline void CUtlMsgBuffer::WriteUInt(const char *name, unsigned int data)
{
	WriteBlob(name, &data, 4);
}

//-----------------------------------------------------------------------------
// Purpose: Writes string data to the message
// Input  : *name - name of the variable
//			*data - pointer to the string data to write
//-----------------------------------------------------------------------------
inline void CUtlMsgBuffer::WriteString(const char *name, const char *data)
{
	// write out the variable type
	unsigned char vtype = PACKBIT_CONTROLBIT;	// stringname var, string data
	Write(&vtype, 1);
	
	// write out the variable name
	Write(name, strlen(name) + 1);

	// write out the size of the data
	unsigned short size = (unsigned short)(strlen(data) + 1);
	Write(&size, 2);

	// write out the data itself
	Write(data, size);
}

//-----------------------------------------------------------------------------
// Purpose: Writes out a named block of data
//-----------------------------------------------------------------------------
inline void CUtlMsgBuffer::WriteBlob(const char *name, const void *data, int dataSize)
{
	// write out the variable type
	unsigned char vtype = PACKBIT_CONTROLBIT | PACKBIT_BINARYDATA;	// stringname var, binary data
	Write(&vtype, 1);
	
	// write out the variable name
	Write(name, strlen(name) + 1);

	// write out the size of the data
	unsigned short size = (unsigned short)dataSize;
	Write(&size, 2);

	// write out the data itself
	Write(data, dataSize);
}

//-----------------------------------------------------------------------------
// Purpose: Reads from the buffer, increments read position
//			returns false if past end of buffer
//-----------------------------------------------------------------------------
inline bool CUtlMsgBuffer::Read(void *buffer, int readAmount)
{
	if (m_iReadPos + readAmount > m_Memory.NumAllocated())
		return false;

	memcpy(buffer, &m_Memory[m_iReadPos], readAmount);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Reads characterse from the buffer until a null is hit
//-----------------------------------------------------------------------------
inline bool CUtlMsgBuffer::ReadUntilNull(void *buffer, int bufferSize)
{
	int nullPos = m_iReadPos;

	// look through the buffer for the null terminator
	while (nullPos < m_Memory.NumAllocated() && m_Memory[nullPos] != 0)
	{
		nullPos++;
	}
	
	if (nullPos >= m_Memory.NumAllocated())
	{
		// never found a null terminator
		((char *)buffer)[0] = 0;
		return false;
	}

	// copy from the null terminator
	int copySize = nullPos - m_iReadPos;
	if (copySize > bufferSize)
	{
		copySize = bufferSize - 1;
	}

	// copy out the data and return
	memcpy(buffer, &m_Memory[m_iReadPos], copySize);
	((char *)buffer)[copySize] = 0;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Writes to the buffer, incrementing the write position
//			assumes enough space has already been allocated for the write
//-----------------------------------------------------------------------------
inline void CUtlMsgBuffer::Write(void const *data, int size)
{
	// make sure it will fit
	m_Memory.EnsureCapacity(m_iWritePos + size);
		
	// normal write
	memcpy(&m_Memory[m_iWritePos], data, size);

	// increment write position
	m_iWritePos += size;
}




#endif // UTLMSGBUFFER_H
