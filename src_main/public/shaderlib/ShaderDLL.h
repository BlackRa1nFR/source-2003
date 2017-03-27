//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#ifndef SHADERDLL_H
#define SHADERDLL_H

#ifdef _WIN32
#pragma once
#endif

#include <materialsystem/IShader.h>

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IShader;
struct PrecompiledShader_t;


//-----------------------------------------------------------------------------
// The standard implementation of CShaderDLL
//-----------------------------------------------------------------------------
class IShaderDLL
{
public:
	// Adds a shader to the list of shaders
	virtual void InsertShader( IShader *pShader ) = 0;

	// Adds a precompiled shader to the list of shaders
	virtual void InsertPrecompiledShader( PrecompiledShaderType_t type, const PrecompiledShader_t *pVertexShader ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton interface
//-----------------------------------------------------------------------------
IShaderDLL *GetShaderDLL();


#endif // SHADERDLL_H
