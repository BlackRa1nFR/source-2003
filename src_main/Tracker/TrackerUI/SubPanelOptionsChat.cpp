//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelOptionsChat.h"

#include "Tracker.h"
#include "TrackerDoc.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelOptionsChat::CSubPanelOptionsChat()
{
	LoadControlSettings("Friends/SubPanelOptionsChat.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelOptionsChat::~CSubPanelOptionsChat()
{
}

//-----------------------------------------------------------------------------
// Purpose: Loads data from doc
//-----------------------------------------------------------------------------
void CSubPanelOptionsChat::OnResetData()
{
	KeyValues *docData = GetDoc()->Data()->FindKey("User", true);
	SetControlInt("ChatAlwaysOnTop", docData->GetInt("ChatAlwaysOnTop", 0));
}

//-----------------------------------------------------------------------------
// Purpose: Writes data to doc
//-----------------------------------------------------------------------------
void CSubPanelOptionsChat::OnApplyChanges()
{
	KeyValues *docData = GetDoc()->Data()->FindKey("User", true);
	docData->SetInt("ChatAlwaysOnTop", GetControlInt("ChatAlwaysOnTop", docData->GetInt("ChatAlwaysOnTop", 0)));
}
