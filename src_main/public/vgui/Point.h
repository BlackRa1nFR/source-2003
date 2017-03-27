//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef POINT_H
#define POINT_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Basic handler for a Points in 2 dimensions
//			This class is fully inline
//-----------------------------------------------------------------------------
class Point
{
public:
	// constructors
	Point()
	{
		SetPoint(0, 0);
	}
	Point(int x,int y)
	{
		SetPoint(x,y);
	}

	void SetPoint(int x1, int y1)
	{
		x=x1;
		y=y1;	
	}

	void GetPoint(int &x1, int &y1) const
	{
		x1 = x;
		y1 = y;
	
	}

	bool operator == (Point &rhs) const
	{
		return (x == rhs.x && y == rhs.y);
	}

private:
	int x, y;
};

} // namespace vgui

#endif // POINT_H
