//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Client DLL VGUI2 Viewport
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#pragma warning( disable : 4800  )  // disable forcing int to bool performance warning

// VGUI panel includes
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui/Cursor.h>
#include <vgui/IScheme.h>
#include <vgui/IVGUI.h>
#include <vgui/ILocalize.h>
#include <vgui/VGUI.h>
#include <vgui_controls/AnimationController.h>

// client dll/engine defines
#include "hud.h"
#include "parsemsg.h" // BEGIN_READ, READ_*

// sub dialogs - for vgui_TeamFortressViewport.h
#include <game_controls/ClientScoreBoardDialog.h>
#include <game_controls/ClientMOTD.h>
#include <game_controls/SpectatorGUI.h>
#include <game_controls/VGUITextWindow.h>

// cstrike specific dialogs
#include "CstrikeTeamMenu.h"
#include "CstrikeClassMenu.h"
#include "BuyMenu.h"
#include "CstrikeSpectatorGUI.h"
#include <game_controls/MapOverview.h>
#include "CstrikeClientScoreBoard.h"
#include "clientmode_csnormal.h"

#include "IGameUIFuncs.h"
#include <cl_dll/IVGuiClientDll.h>


// viewport definitions
#include <game_controls/vgui_TeamFortressViewport.h>
#include "CounterStrikeViewport.h"
#include "cs_gamerules.h"
#include "c_user_message_register.h"
#include "vguicenterprint.h"
#include "text_message.h"

ICSViewPortMsgs *gCSViewPortMsgs = NULL;
extern VGuiLibraryInterface_t clientInterface;
extern ConVar hud_autoreloadscript;

CON_COMMAND( buyequip, "Buy CStrike equipment" )
{
	if ( gViewPortInterface ) 
		gViewPortInterface->ShowVGUIMenu( MENU_EQUIPMENT );
}

CON_COMMAND( buy, "Buy CStrike items" )
{
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( pPlayer && pPlayer->CanShowBuyMenu() )
	{
		gViewPortInterface->ShowVGUIMenu( MENU_BUY );
	}
	else
	{
		internalCenterPrint->Print( hudtextmessage->LookupString( "#Cstrike_Not_In_Buy_Zone" ) );
	}
}

CON_COMMAND( chooseteam, "Choose a new team" )
{
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( pPlayer && pPlayer->CanShowTeamMenu() )
		gViewPortInterface->ShowVGUIMenu( MENU_TEAM );
}

CON_COMMAND( commandmenu, "Show command menu" )
{
	if ( gViewPortInterface )
		gViewPortInterface->ShowCommandMenu();
}

CON_COMMAND( spec_help, "Show spectator help screen")
{
	if ( gViewPortInterface )
		gViewPortInterface->ShowVGUIMenu( MENU_SPECHELP );
}

CON_COMMAND( spec_menu, "Activates spectator menu")
{
	bool showIt = true;

	if ( engine->Cmd_Argc() == 2 )
	{
		 showIt = atoi( engine->Cmd_Argv( 1 ) ) == 1;
	}

	if ( !gViewPortInterface )
		return;

	if( showIt  )
	{
		gViewPortInterface->GetSpectatorInterface()->ShowGUI();
	}
	else
	{
		gViewPortInterface->GetSpectatorInterface()->HideGUI();
	}
}

CON_COMMAND( togglescores, "Toggles score panel")
{
	if ( gViewPortInterface )
	{
		if (gViewPortInterface->IsScoreBoardVisible() )
		{
			gViewPortInterface->HideScoreBoard();
		}
		else
		{
			gViewPortInterface->ShowScoreBoard();
		}
	}
}

CON_COMMAND( overview_zoom, "Sets overview map zoom" )
{
	if ( engine->Cmd_Argc() == 2 )
	{
		gViewPortInterface->GetMapOverviewInterface()->SetZoom( atof(engine->Cmd_Argv( 1 )) );
	}
}

CON_COMMAND( overview_size, "Sets overview map size" )
{
	if ( engine->Cmd_Argc() == 2 )
	{
		int s = atoi(engine->Cmd_Argv( 1 ));
		gViewPortInterface->GetMapOverviewInterface()->SetBounds( 0,0, s, s );
		gViewPortInterface->GetSpectatorInterface()->Update();
	}
}


//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CounterStrikeViewport::CounterStrikeViewport() : m_CursorNone(vgui::dc_none)
{
	gCSViewPortMsgs = this;

	SetCursor( m_CursorNone );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);

	// use a custom scheme for the hud
	m_pAnimController = new vgui::AnimationController(this);
	// create our animation controller
	m_pAnimController->SetScheme(scheme);
	m_pAnimController->SetProportional(true);
	if (!m_pAnimController->SetScriptFile("scripts/HudAnimations.txt"))
	{
		Assert(0);
	}

	SetProportional(true);

	ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
CounterStrikeViewport::~CounterStrikeViewport() 
{

}

void CounterStrikeViewport::Enable()
{
	
}

void CounterStrikeViewport::Disable()
{
}

void CounterStrikeViewport::OnThink()
{
	m_pAnimController->UpdateAnimations( gpGlobals->curtime );

	// check the auto-reload cvar
	m_pAnimController->SetAutoReloadScript(hud_autoreloadscript.GetBool());
}

void CounterStrikeViewport::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	gHUD.InitColors( pScheme );

	SetPaintBackgroundEnabled( false );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CounterStrikeViewport::ReloadScheme()
{
	// See if scheme should change
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;
	int team = pPlayer->GetTeamNumber();
	if ( team )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile( "resource/ClientScheme.res", "HudScheme" );

		SetScheme(scheme);
		SetProportional( true );
		m_pAnimController->SetScheme(scheme);
	}

	// Force a reload
	if ( !m_pAnimController->SetScriptFile("scripts/HudAnimations.txt", true) )
	{
		Assert( 0 );
	}

	SetProportional( true );
	
	// reload the .res file from disk
	LoadControlSettings("scripts/HudLayout.res");

	gHUD.RefreshHudTextures();

	InvalidateLayout( true, true );
}


//-----------------------------------------------------------------------------
// Purpose: called when engine starts up initially
//-----------------------------------------------------------------------------
void CounterStrikeViewport::Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager * gameeventmanager )
{
	// construct our panels
	m_pClassMenu = NULL; 
	m_pTeamMenu = new CCSTeamMenu(NULL);
	m_pBuyMenu = NULL; // buy menu needs money info, so create it at the last possible moment

	SetClientDllInterface( &clientInterface );

	BaseClass::Start( pGameUIFuncs, gameeventmanager );

	m_pPrivateSpectatorGUI = new CCSSpectatorGUI( NULL );
	m_pSpectatorGUI = m_pPrivateSpectatorGUI; // override spectator interface so the viewport uses our spectatorUI

	m_pPrivateClientScoreBoard = new CCSClientScoreBoardDialog( NULL );
	m_pClientScoreBoard = m_pPrivateClientScoreBoard;
}

//-----------------------------------------------------------------------------
// Purpose: sets the parents on our subpanels
//-----------------------------------------------------------------------------
void CounterStrikeViewport::SetParent(vgui::VPANEL parent)
{
	BaseClass::SetParent( parent );
	if ( m_pClassMenu )
	{
		m_pClassMenu->SetParent( parent );
	}
	m_pTeamMenu->SetParent( parent );
	if ( m_pBuyMenu )
	{
		m_pBuyMenu->SetParent( parent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: called on each level change
//-----------------------------------------------------------------------------
void CounterStrikeViewport::OnLevelChange(const char * mapname)
{
	BaseClass::OnLevelChange( mapname );
}

//-----------------------------------------------------------------------------
// Purpose: hides all vgui menus
//-----------------------------------------------------------------------------
void CounterStrikeViewport::HideAllVGUIMenu( void )
{
	BaseClass::HideAllVGUIMenu();
	if ( m_pClassMenu )
	{
		m_pClassMenu->SetVisible(false);
	}
	m_pTeamMenu->SetVisible(false);
	if ( m_pBuyMenu )
	{
		m_pBuyMenu->SetVisible(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: process key input
//-----------------------------------------------------------------------------
int	 CounterStrikeViewport::KeyInput( int down, int keynum, const char *pszCurrentBinding )
{
// DON'T run the base class code here, enter shouldn't bring up the team menu in CS

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: hides a particular vgui menu
//-----------------------------------------------------------------------------
void CounterStrikeViewport::HideVGUIMenu( int iMenu )
{
	switch( iMenu )
	{
	case MENU_TERRORIST:
	case MENU_CT:
		if ( m_pClassMenu )
		{
			m_pClassMenu->SetVisible( false );
		}
		break;
	case MENU_TEAM:
		if ( m_pTeamMenu )
		{
			m_pTeamMenu->SetVisible( false );
		}
		break;
	case MENU_BUY:
	case MENU_EQUIPMENT:
		if ( m_pBuyMenu )
		{
			m_pBuyMenu->SetVisible( false );
		}
		break;
	default:
		BaseClass::HideVGUIMenu( iMenu );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the iMenu is visible
//-----------------------------------------------------------------------------
bool CounterStrikeViewport::IsVGUIMenuActive( int iMenu )
{
	switch( iMenu )
	{
	case MENU_TERRORIST:
	case MENU_CT:
		if ( m_pClassMenu )
		{
			return (m_pClassMenu->IsVisible() || m_pClientScoreBoard->IsVisible() ) ;
		}
		else
		{
			return false;
		}
		break;
	case MENU_TEAM:
		if ( m_pTeamMenu )
		{
			return (m_pTeamMenu->IsVisible() || m_pClientScoreBoard->IsVisible()) ;
		}
		else
		{
			return false;
		}
		break;
	case MENU_BUY:
	case MENU_EQUIPMENT:
		if ( m_pBuyMenu )
		{
			return m_pBuyMenu->IsVisible();
		}
		else
		{
			return false;
		}
		break;
	default:
		return BaseClass::IsVGUIMenuActive( iMenu );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: displays the menu specified by iMenu
//-----------------------------------------------------------------------------
void CounterStrikeViewport::DisplayVGUIMenu( int iMenu )
{
	m_pBackGround->SetVisible( true );

	switch( iMenu )
	{
	case MENU_TERRORIST:
		
		if ( m_pClassMenu )
		{
			m_pClassMenu->MarkForDeletion(); // kill the old menu
		}
		g_lastPanel = NULL;
		
		m_pClassMenu = new CCSClassMenu(GetViewPortPanel());

		m_pClassMenu->Update(NULL, TEAM_TERRORIST);
		m_pClassMenu->Activate();
		break;
	case MENU_CT:
	
		if ( m_pClassMenu )
		{
			m_pClassMenu->MarkForDeletion(); // kill the old menu
		}
		g_lastPanel = NULL;
		
		m_pClassMenu = new CCSClassMenu(GetViewPortPanel());

		m_pClassMenu->Update(NULL, TEAM_CT);
		m_pClassMenu->Activate();
		break;
	case MENU_TEAM:
		m_pTeamMenu->Update( clientInterface.GetLevelName(), clientInterface.IsSpectatingAllowed(), const_cast<const char **>(m_sTeamNames), BaseClass::GetNumberOfTeams() );
		m_pTeamMenu->Activate();
		break;
	case MENU_BUY:
		if ( !m_pBuyMenu )
		{
			m_pBuyMenu = new CBuyMenu( GetViewPortPanel() );
		}
		m_pBuyMenu->ActivateBuyMenu();
		break;
	case MENU_EQUIPMENT:
		if ( !m_pBuyMenu )
		{
			m_pBuyMenu = new CBuyMenu( GetViewPortPanel() );
		}
		m_pBuyMenu->ActivateEquipmentMenu();
		break;
	default:
		BaseClass::DisplayVGUIMenu( iMenu );
		break;
	}

}

void CounterStrikeViewport::UpdateSpectatorPanel()
{
	BaseClass::UpdateSpectatorPanel();
	m_pSpectatorGUI->UpdateTimer();
}

//-----------------------------------------------------------------------------
// Purpose: this gets called each frame, used to update spectator gui label
//-----------------------------------------------------------------------------
void CounterStrikeViewport::OnTick()
{
	BaseClass::OnTick();
	if ( m_pSpectatorGUI->IsGUIVisible() )
	{
		m_pSpectatorGUI->UpdateTimer();
	}
}

int CounterStrikeViewport::MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf )
{
	//MIKETODO: team state networking
	/*
	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE();
	
	if ( cl > 0 && cl <= MAX_PLAYERS )
	{  
		// set the players team
		strncpy( g_PlayerExtraInfo[cl].teamname, READ_STRING(), MAX_TEAM_NAME );

		if (!stricmp(g_PlayerExtraInfo[cl].teamname, "TEAM_CT"))
		{
			g_PlayerExtraInfo[cl].teamnumber = 2;
		}
		else if (!stricmp(g_PlayerExtraInfo[cl].teamname, "TEAM_TERRORIST"))
		{
			g_PlayerExtraInfo[cl].teamnumber = 1;
		}
		else
		{
			g_PlayerExtraInfo[cl].teamnumber = 0;
		}
	}

	// rebuild the list of teams
	GetAllPlayersInfo();
	m_pClientScoreBoard->RebuildTeams(GetServerName(),gHUD.m_Teamplay,  m_pSpectatorGUI->IsVisible());
	*/

	return 1;
}

int CounterStrikeViewport::MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf )
{
	//MIKETODO: team state networking
	/*
	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE();
	short frags = READ_SHORT();
	short deaths = READ_SHORT();
	short playerclass = READ_SHORT();
	short teamnumber = READ_SHORT();

	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_PlayerExtraInfo[cl].frags = frags;
		g_PlayerExtraInfo[cl].deaths = deaths;
		g_PlayerExtraInfo[cl].playerclass = playerclass;

		// don't update this as it can be WRONG!, 
		//	g_PlayerExtraInfo[cl].teamnumber = teamnumber;

		//Dont go bellow 0!
		if ( g_PlayerExtraInfo[cl].teamnumber < 0 )
			 g_PlayerExtraInfo[cl].teamnumber = 0;

		if (m_pTeamMenu)
			m_pTeamMenu->Update( BaseClass::GetMapName(), BaseClass::GetAllowSpectators(), const_cast<const char **>(m_sTeamNames), BaseClass::GetNumberOfTeams() );

		GetAllPlayersInfo();
		m_pClientScoreBoard->Update(GetServerName(),static_cast<bool>(gHUD.m_Teamplay),  m_pSpectatorGUI->IsVisible() );
		m_pSpectatorGUI->UpdateSpectatorPlayerList();
	}
	*/

	return 1;
}

// Message handler for TeamScore message
// accepts three values:
//		string: team name
//		short: teams kills
//		short: teams deaths 
// if this message is never received, then scores will simply be the combined totals of the players.
int CounterStrikeViewport::MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *TeamName = READ_STRING();

	// find the team matching the name
	for ( int i = 1; i <= BaseClass::GetNumberOfTeams(); i++ )
	{
		if ( !strnicmp( TeamName, m_pClientScoreBoard->GetTeamName( i ), strlen(TeamName) ) )
			break;
	}

	if ( i >BaseClass::GetNumberOfTeams() && i< MAX_TEAMS) // new team
	{
		BaseClass::SetNumberOfTeams(GetNumberOfTeams()+1);
		if ( !m_sTeamNames[i] || strlen( m_sTeamNames[i] ) < (strlen(TeamName) + 1 ) ) 
		{
			if ( m_sTeamNames[i] )
			{
				free( m_sTeamNames[i] );
			}
			m_sTeamNames[i] = (char *)malloc( strlen( TeamName ) + 1 );
		}
		strcpy( m_sTeamNames[i], TeamName );
		m_pClientScoreBoard->SetTeamName( i, TeamName );
	}

	// use this new score data instead of combined player scores
	//g_TeamInfo[i].scores_overriden = TRUE;
	//g_TeamInfo[i].frags = ;
	//g_TeamInfo[i].deaths = 
	m_pClientScoreBoard->SetTeamDetails( i, READ_SHORT(), 0 );

	return 1;
}


/*
==========================
HUD_ChatInputPosition

Sets the location of the input for chat text
==========================
*/
//MIKETODO: positioning of chat text (and other engine output)
/*
	#include "Exports.h"

	void CL_DLLEXPORT HUD_ChatInputPosition( int *x, int *y )
	{
		RecClChatInputPosition( x, y );
		if ( gViewPortInterface )
		{
			gViewPortInterface->ChatInputPosition( x, y );
		}
	}

	EXPOSE_SINGLE_INTERFACE(CounterStrikeViewport, IClientVGUI, CLIENTVGUI_INTERFACE_VERSION);
*/