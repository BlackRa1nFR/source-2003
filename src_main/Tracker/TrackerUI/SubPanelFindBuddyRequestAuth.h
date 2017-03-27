//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELFINDBUDDYREQUESTAUTH_H
#define SUBPANELFINDBUDDYREQUESTAUTH_H
#pragma once

#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class Label;
class RadioButton;
};

//-----------------------------------------------------------------------------
// Purpose: Requests authorization from specified user, with a specified message
//-----------------------------------------------------------------------------
class CSubPanelFindBuddyRequestAuth : public vgui::WizardSubPanel
{
public:
	CSubPanelFindBuddyRequestAuth(vgui::Panel *parent, const char *panelName);
	~CSubPanelFindBuddyRequestAuth();

	virtual vgui::WizardSubPanel *GetNextSubPanel();
	virtual void OnDisplayAsNext();
	virtual void OnDisplay();
	virtual bool OnNextButton();

protected:
	DECLARE_PANELMAP();

	virtual void OnTextChanged();

private:
	vgui::Label *m_pInfoText;
//	vgui::Label *m_pReqText;
//	vgui::RadioButton *m_pAuthYes;
//	vgui::RadioButton *m_pAuthNo;
};


#endif // SUBPANELFINDBUDDYREQUESTAUTH_H
