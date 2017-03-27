//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CAMERA_H
#define CAMERA_H
#ifdef _WIN32
#pragma once
#endif


#include "worldcraft_mathlib.h"


#define OCTANT_X_POSITIVE		1
#define OCTANT_Y_POSITIVE		2
#define OCTANT_Z_POSITIVE		4


//
// Return values from BoxIsVisible.
//
enum Visibility_t
{
	VIS_NONE = 0,		// The box is completely outside the view frustum.
	VIS_PARTIAL,		// The box is partially inside the view frustum.
	VIS_TOTAL			// The box is completely inside the view frustum.
};


class CCamera  
{
	public:

		CCamera(void);
		virtual ~CCamera(void);

		void MoveAbsX(float fUnits);
		void MoveAbsY(float fUnits);
		void MoveAbsZ(float fUnits);

		void MoveForward(float fUnits);
		void MoveRight(float fUnits);
		void MoveUp(float fUnits);
		void GetMoveForward(Vector& fNewViewPoint, float fUnits);

		void Pitch(float fDegrees);
		void Roll(float fDegrees);
		void Yaw(float fDegrees);

		void GetViewPoint(Vector& fViewPoint) const;
		void GetViewTarget(Vector& fViewTarget);
		void GetViewForward(Vector& ViewForward) const;
		void GetViewUp(Vector& ViewUp) const;
		void GetViewRight(Vector& ViewRight) const;
		int GetViewOctant(void);
		void GetViewMatrix(matrix4_t& Matrix);

		float GetYaw(void);
		float GetPitch(void);
		float GetRoll(void);

		float GetHorizontalFOV(void);
		float GetVerticalFOV(void);
		float GetNearClip(void);
		float GetFarClip(void);

		void SetYaw(float fDegrees);
		void SetPitch(float fDegrees);
		void SetRoll(float fDegrees);

		void SetViewPoint(const Vector &ViewPoint);
		void SetViewTarget(const Vector &ViewTarget);

		void SetFarClip(float fFarZ);
		void SetNearClip(float fNearZ);
		void SetFrustum(float fHorizontalFOV, float fVerticalFOV, float fNearZ, float fFarZ);

        void SetZoom(float fScale);
        void Zoom(float fScale);
        float GetZoom(void);

protected:

		void BuildViewMatrix(void);


		float m_fYaw;				// Counterclockwise rotation around the CAMERA_UP axis, in degrees [-359, 359].
		float m_fPitch;				// Counterclockwise rotation around the CAMERA_RIGHT axis, in degrees [-90, 90].
		float m_fRoll;				// Counterclockwise rotation around the CAMERA_FORWARD axis, in degrees [-359, 359].

		Vector m_ViewPoint;			// Camera location in world coordinates.
		matrix4_t m_ViewMatrix;		// Camera view matrix, based on current yaw, pitch, and roll.

		float m_fHorizontalFOV;		// Horizontal field of view in degrees.
		float m_fVerticalFOV;		// Vertical field of view in degrees.
		float m_fNearClip;			// Distance to near clipping plane.
		float m_fFarClip;			// Distance to far clipping plane.

        float m_fZoomScale;         // Orthographic zoom scale
};


#endif // CAMERA_H
