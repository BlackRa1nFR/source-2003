//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "BuddyButton.h"
#include "TrackerDoc.h"
#include "Buddy.h"
#include "OnlineStatus.h"
#include "DialogChat.h"
#include "TrackerDialog.h"
#include "TrackerMessageFlags.h"
#include "Tracker.h"
#include "IRunGameEngine.h"
#include "ServerSession.h"
#include "DialogRemoveUser.h"

#include <VGUI_Controls.h>
#include <VGUI_IScheme.h>
#include <VGUI_ISystem.h>
#include <VGUI_IInput.h>
#include <VGUI_IPanel.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>

#include <VGUI_Cursor.h>
#include <VGUI_Menu.h>
#include <VGUI_MouseCode.h>
#include <VGUI_TextImage.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : char *caption - 
//			x - 
//			y - 
//			wide - 
//			tall - 
//-----------------------------------------------------------------------------
CBuddyButton::CBuddyButton(Panel *parent, int buddyID) : MenuButton(parent, NULL, "")
{
	SetContentAlignment(Label::a_west);
	SetButtonBorderEnabled(false);
	SetOpenDirection(CURSOR);

	SetMouseClickEnabled(MOUSE_LEFT, false);
	SetMouseClickEnabled(MOUSE_RIGHT, true);

	m_iBuddyID = buddyID;
	m_iBuddyStatus = 0;
	m_bDragging = false;
	m_pBuddyData = NULL;

	m_iImageOffset = 4;
	m_iGameRefreshTime = 0;

	// create us a sub menu
	m_pMenu = new Menu(this, NULL);
	MenuButton::SetMenu(m_pMenu);

	m_pMenu->AddMenuItem("AuthReq", "#TrackerUI_ViewAuthRequest", "AuthReq", this);
	m_pMenu->AddMenuItem("Chat", "#TrackerUI_SendInstantMessage", "Chat", this);
	m_pMenu->AddMenuItem("JoinGame", "#TrackerUI_JoinGame", "JoinGame", this);
	m_pMenu->AddMenuItem("GameInfo", "#TrackerUI_ViewGameInfo", "GameInfo", this);
	m_pMenu->AddMenuItem("UserInfo", "#TrackerUI_UserDetails", "UserInfo", this);
	m_pMenu->AddMenuItem("Block", "#TrackerUI_BlockUser", "Block", this);
	m_pMenu->AddMenuItem("Unblock", "#TrackerUI_UnblockUser", "Unblock", this);
	m_pMenu->AddMenuItem("Remove", "#TrackerUI_RemoveUser", "Remove", this);

	m_pDefaultCommand = "Chat";
	_statusImage = NULL;
	SetText("");
	_statusText = new TextImage("");
	Label::AddImage(_statusImage, 0);
	Label::SetTextImageIndex(1);
	Label::AddImage(_statusText, 6);

	m_pBuddy = GetDoc()->GetBuddy(buddyID);

	if (!Tracker_StandaloneMode())
	{
		Color col = GetBgColor();
		SetBgColor(Color(col[0], col[1], col[2], 255));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBuddyButton::~CBuddyButton()
{
	delete _statusText;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame that the button is visible
//-----------------------------------------------------------------------------
void CBuddyButton::OnThink()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void CBuddyButton::OnMouseDoublePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		// recalculate the default command
		recalculateMenuItems();

		// do the default Activate thing
		OnCommand(m_pDefaultCommand);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CBuddyButton::OnCommand(const char *command)
{
	if (!stricmp(command, "Chat"))
	{
		if (m_iBuddyStatus >= COnlineStatus::ONLINE)
		{
			// open send message dialog
			m_pBuddy->OpenChatDialog(false);
		}
		return;
	}
	else if (!stricmp(command, "GameInfo"))
	{
		if (m_iBuddyStatus == COnlineStatus::INGAME)
		{
			m_pBuddy->OpenGameInfoDialog(false);
		}
		return;
	}
	else if (!stricmp(command, "JoinGame"))
	{
		if (m_iBuddyStatus == COnlineStatus::INGAME)
		{
			m_pBuddy->OpenGameInfoDialog(true);
		}
		return;
	}
	else if (!stricmp(command, "AuthReq"))
	{
		if (m_iBuddyStatus == COnlineStatus::REQUESTINGAUTHORIZATION)
		{
			m_pBuddy->OpenAuthRequestDialog(false);
		}
		return;
	}
	else if (!stricmp(command, "UserInfo"))
	{
		// open the user info dialog
		m_pBuddy->OpenUserInfoDialog(false);
		return;
	}
	else if (!stricmp(command, "Block"))
	{
		m_pBuddy->SetRemoteBlock(CBuddy::BLOCK_ONLINE);
	}
	else if (!stricmp(command, "Unblock"))
	{
		m_pBuddy->SetRemoteBlock(CBuddy::BLOCK_NONE);
	}
	else if (!stricmp(command, "Remove"))
	{
		// open the warning dialog
		CDialogRemoveUser *dialog = new CDialogRemoveUser(m_iBuddyID);
		vgui::surface()->CreatePopup(dialog->GetVPanel(), false);

		// center the dialog in screen
		int x, y, wide, tall;
		vgui::surface()->GetScreenSize(wide, tall);
		x = wide / 2;
		y = tall / 2;

		dialog->GetSize(wide, tall);
		dialog->SetPos(x - (wide / 2), y - (tall / 2));

		dialog->Open();
	}

	// chain
	MenuButton::OnCommand(command);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuddyButton::RefreshBuddyStatus(void)
{
	if (m_pBuddyData)
	{
		m_iBuddyStatus = m_pBuddyData->GetInt("Status", /* default => */0);
		m_bHasMessage = m_pBuddyData->GetInt("MessageAvailable");
	}
	else
	{
		m_iBuddyStatus = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Recalculates the button layout
//-----------------------------------------------------------------------------
void CBuddyButton::PerformLayout(void)
{
	// reget the document pointer
	m_pBuddyData = GetDoc()->GetBuddy(m_iBuddyID)->Data();
	if (!m_pBuddyData)
	{
		MenuButton::PerformLayout();
		return;
	}

	// buddy name & online status
	const char *name = GetDoc()->GetBuddy(m_iBuddyID)->DisplayName();
	RefreshBuddyStatus();

	SetText(name);
	SetImagePreOffset(1, 4);

	bool bRemoteBlock = m_pBuddyData->GetInt("RemoteBlock");

	if (m_iBuddyStatus == COnlineStatus::INGAME)
	{
		// check to see if we're in the same game
		bool bInSameGame = false;
		// get the buddies name from the game
		const char *ingameName = Tracker_GetRunGameEngineInterface()->GetUserName(m_iBuddyID);
		if (ingameName)
		{
			bInSameGame = true;
			_statusText->SetText(ingameName);
			_statusText->SetColor(m_GameNameColor);
		}

		// if the buddy isn't in the same game as us, just put the game dir in the description
		if (!bInSameGame)
		{
			//!! get game description - broken since description isn't written
			const char *gameText = m_pBuddyData->GetString("Description", NULL);
			if (!gameText || !gameText[0])
			{
				gameText = m_pBuddyData->GetString("Game", "");

				// request game info
			}

			// make sure the text is lower case
			char buf[256];
			strncpy(buf, gameText, sizeof(buf) - 1);
			buf[sizeof(buf) - 1]  = 0;
			strlwr(buf);

			_statusText->SetText(buf);
			_statusText->SetColor(m_GameColor);
		}
	}
	else if (bRemoteBlock)
	{
		_statusText->SetText("blocked");
		_statusText->SetColor(m_GameColor);
	}
	else
	{
		_statusText->SetText("");
	}

	const char *status;
	if (bRemoteBlock)
	{
		status = "blocked";
	}
	else if (m_bHasMessage)
	{
		status = "message";		// we have a message
	}
	else
	{
		status = COnlineStatus::IDToString(m_iBuddyStatus);
	}

	if (m_iBuddyStatus == COnlineStatus::AWAY)
	{
		int x = 3;
	}

	char buf[128];
	sprintf(buf, "friends/icon_%s", status);
	_statusImage = scheme()->GetImage(GetScheme(), buf);
	
	if (!_statusImage)
	{
		_statusImage = scheme()->GetImage(GetScheme(), "resource/icon_blank");
	}

	SetImageAtIndex(0, _statusImage, m_iImageOffset);

	if (m_iBuddyStatus > 0)
	{
		if (IsArmed())
		{
			SetFgColor(m_ArmedFgColor1);
		}
		else
		{
			SetFgColor(m_FgColor1);
		}
		SetDefaultColor(m_FgColor1, m_BgColor);
		SetArmedColor(m_ArmedFgColor1, m_ArmedBgColor);
		SetDepressedColor(m_ArmedFgColor1, m_ArmedBgColor);
	}
	else
	{
		if (IsArmed())
		{
			SetFgColor(m_ArmedFgColor2);
		}
		else
		{
			SetFgColor(m_FgColor2);
		}
		SetDefaultColor(m_FgColor2, m_BgColor);
		SetArmedColor(m_ArmedFgColor2, m_ArmedBgColor);
		SetDepressedColor(m_ArmedFgColor2, m_ArmedBgColor);
	}

	MenuButton::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: !!Hack to get highlights to draw in right place
//-----------------------------------------------------------------------------
void CBuddyButton::PaintBackground(void)
{
	int wide, tall;
	GetSize(wide, tall);

	surface()->DrawSetColor(GetBgColor());

	// don't draw behind the icon
	int iWide, iTall;
	_statusImage->GetSize(iWide, iTall);
	iWide += m_iImageOffset;

	surface()->DrawFilledRect(iWide, 0, wide, tall);

	//This makes it so the menu will always be in front in GameUI
	if (m_pMenu->IsVisible())
		m_pMenu->MoveToFront();

}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *inResourceData - 
//-----------------------------------------------------------------------------
void CBuddyButton::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	Button::ApplySchemeSettings(pScheme);

	SetImageAtIndex(0, NULL, 0);

	// get buddybutton specific settings
	m_ArmedFgColor1 = GetSchemeColor("BuddyButton/ArmedFgColor1");
	m_ArmedFgColor2 = GetSchemeColor("BuddyButton/ArmedFgColor2");
	m_ArmedBgColor = GetSchemeColor("BuddyButton/ArmedBgColor");

	SetArmedColor(m_ArmedFgColor2, m_ArmedBgColor);
	SetDepressedColor(m_ArmedFgColor2, m_ArmedBgColor);

	m_FgColor1 = GetSchemeColor("BuddyButton/FgColor1");
	m_FgColor2 = GetSchemeColor("BuddyButton/FgColor2");

	m_BgColor = GetSchemeColor("BuddyListBgColor", GetBgColor());

	m_GameColor = GetSchemeColor("StatusSelectFgColor2");
	m_GameNameColor = GetSchemeColor("BaseBrightText");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : depressed - 
//			armed - 
//			selected - 
// Output : Border
//-----------------------------------------------------------------------------
IBorder *CBuddyButton::GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
{
	// never use a border
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuddyButton::recalculateMenuItems()
{
	// recalculate menu items, in order of priority (lowest to highest)
	// the last visible item will be the default (double-click) menu option

	// block/unblock user
	Panel *panel = m_pMenu->FindChildByName("Block");
	if (panel)
	{
		if (m_pBuddyData->GetInt("RemoteBlock"))
		{
			panel->SetVisible(false);
		}
		else
		{
			panel->SetVisible(true);
		}
	}

	panel = m_pMenu->FindChildByName("Unblock");
	if (panel)
	{
		if (m_pBuddyData->GetInt("RemoteBlock"))
		{
			panel->SetVisible(true);
		}
		else
		{
			panel->SetVisible(false);
		}
	}

	// user info
	panel = m_pMenu->FindChildByName("UserInfo");
	if (panel)
	{
		panel->SetVisible(true);
		m_pDefaultCommand = "UserInfo";
	}

	// game information
	panel = m_pMenu->FindChildByName("GameInfo");
	Panel *panel2 = m_pMenu->FindChildByName("JoinGame");
	if (panel && panel2)
	{
		// don't let us view the game info if they're not in a game
		if (m_iBuddyStatus != COnlineStatus::INGAME)
		{
			panel->SetVisible(false);
			panel2->SetVisible(false);
		}
		else
		{
			panel->SetVisible(true);
			panel2->SetVisible(true);
			m_pDefaultCommand = "GameInfo";
		}
	}

	// instant messaging
	panel = m_pMenu->FindChildByName("Chat");
	if (panel)
	{
		// don't send messages to people who are offline
		if (m_iBuddyStatus < COnlineStatus::ONLINE)
		{
			panel->SetVisible(false);
		}
		else
		{
			panel->SetVisible(true);
			m_pDefaultCommand = "Chat";
		}
	}

	// add/remove the auth request menu option
	panel = m_pMenu->FindChildByName("AuthReq");
	if (panel)
	{
		if (m_iBuddyStatus != COnlineStatus::REQUESTINGAUTHORIZATION)
		{
			panel->SetVisible(false);
		}
		else
		{
			panel->SetVisible(true);
			m_pDefaultCommand = "AuthReq";
		}
	}


	m_pMenu->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to the right-click context menu
//-----------------------------------------------------------------------------
Menu *CBuddyButton::GetContextMenu()
{
	return m_pMenu;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuddyButton::setNameText(const char *text)
{
	SetText(text);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuddyButton::setStatusText(const char *text)
{
	_statusText->SetText(text);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *menu - 
//-----------------------------------------------------------------------------
void CBuddyButton::OnShowMenu(Menu *menu)
{
	recalculateMenuItems();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void CBuddyButton::OnMousePressed(vgui::MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		input()->SetMouseCapture(GetVPanel());
		m_bDragging = true;
	}
	else
	{
		BaseClass::OnMousePressed(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Drag-drop support - sees what panel we're over and sees if we can drop on it
// Input  : x - 
//			y - 
//-----------------------------------------------------------------------------
void CBuddyButton::OnCursorMoved(int x, int y)
{
	if (m_bDragging)
	{
		// get the panel the mouse is over
		VPanel *mouseOver = input()->GetMouseOver();
		if (mouseOver != GetVPanel())
		{
			bool bAccept = false;
			if (mouseOver)
			{
				// check to see if the panel has drag/drop support
				KeyValues *keyVal = new KeyValues("DragDrop", "type", "TrackerFriend");
				keyVal->SetInt("friendID", m_iBuddyID);
				if (ipanel()->Client(mouseOver)->RequestInfo(keyVal))
				{
					// info request was successful, set the cursor to acceptable
					bAccept = true;
				}

				keyVal->deleteThis();
			}

			if (bAccept)
			{
				SetCursor(dc_hand);
			}
			else
			{
				SetCursor(dc_no);
			}
		}
		else
		{
			SetCursor(dc_arrow);
		}
	}

	BaseClass::OnCursorMoved(x, y);
}

//-----------------------------------------------------------------------------
// Purpose: Called when mouse capture has been lost
//-----------------------------------------------------------------------------
void CBuddyButton::OnMouseCaptureLost()
{
	m_bDragging = false;
	SetCursor(dc_arrow);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void CBuddyButton::OnMouseReleased(vgui::MouseCode code)
{
	if (m_bDragging)
	{
		// make sure we're over a valid panel
		VPanel *imouseOver = input()->GetMouseOver();
		if (imouseOver)
		{
			Panel *mouseOver = ipanel()->Client(imouseOver)->GetPanel();
			KeyValues *keyVal = new KeyValues("DragDrop", "type", "TrackerFriend");
			keyVal->SetInt("friendID", m_iBuddyID);

			if (mouseOver->RequestInfo(keyVal))
			{
				Panel *acceptPanel = (Panel *)keyVal->GetPtr("AcceptPanel");
				if (acceptPanel)
				{
					PostMessage(acceptPanel, keyVal);
					keyVal = NULL;
				}
			}

			if (keyVal)
			{
				keyVal->deleteThis();
			}
		}

		input()->SetMouseCapture(NULL);
	}

	m_bDragging = false;
	
	BaseClass::OnMouseReleased(code);
}


