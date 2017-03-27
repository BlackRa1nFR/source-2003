//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELFINDBUDDY_H
#define SUBPANELFINDBUDDY_H
#pragma once

#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class TextEntry;
class RadioButton;
};

//-----------------------------------------------------------------------------
// Purpose: First sub panel of the find buddy wizard
//			Asks the user how they want to search
//-----------------------------------------------------------------------------
class CSubPanelFindBuddy : public vgui::WizardSubPanel
{
public:
	CSubPanelFindBuddy(vgui::Panel *parent, const char *panelName);
	~CSubPanelFindBuddy();

	virtual vgui::WizardSubPanel *GetNextSubPanel();
	virtual void OnDisplayAsNext();
	virtual void OnDisplayAsPrev();
	virtual bool OnNextButton();
	virtual void PerformLayout();

	virtual void OnRadioButtonChecked(vgui::Panel *panel);
	virtual void OnTextChanged();

	DECLARE_PANELMAP();

private:
	vgui::TextEntry *m_pFirstNameEdit;
	vgui::TextEntry *m_pLastNameEdit;
	vgui::TextEntry *m_pUserNameEdit;
	vgui::TextEntry *m_pEmailEdit;
};


#endif // SUBPANELFINDBUDDY_H
