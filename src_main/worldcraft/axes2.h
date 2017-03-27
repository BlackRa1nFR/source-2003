//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Defines a base set of services for operations in an orthorgraphic
//			projection. This is used as a base class for the 2D view and for
//			the tools that work in the 2D views.
//
// $NoKeywords: $
//=============================================================================

#ifndef AXES2_H
#define AXES2_H
#ifdef _WIN32
#pragma once
#endif


class Vector;


enum
{
	AXIS_X,
	AXIS_Y,
	AXIS_Z
};


class Axes2
{
public:
	Axes2() { bInvertHorz = bInvertVert = 0; }

	void SetAxes(int h, BOOL bInvertH, int v, BOOL bInvertV);
	void SetAxes(Axes2 &axes);

	void PointToMap(POINT &pt);
	inline void PointToScreen(POINT &pt) { PointToMap(pt); }

	void RectToMap(RECT &rect);
	inline void RectToScreen(RECT &rect) { RectToMap(rect); }

	bool CompareVectors2D(const Vector &vec1, const Vector &vec2);

	bool bInvertHorz;	// Whether the horizontal axis is inverted.
	bool bInvertVert;	// Whether the vertical axis is inverted.

	int axHorz;			// Index of the horizontal axis (x=0, y=1, z=2)
	int axVert;			// Index of the vertical axis (x=0, y=1, z=2)
	int axThird;		// Index of the "out of the screen" axis (x=0, y=1, z=2)
};


#endif // AXES2_H
