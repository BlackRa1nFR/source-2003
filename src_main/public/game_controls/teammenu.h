//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TEAMMENU_H
#define TEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui/KeyCode.h>
#include <UtlVector.h>

namespace vgui
{
	class RichText;
	class HTML;
}
class TeamFortressViewport;


//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CTeamMenu : public vgui::Frame
{
private:
	typedef vgui::Frame BaseClass;


public:
	CTeamMenu(vgui::Panel *parent);
	virtual ~CTeamMenu();

	void Activate( );
	void Update( const char *mapName, bool allowSpectators, const char **teamNames, int numTeams );
	void AutoAssign();
	
protected:

	int GetNumTeams() { return m_iNumTeams; }
	CUtlVector<vgui::Button *> m_pTeamButtons;

	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);

private:
	enum { MAX_VALID_TEAMS = 6 };

	// helper functions
	void SetLabelText(const char *textEntryName, const char *text);
	void LoadMapPage( const char *mapName );
	void MakeTeamButtons( const char **teamNames, int numTeams );
	
	// command callbacks
	void OnTeamButton( int team );


	vgui::RichText *m_pMapInfo;
	int m_iNumTeams;
	int m_iJumpKey;
	int m_iScoreBoardKey;

	DECLARE_PANELMAP();

};


#endif // TEAMMENU_H
