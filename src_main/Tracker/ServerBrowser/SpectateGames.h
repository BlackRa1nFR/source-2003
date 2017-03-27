//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SPECTATEGAMES_H
#define SPECTATEGAMES_H
#ifdef _WIN32
#pragma once
#endif

#include "InternetGames.h"

//-----------------------------------------------------------------------------
// Purpose: Spectator games list
//-----------------------------------------------------------------------------
class CSpectateGames : public CInternetGames
{
public:
	CSpectateGames(vgui::Panel *parent) : CInternetGames(parent, "SpectateGames") {}

protected:
	// filters by spectator games
	virtual void RequestServers(int Start, const char *filterString);
	virtual bool CheckPrimaryFilters(serveritem_t &server);

private:
	typedef CInternetGames BaseClass;
};


#endif // SPECTATEGAMES_H
