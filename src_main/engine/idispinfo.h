//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef IDISPINFO_H
#define IDISPINFO_H
#pragma once

//=============================================================================

#include <assert.h>
#include "bspfile.h"
#include "vmatrix.h"
#include "dispnode.h"
#include "builddisp.h"
#include "utlvector.h"
#include "terrainmod_functions.h"
#include "engine/ishadowmgr.h"
#include "getintersectingsurfaces_struct.h"


struct model_t;
class CDispLeafLink;
struct Ray_t;
struct RayDispOutput_t;
struct decal_t;
class CMeshBuilder;


//-----------------------------------------------------------------------------
// Handle to decals + shadows on displacements
//-----------------------------------------------------------------------------
typedef unsigned short DispDecalHandle_t;

enum
{
	DISP_DECAL_HANDLE_INVALID = (DispDecalHandle_t)~0
};

typedef unsigned short DispShadowHandle_t;

enum
{
	DISP_SHADOW_HANDLE_INVALID = (DispShadowHandle_t)~0
};


//-----------------------------------------------------------------------------
// Displacement interface to the engine (and WorldCraft?)
//-----------------------------------------------------------------------------
class IDispInfo
{
public:

	virtual				~IDispInfo() {}

	// Builds a list of displacement triangles intersecting the sphere.
	virtual void		GetIntersectingSurfaces( GetIntersectingSurfaces_Struct *pStruct ) = 0;

	virtual void		RenderWireframeInLightmapPage( void ) = 0;
	
	virtual void		UpdateLightmapAlpha( Vector4D *pLightmapPage, int maxLMPageLuxels ) = 0;

	virtual void		GetBoundingBox( Vector &bbMin, Vector &bbMax ) = 0;

	// Get and set the parent surfaces.
	virtual void		SetParent( int surfID ) = 0;
	virtual int			GetParent() = 0;

	// Link them into lists.
	virtual void		SetNextInRenderChain( IDispInfo *pDispInfoNext ) = 0;
	virtual IDispInfo*	GetNextInRenderChain() = 0;
	virtual void		SetNextInRayCastChain( IDispInfo *pDispInfoNext ) = 0;
	virtual IDispInfo*	GetNextInRayCastChain() = 0;

	// Modify the terrain in this dispinfo.
	virtual void		ApplyTerrainMod( ITerrainMod *pMod ) = 0;
	
	// Don't modify the terrain but fill in a list of verts that would have been modified.
	// Returns the number of verts filled in.
	virtual int			ApplyTerrainMod_Speculative( 
		ITerrainMod *pMod,
		CSpeculativeTerrainModVert *pSpeculativeVerts,
		int nMaxVerts ) = 0;

	// Add dynamic lights to the lightmap for this surface.
	virtual void		AddDynamicLights( Vector4D *blocklight ) = 0;

	// Add and remove decals.
	virtual DispDecalHandle_t	NotifyAddDecal( decal_t *pDecal ) = 0;
	virtual void				NotifyRemoveDecal( DispDecalHandle_t h ) = 0;
	virtual DispShadowHandle_t	AddShadowDecal( ShadowHandle_t shadowHandle ) = 0;
	virtual void				RemoveShadowDecal( DispShadowHandle_t handle ) = 0;

	// Compute shadow fragments for a particular shadow, return the vertex + index count of all fragments
	virtual bool		ComputeShadowFragments( DispShadowHandle_t h, int& vertexCount, int& indexCount ) = 0;

	// Tag the surface and check if it's tagged. You can untag all the surfaces
	// with DispInfo_ClearAllTags. Note: it just uses a frame counter to track the
	// tag state so it's really really fast to call ClearAllTags (just increments
	// a variable 99.999% of the time).
	virtual bool		GetTag() = 0;
	virtual void		SetTag() = 0;

	// Used to link the displacement into leaves.
	virtual CDispLeafLink*	&GetLeafLinkHead() = 0;

	// Cast a ray against this surface
	virtual	bool	TestRay( Ray_t const& ray, float start, float end, float& dist, Vector2D* lightmapUV, Vector2D* textureUV ) = 0;

	// Computes the texture + lightmap coordinate given a displacement uv
	virtual void	ComputeLightmapAndTextureCoordinate( RayDispOutput_t const& uv, Vector2D* luv, Vector2D* tuv ) = 0;
};


// ----------------------------------------------------------------------------- //
// Adds shadow rendering data to a particular mesh builder
// The function will return the new base index
// ----------------------------------------------------------------------------- //
int				DispInfo_AddShadowsToMeshBuilder( CMeshBuilder& meshBuilder, 
										DispShadowHandle_t h, int baseIndex );


typedef void* HDISPINFOARRAY;


// Init and shutdown for the material system (references global, materialSystemInterface).
void			DispInfo_InitMaterialSystem();
void			DispInfo_ShutdownMaterialSystem();


// Use these to manage a list of IDispInfos.
HDISPINFOARRAY	DispInfo_CreateArray( int nElements );
void			DispInfo_DeleteArray( HDISPINFOARRAY hArray );
IDispInfo*		DispInfo_IndexArray( HDISPINFOARRAY hArray, int iElement );
int				DispInfo_ComputeIndex( HDISPINFOARRAY hArray, IDispInfo* pInfo );

// Clear the tags for all displacements in the array.
void			DispInfo_ClearAllTags( HDISPINFOARRAY hArray );


// Call this to render a list of displacements.
// If bOrtho is true, then no backface removal is done on dispinfos.
void			DispInfo_RenderChain( IDispInfo *pHead, bool bOrtho );


// This should be called from Map_LoadDisplacements (while the map file is open).
// It loads the displacement data from the file and prepares the displacements for rendering.
//
// bRestoring is set to true when just restoring the data from the mapfile
// (ie: displacements already are initialized but need new static buffers).
bool			DispInfo_LoadDisplacements( model_t *pWorld, bool bRestoring );

// Deletes all the static vertex buffers.
void			DispInfo_ReleaseMaterialSystemObjects( model_t *pWorld );


#endif // IDISPINFO_H
