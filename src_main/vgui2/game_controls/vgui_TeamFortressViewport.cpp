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

#pragma warning( disable : 4800  )  // disable forcing int to bool performance warning

// VGUI panel includes
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>

// client dll/engine defines
//#include "pm_shared.h" // OBS_ defines
//#include "quakedef.h" // MAX_OSPATH, byte
//#include "util.h" // common\util.h
#include <keydefs.h> // K_ENTER, ... define

// sub dialogs
#include <game_controls/ClientScoreBoardDialog.h>
#include <game_controls/ClientMOTD.h>
#include <game_controls/SpectatorGUI.h>
#include <game_controls/TeamMenu.h>
#include <game_controls/ClassMenu.h>
#include <game_controls/VGUITextWindow.h>
#include "IGameUIFuncs.h"
#include <game_controls/commandmenu.h>
#include <game_controls/mapoverview.h>

// our definition
#include <game_controls/vgui_TeamFortressViewport.h>
#include "filesystem.h"

void DuckMessage(const char *str);

IViewPort *gViewPortInterface = NULL;
IViewPortMsgs *gViewPortMsgs = NULL;

vgui::Panel *g_lastPanel = NULL; // used for mouseover buttons, keeps track of the last active panel
using namespace vgui;


bool Helper_LoadFile( IBaseFileSystem *pFileSystem, const char *pFilename, CUtlVector<char> &buf )
{
	FileHandle_t hFile = pFileSystem->Open( pFilename, "rt" );
	if ( hFile == FILESYSTEM_INVALID_HANDLE )
	{
		Warning( "Helper_LoadFile: missing %s", pFilename );
		return false;
	}

	unsigned long len = pFileSystem->Size( hFile );
	buf.SetSize( len );
	pFileSystem->Read( buf.Base(), buf.Count(), hFile );
	pFileSystem->Close( hFile );

	return true;
}


//================================================================
TeamFortressViewport::TeamFortressViewport() : vgui::EditablePanel( NULL, "TeamFortressViewport")
{
	gViewPortInterface = this;
	gViewPortMsgs = this;
	m_pClientDllInterface = NULL;
	m_bInitialized = false;
	m_iNumTeams = 0;
	m_PendingDialogs.Purge();
	m_szServerName[0] = 0;
	m_GameuiFuncs = NULL;
	m_GameEventManager = NULL;
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
}

TeamFortressViewport::~TeamFortressViewport()
{
	m_bInitialized = false;
	for (int i = 0; i < 5; i++)
	{
		if ( m_sTeamNames[i] )
		{
			free( m_sTeamNames[i] );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: called when the VGUI subsystem starts up
//			Creates the sub panels and initialises them
//-----------------------------------------------------------------------------
void TeamFortressViewport::Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager * pGameEventManager )
{
	m_GameuiFuncs = pGameUIFuncs;
	m_GameEventManager = pGameEventManager;

	m_pBackGround = new CBackGroundPanel( NULL );
	m_pBackGround->SetZPos( -20 ); // send it to the back 
	m_pBackGround->SetVisible( false );
	
	m_pPrivateClientScoreBoard = SETUP_PANEL(new CClientScoreBoardDialog(NULL));
	m_pClientScoreBoard = m_pPrivateClientScoreBoard;
	m_pMOTD = new CClientMOTD(NULL);
	m_pPrivateSpectatorGUI = new CSpectatorGUI(NULL);
	m_pSpectatorGUI = m_pPrivateSpectatorGUI;
	m_pTeamMenu = new CTeamMenu(NULL);
	m_pClassMenu = new CClassMenu(NULL);
	m_pSpecHelp = new CSpecHelpWindow(NULL, m_pSpectatorGUI);
	m_pClassHelp = new CVGUITextWindow(NULL);
	m_pMapBriefing = new CVGUITextWindow(NULL);
	m_pMapOverview = new CMapOverview(NULL, m_GameEventManager );

	m_iCurrentTeamNumber = 0;
	Q_strcpy(m_szCurrentMap, "");

	m_pCommandMenu = new CommandMenu(NULL, "commandmenu", gViewPortInterface );
	m_pCommandMenu->LoadFromFile("Resource/commandmenu.res");
		
	// Make sure all menus are hidden
	HideAllVGUIMenu();
	m_pClientScoreBoard->Reset();

	// Clear out some data
	m_iGotAllMOTD = true;
	m_iRandomPC = false;
	m_flScoreBoardLastUpdated = 0;
	
//	strcpy(m_sMapName, "");
	Q_strcpy(m_szServerName, "");
	for (int i = 0; i < 5; i++)
	{
		m_iValidClasses[i] = 0;
		m_sTeamNames[i] = NULL;
	}

	m_bInitialized = true;
}

/*

//-----------------------------------------------------------------------------
// Purpose: displays the spectator gui
//-----------------------------------------------------------------------------
void TeamFortressViewport::ShowSpectatorGUI()
{
	if (!m_bInitialized)
		return;

	m_pBackGround->SetVisible( false );
	m_pSpectatorGUI->Activate();
}

void TeamFortressViewport::ShowSpectatorGUIBar()
{
	if (!m_bInitialized)
		return;

	m_pBackGround->SetVisible( false );
	m_pSpectatorGUI->ShowBottomBar();
}

//-----------------------------------------------------------------------------
// Purpose: hides the spectator gui
//-----------------------------------------------------------------------------
void TeamFortressViewport::HideSpectatorGUI()
{
	if (!m_bInitialized)
		return;

	if( m_pSpectatorGUI->IsVisible() && m_pClientDllInterface->IsSpectator())
	{
		DuckMessage( "#Spec_Duck");
	}

	m_pSpectatorGUI->HideGUI();
}

void TeamFortressViewport::DeactivateSpectatorGUI()
{
	if (!m_bInitialized)
		return;

	m_pSpectatorGUI->DeactivateGUI();

	DuckMessage( "#Spec_Duck" );
}

//-----------------------------------------------------------------------------
// Purpose: returns true if ANY of the spectator GUI is visible
//-----------------------------------------------------------------------------
bool TeamFortressViewport::IsSpectatorGUIVisible()
{
	if (!m_bInitialized)
		return false;
	
	return m_pSpectatorGUI->IsGUIVisible();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the bottom bar of the spectator GUI is visible (i.e the input bit)
//-----------------------------------------------------------------------------
bool TeamFortressViewport::IsSpectatorBarVisible()
{
	if (!m_bInitialized)
		return false;
	
	return m_pSpectatorGUI->IsActiveGUIVisible();
} */

ISpectatorInterface * TeamFortressViewport::GetSpectatorInterface()
{
	return (ISpectatorInterface*) m_pSpectatorGUI;
}


IMapOverview * TeamFortressViewport::GetMapOverviewInterface()
{
	return (IMapOverview*) m_pMapOverview;
}

//-----------------------------------------------------------------------------
// Purpose: sets the image for the spectator banner of the spectator GUI to use
/*-----------------------------------------------------------------------------
void TeamFortressViewport::SetSpectatorBanner(char const *image)
{
	m_pSpectatorGUI->SetImage(image);
}*/

//-----------------------------------------------------------------------------
// Purpose: Bring up the scoreboard
//-----------------------------------------------------------------------------
void TeamFortressViewport::ShowScoreBoard( void )
{
	// No Scoreboard in single-player
	if( CanShowScoreBoard() )
	{
		GetAllPlayersInfo();
		if (!m_bInitialized)
			return;

		m_pClientScoreBoard->Activate( false /*m_pSpectatorGUI->IsVisible()*/ );
		m_pClientScoreBoard->MoveToFront();

	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if we should be drawing a scoreboard
//			default is no for single player
//-----------------------------------------------------------------------------
bool TeamFortressViewport::CanShowScoreBoard( void )
{
	return ( m_pClientDllInterface->GetMaxPlayers() > 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the scoreboard is up
//-----------------------------------------------------------------------------
bool TeamFortressViewport::IsScoreBoardVisible( void )
{
	if (!m_bInitialized)
		return false;
	
	return m_pClientScoreBoard->IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: Hide the scoreboard
//-----------------------------------------------------------------------------
void TeamFortressViewport::HideScoreBoard( void )
{
	// Prevent removal of scoreboard during intermission
	if ( m_pClientDllInterface->InIntermission() || !m_bInitialized)
		return;

	m_pClientScoreBoard->SetVisible(false);

	if( m_PendingDialogs.Count() >= 1  ) // show any active menus on the queue
	{
		int menu = m_PendingDialogs.Head();
	
		m_pBackGround->SetVisible( true );
		if( m_pSpectatorGUI->IsGUIVisible() )
		{
			m_pSpectatorGUI->HideGUI();
		}
		DisplayVGUIMenu( menu );
	}

	m_pClientDllInterface->GameVoice_StopSquelchMode();
}

//-----------------------------------------------------------------------------
// Purpose: Updates the spectator panel with new player info
//-----------------------------------------------------------------------------
void TeamFortressViewport::UpdateSpectatorPanel()
{
	/* MIKETODO: spectator
	char bottomText[128];
	int player = -1;
	const char *name;
	sprintf(bottomText,"#Spec_Mode%d", m_pClientDllInterface->SpectatorMode() );

	m_pClientDllInterface->CheckSettings();
	// check if we're locked onto a target, show the player's name
	if ( (m_pClientDllInterface->SpectatorTarget() > 0) && (m_pClientDllInterface->SpectatorTarget() <= m_pClientDllInterface->GetMaxPlayers()) && (m_pClientDllInterface->SpectatorMode() != OBS_ROAMING) )
	{
		player = m_pClientDllInterface->SpectatorTarget();
	}

		// special case in free map and inset off, don't show names
	if ( ((m_pClientDllInterface->SpectatorMode() == OBS_MAP_FREE) && !m_pClientDllInterface->PipInsetOff()) || player == -1 )
		name = NULL;
	else
		name = m_pClientDllInterface->GetPlayerInfo(player).name;

	// create player & health string
	if ( player && name )
	{
		strcpy( bottomText, name );
	}
	char szMapName[64];
	m_pClientDllInterface->COM_FileBase( const_cast<char *>(m_pClientDllInterface->GetLevelName()), szMapName );

	m_pSpectatorGUI->Update(bottomText, player, m_pClientDllInterface->SpectatorMode(), m_pClientDllInterface->IsSpectateOnly(), m_pClientDllInterface->SpectatorNumber(), szMapName );
	m_pSpectatorGUI->UpdateSpectatorPlayerList();
	*/
}


//================================================================
// VGUI Menus

//-----------------------------------------------------------------------------
// Purpose: Queues the menu iMenu to be displayed when possible
//-----------------------------------------------------------------------------
void TeamFortressViewport::ShowVGUIMenu( int iMenu )
{
	if (!m_bInitialized)
		return;
	
	// Don't open menus in demo playback
	if ( m_pClientDllInterface->IsDemoPlayingBack() )
		return;

	// Don't open any menus except the MOTD during intermission
	// MOTD needs to be accepted because it's sent down to the client 
	// after map change, before intermission's turned off
	if ( m_pClientDllInterface->InIntermission() && iMenu != MENU_INTRO )
		return;


	if(m_PendingDialogs.Count() == 0 )
	{
		m_PendingDialogs.Insert( iMenu ); // we have a new current menu
		m_pBackGround->SetVisible( true );
		if( m_pSpectatorGUI->IsVisible() )
		{
			m_pSpectatorGUI->Deactivate();
		}

		DisplayVGUIMenu( iMenu );
	}
	else
	{
		if ( ! m_PendingDialogs.Check( iMenu ) )	// TFC sends a MENU_TEAM every 2 seconds until a team is chosen...
												// this stops multiple queueing of the same menu
		{
			m_PendingDialogs.Insert( iMenu );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Displays the menu specified by iMenu
//-----------------------------------------------------------------------------
void TeamFortressViewport::DisplayVGUIMenu( int iMenu )
{
	char cTitle[32];
	char sz[64];
	char *pfile = NULL;
	CUtlVector<char> fileData;
	char	mapName[64];

	m_pBackGround->SetVisible( true );

	if( m_pSpectatorGUI->IsVisible() )
	{
		m_pSpectatorGUI->Deactivate();
	}

	m_pClientDllInterface->COM_FileBase( const_cast<char *>(m_pClientDllInterface->GetLevelName()), mapName );

	switch ( iMenu )
	{
	case MENU_TEAM:	
		m_pTeamMenu->Update( mapName, m_pClientDllInterface->IsSpectatingAllowed(), (const char **)m_sTeamNames, m_iNumTeams );
		m_pTeamMenu->Activate();
		break;

	case MENU_INTRO:
		if (!m_szServerName || !m_szServerName[0])
			strncpy( cTitle, "Half-Life" ,MAX_SERVERNAME_LENGTH);
		else
			strncpy( cTitle, m_szServerName, MAX_SERVERNAME_LENGTH );
		cTitle[ MAX_SERVERNAME_LENGTH -1 ] = '\0';

		m_pMOTD->Activate( cTitle, m_szMOTD);
		break;

	case MENU_MAPBRIEFING: // I don't think this option is actually used...
		// Get the current mapname, and open it's map briefing text
		if ( mapName[0] )
		{
			strcpy( sz, "maps/");
			strcat( sz, mapName );
			strcat( sz, ".txt" );
		}
		else
			return;
		
		if ( !Helper_LoadFile( filesystem(), sz, fileData ) )
		{
			Error( "MENU_MAPBRIEFING: %s missing", sz );
		}
		pfile = fileData.Base();

		strncpy( cTitle, mapName, sizeof( cTitle ) );
		cTitle[sizeof( cTitle )-1] = 0;

		m_pMapBriefing->Activate( cTitle, pfile );
		break;

	case MENU_SPECHELP:
		m_pSpectatorGUI->SetVisible(false); // hide the spectator UI, closing the spec help one reveals it again
		m_pSpecHelp->Activate( "#Spec_Help_Title", "#Spec_Help_Text" );
		break;
		
	case MENU_CLASS:
		m_pClassMenu->Update(m_iValidClasses, 5);
		m_pClassMenu->Activate();
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if iMenu's menu is visible (i.e active)
//-----------------------------------------------------------------------------
bool TeamFortressViewport::IsVGUIMenuActive( int iMenu )
{
	switch ( iMenu )
	{
	case MENU_TEAM:		
		return (m_pTeamMenu->IsVisible() || m_pClientScoreBoard->IsVisible());
		break;

	case MENU_INTRO:
		return (m_pMOTD->IsVisible() || m_pClientScoreBoard->IsVisible());
		break;

	case MENU_CLASSHELP:
		return m_pClassHelp->IsVisible();
		break;

	case MENU_SPECHELP:
		return m_pSpecHelp->IsVisible();
		break;

	case MENU_MAPBRIEFING:
		return m_pMapBriefing->IsVisible();
		break;
		
	case MENU_CLASS:
		return (m_pClassMenu->IsVisible() || m_pClientScoreBoard->IsVisible());
		break;

	default:
		break;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: hides the menu specified by iMenu
//-----------------------------------------------------------------------------
void TeamFortressViewport::HideVGUIMenu( int iMenu )
{
	switch ( iMenu )
	{
	case MENU_TEAM:		
		m_pTeamMenu->SetVisible( false );
		break;

	case MENU_INTRO:
		m_pMOTD->SetVisible( false );
		break;

	case MENU_CLASSHELP:
		m_pClassHelp->SetVisible( false );
		break;

	case MENU_SPECHELP:
		m_pSpecHelp->SetVisible( false );
		break;

	case MENU_MAPBRIEFING:
		m_pMapBriefing->SetVisible( false );
		break;
		
	case MENU_CLASS:
		m_pClassMenu->SetVisible( false );
		break;

	default:
		break;
	}
}


// Return TRUE if the HUD's allowed to print text messages
bool TeamFortressViewport::AllowedToPrintText( void )
{
	int iId = GetCurrentMenuID();
	if ( iId == MENU_TEAM || iId == MENU_CLASS || iId == MENU_INTRO || iId == MENU_CLASSHELP )
		return false;
	return true;
}





//======================================================================================
// UPDATE HUD SECTIONS
//======================================================================================
void HudMessage(char *str)
{
	gViewPortInterface->GetClientDllInterface()->MessageHud(NULL,strlen(str)+1,static_cast<void *>(str));
}

//-----------------------------------------------------------------------------
// Purpose: returns true if a particular class is valid for this map
//-----------------------------------------------------------------------------
int GetValidClasses(int playerClass)
{
	return gViewPortInterface->GetValidClasses(playerClass);
}


//-----------------------------------------------------------------------------
// Purpose: Updates each players info structure
//-----------------------------------------------------------------------------
void TeamFortressViewport::GetAllPlayersInfo( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: think function, called in hud_redraw.cpp:Think()
//-----------------------------------------------------------------------------
void TeamFortressViewport::OnTick()
{
	// See if the Spectator Menu needs to be update
	
	if ( m_pClientDllInterface->IsSpectator() && !m_pSpectatorGUI->IsVisible()  ) 
	{
		GetAllPlayersInfo();
		m_pSpectatorGUI->Activate();
	}
	else if ( !m_pClientDllInterface->IsSpectator() && m_pSpectatorGUI->IsVisible() )
	{
		m_pSpectatorGUI->Deactivate();
	}

/*	else if ( false spec panel needs an update )
	{
		UpdateSpectatorPanel();
	}
	

	if ( IsSpectatorGUIVisible() )
	{
		GetAllPlayersInfo();
	} */

	// Update the Scoreboard, if it's visible
	if ( m_pClientScoreBoard->IsVisible() && m_pClientDllInterface->HudTime() > m_flScoreBoardLastUpdated )
	{
		m_flScoreBoardLastUpdated = ((float)m_pClientDllInterface->HudTime() + 1.0);

		GetAllPlayersInfo();
		m_pClientScoreBoard->Update(m_szServerName, m_pClientDllInterface->TeamPlay(), m_pSpectatorGUI->IsVisible());
	}

	if( m_PendingDialogs.Count() > 1 && !IsVGUIMenuActive( m_PendingDialogs.Head() ) ) // last show menu isn't active, show the next
	{
		m_PendingDialogs.RemoveAtHead();
		int menu = m_PendingDialogs.Head();
	
		m_pBackGround->SetVisible( true );
	
		DisplayVGUIMenu( menu );
	}
	else if ( m_PendingDialogs.Count() == 1 && !IsVGUIMenuActive( m_PendingDialogs.Head()) )
	{
		m_PendingDialogs.RemoveAtHead(); // the menu at the top of the queue is not showing so delete it
	}
 
}

//-----------------------------------------------------------------------------
// Purpose: Direct Key Input
//-----------------------------------------------------------------------------
int	TeamFortressViewport::KeyInput( int down, int keynum, const char *pszCurrentBinding )
{
	// Enter gets out of Spectator Mode by bringing up the Team Menu
	if ( m_pClientDllInterface->IsSpectator() )
	{
		if ( down && (keynum == K_ENTER || keynum == K_KP_ENTER) )
			ShowVGUIMenu( MENU_TEAM );
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Activate's the player special ability
//			called when the player hits their "special" key
//-----------------------------------------------------------------------------
void TeamFortressViewport::InputPlayerSpecial( void )
{
	if (!m_bInitialized)
		return;

	// if it's any other class, just send the command down to the server
	m_pClientDllInterface->ClientCmd( "_special" );
}



//-----------------------------------------------------------------------------
// Purpose: Sets the parent for each panel to use
//-----------------------------------------------------------------------------
void TeamFortressViewport::SetParent(vgui::VPANEL parent)
{
	EditablePanel::SetParent( parent );

	m_pBackGround->SetParent( (vgui::VPANEL)parent );
	m_pClientScoreBoard->SetParent((vgui::VPANEL)parent);
	m_pMOTD->SetParent((vgui::VPANEL)parent);
	m_pSpectatorGUI->SetParent((vgui::VPANEL)parent);
	m_pTeamMenu->SetParent((vgui::VPANEL)parent);
	m_pClassMenu->SetParent((vgui::VPANEL)parent);
	m_pSpecHelp->SetParent((vgui::VPANEL)parent);
	m_pClassHelp->SetParent((vgui::VPANEL)parent);
	m_pMapBriefing->SetParent((vgui::VPANEL)parent);
	m_pCommandMenu->SetParent((vgui::VPANEL)parent);
	m_pMapOverview->SetParent((vgui::VPANEL)parent);
	
	m_pClientScoreBoard->SetMouseInputEnabled(false); // the SetParent() call resets this	
}

//-----------------------------------------------------------------------------
// Purpose: called when the engine shows the base client VGUI panel (i.e when entering a new level or exiting GameUI )
//-----------------------------------------------------------------------------
void TeamFortressViewport::ActivateClientUI() 
{
}

//-----------------------------------------------------------------------------
// Purpose: called when the engine hides the base client VGUI panel (i.e when the GameUI is comming up ) 
//-----------------------------------------------------------------------------
void TeamFortressViewport::HideClientUI()
{
}


//-----------------------------------------------------------------------------
// Purpose: updates the scoreboard when something changes
//-----------------------------------------------------------------------------
void TeamFortressViewport::UpdateScoreBoard()
{
	GetAllPlayersInfo();
	m_pClientScoreBoard->RebuildTeams(m_szServerName,m_pClientDllInterface->TeamPlay(),  m_pSpectatorGUI->IsVisible());
}

//-----------------------------------------------------------------------------
// Purpose: passes death msgs to the scoreboard to display specially
//-----------------------------------------------------------------------------
void TeamFortressViewport::DeathMsg( int killer, int victim )
{
	m_pClientScoreBoard->DeathMsg(killer,victim);
	if ( victim == m_pClientDllInterface->GetLocalPlayerIndex() )
	{
		UpdateSpectatorPanel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hides all the VGUI menus at once (except the scoreboard, as it is used in intermissions)
//-----------------------------------------------------------------------------
void TeamFortressViewport::HideAllVGUIMenu()
{
	m_pTeamMenu->SetVisible(false);
	m_pClassMenu->SetVisible(false);
	m_pMOTD->SetVisible(false);
	m_pSpecHelp->SetVisible(false);
	m_pClassHelp->SetVisible(false);
	m_pMapBriefing->SetVisible(false);
	m_pSpectatorGUI->SetVisible(false);
	m_pCommandMenu->SetVisible(false);
		
	m_PendingDialogs.Purge(); // clear the dialog queue
}


//-----------------------------------------------------------------------------
// Purpose: returns the current map, tries to determine it if the string is empty
/*-----------------------------------------------------------------------------
const char *TeamFortressViewport::GetMapName()
{
	if( strlen(m_sMapName) <= 0 )
	{
		m_pClientDllInterface->COM_FileBase( const_cast<char *>(m_pClientDllInterface->GetLevelName()), m_sMapName );
	}
	return m_sMapName; 
}*/


//-----------------------------------------------------------------------------
// Purpose: called on each level change, resets the map name and hides all vgui menus
//-----------------------------------------------------------------------------
void TeamFortressViewport::OnLevelChange(const char * mapname)
{
	m_flScoreBoardLastUpdated = 0;
	
	HideAllVGUIMenu();

	m_pClientScoreBoard->Reset();

	if ( m_pSpectatorGUI )
	{
		m_pSpectatorGUI->SetVisible(false);
	}
	
	if ( m_pClientScoreBoard )
	{
		m_pClientScoreBoard->SetVisible(false);
	}

	if ( m_pMapOverview )
	{
		m_pMapOverview->SetMap( mapname );
	}

}

//-----------------------------------------------------------------------------
// Purpose: handle showing and hiding of the command menu
//-----------------------------------------------------------------------------
void TeamFortressViewport::ShowCommandMenu()
{
	// m_pCommandPanel->ShowCommandPanel();

	if ( m_pCommandMenu->IsVisible() )
		return;

	// check, if we must update command menu before showing it

	bool bNeedUpdate = false;

	// has our team number changed?
	if ( m_iCurrentTeamNumber != gViewPortInterface->GetClientDllInterface()->TeamNumber() )
	{
		m_iCurrentTeamNumber = gViewPortInterface->GetClientDllInterface()->TeamNumber();
		bNeedUpdate = true;
	}

	// has the map changed?
	const char *pszLevel = gViewPortInterface->GetClientDllInterface()->GetLevelName();
	
	if ( pszLevel )
	{
		// Does it match the current map name?
		if ( strcmp( pszLevel, m_szCurrentMap ) != 0 )
		{
			strncpy( m_szCurrentMap, pszLevel, 256 - 1 );
			m_szCurrentMap[256 - 1] = '\0';
			bNeedUpdate = true;
		}
	}

	if ( bNeedUpdate )
	{
		UpdateCommandMenu();
	}

	m_pCommandMenu->SetVisible( true );
}

void TeamFortressViewport::UpdateCommandMenu()
{
	int iWide, iTall, screenW, screenH;

	m_pCommandMenu->RebuildMenu();
	
	// check screen position

	m_pCommandMenu->GetSize( iWide, iTall);
	surface()->GetScreenSize( screenW, screenH );

	// center it vertically on the left-hand side of the screen
	int y =  ( screenH - iTall ) / 2;
	if ( y < 0 ) // make sure the beginning of the menu is on the screen
	{
		y = 0;
	}

	m_pCommandMenu->SetPos( 0, y );
}

void TeamFortressViewport::HideCommandMenu()
{
	// m_pCommandPanel->HideCommandPanel();
	m_pCommandMenu->SetVisible( false );

}

int TeamFortressViewport::IsCommandMenuVisible()
{
	return m_pCommandMenu->IsVisible();
}

//================================================================
// Number Key Input
bool TeamFortressViewport::SlotInput( int iSlot )
{
	// If the command menu is up, give it the input
	return false; // m_pCommandPanel->SlotInput( iSlot ); TODO ?
}

void TeamFortressViewport::ChatInputPosition(int *x, int *y )
{
	if ( m_pClientDllInterface->GetSpectatorMode() != 0 || m_pClientDllInterface->IsHLTVMode() )
	{
		if ( m_pClientDllInterface->PipInsetOff() )
		{
			*y = m_pSpectatorGUI->GetTopBarHeight() + 5;
		}
		else
		{
			// int tmpX, tmpY, dummy;

			// m_pClientDllInterface->InsetValues( tmpX, tmpY, dummy, dummy);
			// *y =  tmpY + 5 ;
			*y = 0;
			*x = 0;

		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: handle showing and hiding of the command menu
//-----------------------------------------------------------------------------
VGuiLibraryInterface_t *TeamFortressViewport::GetClientDllInterface()
{
	return m_pClientDllInterface;
}

void TeamFortressViewport::SetClientDllInterface(VGuiLibraryInterface_t *clientInterface)
{
	m_pClientDllInterface = clientInterface;
	Assert( m_pClientDllInterface );
}
