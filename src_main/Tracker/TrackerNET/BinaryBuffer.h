//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Generic binary buffer interface
//=============================================================================

#ifndef BINARYBUFFER_H
#define BINARYBUFFER_H
#pragma once

class IBufferAllocator;

//-----------------------------------------------------------------------------
// Purpose: binary buffer interface
//-----------------------------------------------------------------------------
class IBinaryBuffer
{
public:
	// writes numBytes of data into the buffer, at the current position, and advances the current position
	virtual bool Write( const void *data, int numBytes ) = 0;

	// advances the current write position numBytes
	virtual bool Advance( int numBytes ) = 0;

	// returns the total size of the buffer
	virtual int GetSize( void ) = 0;

	// sets the size of the buffer
	virtual void SetSize(int desiredSize) = 0;

	// reads data from the buffer at the current position, advancing the current position
	virtual bool Read( void *outData, int numBytes ) = 0;

	// reads a single character, returning -1 if invalid
	virtual int ReadChar( void ) = 0;

	// reads a string of characters up to a null terminator, returning the number of bytes read (including terminator)
	virtual int ReadString( void *outData, int outBufferSize ) = 0;

	// reads a short from the buffer
	virtual short ReadShort( void ) = 0;

	// reads an int from the buffer
	virtual int ReadInt( void ) = 0;

	// returns a handle to the current write position
	virtual void *GetPosition( void ) = 0;

	// sets the current write position via a handle gotten from GetPosition()
	virtual bool SetPosition( void *position ) = 0;

	// returns a pointer to the buffer data
	virtual void *GetBufferData() = 0;

	// returns the number of bytes written into a buffer
	virtual int GetBufferSize() = 0;

	// returns the amount of size reserved at the Start of each buffer
	virtual int GetReservedSize() = 0;

	// returns the number of bytes the buffer can contain
	virtual int GetBufferMaxSize() = 0;

	// reference counting
	virtual void AddReference() = 0;
	virtual void Release() = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Allocates memory and constructs a new binary buffer
//			This should rarely be used directly
// Input  : *bufferAllocator - 
//-----------------------------------------------------------------------------
extern IBinaryBuffer *Create_BinaryBuffer(int size, int reservedSpace);

#endif // BINARYBUFFER_H
