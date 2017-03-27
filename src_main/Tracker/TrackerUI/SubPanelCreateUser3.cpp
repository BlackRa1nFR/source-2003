//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelCreateUser3.h"
#include "Tracker.h"

#include "ServerSession.h"
#include "TrackerDoc.h"
#include "TrackerProtocol.h"

#include <VGUI_Controls.h>
#include <VGUI_ILocalize.h>
#include <VGUI_ISystem.h>
#include <VGUI_IVGui.h>
#include <VGUI_Label.h>
#include <VGUI_ProgressBar.h>
#include <VGUI_WizardPanel.h>

using namespace vgui;

static const int CREATEUSER_TIMEOUT = 10000;	// 10 second timeout

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelCreateUser3::CSubPanelCreateUser3(Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	LoadControlSettings("Friends/SubPanelCreateUser3.res");

	m_pProgressBar = dynamic_cast<ProgressBar *>(FindChildByName("ProgressBar"));
	m_pLabel = dynamic_cast<Label *>(FindChildByName("InfoText"));
	m_pProgressLabel = dynamic_cast<Label *>(FindChildByName("ProgressText"));

	m_pFinishedMessage = NULL;
	m_iTimeoutTime = 0;

	ServerSession().AddNetworkMessageWatch(this, TSVC_USERCREATED);
	ServerSession().AddNetworkMessageWatch(this, TSVC_USERCREATEDENIED);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelCreateUser3::~CSubPanelCreateUser3()
{
	ServerSession().RemoveNetworkMessageWatch(this);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CSubPanelCreateUser3::PerformLayout()
{
	GetWizardPanel()->SetTitle("#TrackerUI_Friends_CreateNewUser_3_of_3_Title", false);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *previousSubPanel - 
//			*wizardPanel - 
//-----------------------------------------------------------------------------
void CSubPanelCreateUser3::OnDisplayAsNext()
{
	GetWizardPanel()->SetNextButtonEnabled(false);
	GetWizardPanel()->SetPrevButtonEnabled(false);
	GetWizardPanel()->SetFinishButtonEnabled(false);

	// Start the networking
	KeyValues *dat = GetWizardData();
	ServerSession().CreateNewUser(dat->GetString("email"), 
												dat->GetString("username"), 
												dat->GetString("firstname"), 
												dat->GetString("lastname"), 
												dat->GetString("password"));

	m_iTimeoutTime = system()->GetTimeMillis() + CREATEUSER_TIMEOUT;

	// set progress bar to be a quarter of the way done
	m_pProgressBar->SetProgress(0.25);
	m_pProgressLabel->SetText("#TrackerUI_SendingDataToServer");

	// register for "Tick" messages
	ivgui()->AddTickSignal(this->GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the user has entered enough info to be allowed to continue
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelCreateUser3::VerifyEntriesAreValid()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when the text in one of the text entries changes
//-----------------------------------------------------------------------------
void CSubPanelCreateUser3::OnTextChanged()
{
	if (IsVisible())
	{
		GetWizardPanel()->SetNextButtonEnabled(VerifyEntriesAreValid());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : WizardSubPanel
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelCreateUser3::GetNextSubPanel()
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Tells the wizard to jump back to the enter email address screen (for when create user fails)
// Input  : 
// Output : WizardSubPanel
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelCreateUser3::GetPrevSubPanel()
{
	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelCreateUser"));
}

//-----------------------------------------------------------------------------
// Purpose: Network message handler
// Input  : *replyMsg - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CSubPanelCreateUser3::OnUserCreated(vgui::KeyValues *data)
{
	m_pProgressLabel->SetText("#TrackerUI_UserCreateComplete");
	m_pProgressBar->SetProgress(1.0f);
	m_iTimeoutTime = 0;

	KeyValues *wizData = GetWizardData();

	if (data->GetInt("newUID") < 1)
	{
		// we've been thrown back a bad ID
		OnUserCreateDenied(data);
		return;
	}

	// save our new details in the doc
	KeyValues *appData = ::GetDoc()->Data()->FindKey("App", true);
	appData->SetInt("UID", data->GetInt("newUID"));
	
	// save the password automatically (this could become an option later)
	appData->SetString("password", wizData->GetString("password"));

	// save user details
	KeyValues *userData = ::GetDoc()->Data()->FindKey("User", true);
	userData->SetString("UserName", wizData->GetString("UserName"));
	userData->SetString("FirstName", wizData->GetString("FirstName"));
	userData->SetString("LastName", wizData->GetString("LastName"));
	userData->SetString("email", wizData->GetString("email"));
	userData->SetString("password", wizData->GetString("password"));

	// user can't turn back now
	GetWizardPanel()->SetPrevButtonEnabled(false);
	GetWizardPanel()->SetNextButtonEnabled(false);
	GetWizardPanel()->SetCancelButtonEnabled(false);
	GetWizardPanel()->SetFinishButtonEnabled(true);

	GetWizardPanel()->InvalidateLayout(false);
	GetWizardPanel()->ResetKeyFocus();

	// finish up the dialog
	wchar_t unicode[256], unicodeName[64];
	localize()->ConvertANSIToUnicode(GetWizardData()->GetString("username"), unicodeName, sizeof( unicodeName ) / sizeof( wchar_t ));
	localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_WelcomeToTracker_Name"), 1, unicodeName);
	
	m_pLabel->SetText(unicode);

	// setup the message to be sent when we're finished
	m_pFinishedMessage = new KeyValues("UserCreateFinished");
	m_pFinishedMessage->SetInt("userid", data->GetInt("newUID"));
	m_pFinishedMessage->SetString("password", GetWizardData()->GetString("password"));
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *data - 
//-----------------------------------------------------------------------------
void CSubPanelCreateUser3::OnUserCreateDenied(vgui::KeyValues *data)
{
	GetWizardPanel()->SetPrevButtonEnabled(true);
	GetWizardPanel()->ResetKeyFocus();
	m_iTimeoutTime = 0;

	// change the text message to be the error
	m_pLabel->SetText(data->GetString("error"));

	m_pProgressLabel->SetText("#TrackerUI_CreateUserFailed");
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user hits the finish button
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelCreateUser3::OnFinishButton()
{
	if (!m_pFinishedMessage)
		return false;

	// write into the doc
	GetWizardPanel()->PostActionSignal(m_pFinishedMessage);
	m_pFinishedMessage = NULL;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Changes the dialog to report the failure state
//-----------------------------------------------------------------------------
void CSubPanelCreateUser3::OnTimeout()
{
	// change the text message to be the error
	m_pLabel->SetText("#TrackerUI_CouldNotConnectToServer");
	m_pProgressLabel->SetText("#TrackerUI_CreateUserFailed");

	GetWizardPanel()->SetCancelButtonEnabled(true);
	GetWizardPanel()->ResetKeyFocus();

	m_iTimeoutTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Advances the status bar if we haven't got a reply yet
//-----------------------------------------------------------------------------
void CSubPanelCreateUser3::OnTick()
{
	if (m_iTimeoutTime)
	{
		int currentTime = system()->GetTimeMillis();

		// check for timeout
		if (currentTime > m_iTimeoutTime)
		{
			// timed out
			OnTimeout();
		}
		else
		{
			// advance the status bar, in the range 25% to 75%
			float timePassedPercentage = (float)(currentTime - m_iTimeoutTime + CREATEUSER_TIMEOUT) / (float)CREATEUSER_TIMEOUT;
			m_pProgressBar->SetProgress((timePassedPercentage / 2) + 0.25f);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelCreateUser3::m_MessageMap[] =
{
	MAP_MSGID_PARAMS( CSubPanelCreateUser3, TSVC_USERCREATED, OnUserCreated ),		// network message
	MAP_MSGID_PARAMS( CSubPanelCreateUser3, TSVC_USERCREATEDENIED, OnUserCreateDenied ),	// network message

	MAP_MESSAGE( CSubPanelCreateUser3, "TextChanged", OnTextChanged ),	// custom message
};
IMPLEMENT_PANELMAP(CSubPanelCreateUser3, Panel);

