//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelSelectLoginOption.h"
#include "Tracker.h"

#include <VGUI_WizardPanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSubPanelSelectLoginOption::CSubPanelSelectLoginOption(Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	m_pCreateUserButton = new RadioButton(this, "CreateUser", "#TrackerUI_CreateNewUser");
	m_pExistingUserButton = new RadioButton(this, "ExistingUser", "#TrackerUI_LoginAsExistingUser");
	new Label(this, "InfoText", "Info");

	m_pExistingUserButton->AddActionSignalTarget(this);
	m_pCreateUserButton->AddActionSignalTarget(this);

	m_pCreateUserButton->SetSelected(true);

	LoadControlSettings("Friends/SubPanelSelectLoginOption.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
CSubPanelSelectLoginOption::~CSubPanelSelectLoginOption()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panel - 
//-----------------------------------------------------------------------------
void CSubPanelSelectLoginOption::OnRadioButtonChecked(Panel *panel)
{
	if (GetWizardPanel())
	{
		GetWizardPanel()->SetNextButtonEnabled(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *previousSubPanel - 
//			*wizardPanel - 
//-----------------------------------------------------------------------------
void CSubPanelSelectLoginOption::PerformLayout()
{
	GetWizardPanel()->SetFinishButtonEnabled(false);

	// only have the next button enabled if one of the radio buttons is selected
	if (m_pExistingUserButton->IsSelected() || m_pCreateUserButton->IsSelected())
	{
		GetWizardPanel()->SetNextButtonEnabled(true);
	}
	else
	{
		GetWizardPanel()->SetNextButtonEnabled(false);
	}

	GetWizardPanel()->SetTitle("#TrackerUI_Friends_SelectLoginMethodTitle", true);
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to the next panel to display, based on this panels state
// Output : WizardSubPanel *
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelSelectLoginOption::GetNextSubPanel()
{
	if (m_pExistingUserButton->IsSelected())
	{
		return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelLoginUser"));
	}

	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelCreateUser"));
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelSelectLoginOption::m_MessageMap[] =
{
	MAP_MESSAGE_PTR( CSubPanelSelectLoginOption, "RadioButtonChecked", OnRadioButtonChecked, "panel" ),	// custom message
};
IMPLEMENT_PANELMAP(CSubPanelSelectLoginOption, Panel);


