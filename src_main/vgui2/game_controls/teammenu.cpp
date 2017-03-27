//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <game_controls/TeamMenu.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/RichText.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>

#include "IGameUIFuncs.h" // for key bindings
extern IGameUIFuncs *gameuifuncs; // for key binding details
#include <game_controls/IViewPort.h>


#include <stdlib.h> // MAX_PATH define
#include <stdio.h>

using namespace vgui;

void UpdateCursorState();
void DuckMessage(const char *str);

// helper function
const char *GetStringTeamColor( int i )
{
	switch( i )
	{
	case 0:
		return "team0";

	case 1:
		return "team1";

	case 2:
		return "team2";

	case 3:
		return "team3";

	case 4:
	default:
		return "team4";
	}
}



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamMenu::CTeamMenu(vgui::Panel *parent) : Frame(parent, "TeamMenu")
{
	m_iNumTeams = 0;
	m_iJumpKey = KEY_NONE; // this is looked up in Activate()
	m_iScoreBoardKey = KEY_NONE; // this is looked up in Activate()

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

	// info window about this map
	m_pMapInfo = new RichText( this, "MapInfo" );

	int i;
	for ( i = 0; i < MAX_VALID_TEAMS; i++ )
	{
			char name[20];
			_snprintf( name, sizeof(name), "teambutton%i",i);
			Button *newButton = new Button( this, name, "" );
			newButton->SetVisible( false );
			m_pTeamButtons.AddToTail( newButton );
	}

	LoadControlSettings("Resource/UI/TeamMenu.res");
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTeamMenu::~CTeamMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CTeamMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	m_pMapInfo->SetFgColor( pScheme->GetColor("MapDescriptionText", Color(255, 255, 255, 0)) ); 
}

//-----------------------------------------------------------------------------
// Purpose: makes the user choose the auto assign option
//-----------------------------------------------------------------------------
void CTeamMenu::AutoAssign()
{
  OnTeamButton( m_iNumTeams );
}


//-----------------------------------------------------------------------------
// Purpose: shows the team menu
//-----------------------------------------------------------------------------
void CTeamMenu::Activate( )
{
	BaseClass::Activate();
	//SetVisible(true);

	if( m_iJumpKey == KEY_NONE ) // you need to lookup the jump key AFTER the engine has loaded
	{
		m_iJumpKey = gameuifuncs->GetVGUI2KeyCodeForBind( "jump" );
	}
	if ( m_iScoreBoardKey == KEY_NONE ) 
	{
		m_iScoreBoardKey = gameuifuncs->GetVGUI2KeyCodeForBind( "showscores" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CTeamMenu::Update( const char *mapName, bool allowSpectators, const char **teamNames, int numTeams )
{
	SetLabelText( "mapname", mapName );
	LoadMapPage( mapName );
	if ( teamNames && teamNames[0] )
	{
		MakeTeamButtons( teamNames, numTeams );
	}
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CTeamMenu::LoadMapPage( const char *mapName )
{
	char mapRES[ _MAX_PATH ];

	_snprintf( mapRES, sizeof( mapRES ), "maps/%s.txt", mapName);

	if( filesystem()->FileExists( mapRES ) )
	{
	}
	else if ( filesystem()->FileExists( "maps/default.txt" ) )
	{
		_snprintf ( mapRES, sizeof( mapRES ), "maps/default.txt");
	}
	else
	{
		return; 
	}

	FileHandle_t f = filesystem()->Open( mapRES, "r" );

	// read into a memory block
	int fileSize = filesystem()->Size(f) ;
	wchar_t *memBlock = (wchar_t *)malloc(fileSize + sizeof(wchar_t));
	memset( memBlock, 0x0, fileSize + sizeof(wchar_t));
	filesystem()->Read(memBlock, fileSize, f);

	// null-terminate the stream
	memBlock[fileSize / sizeof(wchar_t)] = 0x0000;

	// check the first character, make sure this a little-endian unicode file
	if (memBlock[0] != 0xFEFF)
	{
		// its a ascii char file
		m_pMapInfo->SetText( reinterpret_cast<char *>( memBlock ) );
		
	}
	else
	{
		memBlock++;
		m_pMapInfo->SetText( memBlock );
	}
	// go back to the top of the text buffer
	m_pMapInfo->GotoTextStart();

	filesystem()->Close( f );
	free(memBlock);

	InvalidateLayout();
	Repaint();

}

//-----------------------------------------------------------------------------
// Purpose: sets the text on and displays the team buttons
//-----------------------------------------------------------------------------
void CTeamMenu::MakeTeamButtons( const char **teamNames, int numTeams )
{
	int i;
	m_iNumTeams = numTeams;

	for( i = 0; i< m_pTeamButtons.Count(); i++ )
	{
		m_pTeamButtons[i]->SetVisible(false);
	}

	for ( i = 0 ; i< numTeams + 2; i++)// there are always 2 extra buttons, auto-assign and spectate
	{
		char buttonText[32];
		_snprintf( buttonText, sizeof(buttonText), "&%i %s", i +1, teamNames[i] ); 
		m_pTeamButtons[i]->SetText( buttonText );

		m_pTeamButtons[i]->SetCommand( new KeyValues("TeamButton", "team", i ) );	
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		m_pTeamButtons[i]->SetArmedColor(pScheme->GetColor(GetStringTeamColor(i), Color(255, 255, 255, 255))  ,  pScheme->GetColor("SelectionBG", Color(255, 255, 255, 0)) );
		m_pTeamButtons[i]->SetDepressedColor( pScheme->GetColor(GetStringTeamColor(i), Color(255, 255, 255, 255)), pScheme->GetColor("ButtonArmedBgColor", Color(255, 255, 255, 0)) );
		m_pTeamButtons[i]->SetDefaultColor( pScheme->GetColor(GetStringTeamColor(i), Color(255, 255, 255, 255)), pScheme->GetColor("ButtonDepressedBgColor", Color(255, 255, 255, 0)) );
		m_pTeamButtons[i]->SetVisible(true);
	}
}


//-----------------------------------------------------------------------------
// Purpose: When a team button is pressed it triggers this function to cause the player to join a team
//-----------------------------------------------------------------------------
void CTeamMenu::OnTeamButton( int team )
{
	char cmd[64];
	if( team >= m_iNumTeams )  // its a special button
	{
		if( team == m_iNumTeams ) // first extra team is auto assign	
		{
			_snprintf( cmd, sizeof( cmd ), "jointeam 5" );
		}
		else // next is spectate
		{
			DuckMessage( "#Spec_Duck" );
			gViewPortInterface->HideBackGround();
		}
	}
	else
	{
		_snprintf( cmd, sizeof( cmd ), "jointeam %i", team + 1 );
		//g_iTeamNumber = team + 1;
	}

	gViewPortInterface->GetClientDllInterface()->ClientCmd(cmd);
	SetVisible( false );
	OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CTeamMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

void CTeamMenu::OnKeyCodePressed(KeyCode code)
{
	if( m_iJumpKey!=KEY_NONE && m_iJumpKey == code )
	{
		AutoAssign();
	}
	else if ( m_iScoreBoardKey!=KEY_NONE && m_iScoreBoardKey == code )
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
		BaseClass::OnKeyCodePressed( code );
	}
}




//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t CTeamMenu::m_MessageMap[] =
{
	MAP_MESSAGE_INT( CTeamMenu, "TeamButton", OnTeamButton, "team" ),
};
IMPLEMENT_PANELMAP( CTeamMenu, BaseClass );
