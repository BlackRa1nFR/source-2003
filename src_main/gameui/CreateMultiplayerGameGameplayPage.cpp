//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include <time.h>

#include "CreateMultiplayerGameGameplayPage.h"

using namespace vgui;

#include <KeyValues.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>

#include "FileSystem.h"
#include "PanelListPanel.h"
#include "ScriptObject.h"
#include <tier0/vcrmode.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern void UTIL_StripInvalidCharacters( char *pszInput );

//-----------------------------------------------------------------------------
// Purpose: class for loading/saving server config file
//-----------------------------------------------------------------------------
class CServerDescription : public CDescription
{
public:
	CServerDescription( CPanelListPanel *panel );

	void WriteScriptHeader( FileHandle_t fp );
	void WriteFileHeader( FileHandle_t fp ); 
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameGameplayPage::CCreateMultiplayerGameGameplayPage(vgui::Panel *parent, const char *name) : PropertyPage(parent, name)
{
	m_pOptionsList = new CPanelListPanel(this, "GameOptions");

	m_pDescription = new CServerDescription(m_pOptionsList);
	m_pDescription->InitFromFile( "settings.scr" );
	m_pDescription->TransferCurrentValues( NULL );
	m_pList = NULL;

	LoadControlSettings("Resource/CreateMultiplayerGameGameplayPage.res");

	LoadGameOptionsList();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameGameplayPage::~CCreateMultiplayerGameGameplayPage()
{
}

//-----------------------------------------------------------------------------
// Purpose: called to get data from the page
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameGameplayPage::OnApplyChanges()
{
	// Get the values from the controls
	GatherCurrentValues();

	// Create the game.cfg file
	if ( m_pDescription )
	{
		FileHandle_t fp;

		// Add settings to config.cfg
		m_pDescription->WriteToConfig();

		// save out in the settings file
		fp = filesystem()->Open( "settings.scr", "wb" );
		if ( fp )
		{
			m_pDescription->WriteToScriptFile( fp );
			filesystem()->Close( fp );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates all the controls in the game options list
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameGameplayPage::LoadGameOptionsList()
{
	// destroy any existing controls
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


	// Go through desciption creating controls
	CScriptObject *pObj;

	pObj = m_pDescription->pObjList;

	mpcontrol_t	*pCtrl;

	CheckButton *pBox;
	TextEntry *pEdit;
	ComboBox *pCombo;
	CScriptListItem *pListItem;

	Panel *objParent = m_pOptionsList;

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
				pCombo->AddItem(pListItem->szItemText, NULL);
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
		m_pOptionsList->AddItem( pCtrl );

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

//-----------------------------------------------------------------------------
// Purpose: applies all the values in the page
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameGameplayPage::GatherCurrentValues()
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
			pCombo = ( ComboBox *)pList->pControl;
			pCombo->GetText( strValue, sizeof( strValue ) );
			
			pItem = pObj->pListItems;
			int n = (int)pObj->fdefValue;

			while ( pItem )
			{
				if ( !stricmp( pItem->szItemText, strValue ) )
				{
					break;
				}
				pItem = pItem->pNext;
			}

			if ( pItem )
			{
				sprintf( szValue, "%s", pItem->szValue );
			}
			else  //Couln't find index
			{
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


//-----------------------------------------------------------------------------
// Purpose: Constructor, load/save server settings object
//-----------------------------------------------------------------------------
CServerDescription::CServerDescription(CPanelListPanel *panel) : CDescription(panel)
{
	setHint( "// NOTE:  THIS FILE IS AUTOMATICALLY REGENERATED, \r\n"
"//DO NOT EDIT THIS HEADER, YOUR COMMENTS WILL BE LOST IF YOU DO\r\n"
"// Multiplayer options script\r\n"
"//\r\n"
"// Format:\r\n"
"//  Version [float]\r\n"
"//  Options description followed by \r\n"
"//  Options defaults\r\n"
"//\r\n"
"// Option description syntax:\r\n"
"//\r\n"
"//  \"cvar\" { \"Prompt\" { type [ type info ] } { default } }\r\n"
"//\r\n"
"//  type = \r\n"
"//   BOOL   (a yes/no toggle)\r\n"
"//   STRING\r\n"
"//   NUMBER\r\n"
"//   LIST\r\n"
"//\r\n"
"// type info:\r\n"
"// BOOL                 no type info\r\n"
"// NUMBER       min max range, use -1 -1 for no limits\r\n"
"// STRING       no type info\r\n"
"// LIST         "" delimited list of options value pairs\r\n"
"//\r\n"
"//\r\n"
"// default depends on type\r\n"
"// BOOL is \"0\" or \"1\"\r\n"
"// NUMBER is \"value\"\r\n"
"// STRING is \"value\"\r\n"
"// LIST is \"index\", where index \"0\" is the first element of the list\r\n\r\n\r\n" );

	setDescription ( "SERVER_OPTIONS" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerDescription::WriteScriptHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";
    tm newtime;
	g_pVCR->Hook_LocalTime( &newtime );

    if( newtime.tm_hour > 12 )        /* Set up extension. */
		strcpy( am_pm, "PM" );
	if( newtime.tm_hour > 12 )        /* Convert from 24-hour */
		newtime.tm_hour -= 12;    /*   to 12-hour clock.  */
	if( newtime.tm_hour == 0 )        /*Set hour to 12 if midnight. */
		newtime.tm_hour = 12;

	filesystem()->FPrintf( fp, (char *)getHint() );

// Write out the comment and Cvar Info:
	filesystem()->FPrintf( fp, "// Half-Life Server Configuration Layout Script (stores last settings chosen, too)\r\n" );
	filesystem()->FPrintf( fp, "// File generated:  %.19s %s\r\n", asctime( &newtime ), am_pm );
	filesystem()->FPrintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );

	filesystem()->FPrintf( fp, "VERSION %.1f\r\n\r\n", SCRIPT_VERSION );

	filesystem()->FPrintf( fp, "DESCRIPTION SERVER_OPTIONS\r\n{\r\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerDescription::WriteFileHeader( FileHandle_t fp )
{
    char am_pm[] = "AM";
    tm newtime;
	g_pVCR->Hook_LocalTime( &newtime );

    if( newtime.tm_hour > 12 )        /* Set up extension. */
		strcpy( am_pm, "PM" );
	if( newtime.tm_hour > 12 )        /* Convert from 24-hour */
		newtime.tm_hour -= 12;    /*   to 12-hour clock.  */
	if( newtime.tm_hour == 0 )        /*Set hour to 12 if midnight. */
		newtime.tm_hour = 12;

	filesystem()->FPrintf( fp, "// Half-Life Server Configuration Settings\r\n" );
	filesystem()->FPrintf( fp, "// DO NOT EDIT, GENERATED BY HALF-LIFE\r\n" );
	filesystem()->FPrintf( fp, "// File generated:  %.19s %s\r\n", asctime( &newtime ), am_pm );
	filesystem()->FPrintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
}
