//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "CstrikeClientScoreBoard.h"


#include <KeyValues.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVgui.h>
#include <vgui_controls/SectionedListPanel.h>

#include "voice_status.h"
#include "Friends/IFriendsUser.h"
#include <gameeventdefs.h>
//#include "player_info.h"

#include <game_controls/IViewPort.h>

using namespace vgui;

// id's of sections used in the scoreboard
enum EScoreboardSections
{
	SCORESECTION_TERRORIST = 1,
	SCORESECTION_CT = 2,
	SCORESECTION_SPECTATOR = 3,
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCSClientScoreBoardDialog::CCSClientScoreBoardDialog(vgui::Panel *parent) : CClientScoreBoardDialog(parent)
{
	m_iNumTeams = 2;
	gameeventmanager->AddListener(this, GAME_EVENT_PLAYER_DEATH );	
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCSClientScoreBoardDialog::~CCSClientScoreBoardDialog()
{
}

void CCSClientScoreBoardDialog::FireGameEvent( KeyValues * event)
{
	// the event should be GAME_EVENT_PLAYER_DEATH, it's
	// the only one we registered for

	int victim = event->GetInt( "victim" );
	
	if ( victim ) 
	{
		// TODO
		// g_PlayerExtraInfo[ victim ].dead = true;
		// g_PlayerExtraInfo[ victim ].has_c4 = 0;
	}
}

void CCSClientScoreBoardDialog::GameEventsUpdated()
{

}


//-----------------------------------------------------------------------------
// Purpose: sets up base sections
//-----------------------------------------------------------------------------
void CCSClientScoreBoardDialog::InitScoreboardSections()
{
	// fill out the structure of the scoreboard
	AddHeader();

	// add the team sections
	AddSection( TEAM_YES, SCORESECTION_TERRORIST );
	AddSection( TEAM_YES, SCORESECTION_CT );
	AddSection( TEAM_SPECTATORS, SCORESECTION_SPECTATOR );
}

//-----------------------------------------------------------------------------
// Purpose: resets the scoreboard team info
//-----------------------------------------------------------------------------
void CCSClientScoreBoardDialog::UpdateTeamInfo()
{
	// clear out team scores
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		if ( !m_TeamInfo[i].scores_overriden )
		{
			m_TeamInfo[i].frags = m_TeamInfo[i].deaths = 0;
		}

		m_TeamInfo[i].players = m_TeamInfo[i].ping = m_TeamInfo[i].packetloss = 0;
	}

	// recalc the team scores, then draw them
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		VGuiLibraryPlayer_t playerInfo = gViewPortInterface->GetClientDllInterface()->GetPlayerInfo( i );

		if ( playerInfo.name == NULL )
			continue; // empty player slot, skip

		if ( playerInfo.teamnumber == TEAM_UNASSIGNED || playerInfo.teamnumber == TEAM_SPECTATOR )
			continue; // skip over players who are not in a team

		struct ScoreBoardTeamInfo &teamInfo = m_TeamInfo[playerInfo.teamnumber];
		teamInfo.ping += playerInfo.ping;
		teamInfo.packetloss += playerInfo.packetloss;

		if ( playerInfo.thisplayer )
			teamInfo.ownteam = true;
		else
			teamInfo.ownteam = false;

		// Set the team's number (used for team colors)
		teamInfo.teamnumber = playerInfo.teamnumber;
		teamInfo.players++;
	}

	// find team ping/packetloss averages
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		m_TeamInfo[i].already_drawn = false;

		if ( m_TeamInfo[i].players > 0 )
		{
			m_TeamInfo[i].ping /= m_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
			m_TeamInfo[i].packetloss /= m_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
		}
	}

	// update the team sections in the scoreboard
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		wchar_t *teamName = NULL;;
		int sectionID = 0;
		if ( !stricmp( m_TeamInfo[i].name, "CT" ) )
		{
			sectionID = SCORESECTION_CT;
			teamName = localize()->Find("#Cstrike_ScoreBoard_CT");
		}
		else if ( !stricmp( m_TeamInfo[i].name, "TERRORIST" ) )
		{
			sectionID = SCORESECTION_TERRORIST;
			teamName = localize()->Find("#Cstrike_ScoreBoard_Ter");
		}
		else
		{
			sectionID = SCORESECTION_SPECTATOR;
		}

		// update team name
		wchar_t name[64];
		wchar_t string1[1024];
		wchar_t wNumPlayers[6];
		_snwprintf(wNumPlayers, 6, L"%i", m_TeamInfo[i].players);
		if (!teamName)
		{
			localize()->ConvertANSIToUnicode(m_TeamInfo[i].name, name, sizeof(name));
			teamName = name;
		}
		if (m_TeamInfo[i].players == 1)
		{
			localize()->ConstructString( string1, sizeof(string1), localize()->Find("#Cstrike_ScoreBoard_Player"), 2, teamName, wNumPlayers );
		}
		else
		{
			localize()->ConstructString( string1, sizeof(string1), localize()->Find("#Cstrike_ScoreBoard_Players"), 2, teamName, wNumPlayers );
		}
		m_pPlayerList->ModifyColumn(sectionID, "name", string1);

		// update stats
		wchar_t val[6];
		swprintf(val, L"%d", m_TeamInfo[i].frags);
		m_pPlayerList->ModifyColumn(sectionID, "frags", val);
		if (m_TeamInfo[i].ping < 1)
		{
			m_pPlayerList->ModifyColumn(sectionID, "ping", L"");
		}
		else
		{
			swprintf(val, L"%d", m_TeamInfo[i].ping);
			m_pPlayerList->ModifyColumn(sectionID, "ping", val);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CCSClientScoreBoardDialog::AddHeader()
{
	// add the top header
	m_pPlayerList->AddSection(0, "");
	m_pPlayerList->SetSectionAlwaysVisible(0);
	m_pPlayerList->AddColumnToSection(0, "name", "", 0, scheme()->GetProportionalScaledValue( CSTRIKE_NAME_WIDTH ) );
	m_pPlayerList->AddColumnToSection(0, "class", "", 0, scheme()->GetProportionalScaledValue( CSTRIKE_CLASS_WIDTH ) );
	m_pPlayerList->AddColumnToSection(0, "frags", "#PlayerScore", 0 | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue( CSTRIKE_SCORE_WIDTH ) );
	m_pPlayerList->AddColumnToSection(0, "deaths", "#PlayerDeath", 0 | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue( CSTRIKE_DEATH_WIDTH ) );
	m_pPlayerList->AddColumnToSection(0, "ping", "#PlayerPing", 0 | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue( CSTRIKE_PING_WIDTH ) );
//	m_pPlayerList->AddColumnToSection(0, "voice", "#PlayerVoice", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::HEADER_TEXT| SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue( CSTRIKE_VOICE_WIDTH ) );
//	m_pPlayerList->AddColumnToSection(0, "tracker", "#PlayerTracker", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::HEADER_TEXT, scheme()->GetProportionalScaledValue( CSTRIKE_FRIENDS_WIDTH ) );
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
void CCSClientScoreBoardDialog::AddSection(int teamType, int teamNumber)
{
	int sectionID = 0;
	if ( teamType == TEAM_YES )
	{
 		if (teamNumber == 1)
  		{
  			sectionID = SCORESECTION_TERRORIST;
  		}
  		else if (teamNumber == 2)
  		{
  			sectionID = SCORESECTION_CT;
  		}
 
		m_pPlayerList->AddSection(sectionID, "", StaticPlayerSortFunc);

		// setup the columns
		m_pPlayerList->AddColumnToSection(sectionID, "name", "", 0, scheme()->GetProportionalScaledValue( CSTRIKE_NAME_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "class", "" , 0, scheme()->GetProportionalScaledValue( CSTRIKE_CLASS_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "frags", "", SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue( CSTRIKE_SCORE_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "deaths", "", SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue( CSTRIKE_DEATH_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "ping", "", SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue( CSTRIKE_PING_WIDTH ) );

		// set the section to have the team color
		if (teamNumber)
		{
			m_pPlayerList->SetSectionFgColor(sectionID, GetTeamColor(teamNumber));
		}
	}
	else if (teamType == TEAM_SPECTATORS)
	{
		sectionID = SCORESECTION_SPECTATOR;
		m_pPlayerList->AddSection(sectionID, "");
		m_pPlayerList->AddColumnToSection(sectionID, "name", "#Spectators", 0, scheme()->GetProportionalScaledValue( CSTRIKE_NAME_WIDTH ));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CCSClientScoreBoardDialog::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
{
	//MIKETODO: scoreboard
	/*
	if (g_PlayerExtraInfo[ playerIndex ].teamnumber)
	{
		kv->SetInt("deaths", g_PlayerExtraInfo[ playerIndex ].deaths);
		kv->SetInt("frags", g_PlayerExtraInfo[ playerIndex ].frags);
		if (g_PlayerInfoList[ playerIndex ].ping < 1)
		{
			const char *isBotString = gEngfuncs.PlayerInfo_ValueForKey(playerIndex, "*bot");
			if (isBotString && atoi(isBotString) > 0)
			{
				kv->SetString("ping", "BOT");
			}
			else
			{
				kv->SetString("ping", "");
			}
		}
		else
		{
			kv->SetInt("ping", g_PlayerInfoList[ playerIndex ].ping);
		}
	}

	kv->SetString("name", g_PlayerInfoList[ playerIndex ].name );

	if ( g_PlayerExtraInfo[ playerIndex ].dead == true )
	{
		// don't show death status to living enemy players
#ifdef DONT_SHOW_DEATH_STATUS_TO_LIVING_PLAYERS
		if ( C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() == TEAM_UNASSIGNED || // we're a spectator
			 gEngfuncs.IsSpectateOnly() || // this is HLTV
			 g_PlayerExtraInfo[ gEngfuncs.GetLocalPlayer()->index ].dead == true || // we're dead
			 g_PlayerExtraInfo[ playerIndex ].teamnumber == C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() ) // we're on the same team
#endif
		{
			kv->SetString("class", "#Cstrike_DEAD");
		}
	}
	else if ( g_PlayerExtraInfo[ playerIndex ].has_c4 && 
		( ( C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() == TEAM_TERRORIST || 
		C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() == TEAM_UNASSIGNED ) || 
		g_PlayerExtraInfo[ gEngfuncs.GetLocalPlayer()->index ].dead == true )  ) // g_iTeamNumber is 0 for spectators
	{
	//	pLabel->setFgColor(50,200,50,0);	//	Green
		kv->SetString("class", "#Cstrike_BOMB");

	}
	else if ( g_PlayerExtraInfo[ playerIndex ].vip && 
		( ( C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() == TEAM_CT || 
		C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() == TEAM_UNASSIGNED ) || 
		g_PlayerExtraInfo[ gEngfuncs.GetLocalPlayer()->index ].dead == true ) ) // g_iTeamNumber is 0 for spectators
	{
		//pLabel->setFgColor(255,205,45,0);	//	Yellow
		kv->SetString("class", "#Cstrike_VIP");
	}
	else
	{
		kv->SetString("class", "");
	}

	kv->SetInt("playerIndex", playerIndex);
	*/
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCSClientScoreBoardDialog::UpdatePlayerInfo()
{
	m_iSectionId = 0; // 0'th row is a header
	int selectedRow = -1;

	// walk all the players and make sure they're in the scoreboard
	for ( int i = 1; i < gViewPortInterface->GetClientDllInterface()->GetMaxPlayers(); i++ )
	{
		VGuiLibraryPlayer_t playerInfo = gViewPortInterface->GetClientDllInterface()->GetPlayerInfo( i );

		if ( playerInfo.name )
		{
			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
			GetPlayerScoreInfo( i, playerData );
			int itemID = FindItemIDForPlayerIndex( i );
  			int sectionID = playerInfo.teamnumber;
			
			//!! hack, puts people not in a team into spectators
			if (playerInfo.teamnumber == 0)
			{
				sectionID = 3;
			}
			if (playerInfo.thisplayer)
			{
				selectedRow = itemID;
			}
			if (itemID == -1)
			{
				// add a new row
				itemID = m_pPlayerList->AddItem( sectionID, playerData );
			}
			else
			{
				// modify the current row
				m_pPlayerList->ModifyItem( itemID, sectionID, playerData );
			}

			// set the row color based on the players team
			m_pPlayerList->SetItemFgColor( itemID, GetTeamColor(playerInfo.teamnumber) );

			playerData->deleteThis();
		}
		else
		{
			// remove the player
			int itemID = FindItemIDForPlayerIndex( i );
			if (itemID != -1)
			{
				m_pPlayerList->RemoveItem(itemID);
			}
		}
	}

	if ( selectedRow != -1 )
	{
		m_pPlayerList->SetSelectedItem(selectedRow);
	}
}
