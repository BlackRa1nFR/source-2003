//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CPanelValveLogo_H
#define CPanelValveLogo_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_ImagePanel.h>

class KeyValues;

namespace vgui
{
enum MouseCode;
};

//-----------------------------------------------------------------------------
// Purpose: Valve logo panel
//-----------------------------------------------------------------------------
class CPanelValveLogo : public vgui::ImagePanel
{
public:
	CPanelValveLogo();
	~CPanelValveLogo();

	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
};


#endif // CPanelValveLogo_H
