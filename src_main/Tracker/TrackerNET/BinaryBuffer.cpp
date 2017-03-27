//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "BinaryBuffer.h"
#include "BufferAllocator.h"
#include "..\common\MemPool.h"
#include "Threads.h"

#include <assert.h>
#include <memory.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
// Purpose: Fixed-size implementation of the binary buffer
//-----------------------------------------------------------------------------
class CBinaryBuffer : public IBinaryBuffer
{
public:
	CBinaryBuffer(int size, int reservedSpace);
	~CBinaryBuffer();

	// implentation of interface IBinaryBuffer
	virtual bool Write(const void *data, int numBytes);
	virtual void SetSize(int desiredSize);
	
	virtual bool Advance(int numBytes);
	virtual void *GetPosition();
	virtual bool SetPosition(void *positionHandle);

	virtual void *GetBufferData();
	virtual int GetBufferSize();
	virtual int GetSize();
	virtual bool Read(void *outData, int numBytes);
	virtual int ReadChar();
	virtual int ReadString(void *outData, int outBufferSize);
	virtual short ReadShort();
	virtual int ReadInt();
	virtual int GetReservedSize()				{ return m_iReservedSpace; }
	virtual int GetBufferMaxSize()				{ return m_iBufSize; }

	virtual void AddReference()
	{
		m_iReferenceCount++;
	}

	virtual void Release()
	{
		if (--m_iReferenceCount <= 0)
		{
			deleteThis();
		}
	}

	// memory handling
	void *operator new( unsigned int iAllocSize );
	void operator delete( void *pMem );

private:
	virtual void deleteThis();
	int WriteData( char *position, const void *data, int numBytes );
	int ReadData( char *outData, int numBytes );

	int m_iReferenceCount;	// reference count

	int m_iReservedSpace;
	int m_iBufSize;  // total size of buffer
	char *m_pBuf;    // Start of memory block
	char *m_pReadWritePos;  // current write position
	char *m_pEndPos; // pointer at where the buffer memory is invalid
};


//-----------------------------------------------------------------------------
// Purpose: Holds the memory pools and protects them from multithreading damage
//-----------------------------------------------------------------------------
class CBufferManager
{
public:
	CBufferManager() : s_BBMemPool(sizeof(CBinaryBuffer), 32), s_BufferMemPool(sizeof(char) * 1300, 32)
	{
		CreateInterfaceFn factory = Sys_GetFactoryThis();
		m_pThreads = (IThreads *)factory(THREADS_INTERFACE_VERSION, NULL);
		m_csMem = m_pThreads->CreateCriticalSection();

		// test the buffers
		void *mem[32];

		for (int i = 0; i < 32; i++)
		{
			mem[i] = s_BBMemPool.Alloc(sizeof(CBinaryBuffer));
		}
		for (i = 0; i < 32; i++)
		{
			s_BBMemPool.Free(mem[i]);
		}
	}

	~CBufferManager()
	{
		m_pThreads->DeleteCriticalSection(m_csMem);
	}

	inline CMemoryPool &BBMemPool()
	{
		m_pThreads->EnterCriticalSection(m_csMem);
		return s_BBMemPool;
	}

	inline void ReleaseBBMemPool()
	{
		m_pThreads->LeaveCriticalSection(m_csMem);
	}

	inline CMemoryPool &BufferMemPool()
	{
		m_pThreads->EnterCriticalSection(m_csMem);
		return s_BufferMemPool;
	}

	inline void ReleaseBufferMemPool()
	{
		m_pThreads->LeaveCriticalSection(m_csMem);
	}

private:
	IThreads *m_pThreads;
	IThreads::CCriticalSection *m_csMem;
	CMemoryPool s_BBMemPool;
	CMemoryPool s_BufferMemPool;
};

inline CBufferManager &GetMemManager()
{
	static CBufferManager bufMan;
	return bufMan;
}

//-----------------------------------------------------------------------------
// Purpose: Creates a new binary buffer
// Input  : bufferSize - 
//-----------------------------------------------------------------------------
IBinaryBuffer *Create_BinaryBuffer(int size, int reservedSpace)
{
	return new CBinaryBuffer(size, reservedSpace);
};

int g_pBufferCount = 0;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *bufferAllocator - through which new buffers can be allocated
//			bufferSize - size of each buffer
//-----------------------------------------------------------------------------
CBinaryBuffer::CBinaryBuffer(int size, int reservedSpace)
{
	m_iBufSize = size;
	m_iReservedSpace = reservedSpace;

	g_pBufferCount++;

	// allocate the buffer
	if (m_iBufSize == 1300)
	{
		m_pBuf = (char *)GetMemManager().BufferMemPool().Alloc(m_iBufSize);
		GetMemManager().ReleaseBufferMemPool();
	}
	else
	{
		m_pBuf = new char[m_iBufSize];
	}
	m_pReadWritePos = m_pBuf;
	m_pEndPos = (char*)m_pBuf + m_iBufSize;

	m_iReferenceCount = 0;
	Advance( m_iReservedSpace );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the size of the buffer
// Input  : desiredSize - 
//-----------------------------------------------------------------------------
void CBinaryBuffer::SetSize(int desiredSize)
{
	m_pEndPos = m_pBuf + desiredSize;
}

//-----------------------------------------------------------------------------
// Purpose: Frees any subsequent buffers
//-----------------------------------------------------------------------------
CBinaryBuffer::~CBinaryBuffer()
{
	if (m_iBufSize == 1300)
	{
		GetMemManager().BufferMemPool().Free(m_pBuf);
		GetMemManager().ReleaseBufferMemPool();
	}
	else
	{
		delete [] m_pBuf;
	}

	g_pBufferCount--;
}

//-----------------------------------------------------------------------------
// Purpose: self-delete
//-----------------------------------------------------------------------------
void CBinaryBuffer::deleteThis()
{
	delete this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iAllocSize - 
// Output : void *CBinaryBuffer::operator
//-----------------------------------------------------------------------------
void *CBinaryBuffer::operator new(unsigned int iAllocSize)
{
	void *buf = (char *)GetMemManager().BBMemPool().Alloc(iAllocSize);
	GetMemManager().ReleaseBBMemPool();
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMem - 
// Output : void CBinaryBuffer::operator
//-----------------------------------------------------------------------------
void CBinaryBuffer::operator delete(void *pMem)
{
	GetMemManager().BBMemPool().Free(pMem);
	GetMemManager().ReleaseBBMemPool();
}

//-----------------------------------------------------------------------------
// Purpose: Writes to the block at the current write position in the current buffer
// Input  : void *data - 
//			numBytes - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBinaryBuffer::Write(const void *data, int numBytes)
{
	m_pReadWritePos += WriteData(m_pReadWritePos, data, numBytes);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Writes to the block at the specified position
//			Allocates and overflows into the next buffer if necessary
//			Assumes the position pointer is valid
// Input  : *position - 
//			void *data - 
//			numBytes - 
// Output : Returns the number of bytes written
//-----------------------------------------------------------------------------
int CBinaryBuffer::WriteData( char *position, const void *data, int numBytes )
{
	// check to see if it's going to fit
	if (m_pEndPos < m_pReadWritePos + numBytes)
	{
		// write failed, don't write anything
		assert(!("CBinaryBuffer::WriteData() failed: buffer overflow"));
		return 0;
	}

	// normal write
	memcpy(position, data, numBytes);
	return numBytes;
}

//-----------------------------------------------------------------------------
// Purpose: Advances the current write position numBytes
// Input  : numBytes - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBinaryBuffer::Advance(int numBytes)
{
	// check to see if it's going to fit
	if (m_pEndPos < m_pReadWritePos + numBytes)
	{
		// can't advance, fail
		return false;
	}

	m_pReadWritePos += numBytes;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Reads data from the stream, advancing the read pointer
// Input  : *outData - 
//			numBytes - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBinaryBuffer::Read(void *outData, int numBytes)
{
	int read = ReadData((char*)outData, numBytes);
	if (read != numBytes)
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a single character from the stream
// Output : int - the character, or -1 if the read failed
//-----------------------------------------------------------------------------
int CBinaryBuffer::ReadChar( void )
{
	unsigned char x;
	if ( Read(&x, 1) )
		return x;

	// failure
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: reads a string of characters up to a null terminator
//			returns the number of bytes read (including terminator)
// Input  : *outData - 
//			outBufferSize - 
// Output : int
//-----------------------------------------------------------------------------
int CBinaryBuffer::ReadString( void *outData, int outBufferSize )
{
	//!! slow implementation
	char *stringOut = (char*)outData;

	int read = 0;
	while ( read < (outBufferSize + 1) )
	{
		int ch = ReadChar();

		// check for null termination
		if ( ch < 1 )
			break;

		stringOut[read++] = (char)ch;
	}

	stringOut[read] = 0;
	return read + 1;  // bytesRead + null terminator
}

//-----------------------------------------------------------------------------
// Purpose: Returns a short from the buffer
// Output : short
//-----------------------------------------------------------------------------
short CBinaryBuffer::ReadShort( void )
{
	short result;
	Read( &result, sizeof(result) );
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Returns an int from the buffer
// Output : short
//-----------------------------------------------------------------------------
int CBinaryBuffer::ReadInt( void )
{
	int result;
	Read( &result, sizeof(result) );
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Reads bytes from the stream
// Input  : *outData - 
//			numBytes - 
// Output : int
//-----------------------------------------------------------------------------
int CBinaryBuffer::ReadData( char *outData, int numBytes )
{
	// check to see if it's going to fit
	if (m_pEndPos < m_pReadWritePos + numBytes)
	{
		// nothing more to read
		return 0;
	}

	// normal read
	memcpy(outData, m_pReadWritePos, numBytes);
	m_pReadWritePos += numBytes;

	return numBytes;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the current write position
//-----------------------------------------------------------------------------
void *CBinaryBuffer::GetPosition()
{
	return m_pReadWritePos;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the current read/write position in the readbuffer
// Input  : *position - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBinaryBuffer::SetPosition(void *position)
{
	// quick check to see if the position is in the current buffer
	if ((position >= m_pBuf) && (position < m_pEndPos))
	{
		m_pReadWritePos = (char *)position;
		return true;
	}

	// handle is invalid
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the total size of all the buffers
// Output : int
//-----------------------------------------------------------------------------
int CBinaryBuffer::GetSize()
{
	return (m_pEndPos - m_pBuf);
}

//-----------------------------------------------------------------------------
// Purpose: Returns the size of a buffer
// Input  : bufferNum - 
// Output : int
//-----------------------------------------------------------------------------
int CBinaryBuffer::GetBufferSize()
{
	return (char *)m_pReadWritePos - (char *)m_pBuf;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the data in buffer bufferNum 
// Input  : bufferNum - 
//-----------------------------------------------------------------------------
void *CBinaryBuffer::GetBufferData()
{
	return m_pBuf;
}





