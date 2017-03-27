//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//=============================================================================

#if !defined( ICLIENTLEAFSYSTEM_H )
#define ICLIENTLEAFSYSTEM_H
#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Foward declarations
//-----------------------------------------------------------------------------
class IClientRenderable;


//-----------------------------------------------------------------------------
// Handle to an renderable in the client leaf system
//-----------------------------------------------------------------------------
typedef unsigned short ClientRenderHandle_t;

enum
{
	INVALID_CLIENT_RENDER_HANDLE = (ClientRenderHandle_t)0xffff,
};


//-----------------------------------------------------------------------------
// Render groups
//-----------------------------------------------------------------------------
enum RenderGroup_t
{
	RENDER_GROUP_WORLD = 0,
	RENDER_GROUP_OPAQUE_ENTITY,
	RENDER_GROUP_TRANSLUCENT_ENTITY,
	RENDER_GROUP_TWOPASS, // Implied opaque and translucent in two passes
	RENDER_GROUP_VIEW_MODEL,
	RENDER_GROUP_OTHER,	// Unclassfied. Won't get drawn.

	// This one's always gotta be last
	RENDER_GROUP_COUNT
};


#define CLIENTLEAFSYSTEM_INTERFACE_VERSION	"ClientLeafSystem001"


//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
class IClientLeafSystemEngine
{
public:
	// Adds and removes renderables from the leaf lists
	virtual ClientRenderHandle_t CreateRenderableHandle( IClientRenderable* pRenderable, bool bIsStaticProp = false ) = 0;
	virtual void RemoveRenderable( ClientRenderHandle_t handle ) = 0;
	virtual void AddRenderableToLeaves( ClientRenderHandle_t renderable, int nLeafCount, unsigned short *pLeaves ) = 0;
	virtual void ChangeRenderableRenderGroup( ClientRenderHandle_t handle, RenderGroup_t group ) = 0;
};


#endif	// ICLIENTLEAFSYSTEM_H


