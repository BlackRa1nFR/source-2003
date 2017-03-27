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
// Interface to the engine system responsible for dealing with shadows
//=============================================================================

#ifndef ISHADOWMGR_H
#define ISHADOWMGR_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "vmatrix.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class IMaterial;
class Vector;
class Vector2D;
struct model_t;
typedef unsigned short ModelInstanceHandle_t;

// change this when the new version is incompatable with the old
#define ENGINE_SHADOWMGR_INTERFACE_VERSION	"VEngineShadowMgr001"


//-----------------------------------------------------------------------------
//
// Shadow-related functionality exported by the engine
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// This is a handle	to shadows, clients can create as many as they want
//-----------------------------------------------------------------------------
typedef unsigned short ShadowHandle_t;

enum
{
	SHADOW_HANDLE_INVALID = (ShadowHandle_t)~0
};


//-----------------------------------------------------------------------------
// Used for the creation Flags field of CreateShadow
//-----------------------------------------------------------------------------
enum ShadowCreateFlags_t
{
	SHADOW_CACHE_VERTS = 0x1,

	SHADOW_LAST_FLAG = SHADOW_CACHE_VERTS
};


//-----------------------------------------------------------------------------
// Information about a particular shadow
//-----------------------------------------------------------------------------
struct ShadowInfo_t
{
	// Transforms from world space into texture space of the shadow
	VMatrix		m_WorldToShadow;

	// The shadow should no longer be drawn once it's further than MaxDist
	// along z in shadow texture coordinates.
	float			m_FalloffOffset;
	float			m_MaxDist;
	float			m_FalloffAmount;	// how much to lighten the shadow maximally
	Vector2D		m_TexOrigin;
	Vector2D		m_TexSize;
	unsigned char	m_FalloffBias;
};


//-----------------------------------------------------------------------------
// The engine's interface to the shadow manager
//-----------------------------------------------------------------------------
class IShadowMgr
{
public:
	// Create, destroy shadows (see ShadowCreateFlags_t for creationFlags)
	virtual ShadowHandle_t CreateShadow( IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, int creationFlags ) = 0;
	virtual void DestroyShadow( ShadowHandle_t handle ) = 0;

	// Resets the shadow material (useful for shadow LOD.. doing blobby at distance) 
	virtual void SetShadowMaterial( ShadowHandle_t handle, IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy ) = 0;

	// Shadow opacity
//	virtual void SetShadowOpacity( ShadowHandle_t handle, float alpha ) = 0;
//	virtual float GetShadowOpacity( ShadowHandle_t handle ) const = 0;

	// Project a shadow into the world
	// The two points specify the upper left coordinate and the lower-right
	// coordinate of the shadow specified in a shadow "viewplane". The
	// projection matrix is a shadow viewplane->world transformation,
	// and can be orthographic orperspective.

	// I expect that the client DLL will call this method any time the shadow
	// changes because the light changes, or because the entity casting the
	// shadow moves

	// Note that we can't really control the shadows from the engine because
	// the engine only knows about pevs, which don't exist on the client

	// The shadow matrix specifies a world-space transform for the shadow
	// the shadow is projected down the z direction, and the origin of the
	// shadow matrix is the origin of the projection ray. The size indicates
	// the shadow size measured in the space of the shadow matrix; the
	// shadow goes from +/- size.x/2 along the x axis of the shadow matrix
	// and +/- size.y/2 along the y axis of the shadow matrix.
	virtual void ProjectShadow( ShadowHandle_t handle, const Vector& origin,
		const Vector& projectionDir, const VMatrix &worldToShadow, const Vector2D &size,
		float maxDistance, float falloffOffset, float falloffAmount ) = 0;

	// Gets at information about a particular shadow
	virtual const ShadowInfo_t &GetInfo( ShadowHandle_t handle ) = 0;

	// Methods related to shadows on brush models
	virtual void AddShadowToBrushModel( ShadowHandle_t handle, 
		model_t* pModel, const Vector& origin, const QAngle& angles ) = 0;

	// Removes all shadows from a brush model
	virtual void RemoveAllShadowsFromBrushModel( model_t* pModel ) = 0;

	// Sets the texture coordinate range for a shadow...
	virtual void SetShadowTexCoord( ShadowHandle_t handle, float x, float y, float w, float h ) = 0;

	// Methods related to shadows on studio models
	virtual void AddShadowToModel( ShadowHandle_t shadow, ModelInstanceHandle_t instance ) = 0;
	virtual void RemoveAllShadowsFromModel( ModelInstanceHandle_t instance ) = 0;

	// Set extra clip planes related to shadows...
	// These are used to prevent pokethru and back-casting
	virtual void ClearExtraClipPlanes( ShadowHandle_t shadow ) = 0;
	virtual void AddExtraClipPlane( ShadowHandle_t shadow, const Vector& normal, float dist ) = 0;

	// Allows us to disable particular shadows
	virtual void EnableShadow( ShadowHandle_t shadow, bool bEnable ) = 0;

	// Set the darkness falloff bias
	virtual void SetFalloffBias( ShadowHandle_t shadow, unsigned char ucBias ) = 0;
};


#endif
