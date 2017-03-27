//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: VGUI scoreboard
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#if defined( TF2_CLIENT_DLL )
#include "c_basetfplayer.h"
#endif

#include "VGUI_BitmapImage.h"
#include "hud_macros.h"
#include "vgui_ScorePanel.h"
#include "vgui_helpers.h"
#include "vgui_int.h"
#include "c_playerresource.h"

#include "text_message.h"
#include "c_team.h"
#include "iinput.h"
#include "voice_status.h"
#include <vgui/Cursor.h>
#include <vgui/IInput.h>
#include "iviewrender.h"
//#include "ITrackerUser.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//extern ITrackerUser *g_pTrackerUser;

// Scoreboard positions
#define SBOARD_INDENT_X			XRES(104)
#define SBOARD_INDENT_Y			YRES(40)

#define YHEIGHT( x )			( x )

DECLARE_HUDELEMENT( ScorePanel );

DECLARE_HUD_COMMAND( ScorePanel, ShowScores );
DECLARE_HUD_COMMAND( ScorePanel, HideScores );

#define MAX_TEAM_NAME 16

struct extra_player_info_t 
{
	short frags;
	short deaths;
	short playerclass;
	short teamnumber;
	short ping;
	short packetloss;
	char teamname[MAX_TEAM_NAME];
};

struct team_info_t 
{
	char name[MAX_TEAM_NAME];
	short frags;
	short deaths;
	short ping;
	short packetloss;
	short ownteam;
	short players;
	int already_drawn;
	int scores_overriden;
	int teamnumber;
};

hud_player_info_t	 g_PlayerInfoList[MAX_PLAYERS+1];	   // player info from the engine
extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
team_info_t			 g_TeamInfo[MAX_TEAMS+1];
int					 g_IsSpectator[MAX_PLAYERS+1];

static void GetAllPlayersInfo( void )
{
	int i;
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( i > engine->GetMaxClients() )
		{
			memset( &g_PlayerInfoList[ i ], 0, sizeof( g_PlayerInfoList[i] ) );
			memset( &g_PlayerExtraInfo[ i ], 0, sizeof( g_PlayerExtraInfo[i] ) );
			continue;
		}

		engine->GetPlayerInfo( i, &g_PlayerInfoList[i] );

		if ( !g_PR )
			continue;

		if ( !g_PR->Get_Connected(i) )
			continue;

		extra_player_info_t *extra = &g_PlayerExtraInfo[ i ];
		C_Team *team = GetPlayersTeam(i);
		if ( !team )
			continue;

		extra->frags = g_PR->Get_Score( i );
		extra->deaths = g_PR->Get_Deaths( i );
#if defined( TF2_CLIENT_DLL )
		{
			C_BaseTFPlayer *tfplayer = static_cast< C_BaseTFPlayer * >( cl_entitylist->GetBaseEntity( i ) );
			if ( tfplayer )
			{
				extra->playerclass = tfplayer->GetClass();
			}
			else
			{
				extra->playerclass = 0;
			}
		}
#else
		extra->playerclass = 0;
#endif

		extra->teamnumber = team->GetTeamNumber();
		extra->ping = g_PR->Get_Ping( i );
		extra->packetloss = g_PR->Get_Packetloss( i );
		Q_snprintf( extra->teamname, sizeof( extra->teamname ), "%s", team->Get_Name() );
	}

	memset( g_IsSpectator, 0, sizeof( g_IsSpectator ) );
}

// Scoreboard dimensions
#define SBOARD_TITLE_SIZE_Y			YRES(22)

#define X_BORDER					XRES(4)

// Column sizes
class SBColumnInfo
{
public:
	char				*m_pTitle;		// If null, ignore, if starts with #, it's localized, otherwise use the string directly.
	int					m_Width;		// Based on 640 width. Scaled to fit other resolutions.
	Label::Alignment	m_Alignment;	
};

// grid size is marked out for 640x480 screen

SBColumnInfo g_ColumnInfo[NUM_COLUMNS] =
{
	{NULL,			24,			Label::a_east},		// tracker column
	{NULL,			140,		Label::a_east},		// name
	{NULL,			56,			Label::a_east},		// class
	{"#SCORE",		40,			Label::a_east},
	{"#DEATHS",		46,			Label::a_east},
	{"#LATENCY",	46,			Label::a_east},
	{"#VOICE",		40,			Label::a_east},
	{NULL,			2,			Label::a_east},		// blank column to take up the slack
};

#define TEAM_NO				0
#define TEAM_YES			1
#define TEAM_SPECTATORS		2
#define TEAM_BLANK			3

static ScorePanel *g_pScoreBoard = NULL;

static ScorePanel *GetScoreBoard( void )
{
	return g_pScoreBoard;
}

ScorePanel::~ScorePanel( void )
{
	delete m_pTrackerIcon;
	g_pScoreBoard = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Create the ScoreBoard panel
//-----------------------------------------------------------------------------
ScorePanel::ScorePanel( const char *pElementName ) : CHudElement( pElementName ),
	vgui::Panel( NULL, pElementName ),
	m_TitleLabel( this, "TitleLabel", "" ),
	m_PlayerList( this, "PlayerList" )
{
	m_iShowscoresHeld = false;

	SetVisible( false );

	SetKeyBoardInputEnabled( false ); 
	SetMouseInputEnabled( false );

	int x = SBOARD_INDENT_X;
	int y = SBOARD_INDENT_Y;
	int wide = ScreenWidth() - x * 2;
	int tall = ScreenHeight() - y * 2;

	SetBounds( x, y, wide, tall );
	SetBgColor(Color(0, 0, 0, 175));
	SetCursor( dc_arrow );
	SetZPos( 1000 );

	m_pCurrentHighlightLabel = NULL;
	m_iHighlightRow = -1;

	m_pTrackerIcon = new BitmapImage();
	//vgui_LoadTGANoInvertAlpha("gfx/vgui/640_scoreboardtracker.tga");

	// Initialize the top title.
	m_TitleLabel.SetText("");
	m_TitleLabel.SetPaintBackgroundEnabled( false );
	m_TitleLabel.SetContentAlignment( vgui::Label::a_west );
	int xpos = XRES( g_ColumnInfo[0].m_Width + 3 );
	m_TitleLabel.SetBounds(xpos + XRES( 10 ) , 4, wide - XRES( 20 ), SBOARD_TITLE_SIZE_Y);

	SetPaintBorderEnabled(true);

	m_pCloseButton = new Button( this, "Close", "x" );
	m_pCloseButton->SetPaintBackgroundEnabled( false );
	m_pCloseButton->SetFgColor( Color( 255, 255, 255, 255 ) );
	m_pCloseButton->AddActionSignalTarget( this );
	m_pCloseButton->SetBounds( wide-XRES(12 + 6), YRES(2), XRES( 12 ) , YRES( 12 ) );
	m_pCloseButton->SetCommand( "close" );

	memset( m_PlayerGrids, 0, sizeof( m_PlayerGrids ) );
	memset( m_PlayerEntries, 0, sizeof( m_PlayerEntries ) );
	InvalidateLayout( true, true );
}


//-----------------------------------------------------------------------------
// Scheme settings!
//-----------------------------------------------------------------------------
void ScorePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	HFont tfont = pScheme->GetFont( "Trebuchet24" );

	m_TitleLabel.SetFont(tfont);
	m_TitleLabel.SetFgColor( GetSchemeColor( "TitleText", pScheme ) );

	IBorder *border = pScheme->GetBorder( "ToolTipBorder" );
	SetBorder(border);

	HFont smallfont = pScheme->GetFont( "DefaultSmall" );

	for(int i=0; i < NUM_COLUMNS; i++)
	{
		m_HeaderLabels[i].SetFgColor( GetSchemeColor( "BaseText", pScheme ) );
		m_HeaderLabels[i].SetFont(smallfont);
	}

	m_hsfont = pScheme->GetFont( "Trebuchet18" );
	m_htfont = pScheme->GetFont( "Trebuchet24" );
	m_hsmallfont = pScheme->GetFont( "DefaultVerySmall" );

	SetBgColor(Color(0, 0, 0, 175));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void ScorePanel::OnCommand( const char *command )
{
	if ( !stricmp( command, "close" )  )
	{
		UserCmd_HideScores();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void ScorePanel::Initialize( void )
{
	// Clear out scoreboard data
	m_iLastKilledBy = 0;
	m_fLastKillTime = 0;
	m_iPlayerNum = 0;
	m_iNumTeams = 0;
	memset( g_PlayerExtraInfo, 0, sizeof g_PlayerExtraInfo );
	memset( g_TeamInfo, 0, sizeof g_TeamInfo );

	int x = SBOARD_INDENT_X;
	int y = SBOARD_INDENT_Y;
	int wide = ScreenWidth() - x * 2;
	int tall = ScreenHeight() - y * 2;

	// Setup the header (labels like "name", "class", etc..).
	m_HeaderGrid.SetParent(this);
	m_HeaderGrid.SetDimensions(NUM_COLUMNS, 1);
	m_HeaderGrid.SetSpacing(0, 0);
	m_HeaderGrid.SetName( "HeaderGrid" );
	
	// Redo scheme settings...
	InvalidateLayout( false, true );

	for(int i=0; i < NUM_COLUMNS; i++)
	{
		if (g_ColumnInfo[i].m_pTitle && g_ColumnInfo[i].m_pTitle[0] == '#')
		{
			m_HeaderLabels[i].SetText(hudtextmessage->BufferedLocaliseTextString(g_ColumnInfo[i].m_pTitle));
		}
		else if(g_ColumnInfo[i].m_pTitle)
		{
			m_HeaderLabels[i].SetText(g_ColumnInfo[i].m_pTitle);
		}

		int xwide = XRES( g_ColumnInfo[i].m_Width );
		
		m_HeaderGrid.SetColumnWidth(i, xwide);
		m_HeaderGrid.SetEntry(i, 0, &m_HeaderLabels[i]);

		m_HeaderLabels[i].SetPaintBackgroundEnabled( false );
		m_HeaderLabels[i].SetContentAlignment(g_ColumnInfo[i].m_Alignment);

		int yres = YHEIGHT( 12 );
		m_HeaderLabels[i].SetSize(50, yres);
	}

	// Set the width of the last column to be the remaining space.
	int ex, ey, ew, eh;
	m_HeaderGrid.GetEntryBox(NUM_COLUMNS - 2, 0, ex, ey, ew, eh);
	m_HeaderGrid.SetColumnWidth(NUM_COLUMNS - 1, (wide - X_BORDER) - (ex + ew));

	m_HeaderGrid.AutoSetRowHeights();
	m_HeaderGrid.SetBounds(X_BORDER, SBOARD_TITLE_SIZE_Y, wide - X_BORDER*2, m_HeaderGrid.GetRowHeight(0));
	m_HeaderGrid.SetPaintBackgroundEnabled( false );

	// Now setup the listbox with the actual player data in it.
	int headerX, headerY, headerWidth, headerHeight;
	m_HeaderGrid.GetBounds(headerX, headerY, headerWidth, headerHeight);
	m_PlayerList.SetBounds(headerX, headerY+headerHeight, headerWidth, tall - headerY - headerHeight - 6);
	m_PlayerList.SetPaintBackgroundEnabled( false );
	m_PlayerList.SetParent(this);
	m_PlayerList.SetFirstColumnWidth( 0 );
	m_PlayerList.SetPaintBorderEnabled( false );

	for(int row=0; row < NUM_ROWS; row++)
	{
		CGrid *pGridRow = m_PlayerGrids[row];
		if ( !pGridRow )
		{
			pGridRow = new CGrid();
			m_PlayerGrids[ row ] = pGridRow;
		}

		pGridRow->SetDimensions(NUM_COLUMNS, 1);
		
		for(int col=0; col < NUM_COLUMNS; col++)
		{
			CLabelHeader *entry = m_PlayerEntries[col][row];
			if ( !entry )
			{
				entry = new CLabelHeader();
				m_PlayerEntries[col][row] = entry;
			}

			entry->SetRow(row);
			entry->SetBgColor( Color( 0, 0, 0, 0 ) );
			pGridRow->SetEntry( col, 0, entry );
		}

		pGridRow->SetPaintBackgroundEnabled( false );
		pGridRow->SetBgColor( Color( 0,0,0,0 ) );
		pGridRow->SetSpacing(0, 0);
		pGridRow->CopyColumnWidths(&m_HeaderGrid);
		pGridRow->AutoSetRowHeights();
		pGridRow->SetSize(PanelWidth(pGridRow), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();

		Label *label = new Label( this, NULL, "" ); 
		label->SetPaintBackgroundEnabled( false );
		label->SetSize( 0, 0 );

		m_PlayerList.AddItem( label, pGridRow );
	}

	g_pScoreBoard = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ScorePanel::Init(void)
{
	vgui::VPANEL pParent = VGui_GetClientDLLRootPanel();
	SetParent( pParent );
	SetZPos( 1000 );

	Initialize();
}

void ScorePanel::SetVisible( bool state )
{
	BaseClass::SetVisible( state );

	if ( state )
	{
		Open();
	}
	else
	{
		Close();
	}
}

bool ScorePanel::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && m_iShowscoresHeld );
}

void ScorePanel::Paint( void )
{
	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void ScorePanel::OnThink()
{
	/*
	if ( IsVisible() )
	{
		vgui::input()->SetMouseFocus( GetVPanel() );
	}
	*/
	int i;

	// Set the title
	if ( engine->IsInGame() )
	{
		char sz[ 256 ];
		Q_snprintf(sz, sizeof( sz ), "%s", IGameSystem::MapName() );
		m_TitleLabel.SetText(sz);
	}

	m_iRows = 0;
	GetAllPlayersInfo();

	// Clear out sorts
	for (i = 0; i < NUM_ROWS; i++)
	{
		m_iSortedRows[i] = 0;
		m_iIsATeam[i] = TEAM_NO;
	}
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		m_bHasBeenSorted[i] = false;
	}

	// If it's not teamplay, sort all the players. Otherwise, sort the teams.
	if ( !gHUD.m_bTeamplay )
		SortPlayers( 0, NULL );
	else
		SortTeams();

	// set scrollbar range
	// m_PlayerList.SetScrollRange(m_iRows);

	FillGrid();

	/*
	if ( gViewPort->m_pSpectatorPanel->m_menuVisible )
	{
		 m_pCloseButton->SetVisible ( true );
	}
	else 
	{
		 m_pCloseButton->SetVisible ( false );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void ScorePanel::SortTeams()
{
	// clear out team scores
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		if ( !g_TeamInfo[i].scores_overriden )
			g_TeamInfo[i].frags = g_TeamInfo[i].deaths = 0;
		g_TeamInfo[i].ping = g_TeamInfo[i].packetloss = 0;
	}

	// recalc the team scores, then draw them
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( g_PlayerInfoList[i].name == NULL )
			continue; // empty player slot, skip

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

		// find what team this player is in
		for ( int j = 1; j <= m_iNumTeams; j++ )
		{
			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}
		if ( j > m_iNumTeams )  // player is not in a team, skip to the next guy
			continue;

		if ( !g_TeamInfo[j].scores_overriden )
		{
			g_TeamInfo[j].frags += g_PlayerExtraInfo[i].frags;
			g_TeamInfo[j].deaths += g_PlayerExtraInfo[i].deaths;
		}

		g_TeamInfo[j].ping += g_PlayerExtraInfo[i].ping;
		g_TeamInfo[j].packetloss += g_PlayerExtraInfo[i].packetloss;

		if ( g_PlayerInfoList[i].thisplayer )
			g_TeamInfo[j].ownteam = TRUE;
		else
			g_TeamInfo[j].ownteam = FALSE;

		// Set the team's number (used for team colors)
		g_TeamInfo[j].teamnumber = g_PlayerExtraInfo[i].teamnumber;
	}

	// find team ping/packetloss averages
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].already_drawn = FALSE;

		if ( g_TeamInfo[i].players > 0 )
		{
			g_TeamInfo[i].ping /= g_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
			g_TeamInfo[i].packetloss /= g_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
		}
	}

	// Draw the teams
	while ( 1 )
	{
		int highest_frags = -99999; int lowest_deaths = 99999;
		int best_team = 0;

		for ( i = 1; i <= m_iNumTeams; i++ )
		{
			if ( g_TeamInfo[i].players < 1 )
				continue;

			if ( !g_TeamInfo[i].already_drawn && g_TeamInfo[i].frags >= highest_frags )
			{
				if ( g_TeamInfo[i].frags > highest_frags || g_TeamInfo[i].deaths < lowest_deaths )
				{
					best_team = i;
					lowest_deaths = g_TeamInfo[i].deaths;
					highest_frags = g_TeamInfo[i].frags;
				}
			}
		}

		// draw the best team on the scoreboard
		if ( !best_team )
			break;

		// Put this team in the sorted list
		m_iSortedRows[ m_iRows ] = best_team;
		m_iIsATeam[ m_iRows ] = TEAM_YES;
		g_TeamInfo[best_team].already_drawn = TRUE;  // set the already_drawn to be TRUE, so this team won't get sorted again
		m_iRows++;

		// Now sort all the players on this team
		SortPlayers( 0, g_TeamInfo[best_team].name );
	}

	// Add all the players who aren't in a team yet into spectators
	SortPlayers( TEAM_SPECTATORS, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Sort a list of players
//-----------------------------------------------------------------------------
void ScorePanel::SortPlayers( int iTeam, char *team )
{
	bool bCreatedTeam = false;

	// draw the players, in order,  and restricted to team if set
	while ( 1 )
	{
		// Find the top ranking player
		int highest_frags = -99999;	int lowest_deaths = 99999;
		int best_player;
		best_player = 0;

		for ( int i = 1; i < MAX_PLAYERS; i++ )
		{
			if ( !m_bHasBeenSorted[i] && 
				g_PlayerInfoList[i].name && 
				g_PlayerExtraInfo[i].frags >= highest_frags )
			{
				if ( !team || !stricmp(g_PlayerExtraInfo[i].teamname, team) )  
				{
					extra_player_info_t *pl_info = &g_PlayerExtraInfo[i];
					if ( pl_info->frags > highest_frags || pl_info->deaths < lowest_deaths )
					{
						best_player = i;
						lowest_deaths = pl_info->deaths;
						highest_frags = pl_info->frags;
					}
				}
			}
		}

		if ( !best_player )
			break;

		// If we haven't created the Team yet, do it first
		if (!bCreatedTeam && iTeam)
		{
			m_iIsATeam[ m_iRows ] = iTeam;
			m_iRows++;

			bCreatedTeam = true;
		}

		// Put this player in the sorted list
		m_iSortedRows[ m_iRows ] = best_player;
		m_bHasBeenSorted[ best_player ] = true;
		m_iRows++;
	}

	if (team)
	{
		m_iIsATeam[m_iRows++] = TEAM_BLANK;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the existing teams in the match
//-----------------------------------------------------------------------------
void ScorePanel::RebuildTeams()
{
	// clear out player counts from teams
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].players = 0;
	}

	// rebuild the team list
	GetAllPlayersInfo();

	m_iNumTeams = 0;
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( g_PlayerInfoList[i].name == NULL )
			continue;

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

		// is this player in an existing team?
		for ( int j = 1; j <= m_iNumTeams; j++ )
		{
			if ( g_TeamInfo[j].name[0] == '\0' )
				break;

			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}

		if ( j > m_iNumTeams )
		{ // they aren't in a listed team, so make a new one
			// search through for an empty team slot
			for ( int j = 1; j <= m_iNumTeams; j++ )
			{
				if ( g_TeamInfo[j].name[0] == '\0' )
					break;
			}
			m_iNumTeams = max( j, m_iNumTeams );

			Q_strncpy( g_TeamInfo[j].name, g_PlayerExtraInfo[i].teamname, MAX_TEAM_NAME );
			g_TeamInfo[j].players = 0;
		}

		g_TeamInfo[j].players++;
	}

	// clear out any empty teams
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		if ( g_TeamInfo[i].players < 1 )
			memset( &g_TeamInfo[i], 0, sizeof(team_info_t) );
	}

	// Update the scoreboard
	OnThink();
}

void ScorePanel::FillGrid()
{
	// remove highlight row if we're not in squelch mode
	if (!GetClientVoiceMgr()->IsInSquelchMode())
	{
		m_iHighlightRow = -1;
	}

	bool bNextRowIsGap = false;

	for(int row=0; row < NUM_ROWS; row++)
	{
		CGrid *pGridRow = m_PlayerGrids[row];
		if ( !pGridRow )
		{
			pGridRow = new CGrid();
			m_PlayerGrids[ row ] = pGridRow;
		}

		pGridRow->SetRowUnderline(0, false, 0, 0, 0, 0, 0);

		if(row >= m_iRows)
		{
			for(int col=0; col < NUM_COLUMNS; col++)
				m_PlayerEntries[col][row]->SetVisible(false);
		
			continue;
		}

		bool bRowIsGap = false;
		if (bNextRowIsGap)
		{
			bNextRowIsGap = false;
			bRowIsGap = true;
		}

		for(int col=0; col < NUM_COLUMNS; col++)
		{
			CLabelHeader *pLabel = m_PlayerEntries[col][row];

			pLabel->SetVisible(true);
			pLabel->SetText2("");
			pLabel->SetFont(m_hsfont);
			pLabel->SetTextOffset(0, 0);
			
			int rowheight = YHEIGHT( 13 );
			pLabel->SetSize(pLabel->GetWide(), rowheight);
			pLabel->SetBgColor( Color( 0, 0, 0, 0 ) );
			
			char sz[128];
			hud_player_info_t *pl_info = NULL;
			team_info_t *team_info = NULL;

			if (m_iIsATeam[row] == TEAM_BLANK)
			{
				pLabel->SetText(" ");
				continue;
			}
			else if ( m_iIsATeam[row] == TEAM_YES )
			{
				// Get the team's data
				team_info = &g_TeamInfo[ m_iSortedRows[row] ];

				int r, g, b, a;

				r = g = b = a = 255;
				// team color text for team names

#if defined( TF2_CLIENT_DLL )
				int teamNumber = team_info->teamnumber;

				// Relative team colors...
				if (!GetLocalTeam())
				{
					r = g = b = 224;
				}
				else
				{
					if (teamNumber == GetLocalTeam()->GetTeamNumber())
					{
						r = b = 0;
						g = 255;
					}
					else
					{
						g = b = 0;
						r = 255;
					}
				}

				// Absolute team colors
//				CMapTeamColors *team = &MapData().m_TeamColors[ teamNumber ];
//				team->m_clrTeam.GetColor( r, g, b, a );
#endif

				pLabel->SetFgColor(	Color ( r,
									g,
									b,
									255 ) );

				// different height for team header rows
				rowheight = YHEIGHT( 20 );
				pLabel->SetSize(pLabel->GetWide(), rowheight);
				pLabel->SetFont(m_htfont);

				pGridRow->SetRowUnderline(	0,
											true,
											YHEIGHT(1),
											r,
											g,
											b,
											255 );
			}
			else if ( m_iIsATeam[row] == TEAM_SPECTATORS )
			{
				// grey text for spectators
				pLabel->SetFgColor(Color( 100, 100, 100, 255 ));

				// different height for team header rows
				rowheight = YHEIGHT( 20 );
				pLabel->SetSize(pLabel->GetWide(), rowheight);
				pLabel->SetFont(m_htfont);

				pGridRow->SetRowUnderline(0, true, YHEIGHT(3), 100, 100, 100, 255);
			}
			else
			{
				// team color text for team names
				int r, g, b, a;
				r = g = b = a = 255;

#if defined( TF2_CLIENT_DLL )
				int teamNumber = g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber;

				if (!GetLocalTeam())
				{
					r = g = b = 224;
				}
				else
				{
					// Relative team colors...
					if (teamNumber == GetLocalTeam()->GetTeamNumber())
					{
						r = b = 0;
						g = 255;
					}
					else
					{
						g = b = 0;
						r = 255;
					}
				}

//				CMapTeamColors *team = &MapData().m_TeamColors[ teamNumber ];
//				team->m_clrTeam.GetColor( r, g, b, a );
#endif
				// team color text for player names
				pLabel->SetFgColor(	Color( r, g, b, 255 ) );

				// Get the player's data
				pl_info = &g_PlayerInfoList[ m_iSortedRows[row] ];

				// Set background color
				if ( m_iSortedRows[row] == engine->GetPlayer() ) // if it is their name, draw it a different color
				{
					// Highlight this player
					pLabel->SetFgColor( Color( 255, 255, 255, 255 ) );
					pLabel->SetBgColor(	Color( r, g, b, 60 ) );
				}
				else if ( m_iSortedRows[row] == m_iLastKilledBy && m_fLastKillTime && m_fLastKillTime > gpGlobals->curtime )
				{
					// Killer's name
					pLabel->SetBgColor( Color( 255,0,0, ((float)15 * (float)(m_fLastKillTime - gpGlobals->curtime)) ) );
				}
			}				

			// Align 
			if (col == COLUMN_NAME || col == COLUMN_CLASS)
			{
				pLabel->SetContentAlignment( vgui::Label::a_west );
			}
			else if (col == COLUMN_TRACKER)
			{
				pLabel->SetContentAlignment( vgui::Label::a_center );
			}
			else
			{
				pLabel->SetContentAlignment( vgui::Label::a_east );
			}

			// Fill out with the correct data
			strcpy(sz, "");
			if ( m_iIsATeam[row] && team_info )
			{
				char sz2[128];

				switch (col)
				{
				case COLUMN_NAME:
					if ( m_iIsATeam[row] == TEAM_SPECTATORS )
					{
						Q_snprintf( sz2, sizeof( sz2 ), hudtextmessage->BufferedLocaliseTextString( "#Spectators" ) );
					}
					else
					{
						Q_snprintf( sz2, sizeof( sz2 ), team_info->name );
					}

					strcpy(sz, sz2);

					// Append the number of players
					if ( m_iIsATeam[row] == TEAM_YES )
					{
						if (team_info->players == 1)
						{
							Q_snprintf(sz2, sizeof( sz2 ), "(%d %s)", team_info->players, hudtextmessage->BufferedLocaliseTextString( "#Player" ) );
						}
						else
						{
							Q_snprintf(sz2, sizeof( sz2 ), "(%d %s)", team_info->players, hudtextmessage->BufferedLocaliseTextString( "#Player_plural" ) );
						}

						pLabel->SetText2(sz2);
						pLabel->SetFont2(m_hsmallfont);
					}
					break;
				case COLUMN_VOICE:
					break;
				case COLUMN_CLASS:
					break;
				case COLUMN_KILLS:
					if ( m_iIsATeam[row] == TEAM_YES )
						Q_snprintf(sz, sizeof( sz ) , "%d",  team_info->frags );
					break;
				case COLUMN_DEATHS:
					if ( m_iIsATeam[row] == TEAM_YES )
						Q_snprintf(sz, sizeof( sz ) , "%d",  team_info->deaths );
					break;
				case COLUMN_LATENCY:
					if ( m_iIsATeam[row] == TEAM_YES )
						Q_snprintf(sz, sizeof( sz ) , "%d", team_info->ping );
					break;
				default:
					break;
				}
			}
			else if ( pl_info )
			{
				bool bShowClass = false;

				switch (col)
				{
				case COLUMN_NAME:
					/*
					if (g_pTrackerUser)
					{
						int playerSlot = m_iSortedRows[row];
						int trackerID = engine->GetTrackerIDForPlayer(playerSlot);
						const char *trackerName = g_pTrackerUser->GetUserName(trackerID);
						if (trackerName && *trackerName)
						{
							Q_snprintf(sz, "   (%s)", trackerName);
							pLabel->setText2(sz);
						}
					}
					*/
					Q_snprintf(sz, sizeof( sz ), "%s  ", pl_info->name);
					break;
				case COLUMN_VOICE:
					sz[0] = 0;
					// in HLTV mode allow spectator to turn on/off commentator voice
					if (!pl_info->thisplayer /*|| engine->IsSpectateOnly()*/ )
					{
						GetClientVoiceMgr()->UpdateSpeakerImage(pLabel, m_iSortedRows[row]);
					}
					break;
				case COLUMN_CLASS:
					{
						// No class for other team's members (unless allied or spectator)
						int localPlayerTeam = C_BasePlayer::GetLocalPlayer() ? C_BasePlayer::GetLocalPlayer()->GetTeamNumber() : 0;
						if ( localPlayerTeam == g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber )
						{
							bShowClass = true;
						}
						// Don't show classes if this client hasnt picked a team yet
						if ( localPlayerTeam == 0 )
						{
							bShowClass = false;
						}

#if defined( TF2_CLIENT_DLL )
						if (bShowClass)
						{
							// Only print Civilian if this team are all civilians
							int iPlayerClass = g_PlayerExtraInfo[ m_iSortedRows[row] ].playerclass;
							const char *pClassName = GetTFClassInfo( iPlayerClass )->m_pClassName;
							
							char localizedClass[128];
							Q_snprintf( localizedClass, sizeof( localizedClass ), "#%s", pClassName );

							Q_snprintf( sz, sizeof( sz ), "%s", hudtextmessage->BufferedLocaliseTextString( localizedClass ) );
						}
						else
#endif
						{
							strcpy(sz, "");
						}
					}
					break;

				case COLUMN_TRACKER:
					/*
					if (g_pTrackerUser)
					{
						int playerSlot = m_iSortedRows[row];
						int trackerID = engine->GetTrackerIDForPlayer(playerSlot);

						if (g_pTrackerUser->IsFriend(trackerID) && trackerID != g_pTrackerUser->GetTrackerID())
						{
							pLabel->setImage(m_pTrackerIcon);
							pLabel->setFgColorAsImageColor(false);
							m_pTrackerIcon->setColor(Color(255, 255, 255, 255));
						}
					}
					*/
					break;
				case COLUMN_KILLS:
					Q_snprintf(sz, sizeof( sz ), "%d",  g_PlayerExtraInfo[ m_iSortedRows[row] ].frags );
					break;
				case COLUMN_DEATHS:
					Q_snprintf(sz, sizeof( sz ), "%d",  g_PlayerExtraInfo[ m_iSortedRows[row] ].deaths );
					break;
				case COLUMN_LATENCY:
					Q_snprintf(sz, sizeof( sz ), "%d", g_PlayerExtraInfo[ m_iSortedRows[row] ].ping );
					break;
				default:
					break;
				}
			}

			pLabel->SetText(sz);
		}
	}

	for(row=0; row < NUM_ROWS; row++)
	{
		CGrid *pGridRow = m_PlayerGrids[row];

		pGridRow->AutoSetRowHeights();
		pGridRow->SetSize(PanelWidth(pGridRow), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();
	}

	// hack, for the thing to resize
//	m_PlayerList.GetSize(x, y);
//	m_PlayerList.SetSize(x, y);
}


//-----------------------------------------------------------------------------
// Purpose: Setup highlights for player names in scoreboard
//-----------------------------------------------------------------------------
void ScorePanel::DeathMsg( int killer, int victim )
{
	// if we were the one killed,  or the world killed us, set the scoreboard to indicate suicide
	if ( victim == m_iPlayerNum || killer == 0 )
	{
		m_iLastKilledBy = killer ? killer : m_iPlayerNum;
		m_fLastKillTime = gpGlobals->curtime + 10;	// display who we were killed by for 10 seconds

		if ( killer == m_iPlayerNum )
			m_iLastKilledBy = m_iPlayerNum;
	}
}

void ScorePanel::Close( void )
{
	if ( render->IsPlayingDemo() )
		return;

//	::input->ResetMouse();
}

void ScorePanel::Open( void )
{
	RebuildTeams();
	SetZPos( 1000 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ScorePanel::UserCmd_ShowScores( void )
{
	m_iShowscoresHeld = true;

	SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ScorePanel::UserCmd_HideScores( void )
{
	m_iShowscoresHeld = false;

	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highligh status
//-----------------------------------------------------------------------------
void CLabelHeader::PaintBackground()
{
	Color oldBg = GetBgColor();

	if ( GetScoreBoard() && GetScoreBoard()->m_iHighlightRow == _row)
	{
		SetBgColor( Color( 134, 91, 19, 255 ) );
	}

	Panel::PaintBackground();

	SetBgColor(oldBg);
}
		

//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highligh status
//-----------------------------------------------------------------------------
void CLabelHeader::Paint()
{
	Panel::Paint();
	Color oldFg = GetFgColor();

	if ( GetScoreBoard() && GetScoreBoard()->m_iHighlightRow == _row)
	{
		SetFgColor(255, 255, 255, 255);
	}

	// draw text
	int x, y, iwide, itall;
	GetTextSize(iwide, itall);
	CalcAlignment(iwide, itall, x, y);
	_dualImage->SetPos(x, y);

	int x1, y1;
	_dualImage->GetImage(1)->GetPos(x1, y1);
	_dualImage->GetImage(1)->SetPos(_gap, y1);

	_dualImage->Paint();

	// get size of the panel and the image
	TextImage *_image = GetTextImage();

	if (_image)
	{
		Color imgColor;
		imgColor = GetFgColor();
		if( _useFgColorAsImageColor )
		{
			_image->SetColor( imgColor );
		}

		_image->GetSize(iwide, itall);
		CalcAlignment(iwide, itall, x, y);
		_image->SetPos(x, y);
		_image->Paint();
	}

	SetFgColor(oldFg[0], oldFg[1], oldFg[2], oldFg[3]);
}

void CLabelHeader::SetContentAlignment(Alignment alignment)
{
	m_Align = alignment;
	BaseClass::SetContentAlignment( alignment );
}

void CLabelHeader::CalcAlignment(int iwide, int itall, int &x, int &y)
{
	// calculate alignment ourselves, since vgui is so broken
	int wide, tall;
	GetSize(wide, tall);

	x = 0, y = 0;

	// align left/right
	switch (m_Align)
	{
		// left
		case Label::a_northwest:
		case Label::a_west:
		case Label::a_southwest:
		{
			x = 0;
			break;
		}
		
		// center
		case Label::a_north:
		case Label::a_center:
		case Label::a_south:
		{
			x = (wide - iwide) / 2;
			break;
		}
		
		// right
		case Label::a_northeast:
		case Label::a_east:
		case Label::a_southeast:
		{
			x = wide - iwide;
			break;
		}
	}

	// top/down
	switch (m_Align)
	{
		// top
		case Label::a_northwest:
		case Label::a_north:
		case Label::a_northeast:
		{
			y = 0;
			break;
		}
		
		// center
		case Label::a_west:
		case Label::a_center:
		case Label::a_east:
		{
			y = (tall - itall) / 2;
			break;
		}
		
		// south
		case Label::a_southwest:
		case Label::a_south:
		case Label::a_southeast:
		{
			y = tall - itall;
			break;
		}
	}

// don't clip to Y
//	if (y < 0)
//	{
//		y = 0;
//	}
	if (x < 0)
	{
		x = 0;
	}

	x += _offset[0];
	y += _offset[1];
}

/*
int TeamFortressViewport::MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf )
{
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
		g_PlayerExtraInfo[cl].teamnumber = teamnumber;

		//Dont go bellow 0!
		if ( g_PlayerExtraInfo[cl].teamnumber < 0 )
			 g_PlayerExtraInfo[cl].teamnumber = 0;

		UpdateOnPlayerInfo();
	}

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
	BEGIN_READ( pbuf, iSize );
	char *TeamName = READ_STRING();

	// find the team matching the name
	for ( int i = 1; i <= m_pScoreBoard->m_iNumTeams; i++ )
	{
		if ( !stricmp( TeamName, g_TeamInfo[i].name ) )
			break;
	}

	if ( i > m_pScoreBoard->m_iNumTeams )
		return 1;

	// use this new score data instead of combined player scoresw
	g_TeamInfo[i].scores_overriden = TRUE;
	g_TeamInfo[i].frags = READ_SHORT();
	g_TeamInfo[i].deaths = READ_SHORT();

	return 1;
}

// Message handler for TeamInfo message
// accepts two values:
//		byte: client number
//		string: client team name
int TeamFortressViewport::MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf )
{
	if (!m_pScoreBoard)
		return 1;

	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE();
	
	if ( cl > 0 && cl <= MAX_PLAYERS )
	{  
		// set the players team
		strncpy( g_PlayerExtraInfo[cl].teamname, READ_STRING(), MAX_TEAM_NAME );
	}

	// rebuild the list of teams
	m_pScoreBoard->RebuildTeams();

	return 1;
}

void TeamFortressViewport::DeathMsg( int killer, int victim )
{
	m_pScoreBoard->DeathMsg(killer,victim);
}

int TeamFortressViewport::MsgFunc_Spectator( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	short cl = READ_BYTE();
	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_IsSpectator[cl] = READ_BYTE();
	}

	return 1;
}

int TeamFortressViewport::MsgFunc_AllowSpec( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_iAllowSpectators = READ_BYTE();

	// Force the menu to update
	UpdateCommandMenu( m_StandardMenu );

	// If the team menu is up, update it too
	if (m_pTeamMenu)
		m_pTeamMenu->Update();

	return 1;
}
*/
