//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelCreateUser2.h"
#include "SubPanelError.h"
#include "Tracker.h"

#include <VGUI_KeyValues.h>
#include <VGUI_TextEntry.h>
#include <VGUI_WizardPanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
CSubPanelCreateUser2::CSubPanelCreateUser2(vgui::Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	LoadControlSettings("Friends/SubPanelCreateUser2.res");

	m_pUserNameEdit = dynamic_cast<TextEntry *>(FindChildByName("LoginIDEdit"));
	m_pFirstNameEdit = dynamic_cast<TextEntry *>(FindChildByName("FirstNameEdit"));
	m_pLastNameEdit = dynamic_cast<TextEntry *>(FindChildByName("LastNameEdit"));
	m_pPasswordEdit = dynamic_cast<TextEntry *>(FindChildByName("PasswordEdit"));
	m_pPasswordRepeatEdit = dynamic_cast<TextEntry *>(FindChildByName("PasswordRepeatEdit"));

	m_pUserNameEdit->AddActionSignalTarget(this);
	m_pFirstNameEdit->AddActionSignalTarget(this);
	m_pLastNameEdit->AddActionSignalTarget(this);
	m_pPasswordEdit->AddActionSignalTarget(this);
	m_pPasswordRepeatEdit->AddActionSignalTarget(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CSubPanelCreateUser2::PerformLayout()
{
	GetWizardPanel()->SetFinishButtonEnabled(false);
	GetWizardPanel()->SetTitle("#TrackerUI_Friends_CreateNewUser_2_of_3_Title", false);
	GetWizardPanel()->SetNextButtonEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *previousSubPanel - 
//			*wizardPanel - 
//-----------------------------------------------------------------------------
void CSubPanelCreateUser2::OnDisplay()
{
	GetWizardPanel()->SetNextButtonEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelCreateUser2::OnNextButton()
{
	// write the data out into the wizard
	KeyValues *dat = GetWizardData();

	char buf[256];

	m_pUserNameEdit->GetText(0, buf, 255);
	dat->SetString("username", buf);

	m_pFirstNameEdit->GetText(0, buf, 255);
	dat->SetString("firstname", buf);

	m_pLastNameEdit->GetText(0, buf, 255);
	dat->SetString("lastname", buf);

	m_pPasswordEdit->GetText(0, buf, 255);
	dat->SetString("password", buf);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Checks to see if the user has entered enough info to be allowed to continue
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelCreateUser2::VerifyEntriesAreValid()
{
	char buf[256], buf2[256];

	// check user name
	m_pUserNameEdit->GetText(0, buf, 255);
	if (strlen(buf) < 4)
		return false;

	// check passwords are the same
	m_pPasswordEdit->GetText(0, buf, 255);
	m_pPasswordRepeatEdit->GetText(0, buf2, 255);
	if (strlen(buf) < 3 || strcmp(buf, buf2))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when the text in one of the text entries changes
//-----------------------------------------------------------------------------
void CSubPanelCreateUser2::OnTextChanged()
{
	if (IsVisible())
	{
		GetWizardPanel()->SetNextButtonEnabled(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks for errors before moving to next panel
// Output : WizardSubPanel
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelCreateUser2::GetNextSubPanel()
{
	CSubPanelError *errorPanel = dynamic_cast<CSubPanelError *>(GetWizardPanel()->FindChildByName("SubPanelError"));
	if (errorPanel)
	{
		char buf[256], buf2[256];

		// check user name
		m_pUserNameEdit->GetText(0, buf, 255);
		if (strlen(buf) < 3)
		{
			GetWizardPanel()->SetTitle("#TrackerUI_Friends_InvalidUserNameTitle", true);
			errorPanel->SetErrorText("#TrackerUI_InvalidUserNameLessThan3");
			return errorPanel;
		}
		if (strlen(buf) > 24)
		{
			GetWizardPanel()->SetTitle("#TrackerUI_Friends_InvalidUserNameTitle", true);
			errorPanel->SetErrorText("#TrackerUI_InvalidUserNameGreaterThan24");
			return errorPanel;
		}

		// check passwords are the same
		m_pPasswordEdit->GetText(0, buf, 255);
		m_pPasswordRepeatEdit->GetText(0, buf2, 255);
		if (strlen(buf) < 6)
		{
			GetWizardPanel()->SetTitle("#TrackerUI_Friends_InvalidPasswordTitle", true);
			errorPanel->SetErrorText("#TrackerUI_InvalidPasswordMustBeAtLeast6");
			return errorPanel;
		}
		if (strcmp(buf, buf2))
		{
			GetWizardPanel()->SetTitle("#TrackerUI_Friends_InvalidPasswordTitle", true);
			errorPanel->SetErrorText("#TrackerUI_SecondPasswordDidntMatchFirst\n");
			return errorPanel;
		}
	}

	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelCreateUser3"));
}



//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelCreateUser2::m_MessageMap[] =
{
	MAP_MESSAGE( CSubPanelCreateUser2, "TextChanged", OnTextChanged ),	// custom message
};
IMPLEMENT_PANELMAP(CSubPanelCreateUser2, Panel);

