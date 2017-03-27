//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelFindBuddyComplete.h"
#include "TrackerDoc.h"
#include "ServerSession.h"
#include "OnlineStatus.h"
#include "TrackerDialog.h"
#include "Tracker.h"

#include <VGUI_Label.h>
#include <VGUI_WizardPanel.h>
#include <VGUI_KeyValues.h>

#include <stdio.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelFindBuddyComplete::CSubPanelFindBuddyComplete(vgui::Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	m_pInfoText = new Label(this, "InfoText", "");

	LoadControlSettings("Friends/SubPanelFindBuddyComplete.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelFindBuddyComplete::~CSubPanelFindBuddyComplete()
{
}

//-----------------------------------------------------------------------------
// Purpose: Returns the next panel
// Output : WizardSubPanel
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelFindBuddyComplete::GetNextSubPanel()
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *previousSubPanel - 
//			*wizardPanel - 
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyComplete::OnDisplayAsNext()
{
	GetWizardPanel()->SetNextButtonEnabled(false);
	GetWizardPanel()->SetFinishButtonEnabled(true);
	GetWizardPanel()->SetCancelButtonEnabled(false);

	GetWizardPanel()->SetTitle("#TrackerUI_Friends_AddFriendsCompleteTitle", false);

	KeyValues *doc = GetWizardData();
	int userCount = doc->GetInt("UserCount");

	if (userCount == 1)
	{
		m_pInfoText->SetText("#TrackerUI_AuthRequestSent");
	}
	else
	{
		m_pInfoText->SetText("#TrackerUI_AuthRequestsSent");
	}
	
	// walk the list of users
	for (KeyValues *user = doc->FindKey("Users", true)->GetFirstSubKey(); user != NULL; user = user->GetNextKey())
	{
		int targetID = user->GetInt("UID");

		if (!GetDoc()->GetBuddy(targetID))
		{
			// the buddy is not in our list yet, so add them
			KeyValues *buddy = new KeyValues("Buddy");
			buddy->SetInt("UID", targetID);
			buddy->SetString("UserName", user->GetString("UserName"));
			buddy->SetString("DisplayName", user->GetString("UserName"));
			buddy->SetString("FirstName", user->GetString("FirstName"));
			buddy->SetString("LastName", user->GetString("LastName"));
			buddy->SetInt("Status", COnlineStatus::AWAITINGAUTHORIZATION);
			buddy->SetInt("Removed", 0);
			buddy->SetInt("RemoteBlock", 0);

			// add user to buddy list as offline
			GetDoc()->AddNewBuddy(buddy);
			buddy->deleteThis();
			buddy = NULL;
		}

		// build our reason from our name
		char szReason[256];
		const char *userName = GetDoc()->Data()->GetString("User/UserName", NULL);
		const char *firstName = GetDoc()->Data()->GetString("User/FirstName");
		const char *lastName = GetDoc()->Data()->GetString("User/LastName");
		const char *email = GetDoc()->Data()->GetString("User/Email", "");
		if (firstName && lastName)
		{
			sprintf(szReason, "%s, %s %s, %s.", userName, firstName, lastName, email);
		}
		else if (firstName)
		{
			sprintf(szReason, "%s, %s, %s.", userName, firstName, email);
		}
		else if (lastName)
		{
			sprintf(szReason, "%s, %s, %s.", userName, lastName, email);
		}
		else
		{
			sprintf(szReason, "%s, %s.", userName, email);
		}

		// send message requesting authorization
		ServerSession().RequestAuthorizationFromUser(targetID, szReason);
	}

	// request the data be saved
	CTrackerDialog::GetInstance()->MarkUserDataAsNeedsSaving();

	//!! hack to make sure everything gets redrawn
	int wide, tall;
	CTrackerDialog::GetInstance()->GetSize(wide, tall);
	CTrackerDialog::GetInstance()->SetSize(wide + 1, tall);
	CTrackerDialog::GetInstance()->SetSize(wide, tall);
}



