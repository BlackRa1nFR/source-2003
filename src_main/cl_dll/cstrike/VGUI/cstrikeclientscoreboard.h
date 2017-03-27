//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CSTRIKECLIENTSCOREBOARDDIALOG_H
#define CSTRIKECLIENTSCOREBOARDDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <game_controls/ClientScoreBoardDialog.h>
#include "cs_shareddefs.h"
#include <igameevents.h>

//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CCSClientScoreBoardDialog : public CClientScoreBoardDialog, public IGameEventListener
{
	typedef CClientScoreBoardDialog BaseClass;

public:
	CCSClientScoreBoardDialog(vgui::Panel *parent);
	~CCSClientScoreBoardDialog();

public:	
	void FireGameEvent( KeyValues * event);
	void GameEventsUpdated();
	bool IsLocalListener() { return false; };


protected:
	// scoreboard overrides
	virtual void InitScoreboardSections();
	virtual void UpdateTeamInfo();
	virtual bool GetPlayerScoreInfo(int playerIndex, KeyValues *outPlayerInfo);
	virtual void UpdatePlayerInfo();

private:
	virtual void AddHeader(); // add the start header of the scoreboard
	virtual void AddSection(int teamType, int teamNumber); // add a new section header for a team

	enum 
	{ 
		CSTRIKE_NAME_WIDTH = 220,
		CSTRIKE_CLASS_WIDTH = 56,
		CSTRIKE_SCORE_WIDTH = 40,
		CSTRIKE_DEATH_WIDTH = 46,
		CSTRIKE_PING_WIDTH = 46,
//		CSTRIKE_VOICE_WIDTH = 40, 
//		CSTRIKE_FRIENDS_WIDTH = 24,
	};
};


#endif // CSTRIKECLIENTSCOREBOARDDIALOG_H
