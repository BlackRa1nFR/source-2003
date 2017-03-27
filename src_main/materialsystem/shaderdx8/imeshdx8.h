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

#ifndef IMESHDX8_H
#define IMESHDX8_H

#ifdef _WIN32
#pragma once
#endif


#include "materialsystem/IMesh.h"
#include "ShaderAPI.h"

class IMeshDX8 : public IMesh
{
public:
	// Begins a pass
	virtual void BeginPass( ) = 0;

	// Draws a single pass of the mesh
	virtual void RenderPass() = 0;

	// returns the vertex format
	virtual VertexFormat_t GetVertexFormat() const = 0;

	// Does it have a color mesh?
	virtual bool HasColorMesh() const = 0;
};

class IMeshMgr
{
public:
	// Initialize, shutdown
	virtual void Init() = 0;
	virtual void Shutdown() = 0;

	// Task switch...
	virtual void ReleaseBuffers() = 0;
	virtual void RestoreBuffers() = 0;

	// Releases all dynamic vertex buffers
	virtual void DestroyVertexBuffers() = 0;

	// Flushes the dynamic mesh. Should be called when state changes
	virtual void Flush() = 0;

	// Discards the dynamic vertex and index buffer
	virtual void DiscardVertexBuffers() = 0;

	// Creates, destroys static meshes
	virtual IMesh*	CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh ) = 0;
	virtual IMesh*	CreateStaticMesh( VertexFormat_t vertexFormat, bool bSoftwareVertexShader ) = 0;
	virtual void	DestroyStaticMesh( IMesh* pMesh ) = 0;

	// Gets at the dynamic mesh
	virtual IMesh*	GetDynamicMesh( IMaterial* pMaterial, bool buffered = true,
		IMesh* pVertexOverride = 0, IMesh* pIndexOverride = 0) = 0;

	// Computes the vertex format
	virtual VertexFormat_t ComputeVertexFormat( unsigned int flags, 
			int numTexCoords, int* pTexCoordDimensions, int numBoneWeights,
			int userDataSize ) const = 0;

	// Computes a 'standard' format to use that's at least as large
	// as the passed-in format and is cached aligned
	virtual VertexFormat_t ComputeStandardFormat( VertexFormat_t fmt ) const = 0;

	// Returns the number of buffers...
	virtual int BufferCount() const = 0;
};

#endif // IMESHDX8_H
