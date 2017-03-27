//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <ctype.h>
#include <stdio.h>
#include <UtlVector.h>

#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>

#include <vgui_controls/BuildModeDialog.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/FocusNavGroup.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/BuildGroup.h>
#include <vgui_controls/MessageBox.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/PanelListPanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

struct PanelItem_t
{
	PanelItem_t() : m_EditLabel(NULL) {}

	Panel *m_EditLabel;
	TextEntry *m_EditPanel;
	ComboBox *m_pCombo;
	Button *m_EditButton;
	char m_szName[64];
	int m_iType;
};

//-----------------------------------------------------------------------------
// Purpose: Holds a list of all the edit fields for the currently selected panel
//-----------------------------------------------------------------------------
class BuildModeDialog::PanelList
{
public:

	CUtlVector<PanelItem_t> m_PanelList;

	void AddItem( Panel *label, TextEntry *edit, ComboBox *combo, Button *button, const char *name, int type )
	{
		PanelItem_t item;
		item.m_EditLabel = label;
		item.m_EditPanel = edit;
		strncpy(item.m_szName, name, sizeof(item.m_szName) - 1);
		item.m_szName[sizeof(item.m_szName) - 1] = 0;
		item.m_iType = type;
		item.m_pCombo = combo;
		item.m_EditButton = button;

		m_PanelList.AddToTail( item );
	}

	void RemoveAll( void )
	{
		for ( int i = 0; i < m_PanelList.Size(); i++ )
		{
			PanelItem_t *item = &m_PanelList[i];
			delete item->m_EditLabel;
			delete item->m_EditPanel;
			delete item->m_EditButton;
		}

		m_PanelList.RemoveAll();
		m_pControls->RemoveAll();
	}

	KeyValues *m_pResourceData;
	PanelListPanel *m_pControls;
};

//-----------------------------------------------------------------------------
// Purpose: Dialog for adding localized strings
//-----------------------------------------------------------------------------
class BuildModeLocalizedStringEditDialog : public Frame
{
public:
	BuildModeLocalizedStringEditDialog() : Frame(this, NULL)
	{
		m_pTokenEntry = new TextEntry(this, NULL);
		m_pValueEntry = new TextEntry(this, NULL);
		m_pFileCombo = new ComboBox(this, NULL, 12, false);
		m_pOKButton = new Button(this, NULL, "OK");
		m_pCancelButton = new Button(this, NULL, "Cancel");

		m_pCancelButton->SetCommand("Close");
		m_pOKButton->SetCommand("OK");

		// add the files to the combo
		for (int i = 0; i < localize()->GetLocalizationFileCount(); i++)
		{
			m_pFileCombo->AddItem(localize()->GetLocalizationFileName(i), NULL);
		}
	}

	virtual void DoModal(const char *token)
	{
		input()->SetAppModalSurface(GetVPanel());

		// setup data
		m_pTokenEntry->SetText(token);

		// lookup the value
		StringIndex_t val = localize()->FindIndex(token);
		if (val != INVALID_STRING_INDEX)
		{
			m_pValueEntry->SetText(localize()->GetValueByIndex(val));

			// set the place in the file combo
			m_pFileCombo->SetText(localize()->GetFileNameByIndex(val));
		}
		else
		{
			m_pValueEntry->SetText("");
		}
	}

private:
	virtual void PerformLayout()
	{
	}

	virtual void OnClose()
	{
		input()->SetAppModalSurface(NULL);
		BaseClass::OnClose();
		//PostActionSignal(new KeyValues("Command"
	}

	virtual void OnCommand(const char *command)
	{
		if (!stricmp(command, "OK"))
		{
			//!! apply changes
		}
		else
		{
			BaseClass::OnCommand(command);
		}
	}

	vgui::TextEntry *m_pTokenEntry;
	vgui::TextEntry *m_pValueEntry;
	vgui::ComboBox *m_pFileCombo;
	vgui::Button *m_pOKButton;
	vgui::Button *m_pCancelButton;

	typedef vgui::Frame BaseClass;
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
BuildModeDialog::BuildModeDialog(BuildGroup *buildGroup) : Frame(NULL, NULL)
{
	SetMinimumSize(300, 256);
	SetSize(300, 400);
	m_pCurrentPanel = NULL;
	m_pBuildGroup = buildGroup;
	_undoSettings = NULL;
	_copySettings = NULL;
	_autoUpdate = false;

	CreateControls();
	LoadUserConfig("BuildModeDialog");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
BuildModeDialog::~BuildModeDialog()
{
	m_pPanelList->m_pResourceData->deleteThis();
	m_pPanelList->m_pControls->DeleteAllItems();
	if (_undoSettings)
		_undoSettings->deleteThis();
	if (_copySettings)
		_copySettings->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BuildModeDialog::OnClose()
{
	BaseClass::OnClose();
}


//-----------------------------------------------------------------------------
// Purpose: Creates the build mode editing controls
//-----------------------------------------------------------------------------
void BuildModeDialog::CreateControls()
{
	m_pPanelList = new PanelList;
	m_pPanelList->m_pResourceData = new KeyValues( "BuildDialog" );
	m_pPanelList->m_pControls = new PanelListPanel(this, "BuildModeControls");

	// status info at top of dialog
	m_pStatusLabel = new Label(this, "StatusLabel", "[nothing currently selected]");
	m_pStatusLabel->SetTextColorState(Label::CS_DULL);
	m_pDivider = new Divider(this, "Divider");
	// drop-down combo box for adding new controls
	m_pAddNewControlCombo = new ComboBox(this, NULL, 15, false);
	m_pAddNewControlCombo->SetSize(116, 24);

	// controls that can be added
	// this list comes from controls EditablePanel can create by name.
	int defaultItem = m_pAddNewControlCombo->AddItem("None", NULL);
	m_pAddNewControlCombo->AddItem("Button", NULL);
	m_pAddNewControlCombo->AddItem("CheckButton", NULL);
	m_pAddNewControlCombo->AddItem("ComboBox", NULL);
	m_pAddNewControlCombo->AddItem("Divider", NULL);
	m_pAddNewControlCombo->AddItem("ImagePanel", NULL);
	m_pAddNewControlCombo->AddItem("Label", NULL);
	m_pAddNewControlCombo->AddItem("ProgressBar", NULL);
	m_pAddNewControlCombo->AddItem("RadioButton", NULL);
	m_pAddNewControlCombo->AddItem("TextEntry", NULL);
	m_pAddNewControlCombo->AddItem("ToggleButton", NULL);
	m_pAddNewControlCombo->ActivateItem(defaultItem);

	m_pExitButton = new Button(this, "ExitButton", "&Exit");
	m_pExitButton->SetSize(64, 24);

	m_pSaveButton = new Button(this, "SaveButton", "&Save");
	m_pSaveButton->SetSize(64, 24);
	
	m_pApplyButton = new Button(this, "ApplyButton", "&Apply");
	m_pApplyButton->SetSize(64, 24);

	m_pExitButton->SetCommand("Exit");
	m_pSaveButton->SetCommand("Save");
	m_pApplyButton->SetCommand("Apply");

	m_pDeleteButton = new Button(this, "DeletePanelButton", "Delete");
	m_pDeleteButton->SetSize(64, 24);
	m_pDeleteButton->SetCommand("DeletePanel");

	m_pOptionsButton = new Button(this, "OptionsButton", "Options");
	m_pOptionsButton->SetEnabled(false);
	m_pOptionsButton->SetSize(72, 24);

	m_pApplyButton->SetTabPosition(1);
	m_pPanelList->m_pControls->SetTabPosition(2);
	m_pOptionsButton->SetTabPosition(3);
	m_pDeleteButton->SetTabPosition(4);
	m_pAddNewControlCombo->SetTabPosition(5);
	m_pSaveButton->SetTabPosition(6);
	m_pExitButton->SetTabPosition(7);
}

//-----------------------------------------------------------------------------
// Purpose: lays out controls
//-----------------------------------------------------------------------------
void BuildModeDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	// layout parameters
	const int BORDER_GAP = 16, YGAP_SMALL = 4, YGAP_LARGE = 8, TITLE_HEIGHT = 24, BOTTOM_CONTROLS_HEIGHT = 96, XGAP = 6;

	int wide, tall;
	GetSize(wide, tall);
	
	int xpos = BORDER_GAP;
	int ypos = BORDER_GAP + TITLE_HEIGHT;

	// controls from top down
	m_pStatusLabel->SetBounds(xpos, ypos, wide - (BORDER_GAP * 2), m_pStatusLabel->GetTall());
	ypos += (m_pStatusLabel->GetTall() + YGAP_SMALL);

	// center control
	m_pPanelList->m_pControls->SetPos(xpos, ypos);
	m_pPanelList->m_pControls->SetSize(wide - (BORDER_GAP * 2), tall - (ypos + BOTTOM_CONTROLS_HEIGHT));

	// controls from bottom-right
	ypos = tall - BORDER_GAP;
	xpos = BORDER_GAP + m_pOptionsButton->GetWide() + m_pDeleteButton->GetWide() + m_pAddNewControlCombo->GetWide() + (XGAP * 2);

	// bottom row of buttons
	ypos -= m_pApplyButton->GetTall();
	xpos -= m_pApplyButton->GetWide();
	m_pApplyButton->SetPos(xpos, ypos);

	xpos -= m_pExitButton->GetWide();
	xpos -= XGAP;
	m_pExitButton->SetPos(xpos, ypos);

	xpos -= m_pSaveButton->GetWide();
	xpos -= XGAP;
	m_pSaveButton->SetPos(xpos, ypos);

	// divider
	xpos = BORDER_GAP;
	ypos -= (YGAP_LARGE + m_pDivider->GetTall());
	m_pDivider->SetBounds(xpos, ypos, wide - (xpos + BORDER_GAP), 2);

	ypos -= (YGAP_LARGE  + m_pOptionsButton->GetTall());

	// edit buttons
	m_pOptionsButton->SetPos(xpos, ypos);
	xpos += (XGAP + m_pOptionsButton->GetWide());
	m_pDeleteButton->SetPos(xpos, ypos);
	xpos += (XGAP + m_pDeleteButton->GetWide());
	m_pAddNewControlCombo->SetPos(xpos, ypos);
}

//-----------------------------------------------------------------------------
// Purpose: Deletes all the controls from the panel
//-----------------------------------------------------------------------------
void BuildModeDialog::RemoveAllControls( void )
{
	// free the array
	m_pPanelList->RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: simple helper function to get a token from a string
// Input  : char **string - pointer to the string pointer, which will be incremented
// Output : const char * - pointer to the token
//-----------------------------------------------------------------------------
const char *ParseTokenFromString( const char **string )
{
	static char buf[128];
	buf[0] = 0;

	// find the first alnum character
	const char *tok = *string;
	while ( !isalnum(*tok) && *tok != 0 )
	{
		tok++;
	}

	// read in all the alnum characters
	int pos = 0;
	while ( isalnum(tok[pos]) )
	{
		buf[pos] = tok[pos];
		pos++;
	}

	// null terminate the token
	buf[pos] = 0;

	// update the main string pointer
	*string = &(tok[pos]);

	// return a pointer to the static buffer
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: sets up the current control to edit
//-----------------------------------------------------------------------------
void BuildModeDialog::SetActiveControl(Panel *controlToEdit)
{	
	if (m_pCurrentPanel == controlToEdit)
	{
		// it's already set, so just update the property data and quit
		if (m_pCurrentPanel)
		{
			UpdateControlData(m_pCurrentPanel);
		}
		return;
	}

	if (!controlToEdit)
	{
		m_pStatusLabel->SetText("[nothing currently selected]");
		m_pStatusLabel->SetTextColorState(Label::CS_DULL);
		return;
	}

	// delete the old settings if they exist
	if (m_pCurrentPanel)
	{
		RemoveAllControls();
	}

	m_pPanelList->m_pControls->MoveScrollBarToTop();

	// create the new controls
	m_pCurrentPanel = controlToEdit;

	// get the control description string
	const char *controlDesc = m_pCurrentPanel->GetDescription();

	// parse out the control description
	int tabPosition = 1;
	while (1)
	{
		const char *dataType = ParseTokenFromString(&controlDesc);

		// finish when we have no more tokens
		if (*dataType == 0)
			break;

		// default the data type to a string
		int datat = TYPE_STRING;

		if (!stricmp(dataType, "int"))
		{
			datat = TYPE_STRING; //!! just for now
		}
		else if (!stricmp(dataType, "alignment"))
		{
			datat = TYPE_ALIGNMENT;
		}
		else if (!stricmp(dataType, "autoresize"))
		{
			datat = TYPE_AUTORESIZE;
		}
		else if (!stricmp(dataType, "corner"))
		{
			datat = TYPE_CORNER;
		}
		else if (!stricmp(dataType, "localize"))
		{
			datat = TYPE_LOCALIZEDSTRING;
		}

		// get the field name
		const char *fieldName = ParseTokenFromString(&controlDesc);

		// build a control & label
		Label *label = new Label(this, NULL, fieldName);
		label->SetSize(96, 24);
		label->SetContentAlignment(Label::a_east);

		TextEntry *edit = NULL;
		ComboBox *editCombo = NULL;
		Button *editButton = NULL;
		if (datat == TYPE_ALIGNMENT)
		{
			// drop-down combo box
			editCombo = new ComboBox(this, NULL, 9, false);
			editCombo->AddItem("north-west", NULL);
			editCombo->AddItem("north", NULL);
			editCombo->AddItem("north-east", NULL);
			editCombo->AddItem("west", NULL);
			editCombo->AddItem("center", NULL);
			editCombo->AddItem("east", NULL);
			editCombo->AddItem("south-west", NULL);
			editCombo->AddItem("south", NULL);
			editCombo->AddItem("south-east", NULL);
		
			edit = editCombo;
		}
		else if (datat == TYPE_AUTORESIZE)
		{
			// drop-down combo box
			editCombo = new ComboBox(this, NULL, 4, false);
			editCombo->AddItem( "0 - no auto-resize", NULL);
			editCombo->AddItem( "1 - resize right", NULL);
			editCombo->AddItem( "2 - resize down", NULL);
			editCombo->AddItem( "3 - down & right", NULL);
		
			edit = editCombo;
		}
		else if (datat == TYPE_CORNER)
		{
			// drop-down combo box
			editCombo = new ComboBox(this, NULL, 4, false);
			editCombo->AddItem("0 - top-left", NULL);
			editCombo->AddItem("1 - top-right", NULL);
			editCombo->AddItem("2 - bottom-left", NULL);
			editCombo->AddItem("3 - bottom-right", NULL);
		
			edit = editCombo;
		}
		else if (datat == TYPE_LOCALIZEDSTRING)
		{
			editButton = new Button(this, NULL, "...");
			editButton->SetParent(this);
			editButton->AddActionSignalTarget(this);
			editButton->SetTabPosition(tabPosition++);
			label->SetAssociatedControl(editButton);
		}
		else
		{
			// normal string edit
			edit = new TextEntry(this, NULL);
		}

		if (edit)
		{
			edit->SetParent(this);
			edit->AddActionSignalTarget(this);
			edit->SetTabPosition(tabPosition++);
			label->SetAssociatedControl(edit);
		}

		// add to our control list
		m_pPanelList->AddItem(label, edit, editCombo, editButton, fieldName, datat);

		if ( edit )
		{
			m_pPanelList->m_pControls->AddItem(label, edit);
		}
		else
		{
			m_pPanelList->m_pControls->AddItem(label, editButton);
		}
	}

	// check and see if the current panel is a Label
	// iterate through the class hierarchy 
	if ( controlToEdit->IsBuildModeDeletable() )
	{
		m_pDeleteButton->SetEnabled(true);
	}
	else
	{
		m_pDeleteButton->SetEnabled(false);	
	}

	// update the property data in the dialog
	UpdateControlData(m_pCurrentPanel);
	
	// set our title
	if ( m_pBuildGroup->GetResourceName() )
	{
		SetTitle(m_pBuildGroup->GetResourceName(), true);
	}
	else
	{
		SetTitle("Unnamed dialog", true);
	}

	m_pApplyButton->SetEnabled(false);
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Updates the edit fields with information about the control
//-----------------------------------------------------------------------------
void BuildModeDialog::UpdateControlData(Panel *control)
{
	KeyValues *dat = m_pPanelList->m_pResourceData->FindKey( control->GetName(), true );
	control->GetSettings( dat );

	// apply the settings to the edit panels
	for ( int i = 0; i < m_pPanelList->m_PanelList.Size(); i++ )
	{
		const char *name = m_pPanelList->m_PanelList[i].m_szName;
		const char *datstring = dat->GetString( name, "" );

		UpdateEditControl(m_pPanelList->m_PanelList[i], datstring);
	}

	char statusText[512];
	_snprintf(statusText, sizeof(statusText) - 1, "%s: \'%s\'", control->GetClassName(), control->GetName());
	m_pStatusLabel->SetText(statusText);
	m_pStatusLabel->SetTextColorState(Label::CS_NORMAL);
}

//-----------------------------------------------------------------------------
// Purpose: Updates the data in a single edit control
//-----------------------------------------------------------------------------
void BuildModeDialog::UpdateEditControl(PanelItem_t &panelItem, const char *datstring)
{
	switch (panelItem.m_iType)
	{
		case TYPE_AUTORESIZE:
		case TYPE_CORNER:
			{
				int dat = atoi(datstring);
				panelItem.m_pCombo->ActivateItemByRow(dat);
			}
			break;

		case TYPE_LOCALIZEDSTRING:
			{
				panelItem.m_EditButton->SetText(datstring);
			}
			break;

		default:
			{
				wchar_t unicode[512];
				localize()->ConvertANSIToUnicode(datstring, unicode, sizeof(unicode));
				panelItem.m_EditPanel->SetText(datstring);
			}
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when one of the buttons is pressed
//-----------------------------------------------------------------------------
void BuildModeDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "Save"))
	{
		// apply the current data and save it to disk
		if (ApplyDataToControls()) // dont save if we didnt apply successfully	
		{
			if (m_pBuildGroup->SaveControlSettings())
			{
				// disable save button until another change has been made
				m_pSaveButton->SetEnabled(false);
			}
		}
	}
	else if (!stricmp(command, "Exit"))
	{
		// exit build mode
		ExitBuildMode();
	}
	else if (!stricmp(command, "Apply"))
	{
		// apply data to controls
		ApplyDataToControls();
	}
	else if (!stricmp(command, "DeletePanel"))
	{
		DeletePanel();
	}
	else if (!stricmp(command, "RevertToSaved"))
	{
		RevertToSaved();
	}
	else if (!stricmp(command, "ShowHelp"))
	{
		ShowHelp();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Deletes a panel from the buildgroup
//-----------------------------------------------------------------------------
void BuildModeDialog::DeletePanel()
{
	if ( !m_pCurrentPanel->IsBuildModeEditable())
	{
		return; 
	}

	m_pBuildGroup->RemoveSettings();
	SetActiveControl(m_pBuildGroup->GetCurrentPanel());

	_undoSettings->deleteThis();
	_undoSettings = NULL;
	m_pSaveButton->SetEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: Applies the current settings to the build controls
//-----------------------------------------------------------------------------
bool BuildModeDialog::ApplyDataToControls()
{
	// don't apply if the planel is not editable
	if ( !m_pCurrentPanel->IsBuildModeEditable())
	{
		UpdateControlData( m_pCurrentPanel );
		return true; // return success, since we are behaving as expected.
	}

	char fieldName[512];
	if (m_pPanelList->m_PanelList[0].m_EditPanel)
	{
		m_pPanelList->m_PanelList[0].m_EditPanel->GetText(fieldName, sizeof(fieldName));
	}
	else
	{
		m_pPanelList->m_PanelList[0].m_EditButton->GetText(fieldName, sizeof(fieldName));
	}

	// check to see if any buildgroup panels have this name
	Panel *panel = m_pBuildGroup->FieldNameTaken(fieldName);
	if (panel)
	{
		if (panel != m_pCurrentPanel)// make sure name is taken by some other panel not this one
		{
			char messageString[255];
			sprintf(messageString, "Fieldname is not unique: %s\nRename it and try again.", fieldName);
			MessageBox *errorBox = new MessageBox("Cannot Apply", messageString , false);
			errorBox->DoModal();
			UpdateControlData(m_pCurrentPanel);
			m_pApplyButton->SetEnabled(false);
			return false;
		}
	}

	// create a section to store settings
	// m_pPanelList->m_pResourceData->getSection( m_pCurrentPanel->GetName(), true );
	KeyValues *dat = new KeyValues( m_pCurrentPanel->GetName() );

	// loop through the textedit filling in settings
	for ( int i = 0; i < m_pPanelList->m_PanelList.Size(); i++ )
	{
		const char *name = m_pPanelList->m_PanelList[i].m_szName;
		char buf[512];
		if (m_pPanelList->m_PanelList[i].m_EditPanel)
		{
			m_pPanelList->m_PanelList[i].m_EditPanel->GetText(buf, sizeof(buf));
		}
		else
		{
			m_pPanelList->m_PanelList[i].m_EditButton->GetText(buf, sizeof(buf));
		}

		switch (m_pPanelList->m_PanelList[i].m_iType)
		{
		case TYPE_CORNER:
		case TYPE_AUTORESIZE:
			// the integer value is assumed to be the first part of the string for these items
			dat->SetInt(name, atoi(buf));
			break;

		default:
			dat->SetString(name, buf);
			break;
		}
	}

	// dat is built, hand it back to the control
	m_pCurrentPanel->ApplySettings( dat );

	m_pApplyButton->SetEnabled(false);
	m_pSaveButton->SetEnabled(true);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Store the settings of the current panel in a KeyValues
//-----------------------------------------------------------------------------
void BuildModeDialog::StoreUndoSettings()
{
	// don't save if the planel is not editable
	if ( !m_pCurrentPanel->IsBuildModeEditable())
	{
		if (_undoSettings)
			_undoSettings->deleteThis();
		_undoSettings = NULL;
		return; 
	}

	if (_undoSettings)
	{
		_undoSettings->deleteThis();
		_undoSettings = NULL;
	}

	_undoSettings = StoreSettings();
}


//-----------------------------------------------------------------------------
// Purpose: Revert to the stored the settings of the current panel in a keyValues
//-----------------------------------------------------------------------------
void BuildModeDialog::DoUndo()
{
	if ( _undoSettings )
	{		
		m_pCurrentPanel->ApplySettings( _undoSettings );
		UpdateControlData(m_pCurrentPanel);
		_undoSettings->deleteThis();
		_undoSettings = NULL;
	}

	m_pSaveButton->SetEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: Copy the settings of the current panel into a keyValues
//-----------------------------------------------------------------------------
void BuildModeDialog::DoCopy()
{
	if (_copySettings)
	{
		_copySettings->deleteThis();
		_copySettings = NULL;
	}

	_copySettings = StoreSettings();
	strcpy (_copyClassName, m_pCurrentPanel->GetClassName());
}

//-----------------------------------------------------------------------------
// Purpose: Create a new Panel with the _copySettings applied
//-----------------------------------------------------------------------------
void BuildModeDialog::DoPaste()
{
	//Make a new control located where you had the mouse
	int x, y;
	input()->GetCursorPos(x, y);
	m_pBuildGroup->GetContextPanel()->ScreenToLocal(x,y);

	Panel *newPanel = OnNewControl(_copyClassName, x, y);
	if (newPanel)
	{
		newPanel->ApplySettings(_copySettings);
		newPanel->SetPos(x, y);
		char name[255];
		m_pBuildGroup->GetNewFieldName(name, newPanel);
		newPanel->SetName(name);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Store the settings of the current panel in a keyValues
//-----------------------------------------------------------------------------
KeyValues *BuildModeDialog::StoreSettings()
{
	KeyValues *storedSettings;
	storedSettings = new KeyValues( m_pCurrentPanel->GetName() );

	// loop through the textedit filling in settings
	for ( int i = 0; i < m_pPanelList->m_PanelList.Size(); i++ )
	{
		const char *name = m_pPanelList->m_PanelList[i].m_szName;
		char buf[512];
		if (m_pPanelList->m_PanelList[i].m_EditPanel)
		{
			m_pPanelList->m_PanelList[i].m_EditPanel->GetText(buf, sizeof(buf));
		}
		else
		{
			m_pPanelList->m_PanelList[i].m_EditButton->GetText(buf, sizeof(buf));
		}

		switch (m_pPanelList->m_PanelList[i].m_iType)
		{
		case TYPE_CORNER:
		case TYPE_AUTORESIZE:
			// the integer value is assumed to be the first part of the string for these items
			storedSettings->SetInt(name, atoi(buf));
			break;

		default:
			storedSettings->SetString(name, buf);
			break;
		}
	}

	return storedSettings;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void BuildModeDialog::OnKeyCodeTyped(KeyCode code)
{
	if (code == KEY_ENTER) // if someone hits return apply the changes
	{
		ApplyDataToControls();
	}
	else
	{
		Frame::OnKeyCodeTyped(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if any text has changed
//-----------------------------------------------------------------------------
void BuildModeDialog::OnTextChanged()
{
	char buf[40];
	m_pAddNewControlCombo->GetText(buf, 40);
	if (strcmp(buf, "None") != 0)
	{	
		OnNewControl(buf);
		// reset box back to None
		m_pAddNewControlCombo->ActivateItemByRow( 0 );
	}

	if (m_pCurrentPanel->IsBuildModeEditable())
		m_pApplyButton->SetEnabled(true);
	

	if (_autoUpdate) 
		ApplyDataToControls();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BuildModeDialog::ExitBuildMode( void )
{
	// make sure rulers are off
	if (m_pBuildGroup->HasRulersOn())
	{
		m_pBuildGroup->ToggleRulerDisplay();
	}
	m_pBuildGroup->SetEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Create a new control in the context panel
//-----------------------------------------------------------------------------
Panel *BuildModeDialog::OnNewControl( const char *name , int x, int y)
{
	// returns NULL on failure
	Panel *newPanel = m_pBuildGroup->NewControl(name, x, y);
	if (newPanel)
	{
	   // call mouse commands to simulate selecting the new
		// panel. This will set everything up correctly in the buildGroup.
		m_pBuildGroup->MousePressed(MOUSE_LEFT, newPanel);
		m_pBuildGroup->MouseReleased(MOUSE_LEFT, newPanel);
	}

	m_pSaveButton->SetEnabled(true);

	return newPanel;
}

//-----------------------------------------------------------------------------
// Purpose: enable the save button, useful when buildgroup needs to Activate it.
//-----------------------------------------------------------------------------
void BuildModeDialog::EnableSaveButton()
{
	m_pSaveButton->SetEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: Revert to the saved settings in the .res file
//-----------------------------------------------------------------------------
void BuildModeDialog::RevertToSaved()
{
	// hide the dialog as reloading will destroy it
	surface()->SetPanelVisible(this->GetVPanel(), false);
	m_pBuildGroup->ReloadControlSettings();
}

//-----------------------------------------------------------------------------
// Purpose: Display some information about the editor
//-----------------------------------------------------------------------------
void BuildModeDialog::ShowHelp()
{
	char helpText[]= "In the Build Mode Dialog Window:\n" 
		"Delete button - deletes the currently selected panel if it is deletable.\n"
		"Apply button - applies changes to the Context Panel.\n"
		"Save button - saves all settings to file. \n"
		"Revert to saved- reloads the last saved file.\n"
		"Auto Update - any changes apply instantly.\n"
		"Typing Enter in any text field applies changes.\n"
		"New Control menu - creates a new panel in the upper left corner.\n\n" 
		"In the Context Panel:\n"
		"After selecting and moving a panel Ctrl-z will undo the move.\n"
		"Shift clicking panels allows multiple panels to be selected into a group.\n"
		"Ctrl-c copies the settings of the last selected panel.\n"
		"Ctrl-v creates a new panel with the copied settings at the location of the mouse pointer.\n"
		"Arrow keys slowly move panels, holding shift + arrow will slowly resize it.\n"
		"Holding right mouse button down opens a dropdown panel creation menu.\n"
		"  Panel will be created where the menu was opened.\n"
		"Delete key deletes the currently selected panel if it is deletable.\n"
		"  Does nothing to multiple selections.";
		
	MessageBox *helpDlg = new MessageBox ("Build Mode Help", helpText, this);
	helpDlg->AddActionSignalTarget(this);
	helpDlg->DoModal();
}

	
void BuildModeDialog::ShutdownBuildMode()
{
	m_pBuildGroup->SetEnabled(false);
}

void BuildModeDialog::OnPanelMoved()
{
	m_pApplyButton->SetEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t BuildModeDialog::m_MessageMap[] =
{
	MAP_MESSAGE_PTR( BuildModeDialog, "SetActiveControl", SetActiveControl, "PanelPtr" ),		// custom message
	MAP_MESSAGE( BuildModeDialog, "ApplyDataToControls", ApplyDataToControls),		// custom message
	MAP_MESSAGE_PTR( BuildModeDialog, "UpdateControlData", UpdateControlData, "panel"),		// custom message
	MAP_MESSAGE( BuildModeDialog, "TextChanged", OnTextChanged ),	// message from the text entry
	MAP_MESSAGE( BuildModeDialog, "DeletePanel", DeletePanel ),
	MAP_MESSAGE_CONSTCHARPTR( BuildModeDialog, "NewControl", OnNewControl, "ControlName" ),		// custom message
	MAP_MESSAGE( BuildModeDialog, "StoreUndo", StoreUndoSettings),		// custom message
	MAP_MESSAGE( BuildModeDialog, "Undo", DoUndo),		// custom message
	MAP_MESSAGE( BuildModeDialog, "Copy", DoCopy),		// custom message
	MAP_MESSAGE( BuildModeDialog, "Paste", DoPaste),		// custom message
	MAP_MESSAGE( BuildModeDialog, "EnableSaveButton", EnableSaveButton),		// custom message
	MAP_MESSAGE( BuildModeDialog, "Close", ShutdownBuildMode ),
	MAP_MESSAGE( BuildModeDialog, "PanelMoved", OnPanelMoved ),
};

IMPLEMENT_PANELMAP(BuildModeDialog, BaseClass);
