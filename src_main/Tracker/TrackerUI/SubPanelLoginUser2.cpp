//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelLoginUser2.h"
#include "ServerSession.h"
#include "TrackerDoc.h"
#include "TrackerDialog.h"
#include "TrackerProtocol.h"
#include "Tracker.h"

#include <VGUI_Controls.h>
#include <VGUI_ILocalize.h>
#include <VGUI_ISystem.h>
#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_ProgressBar.h>
#include <VGUI_WizardPanel.h>

using namespace vgui;

static const int LOGINUSER_TIMEOUT = 5000;	// 5 second timeout

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSubPanelLoginUser2::CSubPanelLoginUser2(Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	m_pProgressBar = new ProgressBar(this, "ProgressBar");
	m_pLabel = new Label(this, "InfoText", "Login");
	m_pProgressLabel = new Label(this, "ProgressText", "ProgressLabel");

	m_iTimeoutTime = 0;
	m_bGotReply = false;

	// note that loading control settings should always come after creating new controls
	LoadControlSettings("Friends/SubPanelLoginUser2.res");

	ServerSession().AddNetworkMessageWatch(this, TSVC_USERVALID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSubPanelLoginUser2::~CSubPanelLoginUser2()
{
	ServerSession().RemoveNetworkMessageWatch(this);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *previousSubPanel - 
//			*wizardPanel - 
//-----------------------------------------------------------------------------
void CSubPanelLoginUser2::PerformLayout()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *previousSubPanel - 
//			*wizardPanel - 
//-----------------------------------------------------------------------------
void CSubPanelLoginUser2::OnDisplayAsNext()
{
	// layout
	GetWizardPanel()->SetTitle("#TrackerUI_FriendsLoggingInTitle", true);

	wchar_t unicode[256], unicodeEmail[64];
	localize()->ConvertANSIToUnicode(GetWizardData()->GetString("email"), unicodeEmail, sizeof( unicodeEmail ) / sizeof( wchar_t ));
	localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_AttemptingToLogin_Email"), 1, unicodeEmail);
	m_pLabel->SetText(unicode);

	m_pProgressLabel->SetText("#TrackerUI_ConnectingToServer");

	GetWizardPanel()->SetNextButtonEnabled(false);
	GetWizardPanel()->SetPrevButtonEnabled(false);
	GetWizardPanel()->SetCancelButtonEnabled(true);
	GetWizardPanel()->SetFinishButtonEnabled(false);

	// send off to the server for validation
	ServerSession().ValidateUserWithServer(GetWizardData()->GetString("email"), GetWizardData()->GetString("password"));

	// set timeout
	m_iTimeoutTime = system()->GetTimeMillis() + LOGINUSER_TIMEOUT;
	m_bGotReply = false;

	// register for the tick
	ivgui()->AddTickSignal(this->GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSubPanelLoginUser2::OnFinishButton()
{
	// send a message to the app to Start tracker with the user
	CTrackerDialog::GetInstance()->StartTrackerWithUser(GetDoc()->GetUserID());
	
	// returning true indicates it's time to close the wizard
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : WizardSubPanel
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelLoginUser2::GetNextSubPanel()
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *params - 
//-----------------------------------------------------------------------------
void CSubPanelLoginUser2::OnUserValid(KeyValues *params)
{
	int userID = params->GetInt("userID");

	if (userID > 0)
	{
		// kill timeout
		m_iTimeoutTime = 0;

		// valid user ID, proceed to checkout
		m_pLabel->SetText("#TrackerUI_ValidationSuccessful");
		GetWizardPanel()->SetFinishButtonEnabled(true);
		GetWizardPanel()->SetCancelButtonEnabled(false);
		GetWizardPanel()->ResetKeyFocus();

		// save out verified info to doc
		GetDoc()->Data()->SetInt("App/UID", params->GetInt("userID"));
		GetDoc()->Data()->SetString("App/password", GetWizardData()->GetString("password"));

		m_pProgressLabel->SetText("#TrackerUI_LoginSuccessful");
		m_pProgressBar->SetProgress(1.0f);

		return;
	}
	
	if (userID == -1)
	{
		// couldn't find user on one server, but keep waiting
		m_bGotReply = true;
		return;
	}

	// bad user
	// -1 is user doesn't exist
	// -2 is bad password
	if (userID == -2)
	{
		m_pLabel->SetText("#TrackerUI_PasswordInvalidRetry");
	}
	else
	{
		wchar_t unicode[256], unicodeEmail[64];
		localize()->ConvertANSIToUnicode(GetWizardData()->GetString("email"), unicodeEmail, sizeof( unicodeEmail ) / sizeof( wchar_t ));
		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_LoginDoesNotExist_Email"), 1, unicodeEmail);
		m_pLabel->SetText(unicode);
	}

	if (GetWizardPanel())
	{
		GetWizardPanel()->SetFinishButtonEnabled(false);
		GetWizardPanel()->SetPrevButtonEnabled(true);
	}

	m_iTimeoutTime = 0;
	m_pProgressLabel->SetText("#TrackerUI_LoginFailed");
}

//-----------------------------------------------------------------------------
// Purpose: Changes the dialog to report the failure state
//-----------------------------------------------------------------------------
void CSubPanelLoginUser2::OnTimeout()
{
	// change the text message to be the error
	if (m_bGotReply)
	{
		// we did get replies, but nothing good
		wchar_t unicode[256], unicodeEmail[64];
		localize()->ConvertANSIToUnicode(GetWizardData()->GetString("email"), unicodeEmail, sizeof( unicodeEmail ) / sizeof( wchar_t ));
		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_LoginDoesNotExist_Email"), 1, unicodeEmail);
		m_pLabel->SetText(unicode);
	}
	else
	{
		// never got any reply from server
		m_pLabel->SetText("#TrackerUI_LoginServerConnectFailed");
	}

	m_pProgressLabel->SetText("#TrackerUI_LoginFailed");

	GetWizardPanel()->SetCancelButtonEnabled(true);
	GetWizardPanel()->ResetKeyFocus();

	m_iTimeoutTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Advances the status bar if we haven't got a reply yet
//-----------------------------------------------------------------------------
void CSubPanelLoginUser2::OnTick()
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
			float timePassedPercentage = (float)(currentTime - m_iTimeoutTime + LOGINUSER_TIMEOUT) / (float)LOGINUSER_TIMEOUT;
			m_pProgressBar->SetProgress((timePassedPercentage / 2) + 0.25f);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelLoginUser2::m_MessageMap[] =
{
	// network message
	MAP_MSGID_PARAMS( CSubPanelLoginUser2, TSVC_USERVALID, OnUserValid ),
};
IMPLEMENT_PANELMAP(CSubPanelLoginUser2, Panel);
