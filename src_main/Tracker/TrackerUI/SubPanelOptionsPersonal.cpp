//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "ServerSession.h"
#include "SubPanelOptionsPersonal.h"
#include "Tracker.h"
#include "TrackerDialog.h"
#include "TrackerDoc.h"

#include <VGUI_KeyValues.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelOptionsPersonal::CSubPanelOptionsPersonal()
{
	LoadControlSettings("Friends/SubPanelOptionsPersonal.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelOptionsPersonal::~CSubPanelOptionsPersonal()
{
}

//-----------------------------------------------------------------------------
// Purpose: Loads data from doc
//-----------------------------------------------------------------------------
void CSubPanelOptionsPersonal::OnResetData()
{
	vgui::KeyValues *docData = GetDoc()->Data()->FindKey("User", true);

	SetControlString("UserNameEdit", docData->GetString("UserName", ""));
	SetControlString("FirstNameEdit", docData->GetString("FirstName", ""));
	SetControlString("LastNameEdit", docData->GetString("LastName", ""));
	SetControlString("EmailEdit", docData->GetString("Email", ""));
}

//-----------------------------------------------------------------------------
// Purpose: Writes data to doc
//-----------------------------------------------------------------------------
void CSubPanelOptionsPersonal::OnApplyChanges()
{
	KeyValues *docData = GetDoc()->Data()->FindKey("User", true);

	// if anything has changed, reset data on server
	if (stricmp(GetControlString("UserNameEdit", ""), docData->GetString("UserName")) 
		|| stricmp(GetControlString("FirstNameEdit", ""), docData->GetString("FirstName")) 
		|| stricmp(GetControlString("LastNameEdit", ""), docData->GetString("LastName")))
	{
		// update document
		docData->SetString("UserName", GetControlString("UserNameEdit", ""));
		docData->SetString("FirstName", GetControlString("FirstNameEdit", ""));
		docData->SetString("LastName", GetControlString("LastNameEdit", ""));

		// upload
		ServerSession().UpdateUserInfo(GetDoc()->GetUserID(), docData->GetString("UserName"), docData->GetString("FirstName"), docData->GetString("LastName"));

		// redraw local
		CTrackerDialog::GetInstance()->OnFriendsStatusChanged(1);
	}
}


