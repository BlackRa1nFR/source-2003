//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "HelpDialog.h"

#include <KeyValues.h>
#include <vgui/ISystem.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Label.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CHelpDialog::CHelpDialog() : CTaskFrame(NULL, "HelpDialog")
{
	g_pTaskbar->AddTask(GetVPanel());


	m_pInfoText = new Label(this, "InfoText", "#GameUI_InfoText");
	m_pClose = new Button(this, "CloseButton", "#GameUI_Close");
	m_pNeverShowAgain = new CheckButton(this, "NeverShowButton", "#GameUI_NeverShowButton");

	SetTitle("#GameUI_HalfLifeDesktopHelp", true);

	LoadControlSettings("Resource/DialogHelpIngame.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHelpDialog::~CHelpDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHelpDialog::OnClose()
{
	BaseClass::OnClose();

	if (m_pNeverShowAgain->IsSelected())
	{
		// write out that we should never show this again
		system()->SetRegistryInteger("HKEY_CURRENT_USER\\Software\\Valve\\Tracker\\NeverShowHelp", 1);
	}

	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *close - 
//-----------------------------------------------------------------------------
void CHelpDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "Close"))
	{
		PostMessage(this, new KeyValues("Close"));
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

