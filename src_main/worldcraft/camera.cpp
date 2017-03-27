//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a camera for the 3D view.
//
// $NoKeywords: $
//=============================================================================

#include <windows.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "Camera.h"
#include "worldcraft_mathlib.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//
// Indices of camera axes.
//
#define CAMERA_RIGHT		0
#define CAMERA_UP			1
#define CAMERA_FORWARD		2

#define MIN_PITCH		-90.0f
#define MAX_PITCH		90.0f


static void DBG(char *fmt, ...)
{
    char ach[128];
    va_list va;

    va_start(va, fmt);
    vsprintf(ach, fmt, va);
    va_end(va);
    OutputDebugString(ach);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CCamera::CCamera(void)
{
	ZeroVector(m_ViewPoint);

	m_fPitch = 0.0;
	m_fRoll = 0.0;
	m_fYaw = 0.0;

	m_fHorizontalFOV = 90;
	m_fVerticalFOV = 60;
	m_fNearClip = 1.0;
	m_fFarClip = 5000;

    m_fZoomScale = 1.0f;

	BuildViewMatrix();
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CCamera::~CCamera(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current value of the camera's pitch.
//-----------------------------------------------------------------------------
float CCamera::GetPitch(void)
{
	return(m_fPitch);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current value of the camera's roll.
//-----------------------------------------------------------------------------
float CCamera::GetRoll(void)
{
	return(m_fRoll);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the camera's viewpoint.
//-----------------------------------------------------------------------------
void CCamera::GetViewPoint(Vector& ViewPoint) const
{
	ViewPoint = m_ViewPoint;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current value of the camera's yaw.
//-----------------------------------------------------------------------------
float CCamera::GetYaw(void)
{
	return(m_fYaw);
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera along the world's X axis.
// Input  : fUnits - World units to move the camera.
//-----------------------------------------------------------------------------
void CCamera::MoveAbsX(float fUnits)
{
	if (fUnits != 0)
	{
		m_ViewPoint[0] = (m_ViewPoint[0] + fUnits);
		BuildViewMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera along the world's Y axis.
// Input  : fUnits - World units to move the camera.
//-----------------------------------------------------------------------------
void CCamera::MoveAbsY(float fUnits)
{
	if (fUnits != 0)
	{
		m_ViewPoint[1] = (m_ViewPoint[1] + fUnits);
		BuildViewMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera along the world's Z axis.
// Input  : fUnits - World units to move the camera.
//-----------------------------------------------------------------------------
void CCamera::MoveAbsZ(float fUnits)
{
	if (fUnits != 0)
	{
		m_ViewPoint[2] = (m_ViewPoint[2] + fUnits);
		BuildViewMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera along the camera's right axis.
// Input  : fUnits - World units to move the camera.
//-----------------------------------------------------------------------------
void CCamera::MoveRight(float fUnits)
{
	if (fUnits != 0)
	{
		m_ViewPoint[0] = m_ViewPoint[0] + m_ViewMatrix[CAMERA_RIGHT][0] * fUnits;
		m_ViewPoint[1] = m_ViewPoint[1] + m_ViewMatrix[CAMERA_RIGHT][1] * fUnits;
		m_ViewPoint[2] = m_ViewPoint[2] + m_ViewMatrix[CAMERA_RIGHT][2] * fUnits;
		BuildViewMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera along the camera's up axis.
// Input  : fUnits - World units to move the camera.
//-----------------------------------------------------------------------------
void CCamera::MoveUp(float fUnits)
{
	if (fUnits != 0)
	{
		m_ViewPoint[0] = m_ViewPoint[0] + m_ViewMatrix[CAMERA_UP][0] * fUnits;
		m_ViewPoint[1] = m_ViewPoint[1] + m_ViewMatrix[CAMERA_UP][1] * fUnits;
		m_ViewPoint[2] = m_ViewPoint[2] + m_ViewMatrix[CAMERA_UP][2] * fUnits;
		BuildViewMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera along the camera's forward axis.
// Input  : fUnits - World units to move the camera.
//-----------------------------------------------------------------------------
void CCamera::MoveForward(float fUnits)
{
	if (fUnits != 0)
	{
		m_ViewPoint[0] = m_ViewPoint[0] - m_ViewMatrix[CAMERA_FORWARD][0] * fUnits;
		m_ViewPoint[1] = m_ViewPoint[1] - m_ViewMatrix[CAMERA_FORWARD][1] * fUnits;
		m_ViewPoint[2] = m_ViewPoint[2] - m_ViewMatrix[CAMERA_FORWARD][2] * fUnits;
		BuildViewMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Calculates a position along the camera's forward axis.
// Input  : NewViewPoint - Where the camera would be if it moved the given number
//				of units along its forward axis.
//			fUnits - World units along the forward axis.
//-----------------------------------------------------------------------------
void CCamera::GetMoveForward(Vector& NewViewPoint, float fUnits)
{
	if (fUnits != 0)
	{
		NewViewPoint[0] = m_ViewPoint[0] - m_ViewMatrix[CAMERA_FORWARD][0] * fUnits;
		NewViewPoint[1] = m_ViewPoint[1] - m_ViewMatrix[CAMERA_FORWARD][1] * fUnits;
		NewViewPoint[2] = m_ViewPoint[2] - m_ViewMatrix[CAMERA_FORWARD][2] * fUnits;
	}
	else
	{
		NewViewPoint = m_ViewPoint;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns a vector indicating the camera's forward axis.
//-----------------------------------------------------------------------------
void CCamera::GetViewForward(Vector& ViewForward) const
{
	ViewForward[0] = -m_ViewMatrix[CAMERA_FORWARD][0];
	ViewForward[1] = -m_ViewMatrix[CAMERA_FORWARD][1];
	ViewForward[2] = -m_ViewMatrix[CAMERA_FORWARD][2];
}


//-----------------------------------------------------------------------------
// Purpose: Returns a vector indicating the camera's up axis.
//-----------------------------------------------------------------------------
void CCamera::GetViewUp(Vector& ViewUp) const
{
	ViewUp[0] = m_ViewMatrix[CAMERA_UP][0];
	ViewUp[1] = m_ViewMatrix[CAMERA_UP][1];
	ViewUp[2] = m_ViewMatrix[CAMERA_UP][2];
}


//-----------------------------------------------------------------------------
// Purpose: Returns a vector indicating the camera's right axis.
//-----------------------------------------------------------------------------
void CCamera::GetViewRight(Vector& ViewRight) const
{
	ViewRight[0] = m_ViewMatrix[CAMERA_RIGHT][0];
	ViewRight[1] = m_ViewMatrix[CAMERA_RIGHT][1];
	ViewRight[2] = m_ViewMatrix[CAMERA_RIGHT][2];
}


//-----------------------------------------------------------------------------
// Purpose: Returns a vector indicating the camera's forward direction.
//-----------------------------------------------------------------------------
void CCamera::GetViewTarget(Vector& ViewTarget)
{
	GetMoveForward(ViewTarget, 1);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the horizontal field of view in degrees.
//-----------------------------------------------------------------------------
float CCamera::GetHorizontalFOV(void)
{
	return(m_fHorizontalFOV);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the vertical field of view in degrees.
//-----------------------------------------------------------------------------
float CCamera::GetVerticalFOV(void)
{
	return(m_fVerticalFOV);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the distance from the camera to the near clipping plane in world units.
//-----------------------------------------------------------------------------
float CCamera::GetNearClip(void)
{
	return(m_fNearClip);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the distance from the camera to the far clipping plane in world units.
//-----------------------------------------------------------------------------
float CCamera::GetFarClip(void)
{
	return(m_fFarClip);
}


//-----------------------------------------------------------------------------
// Purpose: Sets up fields of view & clip plane distances for the view frustum.
// Input  : fHorizontalFOV - 
//			fVerticalFOV - 
//			fNearClip - 
//			fFarClip - 
//-----------------------------------------------------------------------------
void CCamera::SetFrustum(float fHorizontalFOV, float fVerticalFOV, float fNearClip, float fFarClip)
{
	m_fHorizontalFOV = fHorizontalFOV;
	m_fVerticalFOV = fVerticalFOV;
	m_fFarClip = fFarClip;
	m_fNearClip = fNearClip;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the distance from the camera to the near clipping plane in world units.
//-----------------------------------------------------------------------------
void CCamera::SetNearClip(float fNearClip)
{
	m_fNearClip = fNearClip;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the distance from the camera to the far clipping plane in world units.
//-----------------------------------------------------------------------------
void CCamera::SetFarClip(float fFarClip)
{
	m_fFarClip = fFarClip;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the pitch in degrees, from [MIN_PITCH..MAX_PITCH]
//-----------------------------------------------------------------------------
void CCamera::SetPitch(float fDegrees)
{
	if (m_fPitch != fDegrees)
	{
		m_fPitch = fDegrees;

		if (m_fPitch > MAX_PITCH)
		{
			m_fPitch = MAX_PITCH;
		}
		else if (m_fPitch < MIN_PITCH)
		{
			m_fPitch = MIN_PITCH;
		}

		BuildViewMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the roll in degrees, from [0..360)
//-----------------------------------------------------------------------------
void CCamera::SetRoll(float fDegrees)
{
	while (fDegrees >= 360)
	{
		fDegrees -= 360;
	}

	while (fDegrees < 0)
	{
		fDegrees += 360;
	}

	if (m_fRoll != fDegrees)
	{
		m_fRoll = fDegrees;
		BuildViewMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the yaw in degrees, from [0..360)
//-----------------------------------------------------------------------------
void CCamera::SetYaw(float fDegrees)
{
	while (fDegrees >= 360)
	{
		fDegrees -= 360;
	}

	while (fDegrees < 0)
	{
		fDegrees += 360;
	}

	if (m_fYaw != fDegrees)
	{
		m_fYaw = fDegrees;
		BuildViewMatrix();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the view point.
//-----------------------------------------------------------------------------
void CCamera::SetViewPoint(const Vector &ViewPoint)
{
	m_ViewPoint = ViewPoint;
	BuildViewMatrix();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the camera target, rebuilding the view matrix.
// Input  : ViewTarget - the point in world space that the camera should look at.
//-----------------------------------------------------------------------------
void CCamera::SetViewTarget(const Vector &ViewTarget)
{
	Vector ViewForward;
	VectorSubtract(ViewTarget, m_ViewPoint, ViewForward);
	VectorNormalize(ViewForward);

	// dvsFIXME: Replace the crap math below with this good stuff, but the matrix
	//		   math elsewhere must be fixed or it won't work
//	QAngle angles;
//	VectorAngles(ViewForward, angles);
//	SetPitch(angles[PITCH]);
//	SetYaw(angles[YAW]);

	//
	// Calculate yaw.
	//
	float fYaw;

	if (ViewForward[1] != 0)
	{	
		fYaw = RAD2DEG(atan(ViewForward[0] / ViewForward[1]));
		if (ViewForward[1] < 0)
		{
			fYaw += 180;
		}
	}
	else if (ViewForward[0] < 0)
	{
		fYaw = 270;
	}
	else
	{
		fYaw = 90;
	}
	SetYaw(fYaw);

	//
	// Calculate pitch.
	//
	float fPitch;

	if (ViewForward[1] != 0)
	{	
		fPitch = RAD2DEG(atan(ViewForward[2] / ViewForward[1]));
		if (ViewForward[1] > 0)
		{
			fPitch = -fPitch;
		}
	}
	else if (ViewForward[2] < 0)
	{
		fPitch = 90;
	}
	else
	{
		fPitch = -90;
	}
	SetPitch(fPitch);

	BuildViewMatrix();
}


//-----------------------------------------------------------------------------
// Purpose: Pitches the camera forward axis toward the camera's down axis a
//			given number of degrees.
//-----------------------------------------------------------------------------
void CCamera::Pitch(float fDegrees)
{
	if (fDegrees != 0)
	{
		float fPitch = GetPitch();
		fPitch += fDegrees;
		SetPitch(fPitch);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Rolls the camera's right axis toward the camera's up axis a given
//			number of degrees.
//-----------------------------------------------------------------------------
void CCamera::Roll(float fDegrees)
{
	if (fDegrees != 0)
	{
		float fRoll = GetRoll();
		fRoll += fDegrees;
		SetRoll(fRoll);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Yaws the camera's forward axis toward the camera's right axis a
//			given number of degrees.
//-----------------------------------------------------------------------------
void CCamera::Yaw(float fDegrees)
{
	if (fDegrees != 0)
	{
		float fYaw = GetYaw();
		fYaw += fDegrees;
		SetYaw(fYaw);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Loads the given matrix with an identity matrix.
// Input  : Matrix - 4 x 4 matrix to be loaded with the identity matrix.
//-----------------------------------------------------------------------------
void CameraIdentityMatrix(matrix4_t& Matrix)
{
	// This function produces a transform which transforms from
	// material system camera space to quake camera space

	// Camera right axis lies along the world X axis.
	Matrix[CAMERA_RIGHT][0] = 1;
	Matrix[CAMERA_RIGHT][1] = 0;
	Matrix[CAMERA_RIGHT][2] = 0;
	Matrix[CAMERA_RIGHT][3] = 0;

	// Camera up axis lies along the world Z axis.
	Matrix[CAMERA_UP][0] = 0;
	Matrix[CAMERA_UP][1] = 0;
	Matrix[CAMERA_UP][2] = 1;
	Matrix[CAMERA_UP][3] = 0;

	// Camera forward axis lies along the negative world Y axis.
	Matrix[CAMERA_FORWARD][0] = 0;
	Matrix[CAMERA_FORWARD][1] = -1;
	Matrix[CAMERA_FORWARD][2] = 0;
	Matrix[CAMERA_FORWARD][3] = 0;

	Matrix[3][0] = 0;
	Matrix[3][1] = 0;
	Matrix[3][2] = 0;
	Matrix[3][3] = 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : MatrixSrc - 
//			MatrixDest - 
//-----------------------------------------------------------------------------
void MatrixCopy(matrix4_t& MatrixSrc, matrix4_t& MatrixDest)
{
	for (int i = 0; i < 4 ; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			MatrixDest[i][j] = MatrixSrc[i][j];
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Generates a view matrix based on our current yaw, pitch, and roll.
//			The view matrix does not consider FOV or clip plane distances.
//-----------------------------------------------------------------------------
void CCamera::BuildViewMatrix(void)
{
	matrix4_t YawMatrix;
	matrix4_t PitchMatrix;
	matrix4_t RollMatrix;
	matrix4_t Temp1Matrix;
	matrix4_t Temp2Matrix;

	//
	// Yaw is a clockwise rotation around world Z axis.
	//
	Vector YawAxis( 0, 0, 1 );
	AxisAngleMatrix(YawMatrix, YawAxis, m_fYaw);

	//
	// Pitch is a clockwise rotation around the world X axis.
	//
	Vector PitchAxis( 1, 0, 0 );
	AxisAngleMatrix(PitchMatrix, PitchAxis, m_fPitch);

	//
	// Roll is a clockwise rotation around the world Y axis.
	//
	Vector RollAxis( 0, 1, 0 );
	AxisAngleMatrix(RollMatrix, RollAxis, m_fRoll);

	// The camera transformation is produced by multiplying roll * yaw * pitch.
	// This will transform a point from world space into quake camera space, 
	// which is exactly what we want for our view matrix. However, quake
	// camera space isn't the same as material system camera space, so
	// we're going to have to apply a transformation that goes from quake
	// camera space to material system camera space.
				 
	//
	// Apply the yaw, pitch, and roll transformations in reverse order.
	// Cause they're all measured in world space
	//
	CameraIdentityMatrix(Temp1Matrix);
	MultiplyMatrix(Temp1Matrix, RollMatrix, Temp2Matrix);
	MultiplyMatrix(Temp2Matrix, PitchMatrix, Temp1Matrix);
	MultiplyMatrix(Temp1Matrix, YawMatrix, Temp2Matrix);

	//
	// Translate the viewpoint to the world origin.
	//
	IdentityMatrix(Temp1Matrix);
	Temp1Matrix[0][3] = -m_ViewPoint[0];
	Temp1Matrix[1][3] = -m_ViewPoint[1];
	Temp1Matrix[2][3] = -m_ViewPoint[2];
	MultiplyMatrix( Temp2Matrix, Temp1Matrix, m_ViewMatrix);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Matrix - 
//-----------------------------------------------------------------------------
void CCamera::GetViewMatrix(matrix4_t& Matrix)
{
	for (int i = 0; i < 4 ; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			Matrix[i][j] = m_ViewMatrix[i][j];
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: to set the zoom in the orthographic gl view
//   Input: fScale - the zoom scale
//-----------------------------------------------------------------------------
void CCamera::SetZoom( float fScale )
{
    m_fZoomScale = fScale;
}


//-----------------------------------------------------------------------------
// Purpose: to accumulate the zoom in the orthographic gl view
//   Input: fScale - the zoom scale
//-----------------------------------------------------------------------------
void CCamera::Zoom( float fScale )
{
    m_fZoomScale += fScale;

    // don't zoom negative 
    if( m_fZoomScale < 0.00001f )
        m_fZoomScale = 0.00001f;
}


//-----------------------------------------------------------------------------
// Purpose: to get the zoom in the orthographic gl view
//  Output: return the zoom scale
//-----------------------------------------------------------------------------
float CCamera::GetZoom( void )
{
    return m_fZoomScale;
}

