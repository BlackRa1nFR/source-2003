//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERBROWSERDIALOG_H
#define SERVERBROWSERDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/PHandle.h>

#include "UtlVector.h"
#include "netadr.h"
#include "server.h"

#include "IGameList.h"

class CServerContextMenu;
extern class IRunGameEngine *g_pRunGameEngine;
extern void v_strncpy(char *dest, const char *src, int bufsize);

class CFavoriteGames;
class CInternetGames;
class CSpectateGames;
class CLanGames;
class CFriendsGames;
class CDialogGameInfo;
class CBaseGamesPage;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CServerBrowserDialog : public vgui::Frame
{
	typedef vgui::Frame BaseClass;

public:
	// Construction/destruction
	CServerBrowserDialog( vgui::Panel *parent );
	virtual				~CServerBrowserDialog( void );

	virtual void		Initialize( void );

	// displays the dialog, moves it into focus, updates if it has to
	virtual void		Open( void );

	// gets server info
	serveritem_t &CServerBrowserDialog::GetServer(unsigned int serverID);

	// setup
	virtual void		PerformLayout();

	// updates status text at bottom of window
	virtual void		UpdateStatusText(const char *format, ...);
	
	// updates status text at bottom of window
	virtual void		UpdateStatusText(wchar_t *unicode);

	// context menu access
	virtual CServerContextMenu *GetContextMenu(vgui::Panel *pParent);		

	// returns a pointer to a static instance of this dialog
	// valid for use only in sort functions
	static CServerBrowserDialog *GetInstance();

	// Adds a server to the list of favorites
	virtual void		AddServerToFavorites(serveritem_t &server);

	// begins the process of joining a server from a game list
	// the game info dialog it opens will also update the game list
	virtual CDialogGameInfo *JoinGame(IGameList *gameList, unsigned int serverIndex);

	// joins a game by a specified IP, not attached to any game list
	virtual CDialogGameInfo *JoinGame(int serverIP, int serverPort, const char *titleName);

	// opens a game info dialog from a game list
	virtual CDialogGameInfo *OpenGameInfoDialog(IGameList *gameList, unsigned int serverIndex);

	// opens a game info dialog by a specified IP, not attached to any game list
	virtual CDialogGameInfo *OpenGameInfoDialog(int serverIP, int serverPort, const char *titleName);

	// closes all the game info dialogs
	virtual void CloseAllGameInfoDialogs();

	// accessor to the filter save data
	virtual KeyValues *GetFilterSaveData(const char *filterSet);

	// gets the name of the mod directory we're restricted to accessing, NULL if none
	virtual const char *GetActiveModName();

	// called when dialog is shut down
	virtual void		OnClose();

	// saves filter settings
	virtual void		SaveFilters();

private:

	// current game list change
	virtual void		OnGameListChanged();

	// receives a specified game is active, so no other game types can be displayed in server list
	virtual void		OnActiveGameName(const char *gameName);

	// load/saves filter settings from disk
	virtual void		LoadFilters();

	virtual bool GetDefaultScreenPosition(int &x, int &y, int &wide, int &tall);
	virtual void ActivateBuildMode();
	virtual void PaintBackground();

	DECLARE_PANELMAP();

private:
	// list of all open game info dialogs
	CUtlVector<vgui::PHandle> m_GameInfoDialogs;

	// pointer to current game list
	IGameList *m_pGameList;

	// Status text
	vgui::Label	*m_pStatusLabel;

	// property sheet
	vgui::PropertySheet *m_pTabPanel;
	CFavoriteGames *m_pFavorites;
	CInternetGames *m_pInternetGames;
	CSpectateGames *m_pSpectateGames;
	CLanGames *m_pLanGames;
	CFriendsGames *m_pFriendsGames;

	KeyValues *m_pSavedData;

	// context menu
	CServerContextMenu *m_pContextMenu;

	// active game
	char m_szGameName[128];
};

// singleton accessor
extern CServerBrowserDialog &ServerBrowserDialog();

#endif // SERVERBROWSERDIALOG_H
