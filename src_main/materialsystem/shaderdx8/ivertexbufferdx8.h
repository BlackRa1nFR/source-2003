//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Some extra stuff we need for dx8	implementation of vertex buffers
//=============================================================================

#ifndef IVERTEXBUFFERDX8_H
#define IVERTEXBUFFERDX8_H
#pragma once

#include "IVertexBuffer.h"

class IVertexBufferDX8 : public IVertexBuffer
{
public:
	// TEMPORARY!
	virtual int Begin( int flags, int numVerts ) = 0;

	// Sets up the renderstate
	virtual void SetRenderState( int stream	) = 0;

	// Gets FVF info
	virtual void ComputeFVFInfo( int flags, int& fvf, int& size ) const = 0;

	// Cleans up the vertex buffers
	virtual void CleanUp() = 0;

	// Flushes the vertex buffers
	virtual void Flush() = 0;
};

#endif // IVERTEXBUFFERDX8_H
