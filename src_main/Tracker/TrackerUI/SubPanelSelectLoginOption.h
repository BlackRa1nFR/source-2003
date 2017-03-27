//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELSELECTLOGINOPTION_H
#define SUBPANELSELECTLOGINOPTION_H
#pragma once

#include <VGUI_RadioButton.h>
#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class WizardPanel;
};

//-----------------------------------------------------------------------------
// Purpose: Panel for option choosing
//-----------------------------------------------------------------------------
class CSubPanelSelectLoginOption : public vgui::WizardSubPanel
{
public:
	CSubPanelSelectLoginOption(vgui::Panel *parent, const char *panelName);
	~CSubPanelSelectLoginOption();

	virtual void PerformLayout();

	// returns a pointer to the next panel to display, based on this panels state
	virtual vgui::WizardSubPanel *GetNextSubPanel();

	DECLARE_PANELMAP();
	virtual void OnRadioButtonChecked(vgui::Panel *panel);

private:
	vgui::RadioButton *m_pCreateUserButton;
	vgui::RadioButton *m_pExistingUserButton;
};



#endif // SUBPANELSELECTLOGINOPTION_H
