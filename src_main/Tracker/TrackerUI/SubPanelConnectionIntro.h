//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELCONNECTIONINTRO_H
#define SUBPANELCONNECTIONINTRO_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_WizardSubPanel.h>

//-----------------------------------------------------------------------------
// Purpose: Panel for option choosing
//-----------------------------------------------------------------------------
class CSubPanelConnectionIntro : public vgui::WizardSubPanel
{
public:
	CSubPanelConnectionIntro(vgui::Panel *parent, const char *panelName);
	~CSubPanelConnectionIntro();

	// returns a pointer to the next panel to display, based on this panels state
	virtual vgui::WizardSubPanel *GetNextSubPanel();
	virtual void PerformLayout();
};



#endif // SUBPANELCONNECTIONINTRO_H
