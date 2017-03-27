//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BuddySectionHeader.h"

#include <VGUI_Controls.h>
#include <VGUI_IScheme.h>
#include <VGUI_ISurface.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBuddySectionHeader::CBuddySectionHeader(vgui::Panel *parent, const char *text) : Label(parent, NULL, text)
{
	SetBounds(0, 0, 20, 16);
	SetContentAlignment(Label::a_west);
	SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *inResourceData - 
//-----------------------------------------------------------------------------
void CBuddySectionHeader::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	Label::ApplySchemeSettings(pScheme);
	SetFgColor(GetSchemeColor("SectionTextColor"));
	m_SectionDividerColor = GetSchemeColor("SectionDividerColor");
	SetBgColor(GetSchemeColor("BuddyListBgColor", GetBgColor()));
	SetFont(scheme()->GetFont(scheme()->GetDefaultScheme(), "DefaultVerySmall"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuddySectionHeader::Paint(void)
{
	Label::Paint();

	int x, y, wide, tall;
	GetBounds(x, y, wide, tall);

	y = (tall - 2);	// draw the line under the panel

	surface()->DrawSetColor(m_SectionDividerColor);
	surface()->DrawFilledRect(1, y, GetWide() - 2, y + 1);
}



