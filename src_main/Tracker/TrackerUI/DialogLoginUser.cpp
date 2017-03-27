//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================


#include <VGUI_KeyValues.h>
#include <VGUI_WizardSubPanel.h>

#include "DialogLoginUser.h"
#include "TrackerDialog.h"
#include "TrackerDoc.h"

#include "SubPanelConnectionIntro.h"
#include "SubPanelConnectionTest.h"
#include "SubPanelSelectLoginOption.h"
#include "SubPanelCreateUser.h"
#include "SubPanelCreateUser2.h"
#include "SubPanelCreateUser3.h"
#include "SubPanelLoginUser.h"
#include "SubPanelLoginUser2.h"
#include "SubPanelError.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDialogLoginUser::CDialogLoginUser() : WizardPanel(NULL, "DialogLoginUser")
{
	// set up all our sub panels
	SetBounds(0, 0, 480, 360);
	WizardSubPanel *subPanel = new CSubPanelSelectLoginOption(this, "SubPanelSelectLoginOption");
	subPanel->SetVisible(false);

	subPanel = new CSubPanelConnectionIntro(this, "SubPanelConnectionIntro");
	subPanel->SetVisible(false);

	subPanel = new CSubPanelConnectionTest(this, "SubPanelConnectionTest");
	subPanel->SetVisible(false);
	
	subPanel = new CSubPanelCreateUser(this, "SubPanelCreateUser");
	subPanel->SetVisible(false);

	subPanel = new CSubPanelCreateUser2(this, "SubPanelCreateUser2");
	subPanel->SetVisible(false);

	subPanel = new CSubPanelCreateUser3(this, "SubPanelCreateUser3");
	subPanel->SetVisible(false);

	subPanel = new CSubPanelLoginUser(this, "SubPanelLoginUser");
	subPanel->SetVisible(false);
	
	subPanel = new CSubPanelLoginUser2(this, "SubPanelLoginUser2");
	subPanel->SetVisible(false);

	subPanel = new CSubPanelError(this, "SubPanelError");
	subPanel->SetVisible(false);

	GetDoc()->LoadDialogState(this, "Login");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogLoginUser::~CDialogLoginUser()
{
}

//-----------------------------------------------------------------------------
// Purpose: Activates the dialog
//-----------------------------------------------------------------------------
void CDialogLoginUser::Run(bool performConnectionTest)
{
	WizardSubPanel *startPanel;
	if (performConnectionTest)
	{
		startPanel = dynamic_cast<WizardSubPanel *>(FindChildByName("SubPanelConnectionIntro"));
	}
	else
	{
		startPanel = dynamic_cast<WizardSubPanel *>(FindChildByName("SubPanelSelectLoginOption"));
	}

	MoveToFront();
	RequestFocus();

	WizardPanel::Run(startPanel);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CDialogLoginUser::OnCancelButton()
{
	WizardPanel::OnCancelButton();

	// send a message to quit the whole program - no login, no tracker
	PostActionSignal(new KeyValues("UserCreateCancel"));
}


