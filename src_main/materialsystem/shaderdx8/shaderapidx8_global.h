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
// Contains globals for the DX8 implementation of the shader API
//=============================================================================

#ifndef SHADERAPIDX8_GLOBAL_H
#define SHADERAPIDX8_GLOBAL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "locald3dtypes.h"

#ifdef _DEBUG
#include "tier0/memalloc.h"
#endif

//-----------------------------------------------------------------------------
// Use this to fill in structures with the current board state
//-----------------------------------------------------------------------------

#ifdef _DEBUG
#define DEBUG_BOARD_STATE 1
#endif


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IShaderUtil;
class IVertexBufferDX8;
class IMaterialSystemHardwareConfig;
class IShaderShadowDX8;
class IMeshMgr;
class IShaderAPIDX8;
class CMaterialSystemStatsDX8;
class IBaseFileSystem;
class IShaderManager;


//-----------------------------------------------------------------------------
// A new shader draw flag we need to workaround dx8
//-----------------------------------------------------------------------------
enum
{
	SHADER_HAS_CONSTANT_COLOR = 0x80000000
};

//-----------------------------------------------------------------------------
// The main Shader util interface
//-----------------------------------------------------------------------------
inline IShaderUtil* ShaderUtil()
{
	extern IShaderUtil* g_pShaderUtil;
	return g_pShaderUtil;
}

//-----------------------------------------------------------------------------
// The main shader API
//-----------------------------------------------------------------------------
inline IShaderAPIDX8* ShaderAPI()
{
	extern IShaderAPIDX8 *g_pShaderAPIDX8;
	return g_pShaderAPIDX8;
}

//-----------------------------------------------------------------------------
// File system
//-----------------------------------------------------------------------------
inline IBaseFileSystem* FileSystem()
{
	extern IBaseFileSystem* g_pFileSystem;
	return g_pFileSystem;
}

//-----------------------------------------------------------------------------
// The shader shadow
//-----------------------------------------------------------------------------
IShaderShadowDX8* ShaderShadow();

//-----------------------------------------------------------------------------
// Manager of all vertex + pixel shaders
//-----------------------------------------------------------------------------
inline IShaderManager *ShaderManager()
{
	extern IShaderManager *g_pShaderManager;
	return g_pShaderManager;
}

//-----------------------------------------------------------------------------
// The mesh manager
//-----------------------------------------------------------------------------
IMeshMgr* MeshMgr();

//-----------------------------------------------------------------------------
// The main hardware config interface
//-----------------------------------------------------------------------------
inline IMaterialSystemHardwareConfig* HardwareConfig()
{	
	extern IMaterialSystemHardwareConfig *g_pHardwareConfig;
	return g_pHardwareConfig;
}

//-----------------------------------------------------------------------------
// Gets at the stats interface
//-----------------------------------------------------------------------------
inline CMaterialSystemStatsDX8* MaterialSystemStats()
{
	extern CMaterialSystemStatsDX8 *g_pStats;
	return g_pStats;
}


//-----------------------------------------------------------------------------
// Gets at the D3DDevice
//-----------------------------------------------------------------------------
IDirect3D* D3D();
IDirect3DDevice* D3DDevice();

//-----------------------------------------------------------------------------
// Memory debugging 
//-----------------------------------------------------------------------------
#ifdef _DEBUG
#define BEGIN_D3DX_ALLOCATION()	g_pMemAlloc->PushAllocDbgInfo("D3DX", 0)
#define END_D3DX_ALLOCATION()	g_pMemAlloc->PopAllocDbgInfo()
#else
#define BEGIN_D3DX_ALLOCATION()	0
#define END_D3DX_ALLOCATION()	0
#endif

extern bool g_HasColorMesh; // garymcthack

#endif // SHADERAPIDX8_GLOBAL_H
