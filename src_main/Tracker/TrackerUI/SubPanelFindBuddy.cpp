//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelFindBuddy.h"
#include "Tracker.h"

#include <VGUI_RadioButton.h>
#include <VGUI_TextEntry.h>
#include <VGUI_KeyValues.h>
#include <VGUI_WizardPanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Finds and removes any wildcard characters from the string
// Input  : *buf - 
//-----------------------------------------------------------------------------
void StripWildcards(char *buf)
{
	int len = strlen(buf);
	
	// strip from end
	if (buf[len - 1] == '*')
	{
		buf[len - 1] = 0;
		--len;
	}

	// strip from Start
	if (buf[0] == '*')
	{
		memmove(buf, buf + 1, len);		
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelFindBuddy::CSubPanelFindBuddy(vgui::Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	// create the controls
	m_pUserNameEdit = new TextEntry(this, "UserNameEdit");
	m_pFirstNameEdit = new TextEntry(this, "FirstNameEdit");
	m_pLastNameEdit = new TextEntry(this, "LastNameEdit");
	m_pEmailEdit = new TextEntry(this, "EmailEdit");

	LoadControlSettings("Friends/SubPanelFindBuddy.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelFindBuddy::~CSubPanelFindBuddy()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : WizardSubPanel *
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelFindBuddy::GetNextSubPanel()
{
	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelFindBuddyResults"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelFindBuddy::OnDisplayAsNext()
{
	GetWizardPanel()->SetNextButtonEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CSubPanelFindBuddy::OnDisplayAsPrev()
{
	GetWizardPanel()->SetNextButtonEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: Writes out to the wizard doc the search details
//-----------------------------------------------------------------------------
bool CSubPanelFindBuddy::OnNextButton()
{
	char buf[256];

	KeyValues *doc = GetWizardData();

	m_pEmailEdit->GetText(0, buf, 255);
	StripWildcards(buf);
	doc->SetString("Email", buf);
	
	m_pUserNameEdit->GetText(0, buf, 255);
	StripWildcards(buf);
	doc->SetString("UserName", buf);
	
	m_pFirstNameEdit->GetText(0, buf, 255);
	StripWildcards(buf);
	doc->SetString("FirstName", buf);
	
	m_pLastNameEdit->GetText(0, buf, 255);
	StripWildcards(buf);
	doc->SetString("LastName", buf);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Enables/disables different radio buttons and 
//			edit boxes depending on which radio button
//			is checked
//-----------------------------------------------------------------------------
void CSubPanelFindBuddy::OnRadioButtonChecked(Panel *panel)
{
	InvalidateLayout(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelFindBuddy::PerformLayout()
{
	OnTextChanged();

	GetWizardPanel()->SetTitle("#TrackerUI_Friends_SearchForFriendsTitle", false);

	GetWizardPanel()->SetFinishButtonEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Evaluates if the next button should be enabled
//-----------------------------------------------------------------------------
void CSubPanelFindBuddy::OnTextChanged()
{
	bool nextActive = false;

	// Activate the next button if any one of the name fields has data in it
	char buf[256];
	m_pEmailEdit->GetText(0, buf, 255);
	if (strlen(buf) > 0)
	{
		nextActive = true;
	}
	else
	{
		m_pUserNameEdit->GetText(0, buf, 255);
		if (strlen(buf) > 0)
		{
			nextActive = true;
		}
		else
		{
			m_pFirstNameEdit->GetText(0, buf, 255);
			if (strlen(buf) > 0)
			{
				nextActive = true;
			}
			else
			{
				m_pLastNameEdit->GetText(0, buf, 255);
				if (strlen(buf) > 0)
				{
					nextActive = true;
				}
			}
		}
	}

	GetWizardPanel()->SetNextButtonEnabled(nextActive);
}


//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelFindBuddy::m_MessageMap[] =
{
	MAP_MESSAGE_PTR( CSubPanelFindBuddy, "RadioButtonChecked", OnRadioButtonChecked, "panel" ),	// custom message
	MAP_MESSAGE( CSubPanelFindBuddy, "TextChanged", OnTextChanged ),
};
IMPLEMENT_PANELMAP(CSubPanelFindBuddy, Panel);
