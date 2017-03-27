//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef TRACKERDIALOG_H
#define TRACKERDIALOG_H
#pragma once

#include <stdio.h>

#include <VGUI_Color.h>
#include <VGUI_Dar.h>
#include <VGUI_Frame.h>
#include <VGUI_PHandle.h>

class KeyValues;

namespace vgui
{
class Menu;
class SurfaceBase;
class MenuButton;
class Label;
class ImagePanel;
};

class CSubPanelBuddyList;
class CSubPanelTrackerState;
class CPanelValveLogo;

class CServerSession;
class CTrackerDoc;

class CDialogFindBuddy;
class CDialogLoginUser;
class CDialogOptions;
class CDialogAbout;
class CDialogChat;
class COnlineStatusSelectComboBox;

//-----------------------------------------------------------------------------
// Purpose: The main tracker dialog, handles buddy list, in game, user status
//-----------------------------------------------------------------------------
class CTrackerDialog : public vgui::Frame
{
public:
	CTrackerDialog();
	~CTrackerDialog();

	// initialization
	bool Start();
	void StartTrackerWithUser(int userID);
	void OpenSavedChats();

	// brings tracker to the foreground
	void Activate();

	// closes tracker, shuts down it's networking, saves any data
	void Shutdown();

	// static instance of tracker dialog
	static CTrackerDialog *GetInstance();

	// runs a frame of the networking
	void RunFrame();

	// closes all the Tracker dialogs
	void ShutdownUI();

	void MarkUserDataAsNeedsSaving();
	void SetChatDialogInList(CDialogChat *chatDialog, bool remove);

	// forces all the buddy lists/buddy buttons to redraw
	void OnFriendsStatusChanged(int friendCount);

private:
	void Initialize(void);

	// message handlers
	void OnOnlineStatusChanged(int newStatus);	// called when our online status changes
	void OnUserCreateFinished(int userid, const char *password);
	void OnUserCreateCancel();
	void OnUserChangeStatus(int status);
	void OnReceivedMessage(vgui::KeyValues *data);
	void OnReceivedGameInfo(vgui::KeyValues *data);
	void OnReceivedFriendInfo(vgui::KeyValues *data);
	void OnReceivedUserBlock(vgui::KeyValues *data);
	void OnLoginOK();
	void OnLoginFail(int reason);
	void OnQuit();
	void OnSaveUserData();
	void OnAddedToChat(vgui::KeyValues *msg);
	void OnSystemMessage(vgui::KeyValues *msg);

	// in game handling
	void OnActiveGameName(const char *gameName);
	void OnConnectToGame(int IP, int port);
	void OnDisconnectFromGame();

	DECLARE_PANELMAP();
	/* MESSAGE HANDLING
		"OnlineStatus"	- network message
			"value"	- new online status value
		"OnUserCreated" - panel message
			"userid" - new user id
			"password" - login password
	*/

	// vgui-message handlers
	virtual void OnTick();
	virtual void OnCommand(const char *command );
	virtual void OnClose(void);
	virtual void PerformLayout(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnMinimize();
	virtual void OnSetFocus();

	// methods
	void OnUserCreated(int userid, const char *password);
	void PopupCreateNewUserDialog(bool performConnectionTest);
	void OpenServerBrowser();
	void OpenOptionDialog();
	void OpenAboutDialog();

	// data
	vgui::MenuButton *m_pTrackerButton;
	vgui::Menu *m_pContextMenu;
	vgui::Menu *m_pTrackerMenu;
	CSubPanelBuddyList *m_pBuddyListPanel;
	CSubPanelTrackerState *m_pStatePanel;

	bool m_bOnline;
	bool m_bInGame;
	bool m_bNeedUpdateToFriendStatus;
	bool m_bNeedToSaveUserData;
	bool m_bOpenContextMenu;
	bool m_bInAutoAwayMode;

	int m_iGameIP;
	int m_iGamePort;
	char m_szGameDir[64];

	int m_iLoginTransitionPixel;

	vgui::DHANDLE<CDialogFindBuddy> m_hDialogFindBuddy;
	vgui::DHANDLE<CDialogLoginUser> m_hDialogLoginUser;
	vgui::DHANDLE<CDialogOptions> m_hDialogOptions;
	vgui::DHANDLE<CDialogAbout> m_hDialogAbout;
	vgui::PHandle m_hServerBrowser;

	vgui::Dar<CDialogChat *> m_hChatDialogs;

	typedef vgui::Frame BaseClass;
};


#endif // TRACKERDIALOG_H
