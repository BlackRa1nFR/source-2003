//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( VINTERNETDLG_H )
#define VINTERNETDLG_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>
#include <VGUI_ListPanel.h>
#include <VGUI_PHandle.h>

#include "UtlVector.h"
#include "netadr.h"
#include "server.h"

#include "IGameList.h"

class CServerContextMenu;
namespace vgui
{
class Label;
class Font;
class ListPanel;
class Button;
class ComboBox;
class QueryBox;
class TextEntry;
class CheckButton;
class PropertySheet;
}

extern class IRunGameEngine *g_pRunGameEngine;
extern void v_strncpy(char *dest, const char *src, int bufsize);

class CFavoriteGames;
class CInternetGames;
class CSpectateGames;
class CLanGames;
class CFriendsGames;
class CDialogGameInfo;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class VInternetDlg : public vgui::Frame
{
	typedef vgui::Frame BaseClass;

public:
	// Construction/destruction
						VInternetDlg( void );
	virtual				~VInternetDlg( void );

	virtual void		Initialize( void );

	// displays the dialog, moves it into focus, updates if it has to
	virtual void		Open( void );

	// gets server info
	serveritem_t &VInternetDlg::GetServer(unsigned int serverID);

	// setup
	virtual void		PerformLayout();

	// updates status text at bottom of window
	virtual void		UpdateStatusText(const char *format, ...);
	
	// updates status text at bottom of window
	virtual void		UpdateStatusText(wchar_t *unicode);

	// context menu access
	virtual CServerContextMenu *GetContextMenu();		

	// returns a pointer to a static instance of this dialog
	// valid for use only in sort functions
	static VInternetDlg *GetInstance();

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

	// accessor to the filter save data
	virtual vgui::KeyValues *GetFilterSaveData(const char *filterSet);

	// gets the name of the mod directory we're restricted to accessing, NULL if none
	virtual const char *GetActiveModName();

private:

	// current game list change
	virtual void		OnGameListChanged();

	// receives a specified game is active, so no other game types can be displayed in server list
	virtual void		OnActiveGameName(const char *gameName);

	// load/saves filter settings from disk
	virtual void		LoadFilters();
	virtual void		SaveFilters();

	// Load/saves position and window size of a dialog from disk
	virtual void		LoadDialogState(vgui::Panel *dialog, const char *dialogName);
	virtual void		SaveDialogState(vgui::Panel *dialog, const char *dialogName);

	// called when dialog is shut down
	virtual void		OnClose();

	void PaintBackground();

	DECLARE_PANELMAP();

private:

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

	vgui::KeyValues *m_pSavedData;

	// context menu
	CServerContextMenu *m_pContextMenu;

	// active game
	char m_szGameName[128];
};

#endif // VINTERNETDLG_H