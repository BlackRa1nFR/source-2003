//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CSSPECTATORGUI_H
#define CSSPECTATORGUI_H
#ifdef _WIN32
#pragma once
#endif

#include <game_controls/SpectatorGUI.h>


//-----------------------------------------------------------------------------
// Purpose: Cstrike Spectator UI
//-----------------------------------------------------------------------------
class CCSSpectatorGUI : public CSpectatorGUI
{
private:
	typedef CSpectatorGUI BaseClass;

public:
	CCSSpectatorGUI(vgui::Panel *parent);
	~CCSSpectatorGUI();

	virtual void UpdateSpectatorPlayerList();
	virtual void UpdateTimer();
	virtual void Update();
};


#endif // CSSPECTATORGUI_H
