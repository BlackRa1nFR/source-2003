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
// Internal interface for a material
//=============================================================================

#ifndef IMATERIALINTERNAL_H
#define IMATERIALINTERNAL_H

#ifdef _WIN32
#pragma once
#endif

// identifier was truncated to '255' characters in the debug information
#pragma warning(disable: 4786)

#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "shaderapi.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
enum MaterialPrimitiveType_t;
class IShader;
class IMesh;
struct Shader_VertexArrayData_t;
struct ShaderRenderState_t;


//-----------------------------------------------------------------------------
// Interface for materials used only within the material system
//-----------------------------------------------------------------------------
class IMaterialInternal : public IMaterial
{
public:
	// class factory methods
	static IMaterialInternal* Create( char const* pMaterialName, bool bCreatedFromFile = true );
	static void Destroy( IMaterialInternal* pMaterial );

	// refcount
	virtual int		GetReferenceCount( ) const = 0;

	// enumeration id
	virtual void	SetEnumerationID( int id ) = 0;

	// White lightmap methods
	virtual void	SetNeedsWhiteLightmap( bool val ) = 0;
	virtual bool	GetNeedsWhiteLightmap( ) const = 0;

	// load/unload 
	virtual void	Uncache(  bool bForceUncacheVars = false ) = 0;
	virtual void	Precache() = 0;

	// lightmap pages associated with this material
	virtual void	SetMinLightmapPageID( int pageID ) = 0;
	virtual void	SetMaxLightmapPageID( int pageID ) = 0;;
	virtual int		GetMinLightmapPageID( ) const = 0;
	virtual int		GetMaxLightmapPageID( ) const = 0;

	virtual IShader *GetShader() const = 0;

	// Can we use it?
	virtual bool	IsPrecached( ) const = 0;

	// main draw method
	virtual void	DrawMesh( IMesh* pMesh ) = 0;

	// proxy methods
	virtual bool	HasProxy( ) const = 0;
	virtual void	CallBindProxy( void *proxyData ) = 0;

	// Gets the vertex format
	virtual VertexFormat_t GetVertexFormat() const = 0;
	virtual int GetVertexUsage() const = 0;

	// Performs a debug trace on this material
	virtual bool PerformDebugTrace() const = 0;

	// Can we override this material in debug?
	virtual bool NoDebugOverride() const = 0;

	// Should we draw?
	virtual void ToggleSuppression() = 0;

	// Are we suppressed?
	virtual bool IsSuppressed() const = 0;

	// Should we debug?
	virtual void ToggleDebugTrace() = 0;

	// Do we use fog?
	virtual bool UseFog() const = 0;
	
	// Get the software vertex shader from this material's shader if it is necessary.
	virtual const SoftwareVertexShader_t GetSoftwareVertexShader() const = 0;

	// returns true if there is a software vertex shader.  Software vertex shaders are
	// different from "WillPreprocessData" in that all the work is done on the verts
	// before calling the shader instead of between passes.  Software vertex shaders are
	// preferred when possible.
	virtual bool UsesSoftwareVertexShader( void ) const = 0;

	// Adds a material variable to the material
	virtual void AddMaterialVar( IMaterialVar *pMaterialVar ) = 0;

	// Gets the renderstate
	virtual ShaderRenderState_t *GetRenderState() = 0;
};

#endif // IMATERIALINTERNAL_H
