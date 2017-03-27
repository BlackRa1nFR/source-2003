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
//=============================================================================

#ifndef MATERIALSYSTEM_GLOBAL_H
#define MATERIALSYSTEM_GLOBAL_H

#ifdef _WIN32
#pragma once
#endif

#include "imaterialsysteminternal.h"
#include "tier0/dbg.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ITextureInternal;
class IShaderAPI;
class IHardwareConfigInternal;
class IShaderUtil;
class IShaderShadow;
class IFileSystem;
class IShaderSystemInternal;


//-----------------------------------------------------------------------------
// Constants used by the system
//-----------------------------------------------------------------------------

#define MATERIAL_MAX_PATH 256

// GR - limits for blured image (HDR stuff)
#define MAX_BLUR_IMAGE_WIDTH  256
#define MAX_BLUR_IMAGE_HEIGHT 192

#define CLAMP_BLUR_IMAGE_WIDTH( _w ) ( ( _w < MAX_BLUR_IMAGE_WIDTH ) ? _w : MAX_BLUR_IMAGE_WIDTH )
#define CLAMP_BLUR_IMAGE_HEIGHT( _h ) ( ( _h < MAX_BLUR_IMAGE_HEIGHT ) ? _h : MAX_BLUR_IMAGE_HEIGHT )

//-----------------------------------------------------------------------------
// Global structures
//-----------------------------------------------------------------------------
extern MaterialSystem_Config_t g_config;

//extern MaterialSystem_ErrorFunc_t	Error;
//extern MaterialSystem_WarningFunc_t Warning;

extern float			g_ooOverbright;
extern int				g_FrameNum;

extern IShaderAPI*	g_pShaderAPI;
extern IShaderShadow* g_pShaderShadow;
extern IFileSystem *g_pFileSystem;

IShaderSystemInternal* ShaderSystem();
inline IShaderSystemInternal* ShaderSystem()
{
	extern IShaderSystemInternal *g_pShaderSystem;
	return g_pShaderSystem;
}

inline IHardwareConfigInternal *HardwareConfig()
{
	extern IHardwareConfigInternal* g_pHWConfig;
	return g_pHWConfig;
}

//-----------------------------------------------------------------------------
// Accessor to get at the material system
//-----------------------------------------------------------------------------
IMaterialSystemInternal* MaterialSystem();
IShaderUtil* ShaderUtil();


#endif // MATERIALSYSTEM_GLOBAL_H
