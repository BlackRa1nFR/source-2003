//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "Buddy.h"
#include "ServerSession.h"
#include "SubPanelUserInfoDetails.h"
#include "Tracker.h"
#include "TrackerDoc.h"

#include <ctype.h>

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelUserInfoDetails::CSubPanelUserInfoDetails(int userID)
{
	m_iUserID = userID;
	LoadControlSettings("Friends/SubPanelUserInfoDetails.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelUserInfoDetails::~CSubPanelUserInfoDetails()
{
}

//-----------------------------------------------------------------------------
// Purpose: Lays out the dialogs controls
//-----------------------------------------------------------------------------
void CSubPanelUserInfoDetails::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CSubPanelUserInfoDetails::OnCommand(const char *command)
{
	if (!stricmp(command, "Refresh"))
	{
		// send away for new friend info
		ServerSession().RequestUserInfoFromServer(m_iUserID);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelUserInfoDetails::OnResetData()
{
	KeyValues *dat = GetDoc()->GetBuddy(m_iUserID)->Data();
	SetControlString("DisplayNameEdit", dat->GetString("DisplayName"));
	SetControlString("UserNameEdit", dat->GetString("UserName"));
	SetControlString("FirstNameEdit", dat->GetString("FirstName"));
	SetControlString("LastNameEdit", dat->GetString("LastName"));
	SetControlString("EmailEdit", dat->GetString("Email"));
}

//-----------------------------------------------------------------------------
// Purpose: Called when the OK / Apply button is pressed.  Changed data should be written into document.
//-----------------------------------------------------------------------------
void CSubPanelUserInfoDetails::OnApplyChanges()
{
	KeyValues *dat = GetDoc()->GetBuddy(m_iUserID)->Data();
	const char *displayName = GetControlString("DisplayNameEdit", NULL);
	if (displayName && isprint(displayName[0]))
	{
		dat->SetString("DisplayName", displayName);		
	}
}