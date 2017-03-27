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
// Interface to the client system responsible for dealing with shadows
//=============================================================================

#ifndef ICLIENTSHADOWMGR_H
#define ICLIENTSHADOWMGR_H

#ifdef _WIN32
#pragma once
#endif

#include "IGameSystem.h"
#include "IClientEntityInternal.h"


//-----------------------------------------------------------------------------
// Handles to a client shadow
//-----------------------------------------------------------------------------
typedef unsigned short ClientShadowHandle_t;

enum
{
	CLIENTSHADOW_INVALID_HANDLE = (ClientShadowHandle_t)~0
};


//-----------------------------------------------------------------------------
// Handles to a client shadow
//-----------------------------------------------------------------------------
enum ShadowReceiver_t
{
	SHADOW_RECEIVER_BRUSH_MODEL = 0,
	SHADOW_RECEIVER_STATIC_PROP,
};


//-----------------------------------------------------------------------------
// Flags for the creation method
//-----------------------------------------------------------------------------
enum ShadowFlags_t
{
	SHADOW_FLAGS_USE_RENDER_TO_TEXTURE	= (1 << 0),
	SHADOW_FLAGS_ANIMATING_SOURCE		= (1 << 1),

	// Update this if you add flags
	SHADOW_FLAGS_LAST_FLAG				= SHADOW_FLAGS_ANIMATING_SOURCE
};


//-----------------------------------------------------------------------------
// The class responsible for dealing with shadows on the client side
//-----------------------------------------------------------------------------
class IClientShadowMgr : public IGameSystem
{
public:
	// Create, destroy shadows
	virtual ClientShadowHandle_t CreateShadow( ClientEntityHandle_t entity, int flags ) = 0;
	virtual void DestroyShadow( ClientShadowHandle_t handle ) = 0;

	// Indicate that the shadow should be recomputed due to a change in
	// the client entity
	virtual void UpdateShadow( ClientShadowHandle_t handle, bool force = false ) = 0;

	// deals with shadows being added to shadow receivers
	virtual void AddShadowToReceiver( ClientShadowHandle_t handle,
		IClientRenderable* pRenderable, ShadowReceiver_t type ) = 0;

	virtual void RemoveAllShadowsFromReceiver( 
		IClientRenderable* pRenderable, ShadowReceiver_t type ) = 0;

	// Re-renders all shadow textures for shadow casters that lie in the leaf list
	virtual void ComputeShadowTextures( const CViewSetup *pView, int leafCount, int* pLeafList ) = 0;
	
	// Renders the shadow texture to screen...
	virtual void RenderShadowTexture( int w, int h ) = 0;

	// Sets the shadow direction + color
	virtual void SetShadowDirection( const Vector& dir ) = 0;
	virtual void SetShadowColor( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void SetShadowDistance( float flMaxDistance ) = 0;
	virtual void SetShadowBlobbyCutoffArea( float flMinArea ) = 0;
	virtual void SetFalloffBias( ClientShadowHandle_t handle, unsigned char ucBias ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
extern IClientShadowMgr* g_pClientShadowMgr;

#endif // ICLIENTSHADOWMGR_H
