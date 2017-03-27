//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "Buddy.h"
#include "DialogUserInfo.h"
#include "OnlineStatus.h"
#include "ServerSession.h"
#include "SubPanelUserInfoDetails.h"
#include "SubPanelUserInfoStatus.h"
#include "Tracker.h"
#include "TrackerDoc.h"
#include "TrackerProtocol.h"

#include <VGUI_Button.h>
#include <VGUI_TextEntry.h>

#include <stdio.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : userID - 
//-----------------------------------------------------------------------------
CDialogUserInfo::CDialogUserInfo(int userID) : PropertyDialog(NULL, NULL)
{
	SetBounds(0, 0, 400, 450);

	m_iUserID = userID;

	AddPage(new CSubPanelUserInfoDetails(m_iUserID), "Personal");
	AddPage(new CSubPanelUserInfoStatus(m_iUserID), "Status");

	ServerSession().AddNetworkMessageWatch(this, TSVC_FRIENDINFO);

	int wide, tall;
	GetSize(wide, tall);

	GetDoc()->LoadDialogState(this, "UserInfo", m_iUserID);

	SetSize(wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogUserInfo::~CDialogUserInfo()
{
	ServerSession().RemoveNetworkMessageWatch(this);
}

//-----------------------------------------------------------------------------
// Purpose: Activates the dialog
//-----------------------------------------------------------------------------
void CDialogUserInfo::Open()
{
	Activate();
	Panel *panel = FindChildByName("OK");

	if (panel)
	{
		panel->RequestFocus();
	}
	RequestFocus();

	const char *firstName = GetDoc()->GetBuddy(m_iUserID)->Data()->GetString("FirstName", NULL);
	if (!firstName)
	{
		ServerSession().RequestUserInfoFromServer(m_iUserID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Deletes self when closed
//-----------------------------------------------------------------------------
void CDialogUserInfo::OnClose()
{
	GetDoc()->SaveDialogState(this, "UserInfo", m_iUserID);
	BaseClass::OnClose();
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panelName - 
//			*valueName - 
//-----------------------------------------------------------------------------
void CDialogUserInfo::SetChildText(const char *panelName, const char *valueName)
{
	TextEntry *text = dynamic_cast<TextEntry *>(FindChildByName(panelName));
	if (text)
	{
		char buf[512];
		strncpy(buf, GetDoc()->GetBuddy(m_iUserID)->Data()->GetString(valueName, ""), sizeof(buf) - 1);
		buf[511] = 0;
		text->SetText(buf);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDialogUserInfo::OnReceivedFriendInfo()
{
	InvalidateLayout();
	ResetAllData();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDialogUserInfo::PerformLayout()
{
	char buf[512];
	sprintf(buf, "%s - User Details", GetDoc()->GetBuddy(m_iUserID)->DisplayName());
	SetTitle(buf, true);

//tagES
//	wchar_t unicode[256], unicodeName[64];
//	localize()->ConvertANSIToUnicode(GetDoc()->GetBuddy(m_iUserID)->DisplayName(), unicodeName, sizeof( unicodeName ) / sizeof( wchar_t ));
//	localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_UserDetailsTitle"), 1, unicodeName );
//	SetTitle(unicode, true);

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CDialogUserInfo::m_MessageMap[] =
{
	MAP_MSGID( CDialogUserInfo, TSVC_FRIENDINFO, OnReceivedFriendInfo ),
};

IMPLEMENT_PANELMAP(CDialogUserInfo, vgui::Frame);
