//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CLIENTSCOREBOARDDIALOG_H
#define CLIENTSCOREBOARDDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <cl_dll/IVGuiClientDll.h>
#include "IScoreBoardInterface.h"

#define TEAM_NO				0 // TEAM_NO must be zero :)
#define TEAM_YES			1
#define TEAM_SPECTATORS		2
#define TEAM_BLANK			3

struct ScoreBoardTeamInfo : public VGuiLibraryTeamInfo_t
{

	bool already_drawn;
	bool scores_overriden;
	bool ownteam;
};

//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CClientScoreBoardDialog : public vgui::Frame, public IScoreBoardInterface
{
private:
	typedef vgui::Frame BaseClass;

protected:
// column widths at 640
	enum { NAME_WIDTH = 160, SCORE_WIDTH = 60, DEATH_WIDTH = 60, PING_WIDTH = 80, VOICE_WIDTH = 0, FRIENDS_WIDTH = 0 };
	// total = 340

public:
	CClientScoreBoardDialog(vgui::Panel *parent);
	virtual ~CClientScoreBoardDialog();

	virtual void Reset();

	virtual void Update(const char *servername, bool teamplay, bool spectator);
	virtual void RebuildTeams(const char *servername, bool teamplay, bool spectator);
	virtual void DeathMsg(int killer, int victim);

 	virtual void MoveToFront() { BaseClass::MoveToFront(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
  	virtual void SetVisible( bool state ) { BaseClass::SetVisible( state ); }
  	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
 	virtual void SetMouseInputEnabled( bool state ) { BaseClass::SetMouseInputEnabled( state ); }
   
	virtual void Activate(bool spectatorUIVisible);

	virtual void SetTeamName(  int index,  const char *name);
	virtual void SetTeamDetails( int index, int frags, int deaths);
	virtual const char *GetTeamName( int index );
	virtual VGuiLibraryTeamInfo_t GetPlayerTeamInfo( int playerIndex );


protected:
	// functions to override
	virtual bool GetPlayerScoreInfo(int playerIndex, KeyValues *outPlayerInfo);
	virtual void InitScoreboardSections();
	virtual void UpdateTeamInfo();
	virtual void UpdatePlayerInfo();
	virtual Color GetTeamColor(int teamNumber);

	virtual void AddHeader(); // add the start header of the scoreboard
	virtual void AddSection(int teamType, VGuiLibraryTeamInfo_t *team_info); // add a new section header for a team

	// sorts players within a section
	static bool StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	// finds the player in the scoreboard
	int FindItemIDForPlayerIndex(int playerIndex);

	int m_iNumTeams;

	vgui::SectionedListPanel *m_pPlayerList;
	int				m_iSectionId; // the current section we are entering into

	int s_VoiceImage[5];
	int TrackerImage;

	void MoveLabelToFront(const char *textEntryName);

	struct ScoreBoardTeamInfo m_TeamInfo[5];
	struct ScoreBoardTeamInfo m_BlankTeamInfo;

private:
	int m_iPlayerIndexSymbol;
	int m_iDesiredHeight;



	// methods
	void FillScoreBoard();
};


#endif // CLIENTSCOREBOARDDIALOG_H
