//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELERROR_H
#define SUBPANELERROR_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class Label;
class WizardSubPanel;
};

//-----------------------------------------------------------------------------
// Purpose: Wizard pane that displays a simple error message, and forces the user to go back
//-----------------------------------------------------------------------------
class CSubPanelError : public vgui::WizardSubPanel
{
public:
	CSubPanelError(vgui::Panel *parent, const char *panelName);
	virtual void SetErrorText(const char *errorText);

	virtual void PerformLayout();
	virtual vgui::WizardSubPanel *GetNextSubPanel();

private:
	vgui::Label *m_pErrorText;
};


#endif // SUBPANELERROR_H
