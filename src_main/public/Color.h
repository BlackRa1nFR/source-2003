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

#ifndef COLOR_H
#define COLOR_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Basic handler for an rgb set of colors
//			This class is fully inline
//-----------------------------------------------------------------------------
class Color
{
public:
	// constructors
	Color()
	{
		SetColor(0, 0, 0, 0);
	}
	Color(int r,int g,int b)
	{
		SetColor(r, g, b, 0);
	}
	Color(int r,int g,int b,int a)
	{
		SetColor(r, g, b, a);
	}
	
	// set the color
	// r - red component (0-255)
	// g - green component (0-255)
	// b - blue component (0-255)
	// a - alpha component, controls transparency (0 - transparent, 255 - opaque);
	void SetColor(int r, int g, int b, int a)
	{
		_color[0] = (unsigned char)r;
		_color[1] = (unsigned char)g;
		_color[2] = (unsigned char)b;
		_color[3] = (unsigned char)a;
	}

	void GetColor(int &r, int &g, int &b, int &a) const
	{
		r = _color[0];
		g = _color[1];
		b = _color[2];
		a = _color[3];
	}
	
	unsigned char &operator[](int index)
	{
		return _color[index];
	}

	bool operator == (Color &rhs) const
	{
		return (_color[0] == rhs._color[0] && _color[1] == rhs._color[1] && _color[2] == rhs._color[2] && _color[3] == rhs._color[3]);
	}

	bool operator != (Color &rhs) const
	{
		return !(operator==(rhs));
	}

private:
	unsigned char _color[4];
};


#endif // COLOR_H
