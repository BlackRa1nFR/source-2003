//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <game_controls/SpectatorGUI.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>

//#include "pm_shared.h" // OBS_ defines
#include <stdio.h> // _snprintf define
// #include <game_controls/CreateCommandMenu.h>

#include <game_controls/IViewPort.h>
#include <game_controls/CommandMenu.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Menu.h>
#include "IGameUIFuncs.h" // for key bindings
#include <imapoverview.h>
extern IGameUIFuncs *gameuifuncs; // for key binding details

void DuckMessage(const char *str); // from vgui_teamfortressviewport.cpp


const char *GetSpectatorLabel ( int iMode )
{
	//MIKETODO: spectator
	return "blah";
	/*
	switch ( iMode )
	{
		case OBS_CHASE_LOCKED:
			return "#OBS_CHASE_LOCKED";

		case OBS_CHASE_FREE:
			return "#OBS_CHASE_FREE";

		case OBS_ROAMING:
			return "#OBS_ROAMING";
		
		case OBS_IN_EYE:
			return "#OBS_IN_EYE";

		case OBS_MAP_FREE:
			return "#OBS_MAP_FREE";

		case OBS_MAP_CHASE:
			return "#OBS_MAP_CHASE";

		case OBS_NONE:
		default:
			return "#OBS_NONE";
	}
	*/
}


float *GetClientColor( int clientIndex ); // from death.cpp

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: left and right buttons pointing buttons
//-----------------------------------------------------------------------------
class CLeftButton : public vgui::Button
{
public:
	CLeftButton(Panel *parent, const char *panelName): Button(parent, panelName, "") {}

private:
	void ApplySchemeSettings(IScheme *pScheme)
	{
		Button::ApplySchemeSettings(pScheme);
		SetFont(pScheme->GetFont("Marlett", IsProportional() ) );
		SetText("3");
	}
};

class CRightButton : public vgui::Button
{
public:
	CRightButton(Panel *parent, const char *panelName): Button(parent, panelName, "") {}

private:
	void ApplySchemeSettings(IScheme *pScheme)
	{
		Button::ApplySchemeSettings(pScheme);
		SetFont(pScheme->GetFont("Marlett", IsProportional()) );
		SetText("4");
	}
};



//-----------------------------------------------------------------------------
// Purpose: the bottom bar panel, this is a seperate panel because it
// wants mouse input and the main window doesn't
//----------------------------------------------------------------------------
class CBottomBar : public vgui::Frame 
{
private:
	typedef vgui::EditablePanel BaseClass;
public:
	CBottomBar(vgui::Panel *parent, const char *name);
	~CBottomBar() {}

	void SetViewModeText( const char *text ) { m_pViewOptions->SetText( text ); }
	void SetPlayerNameText(const wchar_t *text ); 
	void SetPlayerFgColor( Color c1 ) { m_pPlayerList->SetFgColor(c1); }
	void PlayerClear() { m_pPlayerList->DeleteAllItems(); }
	int  PlayerAddItem( int itemID, wchar_t *name, KeyValues *kv );
	virtual void SetVisible( bool state );

private:
	// VGUI2 overrides
	virtual void OnTextChanged(KeyValues *data);
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed(KeyCode code);
	virtual void ApplySchemeSettings(IScheme *pScheme);

	vgui::ComboBox *m_pPlayerList;
	vgui::ComboBox *m_pViewOptions;
	vgui::ComboBox *m_pConfigSettings;

	CLeftButton *m_pLeftButton;
	CRightButton *m_pRightButton;

	bool m_bChooseTeamCommand;

	KeyCode m_iChooseTeamKey;

	DECLARE_PANELMAP();
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBottomBar::CBottomBar( vgui::Panel *parent, const char *name ) : Frame( parent, name )
{
	m_iChooseTeamKey = KEY_NONE;
	m_bChooseTeamCommand = false;

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
	SetTitleBarVisible( false ); // don't draw a title bar
	SetMoveable( false );
	SetSizeable( false );

	SetScheme("ClientScheme");

	m_pPlayerList = new ComboBox(this, "playercombo", 10 , false);
	m_pViewOptions = new ComboBox(this, "viewcombo", 10 , false );
	m_pConfigSettings = new ComboBox(this, "settingscombo", 10 , false );	

	m_pLeftButton = new CLeftButton( this, "specprev");
	m_pRightButton = new CRightButton( this, "specnext");

	m_pPlayerList->SetText("");
	m_pViewOptions->SetText("#CAM_OPTIONS");
	m_pConfigSettings->SetText("#SPECT_OPTIONS");

	m_pPlayerList->SetOpenDirection( ComboBox::UP );
	m_pViewOptions->SetOpenDirection( ComboBox::UP );
	m_pConfigSettings->SetOpenDirection( ComboBox::UP );

	// create view config menu
	CommandMenu * menu = new CommandMenu(m_pViewOptions, "spectatormenu", gViewPortInterface);
	menu->LoadFromFile( "Resource/spectatormenu.res" );
	m_pConfigSettings->SetMenu( menu );	// attach menu to combo box

	// create view mode menu
	menu = new CommandMenu(m_pViewOptions, "spectatormodes", gViewPortInterface);
	menu->LoadFromFile("Resource/spectatormodes.res");
	m_pViewOptions->SetMenu( menu );	// attach menu to combo box

	// set ourselves to be as wide as the screen
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetSize(wide, GetTall());

	LoadControlSettings("Resource/UI/BottomSpectator.res");
}

void CBottomBar::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// need to MakeReadyForUse() on the menus so we can set their bg color before they are displayed
	m_pConfigSettings->GetMenu()->MakeReadyForUse();
	m_pViewOptions->GetMenu()->MakeReadyForUse();
	m_pPlayerList->GetMenu()->MakeReadyForUse();

	m_pConfigSettings->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
	m_pViewOptions->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
	m_pPlayerList->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
}


//-----------------------------------------------------------------------------
// Purpose: Handles changes to combo boxes
//-----------------------------------------------------------------------------
void CBottomBar::OnTextChanged(KeyValues *data)
{
	Panel *panel = const_cast<vgui::Panel *>( data->GetPtr("panel") );

	vgui::ComboBox *box = dynamic_cast<vgui::ComboBox *>( panel );

	if( box == m_pConfigSettings) // don't change the text in the config setting combo
	{
		m_pConfigSettings->SetText("#SPECT_OPTIONS");
	}
	else if ( box == m_pPlayerList )
	{
		KeyValues *kv = box->GetActiveItemUserData();
		if ( kv )
		{
			const char *player = kv->GetString("player");
			gViewPortInterface->GetClientDllInterface()->FindPlayer(player);
		}
	}
}

void CBottomBar::OnCommand( const char *command )
{
	if (!stricmp(command, "specnext") )
	{
		gViewPortInterface->GetClientDllInterface()->FindNextPlayer( false );
	}
	else if (!stricmp(command, "specprev") )
	{
		gViewPortInterface->GetClientDllInterface()->FindNextPlayer( true );
	}
}

void CBottomBar::SetVisible( bool state )
{
	BaseClass::SetVisible( state );
	if( m_iChooseTeamKey == KEY_NONE )
	{
		m_iChooseTeamKey = gameuifuncs->GetVGUI2KeyCodeForBind( "chooseteam" );
		if(  m_iChooseTeamKey == KEY_NONE ) // also check for changeteam, which is a synonym for chooseteam :)
		{
			m_iChooseTeamKey = gameuifuncs->GetVGUI2KeyCodeForBind( "changeteam" );
		}
		else
		{
			m_bChooseTeamCommand = true;
		}
	}
}


int CBottomBar::PlayerAddItem( int itemID, wchar_t *name, KeyValues *data ) 
{
	if ( m_pPlayerList->IsItemIDValid( itemID ) )
	{	
		m_pPlayerList->UpdateItem( itemID, name, data );
		return itemID + 1;
	}
	else
	{
		return m_pPlayerList->AddItem( name, data ) + 1; 
	}
}




void CBottomBar::OnKeyCodePressed(KeyCode code)
{
	if ( m_iChooseTeamKey!=KEY_NONE && m_iChooseTeamKey == code )
	{
		gViewPortInterface->GetSpectatorInterface()->HideGUI();

		if( m_bChooseTeamCommand )
		{
			gViewPortInterface->GetClientDllInterface()->ClientCmd("chooseteam\n");
		}
		else
		{
			gViewPortInterface->GetClientDllInterface()->ClientCmd("changeteam\n");
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}

}


void CBottomBar::SetPlayerNameText(const wchar_t *text )
{
	char *ansiText = (char *) _alloca( (wcslen( text ) + 1) * sizeof( char ) );
	if ( ansiText )
	{
		localize()->ConvertUnicodeToANSI( text, ansiText, (wcslen( text ) + 1) * sizeof( char ) );
		m_pPlayerList->SetText( ansiText );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t CBottomBar::m_MessageMap[] =
{
	MAP_MESSAGE_PARAMS( CBottomBar, "TextChanged", OnTextChanged),
};

IMPLEMENT_PANELMAP( CBottomBar, EditablePanel );

//-----------------------------------------------------------------------------
// main spectator panel



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSpectatorGUI::CSpectatorGUI(vgui::Panel *parent) : Frame(parent, "SpectatorGUI")
{
	m_bHelpShown = false;
	m_bInsetVisible = false;
	m_iDuckKey = KEY_NONE;

	// initialize dialog
	SetVisible(false);
	SetTitle("SpectatorGUI", true);
	SetProportional(true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );

	// hide the system buttons
	SetTitleBarVisible( false );

	m_pTopBar = new Panel( this, "topbar" );
	m_pBottomBar = new CBottomBar( this, "bottombar" );
 	m_pBottomBarBlank = new Panel( this, "bottombarblank" );

	// m_pBannerImage = new ImagePanel( m_pTopBar, NULL );
	m_pPlayerLabel = new Label( this, "playerlabel", "" );
	m_pPlayerLabel->SetVisible( false );

	// set ourselves to be as wide as the screen
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetSize(wide, 64);
	m_pTopBar->SetSize(wide, 64);
	m_pBottomBar->SetSize(wide, 64);
 	m_pBottomBarBlank->SetSize(wide, 64);

	LoadControlSettings("Resource/UI/Spectator.res");
	
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	m_pBottomBarBlank->SetVisible( true );
	m_pTopBar->SetVisible( true );

	// m_pBannerImage->SetVisible(false);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSpectatorGUI::~CSpectatorGUI()
{
	
}

void CSpectatorGUI::OnCommand( const char *command)
{
    if (!stricmp(command, "okay"))
    {
		m_pPlayerLabel->SetVisible( true );
		m_pPlayerLabel->MoveToFront();
		m_pBottomBar->SetVisible( false );
	}

	BaseClass::OnCommand(command);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the colour of the top and bottom bars
//-----------------------------------------------------------------------------
void CSpectatorGUI::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor(Color( 0,0,0,0 ) ); // make the background transparent
	m_pTopBar->SetBgColor(BLACK_BAR_COLOR);
	m_pBottomBarBlank->SetBgColor(BLACK_BAR_COLOR);
	m_pBottomBar->SetBgColor(Color( 0,0,0,0 ));
	SetPaintBorderEnabled(false);

	SetBorder( NULL );

}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CSpectatorGUI::PerformLayout()
{
	int w,h;
	surface()->GetScreenSize(w, h);

	// fill the screen
	SetBounds(0,0,w,h);
}

//-----------------------------------------------------------------------------
// Purpose: shows the screen
//-----------------------------------------------------------------------------
void CSpectatorGUI::Activate(  )
{
	BaseClass::Activate();
	m_bHelpShown = false;

	SetVisible( true );
	m_pPlayerLabel->SetVisible( true ); // hide the bottom bar by default
	m_pPlayerLabel->MoveToFront();
	m_pBottomBar->SetVisible( false );

	gViewPortInterface->GetMapOverviewInterface()->SetVisible( true );

	if( m_iDuckKey == KEY_NONE ) // you need to lookup the jump key AFTER the engine has loaded
	{
		m_iDuckKey = gameuifuncs->GetVGUI2KeyCodeForBind( "duck" );
	}

	Update();
}

void CSpectatorGUI::ShowGUI()
{
	m_pBottomBar->SetVisible( true ); 
	UpdateSpectatorPlayerList();	
	m_pPlayerLabel->SetVisible( false ); 
	m_pBottomBar->MoveToFront();

}

//-----------------------------------------------------------------------------
// Purpose: Completely hides the spectator GUI
//-----------------------------------------------------------------------------
void CSpectatorGUI::Deactivate()
{
	SetVisible( false );
	HideGUI();
	
	gViewPortInterface->GetMapOverviewInterface()->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Hides the active portion of the GUI (the combo boxes)
//-----------------------------------------------------------------------------
void CSpectatorGUI::HideGUI()
{
	m_pBottomBar->SetVisible( false );
	m_pPlayerLabel->SetVisible( true );
	m_pPlayerLabel->MoveToFront();
	
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the active (combo box) part of the GUI is visible
//-----------------------------------------------------------------------------
bool CSpectatorGUI::IsGUIVisible()
{ 
	return m_pBottomBar->IsVisible(); 
}

//-----------------------------------------------------------------------------
// Purpose: sets the image to display for the banner in the top right corner
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLogoImage(const char *image)
{
	if ( m_pBannerImage )
	{
		m_pBannerImage->SetImage( scheme()->GetImage(image, false) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLabelText(const char *textEntryName, wchar_t *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::MoveLabelToFront(const char *textEntryName)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->MoveToFront();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Updates the gui, rearragnes elements
//-----------------------------------------------------------------------------
void CSpectatorGUI::Update()
{
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	
	if ( gViewPortInterface->GetMapOverviewInterface()->IsVisible() )
	{
		int mx, my, mwide, mtall;

		gViewPortInterface->GetMapOverviewInterface()->GetBounds( mx , my, mwide, mtall );
		m_pTopBar->SetSize( wide - (mx + mwide), 64);
		m_pTopBar->SetPos( (mx + mwide), 0 );
	}
	else
	{
		m_pTopBar->SetSize( wide , 64);
		m_pTopBar->SetPos( 0, 0 );
	}


/*	m_pBottomBar->SetViewModeText( GetSpectatorLabel( User1 ) );

	if( !m_bHelpShown && !m_pBottomBar->IsVisible() && User1 )
	{
		m_bHelpShown = true;
		DuckMessage( "#Spec_Duck" );
	}

	// in first person mode colorize player names
//MIKETODO: spectator GUI

	if ( (User1 == OBS_IN_EYE) && playernum )
	{
		float * color = GetClientColor( playernum );
		int r = static_cast<int>( color[0]*255);
		int g = static_cast<int>( color[1]*255);
		int b = static_cast<int>( color[2]*255);
		
		m_pBottomBar->SetPlayerFgColor( Color( r, g, b, 255 ) );
		m_pPlayerLabel->SetFgColor( Color( r, g, b, 255 ) );
	}
	else
	{	// restore GUI color
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		m_pBottomBar->SetPlayerFgColor( pScheme->GetColor("BaseText", Color(255, 255, 255, 0)) );
		m_pPlayerLabel->SetFgColor( pScheme->GetColor("BaseText", Color(255, 255, 255, 0)) );
	}


	wchar_t playerText[ 80 ], playerName[ 64 ], health[ 10 ];
	wcscpy( playerText, L"Unable to find #Spec_PlayerItem*" );
	memset( playerName, 0x0, sizeof( playerName ) * sizeof( wchar_t ) );
	
	VGuiLibraryPlayer_t playerInfo = gViewPortInterface->GetClientDllInterface()->GetPlayerInfo(playernum);

	localize()->ConvertANSIToUnicode( playerInfo.name , playerName, sizeof( playerName ) );
	if ( playerInfo.health > 0  )
	{
		_snwprintf( health, sizeof( health ), L"%i", playerInfo.health );
		localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem_Team" ), 2, playerName,  health );
	}
	else
	{
		localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem" ), 1, playerName );
	}

	m_pBottomBar->SetPlayerText( playerText );
	m_pPlayerLabel->SetText( playerText );


	// update extra info field
	wchar_t string1[1024];
	if ( spectate_only )
	{
		char numplayers[6];
		wchar_t wNumPlayers[6];

		_snprintf(numplayers,6,"%i",spec_num);
		localize()->ConvertANSIToUnicode(numplayers,wNumPlayers,sizeof(wNumPlayers));
		localize()->ConstructString( string1,sizeof( string1 ), localize()->Find("#Spectators" ),1, wNumPlayers );
	}
	else
	{
		wchar_t wMapName[64];
		// otherwise show map name
		localize()->ConvertANSIToUnicode(mapname,wMapName,sizeof(wMapName));
		localize()->ConstructString( string1,sizeof( string1 ), localize()->Find("#Spec_Map" ),1, wMapName );
	}


	SetLabelText("extrainfo", string1 ); */
}


//-----------------------------------------------------------------------------
// Purpose: Resets the list of players
//-----------------------------------------------------------------------------
void CSpectatorGUI::UpdateSpectatorPlayerList()
{
	if ( !m_pBottomBar->IsVisible() ) 
	{
		return; 
	}

	int itemID = 0;

	m_pBottomBar->PlayerClear();

	for ( int iPlayerIndex = 1 ; iPlayerIndex < gViewPortInterface->GetClientDllInterface()->GetMaxPlayers() ; ++iPlayerIndex )
	{

		VGuiLibraryPlayer_t playerInfo = gViewPortInterface->GetClientDllInterface()->GetPlayerInfo(iPlayerIndex);

		// does this slot in the array have a name?
		if ( playerInfo.name && ( !playerInfo.thisplayer ) ) // we don't want to put ourselves in our own list
		{
			if ( playerInfo.dead == false /*&& CanSpectateTeam( playerInfo.teamnumber )*/ )
			{	
				wchar_t playerText[ 80 ], playerName[ 64 ], *team, teamText[ 64 ];
				char localizeTeamName[64];
				localize()->ConvertANSIToUnicode( playerInfo.name , playerName, sizeof( playerName ) );
				if ( playerInfo.teamname )
				{	
					_snprintf( localizeTeamName, sizeof( localizeTeamName ), "#%s", playerInfo.teamname);
					team=localize()->Find( localizeTeamName );
					if ( !team ) 
					{
						localize()->ConvertANSIToUnicode( playerInfo.teamname , teamText, sizeof( teamText ) );
						team = teamText;
					}

					localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem_Team" ), 2, playerName,  team );
				}
				else
				{
					localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem" ), 1, playerName );
				}

				KeyValues *kv = new KeyValues("UserData", "player", playerInfo.name);
				itemID = m_pBottomBar->PlayerAddItem( itemID, playerText, kv ); // -1 means a new slot
				kv->deleteThis();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the timer label if one exists
//-----------------------------------------------------------------------------
void CSpectatorGUI::UpdateTimer()
{
	wchar_t szText[ 63 ];

	int timer = 0;

	_snwprintf ( szText, sizeof( szText ), L"%d:%02d\n", (timer / 60), (timer % 60) );

	szText[63] = 0;


	SetLabelText("timerlabel", szText );
}

//-----------------------------------------------------------------------------
// Purpose: when duck is pressed it hides the active part of the GUI
//-----------------------------------------------------------------------------
void CSpectatorGUI::OnKeyCodePressed(KeyCode code)
{
	if( m_iDuckKey!=KEY_NONE && m_iDuckKey == code )
	{
		m_pBottomBar->SetVisible( false );
		m_pPlayerLabel->SetVisible( true );
		m_pPlayerLabel->MoveToFront();
	}
	else
	{
		BaseClass::OnKeyCodePressed(code);
	}
}