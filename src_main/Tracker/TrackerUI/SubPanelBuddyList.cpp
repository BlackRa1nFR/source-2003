//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_ScrollBar.h>

#include "Buddy.h"
#include "BuddyButton.h"
#include "BuddySectionHeader.h"
#include "DialogChat.h"
#include "IRunGameEngine.h"
#include "OnlineStatus.h"
#include "SubPanelBuddyList.h"
#include "Tracker.h"
#include "TrackerDoc.h"
#include "ServerSession.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelBuddyList::CSubPanelBuddyList(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	m_pOnlineLabel = new CBuddySectionHeader(this, "#TrackerUI_OnlineTitle");
	m_pOfflineLabel = new CBuddySectionHeader(this, "#TrackerUI_OfflineTitle");
	m_pSystemLabel = new CBuddySectionHeader(this, "#TrackerUI_SystemTitle");
	m_pIngameLabel = new CBuddySectionHeader(this, "#TrackerUI_InGameTitle");
	m_pCurrentGameLabel = new CBuddySectionHeader(this, "#TrackerUI_CurrentGameTitle");
	m_pUnknownLabel = new CBuddySectionHeader(this, "#TrackerUI_UnknownUsersTitle");
	m_pRequestingAuthLabel = new CBuddySectionHeader(this, "#TrackerUI_RequestingAuthTitle");
	m_pAwaitingAuthLabel = new CBuddySectionHeader(this, "#TrackerUI_AwaitingAuthTitle");

	m_pScrollBar = new ScrollBar(this, "BuddyListScrollBar", true);
	m_pScrollBar->SetVisible(false);
	m_pScrollBar->AddActionSignalTarget(this);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelBuddyList::~CSubPanelBuddyList()
{
}

//-----------------------------------------------------------------------------
// Purpose: Associates the user list chat dialog
// Input  : *chatDialog - 
//-----------------------------------------------------------------------------
void CSubPanelBuddyList::AssociateWithChatDialog(CDialogChat *chatDialog)
{
	m_hChatDialog = chatDialog;
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Clears the buddy list
//-----------------------------------------------------------------------------
void CSubPanelBuddyList::Clear()
{
	// clear data
	for (int i = 0; i < m_BuddyButtonDar.GetCount(); i++)
	{
		CBuddyButton *buddy = m_BuddyButtonDar[i];
		delete buddy;
	}
	m_BuddyButtonDar.RemoveAll();

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: forces all the buttons to redraw
//-----------------------------------------------------------------------------
void CSubPanelBuddyList::InvalidateAll()
{
	// Invalidate all the buttons
	for (int i = 0; i < m_BuddyButtonDar.GetCount(); i++)
	{
		m_BuddyButtonDar[i]->InvalidateLayout();
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Moves all the buddy buttons into position
//-----------------------------------------------------------------------------
void CSubPanelBuddyList::PerformLayout()
{
	int ourStatus = ServerSession().GetStatus();

	// loop through the buddies in the registry
	KeyValues *buddyData = GetDoc()->Data()->FindKey("User/Buddies", true);
	for (KeyValues *dat = buddyData->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		// get the buddies name
		unsigned int buddyID = atoi(dat->GetName());
		if ( !buddyID )
			continue;

		// check to see if the buddy is in the chat (if any)
		// if not in the chat, skip over them
		if (m_hChatDialog.Get() && !(m_hChatDialog->IsUserInChat(buddyID)))
			continue;

		// don't display self in list
		if (buddyID == GetDoc()->GetUserID())
			continue;

		// don't display people we're not authorized to
		if (dat->GetInt("NotAuthed"))
			continue;

		// check to see if that buddy is in the list
		bool buttonFound = false;
		for ( int i = 0; i < m_BuddyButtonDar.GetCount(); i++ )
		{
			if ( m_BuddyButtonDar[i]->GetBuddyID() == buddyID )
			{
				// we already have the button
				buttonFound = true;
				break;
			}
		}

		if ( !buttonFound )
		{
			// create the button
			CBuddyButton *newBuddy = new CBuddyButton(this, buddyID);
			newBuddy->RefreshBuddyStatus();
			newBuddy->InvalidateLayout(true, false);
			newBuddy->Repaint();

			// insert into list in alphabetical order
			bool bInserted = false;
			const char *newName = dat->GetString("DisplayName");
			for (int i = 0; i < m_BuddyButtonDar.GetCount(); i++)
			{
				const char *buddyName = GetDoc()->GetBuddy(m_BuddyButtonDar[i]->GetBuddyID())->DisplayName();
				if (stricmp(newName, buddyName) < 0)
				{
					m_BuddyButtonDar.InsertElementAt(newBuddy, i);
					bInserted = true;
					break;
				}
			}

			if (!bInserted)
			{
				m_BuddyButtonDar.PutElement(newBuddy);
			}
		}
	}

	// sort the buddy buttons into different categories
	panellist_t currentgameBuddies, ingameBuddies, onlineBuddies, offlineBuddies, awaitingAuthBuddies, unknownBuddies, requestingAuthBuddies;

	for (int i = 0; i < m_BuddyButtonDar.GetCount(); i++)
	{
		CBuddyButton *buddy = m_BuddyButtonDar[i];

		buddy->RefreshBuddyStatus();
		int buddyStatus = buddy->GetBuddyStatus();
		buddy->SetVisible(true);

		if (m_hChatDialog.Get() && !m_hChatDialog->IsUserInChat(buddy->GetBuddyID()))
		{
			// user is no longer in chat, hide button
			buddy->SetVisible(false);
		}
		else if (buddyStatus == COnlineStatus::REMOVED)
		{
			// do nothing with removed lads
			buddy->SetVisible(false);
		} 
		else if (buddyStatus == COnlineStatus::REQUESTINGAUTHORIZATION)
		{
			requestingAuthBuddies.PutElement(buddy);
		}
		/* Just showing users we are awaiting authorization from as offline
		else if (buddyStatus == COnlineStatus::AWAITINGAUTHORIZATION)
		{
			awaitingAuthBuddies.PutElement(buddy);
		}
		*/
		else if (buddyStatus == COnlineStatus::UNKNOWNUSER)
		{
			unknownBuddies.PutElement(buddy);
		}
		else if (buddyStatus == COnlineStatus::CHATUSER)
		{
			// only show chat users in the list if we're associated with a chat dialog
			if (m_hChatDialog)
			{
				unknownBuddies.PutElement(buddy);
			}
			else
			{
				buddy->SetVisible(false);
			}
		}
		else if (buddyStatus == COnlineStatus::INGAME)
		{
			// check to see if we're in the same game
			if (ourStatus == COnlineStatus::INGAME)
			{
				buddyData = GetDoc()->GetBuddy(buddy->GetBuddyID())->Data();

				if (Tracker_GetRunGameEngineInterface()->GetUserName(buddy->GetBuddyID()))
				{
					currentgameBuddies.PutElement(buddy);
					continue;
				}
			}

			ingameBuddies.PutElement(buddy);
		}
		else if (buddyStatus > 0)
		{
			if (GetDoc()->GetBuddy(buddy->GetBuddyID())->Data()->GetInt("RemoteBlock"))
			{
				buddyStatus = 99;
			}

			// insert according to status
			bool bAdded = false;
			for (int i = 0; i < onlineBuddies.GetCount(); i++)
			{
				int statusLevel = onlineBuddies[i]->GetBuddyStatus();
				if (GetDoc()->GetBuddy(onlineBuddies[i]->GetBuddyID())->Data()->GetInt("RemoteBlock"))
				{
					statusLevel = 99;
				}

				if (buddyStatus < statusLevel)
				{
					onlineBuddies.InsertElementAt(buddy, i);
					bAdded = true;
					break;
				}
			}

			if (!bAdded)
			{
				onlineBuddies.PutElement(buddy);
			}
		}
		else
		{
			offlineBuddies.PutElement(buddy);
		}
	}

	// reposition buddy buttons
	int cx, cy, cwide, ctall;
	GetBounds(cx, cy, cwide, ctall);

	int x = 2, y = 2;
	int wide = cwide - 10;
	int tall = 20;

	m_iItemHeight = tall;

	if (m_pScrollBar->IsVisible())
	{
		y -= m_pScrollBar->GetValue();		
		wide -= m_pScrollBar->GetWide();
	}

	LayoutButtons(x, y, wide, tall, currentgameBuddies, m_pCurrentGameLabel);
	LayoutButtons(x, y, wide, tall, ingameBuddies, m_pIngameLabel);
	LayoutButtons(x, y, wide, tall, onlineBuddies, m_pOnlineLabel);
	LayoutButtons(x, y, wide, tall, offlineBuddies, m_pOfflineLabel);
	LayoutButtons(x, y, wide, tall, requestingAuthBuddies, m_pRequestingAuthLabel);
	LayoutButtons(x, y, wide, tall, unknownBuddies, m_pUnknownLabel);
	LayoutButtons(x, y, wide, tall, awaitingAuthBuddies, m_pAwaitingAuthLabel);

	// setup scrollbar
	if (m_pScrollBar->IsVisible())
	{
		y += m_pScrollBar->GetValue();
	}

	int scrollBarSize;
	if (y > ctall)
	{
		if (!m_pScrollBar->IsVisible())
		{
			// since we're just about to make the scrollbar visible, we need to re-layout
			// the buttons since they depend on the scrollbar size
			InvalidateLayout();
		}

		m_pScrollBar->SetVisible(true);
		m_pScrollBar->MoveToFront();

		m_pScrollBar->SetPos(cwide - 20, 0);
		m_pScrollBar->SetSize(18, ctall - 2);

		scrollBarSize = m_pScrollBar->GetWide();

		m_pScrollBar->SetRangeWindow(ctall);

		m_pScrollBar->SetRange(0, y);
		m_pScrollBar->InvalidateLayout();
		m_pScrollBar->Repaint();
	}
	else
	{
		m_pScrollBar->SetValue(0);
		m_pScrollBar->SetVisible(false);
		scrollBarSize = 0;
	}
	
	// make the buttons as wide as the frame
	int xpos = (cwide - 5) - scrollBarSize;
	wide -= 2;
	m_pOnlineLabel->GetSize(wide, tall);
	m_pOnlineLabel->SetSize(xpos, tall);
	m_pOfflineLabel->GetSize(wide, tall);
	m_pOfflineLabel->SetSize(xpos, tall);
	m_pIngameLabel->GetSize(wide, tall);
	m_pIngameLabel->SetSize(xpos, tall);
	m_pCurrentGameLabel->GetSize(wide, tall);
	m_pCurrentGameLabel->SetSize(xpos, tall);
	m_pSystemLabel->GetSize(wide, tall);
	m_pSystemLabel->SetSize(xpos, tall);
	m_pRequestingAuthLabel->GetSize(wide, tall);
	m_pRequestingAuthLabel->SetSize(xpos, tall);
	m_pAwaitingAuthLabel->GetSize(wide, tall);
	m_pAwaitingAuthLabel->SetSize(xpos, tall);
	m_pUnknownLabel->GetSize(wide, tall);
	m_pUnknownLabel->SetSize(xpos, tall);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelBuddyList::LayoutButtons(int &x, int &y, int wide, int tall, panellist_t &buddies, Label *label)
{
	// if there are no buddies, don't even drawn the section header
	if (!buddies.GetCount())
	{
		label->SetVisible(false);
		return;
	}

	// add in a gap above the section header
	y += 6;

	// section header
	label->SetPos(x, y);
	label->SetVisible(true);
	y += label->GetTall();

	// button list
	int buttonOffset = 0;
	for (int i = 0; i < buddies.GetCount(); i++)
	{
		buddies[i]->SetPos(x + buttonOffset, y);
		buddies[i]->SetSize(wide - buttonOffset, tall);
		y += tall;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelBuddyList::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// set background color
	SetBgColor(GetSchemeColor("BuddyListBgColor", GetBgColor()));
}


//-----------------------------------------------------------------------------
// Purpose: Called when the scrollbar is moved
//-----------------------------------------------------------------------------
void CSubPanelBuddyList::OnSliderMoved()
{
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Scrolls the list according to the mouse wheel movement
//-----------------------------------------------------------------------------
void CSubPanelBuddyList::OnMouseWheeled(int delta)
{
	int val = m_pScrollBar->GetValue();
	val -= (delta * m_iItemHeight * 3);
	m_pScrollBar->SetValue(val);
}

//-----------------------------------------------------------------------------
// Purpose: Resets the scrollbar position on size change
//-----------------------------------------------------------------------------
void CSubPanelBuddyList::SetSize(int wide, int tall)
{
	Panel::SetSize(wide, tall);
	m_pScrollBar->SetValue(0);
	InvalidateLayout();
	Repaint();
}


//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelBuddyList::m_MessageMap[] =
{
	MAP_MESSAGE( CSubPanelBuddyList, "ScrollBarSliderMoved", OnSliderMoved ),
};
IMPLEMENT_PANELMAP(CSubPanelBuddyList, Panel);


