//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "DialogFindBuddy.h"

#include <VGUI_Controls.h>
#include <VGUI_ISurface.h>

#include "SubPanelFindBuddy.h"
#include "SubPanelFindBuddyResults.h"
#include "SubPanelFindBuddyRequestAuth.h"
#include "SubPanelFindBuddyComplete.h"
#include "TrackerDoc.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDialogFindBuddy::CDialogFindBuddy() : WizardPanel(NULL, "DialogFindBuddy")
{
	SetBounds(0, 0, 480, 360);

	WizardSubPanel *subPanel = new CSubPanelFindBuddy(this, "SubPanelFindBuddy");
	subPanel->SetVisible(false);

	subPanel = new CSubPanelFindBuddyResults(this, "SubPanelFindBuddyResults");
	subPanel->SetVisible(false);

	subPanel = new CSubPanelFindBuddyRequestAuth(this, "SubPanelFindBuddyRequestAuth");
	subPanel->SetVisible(false);

	subPanel = new CSubPanelFindBuddyComplete(this, "SubPanelFindBuddyComplete");
	subPanel->SetVisible(false);

	GetDoc()->LoadDialogState(this, "FindBuddy");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogFindBuddy::~CDialogFindBuddy()
{
	if (GetDoc())
	{
		GetDoc()->SaveDialogState(this, "FindBuddy");
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDialogFindBuddy::Run( void )
{
	SetVisible(true);

	WizardPanel::Run(dynamic_cast<WizardSubPanel *>(FindChildByName("SubPanelFindBuddy")));

	SetTitle("#TrackerUI_AddFriendsTitle", true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDialogFindBuddy::Open()
{
	Activate();
	surface()->SetMinimized(this->GetVPanel(), false);
}
