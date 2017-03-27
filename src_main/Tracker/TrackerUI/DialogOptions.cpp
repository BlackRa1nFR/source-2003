//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "DialogOptions.h"
#include "SubPanelOptionsChat.h"
#include "SubPanelOptionsPersonal.h"
#include "SubPanelOptionsSounds.h"
#include "SubPanelOptionsConnection.h"
#include "Tracker.h"
#include "TrackerDoc.h"

#include <VGUI_PropertySheet.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDialogOptions::CDialogOptions() : PropertyDialog(NULL, "DialogOptions")
{
	SetBounds(0, 0, 400, 300);

	AddPage(new CSubPanelOptionsPersonal(), "Personal");
	AddPage(new CSubPanelOptionsSounds(), "Sounds");
	AddPage(new CSubPanelOptionsChat(), "Chat");
	AddPage(new CSubPanelOptionsConnection(), "Connection");

	GetPropertySheet()->SetTabWidth(72);

	GetDoc()->LoadDialogState(this, "Options");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogOptions::~CDialogOptions()
{
}

//-----------------------------------------------------------------------------
// Purpose: Opens the dialog
//-----------------------------------------------------------------------------
void CDialogOptions::Run()
{
	RequestFocus();
	SetVisible(true);
	MoveToFront();

	SetTitle("#TrackerUI_UserOptionsFriendsTitle", true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDialogOptions::OnClose()
{
	GetDoc()->SaveDialogState(this, "Options");
	BaseClass::OnClose();
	MarkForDeletion();
}
