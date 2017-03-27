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

#ifndef SHADOWMGR_H
#define SHADOWMGR_H

#ifdef _WIN32
#pragma once
#endif

#include "engine/ishadowmgr.h"
#include "vmatrix.h"

//-----------------------------------------------------------------------------
// Shadow decals are applied to a single surface
//-----------------------------------------------------------------------------
typedef unsigned short ShadowDecalHandle_t;

enum
{
	SHADOW_DECAL_HANDLE_INVALID = (ShadowDecalHandle_t)~0
};


//-----------------------------------------------------------------------------
// This structure contains the vertex information for shadows
//-----------------------------------------------------------------------------
struct ShadowVertex_t
{
	Vector	m_Position;
	Vector	m_TexCoord;	// Stored as 3D to help compute the fade amount
};


//-----------------------------------------------------------------------------
// Shadow-related functionality internal to the engine
//-----------------------------------------------------------------------------
class IShadowMgrInternal : public IShadowMgr
{
public:
	// This surface has been rendered; and we'll want to render the shadows
	// on this surface
	virtual void AddShadowsOnSurfaceToRenderList( ShadowDecalHandle_t decalHandle ) = 0;

	// This will render all shadows that were previously added using
	// AddShadowsOnSurfaceToRenderList. If there's a model to world transform
	// for the shadow receiver, then send it in!
	virtual void RenderShadows( VMatrix const* pModelToWorld = 0 ) = 0;

	// Clears the list of shadows to render 
	virtual void ClearShadowRenderList() = 0;

	// Projects + clips shadows
	// count + ppPosition describe an array of pointers to vertex positions
	// of the unclipped shadow
	// ppOutVertex is pointed to the head of an array of pointers to
	// clipped vertices the function returns the number of clipped vertices
	virtual int ProjectAndClipVertices( ShadowHandle_t handle, int count, 
		Vector** ppPosition, ShadowVertex_t*** ppOutVertex ) = 0;

	// Compute darkness when rendering
	virtual unsigned char ComputeDarkness( float z, ShadowInfo_t const& info ) const = 0;

	// Shadow state...
	virtual unsigned short InvalidShadowIndex( ) = 0;
	virtual void SetModelShadowState( ModelInstanceHandle_t instance ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
extern IShadowMgrInternal* g_pShadowMgr;

#endif
