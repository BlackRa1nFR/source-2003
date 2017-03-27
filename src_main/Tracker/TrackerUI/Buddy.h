//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BUDDY_H
#define BUDDY_H
#pragma once

#include <VGUI.h>
#include <VGUI_KeyValues.h>
#include <VGUI_PHandle.h>

#include "ServerBrowser/IServerBrowser.h"

class CDialogChat;
class CDialogAuthRequest;
class CDialogGameInfo;
class CDialogUserInfo;

//-----------------------------------------------------------------------------
// Purpose: Holds all the details of a single buddy
//-----------------------------------------------------------------------------
class CBuddy
{
public:
	CBuddy(unsigned int userID, vgui::KeyValues *data);
	~CBuddy();

	unsigned int BuddyID();
	const char *UserName();
	const char *DisplayName();
	vgui::KeyValues *MessageList();
	vgui::KeyValues *Data() { if (this) return m_pData; return 0; }	// returns a pointer to the raw string data of a buddy

	void UpdateStatus(int newStatus, unsigned int sessionID, unsigned int serverID, unsigned int *IP, unsigned int *port, unsigned int *gameIP, unsigned int *gamePort, const char *gameType);
	void RefreshGameInfoDialog();

	void OpenChatDialog(bool minimized);
	void OpenAuthRequestDialog(bool minimized);
	void OpenUserInfoDialog(bool minimized);

	void OpenGameInfoDialog(bool connectImmediately);

	// closes all dialogs associated with the buddy
	void ShutdownDialogs();

	// returns the chat dialog associated with this buddy, if any
	CDialogChat *GetChatDialog();

	// call to Stop the chat dialog being associated with this buddy
	void DisassociateChatDialog();

	// block levels
	enum BlockLevels_e
	{
		BLOCK_NONE,
		BLOCK_GAME,
		BLOCK_ONLINE,
	};
	void SetBlock(int blockLevel, int fakeStatus);
	void SetRemoteBlock(int blockLevel);

	// sets whether or not messages to this friend should be sent via the main tracker server
	void SetSendViaServer(bool state);
	bool SendViaServer();

	// loads a chat from keyvalues
	void LoadChat();
	
private:
	unsigned int m_iUserID;
	int m_iNextPlayTime;	// the time before the user online sound will play again

	int m_iBlockLevel;		// the level at which we are blocked from seeing this user

	bool m_bSendViaServer;	// indicates that messages to this user should be sent via the main tracker server
	
	vgui::KeyValues *m_pData;
	vgui::DHANDLE<CDialogChat> m_hChatDialog;
	vgui::DHANDLE<CDialogAuthRequest> m_hAuthRequestDialog;
	vgui::DHANDLE<CDialogUserInfo> m_hUserInfoDialog;
	GameHandle_t m_hGameInfoDialog;
};


#endif // BUDDY_H
