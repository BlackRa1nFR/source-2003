//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERBROWSER_H
#define SERVERBROWSER_H
#ifdef _WIN32
#pragma once
#endif

#include "ServerBrowser/IServerBrowser.h"
#include "IVGuiModule.h"

#include <vgui_controls/PHandle.h>

class CServerBrowserDialog;

//-----------------------------------------------------------------------------
// Purpose: Handles the UI and pinging of a half-life game server list
//-----------------------------------------------------------------------------
class CServerBrowser : public IServerBrowser, public IVGuiModule
{
public:
	CServerBrowser();
	~CServerBrowser();

	// IVGui module implementation
	virtual bool Initialize(CreateInterfaceFn *factorylist, int numFactories);
	virtual bool PostInitialize(CreateInterfaceFn *modules, int factoryCount);
	virtual vgui::VPANEL GetPanel();
	virtual bool Activate();
	virtual bool IsValid();
	virtual void Shutdown();
	virtual void Deactivate();
	virtual void Reactivate();
	virtual void SetParent(vgui::VPANEL parent);

	// IServerBrowser implementation
	// opens a game info dialog to watch the specified server; associated with the friend 'userName'
	virtual GameHandle_t OpenGameInfoDialog(unsigned int gameIP, unsigned int gamePort, const char *userName);

	// joins a specified game - game info dialog will only be opened if the server is fully or passworded
	virtual GameHandle_t JoinGame(unsigned int gameIP, unsigned int gamePort, const char *userName);

	// changes the game info dialog to watch a new server; normally used when a friend changes games
	virtual void UpdateGameInfoDialog(GameHandle_t gameDialog, unsigned int gameIP, unsigned int gamePort);

	// forces the game info dialog closed
	virtual void CloseGameInfoDialog(GameHandle_t gameDialog);

	// closes all the game info dialogs
	virtual void CloseAllGameInfoDialogs();

	// methods
	virtual void CreateDialog();
	virtual void Open();

private:
	vgui::DHANDLE<CServerBrowserDialog> m_hInternetDlg;
};


#endif // SERVERBROWSER_H
