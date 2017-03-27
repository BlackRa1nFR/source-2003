//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELCREATEUSER_H
#define SUBPANELCREATEUSER_H
#pragma once

#include <VGUI_WizardSubPanel.h>

//-----------------------------------------------------------------------------
// Purpose: Wizard pane that collects the details needed for creating a new user
//-----------------------------------------------------------------------------
class CSubPanelCreateUser : public vgui::WizardSubPanel
{
public:
	CSubPanelCreateUser(vgui::Panel *parent, const char *panelName);

	virtual void PerformLayout();

	virtual vgui::WizardSubPanel *GetNextSubPanel();


	virtual bool OnNextButton();

	DECLARE_PANELMAP();
	virtual void OnTextChanged();	// called when the text changes
	
	// returns true if the email address contains a '@' and a '.'
	static bool IsEmailAddressValid(const char *emailAddress);
};


#endif // SUBPANELCREATEUSER_H
