//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelTrackerState.h"

#include <VGUI_Controls.h>
#include <VGUI_IScheme.h>

#include <VGUI_TextEntry.h>
#include <VGUI_Button.h>

#include "TrackerDoc.h"
#include "ServerSession.h"
#include "OnlineStatus.h"

#define min(a, b)  (((a) < (b)) ? (a) : (b)) 

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelTrackerState::CSubPanelTrackerState(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
	m_pMessage = new TextEntry(this, "Message");
	m_pMessage->SetMultiline(true);
	m_pMessage->SetRichEdit(true);
	m_pSignInButton = new Button(this, "SigninButton", "");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelTrackerState::~CSubPanelTrackerState()
{
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches button commands
//-----------------------------------------------------------------------------
void CSubPanelTrackerState::OnCommand(const char *command)
{
	if (!stricmp("Cancel", command))
	{
		ServerSession().CancelConnect();
		InvalidateLayout();
	}
	else if (!stricmp("Signin", command))
	{
		ServerSession().SendInitialLogin(COnlineStatus::ONLINE);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates the data on the page
//-----------------------------------------------------------------------------
void CSubPanelTrackerState::PerformLayout()
{
	BaseClass::PerformLayout();

	int pwide, ptall;
	GetSize(pwide, ptall);

	m_pMessage->SetBounds(6, 6, min(pwide - 12, 156), 84);
	m_pSignInButton->SetBounds(6, 96, 64, 24);
	m_pSignInButton->SetVisible(true);

	// update which buttons we show
	if (ServerSession().IsConnectingToServer())
	{
		// show the connecting 
		m_pMessage->SetText("#TrackerUI_SigningInToFriendsNetwork");
		m_pSignInButton->SetText("#TrackerUI_Cancel");
		m_pSignInButton->SetCommand("Cancel");
	}
	else if (!ServerSession().IsConnectedToServer())
	{
		// display the login screen
		m_pMessage->SetText("#TrackerUI_NotSignedInToFriendsNetwork");
		m_pSignInButton->SetText("#TrackerUI_SignIn");
		m_pSignInButton->SetCommand("Signin");
	}
	else
	{
		// we must be connected
		m_pMessage->SetText("#TrackerUI_SignInSuccessful");
		m_pSignInButton->SetVisible(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelTrackerState::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBgColor(GetSchemeColor("BuddyListBgColor", GetBgColor()));
	SetBorder(scheme()->GetBorder(GetScheme(), "ButtonDepressedBorder"));

	// force the message box to have no border
	m_pMessage->InvalidateLayout(true);
	m_pMessage->SetBorder(NULL);
	m_pMessage->SetFgColor(GetSchemeColor("LabelDimText"));
}
