//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "DialogAuthRequest.h"
#include "TrackerDoc.h"
#include "ServerSession.h"
#include "Buddy.h"
#include "OnlineStatus.h"
#include "Tracker.h"
#include "TrackerProtocol.h"

#include <VGUI_Button.h>
#include <VGUI_Controls.h>
#include <VGUI_ILocalize.h>
#include <VGUI_IVGui.h>
#include <VGUI_Label.h>
#include <VGUI_RadioButton.h>

#include <stdio.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDialogAuthRequest::CDialogAuthRequest() : Frame(NULL, "DialogAuthRequest")
{
	SetBounds(0, 0, 240, 400);

	m_pInfoText = new Label(this, "InfoText", "InfoText");
	m_pNameLabel = new Label(this, "NameLabel", "NameLabel");
	m_pAcceptButton = new RadioButton(this, "AcceptButton", "#TrackerUI_Accept");
	m_pDeclineButton = new RadioButton(this, "DeclineButton", "#TrackerUI_Decline");
	m_pOKButton = new Button(this, "OKButton", "#TrackerUI_OK");
	m_pCancelButton = new Button(this, "CancelButton", "#TrackerUI_Cancel");

	LoadControlSettings("Friends/DialogAuthRequest.res");

	m_pAcceptButton->SetText("#TrackerUI_AllowPersonToSeeYou");
	m_pDeclineButton->SetText("#TrackerUI_BlockThisPerson");

	SetSizeable(false);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogAuthRequest::~CDialogAuthRequest()
{
}

//-----------------------------------------------------------------------------
// Purpose: Reads the authorization request
// Input  : buddyID - 
//-----------------------------------------------------------------------------
void CDialogAuthRequest::ReadAuthRequest(int buddyID)
{
	m_iTargetID = buddyID;

	KeyValues *buddy = GetDoc()->GetBuddy(buddyID)->Data();

	// setup the screen text
	m_pInfoText->SetText("#TrackerUI_DoYouWantTo");

	const char *reason = buddy->GetString("Reason", NULL);
	const char *userName = buddy->GetString("UserName", "");

	wchar_t unicode[256], unicodeName[64];
	localize()->ConvertANSIToUnicode(userName, unicodeName, sizeof( unicodeName ) / sizeof( wchar_t ));
	if (reason)
	{
		wchar_t unicodeReason[128];
		localize()->ConvertANSIToUnicode(reason, unicodeReason, sizeof( unicodeReason ) / sizeof( wchar_t ));
		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_WishesToAddToContactList_Name_Reason"), 2, unicodeName, unicodeReason);
	}
	else
	{
		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_WishesToAddToContactList_Name"), 1, unicodeName);
	}
	m_pNameLabel->SetText(unicode);


	char buf[384];
	sprintf(buf, "%s - Authorization Request", userName);

	SetTitle(buf, true);

//tagES
//	wchar_t unicodeTitle[384];
//	localize()->ConstructString(unicodeTitle, sizeof( unicodeTitle ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_AuthRequestTitle"), 1, unicodeName);
//	SetTitle(unicodeTitle, true);

	// default to accepting
	m_pAcceptButton->SetSelected(true);
	GetFocusNavGroup().SetDefaultButton(m_pOKButton);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CDialogAuthRequest::OnCommand(const char *command)
{
	bool bClose = false;
	if (!stricmp(command, "Decline") || !stricmp(command, "Accept"))
	{
		// radio buttons have been toggled
	}
	else if (!stricmp(command, "OK"))
	{
		if (m_pAcceptButton->IsSelected())
		{
			AcceptAuth();
		}
		else if (m_pDeclineButton->IsSelected())
		{
			DenyAuth();
		}

		bClose = true;
	}
	else if (!stricmp(command, "Cancel"))
	{
		bClose = true;
	}

	if (bClose)
	{
		ivgui()->PostMessage(this->GetVPanel(), new KeyValues("Close"), this->GetVPanel());

		// signal that the friends status has changed
		PostActionSignal(new KeyValues("Friends", "_id", TSVC_FRIENDS, "count", 1));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDialogAuthRequest::AcceptAuth()
{
	// send acceptance message to user
	ServerSession().SendAuthUserMessage(m_iTargetID, true);

	// move the user out of requesting auth status
	KeyValues *dat = GetDoc()->GetBuddy(m_iTargetID)->Data();
	if (dat)
	{
		dat->SetInt("Status", COnlineStatus::OFFLINE);
		dat->SetInt("RequestingAuth", 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDialogAuthRequest::DenyAuth()
{
	// send a decline message
	ServerSession().SendAuthUserMessage(m_iTargetID, false);

	//!! remove the user from the list
	KeyValues *dat = GetDoc()->GetBuddy(m_iTargetID)->Data();
	if (dat)
	{
		dat->SetInt("Status", COnlineStatus::REMOVED);
		dat->SetInt("RequestingAuth", 0);
		dat->SetInt("Removed", 1);
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: Delets the dialog upon close
//-----------------------------------------------------------------------------
void CDialogAuthRequest::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}
