//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CSTEAMMENU_H
#define CSTEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <game_controls/TeamMenu.h>

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CCSTeamMenu : public CTeamMenu
{
private:
	typedef CTeamMenu BaseClass;
public:
	CCSTeamMenu(vgui::Panel *parent);
	~CCSTeamMenu();
	void Update( const char *mapName, bool allowSpectators, const char **teamNames, int numTeams );

private:
	enum { NUM_TEAMS = 3 };

	// VGUI2 override
	void OnCommand( const char *command);
	// helper functions
	void SetVisibleButton(const char *textEntryName, bool state);

	bool m_bVIPMap;
};

#endif // CSTEAMMENU_H
