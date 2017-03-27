//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "Buddy.h"
#include "DialogRemoveUser.h"
#include "OnlineStatus.h"
#include "SubPanelUserInfoStatus.h"
#include "Tracker.h"
#include "TrackerDialog.h"
#include "TrackerDoc.h"

#include <VGUI_Controls.h>
#include <VGUI_ISurface.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : userID - 
//-----------------------------------------------------------------------------
CSubPanelUserInfoStatus::CSubPanelUserInfoStatus(int userID)
{
	m_iUserID = userID;

	LoadControlSettings("Friends/SubPanelUserInfoStatus.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelUserInfoStatus::~CSubPanelUserInfoStatus()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelUserInfoStatus::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Handles the remove user button
// Input  : *command - 
//-----------------------------------------------------------------------------
void CSubPanelUserInfoStatus::OnCommand(const char *command)
{
	if (!stricmp(command, "RemoveUser"))
	{
		// open the warning dialog
		CDialogRemoveUser *dialog = new CDialogRemoveUser(m_iUserID);
		vgui::surface()->CreatePopup(dialog->GetVPanel(), false);

		// center the dialog over this panel
		int x, y, wide, tall;
		vgui::surface()->GetScreenSize(wide, tall);
		x = wide / 2;
		y = tall / 2;

		dialog->GetSize(wide, tall);
		dialog->SetPos(x - (wide / 2), y - (tall / 2));

		dialog->Open();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when page is loaded.  Data should be reloaded from document into controls.
//-----------------------------------------------------------------------------
void CSubPanelUserInfoStatus::OnResetData()
{
	KeyValues *data = GetDoc()->GetBuddy(m_iUserID)->Data();

	SetControlInt("InvisibleCheck", data->GetInt("RemoteBlock"));
	SetControlInt("SoundIngameCheck", data->GetInt("SoundIngame"));
	SetControlInt("SoundOnlineCheck", data->GetInt("SoundOnline"));
	SetControlInt("NotifyCheck", data->GetInt("NotifyOnline"));
}

//-----------------------------------------------------------------------------
// Purpose: Called when the OK / Apply button is pressed.  Changed data should be written into document.
//-----------------------------------------------------------------------------
void CSubPanelUserInfoStatus::OnApplyChanges()
{
	KeyValues *data = GetDoc()->GetBuddy(m_iUserID)->Data();

	// block user status
	int remoteBlock = GetControlInt("InvisibleCheck", data->GetInt("RemoteBlock"));
	if (remoteBlock)
	{
		// set the block
		GetDoc()->GetBuddy(m_iUserID)->SetRemoteBlock(CBuddy::BLOCK_ONLINE);
	}
	else
	{
		// remove the block
		GetDoc()->GetBuddy(m_iUserID)->SetRemoteBlock(CBuddy::BLOCK_NONE);
	}

	data->SetInt("SoundIngame", GetControlInt("SoundIngameCheck", 0));
	data->SetInt("SoundOnline", GetControlInt("SoundOnlineCheck", 0));
	data->SetInt("NotifyOnline", GetControlInt("NotifyCheck", 0));
}
