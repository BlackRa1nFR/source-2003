//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BUDDYSECTIONHEADER_H
#define BUDDYSECTIONHEADER_H
#pragma once

#include <VGUI_Label.h>
#include <VGUI_Color.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuddySectionHeader : public vgui::Label
{
public:
	CBuddySectionHeader(vgui::Panel *parent, const char *text);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual void Paint(void);

private:
	vgui::Color m_SectionDividerColor;
};


#endif // BUDDYSECTIONHEADER_H
