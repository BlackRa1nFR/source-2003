//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELCREATEUSER2_H
#define SUBPANELCREATEUSER2_H
#pragma once

#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class TextEntry;
};

//-----------------------------------------------------------------------------
// Purpose: page 2 of the create user wizard
//-----------------------------------------------------------------------------
class CSubPanelCreateUser2 : public vgui::WizardSubPanel
{
public:
	CSubPanelCreateUser2(vgui::Panel *parent, const char *panelName);
	virtual void PerformLayout();
	virtual void OnDisplay();
	virtual bool OnNextButton();
	virtual vgui::WizardSubPanel *GetNextSubPanel();

	virtual bool VerifyEntriesAreValid();

protected:
	virtual void OnTextChanged();

	DECLARE_PANELMAP();

private:
	vgui::TextEntry *m_pUserNameEdit;
	vgui::TextEntry *m_pFirstNameEdit;
	vgui::TextEntry *m_pLastNameEdit;
	vgui::TextEntry *m_pPasswordEdit;
	vgui::TextEntry *m_pPasswordRepeatEdit;
};

#endif // SUBPANELCREATEUSER2_H
