//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef BUFFERALLOCATOR_H
#define BUFFERALLOCATOR_H
#pragma once

class IBinaryBuffer;

//-----------------------------------------------------------------------------
// Purpose: interface to allocating new binary buffers
//-----------------------------------------------------------------------------
class IBufferAllocator
{
public:
	virtual IBinaryBuffer *AllocateBuffer( void ) = 0;
	virtual void ReleaseBuffer( IBinaryBuffer * ) = 0;

	virtual int BufferSize( void ) = 0;
	virtual int ReservedSpace( void ) = 0;
};


#endif // BUFFERALLOCATOR_H
