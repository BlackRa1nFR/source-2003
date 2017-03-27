//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <game_controls/ClientMOTD.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>

#include <vgui_controls/HTML.h>
#include <vgui_controls/Button.h>
#include <FileSystem.h>

#include <game_controls/IViewPort.h>

#include "IGameUIFuncs.h" // for key bindings
extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClientMOTD::CClientMOTD(vgui::Panel *parent) : Frame(parent, "ClientMOTD")
{
	m_bFileWritten = false;
	strcpy( m_szTempFileName, "Resource/motd_temp.html");
	m_iScoreBoardKey = KEY_NONE;

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetProportional(true);

//	SetTitleBarVisible( false ); (do this in the .res file)

	m_pMessage = new HTML(this,"Message");

	LoadControlSettings("Resource/UI/MOTD.res");
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CClientMOTD::~CClientMOTD()
{
	// save the file to disk and then load it
	if( filesystem()->FileExists( m_szTempFileName ) )
	{
		filesystem()->RemoveFile( m_szTempFileName ,"GAME");
	}
}

//-----------------------------------------------------------------------------
// Purpose: called when a button is pressed
//-----------------------------------------------------------------------------
void CClientMOTD::OnCommand( const char *command)
{
    if (!stricmp(command, "okay"))
    {
		// save the file to disk and then load it
		if( filesystem()->FileExists( m_szTempFileName ) )
		{
			filesystem()->RemoveFile( m_szTempFileName ,"GAME");
		}
		Close();
	}

	BaseClass::OnCommand(command);
}

// IClientMOTD interface calls

//-----------------------------------------------------------------------------
// Purpose: shows the MOTD
//-----------------------------------------------------------------------------
void CClientMOTD::Activate( const char *title, const char *msg )
{
	BaseClass::Activate();

	SetTitle( title, false );
	SetControlString( "serverName", title );

	if( IsURL( msg ) )
	{
		m_pMessage->OpenURL( msg );
	}
	else
	{ 
		// save the file to disk and then load it
		if( filesystem()->FileExists( m_szTempFileName ) )
		{
			filesystem()->RemoveFile( m_szTempFileName ,"GAME");
		}
		
		FileHandle_t f = filesystem()->Open( m_szTempFileName, "w+", "GAME" );

		if ( f )
		{
			filesystem()->Write( msg, strlen( msg ), f);
			filesystem()->Close( f );

			char localURL[ _MAX_PATH + 7 ];
			strcpy( localURL, "file:///");
			
			int len = filesystem()->GetLocalPathLen( m_szTempFileName );
			char *pPathData = (char*)stackalloc( len );

			filesystem()->GetLocalPath( m_szTempFileName, pPathData );
			Q_strncat( localURL, pPathData, sizeof( localURL ) );
			
			m_pMessage->OpenURL( localURL );

		}

	}
	
	if ( m_iScoreBoardKey == KEY_NONE ) 
	{
		m_iScoreBoardKey = gameuifuncs->GetVGUI2KeyCodeForBind( "showscores" );
	}

	SetVisible( true );
}

void CClientMOTD::Activate( const wchar_t *title, const wchar_t *msg )
{
	BaseClass::Activate();

	SetTitle( title, false );
	SetLabelText( "serverName", title );
	char ansiURL[256];
	localize()->ConvertUnicodeToANSI( msg, ansiURL, sizeof( ansiURL ));

	if( IsURL( ansiURL ) )
	{
		m_pMessage->OpenURL( ansiURL );
	}
	else
	{ 
		// save the file to disk and then load it
		if( filesystem()->FileExists( m_szTempFileName ) )
		{
			filesystem()->RemoveFile( m_szTempFileName ,"GAME");
		}
		
		FileHandle_t f = filesystem()->Open( m_szTempFileName, "w+", "GAME" );

		if ( f )
		{
			filesystem()->Write( msg, wcslen( msg ), f);
			filesystem()->Close( f );

			char localURL[ _MAX_PATH + 7 ];
			strcpy( localURL, "file:///");

			int len = filesystem()->GetLocalPathLen( m_szTempFileName );
			char *pPathData = (char*)stackalloc( len );

			filesystem()->GetLocalPath( m_szTempFileName, pPathData );
			Q_strncat( localURL, pPathData, sizeof( localURL ) );
			
			m_pMessage->OpenURL( localURL );

		}
	}
	SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: sets the localized text of a label
//-----------------------------------------------------------------------------
void CClientMOTD::SetLabelText(const char *textEntryName, const wchar_t *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

void CClientMOTD::OnKeyCodeTyped( KeyCode key )
{
	if ( key == KEY_SPACE )
	{
		OnCommand("okay");
	}
	else if ( m_iScoreBoardKey!=KEY_NONE && m_iScoreBoardKey == key )
	{
		if ( !gViewPortInterface->IsScoreBoardVisible() )
		{
			gViewPortInterface->HideBackGround();
			gViewPortInterface->ShowScoreBoard();
			SetVisible( false );
		}
	}
	else
	{
		BaseClass::OnKeyCodeTyped( key );
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the string looks like a url
//-----------------------------------------------------------------------------
bool CClientMOTD::IsURL( const char *str )
{
	bool isUrl = false;

	if( strlen( str ) > 7 && str[0] == 'h' && str[1] == 't' && str[2] == 't' && str[3] == 'p' && str[4] == ':' 
		&& str[5] == '/' && str[6] =='/')
	{
		isUrl = true;
	}

	return isUrl;
}

