/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef IVMODELRENDER_H
#define IVMODELRENDER_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "mathlib.h"
#include "istudiorender.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
struct mstudioanimdesc_t;
struct mstudioseqdesc_t;
struct model_t;
class IClientRenderable;
class Vector;
struct studiohdr_t;
class IMaterial;

FORWARD_DECLARE_HANDLE( LightCacheHandle_t ); 


//-----------------------------------------------------------------------------
// Model Rendering + instance data
//-----------------------------------------------------------------------------

// change this when the new version is incompatable with the old
#define VENGINE_HUDMODEL_INTERFACE_VERSION	"VEngineModel006"

typedef unsigned short ModelInstanceHandle_t;

enum
{
	MODEL_INSTANCE_INVALID = (ModelInstanceHandle_t)~0
};

// UNDONE: Move this to hud export code, subsume previous functions
class IVModelRender
{
public:
	virtual int		DrawModel(	int flags,
								IClientRenderable *cliententity,
								ModelInstanceHandle_t instance,
								int entity_index, 
								const model_t *model, 
								Vector const& origin, 
								QAngle const& angles, 
								int sequence, 
								int skin,
								int body,
								int hitboxset,
								const Vector *bboxMins = NULL,
								const Vector *bboxMaxs = NULL,
								const matrix3x4_t *modelToWorld = NULL ) = 0;

	// This causes a material to be used when rendering the model instead 
	// of the materials the model was compiled with
	virtual void	ForcedMaterialOverride( IMaterial *newMaterial ) = 0;

	virtual void	SetViewTarget( const Vector& target ) = 0;
	virtual void	SetFlexWeights( const float *weights, int numweights ) = 0;

	virtual matrix3x4_t* pBoneToWorld(int i) = 0;
	virtual matrix3x4_t* pBoneToWorldArray() = 0;
	
	// Creates, destroys instance data to be associated with the model
	virtual ModelInstanceHandle_t CreateInstance( IClientRenderable *pRenderable, bool bStaticProp = false ) = 0;
	virtual void DestroyInstance( ModelInstanceHandle_t handle ) = 0;

	// Creates a decal on a model instance by doing a planar projection
	// along the ray. The material is the decal material, the radius is the
	// radius of the decal to create.
	virtual void AddDecal( ModelInstanceHandle_t handle, Ray_t const& ray, 
		Vector const& decalUp, int decalIndex, int body, bool noPokeThru = false, int maxLODToDecal = ADDDECAL_TO_ALL_LODS ) = 0;

	// Removes all the decals on a model instance
	virtual void RemoveAllDecals( ModelInstanceHandle_t handle ) = 0;

	// Shadow rendering
	virtual void DrawModelShadow( IClientRenderable *pRenderable, int body ) = 0;

	// Lighting origin override... (NULL means use normal model origin)
	virtual void UseLightcache( LightCacheHandle_t* pHandle = NULL ) = 0;

	// This gets called when overbright, etc gets changed to recompute static prop lighting.
	virtual void RecomputeStaticLighting( IClientRenderable *pRenderable, ModelInstanceHandle_t handle ) = 0;
};


#endif // IVMODELRENDER_H
