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
// An interface that should not ever be accessed directly from shaders
// but instead is visible only to shaderlib.
//=============================================================================

#ifndef ISHADERSYSTEM_H
#define ISHADERSYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include <materialsystem/IShader.h>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
enum TextureStage_t;
class ITexture;
class IPrecompiledShaderDictionary;
struct PrecompiledShader_t;
class IShader;


//-----------------------------------------------------------------------------
// The Shader system interface version
//-----------------------------------------------------------------------------
#define SHADERSYSTEM_INTERFACE_VERSION		"ShaderSystem001"


//-----------------------------------------------------------------------------
// Modulation flags
//-----------------------------------------------------------------------------
enum
{
	SHADER_USING_COLOR_MODULATION = 0x1,
	SHADER_USING_ALPHA_MODULATION = 0x2,
};


//-----------------------------------------------------------------------------
// The shader system (a singleton)
//-----------------------------------------------------------------------------
class IShaderSystem
{
public:
	// Binds a texture
	virtual void BindTexture( TextureStage_t stage, ITexture *pTexture, int nFrameVar = 0 ) = 0;

	// Takes a snapshot
	virtual void TakeSnapshot( ) = 0;

	// Draws a snapshot
	virtual void DrawSnapshot() = 0;

	// Are we using graphics?
	virtual bool IsUsingGraphics() const = 0;
};


//-----------------------------------------------------------------------------
// Information about precompiled (vertex + pixel) shaders
//-----------------------------------------------------------------------------
class IPrecompiledShaderDictionary
{
public:
	// Gets at the precompiled shaders defined in this DLL
	virtual PrecompiledShaderType_t GetType() const = 0;
	virtual int ShaderCount() const = 0;
	virtual const PrecompiledShader_t *GetShader( int nShader ) const = 0;
};


//-----------------------------------------------------------------------------
// The Shader plug-in DLL interface version
//-----------------------------------------------------------------------------
#define SHADER_DLL_INTERFACE_VERSION	"ShaderDLL001"


//-----------------------------------------------------------------------------
// The Shader interface versions
//-----------------------------------------------------------------------------
class IShaderDLLInternal
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;

	// Returns the number of shaders defined in this DLL
	virtual int ShaderCount() const = 0;

	// Returns information about each shader defined in this DLL
	virtual IShader *GetShader( int nShader ) = 0;

	// Gets at precompiled shader dictionaries
	virtual int ShaderDictionaryCount() const = 0;
	virtual IPrecompiledShaderDictionary *GetShaderDictionary( int nDictIndex ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton interface
//-----------------------------------------------------------------------------
IShaderDLLInternal *GetShaderDLLInternal();


#endif // ISHADERSYSTEM_H
