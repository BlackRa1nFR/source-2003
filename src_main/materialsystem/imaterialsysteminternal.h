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
// implementational material system interface
//=============================================================================

#ifndef IMATERIALSYSTEMINTERNAL_H
#define IMATERIALSYSTEMINTERNAL_H

#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/imaterialsystem.h"


class IMaterialInternal;


//-----------------------------------------------------------------------------
// Additional interfaces used internally to the library
//-----------------------------------------------------------------------------
class IMaterialSystemInternal : public IMaterialSystem
{
public:
	// Sets the default state in the currently selected shader
	virtual void SetDefaultState( ) = 0;

	// Returns the current material
	virtual IMaterial* GetCurrentMaterial() = 0;

	virtual int GetLightmapPage( void ) = 0;

	// Gets the maximum lightmap page size...
	virtual int GetLightmapWidth( int lightmap ) const = 0;
	virtual int GetLightmapHeight( int lightmap ) const = 0;

	virtual ITexture *GetLocalCubemap( void ) = 0;

//	virtual bool RenderZOnlyWithHeightClipEnabled( void ) = 0;
	virtual void ForceDepthFuncEquals( bool bEnable ) = 0;
	virtual enum MaterialHeightClipMode_t GetHeightClipMode( void ) = 0;

	// FIXME: Remove? Here for debugging shaders in CShaderSystem
	virtual void AddMaterialToMaterialList( IMaterialInternal *pMaterial ) = 0;
	virtual void RemoveMaterial( IMaterialInternal* pMaterial ) = 0;
};

#endif // IMATERIALSYSTEMINTERNAL_H
