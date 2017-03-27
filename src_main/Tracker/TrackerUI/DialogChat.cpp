//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "Buddy.h"
#include "DialogChat.h"
#include "OnlineStatus.h"
#include "Random.h"
#include "ServerSession.h"
#include "SubPanelBuddyList.h"
#include "Tracker.h"
#include "TrackerDialog.h"
#include "TrackerDoc.h"
#include "TrackerProtocol.h"

#include <VGUI_Button.h>
#include <VGUI_KeyCode.h>
#include <VGUI_Menu.h>
#include <VGUI_MenuButton.h>
#include <VGUI_TextEntry.h>
#include <VGUI_TextImage.h>
#include <VGUI_IPanel.h>

#include <VGUI_Controls.h>
#include <VGUI_IInput.h>
#include <VGUI_IScheme.h>
#include <VGUI_ISurface.h>
#include <VGUI_ISystem.h>

#include <VGUI_ILocalize.h>

#include <ctype.h>
#include <stdio.h>

#define min(a,b)            (((a) < (b)) ? (a) : (b))

using namespace vgui;

#define REMOTETYPING_TIMEOUT_TIME_SECONDS 20	// 20 second timeout for when we Think a user is typing a message

//-----------------------------------------------------------------------------
// Purpose: Displays a list of friends to invite
//-----------------------------------------------------------------------------
class CButtonInvite : public vgui::MenuButton
{
public:
	CButtonInvite(Panel *parent, const char *panelName, CDialogChat *chatDialog) : MenuButton(parent, panelName, "Invite")
	{
		m_pChatDialog = chatDialog;
		m_pMenu = new Menu(NULL, NULL);
		m_pMenu->SetNumberOfVisibleItems(15); 
		m_pMenu->SetVisible(false);
		SetMenu(m_pMenu);
		
		SetText("#TrackerUI_Invite");
		
		m_pDropdownText = new TextImage("u");
		AddImage(m_pDropdownText, 2);
		
		SetContentAlignment(Label::a_center);
	}
	
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme)
	{
		MenuButton::ApplySchemeSettings(pScheme);
		
		m_pDropdownText->SetFont(scheme()->GetFont(scheme()->GetDefaultScheme(), "Marlett"));
		m_pDropdownText->SetColor(GetFgColor());
	}
	
	virtual void OnShowMenu(Menu *menu)
	{
		if (!m_pMenu->IsPopup())
		{
			surface()->CreatePopup(m_pMenu->GetVPanel(), false);
		}
		
		// recalculate the items to show in the menu
		menu->ClearMenu();
		
		// temporary buffer, for sorting
		Dar<int> friendDar;
		
		// loop through the buddies in the doc
		KeyValues *buddyData = GetDoc()->Data()->FindKey("User/Buddies", true);
		for (KeyValues *dat = buddyData->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
		{
			// get the buddies ID
			unsigned int buddyID = (unsigned int)atoi(dat->GetName());
			if (!buddyID)
				continue;
			
			// don't let the user invite themselves
			if (buddyID == GetDoc()->GetUserID())
				continue;
			
			// check to see if they're already in the chat
			if (m_pChatDialog->IsUserInChat(buddyID))
				continue;
			
			// make sure they're online
			if (dat->GetInt("status") <= COnlineStatus::OFFLINE)
				continue;
			
			// insert in alphabetical order
			bool bInserted = false;
			const char *newName = dat->GetString("DisplayName", NULL);
			if (!newName || !newName[0])
			{
				newName = dat->GetString("UserName", NULL);
			}
			if (!newName)
				continue;
			
			for (int i = 0; i < friendDar.GetCount(); i++)
			{
				const char *buddyName = GetDoc()->GetBuddy(friendDar[i])->DisplayName();
				if (stricmp(newName, buddyName) < 0)
				{
					friendDar.InsertElementAt(buddyID, i);
					bInserted = true;
					break;
				}
			}
			if (!bInserted)
			{
				friendDar.PutElement(buddyID);
			}
		}
		
		// add the user to the drop down
		for (int i = 0; i < friendDar.GetCount(); i++)
		{
			char buf[512];
			sprintf(buf, " %s ", GetDoc()->GetBuddy(friendDar[i])->DisplayName());
			menu->AddMenuItem(buf, new KeyValues("InviteUser", "userID", friendDar[i]), m_pChatDialog);
		}
	}
	
	// populates dropdown and returns the number of items in it
	int populateMenu()
	{					   	
		if (!m_pMenu)
			return 0;
		OnShowMenu(m_pMenu);
		return m_pMenu->GetNumberOfCurrentlyVisibleItems();
	}
	
	void SetEnabled(bool state)
	{
		m_pMenu->SetEnabled(state);
		MenuButton::SetEnabled(state);
	}
	
private:
	CDialogChat *m_pChatDialog;
	Menu *m_pMenu;
	TextImage *m_pDropdownText;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDialogChat::CDialogChat(int userID) : Frame(NULL, "ChatDialog")
{
	SetBounds(0, 0, 320, 240);
	
	m_iFriendID = userID;
	m_ChatUsers.AddElement(m_iFriendID);
	m_bMultiUserChat = false;
	m_bRemoteTyping = false;
	m_iExpandSize = 144;
	m_iChatID = 0;
	m_iRemoteTypingTimeoutTimeMillis = 0;
	memset(&m_LastMessageReceivedTime, 0, sizeof(m_LastMessageReceivedTime));
	
	m_pStatusImage = new TextImage("");
	
	m_pSendButton = new Button(this, "SendButton", "#TrackerUI_Send");
	m_pNameLabel = new Label(this, "NameLabel", "");
	m_pMessageState = new Label(this, "MessageState", "");
	m_pTextEntry = new TextEntry(this, "TextEntry");
	m_pChatHistory = new TextEntry(this, "ChatHistory");
	m_pInviteButton = new CButtonInvite(this, "InviteButton", this);
	m_pBuddyList = NULL;	// buddylist is created when used
	m_bPostMessageRecievedTime = false;

	LoadControlSettings("Friends/DialogChat.res");

	// disable Invite button if there are no entries in menulist
	if ( !m_pInviteButton->populateMenu() )
	{
		m_pInviteButton->SetEnabled(false);
	}
	
	m_pTextEntry->SetMultiline(true);
	m_pTextEntry->setMaximumCharCount(500);
	m_pChatHistory->SetMultiline(true);
	m_pChatHistory->SetEditable(false);
	m_pChatHistory->SetRichEdit(true);
	m_pChatHistory->SetVerticalScrollbar(true);
	m_pChatHistory->SetEnabled(false);
	
	m_pSendButton->SetCommand(new KeyValues("SendMessage"));
	m_pMessageState->SetFont(scheme()->GetFont(scheme()->GetDefaultScheme(), "DefaultSmall"));
	
	SetSize(400, 300);
	
	SetMinimumSize(240, 180);
	
	GetDoc()->LoadDialogState(this, "Chat", m_iFriendID);
	
	ServerSession().AddNetworkMessageWatch(this, TCL_MESSAGE);
	ServerSession().AddNetworkMessageWatch(this, TSVC_MESSAGE);
	ServerSession().AddNetworkMessageWatch(this, TCL_MESSAGE_FAIL);
	ServerSession().AddNetworkMessageWatch(this, TSVC_FRIENDS);
	ServerSession().AddNetworkMessageWatch(this, TCL_CHATADDUSER);
	ServerSession().AddNetworkMessageWatch(this, TCL_CHATUSERLEAVE);
	ServerSession().AddNetworkMessageWatch(this, TCL_TYPINGMESSAGE);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogChat::~CDialogChat()
{
	ServerSession().RemoveNetworkMessageWatch(this);
	
	if (CTrackerDialog::GetInstance())
	{
		CTrackerDialog::GetInstance()->SetChatDialogInList(this, true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Walks the list looking to see if the user is in this chat
// Input  : userID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDialogChat::IsUserInChat(int userID)
{
	for (int i = 0; i < m_ChatUsers.GetCount(); i++)
	{
		if (m_ChatUsers[i] == userID)
			return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDialogChat::IsMultiUserChat()
{
	return m_bMultiUserChat;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CDialogChat::GetChatID()
{
	return m_iChatID;
}


//-----------------------------------------------------------------------------
// Purpose: Turns the single-user chat dialog into a multi-user chat session
// Input  : chatID - 
//-----------------------------------------------------------------------------
void CDialogChat::InitiateMultiUserChat(int chatID)
{
	m_bMultiUserChat = true;
	m_iChatID = chatID;
	
	if (m_iChatID == 0)
	{
		// this is a new chat, generate a new random chatID
		m_iChatID = RandomLong(1, MAX_RANDOM_RANGE);
		
		// notify our friend that we're going into a multi-user chat
		KeyValues *msg = new KeyValues("AddedToChat");
		msg->SetInt("UID", GetDoc()->GetUserID());
		msg->SetInt("ChatID", m_iChatID);
		ServerSession().SendKeyValuesMessage(m_iFriendID, TCL_ADDEDTOCHAT, msg);
		msg->deleteThis();
		
		AddUserToChat(m_iFriendID, false);
	}
	
	// disassociate this chat dialog from the friend
	CBuddy *buddy = GetDoc()->GetBuddy(m_iFriendID);
	if (buddy)
	{
		buddy->DisassociateChatDialog();
	}
	
	// add to the chat dialog list
	CTrackerDialog::GetInstance()->SetChatDialogInList(this, false);
}

//-----------------------------------------------------------------------------
// Purpose: invites a user into the current chat
// Input  : userID - 
//-----------------------------------------------------------------------------
void CDialogChat::OnInviteUser(int userID)
{
	// make sure they're online	
	KeyValues *dat = GetChatData(userID);		
	if (dat->GetInt("status") != COnlineStatus::ONLINE)
		return;
	
	if (!m_bMultiUserChat)
	{
		// Start up multi-user chat
		InitiateMultiUserChat(0);
	}
	
	// copy data into chat data area
	// notify other users that the user has been added
	KeyValues *msg = new KeyValues("ChatAddUser", "FriendID", userID);
	
	msg->SetString("UserName", dat->GetString("UserName"));
	msg->SetInt("IP", dat->GetInt("IP"));
	msg->SetInt("Port", dat->GetInt("Port"));
	
	SendMessageToAllUsers(TCL_CHATADDUSER, msg);
	
	// send message inviting user into chat
	msg = new KeyValues("AddedToChat");
	msg->SetInt("UID", GetDoc()->GetUserID());
	msg->SetInt("ChatID", m_iChatID);
	ServerSession().SendKeyValuesMessage(userID, TCL_ADDEDTOCHAT, msg);
	msg->deleteThis();
	
	// send a message to the added user, telling them about all the users that are already in the chat
	msg = new KeyValues("ChatAddUser");
	msg->SetInt("silent", 1); // indicate that the user joining should be a silent action
	msg->SetInt("UID", GetDoc()->GetUserID());
	msg->SetInt("ChatID", m_iChatID);
	for (int i = 0; i < m_ChatUsers.GetCount(); i++)
	{
		dat = GetChatData(m_ChatUsers[i]);
		msg->SetInt("FriendID", m_ChatUsers[i]);
		msg->SetString("UserName", dat->GetString("DisplayName", dat->GetString("UserName")));
		msg->SetInt("IP", dat->GetInt("IP"));
		msg->SetInt("Port", dat->GetInt("Port"));
		
		// send to the new user
		ServerSession().SendKeyValuesMessage(userID, TCL_CHATADDUSER, msg);
	}
	msg->deleteThis();
	
	// print out that the user has been added
	AddUserToChat(userID, true);
	
	// reset the focus
	m_pTextEntry->RequestFocus();
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Chat data
//-----------------------------------------------------------------------------
KeyValues *CDialogChat::GetChatData(int friendID)
{
	CBuddy *buddy = GetDoc()->GetBuddy(friendID);
	if (!buddy)
		return NULL;
	
	return buddy->Data();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the user in the current chat, so they get copied on all future messages
// Input  : userID - 
//-----------------------------------------------------------------------------
void CDialogChat::AddUserToChat(int userID, bool bPrintMessage)
{
	if (m_ChatUsers.HasElement(userID))
		return;
	
	m_ChatUsers.AddElement(userID);
	if (bPrintMessage)
	{
		KeyValues *dat = GetChatData(userID);
		AddTextToHistory(GetFgColor(), dat->GetString("DisplayName", dat->GetString("UserName")));
		AddTextToHistory(GetFgColor(), " has been added to the conversation\n");
	}
	
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a user has been added into the chat
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CDialogChat::OnChatAddUser(vgui::KeyValues *msg)
{
	if (!m_bMultiUserChat)
	{
		// make sure the message is from us
		int fromID = msg->GetInt("UID");
		if (fromID != m_iFriendID)
			return;
		
		// Start multi chat
		InitiateMultiUserChat(msg->GetInt("ChatID"));
	}
	
	int friendID = msg->GetInt("FriendID");
	if (!GetDoc()->GetBuddy(friendID))
	{
		// user is unknown to us;  add them to our list
		char buf[24];
		sprintf(buf, "%d", friendID);
		KeyValues *budData = new KeyValues(buf);
		budData->SetString("UserName", msg->GetString("UserName"));
		budData->SetString("DisplayName", msg->GetString("UserName"));
		budData->SetInt("UID", friendID);
		budData->SetInt("Status", COnlineStatus::CHATUSER);
		budData->SetInt("IP", msg->GetInt("IP"));
		budData->SetInt("Port", msg->GetInt("Port"));
		GetDoc()->AddNewBuddy(budData);
	}
	
	bool bPrintMessage = !msg->GetInt("silent");
	AddUserToChat(friendID, bPrintMessage);
}

//-----------------------------------------------------------------------------
// Purpose: Called when a user leaves a multi-chat
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CDialogChat::OnChatUserLeave(KeyValues *msg)
{
	int chatID = msg->GetInt("ChatID");
	int friendID = msg->GetInt("UID");
	
	// make sure it's the right chat
	if (!chatID || chatID != m_iChatID)
		return;
	
	// print message indicating the user has left
	AddTextToHistory(GetFgColor(), GetDoc()->GetBuddy(friendID)->DisplayName());
	AddTextToHistory(GetFgColor(), " has left the conversation\n");
	
	// remove the user
	m_ChatUsers.RemoveElement(friendID);
	
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user has been invited into a chat
//-----------------------------------------------------------------------------
void CDialogChat::OnInvited()
{
	// print that we have been invited into the chat
	AddTextToHistory(GetFgColor(), "You have been added to the conversation\n");
}

//-----------------------------------------------------------------------------
// Purpose: Called when a message is received
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CDialogChat::OnMessage(KeyValues *msg)
{
	bool fromServer = false;
	if (msg->GetInt("_id") == TSVC_MESSAGE)
	{
		fromServer = true;
	}
	
	int fromID = msg->GetInt("uid");
	if (m_iChatID && msg->GetInt("ChatID") == m_iChatID)
	{
		// it's a multi-chat message
		// add the message
		AddMessageToHistory(false, GetDoc()->GetBuddy(fromID)->DisplayName(), msg->GetString("text"));
	}
	
	if (!fromServer)
	{
		CBuddy *buddy = ::GetDoc()->GetBuddy(fromID);
		if (buddy && buddy->SendViaServer())
		{
			// we received a message directly from this user; next time try and send one directly back
			// it may be that our initial attempt failed because they are using Internet Connection Sharing
			// or some firewall that won't allow incoming connections until the outgoing is established
			buddy->SetSendViaServer(false);
		}
	}
	
	m_bRemoteTyping = false;
	
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Handles a message that couldn't be delivered to the target
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CDialogChat::OnMessageFailed(KeyValues *msg)
{
	// make sure the message was from this dialog
	int targetID = msg->GetInt("targetID");
	if (m_iChatID && msg->GetInt("ChatID") != m_iChatID)
	{
		// message is to wrong multiuserchat
		return;
	}
	else if (m_iFriendID != targetID)
	{
		// message is to different user
		return;
	}
	
	CBuddy *buddy = GetDoc()->GetBuddy(targetID);
	if (!buddy)
		return;
	
	// mark the user as needing to be sent to via server
	buddy->SetSendViaServer(true);
	
	if (msg->GetInt("_id") == TCL_MESSAGE)
	{
		// resend the message via the server
		ServerSession().SendTextMessageToUser(targetID, msg->GetString("text"), m_iChatID);
	}
	
	// print out that the message could not be delivered - no longer needed
	//	AddTextToHistory(GetSchemeColor("BrightBaseText", Color(255, 255, 255, 0)), "Message was not successfully delivered.\n");
}

//-----------------------------------------------------------------------------
// Purpose: Recalculates the layout of the text dialog
//-----------------------------------------------------------------------------
void CDialogChat::PerformLayout()
{
	// title
	// friend list
	char friendsNames[512];
	m_pStatusImage->SetText("");
	if (m_bMultiUserChat)
	{
		friendsNames[0] = 0;
		for (int i = 0; i < m_ChatUsers.GetCount(); i++)
		{
			if (i > 0)
			{
				strcat(friendsNames, ", ");
				
				// don't display more than 5 users
				if (i > 5)
				{
					strcat(friendsNames, "...");
					break;
				}
			}
			strcat(friendsNames, GetDoc()->GetBuddy(m_ChatUsers[i])->DisplayName());
		}
	}
	else
	{
		strcpy(friendsNames, GetDoc()->GetBuddy(m_iFriendID)->DisplayName());
		int friendStatus = GetDoc()->GetBuddy(m_iFriendID)->Data()->GetInt("Status");
		
		m_pStatusImage->SetColor(m_StatusColor);
		if (friendStatus == COnlineStatus::INGAME)
		{
			m_pStatusImage->SetText("#TrackerUI_InGameParanthesis");
		}
		else if (friendStatus == COnlineStatus::AWAY)
		{
			m_pStatusImage->SetText("#TrackerUI_AwayParanthesis");
		}
		else if (friendStatus == COnlineStatus::SNOOZE)
		{
			m_pStatusImage->SetText("#TrackerUI_SnoozingParanthesis");
		}
		else if (friendStatus == COnlineStatus::BUSY)
		{
			m_pStatusImage->SetText("#TrackerUI_BusyParanthesis");
		}
		else if (friendStatus <= COnlineStatus::OFFLINE)
		{
			m_pStatusImage->SetText("#TrackerUI_OfflineParanthesis");
		}
	}

	char buf2[512];
	sprintf(buf2, "To: %s", friendsNames);
	m_pNameLabel->SetText(buf2);
	m_pNameLabel->SetImageAtIndex(1, m_pStatusImage, 6);
	
	// set the title	
	sprintf(buf2, "%s - Instant Message", friendsNames);
	SetTitle(buf2, true);

//tagES
//	wchar_t unicode[512], unicodeNames[512];
//	int iUnicodeSize = sizeof( unicode ) / sizeof( wchar_t );

//	localize()->ConvertANSIToUnicode(friendsNames, unicodeNames, sizeof( unicodeNames ) / sizeof( wchar_t ));
//	localize()->ConstructString(unicode, iUnicodeSize, localize()->Find("#TrackerUI_To_Name"), 1, unicodeNames);

//	m_pNameLabel->SetText(unicode);
//	m_pNameLabel->SetImageAtIndex(1, m_pStatusImage, 6);
	
	// set the title	
//	localize()->ConstructString(unicode, iUnicodeSize, localize()->Find("#TrackerUI_Name_InstantMessageTitle"), 1, unicodeNames);
//	SetTitle(unicode, true);
	
	// set the message status text
	if (m_bRemoteTyping && !m_bMultiUserChat)
	{
		sprintf(buf2, "%s is typing a message.", friendsNames);
		m_pMessageState->SetText(buf2);

//tagES
//		localize()->ConstructString(unicode, iUnicodeSize, localize()->Find("#TrackerUI_Name_IsTypingAMessage"), 1, unicodeNames);
//		m_pMessageState->SetText(unicode);
	}
	// set the last message received time
	else if (m_bPostMessageRecievedTime)
	{
//tagES
		char timeString[64];
		strftime(timeString, sizeof(timeString)-4, "Message last received: %A %b %d %I:%M", &m_LastMessageReceivedTime);
		
		// append am/pm in lower case (since strftime doesn't have a lowercase formatting option)
		if (m_LastMessageReceivedTime.tm_hour > 12)
		{
			strcat(timeString, "pm");
		}
		else
		{
			strcat(timeString, "am");
		}
		m_pMessageState->SetText(timeString);
	}
	else
	{
		m_pMessageState->SetText("");
	}
	
	// enable or disable Invite button as appropriate
	if ( m_pInviteButton->IsEnabled() )
	{
		// disable Invite button if there are no entries in menulist
		if ( !m_pInviteButton->populateMenu() )
		{
			m_pInviteButton->SetEnabled(false);
		}
	}
	// enable Invite button if there are entries in menulist
	else if ( !m_pInviteButton->IsEnabled() )
	{	
		if ( m_pInviteButton->populateMenu() )
		{
			m_pInviteButton->SetEnabled(true);
		}
	}
	
	
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for when text in the TextEntry changes
//-----------------------------------------------------------------------------
void CDialogChat::OnTextChanged(Panel *panel)
{
	if (panel == m_pTextEntry)
	{
		// send a message to the friend if we've just starting typing a message
		bool oldState = m_pSendButton->IsEnabled();
		
		char text[256];
		m_pTextEntry->GetText(0, text, sizeof(text)-1);
		if (strlen(text) > 0)
		{
			m_pSendButton->SetEnabled(true);
		}
		else
		{
			m_pSendButton->SetEnabled(false);
		}
		
		// send our state to the other Client if it's changed
		if (m_pSendButton->IsEnabled() && oldState != m_pSendButton->IsEnabled())
		{
			KeyValues *msg = new KeyValues("TypingMessage");
			msg->SetInt("state", 1);
			msg->SetInt("ChatID", m_iChatID);
			
			SendMessageToAllUsers(TCL_TYPINGMESSAGE, msg);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens the dialog and shows it on the screen
//-----------------------------------------------------------------------------
void CDialogChat::Open(bool minimized)
{
	Activate();
	m_iMessageNumber=0;
	SetFgColor(GetSchemeColor("WindowFgColor"));
	m_ChatTextColor = GetSchemeColor("Chat/TextColor");
	m_ChatSelfTextColor = GetSchemeColor("Chat/SelfTextColor");
	m_ChatSeperatorTextColor = GetSchemeColor("Chat/SeperatorTextColor");
	m_URLTextColor = GetSchemeColor("URLTextColor");
	
	SetVisible(minimized);
	m_pTextEntry->RequestFocus();
	m_pTextEntry->SetText("");
	OnTextChanged(m_pTextEntry);
	GetFocusNavGroup().SetDefaultButton(m_pSendButton);
	
	if (!minimized)
	{
		if (GetDoc()->Data()->GetInt("User/ChatAlwaysOnTop", 0))
		{
			surface()->SetAsTopMost(GetVPanel(), true);
		}
		else
		{
			surface()->SetAsTopMost(GetVPanel(), false);
		}
		
		RequestFocus();
	}
	
	Update(minimized);
	
	if (minimized)
	{
		PlayNewMessageSound();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Plays the new message sound
//-----------------------------------------------------------------------------
void CDialogChat::PlayNewMessageSound()
{
	// play a sound
	if (GetDoc()->Data()->GetInt("User/Sounds/Message", 1))
	{
		surface()->PlaySound("Friends\\Message.wav");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the text box with the latest received messages, if any
// Input  : minimized - 
//-----------------------------------------------------------------------------
void CDialogChat::Update(bool minimized)
{
	if (!minimized)
	{
		// force the dialog visible
		SetVisible(true);
		surface()->SetMinimized(this->GetVPanel(), false);
		MoveToFront();
		m_pTextEntry->RequestFocus();
	}
	
	// get the latest text
	bool bGotNewMessage = false;
	while (1)
	{
		KeyValues *msg = GetDoc()->GetFirstMessage(m_iFriendID);
		if (!msg)
			break;
		
		// fill the panel with the message text
		const char *text = msg->GetString("text");
		AddMessageToHistory(false, GetDoc()->GetBuddy(m_iFriendID)->DisplayName(), text);
		
		// move the message into the history
		GetDoc()->MoveMessageToHistory(m_iFriendID, msg);
		
		// play the message sound if the chat window doesn't have focus, or it is not visible, or it's parent is not visible
		VPanel *focus = input()->GetFocus();
		if (!(focus && ipanel()->HasParent(focus, this->GetVPanel())) || (!IsVisible()) || (GetParent() && !GetParent()->IsVisible()))
		{
			PlayNewMessageSound();
			
			// flash the window
			FlashWindow();
		}
		
		bGotNewMessage = true;
	}
	
	// record the time the message was received
	if (bGotNewMessage)
	{
		time_t timer;
		time(&timer);
		
		struct tm *timestruct = localtime(&timer);
		if (timestruct)
		{
			m_LastMessageReceivedTime = *timestruct;
			m_bPostMessageRecievedTime = true;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sends message to the user
//-----------------------------------------------------------------------------
void CDialogChat::OnSendMessage()
{
	char buf[512];
	m_pTextEntry->GetText(0, buf, 511);
	if (!buf[0])
		return;
	
	// make sure we're still online
	if (ServerSession().GetStatus() <= COnlineStatus::OFFLINE)
	{
		// print an error
		// move to the end of the history
		m_pChatHistory->DoGotoTextEnd();
		
		// add text
		m_pChatHistory->DoInsertString("\n");
		m_pChatHistory->DoInsertIndentChange(2);
		m_pChatHistory->DoInsertColorChange(m_pChatHistory->GetFgColor());
		m_pChatHistory->DoInsertString("#TrackerUI_Offline_MessageNotDelivered");
	}
	// make sure the target user is still online
	else if (!m_bMultiUserChat && GetDoc()->GetBuddy(m_iFriendID)->Data()->GetInt("Status") <= COnlineStatus::OFFLINE)
	{
		// print an error
		// move to the end of the history
		m_pChatHistory->DoGotoTextEnd();
		
		// add text
		m_pChatHistory->DoInsertString("\n");
		m_pChatHistory->DoInsertIndentChange(2);
		m_pChatHistory->DoInsertColorChange(GetSchemeColor("BrightBaseText", Color(255, 255, 255, 0)));
		m_pChatHistory->DoInsertString(GetDoc()->GetBuddy(m_iFriendID)->DisplayName());
		m_pChatHistory->DoInsertString(" has gone offline.\nMessage could not be delivered.\n");

//tagES
//		wchar_t unicode[256], unicodeName[64];
//		localize()->ConvertANSIToUnicode(GetDoc()->GetBuddy(m_iFriendID)->DisplayName(), unicodeName, sizeof( unicodeName ) / sizeof( wchar_t ));
//		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_BuddyOffline_MessageNotDelivered"), 1, unicodeName);
//		m_pChatHistory->DoInsertString(unicode);
	}
	else
	{
		// show text in chat window
		AddMessageToHistory(true, GetDoc()->Data()->GetString("User/UserName"), buf);
		
		// send out the message
		SendTextToAllUsers(buf);
	}
	
	// clear the window
	m_pTextEntry->SetText("");
	m_pSendButton->SetEnabled(false);
	
	// reset the focus
	m_pTextEntry->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to everyone in the chat
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CDialogChat::SendMessageToAllUsers(int msgID, KeyValues *msg)
{
	// set up the message
	msg->SetInt("UID", GetDoc()->GetUserID());
	
	// let the user know our current status
	msg->SetInt("Status", ServerSession().GetStatus());
	
	if (m_bMultiUserChat)
	{
		msg->SetInt("ChatID", m_iChatID);
		
		for (int i = 0; i < m_ChatUsers.GetCount(); i++)
		{
			// write into the message who it belongs to
			msg->SetInt("targetID", m_ChatUsers[i]);
			
			ServerSession().SendKeyValuesMessage(m_ChatUsers[i], msgID, msg);
		}
	}
	else
	{
		// send the message down the wire
		// ServerSession().SendTextMessageToUser(m_iFriendID, msg->GetString("text"));
		msg->SetInt("targetID", m_iFriendID);
		ServerSession().SendKeyValuesMessage(m_iFriendID, msgID, msg);
	}
	
	msg->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Sends text message to everyone in chat, sending via server if necessary
// Input  : *text - 
//-----------------------------------------------------------------------------
void CDialogChat::SendTextToAllUsers(const char *text)
{
	if (m_bMultiUserChat)
	{
		// send message to everyone in chat
		for (int i = 0; i < m_ChatUsers.GetCount(); i++)
		{
			ServerSession().SendTextMessageToUser(m_ChatUsers[i], text, m_iChatID);
		}
	}
	else
	{
		// send the message to the current user
		ServerSession().SendTextMessageToUser(m_iFriendID, text, 0);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Moves a message into the chat history
// Input  : *userName - 
//			*text - 
//-----------------------------------------------------------------------------
void CDialogChat::AddMessageToHistory(bool self, const char *userName, const char *text)
{
	// move to the end of the history
	m_pChatHistory->DoGotoTextEnd();
	
	// add text
	m_pChatHistory->DoInsertColorChange(m_ChatSeperatorTextColor);
	m_pChatHistory->DoInsertString(userName);
	m_pChatHistory->DoInsertString(" says:");
	m_pChatHistory->DoInsertChar('\n');
	m_pChatHistory->DoInsertIndentChange(12);
	
	Color currentColor;
	
	if (self)
	{
		currentColor = m_ChatSelfTextColor;
	}
	else
	{
		currentColor = m_ChatTextColor;
	}
	
	m_pChatHistory->DoInsertColorChange(currentColor);
	
	// parse out the string for URL's
	int len = strlen(text), pos = 0;
	bool clickable = false;
	char resultBuf[512];
	while (pos < len)
	{
		pos = ParseTextStringForUrls(text, pos, resultBuf, sizeof(resultBuf)-1, clickable);
		
		if (clickable)
		{
			m_pChatHistory->DoInsertClickableTextStart();
			m_pChatHistory->DoInsertColorChange(m_URLTextColor);
		}
		
		m_pChatHistory->DoInsertString(resultBuf);
		
		if (clickable)
		{
			m_pChatHistory->DoInsertColorChange(currentColor);
			m_pChatHistory->DoInsertClickableTextEnd();
		}
	}
	
	m_pChatHistory->DoInsertIndentChange(0);
	m_pChatHistory->DoInsertChar('\n');
	
	// log the chat
	KeyValues *data;
	data = GetDoc()->GetBuddy(m_iFriendID)->Data()->FindKey("ActiveChat", true);
	char keyLabel[30]; 
	sprintf (keyLabel, "ChatUser%d", m_iMessageNumber);
	if (self)
		data->SetInt(keyLabel, GetDoc()->GetUserID());
	else
		data->SetInt(keyLabel, m_iFriendID);
	
	sprintf (keyLabel, "ChatText%d", m_iMessageNumber);
	data->SetString(keyLabel, text);
	m_iMessageNumber++;	
}

//-----------------------------------------------------------------------------
// Purpose: Adds a simple string of colored text to the chat history
// Input  : textColor - 
//			*text - 
//-----------------------------------------------------------------------------
void CDialogChat::AddTextToHistory(Color textColor, const char *text)
{
	// move to the end of the history
	m_pChatHistory->DoGotoTextEnd();
	
	// add text
	m_pChatHistory->DoInsertColorChange(textColor);
	m_pChatHistory->DoInsertString(text);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
//			startPos - 
//			*resultBuffer - 
//			resultBufferSize - 
//			&clickable - 
// Output : int
//-----------------------------------------------------------------------------
int CDialogChat::ParseTextStringForUrls(const char *text, int startPos, char *resultBuffer, int resultBufferSize, bool &clickable)
{
	// scan for text that looks like a URL
	int i = startPos;
	while (text[i] != 0)
	{
		bool bURLFound = false;
		
		if (!_strnicmp(text + i, "www.", 4))
		{
			// scan ahead for another '.'
			bool bPeriodFound = false;
			for (const char *ch = text + i + 5; ch != 0; ch++)
			{
				if (*ch == '.')
				{
					bPeriodFound = true;
					break;
				}
			}
			
			// URL found
			if (bPeriodFound)
			{
				bURLFound = true;
			}
		}
		else if (!_strnicmp(text + i, "http://", 7))
		{
			bURLFound = true;
		}
		else if (!_strnicmp(text + i, "ftp://", 6))
		{
			bURLFound = true;
		}
		else if (!_strnicmp(text + i, "mailto:", 7))
		{
			bURLFound = true;
		}
		else if (!_strnicmp(text + i, "\\\\", 2))
		{
			bURLFound = true;
		}
		
		if (bURLFound)
		{
			if (i == startPos)
			{
				// we're at the Start of a URL, so parse that out
				clickable = true;
				int outIndex = 0;
				while (text[i] != 0 && !isspace(text[i]))
				{
					resultBuffer[outIndex++] = text[i++];
				}
				resultBuffer[outIndex] = 0;
				return i;
			}
			else
			{
				// no url
				break;
			}
		}
		
		// increment and loop
		i++;
	}
	
	// nothing found;
	// parse out the text before the end
	clickable = false;
	int outIndex = 0;
	int fromIndex = startPos;
	while (fromIndex < i)
	{
		resultBuffer[outIndex++] = text[fromIndex++];
	}
	resultBuffer[outIndex] = 0;
	return i;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the web browser with the text
// Input  : *text - 
//-----------------------------------------------------------------------------
void CDialogChat::OnTextClicked(const char *text)
{
	system()->ShellExecute("open", text); 
}

//-----------------------------------------------------------------------------
// Purpose: Deletes the chat dialog after it has been closed
//-----------------------------------------------------------------------------
void CDialogChat::OnClose()
{
	if (!m_bMultiUserChat)
	{
		GetDoc()->SaveDialogState(this, "Chat", m_iFriendID);
	}
	else
	{
		// send message to friends saying that we're leaving chat
		KeyValues *msg = new KeyValues("ChatUserLeave");
		SendMessageToAllUsers(TCL_CHATUSERLEAVE, msg);
	}
	
	KeyValues *data = GetDoc()->GetBuddy(m_iFriendID)->Data();
	if (data)
	{
		KeyValues *messages = data->FindKey("ActiveChat", false);	
		if (messages)	
			data->RemoveSubKey(messages);
	}
	
	BaseClass::OnClose();
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *inResourceData - 
//-----------------------------------------------------------------------------
void CDialogChat::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetFgColor(GetSchemeColor("WindowFgColor"));
	m_StatusColor = GetSchemeColor("StatusSelectFgColor2");
	
	// force the chat history scheme settings to be loaded, so we can stomp them
	m_pChatHistory->InvalidateLayout(true);
	
	// non-standard text border
	m_pChatHistory->SetBorder(scheme()->GetBorder(GetScheme(), "BaseBorder"));
	m_pChatHistory->SetDisabledBgColor(GetSchemeColor("ChatBgColor", GetSchemeColor("WindowDisabledBgColor")));
}

//-----------------------------------------------------------------------------
// Purpose: Opens the game info dialog regarding the current chatted-to user
//-----------------------------------------------------------------------------
void CDialogChat::OnViewGameInfo()
{
	GetDoc()->GetBuddy(m_iFriendID)->OpenGameInfoDialog(false);
}


//-----------------------------------------------------------------------------
// Purpose: Called when friends' status is updated, to update the screen
//-----------------------------------------------------------------------------
void CDialogChat::OnFriendsStatusChanged( void )
{
	if (m_pBuddyList)
	{
		m_pBuddyList->InvalidateAll();
	}
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : uid - 
//-----------------------------------------------------------------------------
void CDialogChat::OnTypingMessage(KeyValues *params)
{
	int uid = params->GetInt("UID");
	int state = params->GetInt("state");
	int chatID = params->GetInt("chatID");
	
	// message is for a different chat
	if (chatID != m_iChatID)
		return;
	
	// message if for a different friend
	if (m_iChatID == 0 && uid != m_iFriendID)
		return;
	
	// post a message to check for the typing state to timeout
	PostMessage(this, new KeyValues("CheckRemoteTypingTimeout"), REMOTETYPING_TIMEOUT_TIME_SECONDS);
	m_iRemoteTypingTimeoutTimeMillis = system()->GetTimeMillis() + ((REMOTETYPING_TIMEOUT_TIME_SECONDS - 1) * 1000);
	
	m_bRemoteTyping = state;
	OnFriendsStatusChanged();
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if we should clear the 'friend is typing message' status
//-----------------------------------------------------------------------------
void CDialogChat::OnCheckRemoteTypingTimeout()
{
	if (!m_iRemoteTypingTimeoutTimeMillis)
		return;
	
	int timeRemaining = m_iRemoteTypingTimeoutTimeMillis - system()->GetTimeMillis();
	if (timeRemaining <= 0)
	{
		m_bRemoteTyping = false;
	}
	
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *outputData - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDialogChat::RequestInfo(KeyValues *outputData)
{
	if (!stricmp("DragDrop", outputData->GetName()))
	{
		// drag drop
		const char *type = outputData->GetString("type");
		if (!stricmp(type, "TrackerFriend")	|| !stricmp(type, "Files") || !stricmp(type, "Text"))
		{
			// we accept all tracker friends, files, and text
			// write in the ptr to this panel, so the final message is known where to sent
			outputData->SetPtr("AcceptPanel", (Panel *)this);
			return true;
		}
	}
	
	return BaseClass::RequestInfo(outputData);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *data - 
//-----------------------------------------------------------------------------
void CDialogChat::OnDragDrop(vgui::KeyValues *data)
{
	if (!stricmp(data->GetString("type"), "TrackerFriend"))
	{		
		int friendID = data->GetInt("friendID");
		
		// add the friend to the chat
		if (friendID)
		{	
			OnInviteUser(friendID);
		}
	}
	else if (!stricmp(data->GetString("type"), "Files"))
	{
		m_pTextEntry->DoGotoTextEnd();
		m_pTextEntry->DoInsertString(data->GetString("list/0"));
	}
	else if (!stricmp(data->GetString("type"), "Text"))
	{
		m_pTextEntry->DoGotoTextEnd();
		m_pTextEntry->DoInsertString(data->GetString("text"));
	}
}


void CDialogChat::LoadChat(int buddyID)
{
	if (m_iMessageNumber != 0) // we already loaded it
		return;
	
	KeyValues *data;
	data = GetDoc()->GetBuddy(buddyID)->Data()->FindKey("ActiveChat", false);	
	if (!data)
		return;
	
	int i=0;
	char keyLabel[30];
	char msg[512];
	const char *buf;
	while (1)
	{
		sprintf (keyLabel, "ChatUser%d", i);
		int user = data->GetInt(keyLabel, -2);
		if (user == -2)
			break;
		sprintf (keyLabel, "ChatText%d", i);
		buf = data->GetString(keyLabel);
		strcpy(msg, buf); // have to copy it over because message logging wipes the keyvalues when they are set.
		if (!buf)
			return;
		if (user == (int)GetDoc()->GetUserID())
		{
			char userName[30];
			strcpy (userName, GetDoc()->Data()->GetString("User/UserName"));
			// if for some reason this is not set we'll use the word 'You'
			if (userName == "")
				strcpy (userName, "You");
			AddMessageToHistory(true, userName, msg);
		}
		else
		{
			AddMessageToHistory(false, GetDoc()->GetBuddy(user)->DisplayName(), msg);
		}
		i++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CDialogChat::m_MessageMap[] =
{
	// network messages
	MAP_MSGID( CDialogChat, TSVC_FRIENDS, OnFriendsStatusChanged ),
		MAP_MSGID_PARAMS( CDialogChat, TCL_TYPINGMESSAGE, OnTypingMessage ),
		MAP_MSGID_PARAMS( CDialogChat, TCL_MESSAGE, OnMessage ),
		MAP_MSGID_PARAMS( CDialogChat, TSVC_MESSAGE, OnMessage ),
		MAP_MSGID_PARAMS( CDialogChat, TCL_MESSAGE_FAIL, OnMessageFailed ),
		MAP_MSGID_PARAMS( CDialogChat, TCL_CHATADDUSER, OnChatAddUser ),
		MAP_MSGID_PARAMS( CDialogChat, TCL_CHATUSERLEAVE, OnChatUserLeave ),
		
		// vgui messages
		MAP_MESSAGE( CDialogChat, "SendMessage", OnSendMessage ),
		MAP_MESSAGE( CDialogChat, "ViewGameInfo", OnViewGameInfo ),
		MAP_MESSAGE( CDialogChat, "CheckRemoteTypingTimeout", OnCheckRemoteTypingTimeout ),
		MAP_MESSAGE_INT( CDialogChat, "InviteUser", OnInviteUser, "userID" ),
		MAP_MESSAGE_PARAMS( CDialogChat, "DragDrop", OnDragDrop ),
		MAP_MESSAGE_CONSTCHARPTR( CDialogChat, "TextClicked", OnTextClicked, "text" ),
		MAP_MESSAGE_PTR( CDialogChat, "TextChanged", OnTextChanged, "panel" ),
};
IMPLEMENT_PANELMAP(CDialogChat, vgui::Frame);
