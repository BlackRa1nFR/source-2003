//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Box3D.h"
#include "MapDefs.h" // for COORD_NOTINIT
#include "MapPoint.h"
#include "worldcraft_mathlib.h"
#include <assert.h>


//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes the origin point to all zeros.
//-----------------------------------------------------------------------------
CMapPoint::CMapPoint(void)
{
	ZeroVector(m_Origin);
}


//-----------------------------------------------------------------------------
// Purpose: Returns this point's X, Y, Z coordinates.
//-----------------------------------------------------------------------------
void CMapPoint::GetOrigin(Vector &Origin)
{
	Origin = m_Origin;
}


//-----------------------------------------------------------------------------
// Purpose: Sets this point's X, Y, Z coordinates.
//-----------------------------------------------------------------------------
void CMapPoint::SetOrigin(Vector &Origin)
{
	m_Origin = Origin;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTransBox - 
//-----------------------------------------------------------------------------
void CMapPoint::DoTransform(Box3D *pTransBox)
{
	pTransBox->TranslatePoint(m_Origin);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//-----------------------------------------------------------------------------
void CMapPoint::DoTransFlip(const Vector &RefPoint)
{
	for (int nDim = 0; nDim < 3; nDim++)
	{
		if (RefPoint[nDim] == COORD_NOTINIT)
		{
			continue;
		}
		m_Origin[nDim] = RefPoint[nDim] - (m_Origin[nDim] - RefPoint[nDim]);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Offsets this point by a delta vector.
// Input  : delta - 
//-----------------------------------------------------------------------------
void CMapPoint::DoTransMove(const Vector &delta)
{
	m_Origin += delta;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRefPoint - 
//			Angles - 
//-----------------------------------------------------------------------------
void CMapPoint::DoTransRotate(const Vector *pRefPoint, const QAngle &Angles)
{
	if (pRefPoint != NULL)
	{
		m_Origin -= *pRefPoint;
	}

	if (Angles.x)
	{
		rotate_coords(&m_Origin[1], &m_Origin[2], m_Origin[1], m_Origin[2], Angles.x);
	}

	if (Angles.y)
	{
		rotate_coords(&m_Origin[0], &m_Origin[2], m_Origin[0], m_Origin[2], Angles.y);
	}

	if (Angles.z)
	{
		rotate_coords(&m_Origin[0], &m_Origin[1], m_Origin[0], m_Origin[1], Angles.z);
	}

	if (pRefPoint != NULL)
	{
		m_Origin += *pRefPoint;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//			Scale - 
//-----------------------------------------------------------------------------
void CMapPoint::DoTransScale(const Vector &RefPoint, const Vector &Scale)
{
	m_Origin = RefPoint + ((m_Origin - RefPoint) * Scale);
}

