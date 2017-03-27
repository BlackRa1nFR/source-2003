/*==============================================================================
   Copyright (c) Valve, LLC. 1999
   All Rights Reserved.  Proprietary.
--------------------------------------------------------------------------------
   Author : DSpeyrer
   Purpose: Defines an interface for circular buffers. Data can be written to
			and read from these buffers as though from a file. When it is 
			write-overflowed (you write more data in than the buffer can hold),
			the read pointer is advanced to allow the new data to be written.
			This means old data will be discarded if you write too much data
			into the buffer.

--------------------------------------------------------------------------------
 * $Log:         MikeD:	Moved all the functions into a class.
						Changed it so when the buffer overflows, the old data
						is discarded rather than the new.

 * 				 
 * $NoKeywords:
==============================================================================*/

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#pragma once

class CCircularBuffer
{
protected:
						CCircularBuffer(int size);

	void				SetSize(int nSize);
	void				AssertValid();

public:

	void				Flush();
	int					GetSize();					// Get the size of the buffer (how much can you write without reading
													// before losing data.
	
	int					GetWriteAvailable();		// Get the amount available to write without overflowing.
													// Note: you can write however much you want, but it may overflow,
													// in which case the newest data is kept and the oldest is discarded.
	
	int					GetReadAvailable();			// Get the amount available to read.
	
	int					GetMaxUsed();
	int					Peek(char *pchDest, int nCount);
	int					Advance(int nCount);
	int					Read(void *pchDest, int nCount);
	int					Write(void *pchData, int nCount);

public:
	int		m_nCount;			// Space between the read and write pointers (how much data we can read).
	
	int		m_nRead;			// Read index into circular buffer
	int		m_nWrite;			// Write index into circular buffer
	
	int		m_nSize;			// Size of circular buffer in bytes (how much data it can hold).
	char	m_chData[1];		// Circular buffer holding data
};




// Use this to instantiate a CircularBuffer.
template<int size>
class CSizedCircularBuffer : public CCircularBuffer
{
public:
				CSizedCircularBuffer() :
					CCircularBuffer(size)
				{
				}

private:
	char		myData[size-1];
};


#endif // CIRCULARBUFFER_H