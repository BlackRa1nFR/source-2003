//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "DialogAbout.h"
#include "TrackerDoc.h"

#include <VGUI_Button.h>
#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_TextEntry.h>

#include <stdio.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDialogAbout::CDialogAbout() : Frame(NULL, "DialogAbout")
{
	m_pClose = new Button(this, "CloseButton", "#TrackerUI_Close");
	m_pText = new TextEntry(this, "AboutText");

	m_pText->SetRichEdit(true);
	m_pText->SetMultiline(true);
	m_pText->SetVerticalScrollbar(true);

	SetTitle("#TrackerUI_FriendsAboutTitle", true);

	LoadControlSettings("Friends/DialogAbout.res");

	GetDoc()->LoadDialogState(this, "About");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogAbout::~CDialogAbout()
{
	if (GetDoc())
	{
		GetDoc()->SaveDialogState(this, "About");
	}
}

//-----------------------------------------------------------------------------
// Purpose: activates the dialog, brings it to the front
//-----------------------------------------------------------------------------
void CDialogAbout::Run()
{
	// bring us to the foreground
	m_pClose->RequestFocus();
	RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: sets up the about text for drawing
//-----------------------------------------------------------------------------
void CDialogAbout::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pText->SetText("");

	m_pText->DoInsertString("Friends - Copyright © 2001 Valve LLC\n\nInstalled version: ");

	char versionString[32];
	if (!system()->GetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Tracker\\Version", versionString, sizeof(versionString)))
	{
		strcpy(versionString, "<unknown>");
	}

	m_pText->DoInsertString(versionString);

	m_pText->DoInsertString("\nBuild number: ");

	char buildString[32];
	extern int build_number( void );
	sprintf(buildString, "%d", build_number());

	m_pText->DoInsertString(buildString);
	m_pText->DoInsertString("\n\n");

	// beta
	m_pText->DoInsertString("This software is BETA and is not for public release.\n");
	m_pText->DoInsertString("Please report any bugs or issues to:\n  trackerbugs@valvesoftware.com\n");
}

//-----------------------------------------------------------------------------
// Purpose: dialog deletes itself on close
//-----------------------------------------------------------------------------
void CDialogAbout::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}
