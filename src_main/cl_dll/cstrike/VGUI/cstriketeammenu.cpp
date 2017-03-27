//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "CStrikeTeamMenu.h"
#include <game_controls/IViewPort.h>
#include "hud.h" // for gEngfuncs
#include "c_cs_player.h"
#include "cs_gamerules.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCSTeamMenu::CCSTeamMenu(vgui::Panel *parent) : CTeamMenu(parent)
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCSTeamMenu::~CCSTeamMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: called to update the menu with new information
//-----------------------------------------------------------------------------
void CCSTeamMenu::Update( const char *mapName, bool allowSpectators, const char **teamNames, int numTeams )
{
	BaseClass::Update( mapName, allowSpectators, NULL, 0 );

	if ( allowSpectators )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();	// Get the local player's index

		// if we're not already a CT or T...or the freeze time isn't over yet...or we're dead
		if ( C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() == TEAM_UNASSIGNED || 
			CSGameRules()->IsFreezePeriod() || 
			( pPlayer && pPlayer->IsPlayerDead() ) )
		{
			SetVisibleButton("specbutton", true);
		}
		else
		{
			SetVisibleButton("specbutton", false);
		}
	}
	else
	{
		SetVisibleButton("specbutton", false);
	}

	m_bVIPMap = false;
	if ( !strncmp( mapName, "maps/as_", 8 ) )
	{
		m_bVIPMap = true;
	}
		
	// if this isn't a VIP map or we're a spectator/terrorist, then disable the VIP button
	if ( !CSGameRules()->IsVIPMap() || ( C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() != TEAM_CT ) )
	{
		SetVisibleButton("vipbutton", false);
	}
	else // this must be a VIP map and we must already be a CT
	{
		SetVisibleButton("vipbutton", true);
	}

	if( C_CSPlayer::GetLocalCSPlayer()->GetTeamNumber() == TEAM_UNASSIGNED ) // we aren't on a team yet
	{
		SetVisibleButton("CancelButton", false); 
	}
	else
	{
		SetVisibleButton("CancelButton", true); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: When a team button is pressed it triggers this function to 
//			cause the player to join a team
//-----------------------------------------------------------------------------
void CCSTeamMenu::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( command );
	}
	
	
	BaseClass::OnCommand(command);

	gViewPortInterface->HideBackGround();
	OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the visibility of a button by name
//-----------------------------------------------------------------------------
void CCSTeamMenu::SetVisibleButton(const char *textEntryName, bool state)
{
	Button *entry = dynamic_cast<Button *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetVisible(state);
	}
}
