//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SteamPasswordDialog.h"

#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/TextEntry.h>

#include <stdio.h>
#include "EngineInterface.h"
#include "UtlBuffer.h"
#include "FileSystem.h"
using namespace vgui;

#include "GameUI_Interface.h"
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

char *szMainMessage = "Your Steam ticket is about to expire.";

char *szNormalPromptTemplate = "%s\nPlease enter your Steam Password For:\n\n\t\tAccount: %s\n\t\t     User: %s";
char *szBadPasswordPromptTemplate = "%s\nInvalid Password...\nPlease re-enter your Steam Password For:\n\t\tAccount: %s\n\t\t     User: %s";

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CSteamPasswordDialog::CSteamPasswordDialog( vgui::Panel *parent, const char *szAccountName, const char *szUserName ) : CTaskFrame(parent, "SteamPasswordDialog")
{
	SetBounds(0, 0, 372, 160);
	SetSizeable( false );

	strcpy( m_szAccountName, szAccountName );
	strcpy( m_szUserName, szUserName );

	MakePopup();
	g_pTaskbar->AddTask(GetVPanel());

	SetTitle("#GameUI_SteamPassword", true);

	new vgui::Label( this, "HelpText", "" );
	char szMsg[100+sizeof(szMainMessage)+sizeof(m_szAccountName)+sizeof(m_szUserName)];
	sprintf(szMsg, szNormalPromptTemplate, szMainMessage, m_szAccountName, m_szUserName);
	SetControlString("HelpText", szMsg);

	m_pPassword  = new TextEntry(this, "PasswordEdit");
	m_pPassword->AddActionSignalTarget(this);

	vgui::Button *done = new vgui::Button( this, "Done", "#GameUI_OK" );
	done->SetCommand( "Close" );

	vgui::Button *cancel = new vgui::Button( this, "QuitGame", "#GameUI_Quit" );
	cancel->SetCommand( "QuitGame" );

	LoadControlSettings("Resource\\SteamPasswordDialog.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSteamPasswordDialog::~CSteamPasswordDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CSteamPasswordDialog::OnCommand( const char *command )
{
	// Give them the option of simply quiting out of the game.
	if ( stricmp( command, "QuitGame" ) == 0 )
	{
		engine->ClientCmd("quit\n" );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSteamPasswordDialog::OnClose()
{
	char buf[256];

	// If we have a good password, we're done.
	m_pPassword->GetText(buf, 255);
	if ( _isValidPassword_TempTestImp(buf) )
	{
		BaseClass::OnClose();
		MarkForDeletion();
	}

	// Otherwise inform the user and stick around.
	else
	{
		char szMsg[100+sizeof(szMainMessage)+sizeof(m_szAccountName)+sizeof(m_szUserName)];
		sprintf(szMsg, szBadPasswordPromptTemplate, szMainMessage, m_szAccountName, m_szUserName);
		SetControlString("HelpText", szMsg);
	}
}

//-----------------------------------------------------------------------------
// Purpose: temporary test method for entered password validity.
//-----------------------------------------------------------------------------
bool CSteamPasswordDialog::_isValidPassword_TempTestImp( char *szPassword )
{
	// For test purposes a valid password is anything other than "bad".
	return ( stricmp( szPassword, "bad" ) != 0 );
}
