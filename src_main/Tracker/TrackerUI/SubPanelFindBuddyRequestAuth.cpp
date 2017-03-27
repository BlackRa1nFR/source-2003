//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelFindBuddyRequestAuth.h"
#include "Tracker.h"

#include <VGUI_WizardPanel.h>
#include <VGUI_Label.h>
#include <VGUI_KeyValues.h>
#include <VGUI_RadioButton.h>

#include <stdio.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelFindBuddyRequestAuth::CSubPanelFindBuddyRequestAuth(vgui::Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	m_pInfoText = new Label(this, "InfoText", "Auth");

// removed auth question for now, enforce auto-auth upon request
//	m_pReqText = new Label("AuthToYou", 0, 0, 30, 30);
//	m_pAuthYes = new RadioButton("Yes (recommended)", 20, 20, 40, 40);
//	m_pAuthNo = new RadioButton("No", 30, 30, 50, 50);

	LoadControlSettings("Friends/SubPanelFindBuddyRequestAuth.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelFindBuddyRequestAuth::~CSubPanelFindBuddyRequestAuth()
{
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the panel to move to next
// Output : vgui::WizardSubPanel
//-----------------------------------------------------------------------------
vgui::WizardSubPanel *CSubPanelFindBuddyRequestAuth::GetNextSubPanel()
{
	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelFindBuddyComplete"));
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *previousSubPanel - 
//			*wizardPanel - 
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyRequestAuth::OnDisplayAsNext()
{
	//m_pInfoText->setTextFormatted("%s requires you to be authorized before you can add them\nto your friend list.\n\nEnter your the reason for authorization request below,\nthen hit 'Next'.", GetWizardData()->GetString("UserName"));

//	m_pReqText->SetText("Do you wish %s to be able to see you when you're online?", GetWizardData()->GetString("UserName"));
//	m_pAuthYes->SetSelected(true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyRequestAuth::OnDisplay()
{
	char buf[512];
	sprintf(buf, "Request Authorization - %s", GetWizardData()->GetString("UserName"));
	GetWizardPanel()->SetTitle(buf, true);

//tagES
//	wchar_t unicode[512], unicodeName[64];
//	localize()->ConvertANSIToUnicode(GetWizardData()->GetString("UserName"), unicodeName, sizeof( unicodeName ) / sizeof( wchar_t ));
//	localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_RequestAuthorizationTitle"), 1, unicodeName );
//	SetTitle(unicode, true);

	GetWizardPanel()->SetFinishButtonEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Called when the text in the request box changes.
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyRequestAuth::OnTextChanged()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelFindBuddyRequestAuth::OnNextButton()
{
	// write out the request reason to the doc
//	char buf[255];
//	m_pEdit->getAllText(buf, 254);
//	GetWizardData()->SetString("RequestReason", buf);

//	GetWizardData()->SetInt("AutoAuthorize", m_pAuthYes->IsSelected());
	GetWizardData()->SetInt("AutoAuthorize", true);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelFindBuddyRequestAuth::m_MessageMap[] =
{
	MAP_MESSAGE( CSubPanelFindBuddyRequestAuth, "TextChanged", OnTextChanged ),
};
IMPLEMENT_PANELMAP(CSubPanelFindBuddyRequestAuth, Panel);

