//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "Buddy.h"
#include "DialogRemoveUser.h"
#include "Tracker.h"
#include "TrackerDialog.h"
#include "TrackerDoc.h"

#include <VGUI_Label.h>
#include <VGUI_Controls.h>
#include <VGUI_ILocalize.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDialogRemoveUser::CDialogRemoveUser(int userID) : vgui::Frame(NULL, "DialogRemoveUser")
{
	m_iUserID = userID;

	LoadControlSettings("Friends/DialogRemoveUser.res");

	GetDoc()->LoadDialogState(this, "RemoveUser");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogRemoveUser::~CDialogRemoveUser()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDialogRemoveUser::Open()
{
	Activate();
	PostMessageToChild("Cancel", new KeyValues("RequestFocus"));

	const char *name = GetDoc()->GetBuddy(m_iUserID)->DisplayName();

	char buf[255];
	sprintf(buf, "%s - Remove User", name);
	SetTitle(buf, true);

//tagES
	wchar_t unicode[256], unicodeName[64];
//	localize()->ConvertANSIToUnicode(name, unicodeName, sizeof( unicodeName ) / sizeof( wchar_t ));
//	localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_RemoveUserTitle"), 1, unicodeName );
//	SetTitle(unicode, true);

	Label *warning = dynamic_cast<Label *>(FindChildByName("WarningLabel"));
	if (warning)
	{
		localize()->ConvertANSIToUnicode(name, unicodeName, sizeof( unicodeName ) / sizeof( wchar_t ));
		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_RemoveWarningLabel_FriendName"), 1, unicodeName);
		warning->SetText(unicode);
	}

	Label *info = dynamic_cast<Label *>(FindChildByName("InfoLabel"));
	if (info)
	{
		info->SetText("#TrackerUI_WarningNoLongerSeeYou");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles OK/Cancel inputs
// Input  : *command - 
//-----------------------------------------------------------------------------
void CDialogRemoveUser::OnCommand(const char *command)
{
	if (!stricmp("OK", command))
	{
		GetDoc()->RemoveBuddy(m_iUserID);
		CTrackerDialog::GetInstance()->InvalidateLayout();
		PostMessage(this, new KeyValues("Close"));
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dialog deletes itself when closed
//-----------------------------------------------------------------------------
void CDialogRemoveUser::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}

