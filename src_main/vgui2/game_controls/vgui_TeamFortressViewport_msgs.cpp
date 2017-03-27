//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Client DLL VGUI2 Msgs for viewport, ENGINE DEPENDANT!!!!
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#pragma warning( disable : 4100  )  // unreferenced formal parameter
#pragma warning( disable : 4305  )  // truncation from 'const double' to 'float'

#include <stdio.h>

// VGUI panel includes
#include <vgui_controls/Panel.h>
#include <vgui_controls/Menu.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui/Cursor.h>
#include <vgui/IScheme.h>
#include <vgui/IVGUI.h>
#include <vgui/ILocalize.h>
#include <vgui/VGUI.h>

#include "../../cl_dll/parsemsg.h" // BEGIN_READ, 
#include "../../cl_dll/hud.h"

// sub dialogs
#include <game_controls/ClientScoreBoardDialog.h>
#include <game_controls/ClientMOTD.h>
#include <game_controls/SpectatorGUI.h>
#include <game_controls/TeamMenu.h>
#include <game_controls/ClassMenu.h>
#include <game_controls/VGUITextWindow.h>
#include "IGameUIFuncs.h"
// our definition
#include <game_controls/vgui_TeamFortressViewport.h>


void DuckMessage(const char *str)
{
	/* 
	// FIXME!!!! HACK HACK!!! AHHH!!!
	static 	client_textmessage_t tst; 
	static 	char tempString[128];

	strncpy(tempString,str,128);

	client_textmessage_t *tempMessage = NULL;
	tempMessage = &tst;

	if(tempMessage)
	{
		tempMessage->effect   = 2;

		tempMessage->r1       = 40;
		tempMessage->g1       = 255;
		tempMessage->b1       = 40;
		tempMessage->a1       = 200;

		tempMessage->r2       = 0;
		tempMessage->g2       = 255;
		tempMessage->b2       = 0;
		tempMessage->a2       = 200;

		tempMessage->x        = -1;		// Centered
		tempMessage->y        = 0.7;

		tempMessage->fadein   = 0.01;
		tempMessage->fadeout  = 0.7;
		tempMessage->fxtime   = 0.07;
		tempMessage->holdtime = 6;

		tempMessage->pMessage = tempString;
		tempMessage->pName = "Spec_Duck";

		gViewPortInterface->GetClientDllInterface()->MessageAdd( tempMessage );
	}
	*/
}

//================================================================
// Message Handlers
int TeamFortressViewport::MsgFunc_ValClass(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	for (int i = 0; i < 5; i++)
		m_iValidClasses[i] = READ_SHORT();

	m_pClassMenu->Update(m_iValidClasses, 5);
	return 1;
}

int TeamFortressViewport::MsgFunc_TeamNames(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	m_iNumTeams = READ_BYTE();

	for (int i = 0; i < m_iNumTeams; i++)
	{
		int teamNum = i; // + 1;
		const char *newName = READ_STRING();

		if ( !m_sTeamNames[teamNum] || strlen(m_sTeamNames[teamNum]) < (strlen(newName) + 1) )
		{
			if ( m_sTeamNames[teamNum] )
			{
				free( m_sTeamNames[teamNum] );
			}

			m_sTeamNames[teamNum] = (char *)malloc( strlen(newName) + 1 );
			Assert( m_sTeamNames[teamNum] );
			memset( m_sTeamNames[teamNum], 0x0, strlen(newName) + 1 );
		}


		strcpy( m_sTeamNames[teamNum], newName);
		m_pClientScoreBoard->SetTeamName( i + 1, m_sTeamNames[teamNum] );
	}

	// now add on the two special teams
	if ( !m_sTeamNames[m_iNumTeams] )
	{
		m_sTeamNames[m_iNumTeams] = (char *)malloc( strlen("#Team_AutoAssign") + 1 );
		Assert( m_sTeamNames[m_iNumTeams] );
	}
	strcpy(  m_sTeamNames[ m_iNumTeams ], "#Team_AutoAssign" );
	if ( !m_sTeamNames[m_iNumTeams + 1] )
	{
		m_sTeamNames[m_iNumTeams + 1] = (char *)malloc( strlen("#Menu_Spectate") + 1 );
		Assert( m_sTeamNames[m_iNumTeams + 1] );
	}
	strcpy(  m_sTeamNames[ m_iNumTeams + 1], "#Menu_Spectate" );


	// Update the Team Menu
	if (m_pTeamMenu)
		m_pTeamMenu->Update( m_pClientDllInterface->GetLevelName(), m_pClientDllInterface->IsSpectatingAllowed(), (const char **)m_sTeamNames, m_iNumTeams );

	return 1;
}

int TeamFortressViewport::MsgFunc_Feign(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	m_bIsFeigning = static_cast<bool>( READ_BYTE() );

	return 1;
}

int TeamFortressViewport::MsgFunc_Detpack(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_iIsSettingDetpack = READ_BYTE();

	return 1;
}

int TeamFortressViewport::MsgFunc_VGUIMenu(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int iMenu = READ_BYTE();

	// Bring up the menu
	ShowVGUIMenu( iMenu );

	return 1;
}

int TeamFortressViewport::MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf )
{
	if (m_iGotAllMOTD)
		m_szMOTD[0] = 0;

	BEGIN_READ( pbuf, iSize );

	m_iGotAllMOTD = READ_BYTE();

	int roomInArray = sizeof(m_szMOTD) - strlen(m_szMOTD) - 1;

	strncat( m_szMOTD, READ_STRING(), roomInArray >= 0 ? roomInArray : 0 );
	m_szMOTD[ sizeof(m_szMOTD)-1 ] = '\0';

	// don't show MOTD for HLTV spectators
	if ( m_iGotAllMOTD && !m_pClientDllInterface->IsHLTVMode() )
	{
		ShowVGUIMenu( MENU_INTRO );
		ShowVGUIMenu( MENU_TEAM );
		// (the class menu is put up when the server says to)
	}

	return 1;
}

int TeamFortressViewport::MsgFunc_BuildSt( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_iBuildState = READ_SHORT();

	return 1;
}

int TeamFortressViewport::MsgFunc_RandomPC( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_iRandomPC = READ_BYTE();

	return 1;
}

int TeamFortressViewport::MsgFunc_ServerName( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	strncpy( m_szServerName, READ_STRING(), MAX_SERVERNAME_LENGTH );
	m_szServerName[ MAX_SERVERNAME_LENGTH -1 ] = '\0';

	return 1;
}

int TeamFortressViewport::MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf )
{
	/* MIKETODO: HUD state - use new locations for this data
	BEGIN_READ( pbuf, iSize );
	short cl = (short)READ_BYTE();
	short frags = (short)READ_SHORT();
	short deaths = (short)READ_SHORT();
	short playerclass = (short)READ_SHORT();
	short teamnumber = (short)READ_SHORT();

	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_PlayerExtraInfo[cl].frags = frags;
		g_PlayerExtraInfo[cl].deaths = deaths;
		g_PlayerExtraInfo[cl].playerclass = playerclass;
		g_PlayerExtraInfo[cl].teamnumber = teamnumber;

		//Dont go bellow 0!
		if ( g_PlayerExtraInfo[cl].teamnumber < 0 )
			 g_PlayerExtraInfo[cl].teamnumber = 0;

		if (m_pTeamMenu)
			m_pTeamMenu->Update( m_sMapName, m_iAllowSpectators, (const char **)m_sTeamNames, m_iNumTeams );

		GetAllPlayersInfo();
		m_pClientScoreBoard->Update(m_szServerName,m_pClientDllInterface->TeamPlay(),  m_pSpectatorGUI->IsVisible() );
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
int TeamFortressViewport::MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf )
{
	/* MIKETODO: HUD state - use new locations for this data
	BEGIN_READ( pbuf, iSize );
	const char *TeamName = READ_STRING();

	// find the team matching the name
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		if ( !strnicmp( TeamName, m_pClientScoreBoard->GetTeamName( i ), strlen(TeamName) ) )
			break;
	}

	if ( i >m_iNumTeams && i< MAX_TEAMS) // new team
	{
		m_iNumTeams++;
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
	m_pClientScoreBoard->SetTeamDetails( i, READ_SHORT(), READ_SHORT() );
	*/

	return 1;
}

// Message handler for TeamInfo message
// accepts two values:
//		byte: client number
//		string: client team name
int TeamFortressViewport::MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf )
{
	/* MIKETODO: HUD state - use new locations for this data
	BEGIN_READ( pbuf, iSize );
	short cl = (short)READ_BYTE();
	
	if ( cl > 0 && cl <= MAX_PLAYERS )
	{  
		// set the players team
		strncpy( g_PlayerExtraInfo[cl].teamname, READ_STRING(), MAX_TEAM_NAME );
	}

	GetAllPlayersInfo();
	m_pClientScoreBoard->RebuildTeams(m_szServerName, m_pClientDllInterface->TeamPlay(),  m_pSpectatorGUI->IsVisible());
	*/
	return 1;
}


int TeamFortressViewport::MsgFunc_Spectator( const char *pszName, int iSize, void *pbuf )
{
/*	BEGIN_READ( pbuf, iSize );

	short cl = (short)READ_BYTE();
	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_IsSpectator[cl] = READ_BYTE();
	}
*/
	return 1;
}

/*
int TeamFortressViewport::MsgFunc_AllowSpec( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_iAllowSpectators = READ_BYTE();

	// If the team menu is up, update it too
	if (m_pTeamMenu)
		m_pTeamMenu->Update( m_sMapName, m_iAllowSpectators, (const char **)m_sTeamNames, m_iNumTeams );

	return 1;
}*/

// used to reset the player's screen immediately
int TeamFortressViewport::MsgFunc_ResetFade( const char *pszName, int iSize, void *pbuf )
{
	return 1;
}

// used to fade a player's screen out/in when they're spectating someone who is teleported
int TeamFortressViewport::MsgFunc_SpecFade( const char *pszName, int iSize, void *pbuf )
{
	return 1;
}


