//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "PlayerListDialog.h"
#include "Friends/IFriendsUser.h"
extern IFriendsUser *g_pFriendsUser;

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/ListPanel.h>
#include <KeyValues.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/MessageBox.h>

#include "EngineInterface.h"
#include "cl_dll/IGameClientExports.h"
#include "GameUI_Interface.h"
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPlayerListDialog::CPlayerListDialog(vgui::Panel *parent) : BaseClass(parent, "PlayerListDialog")
{
	SetSize(320, 240);

	if (GameClientExports())
	{
		wchar_t title[256];
		wchar_t hostname[128];
		localize()->ConvertANSIToUnicode(GameClientExports()->GetServerHostName(), hostname, sizeof(hostname));
		localize()->ConstructString(title, sizeof(title), localize()->Find("#GameUI_PlayerListDialogTitle"), 1, hostname);
		SetTitle(title, true);
	}
	else
	{
		SetTitle("Current players", true);
	}

	m_pAddFriendButton = new Button(this, "AddFriendButton", "");
	m_pMuteButton = new Button(this, "MuteButton", "");

	m_pPlayerList = new ListPanel(this, "PlayerList");
	m_pPlayerList->AddColumnHeader(0, "Name", "#GameUI_PlayerName", 128);
	m_pPlayerList->AddColumnHeader(1, "TName", "#GameUI_FriendsName", 128);
	m_pPlayerList->AddColumnHeader(2, "Properties", "#GameUI_Properties", 80);

	m_pPlayerList->SetEmptyListText("#GameUI_NoOtherPlayersInGame");

	LoadControlSettings("Resource/PlayerListDialog.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CPlayerListDialog::~CPlayerListDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerListDialog::Activate()
{
	BaseClass::Activate();

	// refresh player list
	m_pPlayerList->DeleteAllItems();
	int maxClients = engine->GetMaxClients();
	for (int i = 1; i <= maxClients; i++)
	{
		// get the player info from the engine
		char szPlayerIndex[32];
		sprintf(szPlayerIndex, "%d", i);

		const char *idString = engine->PlayerInfo_ValueForKey(i, "*tracker");
		if (!idString)
			break;
		
		unsigned int userID = atoi(idString);

		// don't add self to list
		if (g_pFriendsUser && userID == g_pFriendsUser->GetFriendsID())
			continue;

		// collate user data then add it to the table
		KeyValues *data = new KeyValues(szPlayerIndex);
		const char *playerName = engine->PlayerInfo_ValueForKey(i, "name");
		data->SetString("Name", playerName);
		data->SetString("TID", idString);
		const char *userName = g_pFriendsUser->GetUserName(userID);
		data->SetString("TName", userName);
		data->SetInt("index", i);

		// add to the list
		m_pPlayerList->AddItem(data, 0, false, false);
	}

	// refresh player properties info
	RefreshPlayerProperties();

	// select the first item by default
	m_pPlayerList->SetSingleSelectedItem( m_pPlayerList->GetItemIDFromRow(0) );

	// toggle button states
	OnItemSelected();
}

//-----------------------------------------------------------------------------
// Purpose: walks the players and sets their info display in the list
//-----------------------------------------------------------------------------
void CPlayerListDialog::RefreshPlayerProperties()
{
	for (int i = 0; i <= m_pPlayerList->GetItemCount(); i++)
	{
		KeyValues *data = m_pPlayerList->GetItem(i);
		if (!data)
			continue;

		// assemble properties
		int playerIndex = data->GetInt("index");
		int friendsID = data->GetInt("TID");

		// make sure the player is still valid
		const char *playerName = engine->PlayerInfo_ValueForKey(playerIndex, "name");
		if (!playerName)
		{
			// disconnected
			data->SetString("properties", "Disconnected");
			continue;
		}
		data->SetString("name", playerName);

		bool muted = false, friends = false, bot = false;
		if (GameClientExports() && GameClientExports()->IsPlayerGameVoiceMuted(playerIndex))
		{
			muted = true;
		}
		if (g_pFriendsUser && friendsID && g_pFriendsUser->IsBuddy(friendsID))
		{
			friends = true;
		}
		const char *botValue = engine ? engine->PlayerInfo_ValueForKey(playerIndex, "*bot") : NULL;
		if (botValue && !stricmp("1", botValue))
		{
			bot = true;
		}

		if (bot)
		{
			data->SetString("properties", "CPU Player");
		}
		else if (muted && friends)
		{
			data->SetString("properties", "Friend; Muted");
		}
		else if (muted)
		{
			data->SetString("properties", "Muted");
		}
		else if (friends)
		{
			data->SetString("properties", "Friend");
		}
		else
		{
			data->SetString("properties", "");
		}
	}
	m_pPlayerList->RereadAllItems();
}

//-----------------------------------------------------------------------------
// Purpose: Handles the AddFriend command
//-----------------------------------------------------------------------------
void CPlayerListDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "AddFriend"))
	{
		RequestAuthorizationFromSelectedUser();
	}
	else if (!stricmp(command, "Mute"))
	{
		ToggleMuteStateOfSelectedUser();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: toggles whether a user is muted or not
//-----------------------------------------------------------------------------
void CPlayerListDialog::ToggleMuteStateOfSelectedUser()
{
	if (!GameClientExports())
		return;

	KeyValues *data = m_pPlayerList->GetItem(m_pPlayerList->GetSelectedItem(0));
	if (!data)
		return;
	int playerIndex = data->GetInt("index");
	assert(playerIndex);

	if (GameClientExports()->IsPlayerGameVoiceMuted(playerIndex))
	{
		GameClientExports()->UnmutePlayerGameVoice(playerIndex);
	}
	else
	{
		GameClientExports()->MutePlayerGameVoice(playerIndex);
	}

	RefreshPlayerProperties();
	OnItemSelected();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerListDialog::RequestAuthorizationFromSelectedUser()
{
	bool bSuccess = false;

	// get the user ID
	int itemID = m_pPlayerList->GetSelectedItem(0);
	KeyValues *data = NULL;
	unsigned int userID = 0;
	if ( m_pPlayerList->IsValidItemID(itemID) )
	{
		data = m_pPlayerList->GetItem(itemID);
	}
	if (!data)
		return;

	userID = data->GetInt("TID");
	if (userID > 0 && userID != g_pFriendsUser->GetFriendsID())
	{
		g_pFriendsUser->RequestAuthorizationFromUser(userID);
		bSuccess = true;
	}

	// popup a message box indicating success/failure
	wchar_t message[512], friendName[64];
	const char *playerName = data->GetString("TName", "");
	if (!playerName[0])
	{
		playerName = data->GetString("Name", "");
	}
	localize()->ConvertANSIToUnicode(playerName, friendName, sizeof(friendName));
	if (data && userID != g_pFriendsUser->GetFriendsID() && bSuccess)
	{
		localize()->ConstructString(message, sizeof(message), localize()->Find("GameUI_FriendAddedToList"), 1, friendName);
	}
	else
	{
		localize()->ConstructString(message, sizeof(message), localize()->Find("#GameUI_AddFriendFailed"), 1, friendName);
		
	}
	MessageBox *messageBox = new MessageBox(localize()->Find("#GameUI_AddFriendTitle"), message);
	messageBox->DoModal();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerListDialog::OnItemSelected()
{
	// make sure the data is up-to-date
	RefreshPlayerProperties();

	// set the button state based on the selected item
	if (m_pPlayerList->GetSelectedItemsCount() > 0)
	{
		KeyValues *data = m_pPlayerList->GetItem(m_pPlayerList->GetSelectedItem(0));
		const char *botValue = data ? engine->PlayerInfo_ValueForKey(data->GetInt("index"), "*bot") : NULL;
		bool isValidPlayer = botValue ? !stricmp("1", botValue) : false;
		// make sure the player is valid
		if (!engine->PlayerInfo_ValueForKey(data->GetInt("index"), "name"))
		{
			// invalid player, 
			isValidPlayer = true;
		}
		if (data && !isValidPlayer && GameClientExports() && GameClientExports()->IsPlayerGameVoiceMuted(data->GetInt("index")))
		{
			m_pMuteButton->SetText("#GameUI_UnmuteIngameVoice");
		}
		else
		{
			m_pMuteButton->SetText("#GameUI_MuteIngameVoice");
		}

		if (GameClientExports() && !isValidPlayer)
		{
			m_pMuteButton->SetEnabled(true);
		}
		else
		{
			m_pMuteButton->SetEnabled(false);
		}

		if (!isValidPlayer && g_pFriendsUser && !g_pFriendsUser->IsBuddy(data->GetInt("TID")))
		{
			m_pAddFriendButton->SetEnabled(true);
		}
		else
		{
			m_pAddFriendButton->SetEnabled(false);
		}
	}
	else
	{
		m_pAddFriendButton->SetEnabled(false);
		m_pMuteButton->SetEnabled(false);
		m_pMuteButton->SetText("#GameUI_MuteIngameVoice");
	}
}

//-----------------------------------------------------------------------------
// Purpose: empty message map
//-----------------------------------------------------------------------------
MessageMapItem_t CPlayerListDialog::m_MessageMap[] =
{
  	MAP_MESSAGE( CPlayerListDialog, "ItemSelected", OnItemSelected ),
};
IMPLEMENT_PANELMAP(CPlayerListDialog, Frame);
