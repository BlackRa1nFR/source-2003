//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


#include "LocalizationDialog.h"
#include "CreateTokenDialog.h"

#include <VGUI_Button.h>
#include <VGUI_Controls.h>
#include <VGUI_ListPanel.h>
#include <VGUI_TextEntry.h>
#include <VGUI_IVGui.h>
#include <VGUI_ILocalize.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Menu.h>
#include <VGUI_MenuButton.h>
#include <VGUI_MessageBox.h>
#include <VGUI_FileOpenDialog.h>

#include <stdio.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CLocalizationDialog::CLocalizationDialog(const char *fileName) : Frame(NULL, "LocalizationDialog")
{
	m_iCurrentToken = -1;

	m_pTokenList = new ListPanel(this, "TokenList");

	m_pTokenList->AddColumnHeader(0, "Token", "Token Name", 128, true, RESIZABLE, RESIZABLE);
	
	m_pLanguageEdit = new TextEntry(this, "LanguageEdit");
	m_pLanguageEdit->SetMultiline(true);
	m_pLanguageEdit->SetVerticalScrollbar(true);
	m_pLanguageEdit->SetCatchEnterKey(true);
	m_pEnglishEdit = new TextEntry(this, "EnglishEdit");
	m_pEnglishEdit->SetMultiline(true);
	m_pEnglishEdit->SetVerticalScrollbar(true);
	m_pEnglishEdit->SetVerticalScrollbar(true);

	m_pFileMenu = new Menu(this, "FileMenu");

	m_pFileMenu->AddMenuItem(" &Open File ", new KeyValues("FileOpen"), this);
	m_pFileMenu->AddMenuItem(" &Save File ", new KeyValues("FileSave"), this);
	m_pFileMenu->AddMenuItem(" E&xit Localizer ", new KeyValues("Close"), this);
	m_pFileMenuButton = new MenuButton(this, "FileMenuButton", "File");
	m_pFileMenuButton->SetMenu(m_pFileMenu);
	m_pApplyButton = new Button(this, "ApplyButton", "Apply");
	m_pApplyButton->SetCommand(new KeyValues("ApplyChanges"));
	m_pTestLabel = new Label(this, "TestLabel", "");

	LoadControlSettings("Resource/LocalizationDialog.res");

	strcpy(m_szFileName, fileName);

	char buf[512];
	sprintf(buf, "%s - Localization Editor", m_szFileName);
	SetTitle(buf, true);

	// load in the string table
	if (!localize()->AddFile(filesystem(), m_szFileName))
	{
		MessageBox *msg = new MessageBox("Fatal error", "couldn't load specified file");
		msg->SetCommand("Close");
		msg->AddActionSignalTarget(this);
		msg->DoModal();
		return;	
	}

	// populate the dialog with the strings
	StringIndex_t idx = localize()->GetFirstStringIndex();
	while (idx != VGUI_INVALID_STRING_INDEX)
	{
		// adds the strings into the table, along with the indexes
		m_pTokenList->AddItem(new KeyValues("LString", "Token", localize()->GetNameByIndex(idx)), idx);

		// move to the next string
		idx = localize()->GetNextStringIndex(idx);
	}

	// sort the table
	m_pTokenList->SetSortColumn(0);
	m_pTokenList->SortList();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CLocalizationDialog::~CLocalizationDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: Handles closing of the dialog - shuts down the whole app
//-----------------------------------------------------------------------------
void CLocalizationDialog::OnClose()
{
	BaseClass::OnClose();

	// Stop vgui running
	vgui::ivgui()->Stop();
}

//-----------------------------------------------------------------------------
// Purpose: lays out the dialog
//-----------------------------------------------------------------------------
void CLocalizationDialog::PerformLayout()
{
	OnTextChanged();

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the currently selected token
//-----------------------------------------------------------------------------
void CLocalizationDialog::OnTokenSelected()
{
	if (m_pTokenList->GetNumSelectedRows() != 1)
	{
		// clear the list
		m_pLanguageEdit->SetText("");
		m_pEnglishEdit->SetText("");
		
		//!! unicode test label
		m_pTestLabel->SetText("");

		m_iCurrentToken = -1;
	}
	else
	{
		// get the data
		ListPanel::DATAITEM *data = m_pTokenList->GetDataItem(m_pTokenList->GetSelectedRow(0));
		m_iCurrentToken = data->userData;
		wchar_t *unicodeString = localize()->GetValueByIndex(m_iCurrentToken);

		char value[2048];
		localize()->ConvertUnicodeToANSI(unicodeString, value, sizeof(value));

		//!! unicode test label
		m_pTestLabel->SetText(unicodeString);

		// set the text
		m_pLanguageEdit->SetText(value);
		m_pEnglishEdit->SetText(value);
	}

	m_pApplyButton->SetEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if any text has changed
//-----------------------------------------------------------------------------
void CLocalizationDialog::OnTextChanged()
{
	static char buf1[1024], buf2[1024];

	m_pLanguageEdit->GetText(0, buf1, sizeof(buf1));
	m_pEnglishEdit->GetText(0, buf2, sizeof(buf2));

	if (!strcmp(buf1, buf2))
	{
		m_pApplyButton->SetEnabled(false);
	}
	else
	{
		m_pApplyButton->SetEnabled(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copies any changes made into the main database
//-----------------------------------------------------------------------------
void CLocalizationDialog::OnApplyChanges()
{
	if (m_iCurrentToken < 0)
		return;

	static char buf1[1024];
	static wchar_t unicodeString[1024];
	m_pLanguageEdit->GetText(0, buf1, sizeof(buf1));
	localize()->ConvertANSIToUnicode(buf1, unicodeString, sizeof(unicodeString) / sizeof(wchar_t));

	//!! unicode test label
	m_pTestLabel->SetText(unicodeString);

	// apply the text change to the database
	localize()->SetValueByIndex(m_iCurrentToken, unicodeString);

	// disable the apply button
	m_pApplyButton->SetEnabled(false);

	// reselect the token
	OnTokenSelected();
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for saving current file
//-----------------------------------------------------------------------------
void CLocalizationDialog::OnFileSave()
{
	if (localize()->SaveToFile(filesystem(), m_szFileName))
	{
		// success
		MessageBox *box = new MessageBox("Save Successful - VLocalize", "File was successfully saved.", false);
		box->DoModal();
	}
	else
	{
		// failure
		MessageBox *box = new MessageBox("Error during save - VLocalize", "Error - File was not successfully saved.", false);
		box->DoModal();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for loading a file
//-----------------------------------------------------------------------------
void CLocalizationDialog::OnFileOpen()
{
	FileOpenDialog *box = new FileOpenDialog(this, "Open");

	box->SetStartDirectory("u:\\");
	box->AddFilter("*.*", "All Files (*.*)");
	box->DoModal(false);
}

//-----------------------------------------------------------------------------
// Purpose: Handles a token created message
// Input  : *tokenName - the name of the newly created token
//-----------------------------------------------------------------------------
void CLocalizationDialog::OnTokenCreated(const char *tokenName)
{
	// add the new string table token to the token list
	int idx = localize()->FindIndex(tokenName);
	int newRow = m_pTokenList->AddItem(new KeyValues("LString", "Token", localize()->GetNameByIndex(idx)), idx);

	// make that currently selected
	m_pTokenList->SetSelectedRows(newRow, newRow);
	OnTokenSelected();
}

//-----------------------------------------------------------------------------
// Purpose: Creates a new token
//-----------------------------------------------------------------------------
void CLocalizationDialog::OnCreateToken()
{
	CCreateTokenDialog *dlg = new CCreateTokenDialog();
	dlg->AddActionSignalTarget(this);
	dlg->CreateSingleToken();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CLocalizationDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "CreateToken"))
	{
		OnCreateToken();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}


//-----------------------------------------------------------------------------
// Purpose: empty message map
//-----------------------------------------------------------------------------
MessageMapItem_t CLocalizationDialog::m_MessageMap[] =
{
	MAP_MESSAGE( CLocalizationDialog, "RowSelected", OnTokenSelected ),	// message from the m_pTokenList
	MAP_MESSAGE( CLocalizationDialog, "TextChanged", OnTextChanged ),	// message from the text entry
	MAP_MESSAGE( CLocalizationDialog, "ApplyChanges", OnApplyChanges ),	// message from the text entry
	MAP_MESSAGE( CLocalizationDialog, "FileSave", OnFileSave ),
	MAP_MESSAGE( CLocalizationDialog, "FileOpen", OnFileOpen ),
	MAP_MESSAGE_CONSTCHARPTR( CLocalizationDialog, "TokenCreated", OnTokenCreated, "name" ),
};
IMPLEMENT_PANELMAP(CLocalizationDialog, BaseClass);
