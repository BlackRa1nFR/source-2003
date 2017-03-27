//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "Buddy.h"
#include "DialogAuthRequest.h"
#include "DialogChat.h"
#include "DialogUserInfo.h"
#include "OnlineStatus.h"
#include "ServerSession.h"
#include "Tracker.h"
#include "TrackerDialog.h"
#include "TrackerDoc.h"

#include <VGUI_KeyValues.h>

#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>
                                                                                                                                             
using namespace vgui;

extern IServerBrowser *g_pIServerBrowser;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBuddy::CBuddy(unsigned int userID, KeyValues *data) : m_iUserID(userID), m_pData(data)
{
	m_bSendViaServer = false;
	m_iNextPlayTime = 0;
	m_iBlockLevel = m_pData->GetInt("BlockLevel");
	m_hGameInfoDialog = 0;

	// don't play sounds right after creation
	if (CTrackerDialog::GetInstance())
	{
		m_iNextPlayTime = system()->GetTimeMillis() + 2000;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBuddy::~CBuddy()
{
}

//-----------------------------------------------------------------------------
// Purpose: closes all dialogs associated with the buddy
//-----------------------------------------------------------------------------
void CBuddy::ShutdownDialogs()
{
	if (m_hChatDialog)
	{
		ivgui()->PostMessage(m_hChatDialog->GetVPanel(), new KeyValues("Close"), NULL);
	}
	if (m_hAuthRequestDialog)
	{
		ivgui()->PostMessage(m_hAuthRequestDialog->GetVPanel(), new KeyValues("Close"), NULL);
	}
	if (m_hUserInfoDialog)
	{
		ivgui()->PostMessage(m_hUserInfoDialog->GetVPanel(), new KeyValues("Close"), NULL);
	}
	if (m_hGameInfoDialog)
	{
		g_pIServerBrowser->CloseGameInfoDialog(m_hGameInfoDialog);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CBuddy::MessageList()
{
	return Data()->FindKey("Messages", true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
unsigned int CBuddy::BuddyID()
{
	return m_iUserID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBuddy::UserName()
{
	return Data()->GetString("UserName", /* Default value => */ "Unnamed");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBuddy::DisplayName()
{
	const char *name = Data()->GetString("DisplayName", NULL);
	if (!name || !name[0])
	{
		name = UserName();
		Data()->SetString("DisplayName", name);
	}

	return name;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuddy::UpdateStatus(int status, unsigned int sessionID, unsigned int serverID, unsigned int *IP, unsigned int *port, unsigned int *gameIP, unsigned int *gamePort, const char *gameType)
{
	int realStatus = status;
	int oldStatus = Data()->GetInt("Status");

	// check for blocks
	if (status >= COnlineStatus::ONLINE && m_iBlockLevel == BLOCK_ONLINE)
	{
		// this is a bit of a hack; Client should receive real status from server if blocked
		// good solution until people hack the Client, low priority anyway
		status = Data()->GetInt("FakeStatus");
	}
	else if (status == COnlineStatus::INGAME && m_iBlockLevel == BLOCK_GAME)
	{
		// don't show that this user is in a game
		status = Data()->GetInt("FakeStatus");
	}

	// check for playing sounds/alerts
	if (status == COnlineStatus::INGAME && status != oldStatus)
	{
		// play the sound if the doc says to
		if (Data()->GetInt("SoundIngame") || GetDoc()->Data()->GetInt("User/Sounds/Ingame", 1))
		{
			surface()->PlaySound("Friends\\friend_join.wav");
			m_iNextPlayTime = system()->GetTimeMillis() + 5000;
		}
	}
	else if (status >= COnlineStatus::ONLINE && oldStatus < COnlineStatus::ONLINE)
	{
		// don't play the online sound too often
		if (m_iNextPlayTime < system()->GetTimeMillis())
		{
			// play the sound if the doc says to
			if (Data()->GetInt("SoundOnline") || GetDoc()->Data()->GetInt("User/Sounds/Online", 0))
			{
				surface()->PlaySound("Friends\\friend_online.wav");
				m_iNextPlayTime = system()->GetTimeMillis() + 5000;
			}

			// open up the chat dialog when the user comes online
			if (Data()->GetInt("NotifyOnline"))
			{
				OpenChatDialog(true);
				char buf[512];
				sprintf(buf, "%s has just come online.\n", DisplayName());
				m_hChatDialog->AddTextToHistory(m_hChatDialog->GetFgColor(), buf);
			}
		}
	}
	else if (status < COnlineStatus::ONLINE && oldStatus >= COnlineStatus::ONLINE)
	{
		// if they've just dropped offline, don't replay the online sound again if they got right online again
		m_iNextPlayTime = system()->GetTimeMillis() + 5000;
	}

	Data()->SetInt("Status", status);

	if (Data()->GetInt("RequestingAuth", 0))
	{
		Data()->SetInt("Status", COnlineStatus::REQUESTINGAUTHORIZATION);
	}
	else if (Data()->GetInt("Removed", 0))
	{
		Data()->SetInt("Status", COnlineStatus::REMOVED);
	}

	// clear any game settings if we're not in a game
	if (status != COnlineStatus::INGAME)
	{
		Data()->SetString("Game", "");
		Data()->SetInt("GameIP", 0);
		Data()->SetInt("GamePort", 0);
	}

	if (IP && *IP)
	{
		Data()->SetInt("IP", *IP);
	}
	if (port && *port)
	{
		Data()->SetInt("Port", *port);
	}

	if (gameIP)
	{
		Data()->SetInt("GameIP", *gameIP);
	}
	if (gamePort)
	{
		Data()->SetInt("GamePort", *gamePort);
	}
	if (gameType)
	{
		Data()->SetString("Game", gameType);
	}

	if (sessionID)
	{
		Data()->SetInt("SessionID", sessionID);
	}
	if (serverID)
	{
		Data()->SetInt("ServerID", serverID);
	}

	// check for updating block status
	if (realStatus >= COnlineStatus::ONLINE && IP && port)
	{
		if (Data()->GetInt("UpdateBlock"))
		{
			// resend the block to the newly online user
			SetRemoteBlock(Data()->GetInt("RemoteBlock"));
			
			// unset flag
			Data()->SetInt("UpdateBlock", 0);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens chat dialog
// Input  : *surface - 
//			minimized - 
//-----------------------------------------------------------------------------
void CBuddy::OpenChatDialog(bool minimized)
{
	// create the dialog if it isn't already open
	if (!m_hChatDialog.Get())
	{
		m_hChatDialog = new CDialogChat(m_iUserID);
		vgui::surface()->CreatePopup(m_hChatDialog->GetVPanel(), minimized);

		// flash the window
		m_hChatDialog->FlashWindow();
		m_hChatDialog->Open(minimized);
	}
	else
	{
		// update
		m_hChatDialog->Update(minimized);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CDialogChat
//-----------------------------------------------------------------------------
CDialogChat *CBuddy::GetChatDialog()
{
	return m_hChatDialog.Get();
}

//-----------------------------------------------------------------------------
// Purpose: Opens the user info dialog
// Input  : *surface - 
//			minimized - 
//-----------------------------------------------------------------------------
void CBuddy::OpenUserInfoDialog(bool minimized)
{
	if (!m_hUserInfoDialog.Get())
	{
		m_hUserInfoDialog = new CDialogUserInfo(m_iUserID);
		vgui::surface()->CreatePopup(m_hUserInfoDialog->GetVPanel(), minimized);

		m_hUserInfoDialog->Open();
	}
	else
	{
		// update
		m_hUserInfoDialog->Open();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens the auth request dialog
// Input  : *surface - 
//			minimized - 
//-----------------------------------------------------------------------------
void CBuddy::OpenAuthRequestDialog(bool minimized)
{
	// remove any messages from the queue
	KeyValues *msgList = MessageList();
	KeyValues *msg = msgList->GetFirstSubKey();
	while (msg)
	{
		// get the reason for request
		Data()->SetString("Reason", msg->GetString("text", ""));

		GetDoc()->MoveMessageToHistory(m_iUserID, msg);
		msg = msgList->GetFirstSubKey();
		Data()->SetInt("MessageAvailable", 0);
	}

	if (m_hAuthRequestDialog)
	{
		// just bring it to the fore
		if (!minimized)
		{
			vgui::surface()->SetMinimized(m_hAuthRequestDialog->GetVPanel(), minimized);
			m_hAuthRequestDialog->SetVisible(true);
			m_hAuthRequestDialog->MoveToFront();
		}
	}
	else
	{
		CDialogAuthRequest *dialog = new CDialogAuthRequest();
		vgui::surface()->CreatePopup(dialog->GetVPanel(), minimized);
		dialog->AddActionSignalTarget(CTrackerDialog::GetInstance());
		dialog->ReadAuthRequest(m_iUserID);

		if (minimized)
		{
			vgui::surface()->SetMinimized(dialog->GetVPanel(), true);
			dialog->FlashWindow();
		}
		else
		{
			dialog->MoveToFront();
		}

		// play a sound
		if (GetDoc()->Data()->GetInt("User/Sounds/Message", 1))
		{
			surface()->PlaySound("Friends\\Message.wav");
		}

		m_hAuthRequestDialog = dialog;
	}
}


//-----------------------------------------------------------------------------
// Purpose: opens a view onto the game server the buddy is currently playing on
//-----------------------------------------------------------------------------
void CBuddy::OpenGameInfoDialog(bool connectImmediately)
{
	int gameIP = Data()->GetInt("GameIP");
	int gamePort = Data()->GetInt("GamePort");

	if (gameIP && gamePort)
	{
		if (connectImmediately)
		{
			m_hGameInfoDialog = g_pIServerBrowser->JoinGame(gameIP, gamePort, DisplayName());
		}
		else
		{
			m_hGameInfoDialog = g_pIServerBrowser->OpenGameInfoDialog(gameIP, gamePort, DisplayName());
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets the game info dialog
//-----------------------------------------------------------------------------
void CBuddy::RefreshGameInfoDialog()
{
	if (m_hGameInfoDialog)
	{
		// the buddy may have changed games, so notify the game info dialog
		int gameIP = Data()->GetInt("GameIP");
		int gamePort = Data()->GetInt("GamePort");

		if (gameIP && gamePort)
		{
			g_pIServerBrowser->UpdateGameInfoDialog(m_hGameInfoDialog, gameIP, gamePort);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: stops the chat dialog being associated with this buddy
//-----------------------------------------------------------------------------
void CBuddy::DisassociateChatDialog()
{
	m_hChatDialog = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sets a new block on what the other user can be seen as
// Input  : blockLevel - what things to block
//			fakeStatus - new status to show (this is the real status if block is being removed)
//-----------------------------------------------------------------------------
void CBuddy::SetBlock(int blockLevel, int fakeStatus)
{
	m_iBlockLevel = blockLevel;
	Data()->SetInt("BlockLevel", blockLevel);
	Data()->SetInt("FakeStatus", fakeStatus);

	UpdateStatus(fakeStatus, 0, 0, NULL, NULL, NULL, NULL, NULL);

	// make everything refresh
	CTrackerDialog::GetInstance()->OnFriendsStatusChanged(1);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the block level of a user seeing us
// Input  : blockLevel - 
//-----------------------------------------------------------------------------
void CBuddy::SetRemoteBlock(int blockLevel)
{
	Data()->SetInt("RemoteBlock", blockLevel);
	Data()->SetInt("UpdateBlock", 1);

	int fakeStatus = ServerSession().GetStatus();

	// make up a fake status to tell the user if we're blocking them
	if (blockLevel == BLOCK_GAME)
	{
		fakeStatus = COnlineStatus::AWAY;
	}
	else if (blockLevel == BLOCK_ONLINE)
	{
		fakeStatus = COnlineStatus::OFFLINE;
	}
	ServerSession().SetUserBlock(m_iUserID, blockLevel, fakeStatus);

	// make everything refresh
	CTrackerDialog::GetInstance()->OnFriendsStatusChanged(1);
}


//-----------------------------------------------------------------------------
// Purpose: sets whether or not messages to this friend should be sent via the main tracker server
// Input  : state - 
//-----------------------------------------------------------------------------
void CBuddy::SetSendViaServer(bool state)
{
	m_bSendViaServer = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBuddy::SendViaServer()
{
	return m_bSendViaServer;
}


//-----------------------------------------------------------------------------
// Purpose: Load a chat from keyvalues 
//          This is used to load saved chats from the last time you quit tracker
//          with a chat window open.
//-----------------------------------------------------------------------------
void CBuddy::LoadChat()
{
	m_hChatDialog->LoadChat(m_iUserID);
}

