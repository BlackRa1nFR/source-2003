//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <time.h>
#include "EngineInterface.h"

#include "MultiplayerAdvancedDialog.h"
#include <vgui/ISurface.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/TextEntry.h>
#include "PanelListPanel.h"

#include <vgui/IScheme.h>
#include "FileSystem.h"
#include <tier0/vcrmode.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

void UTIL_StripInvalidCharacters( char *pszInput );

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CMultiplayerAdvancedPage::CMultiplayerAdvancedPage(vgui::Panel *parent) 
: vgui::PropertyPage(parent, "MultiplayerAdvancedDialog")
{
	m_pListPanel = new CPanelListPanel( this, "PanelListPanel" );

	m_pList = NULL;

	m_pDescription = new CInfoDescription( m_pListPanel );
	m_pDescription->InitFromFile( "user.scr" );
	m_pDescription->TransferCurrentValues( NULL );

	CreateControls();
	LoadControlSettings("Resource\\MultiplayerAdvancedPage.res");

}

CMultiplayerAdvancedDialog::CMultiplayerAdvancedDialog(vgui::Panel *parent) 
: CTaskFrame(parent, "MultiplayerAdvancedDialog")
{
	SetBounds(0, 0, 372, 160);
	SetSizeable( false );

	MakePopup();
	g_pTaskbar->AddTask(GetVPanel());

	SetTitle("#GameUI_MultiplayerAdvanced", true);

	Button *cancel = new Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	Button *ok = new Button( this, "OK", "#GameUI_OK" );
	ok->SetCommand( "Ok" );

	m_pListPanel = new CPanelListPanel( this, "PanelListPanel" );

	m_pList = NULL;

	m_pDescription = new CInfoDescription( m_pListPanel );
	m_pDescription->InitFromFile( "user.scr" );
	m_pDescription->TransferCurrentValues( NULL );

	CreateControls();
	LoadControlSettings("Resource\\MultiplayerAdvancedDialog.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMultiplayerAdvancedDialog::~CMultiplayerAdvancedDialog()
{
	//DestroyControls();
}

CMultiplayerAdvancedPage::~CMultiplayerAdvancedPage()
{
	//DestroyControls();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiplayerAdvancedDialog::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CMultiplayerAdvancedDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "Ok" ) )
	{
		// OnApplyChanges();
		SaveValues();
		OnClose();
		return;
	}

	BaseClass::OnCommand( command );
}


void CMultiplayerAdvancedPage::OnResetData()
{
	GatherCurrentValues();
}
void CMultiplayerAdvancedPage::OnApplyChanges()
{
	SaveValues();
}


void CMultiplayerAdvanced::GatherCurrentValues()
{
	if ( !m_pDescription )
		return;

	// OK
	CheckButton *pBox;
	TextEntry *pEdit;
	ComboBox *pCombo;

	mpcontrol_t *pList;

	CScriptObject *pObj;
	CScriptListItem *pItem;

	char szValue[256];
	char strValue[ 256 ];

	pList = m_pList;
	while ( pList )
	{
		pObj = pList->pScrObj;

		if ( !pList->pControl )
		{
			pObj->SetCurValue( pObj->defValue );
			pList = pList->next;
			continue;
		}

		switch ( pObj->type )
		{
		case O_BOOL:
			pBox = (CheckButton *)pList->pControl;
			sprintf( szValue, "%s", pBox->IsSelected() ? "1" : "0" );
			break;
		case O_NUMBER:
			pEdit = ( TextEntry * )pList->pControl;
			pEdit->GetText( strValue, sizeof( strValue ) );
			sprintf( szValue, "%s", strValue );
			break;
		case O_STRING:
			pEdit = ( TextEntry * )pList->pControl;
			pEdit->GetText( strValue, sizeof( strValue ) );
			sprintf( szValue, "%s", strValue );
			break;
		case O_LIST:
			pCombo = (ComboBox *)pList->pControl;
			// pCombo->GetText( strValue, sizeof( strValue ) );
			int activeItem = pCombo->GetActiveItem();
			
			pItem = pObj->pListItems;
			int n = (int)pObj->fdefValue;

			while ( pItem )
			{
				if (!activeItem--)
					break;

				pItem = pItem->pNext;
			}

			if ( pItem )
			{
				sprintf( szValue, "%s", pItem->szValue );
			}
			else  // Couln't find index
			{
				assert(!("Couldn't find string in list, using default value"));
				sprintf( szValue, "%s", pObj->defValue );
			}
			break;
		}

		// Remove double quotes and % characters
		UTIL_StripInvalidCharacters( szValue );

		strcpy( strValue, szValue );

		pObj->SetCurValue( strValue );

		pList = pList->next;
	}
}

void CMultiplayerAdvanced::CreateControls()
{
	DestroyControls();

	// Go through desciption creating controls
	CScriptObject *pObj;

	pObj = m_pDescription->pObjList;

	mpcontrol_t	*pCtrl;

	CheckButton *pBox;
	TextEntry *pEdit;
	ComboBox *pCombo;
	CScriptListItem *pListItem;

	Panel *objParent = m_pListPanel;

	while ( pObj )
	{
		pCtrl = new mpcontrol_t( objParent, "mpcontrol_t" );
		pCtrl->type = pObj->type;

		switch ( pCtrl->type )
		{
		case O_BOOL:
			pBox = new CheckButton( pCtrl, "DescCheckButton", pObj->prompt );
			pBox->SetSelected( pObj->fdefValue != 0.0f ? true : false );
			
			pCtrl->pControl = (Panel *)pBox;
			break;
		case O_STRING:
		case O_NUMBER:
			pEdit = new TextEntry( pCtrl, "DescTextEntry");
			pEdit->InsertString(pObj->defValue);
			pCtrl->pControl = (Panel *)pEdit;
			break;
		case O_LIST:
			pCombo = new ComboBox( pCtrl, "DescComboBox", 5, false );

			pListItem = pObj->pListItems;
			while ( pListItem )
			{
				pCombo->AddItem( pListItem->szItemText, NULL );
				pListItem = pListItem->pNext;
			}

			pCombo->ActivateItemByRow((int)pObj->fdefValue);

			pCtrl->pControl = (Panel *)pCombo;
			break;
		default:
			break;
		}

		if ( pCtrl->type != O_BOOL )
		{
			pCtrl->pPrompt = new vgui::Label( pCtrl, "DescLabel", "" );
			pCtrl->pPrompt->SetContentAlignment( vgui::Label::a_west );
			pCtrl->pPrompt->SetTextInset( 5, 0 );
			pCtrl->pPrompt->SetText( pObj->prompt );
		}

		pCtrl->pScrObj = pObj;
		pCtrl->SetSize( 100, 28 );
		//pCtrl->SetBorder( scheme()->GetBorder(1, "DepressedButtonBorder") );
		m_pListPanel->AddItem( pCtrl );

		// Link it in
		if ( !m_pList )
		{
			m_pList = pCtrl;
			pCtrl->next = NULL;
		}
		else
		{
			mpcontrol_t *p;
			p = m_pList;
			while ( p )
			{
				if ( !p->next )
				{
					p->next = pCtrl;
					pCtrl->next = NULL;
					break;
				}
				p = p->next;
			}
		}

		pObj = pObj->pNext;
	}
}

void CMultiplayerAdvanced::DestroyControls()
{
	mpcontrol_t *p, *n;

	p = m_pList;
	while ( p )
	{
		n = p->next;
		//
		delete p->pControl;
		delete p->pPrompt;
		delete p;
		p = n;
	}

	m_pList = NULL;
}

void CMultiplayerAdvanced::SaveValues() 
{
	// Get the values from the controls:
	GatherCurrentValues();

	// Create the game.cfg file
	if ( m_pDescription )
	{
		FileHandle_t fp;

		// Add settings to config.cfg
		m_pDescription->WriteToConfig();

		fp = filesystem()->Open( "user.scr", "wb" );
		if ( fp )
		{
			m_pDescription->WriteToScriptFile( fp );
			filesystem()->Close( fp );
		}
	}
}

void CInfoDescription::WriteScriptHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";
	tm newtime;
	g_pVCR->Hook_LocalTime( &newtime );

	filesystem()->FPrintf( fp, (char *)getHint() );

// Write out the comment and Cvar Info:
	filesystem()->FPrintf( fp, "// Half-Life User Info Configuration Layout Script (stores last settings chosen, too)\r\n" );
	filesystem()->FPrintf( fp, "// File generated:  %.19s %s\r\n", asctime( &newtime ), am_pm );
	filesystem()->FPrintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
	filesystem()->FPrintf( fp, "VERSION %.1f\r\n\r\n", SCRIPT_VERSION );
	filesystem()->FPrintf( fp, "DESCRIPTION INFO_OPTIONS\r\n{\r\n" );
}

void CInfoDescription::WriteFileHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";
	tm newtime;
	g_pVCR->Hook_LocalTime( &newtime );

	filesystem()->FPrintf( fp, "// Half-Life User Info Configuration Settings\r\n" );
	filesystem()->FPrintf( fp, "// DO NOT EDIT, GENERATED BY HALF-LIFE\r\n" );
	filesystem()->FPrintf( fp, "// File generated:  %.19s %s\r\n", asctime( &newtime ), am_pm );
	filesystem()->FPrintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
}

CInfoDescription::CInfoDescription( CPanelListPanel *panel )
: CDescription( panel )
{
	setHint( "// NOTE:  THIS FILE IS AUTOMATICALLY REGENERATED, \r\n\
//DO NOT EDIT THIS HEADER, YOUR COMMENTS WILL BE LOST IF YOU DO\r\n\
// User options script\r\n\
//\r\n\
// Format:\r\n\
//  Version [float]\r\n\
//  Options description followed by \r\n\
//  Options defaults\r\n\
//\r\n\
// Option description syntax:\r\n\
//\r\n\
//  \"cvar\" { \"Prompt\" { type [ type info ] } { default } }\r\n\
//\r\n\
//  type = \r\n\
//   BOOL   (a yes/no toggle)\r\n\
//   STRING\r\n\
//   NUMBER\r\n\
//   LIST\r\n\
//\r\n\
// type info:\r\n\
// BOOL                 no type info\r\n\
// NUMBER       min max range, use -1 -1 for no limits\r\n\
// STRING       no type info\r\n\
// LIST         "" delimited list of options value pairs\r\n\
//\r\n\
//\r\n\
// default depends on type\r\n\
// BOOL is \"0\" or \"1\"\r\n\
// NUMBER is \"value\"\r\n\
// STRING is \"value\"\r\n\
// LIST is \"index\", where index \"0\" is the first element of the list\r\n\r\n\r\n" );

	setDescription( "INFO_OPTIONS" );
}