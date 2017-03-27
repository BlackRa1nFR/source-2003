//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include "DialogAddBan.h"

#include <VGUI_Button.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_TextEntry.h>
#include <VGUI_CheckButton.h>
#include <VGUI_MessageBox.h>

#include <VGUI_Controls.h>
#include <VGUI_ISurface.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDialogAddBan::CDialogAddBan() : Frame(NULL, "DialogAddBan")
{
	SetSize(320, 200);


	m_pIDTextEntry = new TextEntry(this, "IDTextEntry");
	m_pTimeTextEntry = new TextEntry(this, "TimeTextEntry");

	m_pOkayButton = new Button(this, "OkayButton", "&Okay");
	//m_pCvarEntry->setTextHidden(true);

	LoadControlSettings("Admin\\DialogAddBan.res");

	SetTitle("Enter new Ban", true);

	// set our initial position in the middle of the workspace
	MoveToCenterOfScreen();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogAddBan::~CDialogAddBan()
{
}


//-----------------------------------------------------------------------------
// Purpose: initializes the dialog and brings it to the foreground
//-----------------------------------------------------------------------------
void CDialogAddBan::Activate(const char *type)
{

	m_cType=type;

	m_pOkayButton->SetAsDefaultButton(true);
	MakePopup();
	MoveToFront();

	RequestFocus();
	m_pIDTextEntry->RequestFocus();
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a labell by name
//-----------------------------------------------------------------------------
void CDialogAddBan::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

bool CDialogAddBan::IsIPCheck()
{
	CheckButton *entry = dynamic_cast<CheckButton *>(FindChildByName("IPCheck"));
	if (entry)
	{
		return entry->IsSelected();
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CDialogAddBan::OnCommand(const char *command)
{
	bool bClose = false;

	if (!stricmp(command, "Okay"))
	{
		KeyValues *msg = new KeyValues("AddBanValue");
		char buf[64];
		m_pTimeTextEntry->GetText(0,buf,64);
		float time;
		sscanf(buf,"%f",&time);
		if(time<0) 
		{
			MessageBox *dlg = new MessageBox("Add Ban Error", "The time you entered is invalid. \nIt must be equal to or greater than zero.");
			dlg->DoModal();
			bClose=false;
		}
		else
		{
			msg->SetString("time", buf );
			m_pIDTextEntry->GetText(0, buf, sizeof(buf)-1);
			msg->SetString("id", buf);
			msg->SetString("type",m_cType);
			msg->SetInt("ipcheck",IsIPCheck());

			PostActionSignal(msg);

			bClose = true;
		}
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
void CDialogAddBan::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: deletes the dialog on close
//-----------------------------------------------------------------------------
void CDialogAddBan::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}




