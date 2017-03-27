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

#ifndef IBORDER_H
#define IBORDER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

class KeyValues;

namespace vgui
{

class IScheme;

//-----------------------------------------------------------------------------
// Purpose: Interface to panel borders
//			Borders have a close relationship with panels
//			They are the edges of the panel.
//-----------------------------------------------------------------------------
class IBorder
{
public:
	virtual void Paint(VPANEL panel) = 0;
	virtual void Paint(int x0, int y0, int x1, int y1) = 0;
	virtual void Paint(int x0, int y0, int x1, int y1, int breakSide, int breakStart, int breakStop) = 0;
	virtual void SetInset(int left, int top, int right, int bottom) = 0;
	virtual void GetInset(int &left, int &top, int &right, int &bottom) = 0;
	virtual void ApplySchemeSettings(IScheme *pScheme, KeyValues *inResourceData) = 0;
	virtual const char *GetName() = 0;
	virtual void SetName(const char *name) = 0;

	enum sides_e
	{
		SIDE_LEFT = 0,
		SIDE_TOP = 1,
		SIDE_RIGHT = 2,
		SIDE_BOTTOM = 3
	};
};

} // namespace vgui


#endif // IBORDER_H
