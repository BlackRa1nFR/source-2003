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

#ifndef DIVIDER_H
#define DIVIDER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Thin line used to divide sections in dialogs
//-----------------------------------------------------------------------------
class Divider : public Panel
{
public:
	Divider(Panel *parent, const char *name);
	~Divider();

	virtual void ApplySchemeSettings(IScheme *pScheme);

private:
	DECLARE_PANELMAP();

	typedef Panel BaseClass;
};


} // namespace vgui


#endif // DIVIDER_H
