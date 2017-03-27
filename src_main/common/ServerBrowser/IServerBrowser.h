//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ISERVERBROWSER_H
#define ISERVERBROWSER_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

// handle to a game window
typedef unsigned int GameHandle_t;

//-----------------------------------------------------------------------------
// Purpose: Interface to server browser module
//-----------------------------------------------------------------------------
class IServerBrowser
{
public:
	// activates the server browser window, brings it to the foreground
	virtual bool Activate() = 0;

	// opens a game info dialog to watch the specified server; associated with the friend 'userName'
	virtual GameHandle_t OpenGameInfoDialog(unsigned int gameIP, unsigned int gamePort, const char *userName) = 0;

	// joins a specified game - game info dialog will only be opened if the server is fully or passworded
	virtual GameHandle_t JoinGame(unsigned int gameIP, unsigned int gamePort, const char *userName) = 0;

	// changes the game info dialog to watch a new server; normally used when a friend changes games
	virtual void UpdateGameInfoDialog(GameHandle_t gameDialog, unsigned int gameIP, unsigned int gamePort) = 0;

	// forces the game info dialog closed
	virtual void CloseGameInfoDialog(GameHandle_t gameDialog) = 0;
};

#define SERVERBROWSER_INTERFACE_VERSION "ServerBrowser001"



#endif // ISERVERBROWSER_H
