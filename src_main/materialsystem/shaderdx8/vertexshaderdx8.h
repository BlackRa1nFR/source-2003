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
// Code for dealing with lovely vertex shaders
//=============================================================================

#ifndef VERTEXSHADERDX8_H
#define VERTEXSHADERDX8_H

#ifdef _WIN32
#pragma once
#endif

#include "shaderapi.h"
#include "locald3dtypes.h"

enum VertexShaderLightTypes_t
{
	LIGHT_NONE = -1,
	LIGHT_SPOT = 0,
	LIGHT_POINT = 1,
	LIGHT_DIRECTIONAL = 2,
	LIGHT_STATIC = 3,
	LIGHT_AMBIENTCUBE = 4,
};

//-----------------------------------------------------------------------------
// Vertex + pixel shader manager
//-----------------------------------------------------------------------------
class IShaderManager
{
public:
	// Initialize, shutdown
	virtual void Init( bool bPreloadShaders ) = 0;
	virtual void Shutdown() = 0;

	// Creates vertex, pixel shaders
	virtual VertexShader_t CreateVertexShader( const char *pVertexShaderFile, int nStaticVshIndex = 0 ) = 0;
	virtual PixelShader_t CreatePixelShader( const char *pPixelShaderFile, int nStaticPshIndex = 0 ) = 0;

	// Sets which dynamic version of the vertex + pixel shader to use
	virtual void SetVertexShaderIndex( int vshIndex ) = 0;
	virtual void SetPixelShaderIndex( int pshIndex ) = 0;

	// Sets the vertex + pixel shader render state
	virtual void SetVertexShader( VertexShader_t shader, int nStaticVshIndex = 0 ) = 0;
	virtual void SetPixelShader( PixelShader_t shader, int nStaticPshIndex = 0 ) = 0;

	// Resets the vertex + pixel shader state
	virtual void ResetShaderState() = 0;

	// Returns the current vertex + pixel shaders
	virtual void*	GetCurrentVertexShader() = 0;
	virtual void*	GetCurrentPixelShader() = 0;

	// Precompiled shaders in seperate DLLs
	virtual ShaderDLL_t AddShaderDLL( ) = 0;
	virtual void RemoveShaderDLL( ShaderDLL_t hShaderDLL ) = 0;
	virtual void AddShaderDictionary( ShaderDLL_t hShaderDLL, IPrecompiledShaderDictionary *pDict ) = 0;
	virtual void SetShaderDLL( ShaderDLL_t hShaderDLL ) = 0;
};


//-----------------------------------------------------------------------------
// Computes the lighting type from the light types...
// FIXME: Remove this!!!
//----------------------------------------------------------------------------
int ComputeLightIndex( int numLights, const VertexShaderLightTypes_t *pLightType, 
					  bool bUseAmbientCube, bool bHasColorMesh );


#endif // VERTEXSHADERDX8_H