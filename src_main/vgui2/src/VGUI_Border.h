//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Core implementation of vgui
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_BORDER_H
#define VGUI_BORDER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/IBorder.h>
#include <vgui/IScheme.h>
#include <Color.h>

class KeyValues;

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Interface to panel borders
//			Borders have a close relationship with panels
//-----------------------------------------------------------------------------
class Border : public IBorder
{
public:
	Border();
	~Border();

	virtual void Paint(VPANEL panel);
	virtual void Paint(int x0, int y0, int x1, int y1);
	virtual void Paint(int x0, int y0, int x1, int y1, int breakSide, int breakStart, int breakStop);
	virtual void SetInset(int left, int top, int right, int bottom);
	virtual void GetInset(int &left, int &top, int &right, int &bottom);

	virtual void ApplySchemeSettings(IScheme *pScheme, KeyValues *inResourceData);

	virtual const char *GetName();
	virtual void SetName(const char *name);

protected:
	int _inset[4];

private:
	// protected copy constructor to prevent use
	Border(Border&);

	void ParseSideSettings(int side_index, KeyValues *inResourceData, IScheme *pScheme);

	char *_name;

	// border drawing description
	struct line_t
	{
		Color col;
		int startOffset;
		int endOffset;
	};

	struct side_t
	{
		int count;
		line_t *lines;
	};

	side_t _sides[4];	// left, top, right, bottom

	friend class VPanel;
};

} // namespace vgui

#endif // VGUI_BORDER_H
