//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
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
#if !defined( VIEW_SHARED_H )
#define VIEW_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// renderer interface
class CViewSetup
{
public:
	CViewSetup()
	{
		m_bForceAspectRatio1To1 = false;
	}
	// shared by 2D & 3D views
	int			context;			// User specified context

	int			x;					// left side of view window
	int			y;					// top side of view window
	int			width;				// width of view window
	int			height;				// height of view window

	bool		clearColor;			// clear the color buffer before rendering this view?
	bool		clearDepth;			// clear the Depth buffer before rendering this view?

	// the rest are only used by 3D views
	bool		m_bOrtho;			// Orthographic projection?
	float		m_OrthoLeft;		// View-space rectangle for ortho projection.
	float		m_OrthoTop;
	float		m_OrthoRight;
	float		m_OrthoBottom;

	float		fov;				// horizontal FOV in degrees
	float		fovViewmodel;		// horizontal FOV in degrees for in-view model

	Vector		origin;					// 3D origin of camera
	Vector		m_vUnreflectedOrigin;	// Origin gets reflected on the water surface, but things like
										// displacement LOD need to be calculated from the viewer's 
										// real position.																				
	
	QAngle		angles;				// heading of camera (pitch, yaw, roll)
	float		zNear;				// local Z coordinate of near plane of camera
	float		zFar;				// local Z coordinate of far plane of camera

	float		zNearViewmodel;		// local Z coordinate of near plane of camera ( when rendering view model )
	float		zFarViewmodel;		// local Z coordinate of far plane of camera ( when rendering view model )

	bool		m_bForceAspectRatio1To1;
};

#endif // VIEW_SHARED_H