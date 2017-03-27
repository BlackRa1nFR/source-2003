//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <VGUI_WizardPanel.h>

#include "SubPanelConnectionIntro.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelConnectionIntro::CSubPanelConnectionIntro(Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	LoadControlSettings("Friends/SubPanelConnectionIntro.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelConnectionIntro::~CSubPanelConnectionIntro()
{
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to the next panel to display, based on this panels state
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelConnectionIntro::GetNextSubPanel()
{
	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelConnectionTest"));
}

//-----------------------------------------------------------------------------
// Purpose: Sets up wizard button state
//-----------------------------------------------------------------------------
void CSubPanelConnectionIntro::PerformLayout()
{
	BaseClass::PerformLayout();

	GetWizardPanel()->SetTitle("#TrackerUI_WelcomeFriendsTitle", true);

	GetWizardPanel()->SetNextButtonEnabled(true);
	GetWizardPanel()->SetFinishButtonEnabled(false);
	GetWizardPanel()->SetCancelButtonEnabled(true);
}

