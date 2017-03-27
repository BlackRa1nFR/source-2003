//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelOptionsSounds.h"
#include "Tracker.h"
#include "TrackerDoc.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelOptionsSounds::CSubPanelOptionsSounds()
{
	LoadControlSettings("Friends/SubPanelOptionsSounds.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelOptionsSounds::~CSubPanelOptionsSounds()
{
}

//-----------------------------------------------------------------------------
// Purpose: Loads data from doc
//-----------------------------------------------------------------------------
void CSubPanelOptionsSounds::OnResetData()
{
	KeyValues *docData = GetDoc()->Data()->FindKey("User/Sounds", true);

	SetControlInt("IngameSoundCheck", docData->GetInt("Ingame", 1));
	SetControlInt("OnlineSoundCheck", docData->GetInt("Online", 0));
	SetControlInt("MessageSoundCheck", docData->GetInt("Message", 1));
}


//-----------------------------------------------------------------------------
// Purpose: Writes data to doc
//-----------------------------------------------------------------------------
void CSubPanelOptionsSounds::OnApplyChanges()
{
	KeyValues *docData = GetDoc()->Data()->FindKey("User/Sounds", true);

	docData->SetInt("Ingame", GetControlInt("IngameSoundCheck", docData->GetInt("Ingame", 1)));
	docData->SetInt("Online", GetControlInt("OnlineSoundCheck", docData->GetInt("Online", 0)));
	docData->SetInt("Message", GetControlInt("MessageSoundCheck", docData->GetInt("Message", 1)));
}

