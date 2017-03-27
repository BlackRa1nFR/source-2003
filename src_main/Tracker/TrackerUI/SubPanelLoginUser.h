//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELLOGINUSER_H
#define SUBPANELLOGINUSER_H
#pragma once

#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class TextEntry;
};

//-----------------------------------------------------------------------------
// Purpose: Dialog that collects the details the user uses to log into the server
//-----------------------------------------------------------------------------
class CSubPanelLoginUser : public vgui::WizardSubPanel
{
public:
	CSubPanelLoginUser(vgui::Panel *parent, const char *panelName);

	virtual void PerformLayout();
	virtual vgui::WizardSubPanel *GetNextSubPanel();

	virtual bool OnNextButton();

	virtual bool VerifyEntriesAreValid();

	virtual void OnTextChanged();

	DECLARE_PANELMAP();

public:
	vgui::TextEntry *m_pEmailEdit;
	vgui::TextEntry *m_pPassword;

};




#endif // SUBPANELLOGINUSER_H
