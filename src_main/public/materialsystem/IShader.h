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
// The interface plug-in shader DLLs must implement
//=============================================================================

#ifndef ISHADER_H
#define ISHADER_H

#ifdef _WIN32
#pragma once
#endif


#include "materialsystem/imaterialsystem.h"
#include "materialsystem/ishaderapi.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterialVar;
class IShaderShadow;
class IShaderDynamicAPI;
class IShaderInit;


//-----------------------------------------------------------------------------
// Information about each shader parameter
//-----------------------------------------------------------------------------
struct ShaderParamInfo_t
{
	const char *m_pName;
	const char *m_pHelp;
	ShaderParamType_t m_Type;
	const char *m_pDefaultValue;
};


//-----------------------------------------------------------------------------
// The public methods exposed by each shader
//-----------------------------------------------------------------------------
class IShader
{
public:
	// Returns the shader name
	virtual char const* GetName( ) const = 0;

	// returns the shader fallbacks
	virtual char const* GetFallbackShader( IMaterialVar** params ) const = 0;

	// Shader parameters
	virtual int GetNumParams( ) const = 0;

	// These functions must be implemented by the shader
	virtual void InitShaderParams( IMaterialVar** ppParams, const char *pMaterialName ) = 0;
	virtual void InitShaderInstance( IMaterialVar** ppParams, IShaderInit *pShaderInit, const char *pMaterialName ) = 0;
	virtual void DrawElements( IMaterialVar **params, int nModulationFlags,
		IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI ) = 0;

//	virtual const ShaderParamInfo_t& GetParamInfo( int paramIndex ) const = 0;
	virtual char const* GetParamName( int paramIndex ) const = 0;
	virtual char const* GetParamHelp( int paramIndex ) const = 0;
	virtual ShaderParamType_t GetParamType( int paramIndex ) const = 0;
	virtual char const* GetParamDefault( int paramIndex ) const = 0;

	// Returns the software vertex shader (if any)
	virtual	const SoftwareVertexShader_t GetSoftwareVertexShader() const = 0;

	// FIXME: Figure out a better way to do this?
	virtual int ComputeModulationFlags( IMaterialVar** params ) = 0;
	virtual bool NeedsFrameBufferTexture( IMaterialVar **params ) const = 0;

	// FIXME: This is insecure. Move this logic into the materialsystem?
	virtual void SetShaderDLLHandle( int hShaderDLL ) = 0;
	virtual int GetShaderDLLHandle( ) const = 0;
};


//-----------------------------------------------------------------------------
// Shader dictionaries defined in DLLs
//-----------------------------------------------------------------------------
enum PrecompiledShaderType_t
{
	PRECOMPILED_VERTEX_SHADER = 0,
	PRECOMPILED_PIXEL_SHADER,

	PRECOMPILED_SHADER_TYPE_COUNT,
};


//-----------------------------------------------------------------------------
// Flags field of PrecompiledShader_t
//-----------------------------------------------------------------------------
enum
{
	SHADER_USES_SKINNING = 0x1,
	SHADER_USES_LIGHTING = 0x2,
	SHADER_USES_HEIGHT_FOG = 0x4,
	SHADER_USES_MODEL_VIEW_MATRIX = 0x8,
	SHADER_USES_MODEL_VIEW_PROJ_MATRIX = 0x10,
	SHADER_USES_MODEL_MATRIX = 0x20,
	SHADER_CUSTOM_ENUMERATION = 0x40,
};


//-----------------------------------------------------------------------------
// Describes precompiled shaders
//-----------------------------------------------------------------------------
struct PrecompiledShaderByteCode_t
{
	void *m_pRawData;
	int m_nSizeInBytes;
};

struct PrecompiledShader_t
{
	PrecompiledShader_t()
	{
		// Just in case the preprocessor doesn't set this (ie vsh_prep and psh_prep)
		m_nCentroidMask = 0;
		m_nDynamicCombos = 1;
	}
	int m_nShaderCount;
	int m_nDynamicCombos;
	PrecompiledShaderByteCode_t *m_pByteCode;
	unsigned long m_nFlags;
	unsigned long m_nCentroidMask;
	const char *m_pName;
};


#endif // ISHADER_H
