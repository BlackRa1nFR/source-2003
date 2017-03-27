//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "TrackerMessageFlags.h"
#include "ServerBrowser/IServerBrowser.h"

#include "Buddy.h"
#include "DialogAuthRequest.h"
#include "DialogFindBuddy.h"
#include "DialogLoginUser.h"
#include "DialogOptions.h"
#include "DialogChat.h"
#include "DialogAbout.h"
#include "interface.h"
#include "IRunGameEngine.h"
#include "OnlineStatus.h"
#include "OnlineStatusSelectComboBox.h"
#include "PanelValveLogo.h"
#include "Tracker.h"
#include "TrackerDialog.h"
#include "TrackerDoc.h"
#include "TrackerProtocol.h"
#include "ServerSession.h"
#include "SubPanelBuddyList.h"
#include "SubPanelTrackerState.h"
#include "FileSystem.h"

#include <VGUI_Button.h>
#include <VGUI_Controls.h>
#include <VGUI_ILocalize.h>
#include <VGUI_IInput.h>
#include <VGUI_IScheme.h>
#include <VGUI_ISurface.h>
#include <VGUI_ISystem.h>
#include <VGUI_IVGui.h>
#include <VGUI_Menu.h>
#include <VGUI_MenuItem.h>
#include <VGUI_MenuButton.h>
#include <VGUI_MessageBox.h>
#include "FileSystem.h"



using namespace vgui;

static CTrackerDialog *s_pTrackerDialog = NULL;

CSysModule *g_hServerBrowserModule = NULL;

IServerBrowser *g_pIServerBrowser = NULL;

static CTrackerDoc *g_pTrackerDoc = NULL;

// time in seconds before we go to sleep
static const int TIME_AUTOAWAY = (60 * 15);
static const int TIME_AUTOSNOOZE = (60 * 120);

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the instance of the TrackerDialog
//-----------------------------------------------------------------------------
CTrackerDialog *CTrackerDialog::GetInstance()
{
	return s_pTrackerDialog;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the document
// Output : CTrackerDoc
//-----------------------------------------------------------------------------
CTrackerDoc *GetDoc()
{
	return g_pTrackerDoc;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTrackerDialog::CTrackerDialog() : Frame(NULL, "TrackerDialog")
{
	SetTitle("#TrackerUI_Friends_Title", true);

	SetBounds(20, 20, 200, 400);
	SetMinimumSize(160, 250);

	m_pTrackerButton = NULL;
	m_pBuddyListPanel = NULL;
	m_pStatePanel = NULL;
	m_bOnline = false;
	m_iLoginTransitionPixel = -1;

	m_hDialogLoginUser = NULL;

	s_pTrackerDialog = this;
	m_bInGame = false;
	m_bNeedToSaveUserData = false;
	m_bNeedUpdateToFriendStatus = false;
	m_bInAutoAwayMode = false;

	m_pTrackerMenu = NULL;

	// tracker doc
	g_pTrackerDoc = new CTrackerDoc();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTrackerDialog::~CTrackerDialog()
{
	system()->SetWatchForComputerUse(false);
	ServerSession().RemoveNetworkMessageWatch(this);

	delete g_pTrackerDoc;
	g_pTrackerDoc = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Tries to Start tracker
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTrackerDialog::Start()
{
	// set up some VGUI
	Initialize();

	// try and load the user
	KeyValues *pAppData = ::GetDoc()->Data()->FindKey("App", true);
	int currentUID = pAppData->GetInt("uid", 0);
	if (currentUID)
	{
		StartTrackerWithUser(currentUID);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks for auto-away
//-----------------------------------------------------------------------------
void CTrackerDialog::OnTick()
{
	ServerSession().RunFrame();

	//!! hack, reset the login transition slide if we're not visible
	if (!IsVisible())
	{
		m_bOnline = ServerSession().IsConnectedToServer();
		m_iLoginTransitionPixel = -1;
	}

	// check auto-away status
	if (Tracker_StandaloneMode())
	{
		int currentStatus = ServerSession().GetStatus();

		if (m_bInAutoAwayMode)
		{
			// check for going back to normal status
			double lastTime = system()->GetTimeSinceLastUse();
			if (lastTime < TIME_AUTOAWAY)
			{
				OnUserChangeStatus(COnlineStatus::ONLINE);
				m_bInAutoAwayMode = false;
			}
			else if (currentStatus == COnlineStatus::AWAY && lastTime > TIME_AUTOSNOOZE)
			{
				// check for going to mega-away status
				OnUserChangeStatus(COnlineStatus::SNOOZE);
			}
		}
		else if (currentStatus == COnlineStatus::ONLINE)
		{
			// check for auto-away
			if (system()->GetTimeSinceLastUse() > TIME_AUTOAWAY)
			{
				OnUserChangeStatus(COnlineStatus::AWAY);
				m_bInAutoAwayMode = true;
			}
		}
		else if (currentStatus == COnlineStatus::SNOOZE)
		{
			// we shouldn't be snoozing if we're not in auto-away mode
			OnUserChangeStatus(COnlineStatus::ONLINE);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: brings tracker to the foreground
//-----------------------------------------------------------------------------
void CTrackerDialog::Activate()
{
	surface()->SetMinimized(GetVPanel(), false);
	SetVisible(true);
	MoveToFront();
	RequestFocus();
	InvalidateLayout();
	m_pBuddyListPanel->InvalidateAll();
	m_pStatePanel->InvalidateLayout();

	if (::GetDoc()->GetUserID() <= 0)
	{
		PopupCreateNewUserDialog(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDialog::ShutdownUI()
{
	// save off our current status
	int currentStatus = ServerSession().GetStatus();
	GetDoc()->Data()->SetInt("User/LastStatus", currentStatus);

	// Stop watching the keyboard
	system()->SetWatchForComputerUse(false);

	// save off our minimized status
	if (Tracker_StandaloneMode())
	{
		GetDoc()->Data()->SetInt("App/StartMinimized", !IsVisible());

		int x, y, wide, tall;
		GetBounds(x, y, wide, tall);
		GetDoc()->Data()->SetInt("App/win_x", x);
		GetDoc()->Data()->SetInt("App/win_y", y);
		GetDoc()->Data()->SetInt("App/win_w", wide);
		GetDoc()->Data()->SetInt("App/win_t", tall);
	}
	else
	{
	   	int x, y, wide, tall;
		GetBounds(x, y, wide, tall);
		GetDoc()->Data()->SetInt("App/win_game_x", x);
		GetDoc()->Data()->SetInt("App/win_game_y", y);
		GetDoc()->Data()->SetInt("App/win_game_w", wide);
		GetDoc()->Data()->SetInt("App/win_game_t", tall);
	}

	// save our data to disk
	// only "App/" and "User/" are saved, any other sections are temporary
	OnSaveUserData();

	// close down all dialogs
	GetDoc()->ShutdownAllDialogs();
	for ( int i = m_hChatDialogs.GetCount() - 1; i >= 0; i--)
	{
		if (m_hChatDialogs[i])
		{
			ivgui()->PostMessage(m_hChatDialogs[i]->GetVPanel(), new KeyValues("Close"), this->GetVPanel());
		}
	}

	// send a network message detailing that we are logging off
	ServerSession().UserSelectNewStatus(COnlineStatus::OFFLINE);
}

//-----------------------------------------------------------------------------
// Purpose: closes tracker, shuts down it's networking, saves any data
//-----------------------------------------------------------------------------
void CTrackerDialog::Shutdown()
{
	// shutdown the UI
	ShutdownUI();

	// close down the networking
	ServerSession().Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Opens the login user wizard
//-----------------------------------------------------------------------------
void CTrackerDialog::PopupCreateNewUserDialog(bool performConnectionTest)
{
	if (m_hDialogLoginUser)
	{
		// dialog is already active
		m_hDialogLoginUser->MoveToFront();
		return;
	}

	// if we're not logged in, hide the main dialog
	if (::GetDoc()->GetUserID() <= 0)
	{
		SetVisible(false);
		surface()->SetMinimized(this->GetVPanel(), true);
	}

	m_hDialogLoginUser = new CDialogLoginUser();
	m_hDialogLoginUser->AddActionSignalTarget(this);

	// titles
	m_hDialogLoginUser->SetTitle("#TrackerUI_Friends_LoginUserTitle", true);

	m_hDialogLoginUser->RequestFocus();

	// Start the panel (it will call back when done)
	m_hDialogLoginUser->Run(performConnectionTest);

	if (ServerSession().GetStatus() >= COnlineStatus::ONLINE)
	{
		// log out before the user tries to log in as a different user
		ServerSession().UserSelectNewStatus(COnlineStatus::OFFLINE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Starts tracker main panel with the specified user
//-----------------------------------------------------------------------------
void CTrackerDialog::StartTrackerWithUser(int userID)
{
	GetDoc()->Data()->SetInt("App/UID", userID);

	// record this last login in the registry
	vgui::system()->SetRegistryInteger("HKEY_CURRENT_USER\\Software\\Valve\\Tracker\\LastUserID", userID);

	m_pBuddyListPanel->Clear();

	// load the user-specific data
	GetDoc()->LoadUserData(userID);

	// fire up the main panel
	SetEnabled(true);

	// always Start invisible
	SetVisible(false);

	// set our initial position
	int x, y, wide, tall;
	GetBounds(x, y, wide, tall);
	y=0;
	int swide, stall;
	surface()->GetScreenSize(swide, stall);
	int trackerDefaultWidth = 200;

	if (Tracker_StandaloneMode())
	{
		SetBounds(GetDoc()->Data()->GetInt("App/win_x", swide - wide), 
		GetDoc()->Data()->GetInt("App/win_y", y), 
		GetDoc()->Data()->GetInt("App/win_w", trackerDefaultWidth), 
		GetDoc()->Data()->GetInt("App/win_t", tall));
	}
	else
	{			
		
		SetBounds(GetDoc()->Data()->GetInt("App/win_game_x", swide - wide), 
		GetDoc()->Data()->GetInt("App/win_game_y", 154), 
		GetDoc()->Data()->GetInt("App/win_game_w", wide), 
		GetDoc()->Data()->GetInt("App/win_game_t", tall - (155)));	
	}
	
	GetBounds(x, y, wide, tall);

	// make sure we're not out of range
	int sw, st;
	surface()->GetScreenSize(sw, st);
	if (x + wide > sw)
	{
		x = sw - wide;
	}
	if (y + tall > st)
	{
		y = st - tall;
	}
	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}
	SetPos(x, y);

	// tell the panel to update
	InvalidateLayout(false);

	int prevStatus = ::GetDoc()->Data()->GetInt("User/LastStatus", COnlineStatus::ONLINE);
	if (prevStatus < COnlineStatus::ONLINE)
	{
		prevStatus = COnlineStatus::ONLINE;
	}

	// Start login procedure
	ServerSession().UserSelectNewStatus(prevStatus);

	// notify whomever ran us
	Tracker_GetRunGameEngineInterface()->SetTrackerUserID(userID);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDialog::Initialize( void )
{
	// set our frame panel
	m_pBuddyListPanel = new CSubPanelBuddyList(this, "BuddyList");
	m_pStatePanel = new CSubPanelTrackerState(this, "FriendsState");

	// set up our controls
	/*
	KeyValues *command = new KeyValues("UserChangeStatus");

	command->SetInt("status", COnlineStatus::OFFLINE);
	m_pOnlineStatusButton->AddItem("offline", command->MakeCopy(), this);
	command->SetInt("status", COnlineStatus::AWAY);
	m_pOnlineStatusButton->AddItem("away", command->MakeCopy(), this);
	command->SetInt("status", COnlineStatus::BUSY);
	m_pOnlineStatusButton->AddItem("busy", command->MakeCopy(), this);
	command->SetInt("status", COnlineStatus::ONLINE);
	m_pOnlineStatusButton->AddItem("online", command->MakeCopy(), this);
	*/

	// network message watch
	ServerSession().AddNetworkMessageWatch(this, TSVC_FRIENDS);
	ServerSession().AddNetworkMessageWatch(this, TCL_MESSAGE);
	ServerSession().AddNetworkMessageWatch(this, TSVC_MESSAGE);
	ServerSession().AddNetworkMessageWatch(this, TSVC_LOGINOK);
	ServerSession().AddNetworkMessageWatch(this, TSVC_LOGINFAIL);
	ServerSession().AddNetworkMessageWatch(this, TSVC_GAMEINFO);
	ServerSession().AddNetworkMessageWatch(this, TSVC_FRIENDINFO);
	ServerSession().AddNetworkMessageWatch(this, TCL_USERBLOCK);
	ServerSession().AddNetworkMessageWatch(this, TCL_ADDEDTOCHAT);


	//// CONTEXT MENU ////
	m_pTrackerButton = new MenuButton(this, "TrackerMenuButton", "&Options");
	m_pTrackerButton->SetOpenDirection(MenuButton::UP);
	m_pTrackerMenu = new Menu(m_pTrackerButton, "TrackerMenu");

	// add a hierarchial checkable menu
	Menu *statusSubMenu = new Menu(NULL, "StatusMenu");
	statusSubMenu->AddCheckableMenuItem("Online", new KeyValues("UserChangeStatus", "status", COnlineStatus::ONLINE), this);
	statusSubMenu->AddCheckableMenuItem("Away", new KeyValues("UserChangeStatus", "status", COnlineStatus::AWAY), this);
	statusSubMenu->AddCheckableMenuItem("Busy", new KeyValues("UserChangeStatus", "status", COnlineStatus::BUSY), this);
	statusSubMenu->AddCheckableMenuItem("Offline", new KeyValues("UserChangeStatus", "status", COnlineStatus::OFFLINE), this);

	m_pTrackerMenu->AddMenuItem(new MenuItem(NULL, "MyStatus", "&My Status", statusSubMenu));
	m_pTrackerMenu->AddMenuItem("&Add Friends", "OpenFindBuddyDialog", this);
	m_pTrackerMenu->AddMenuItem("&Edit My Profile", "OpenOptionsDialog", this);
	if (Tracker_StandaloneMode())
	{
		// only let users change login info outside of game
		m_pTrackerMenu->AddMenuItem("&Change Login ", "OpenCreateNewUserDialog", this);
	}

	m_pTrackerButton->SetMenu(m_pTrackerMenu);

	m_pContextMenu = new Menu(NULL, NULL);
	m_pContextMenu->AddMenuItem("&Open", "Open", this);
	m_pContextMenu->AddMenuItem("O&ptions", "OpenOptionsDialog", this);
	m_pContextMenu->AddMenuItem("Find &Game", "ServerBrowser", this);
	m_pContextMenu->AddMenuItem("&Hide", "Minimize", this);
	m_pContextMenu->AddMenuItem("&About ", "About", this);
	m_pContextMenu->SetVisible(false);
	surface()->CreatePopup(m_pContextMenu->GetVPanel(), false);
	surface()->SetAsToolBar(m_pContextMenu->GetVPanel(), true);


	// set initial online status to offline
	// do this after you create the menus so they will be checked properly
	OnOnlineStatusChanged(COnlineStatus::OFFLINE);


	// modify system menu
	Menu *sysMenu = GetSysMenu();
	sysMenu->AddMenuItem("About", "&About...", "About", this);
	MenuItem *panel = dynamic_cast<MenuItem *>(sysMenu->FindChildByName("Close"));
	if (panel)
	{
		panel->SetText("#TrackerUI_MenuHide");
	}

	m_bNeedUpdateToFriendStatus = false;

	// load settings (they are dynamically resized after this)
	LoadControlSettings("Friends/TrackerDialog.res");

	// Start minimized
	SetVisible(false);

	if (Tracker_StandaloneMode())
	{
		// initialize auto-away
		system()->SetWatchForComputerUse(true);
	}

	ivgui()->AddTickSignal(this->GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDialog::OpenServerBrowser()
{
	if (!g_pIServerBrowser)
		return;

	g_pIServerBrowser->Activate();
}


//-----------------------------------------------------------------------------
// Purpose: Opens the options/preferences dialog
//-----------------------------------------------------------------------------
void CTrackerDialog::OpenOptionDialog()
{
	if (!m_hDialogOptions.Get())
	{
		m_hDialogOptions = new CDialogOptions();
		surface()->CreatePopup(m_hDialogOptions->GetVPanel(), false);
		m_hDialogOptions->AddActionSignalTarget(this);
	}   

	// open the dialog
	m_hDialogOptions->Run();
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDialog::OpenAboutDialog()
{
	if (!m_hDialogAbout.Get())
	{
		m_hDialogAbout = new CDialogAbout();
		surface()->CreatePopup(m_hDialogAbout->GetVPanel(), false);
	}

	// open the dialog
	m_hDialogAbout->Run();
}

//-----------------------------------------------------------------------------
// Purpose: Message handler
// Input  : userid - 
//-----------------------------------------------------------------------------
void CTrackerDialog::OnUserCreateFinished(int userid, const char *password)
{
	OnUserCreated(userid, password);
}

//-----------------------------------------------------------------------------
// Purpose: Handles when user creation has been cancelled
//-----------------------------------------------------------------------------
void CTrackerDialog::OnUserCreateCancel()
{
	if (!GetDoc()->GetUserID())
	{
		// if we aren't logged in, and we can't create a user, just close the program
		ivgui()->PostMessage(this->GetVPanel(), new KeyValues("Command", "command", "Quit"), this->GetVPanel());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : newStatus - 
//-----------------------------------------------------------------------------
void CTrackerDialog::OnOnlineStatusChanged(int newStatus)
{
	if (newStatus < 1)
	{
		// we're offline, so clear our buddy states
		GetDoc()->ClearBuddyStatus();

		// mark ourselves as being offline
		SetTitle("#TrackerUI_Friends_OfflineTitle", true);
	}
	else
	{
		SetTitle("#TrackerUI_Friends_Title", true);
	}

	// set the check in the menu
	// first uncheck all in the submenu.
	if (m_pTrackerMenu)
	{
		Menu *subMenu =  m_pTrackerMenu->GetMenuItem(0)->GetMenu();
		assert(subMenu);
		for (int i = 0; i < subMenu->GetNumberOfMenuItems(); i++)
		{
			subMenu->GetMenuItem(i)->SetChecked(false);
		}
		// now set the correct item checked
		if (newStatus == COnlineStatus::ONLINE)
		{
			subMenu->GetMenuItem(0)->SetChecked(true);
		}
		else if (newStatus == COnlineStatus::AWAY)
		{
			subMenu->GetMenuItem(1)->SetChecked(true);
		}
		else if (newStatus == COnlineStatus::BUSY)
		{
			subMenu->GetMenuItem(2)->SetChecked(true);
		}
		else if ( (newStatus == COnlineStatus::OFFLINE) ||
			(newStatus == COnlineStatus::CONNECTING) || 
			(newStatus == COnlineStatus::RECENTLYONLINE) )

		{
			subMenu->GetMenuItem(3)->SetChecked(true);
		}
	}
	
	
	m_pBuddyListPanel->InvalidateAll();
	m_pStatePanel->InvalidateLayout();
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the use selects a new status from the menu
//-----------------------------------------------------------------------------
void CTrackerDialog::OnUserChangeStatus(int status)
{
	if (m_bInGame && m_iGameIP && status == COnlineStatus::ONLINE)
	{
		// online status in the game means ingame status
		status = COnlineStatus::INGAME;
	}

	// set the requested mode
	ServerSession().UserSelectNewStatus(status);


	input()->SetMouseCapture(NULL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDialog::OnMinimize()
{
	BaseClass::OnMinimize();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDialog::OnSetFocus()
{
	BaseClass::OnSetFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Handles messages from the tracker system
//-----------------------------------------------------------------------------
void CTrackerDialog::OnSystemMessage(KeyValues *msg)
{
	// play the message sound, if they have it enabled
	if (GetDoc()->Data()->GetInt("User/Sounds/Message", 1))
	{
		surface()->PlaySound("Friends\\Message.wav");
	}

	// display the message in a message box
	MessageBox *box = new MessageBox("#TrackerUI_FriendsSystemMessageTitle", msg->GetString("text"));
	box->DoModal();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDialog::OnReceivedMessage(KeyValues *data)
{
	int friendID = data->GetInt("uid");
	int chatID = data->GetInt("ChatID", 0);

	if (data->GetInt("flags") & MESSAGEFLAG_SYSTEM)
	{
		// it's a message from the tracker system, handle it specially
		OnSystemMessage(data);
		return;
	}
	
	KeyValues *buddy = GetDoc()->GetBuddy(friendID)->Data();
	if (!buddy && friendID > 0)
	{
		// create the buddy
		buddy = new KeyValues("Buddy");

		buddy->SetInt("uid", friendID);
		buddy->SetInt("Status", COnlineStatus::UNKNOWNUSER);

		if (data->GetString("UserName", NULL))
		{
			buddy->SetString("UserName", data->GetString("UserName"));
		}
		else
		{
			// just use their ID for now
			char buf[52];
			sprintf(buf, "(user #%d)", friendID);
			buddy->SetString("UserName", buf);
		}
		if (!data->GetString("DisplayName", NULL))
		{
			data->SetString("DisplayName", buddy->GetString("UserName"));
		}
	
		GetDoc()->AddNewBuddy(buddy);
		if (buddy)
		{
			buddy->deleteThis();
		}

		buddy = GetDoc()->GetBuddy(friendID)->Data();

		InvalidateLayout();
		Repaint();
	}

	if (!buddy)
		return;

	if (chatID > 0)
	{
		// it's a multi-user chat, let the chat dialog handle it
	}
	else
	{
		// check for special messages
		int flags = data->GetInt("flags");

		if (flags & MESSAGEFLAG_AUTHED)
		{
			if (buddy->GetInt("Status", COnlineStatus::OFFLINE) == COnlineStatus::AWAITINGAUTHORIZATION)
			{
				// move them to offline right away
				buddy->SetInt("Status", COnlineStatus::OFFLINE);
			}

			// removed any not authed flag
			buddy->SetInt("NotAuthed", 0);

			// only display the message if there is more than just an auth
			if (!(flags & MESSAGEFLAG_REQUESTAUTH))
			{
				InvalidateLayout();
				Repaint();
				return;
			}
		}

		// add it into the buddies message queue
		KeyValues *msg = buddy->FindKey("Messages", true)->CreateNewKey();

		// copy in the text
		msg->SetString("text", data->GetString("text"));
		msg->SetInt("flags", flags);

		// make sure the buddy status is up to date
		int status = data->GetInt("Status", COnlineStatus::ONLINE);
		GetDoc()->UpdateBuddyStatus(friendID, status, 0, 0, NULL, NULL, NULL, NULL, NULL);

		if (flags & MESSAGEFLAG_REQUESTAUTH)
		{
			buddy->SetInt("Status", COnlineStatus::REQUESTINGAUTHORIZATION);
			buddy->SetInt("RequestingAuth", 1);

			// removed any not authed flag
			buddy->SetInt("NotAuthed", 0);
		}

		// make sure we have a user name set
		const char *buddyName = buddy->GetString("UserName", "");
		if (!buddyName[0] || !stricmp(buddyName, "Unnamed"))
		{
			buddy->SetString("UserName", data->GetString("UserName"));
			buddy->SetString("DisplayName", data->GetString("UserName"));
		}

		// flag the buddy as having a message available
		buddy->SetInt("MessageAvailable", 1);

		if (flags & MESSAGEFLAG_REQUESTAUTH)
		{
			GetDoc()->GetBuddy(friendID)->OpenAuthRequestDialog(true);
		}
		else
		{
			GetDoc()->GetBuddy(friendID)->OpenChatDialog(true);
		}
	}

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Friend is telling us about a game they're in
//-----------------------------------------------------------------------------
void CTrackerDialog::OnReceivedGameInfo(KeyValues *data)
{
	// redraw the current screen
	m_pBuddyListPanel->InvalidateAll();
}

//-----------------------------------------------------------------------------
// Purpose: Called when we receive information about a specific friend
//-----------------------------------------------------------------------------
void CTrackerDialog::OnReceivedFriendInfo(KeyValues *data)
{
	unsigned int friendID = data->GetInt("uid");
	if (!friendID)
		return;

	if (GetDoc()->GetUserID() == friendID)
	{
		// the information is about the currently logged in user
		KeyValues *self = GetDoc()->Data()->FindKey("User", true);
		self->SetString("UserName", data->GetString("UserName"));
		self->SetString("FirstName", data->GetString("FirstName"));
		self->SetString("LastName", data->GetString("LastName"));
		self->SetString("Email", data->GetString("Email"));
		InvalidateLayout();
	}

	// it's information about a friend
	CBuddy *buddy = GetDoc()->GetBuddy(friendID);

	// find out if we're authorized to see this users info
	if (data->GetInt("NotAuthed"))
	{
		// don't show this buddy
		if (buddy)
		{
			buddy->Data()->SetInt("NotAuthed", 1);
		}
	}
	else
	{
		// get data
		if (buddy)
		{
			// if user hasn't changed the buddies username, update the buddies displayname as well
			const char *displayName = buddy->Data()->GetString("DisplayName");
			if (!stricmp(displayName, "Unnamed") || !stricmp(buddy->Data()->GetString("UserName"), displayName))
			{
				buddy->Data()->SetString("DisplayName", data->GetString("UserName"));
			}

			buddy->Data()->SetString("UserName", data->GetString("UserName"));
			buddy->Data()->SetString("FirstName", data->GetString("FirstName"));
			buddy->Data()->SetString("LastName", data->GetString("LastName"));
			buddy->Data()->SetString("Email", data->GetString("Email"));

			if (!buddy->Data()->GetString("DisplayName"))
			{
				buddy->Data()->SetString("DisplayName", data->GetString("UserName"));
			}

			buddy->Data()->SetInt("NotAuthed", 0);
		}
		else
		{
			// create a new entry
			GetDoc()->AddNewBuddy(data);
		}

		OnFriendsStatusChanged(1);
	}

	MarkUserDataAsNeedsSaving();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the block level of a user
//-----------------------------------------------------------------------------
void CTrackerDialog::OnReceivedUserBlock(KeyValues *msg)
{
	int buddyID = msg->GetInt("uid");
	int blockLevel = msg->GetInt("Block");
	int fakeStatus = msg->GetInt("FakeStatus");

	CBuddy *buddy = GetDoc()->GetBuddy(buddyID);
	if (!buddy)
		return;

	buddy->SetBlock(blockLevel, fakeStatus);
}

//-----------------------------------------------------------------------------
// Purpose: Called when friends' status is updated
//-----------------------------------------------------------------------------
void CTrackerDialog::OnFriendsStatusChanged(int numberOfFriends)
{
	if (numberOfFriends > 1)
	{
		// we've got our full friends list, so don't bother to get again
		m_bNeedUpdateToFriendStatus = false;
	}

	m_pBuddyListPanel->InvalidateAll();
	m_pStatePanel->InvalidateLayout();
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: MENU INPUT HANDLING
//-----------------------------------------------------------------------------
void CTrackerDialog::OnCommand(const char *command)
{
	if (!strcmp(command, "OpenFindBuddyDialog"))
	{
		// pop up the find buddy dialog
		if (m_hDialogFindBuddy.Get())
		{
			m_hDialogFindBuddy->Open();
		}
		else
		{
			CDialogFindBuddy *pDialogFindBuddy = new CDialogFindBuddy();
			surface()->CreatePopup(pDialogFindBuddy->GetVPanel(), false);
			pDialogFindBuddy->Run();
			m_hDialogFindBuddy = pDialogFindBuddy;
		}
	}
	else if (!stricmp(command, "OpenCreateNewUserDialog"))
	{
		PopupCreateNewUserDialog(false);
	}
	else if (!stricmp(command, "AddFriends"))
	{
		// delay the dialog opening
		PostMessage(this, new KeyValues("Command", "command", "OpenFindBuddyDialog"), 0.5f);
	}
	else if (!stricmp(command, "Quit"))
	{
		OnQuit();
	}
	else if (!stricmp(command, "Open"))
	{
		Activate();
	}
	else if (!stricmp(command, "Minimize"))
	{
		OnClose();
	}
	else if (!stricmp(command, "ServerBrowser"))
	{
		OpenServerBrowser();
	}
	else if (!stricmp(command, "OpenOptionsDialog"))
	{
		OpenOptionDialog();
	}
	else if (!stricmp(command, "About"))
	{
		OpenAboutDialog();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the X button is hit, minimizes tracker to icon
//-----------------------------------------------------------------------------
void CTrackerDialog::OnClose()
{
	SetVisible(false);
}


//-----------------------------------------------------------------------------
// Purpose: Closes the app
//-----------------------------------------------------------------------------
void CTrackerDialog::OnQuit()
{
	// make us disappear
	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Recalculates the buddy list
//-----------------------------------------------------------------------------
void CTrackerDialog::PerformLayout()
{
	// chain back
	BaseClass::PerformLayout();

	// set our title
	char szTitle[256];
	switch (ServerSession().GetStatus())
	{
	case COnlineStatus::OFFLINE:
	case COnlineStatus::CONNECTING:
		sprintf(szTitle, "Friends - %s - offline", GetDoc()->GetUserName());
		break;

	case COnlineStatus::AWAY:
		sprintf(szTitle, "Friends - %s - away", GetDoc()->GetUserName());
		break;

	case COnlineStatus::BUSY:
		sprintf(szTitle, "Friends - %s - busy", GetDoc()->GetUserName());
		break;

	case COnlineStatus::ONLINE:
	default:
		sprintf(szTitle, "Friends - %s", GetDoc()->GetUserName());
	}
	SetTitle(szTitle, true);

//tagES
/*	wchar_t unicode[256], unicodeName[64];
	localize()->ConvertANSIToUnicode(GetDoc()->GetUserName(), unicodeName, sizeof( unicodeName ) / sizeof( wchar_t ));

	switch (ServerSession().GetStatus())
	{
	case COnlineStatus::OFFLINE:
	case COnlineStatus::CONNECTING:
		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_Friends_Name_OfflineTitle"), 1, unicodeName );
		break;

	case COnlineStatus::AWAY:
		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_Friends_Name_AwayTitle"), 1, unicodeName );
		break;

	case COnlineStatus::BUSY:
		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_Friends_Name_BusyTitle"), 1, unicodeName );
		break;

	case COnlineStatus::ONLINE:
	default:
		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_Friends_Name_Title"), 1, unicodeName );
	}
	SetTitle(unicode, true);
*/

	// check to see if we've changed status
	if (m_bOnline != ServerSession().IsConnectedToServer())
	{
		if (!m_bOnline)
		{
			// we're coming online, start the transition
			m_iLoginTransitionPixel = 0;
		}
	}
	m_bOnline = ServerSession().IsConnectedToServer();

	// layout buttons on this dialog
	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);

	// put menu buttons below panel
	int addFriendsRightSize = x + 0;
	if (m_pTrackerButton->IsVisible())
	{
		m_pTrackerButton->SetBounds(x, y + tall - 28, 64, 24);
		addFriendsRightSize += 64;
	}

	// put the valve logo in the bottom right, if there is room
	// size the border panel
	int bottomOfButton = 32;
	int x3 = 6;
	
	int x2, y2;
	m_pTrackerButton->GetPos(x2, y2);
	int topOfTrackerButton = y2;

	m_pBuddyListPanel->SetBounds(x3, bottomOfButton, wide, topOfTrackerButton - bottomOfButton - 4);
	m_pStatePanel->SetBounds(x3, bottomOfButton, wide, topOfTrackerButton - bottomOfButton - 4);

	if (ServerSession().IsConnectedToServer())
	{
		m_pBuddyListPanel->SetVisible(true);
		m_pStatePanel->SetVisible(false);
	}
	else
	{
		m_pStatePanel->SetVisible(true);
		m_pBuddyListPanel->SetVisible(false);
	}

	// add the pixel slide for when we change connection state
	if (m_iLoginTransitionPixel > -1)
	{
		m_pBuddyListPanel->SetVisible(true);
		m_pStatePanel->SetVisible(true);

		m_pStatePanel->SetPos(x3 - m_iLoginTransitionPixel, bottomOfButton);
		m_pBuddyListPanel->SetPos(x3 - m_iLoginTransitionPixel + wide, bottomOfButton);

		if (ServerSession().IsConnectedToServer())
		{
			m_iLoginTransitionPixel++;
		}
		else
		{
			m_iLoginTransitionPixel--;
		}

		if (m_iLoginTransitionPixel >= wide)
		{
			m_iLoginTransitionPixel = -1;
		}

		InvalidateLayout();
	}


	m_pBuddyListPanel->InvalidateLayout(true);
	m_pStatePanel->InvalidateLayout(true);

	OpenSavedChats();
}

//-----------------------------------------------------------------------------
// Purpose: Called when tracker is fully logged in to the server
//-----------------------------------------------------------------------------
void CTrackerDialog::OnLoginOK()
{
	// make sure we have all the necessary information about ourself
	ServerSession().RequestUserInfoFromServer(GetDoc()->GetUserID());

	// let the game know our userID
	Tracker_GetRunGameEngineInterface()->SetTrackerUserID(GetDoc()->GetUserID());	
}

//-----------------------------------------------------------------------------
// Purpose: Popup the any previous chats we had going (not multiuser chats)
//-----------------------------------------------------------------------------
void CTrackerDialog::OpenSavedChats()
{
	static done = false;
	
	if (done)
		return;

	// open any previous chats we had going
	// save chat window 
	int numBuds = GetDoc()->GetNumberOfBuddies();
	for ( int i = 0; i < numBuds; i++)
	{
		unsigned int buddyID = GetDoc()->GetBuddyByIndex(i);	  
		CBuddy *buddy = GetDoc()->GetBuddy( buddyID);
		if (buddy)
		{
			KeyValues *checkChat = buddy->Data()->FindKey("ActiveChat", NULL);
			if (checkChat)
			{
				buddy->OpenChatDialog(false);
				buddy->LoadChat();
			}
		}
	}

	done = true;
}

//-----------------------------------------------------------------------------
// Purpose: Popup the login user dialog, if it's not already up
//-----------------------------------------------------------------------------
void CTrackerDialog::OnLoginFail(int reason)
{
	// determine error
	const char *errorText;
	bool bQuitTracker = false;
	switch (reason)
	{
	case -1:
		errorText = "#TrackerUI_ErrorIncorrectPassword";
		break;

	case -2:
		errorText = "#TrackerUI_ErrorUserDoesntExist";
		break;

	case -3:
		// 'account already in use' error
		// wait 10 seconds then try and connect again
		return;
		break;
	case -4:
		//!! need better error
		errorText = "#TrackerUI_ErrorTrackerOutOfDate";
		bQuitTracker = true;
		break;
		
	default:
		errorText = "#TrackerUI_ErrorPleaseInstallLatestVersion";
		break;
	}

	// popup error dialog
	MessageBox *box = new MessageBox("", errorText);
	box->SetSize(320, 160);
	box->SetVisible(true);
	box->SetTitle("#TrackerUI_Friends_LoginErrorTitle", true);
	box->AddActionSignalTarget(this);
	if (bQuitTracker)
	{
		box->SetCommand("Quit");
	}
	else
	{
		box->SetCommand("OpenCreateNewUserDialog");
	}
	box->MoveToFront();
	box->DoModal();

	// hide self
	SetVisible(false);
	Tracker_GetRunGameEngineInterface()->SetTrackerUserID(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDialog::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pBuddyListPanel->SetBorder(scheme()->GetBorder(GetScheme(), "ButtonDepressedBorder"));
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new user is created
//-----------------------------------------------------------------------------
void CTrackerDialog::OnUserCreated(int userid, const char *password)
{
	// save the current user state
	MarkUserDataAsNeedsSaving();

	// kill the old panel
	if (m_hDialogLoginUser)
	{
		m_hDialogLoginUser->MarkForDeletion();
		m_hDialogLoginUser = NULL;
	}

	surface()->SetMinimized(this->GetVPanel(), false);

	// move to main logging in screen
	StartTrackerWithUser(GetDoc()->GetUserID());
}

//-----------------------------------------------------------------------------
// Purpose: Called when the tracker is initialized in the game, with the current game name
//-----------------------------------------------------------------------------
void CTrackerDialog::OnActiveGameName(const char *gameName)
{
	strncpy(m_szGameDir, gameName, sizeof(m_szGameDir) - 1);

	// set the server state
	ServerSession().SetGameInfo(m_szGameDir, m_iGameIP, m_iGamePort);

	// tell the game server our id
	Tracker_GetRunGameEngineInterface()->SetTrackerUserID(GetDoc()->GetUserID());
}

//-----------------------------------------------------------------------------
// Purpose: Called when the Client joins a server, sets us up into tracker ingame mode
//-----------------------------------------------------------------------------
void CTrackerDialog::OnConnectToGame(int IP, int port)
{
	// save game data
	m_iGameIP = IP;
	m_iGamePort = port;

	// set in document
	GetDoc()->Data()->SetInt("Login/GameIP", m_iGameIP);
	GetDoc()->Data()->SetInt("Login/GamePort", m_iGamePort);
	GetDoc()->Data()->SetString("Login/GameDir", m_szGameDir);

	// tell server
	ServerSession().SetGameInfo(m_szGameDir, m_iGameIP, m_iGamePort);

	if (!m_bInGame)
	{
		// flip us to ingame mode if we're online
		m_bInGame = true;
		ServerSession().UserSelectNewStatus(COnlineStatus::INGAME);		
	}

	// tell the game server our id
	Tracker_GetRunGameEngineInterface()->SetTrackerUserID(GetDoc()->GetUserID());
}

//-----------------------------------------------------------------------------
// Purpose: Called when we disconnect from a game, resets tracker mode
//-----------------------------------------------------------------------------
void CTrackerDialog::OnDisconnectFromGame()
{
	if (m_bInGame)
	{
		m_bInGame = false;
		
		// go offline when not in a server, since the user can't access the UI anyway
		m_iGameIP = 0;
		m_iGamePort = 0;
		ServerSession().SetGameInfo(m_szGameDir, m_iGameIP, m_iGamePort);
		ServerSession().UserSelectNewStatus(COnlineStatus::ONLINE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message handler to save user data out to disk
//-----------------------------------------------------------------------------
void CTrackerDialog::OnSaveUserData()
{
	GetDoc()->SaveAppData();
	GetDoc()->SaveUserData();
	m_bNeedToSaveUserData = false;
}

//-----------------------------------------------------------------------------
// Purpose: Marks the user data that it needs to be saved to disk
//-----------------------------------------------------------------------------
void CTrackerDialog::MarkUserDataAsNeedsSaving()
{
	if (!m_bNeedToSaveUserData)
	{
		PostMessage(this, new KeyValues("SaveUserData"), 2.0f);
		m_bNeedToSaveUserData = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *chatDialog - 
//			remove - 
//-----------------------------------------------------------------------------
void CTrackerDialog::SetChatDialogInList(CDialogChat *chatDialog, bool remove)
{
	if (remove)
	{
		m_hChatDialogs.RemoveElement(chatDialog);
	}
	else
	{
		m_hChatDialogs.PutElement(chatDialog);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens a chat dialog with other users
//-----------------------------------------------------------------------------
void CTrackerDialog::OnAddedToChat(KeyValues *msg)
{
	int friendID = msg->GetInt("uid");
	int chatID = msg->GetInt("ChatID");

	// check to see if we're already in this chat
	for (int i = 0; i < m_hChatDialogs.GetCount(); i++)
	{
		if (m_hChatDialogs[i] && m_hChatDialogs[i]->GetChatID() == chatID)
			return;
	}

	// see if we already have a chat dialog open with this user
	CDialogChat *dialogChat = GetDoc()->GetBuddy(friendID)->GetChatDialog();
	if (!dialogChat)
	{
		// create the new multi-chat dialog
		dialogChat = new CDialogChat(friendID);
		surface()->CreatePopup(dialogChat->GetVPanel(), true);
		dialogChat->Open(true);
		dialogChat->OnInvited();
	}

	dialogChat->InitiateMultiUserChat(chatID);
	dialogChat->FlashWindow();
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CTrackerDialog::m_MessageMap[] =
{
	// network messages
	MAP_MSGID( CTrackerDialog, TSVC_LOGINOK, OnLoginOK ),
	MAP_MSGID_INT( CTrackerDialog, TSVC_FRIENDS, OnFriendsStatusChanged, "count" ),
	MAP_MSGID_INT( CTrackerDialog, TSVC_LOGINFAIL, OnLoginFail, "reason" ),
	MAP_MSGID_PARAMS( CTrackerDialog, TCL_ADDEDTOCHAT, OnAddedToChat ),
	MAP_MSGID_PARAMS( CTrackerDialog, TCL_MESSAGE, OnReceivedMessage ),
	MAP_MSGID_PARAMS( CTrackerDialog, TSVC_MESSAGE, OnReceivedMessage ),
	MAP_MSGID_PARAMS( CTrackerDialog, TSVC_GAMEINFO, OnReceivedGameInfo ),
	MAP_MSGID_PARAMS( CTrackerDialog, TSVC_FRIENDINFO, OnReceivedFriendInfo ),
	MAP_MSGID_PARAMS( CTrackerDialog, TCL_USERBLOCK, OnReceivedUserBlock ),

	MAP_MESSAGE( CTrackerDialog, "UserCreateCancel", OnUserCreateCancel ),
	MAP_MESSAGE( CTrackerDialog, "SaveUserData", OnSaveUserData ),
	MAP_MESSAGE( CTrackerDialog, "DisconnectedFromGame", OnDisconnectFromGame ),
	MAP_MESSAGE_INT( CTrackerDialog, "UserChangeStatus", OnUserChangeStatus, "status" ),
	MAP_MESSAGE_INT( CTrackerDialog, "OnlineStatus", OnOnlineStatusChanged, "value" ),
	MAP_MESSAGE_CONSTCHARPTR( CTrackerDialog, "ActiveGameName", OnActiveGameName, "name" ),
	MAP_MESSAGE_INT_INT( CTrackerDialog, "ConnectedToGame", OnConnectToGame, "ip", "port" ),
	MAP_MESSAGE_INT_CONSTCHARPTR( CTrackerDialog, "UserCreateFinished", OnUserCreateFinished, "userid", "password" ),
};

IMPLEMENT_PANELMAP(CTrackerDialog, vgui::Frame);
