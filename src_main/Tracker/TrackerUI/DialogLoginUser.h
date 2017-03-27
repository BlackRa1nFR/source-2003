//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef DIALOGLOGINUSER_H
#define DIALOGLOGINUSER_H
#pragma once

#include <VGUI_Panel.h>
#include <VGUI_TextEntry.h>
#include <VGUI_CheckButton.h>
#include <VGUI_WizardPanel.h>

//-----------------------------------------------------------------------------
// Purpose: Login user dialog
//-----------------------------------------------------------------------------
class CDialogLoginUser : public vgui::WizardPanel
{
public:
	CDialogLoginUser();
	~CDialogLoginUser();

	// starts the wizard
	virtual void Run(bool performConnectionTest);

private:
	// quits the app
	virtual void OnCancelButton();

};


#endif // DIALOGLOGINUSER_H
