//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef C_POINTCAMERA_H
#define C_POINTCAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"

class C_PointCamera : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_PointCamera, C_BaseEntity );
	DECLARE_CLIENTCLASS();

public:
	C_PointCamera();

	bool IsActive();
	
	// C_BaseEntity.
	virtual bool	ShouldDraw();

	float			GetFOV();
	float			GetResolution();

private:
	float m_FOV;
	float m_Resolution;
	bool m_bActive;
};

#endif // C_POINTCAMERA_H
