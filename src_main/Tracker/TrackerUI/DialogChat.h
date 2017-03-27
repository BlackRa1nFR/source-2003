//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DIALOGCHAT_H
#define DIALOGCHAT_H
#pragma once

#include <VGUI_Frame.h>
#include <time.h>

namespace vgui
{
class TextEntry;
class Label;
class Button;
class MenuButton;
class TextImage;
enum KeyCode;
};

class CButtonInvite;
class CSubPanelBuddyList;

//-----------------------------------------------------------------------------
// Purpose: Two-way chat dialog for users
//			One instance per user
//-----------------------------------------------------------------------------
class CDialogChat : public vgui::Frame
{
public:
	CDialogChat(int userID);
	~CDialogChat();

	void Open(bool minimized);
	void Update(bool minimized);

	void AddMessageToHistory(bool self, const char *userName, const char *text);
	void AddTextToHistory(vgui::Color textColor, const char *text);

	void PlayNewMessageSound();

	// parses out URL's from a text message
	static int ParseTextStringForUrls(const char *text, int startPos, char *resultBuffer, int resultBufferSize, bool &clickable);

	// multi-user chat
	bool IsMultiUserChat();
	void InitiateMultiUserChat(int chatID);
	bool IsUserInChat(int userID);
	int  GetChatID();

	// network messages
	virtual void OnChatUserLeave(vgui::KeyValues *msg);
	virtual void OnMessage(vgui::KeyValues *msg);
	virtual void OnMessageFailed(vgui::KeyValues *msg);
	virtual void OnInvited();
	virtual void OnTypingMessage(vgui::KeyValues *params);

	void LoadChat(int buddyID); // load chat into dialog

private:
	// controls messages
	virtual void OnSendMessage();
	virtual void OnViewGameInfo();
	virtual void OnTextChanged(vgui::Panel *panel);
	virtual void OnTextClicked(const char *text);
	virtual void OnInviteUser(int userID);
	virtual void OnFriendsStatusChanged();
	virtual void OnDragDrop(vgui::KeyValues *data);
	virtual void OnCheckRemoteTypingTimeout();

	// Dialog overrides
	virtual void PerformLayout();
	virtual void OnClose();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual bool RequestInfo(vgui::KeyValues *outputData);

	// multi-user chat
	vgui::KeyValues *GetChatData(int friendID);

	void AddUserToChat(int userID, bool bPrintMessage);
	void SendMessageToAllUsers(int msgID, vgui::KeyValues *msg);
	void SendTextToAllUsers(const char *text);
	void OnChatAddUser(vgui::KeyValues *msg);

	DECLARE_PANELMAP();

	// data
	int m_iFriendID;
	vgui::Button *m_pSendButton;
	vgui::Button *m_pExpandButton;
	vgui::Label  *m_pNameLabel;
	vgui::Label  *m_pMessageState;
	vgui::TextEntry *m_pTextEntry;
	vgui::TextEntry *m_pChatHistory;
	CButtonInvite *m_pInviteButton;
	CSubPanelBuddyList *m_pBuddyList;

	vgui::TextImage *m_pStatusImage;

	vgui::Color m_ChatTextColor;
	vgui::Color m_ChatSelfTextColor;
	vgui::Color m_ChatSeperatorTextColor;
	vgui::Color m_URLTextColor;
	vgui::Color m_StatusColor;

	vgui::Dar<int> m_ChatUsers;

	int m_hSoundBeep;
	int m_iExpandSize;

	bool m_bMultiUserChat;
	bool m_bRemoteTyping;
	int m_iRemoteTypingTimeoutTimeMillis;
	int m_iChatID;
	struct tm m_LastMessageReceivedTime;
	bool m_bPostMessageRecievedTime;
	int m_iMessageNumber; // message counter for saving message history

	typedef vgui::Frame BaseClass;
};



#endif // DIALOGCHAT_H
