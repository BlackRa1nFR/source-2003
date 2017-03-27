//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "MapDoc.h"
#include "MapView2D.h"
#include "Tool3D.h"
#include "worldcraft_mathlib.h"


#pragma warning(disable:4244)


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Tool3D::Tool3D(void)
{
	m_bTranslating = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
// Output : 
//-----------------------------------------------------------------------------
BOOL Tool3D::StartTranslation(CPoint &pt)
{
	m_bTranslating = true;

	m_ptOrigin = pt;

	m_axFirstHorz = axHorz;
	m_axFirstVert = axVert;

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bSave - 
//-----------------------------------------------------------------------------
void Tool3D::FinishTranslation(BOOL bSave)
{
	m_bTranslating = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt3 - 
//			pt2 - 
//-----------------------------------------------------------------------------
void Tool3D::PointToScreen(Vector& pt3, POINT& pt2)
{
	pt2.x = pt3[axHorz] * fZoom;
	pt2.y = pt3[axVert] * fZoom;

	Axes2::PointToScreen(pt2);
}


//-----------------------------------------------------------------------------
// Purpose: Called by the 2D view to set up mapping before dispatching messages
//			to the tool.
//			TODO: If possible, we should move most mapping-specific services into the view.
// Input  : nHorz - 
//			bInvertH - 
//			nVert - 
//			bInvertV - 
//			flSetZoom - 
//-----------------------------------------------------------------------------
void Tool3D::SetAxesZoom(int nHorz, bool bInvertH, int nVert, bool bInvertV, float flSetZoom)
{
	axHorz = nHorz;
	bInvertHorz = bInvertH;

	axVert = nVert;
	bInvertVert = bInvertV;

	if ((axHorz != 0) && (axVert != 0))
	{
		axThird = 0;
	}
	else if ((axHorz != 1) && (axVert != 1))
	{
		axThird = 1;
	}
	else
	{
		axThird = 2;
	}

	fZoom = flSetZoom;
}


