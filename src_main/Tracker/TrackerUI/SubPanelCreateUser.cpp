//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelCreateUser.h"
#include "SubPanelError.h"
#include "Tracker.h"

#include <VGUI_Controls.h>
#include <VGUI_ISurface.h>
#include <VGUI_KeyValues.h>
#include <VGUI_TextEntry.h>
#include <VGUI_WizardPanel.h>

#include <string.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
CSubPanelCreateUser::CSubPanelCreateUser(Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	LoadControlSettings("Friends/SubPanelCreateUser.res");

	// get the email edit control
	Panel *emailEdit = FindChildByName("EmailEdit");
	if (emailEdit)
	{
		emailEdit->AddActionSignalTarget(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelCreateUser::OnNextButton()
{
	TextEntry *emailEdit = dynamic_cast<TextEntry *>(FindChildByName("EmailEdit"));
	if (!emailEdit)
		return false;

	char buf[256];
	emailEdit->GetText(0, buf, 255);

	// write the data out into the wizard
	KeyValues *dat = GetWizardData();
	dat->SetString("email", buf);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CSubPanelCreateUser::PerformLayout()
{
	GetWizardPanel()->SetTitle("#TrackerUI_Friends_CreateNewUser_1_of_3_Title", false);
	surface()->SetTitle(GetVPanel(), "#TrackerUI_Friends_CreateNewUserTitle");
	GetWizardPanel()->SetFinishButtonEnabled(false);

	// make sure we have a valid email address before letting the user move on
	bool nextValid = false;
	TextEntry *emailEdit = dynamic_cast<TextEntry *>(FindChildByName("EmailEdit"));
	if (emailEdit)
	{
		char buf[256];
		emailEdit->GetText(0, buf, 255);
		if (strlen(buf) > 0)
		{
			nextValid = true;
		}
	}

	GetWizardPanel()->SetNextButtonEnabled(nextValid);
}

//-----------------------------------------------------------------------------
// Purpose: Returns the next sub panel is email address is valid, error box otherwise.
// Input  : 
// Output : WizardSubPanel
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelCreateUser::GetNextSubPanel()
{
	TextEntry *emailEdit = dynamic_cast<TextEntry *>(FindChildByName("EmailEdit"));
	if (emailEdit)
	{
		char buf[256];
		emailEdit->GetText(0, buf, 255);
		if (!IsEmailAddressValid(buf))
		{
			// return error dialog
			CSubPanelError *errorPanel = dynamic_cast<CSubPanelError *>(GetWizardPanel()->FindChildByName("SubPanelError"));
			if (errorPanel)
			{
				GetWizardPanel()->SetTitle("#TrackerUI_Friends_InvalidEmailTitle", true);
				errorPanel->SetErrorText("#TrackerUI_EMailIsInvalidTryAgain");
				return errorPanel;
			}
		}
	}

	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelCreateUser2"));
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
//-----------------------------------------------------------------------------
void CSubPanelCreateUser::OnTextChanged()
{
	if (IsVisible())
	{
		TextEntry *emailEdit = dynamic_cast<TextEntry *>(FindChildByName("EmailEdit"));
		if (emailEdit)
		{
			char text[256];
			emailEdit->GetText(0, text, 254);
			GetWizardPanel()->SetNextButtonEnabled(strlen(text) > 0);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelCreateUser::IsEmailAddressValid(const char *email)
{
	// check for initial valid characters
	const char *ch = email;

	// look for the '@'
	while (1)
	{
		if (*ch == 0)
		{
			return false;
		}

		if (*ch == '@')
		{
			if ((ch - email) < 1)
				return false;

			// first test passed
			break;
		}

		ch++;
	}

	const char *andpoint = ch;

	// look for the period
	while (1)
	{
		if (*ch == 0)
		{
			return false;
		}

		if (*ch == '.')
		{
			if ((ch - andpoint) < 1)
				return false;

			// second test passed
			break;
		}

		ch++;
	}

	// make sure there is at least one more character following the period
	if (ch[1] == 0)
		return false;

	// success
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelCreateUser::m_MessageMap[] =
{
	MAP_MESSAGE( CSubPanelCreateUser, "TextChanged", OnTextChanged ),	// custom message
};
IMPLEMENT_PANELMAP(CSubPanelCreateUser, Panel);


