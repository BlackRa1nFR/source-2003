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
//
// Shader system:
//	The shader system makes a few fundamental assumptions about when 
//	certain types of state get set.
//
// 1) Anything that can potentially affect vertex format must be set up
//		during the shader shadow/snapshot phase
// 2) Anything that we can dynamically mess with (through a material var)
//		should happen in the dynamic/render phase
// 3) In general, we try to cache off expensive state pre-processing in
//		the shader shadow phase (like texture stage pipeline).
//
//=============================================================================

#ifndef SHADERSYSTEM_H
#define SHADERSYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/IShaderSystem.h"
#include "shaderlib/BaseShader.h"
#include "materialsystem/materialsystem_config.h"
#include "shaderapi.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterialVar;
class TextureManager_t;
class ITextureInternal;
class ShaderSystem_t;
class IMesh;
class Vector;
enum MaterialPrimitiveType_t;
enum MaterialPropertyTypes_t;
enum MaterialVertexFormat_t;
enum ShaderParamType_t;


//-----------------------------------------------------------------------------
// for ShaderRenderState_t::m_flags
//-----------------------------------------------------------------------------
enum
{
	// The flags up here are computed from the shaders themselves

/*
	// lighting flags
	SHADER_UNLIT					= 0x0000,
	SHADER_VERTEX_LIT				= 0x0001,
	SHADER_NEEDS_LIGHTMAP			= 0x0002, 
	SHADER_NEEDS_BUMPED_LIGHTMAPS	= 0x0004,
	SHADER_LIGHTING_MASK			= 0x0007,
*/

	// opacity flags
	SHADER_OPACITY_ALPHATEST		= 0x0010,
	SHADER_OPACITY_OPAQUE			= 0x0020,
	SHADER_OPACITY_TRANSLUCENT		= 0x0040,
	SHADER_OPACITY_MASK				= 0x0070,

	// optimization flags....
	SHADER_USES_AMBIENT_CUBE_STAGE0	= 0x0100,
};


enum
{
	MAX_RENDER_PASSES = 4
};


//-----------------------------------------------------------------------------
// Information for a single render pass
//-----------------------------------------------------------------------------
struct RenderPassList_t
{
	int m_nPassCount;
	StateSnapshot_t	m_Snapshot[MAX_RENDER_PASSES];
};

struct ShaderRenderState_t
{
	// We store 4 sets of snapshots; one for each combo of alpha mod + color mod
	RenderPassList_t m_Snapshots[4];

	// These are the same, regardless of whether alpha or color mod is used
	int				m_Flags;	// Can't shrink this to a short
	VertexFormat_t	m_VertexFormat;
	int				m_VertexUsage;
};


//-----------------------------------------------------------------------------
// Utility methods
//-----------------------------------------------------------------------------
inline void SetFlags( IMaterialVar **params, MaterialVarFlags_t _flag )
{
	params[FLAGS]->SetIntValue( params[FLAGS]->GetIntValue() | (_flag) );
}

inline void SetFlags2( IMaterialVar **params, MaterialVarFlags2_t _flag )
{
	params[FLAGS2]->SetIntValue( params[FLAGS2]->GetIntValue() | (_flag) );
}

inline bool IsFlagSet( IMaterialVar **params, MaterialVarFlags_t _flag )
{
	return ((params[FLAGS]->GetIntValue() & (_flag) ) != 0);
}

inline bool IsFlag2Set( IMaterialVar **params, MaterialVarFlags2_t _flag )
{
	return ((params[FLAGS2]->GetIntValue() & (_flag) ) != 0);
}



//-----------------------------------------------------------------------------
// Poll params + renderstate
//-----------------------------------------------------------------------------
inline bool	IsTranslucent( ShaderRenderState_t* pRenderState )
{
	return (pRenderState->m_Flags & SHADER_OPACITY_TRANSLUCENT) != 0;
}

inline bool	IsAlphaTested( ShaderRenderState_t* pRenderState )
{
	return (pRenderState->m_Flags & SHADER_OPACITY_ALPHATEST) != 0;
}


//-----------------------------------------------------------------------------
// The shader system (a singleton)
//-----------------------------------------------------------------------------
class IShaderSystemInternal : public IShaderInit, public IShaderSystem
{
public:
	// Initialization, shutdown
	virtual void		Init() = 0;
	virtual void		Shutdown() = 0;

	// Methods related to reading in shader DLLs
	virtual bool		LoadShaderDLL( const char *pFullPath ) = 0;
	virtual void		UnloadShaderDLL( const char *pFullPath ) = 0;

	// Find me a shader!
	virtual IShader*	FindShader( char const* pShaderName ) = 0;

	// returns various vertex formats
	virtual VertexFormat_t GetVertexFormat( MaterialVertexFormat_t vfmt ) = 0;

	// returns strings associated with the shader state flags...
	virtual char const* ShaderStateString( int i ) = 0;

	// Rendering related methods

	// Create debugging materials
	virtual void CreateDebugMaterials() = 0;

	// Cleans up the debugging materials
	virtual void CleanUpDebugMaterials() = 0;

	// Call the SHADER_PARAM_INIT block of the shaders
	virtual void InitShaderParameters( IShader *pShader, IMaterialVar **params, const char *pMaterialName ) = 0;

	// Call the SHADER_INIT block of the shaders
	virtual void InitShaderInstance( IShader *pShader, IMaterialVar **params, const char *pMaterialName ) = 0;

	// go through each param and make sure it is the right type, load textures, 
	// compute state snapshots and vertex types, etc.
	virtual bool InitRenderState( IShader *pShader, int numParams, IMaterialVar **params, ShaderRenderState_t* pRenderState, char const* pMaterialName ) = 0;

	// When you're done with the shader, be sure to call this to clean up
	virtual void CleanupRenderState( ShaderRenderState_t* pRenderState ) = 0;

	// Draws the shader
	virtual void DrawElements( IShader *pShader, IMaterialVar **params, ShaderRenderState_t* pShaderState ) = 0;
};


#endif // SHADERSYSTEM_H
