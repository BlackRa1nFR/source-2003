//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef DIALOGFINDBUDDY_H
#define DIALOGFINDBUDDY_H
#pragma once

#include <VGUI_WizardPanel.h>

//-----------------------------------------------------------------------------
// Purpose: Wizard for searching for a buddy
//-----------------------------------------------------------------------------
class CDialogFindBuddy : public vgui::WizardPanel
{
public:
	CDialogFindBuddy();
	~CDialogFindBuddy();

	// activates the wizard
	virtual void Run(void);

	// brings to front
	virtual void Open();


private:

};


#endif // DIALOGFINDBUDDY_H
