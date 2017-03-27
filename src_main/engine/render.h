//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Revision: $
// $NoKeywords: $
//
//=============================================================================

#ifndef RENDER_H
#define RENDER_H
#pragma once

#include "packed_entity.h"
#include "vector.h"
#include "interface.h"

// render.h -- public interface to refresh functions

#ifndef PROGS_H
#include "progs.h"
#endif

extern float r_blend; // Global blending factor for the current entity
extern float r_colormod[3]; // Global color modulation for the current entity

//
// refresh
//
extern Vector	r_origin;
extern Vector vpn, vright, vup;

void R_Init (void);
void R_NewMap (void);

#include "view_shared.h"
#include "ivrenderview.h"

class VMatrix;

class IRender
{
public:
	virtual void	FrameBegin( void ) = 0;
	virtual void	FrameEnd( void ) = 0;
	
	virtual void	ViewSetupVis( bool novis, int numorigins, const Vector origin[] ) = 0;

	virtual void	ViewSetup3D( const CViewSetup *pView, Frustum frustumPlanes ) = 0;
	virtual void	ViewDrawFade( byte *color, IMaterial* pFadeMaterial ) = 0;

	virtual void	DrawSceneBegin( void ) = 0;
	virtual void	DrawSceneEnd( void ) = 0;

	virtual void	SetProjectionMatrix( float fov, float zNear, float zFar, bool forceAspectRatio1To1 = false ) = 0;

	virtual void	BuildWorldLists( WorldListInfo_t* pInfo, bool updateLightmaps, int iForceViewLeaf ) = 0;
	virtual void	DrawWorldLists( unsigned long flags ) = 0;

	// UNDONE: these are temporary functions that will end up on the other
	// side of this interface
	// accessors
	virtual const Vector& UnreflectedViewOrigin() = 0;
	virtual const Vector& ViewOrigin( void ) = 0;
	virtual const QAngle& ViewAngles( void ) = 0;
	virtual CViewSetup const &ViewGetCurrent( void ) = 0;
	virtual const VMatrix& ViewMatrix( void ) = 0;
	virtual const VMatrix& WorldToScreenMatrix( void ) = 0;
	virtual float	GetFramerate( void ) = 0;
	virtual float	GetZNear( void ) = 0;
	virtual float	GetZFar( void ) = 0;

	// Query current fov and view model fov
	virtual float	GetFov( void ) = 0;
	virtual float	GetFovY( void ) = 0;
	virtual float	GetFovViewmodel( void ) = 0;

	// Compute the number of pixels in screen space wide a particular sphere is 
	virtual float	GetScreenSize( const Vector& origin, float radius ) = 0;

	// Compute the clip-space coordinates of a point in 3D
	// Clip-space is normalized screen coordinates (-1 to 1 in x and y)
	// Returns true if the point is behind the camera
	virtual bool	ClipTransform( Vector const& point, Vector* pClip ) = 0;

	// Compute the screen-space coordinates of a point in 3D
	// This returns actual pixels
	// Returns true if the point is behind the camera
	virtual bool	ScreenTransform( Vector const& point, Vector* pScreen ) = 0;
};

void R_PushDlights (void);

// UNDONE Remove this - pass this around to functions/systems that need it.
extern IRender *g_EngineRenderer;
#endif			// RENDER_H
