//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Axes2.h"
#include "Vector.h"


void Axes2::PointToMap(POINT &pt)
{
	if(bInvertHorz)
		pt.x = -pt.x;
	if(bInvertVert)
		pt.y = -pt.y;
}

void Axes2::RectToMap(RECT &rect)
{
	if(bInvertHorz)
	{
		rect.left = -rect.left;
		rect.right = -rect.right;
	}
	if(bInvertVert)
	{
		rect.top = -rect.top;
		rect.bottom = -rect.bottom;
	}
}

void Axes2::SetAxes(int h, BOOL bInvertH, int v, BOOL bInvertV)
{
	bInvertHorz = (bInvertH == TRUE);
	bInvertVert = (bInvertV == TRUE);

	axHorz = h;
	axVert = v;

	if(h != AXIS_X && v != AXIS_X)
		axThird = AXIS_X;
	if(h != AXIS_Y && v != AXIS_Y)
		axThird = AXIS_Y;
	if(h != AXIS_Z && v != AXIS_Z)
		axThird = AXIS_Z;
}

void Axes2::SetAxes(Axes2 &axes)
{
	*this = axes;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the vectors are the same along these axes.
//-----------------------------------------------------------------------------
bool Axes2::CompareVectors2D(const Vector &vec1, const Vector &vec2)
{
	return ((vec1[axHorz] == vec2[axHorz]) && (vec1[axVert] == vec2[axVert]));
}


