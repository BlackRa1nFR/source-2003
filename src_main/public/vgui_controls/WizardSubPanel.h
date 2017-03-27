//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef WIZARDSUBPANEL_H
#define WIZARDSUBPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>

namespace vgui
{

class WizardPanel;
class BuildGroup;

//-----------------------------------------------------------------------------
// Purpose: Base panel for use in Wizards and in property sheets
//-----------------------------------------------------------------------------
class WizardSubPanel : public EditablePanel
{
public:
	// constructor
	WizardSubPanel(Panel *parent, const char *panelName);
	~WizardSubPanel();

	// called when the subpanel is displayed
	// All controls & data should be reinitialized at this time
	virtual void OnDisplayAsNext() {}

	// called anytime the panel is first displayed, whether the user is moving forward or back
	// called immediately after OnDisplayAsNext/OnDisplayAsPrev
	virtual void OnDisplay() {}

	// called when displayed as previous
	virtual void OnDisplayAsPrev() {}

	// called when one of the wizard buttons are pressed
	// returns true if the wizard should advance, false otherwise
	virtual bool OnNextButton() { return true; }
	virtual bool OnPrevButton() { return true; }
	virtual bool OnFinishButton() { return true; }
	virtual bool OnCancelButton() { return true; }

	// return true if this subpanel doesn't need the next/prev/finish/cancel buttons or will do it itself
	virtual bool isNonWizardPanel() { return false; }

	// returns a pointer to the next subpanel that should be displayed
	virtual WizardSubPanel *GetNextSubPanel()  = 0;

	// returns a pointer to the panel to return to
	// it must be a panel that is already in the wizards panel history
	// returning NULL tells it to use the immediate previous panel in the history
	virtual WizardSubPanel *GetPrevSubPanel() { return NULL; }

	virtual WizardPanel *GetWizardPanel() { return _wizardPanel; }
	virtual void SetWizardPanel(WizardPanel *wizardPanel) { _wizardPanel = wizardPanel; }

	// returns a pointer to the wizard's doc
	virtual KeyValues *GetWizardData();

	// returns a pointer
	virtual WizardSubPanel *GetSiblingSubPanelByName(const char *pageName);

protected:
	virtual void ApplySchemeSettings(IScheme *pScheme);

private:
	typedef vgui::EditablePanel BaseClass;

	WizardPanel *_wizardPanel;
};

} // namespace vgui


#endif // WIZARDSUBPANEL_H	   
