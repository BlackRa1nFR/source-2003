//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include "DialogCvarChange.h"

#include <VGUI_Button.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_TextEntry.h>
#include <VGUI_IInput.h>

#include <VGUI_Controls.h>
#include <VGUI_ISurface.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDialogCvarChange::CDialogCvarChange() : Frame(NULL, "DialogCvarChange")
{
	SetSize(320, 200);

	m_bAddCvarText = true;

	m_pInfoLabel = new Label(this, "InfoLabel", "");
	m_pCvarLabel = new Label(this, "CvarLabel", "");
	m_pCvarEntry = new TextEntry(this, "CvarEntry");
	m_pOkayButton = new Button(this, "OkayButton", "&Okay");
	//m_pCvarEntry->setTextHidden(true);

	LoadControlSettings("Admin\\DialogCvarChange.res");

	SetTitle("Enter new CVAR value", true);

	// set our initial position in the middle of the workspace
	MoveToCenterOfScreen();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogCvarChange::~CDialogCvarChange()
{
}

void CDialogCvarChange::MakePassword()
{
	m_pCvarEntry->SetTextHidden(true);
	m_bAddCvarText = false; // this isn't asking about a cvar
//	onTop();
}

//-----------------------------------------------------------------------------
// Purpose: initializes the dialog and brings it to the foreground
//-----------------------------------------------------------------------------
void CDialogCvarChange::Activate(const char *cvarName, const char *curValue, const char *type, const char *question)
{
	m_pCvarLabel->SetText(cvarName);
	if (! m_bAddCvarText ) 
	{
		m_pCvarLabel->SetVisible(false); // hide this
	}

	m_pInfoLabel->SetText(question);

	m_cType=type;

	m_pOkayButton->SetAsDefaultButton(true);
	MakePopup();
	MoveToFront();

	m_pCvarEntry->SetText(curValue);
	m_pCvarEntry->RequestFocus();
	RequestFocus();

	// make it modal
	input()->SetAppModalSurface(GetVPanel());

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a labell by name
//-----------------------------------------------------------------------------
void CDialogCvarChange::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CDialogCvarChange::OnCommand(const char *command)
{
	bool bClose = false;

	if (!stricmp(command, "Okay"))
	{
		KeyValues *msg = new KeyValues("CvarChangeValue");
		char buf[64];
		m_pCvarLabel->GetText(buf,64);

		msg->SetString("player", buf );
		m_pCvarEntry->GetText(0, buf, sizeof(buf)-1);
		msg->SetString("value", buf);
		msg->SetString("type",m_cType);

		PostActionSignal(msg);

		bClose = true;
	}
	else if (!stricmp(command, "Close"))
	{
		bClose = true;
	}
	else
	{
		BaseClass::OnCommand(command);
	}

	if (bClose)
	{
		PostMessage(this, new KeyValues("Close"));
		MarkForDeletion();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDialogCvarChange::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: deletes the dialog on close
//-----------------------------------------------------------------------------
void CDialogCvarChange::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}




