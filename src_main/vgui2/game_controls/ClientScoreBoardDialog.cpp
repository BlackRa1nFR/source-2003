//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


#include <stdio.h>
#include <game_controls/ClientScoreBoardDialog.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vstdlib/IKeyValuesSystem.h>

#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/SectionedListPanel.h>

#include <game_controls/IViewPort.h>

//#include "voice_status.h"
//#include "Friends/IFriendsUser.h"

// prototypes
void HudMessage(char *str);

// extern vars
//extern IFriendsUser *g_pFriendsUser;

// basic team colors
#define COLOR_RED		Color(255, 64, 64, 255)
#define COLOR_BLUE		Color(153, 204, 255, 255)
#define COLOR_YELLOW	Color(255, 178, 0, 255)
#define COLOR_GREEN		Color(153, 255, 153, 255)
#define COLOR_GREY		Color(204, 204, 204, 255)

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClientScoreBoardDialog::CClientScoreBoardDialog(vgui::Panel *parent) : Frame(parent, "ClientScoreBoard")
{
	m_iNumTeams = 0;
	m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString("playerIndex");

	//memset(s_VoiceImage, 0x0, sizeof( s_VoiceImage ));
	memset( m_TeamInfo, 0x0, sizeof( m_TeamInfo ));
	memset( &m_BlankTeamInfo, 0x0, sizeof( m_BlankTeamInfo ));
	TrackerImage = 0;

	// initialize dialog
	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );

	// set the scheme before any child control is created
	SetScheme("ClientScheme");

	m_pPlayerList = new SectionedListPanel(this, "PlayerList");
	m_pPlayerList->SetVerticalScrollbar(false);

	LoadControlSettings("Resource/UI/ScoreBoard.res");
	m_iDesiredHeight = GetTall();
	m_pPlayerList->SetVisible( false ); // hide this until we load the images in applyschemesettings
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CClientScoreBoardDialog::~CClientScoreBoardDialog()
{

}

//-----------------------------------------------------------------------------
// Purpose: clears everything in the scoreboard and all it's state
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::Reset()
{
	// clear
	m_pPlayerList->DeleteAllItems();
	m_pPlayerList->RemoveAllSections();

	m_iSectionId = 0;
	// add all the sections
	InitScoreboardSections();
}

//-----------------------------------------------------------------------------
// Purpose: adds all the team sections to the scoreboard
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::InitScoreboardSections()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets up screen
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	ImageList *imageList = new ImageList(false);
//	s_VoiceImage[0] = 0;	// index 0 is always blank
//	s_VoiceImage[CVoiceStatus::VOICE_NEVERSPOKEN] = imageList->AddImage(scheme()->GetImage("gfx/vgui/640_speaker1", true));
//	s_VoiceImage[CVoiceStatus::VOICE_NOTTALKING] = imageList->AddImage(scheme()->GetImage("gfx/vgui/640_speaker2", true));
//	s_VoiceImage[CVoiceStatus::VOICE_TALKING] = imageList->AddImage(scheme()->GetImage( "gfx/vgui/640_speaker3", true));
//	s_VoiceImage[CVoiceStatus::VOICE_BANNED] = imageList->AddImage(scheme()->GetImage("gfx/vgui/640_voiceblocked", true));
	
	TrackerImage = imageList->AddImage(scheme()->GetImage("gfx/vgui/640_scoreboardtracker", true));

	// resize the images to our resolution
	for (int i = 0; i < imageList->GetImageCount(); i++ )
	{
		int wide, tall;
		imageList->GetImage(i)->GetSize(wide, tall);
		imageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValue(wide), scheme()->GetProportionalScaledValue(tall));
	}

	m_pPlayerList->SetImageList(imageList, false);
	m_pPlayerList->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::Activate( bool spectatorUIVisible )
{
	BaseClass::Activate();
	SetVisible(true);

	// setups whether to hide or show the frame buttons
	SetMenuButtonVisible( spectatorUIVisible );
	SetMinimizeButtonVisible( spectatorUIVisible );
	SetMaximizeButtonVisible( spectatorUIVisible );
	SetCloseButtonVisible( spectatorUIVisible );
	SetKeyBoardInputEnabled(spectatorUIVisible);
	SetMouseInputEnabled(spectatorUIVisible);
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::Update(const char *servername, bool teamplay, bool spectator)
{
	// Set the title
	if (servername)
	{
		SetControlString("ServerName", servername);
		MoveLabelToFront("ServerName");
	}

	if (!IsVisible())
		return;

	FillScoreBoard();

	// grow the scoreboard to fit all the players
	int wide, tall;
	m_pPlayerList->GetContentSize(wide, tall);
	if (m_iDesiredHeight < tall)
	{
		SetSize(GetWide(), tall);
	}
	else
	{
		SetSize(GetWide(), m_iDesiredHeight);
	}
	MoveToCenterOfScreen();
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdateTeamInfo()
{
	// clear out team scores
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		if ( !m_TeamInfo[i].scores_overriden )
		{
			m_TeamInfo[i].frags = m_TeamInfo[i].deaths = 0;
		}
		m_TeamInfo[i].ping = m_TeamInfo[i].packetloss = 0;
	}

	// recalc the team scores, then draw them
	for ( i = 1; i < gViewPortInterface->GetClientDllInterface()->GetMaxPlayers(); i++ )
	{
		VGuiLibraryPlayer_t playerInfo = gViewPortInterface->GetClientDllInterface()->GetPlayerInfo( i );

		if ( playerInfo.name == NULL )
			continue; // empty player slot, skip

		if ( !playerInfo.teamname )
			continue; // skip over players who are not in a team

		// find what team this player is in
		for ( int j = 1; j <= m_iNumTeams; j++ )
		{
			if ( !stricmp( playerInfo.teamname, m_TeamInfo[j].name ) )
				break;
		}
		if ( j > m_iNumTeams )  // player is not in a team, skip to the next guy
			continue;

		if ( !m_TeamInfo[j].scores_overriden )
		{
			m_TeamInfo[j].frags += playerInfo.frags;
			m_TeamInfo[j].deaths += playerInfo.deaths;
		}

		m_TeamInfo[j].ping += playerInfo.ping;
		m_TeamInfo[j].packetloss += playerInfo.packetloss;

		if ( playerInfo.thisplayer )
			m_TeamInfo[j].ownteam = true;
		else
			m_TeamInfo[j].ownteam = false;

		// Set the team's number (used for team colors)
		m_TeamInfo[j].teamnumber = playerInfo.teamnumber;
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

	// Draw the teams
	while ( 1 )
	{
		int highest_frags = -99999; int lowest_deaths = 99999;
		int best_team = 0;

		for ( i = 1; i <= m_iNumTeams; i++ )
		{
			if ( m_TeamInfo[i].players < 1 )
				continue;

			if ( !m_TeamInfo[i].already_drawn && m_TeamInfo[i].frags >= highest_frags )
			{
				if ( m_TeamInfo[i].frags > highest_frags || m_TeamInfo[i].deaths < lowest_deaths )
				{
					best_team = i;
					lowest_deaths = m_TeamInfo[i].deaths;
					highest_frags = m_TeamInfo[i].frags;
				}
			}
		}

		// draw the best team on the scoreboard
		if ( !best_team )
			break;

		// Put this team in the sorted list
		m_TeamInfo[best_team].already_drawn = true;  // set the already_drawn to be true, so this team won't get sorted again
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdatePlayerInfo()
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::RebuildTeams(const char *servername, bool teamplay, bool spectator)
{
	// nothing to do here, team info recalculated when needed
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::AddHeader()
{
	// add the top header
	m_pPlayerList->AddSection(m_iSectionId, "");
	m_pPlayerList->SetSectionAlwaysVisible(m_iSectionId);
	m_pPlayerList->AddColumnToSection(m_iSectionId, "name", "#PlayerName", 0, scheme()->GetProportionalScaledValue(NAME_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "frags", "#PlayerScore", 0, scheme()->GetProportionalScaledValue(SCORE_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "deaths", "#PlayerDeath", 0, scheme()->GetProportionalScaledValue(DEATH_WIDTH) );
	m_pPlayerList->AddColumnToSection(m_iSectionId, "ping", "#PlayerPing", 0, scheme()->GetProportionalScaledValue(PING_WIDTH) );
//	m_pPlayerList->AddColumnToSection(m_iSectionId, "voice", "#PlayerVoice", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
//	m_pPlayerList->AddColumnToSection(m_iSectionId, "tracker", "#PlayerTracker", SectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValue(FRIENDS_WIDTH) );
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::AddSection(int teamType, VGuiLibraryTeamInfo_t *team_info)
{
	if (teamType == TEAM_YES)
	{
		KeyValues *kv = new KeyValues("data");

		// setup the team name
		wchar_t *teamName = localize()->Find(team_info->name);
		wchar_t name[64];
		wchar_t string1[1024];
		wchar_t wNumPlayers[6];
		_snwprintf(wNumPlayers, 6, L"%i", team_info->players);

		if (!teamName)
		{
			localize()->ConvertANSIToUnicode(team_info->name, name, sizeof(name));
			teamName = name;
		}
		if (team_info->players == 1)
		{
			localize()->ConstructString( string1, sizeof( string1 ), localize()->Find("#Player"), 2, teamName, wNumPlayers );
		}
		else
		{
			localize()->ConstructString( string1, sizeof( string1 ), localize()->Find("#Players"), 2, teamName, wNumPlayers );
		}
		m_pPlayerList->AddSection(m_iSectionId, "", StaticPlayerSortFunc);
		m_pPlayerList->AddColumnToSection(m_iSectionId, "name", string1, 0, scheme()->GetProportionalScaledValue(NAME_WIDTH) );

		kv->SetInt("frags", team_info->frags);
		kv->SetInt("deaths", team_info->deaths);
		kv->SetInt("ping", team_info->ping);

		m_pPlayerList->AddColumnToSection(m_iSectionId, "frags", kv->GetString("frags"), 0, scheme()->GetProportionalScaledValue(SCORE_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "deaths", kv->GetString("deaths"), 0, scheme()->GetProportionalScaledValue(DEATH_WIDTH) );
		m_pPlayerList->AddColumnToSection(m_iSectionId, "ping", kv->GetString("ping"), 0, scheme()->GetProportionalScaledValue(PING_WIDTH) );

		kv->deleteThis();
	}
	else if ( teamType == TEAM_SPECTATORS )
	{
		m_pPlayerList->AddSection(m_iSectionId, "");
		m_pPlayerList->AddColumnToSection(m_iSectionId, "name", "#Spectators", 0, 100);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CClientScoreBoardDialog::StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2)
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	// first compare frags
	int v1 = it1->GetInt("frags");
	int v2 = it2->GetInt("frags");
	if (v1 > v2)
		return true;
	else if (v1 < v2)
		return false;

	// next compare deaths
	v1 = it1->GetInt("deaths");
	v2 = it2->GetInt("deaths");
	if (v1 > v2)
		return false;
	else if (v1 < v2)
		return true;

	// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
	return itemID1 < itemID2;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CClientScoreBoardDialog::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
{
	VGuiLibraryPlayer_t playerInfo = gViewPortInterface->GetClientDllInterface()->GetPlayerInfo( playerIndex );

	kv->SetInt("deaths", playerInfo.deaths);
	kv->SetInt("frags", playerInfo.frags);
	kv->SetInt("ping", playerInfo.ping) ;
	kv->SetString("name", playerInfo.name );
	kv->SetInt("playerIndex", playerIndex);

//	kv->SetInt("voice",	s_VoiceImage[GetClientVoice()->GetSpeakerStatus( playerIndex - 1) ]);	

/*	// setup the tracker column
	if (g_pFriendsUser)
	{
		unsigned int trackerID = gEngfuncs.GetTrackerIDForPlayer(row);

		if (g_pFriendsUser->IsBuddy(trackerID) && trackerID != g_pFriendsUser->GetFriendsID())
		{
			kv->SetInt("tracker",TrackerImage);
		}
	}
*/
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: gets team color
//-----------------------------------------------------------------------------
Color CClientScoreBoardDialog::GetTeamColor(int teamNumber)
{
	switch (teamNumber)
	{
	case 1:		return COLOR_RED;
	case 2:		return COLOR_BLUE;
	case 3:		return COLOR_YELLOW;
	case 4:		return COLOR_GREEN;
	}
	return COLOR_GREY;
}

//-----------------------------------------------------------------------------
// Purpose: reload the player list on the scoreboard
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::FillScoreBoard()
{
	// update totals information
	UpdateTeamInfo();

	// update player info
	UpdatePlayerInfo();
} 

//-----------------------------------------------------------------------------
// Purpose: called when a player dies, can be overriden
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::DeathMsg( int killer, int victim )
{
}

//-----------------------------------------------------------------------------
// Purpose: searches for the player in the scoreboard
//-----------------------------------------------------------------------------
int CClientScoreBoardDialog::FindItemIDForPlayerIndex(int playerIndex)
{
	for (int i = 0; i <= m_pPlayerList->GetHighestItemID(); i++)
	{
		if (m_pPlayerList->IsItemIDValid(i))
		{
			KeyValues *kv = m_pPlayerList->GetItemData(i);
			kv = kv->FindKey(m_iPlayerIndexSymbol);
			if (kv && kv->GetInt() == playerIndex)
				return i;
		}
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the name of this team
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::SetTeamName( int index, const char *name)
{
	if ( index < (sizeof( m_TeamInfo )/ sizeof(m_TeamInfo[0])) )
	{
		if ( m_TeamInfo[index].name )
		{
			free( (void *)m_TeamInfo[index].name );
		}
		m_TeamInfo[index].name = (const char *)malloc( strlen(name) +1 );
		Assert( m_TeamInfo[index].name );
		if ( m_TeamInfo[index].name )
		{
			strcpy( (char *)m_TeamInfo[index].name, name );
			//((char *)m_TeamInfo[index].name)[ strlen(name) ] = '\0';
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the team name of this team
//-----------------------------------------------------------------------------
const char *CClientScoreBoardDialog::GetTeamName( int index )
{
	if ( index < (sizeof( m_TeamInfo )/ sizeof(m_TeamInfo[0])) )
	{
		return m_TeamInfo[index].name;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sets frags and deaths of a team
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::SetTeamDetails( int index, int frags, int deaths)
{
	if ( index < (sizeof( m_TeamInfo )/ sizeof(m_TeamInfo[0])) )
	{
		m_TeamInfo[index].scores_overriden = true;
		m_TeamInfo[index].frags = frags;
		m_TeamInfo[index].deaths = deaths;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the team info struct about a team
//-----------------------------------------------------------------------------
VGuiLibraryTeamInfo_t CClientScoreBoardDialog::GetPlayerTeamInfo( int playerIndex )
{
	if ( playerIndex < (sizeof( m_TeamInfo )/ sizeof(m_TeamInfo[0]))  )
	{
		return m_TeamInfo[playerIndex];
	}
	return m_BlankTeamInfo;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::MoveLabelToFront(const char *textEntryName)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->MoveToFront();
	}
}

