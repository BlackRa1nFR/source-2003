//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELCREATEUSER3_H
#define SUBPANELCREATEUSER3_H
#pragma once

#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class ProgressBar;
class Label;
};

//-----------------------------------------------------------------------------
// Purpose: page 2 of the create user wizard
//-----------------------------------------------------------------------------
class CSubPanelCreateUser3 : public vgui::WizardSubPanel
{
public:
	CSubPanelCreateUser3(vgui::Panel *parent, const char *panelName);
	~CSubPanelCreateUser3();

	virtual void PerformLayout();
	virtual void OnDisplayAsNext();
	virtual vgui::WizardSubPanel *GetNextSubPanel();
	virtual vgui::WizardSubPanel *GetPrevSubPanel();
	virtual bool VerifyEntriesAreValid();

	virtual bool OnFinishButton();

protected:
	virtual void OnTick();
	virtual void OnTextChanged();
	virtual void OnTimeout();

	// networking callback interface
	virtual void OnUserCreated(vgui::KeyValues *data);
	virtual void OnUserCreateDenied(vgui::KeyValues *data);

	DECLARE_PANELMAP();

private:
	int m_iTimeoutTime;

	vgui::ProgressBar *m_pProgressBar;
	vgui::Label *m_pLabel;
	vgui::Label *m_pProgressLabel;
	vgui::KeyValues *m_pFinishedMessage;
};

#endif // SUBPANELCREATEUSER3_H
