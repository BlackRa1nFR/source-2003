//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "CreateTokenDialog.h"

#include <VGUI_TextEntry.h>
#include <VGUI_Button.h>
#include <VGUI_MessageBox.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Controls.h>
#include <VGUI_ILocalize.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Cosntructor
//-----------------------------------------------------------------------------
CCreateTokenDialog::CCreateTokenDialog() : Frame(NULL, "CreateTokenDialog")
{
	MakePopup();

	SetTitle("Create New Token - Localizer", true);

	m_pSkipButton = new Button(this, "SkipButton", "&Skip Token");
	m_pTokenName = new TextEntry(this, "TokenName");

	m_pTokenValue = new TextEntry(this, "TokenValue");
	m_pTokenValue->SetMultiline(true);
	m_pTokenValue->SetCatchEnterKey(true);
	
	m_pSkipButton->SetCommand("SkipToken");
	m_pSkipButton->SetVisible(false);

	LoadControlSettings("Resource/CreateTokenDialog.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCreateTokenDialog::~CCreateTokenDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: prompts user to create a single token
//-----------------------------------------------------------------------------
void CCreateTokenDialog::CreateSingleToken()
{
	// bring us to the front
	SetVisible(true);
	RequestFocus();
	MoveToFront();
	m_pTokenName->RequestFocus();
	m_pSkipButton->SetVisible(false);

	m_bMultiToken = false;
}

//-----------------------------------------------------------------------------
// Purpose: loads a file to create multiple tokens
//-----------------------------------------------------------------------------
void CCreateTokenDialog::CreateMultipleTokens()
{
	SetVisible(true);
	RequestFocus();
	MoveToFront();
	m_pTokenValue->RequestFocus();
	m_pSkipButton->SetVisible(true);

	m_bMultiToken = true;

	//!! read tokens from file, prompt user to each in turn
}

//-----------------------------------------------------------------------------
// Purpose: Handles an OK message, creating the current token
//-----------------------------------------------------------------------------
void CCreateTokenDialog::OnOK()
{
	// get the data
	char tokenName[1024], tokenValue[1024];
	m_pTokenName->GetText(0, tokenName, 1023);
	m_pTokenValue->GetText(0, tokenValue, 1023);

	if (strlen(tokenName) < 4)
	{
		MessageBox *box = new MessageBox("Create Token Error", "Could not create token.\nToken names need to be at least 4 characters long.");
		box->DoModal();
	}
	else
	{
		// create the token
		wchar_t unicodeString[1024];
		vgui::localize()->ConvertANSIToUnicode(tokenValue, unicodeString, sizeof(unicodeString) / sizeof(wchar_t));
		vgui::localize()->AddString(tokenName, unicodeString);

		// notify the dialog creator
		PostActionSignal(new KeyValues("TokenCreated", "name", tokenName));

		// close
		if (!m_bMultiToken)
		{
			PostMessage(this, new KeyValues("Close"));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: skips the current token in the multitoken edit mode
//-----------------------------------------------------------------------------
void CCreateTokenDialog::OnSkip()
{
}

//-----------------------------------------------------------------------------
// Purpose: handles a button command
// Input  : *command - 
//-----------------------------------------------------------------------------
void CCreateTokenDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "OK"))
	{
		OnOK();
	}
	else if (!stricmp(command, "SkipToken"))
	{
		OnSkip();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles the close message
//-----------------------------------------------------------------------------
void CCreateTokenDialog::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}


