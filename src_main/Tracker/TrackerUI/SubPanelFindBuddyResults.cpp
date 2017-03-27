//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelFindBuddyResults.h"
#include "ServerSession.h"
#include "Tracker.h"
#include "TrackerDoc.h"
#include "TrackerProtocol.h"

#include <VGUI_WizardPanel.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_ListPanel.h>

#include <VGUI_Controls.h>
#include <VGUI_ILocalize.h>

using namespace vgui;

#include <stdio.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSubPanelFindBuddyResults::CSubPanelFindBuddyResults(vgui::Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	ServerSession().AddNetworkMessageWatch(this, TSVC_FRIENDSFOUND);
	ServerSession().AddNetworkMessageWatch(this, TSVC_NOFRIENDS);

	m_pTable = new ListPanel(this, "Table");
	m_iAttemptID = 0;

	m_pTable->AddColumnHeader(0, "UserName", "User name", 150);
	m_pTable->AddColumnHeader(1, "FirstName", "First name", 150);
	m_pTable->AddColumnHeader(2, "LastName", "Last name", 150);

	m_pInfoText = new Label(this, "InfoText", "");

	LoadControlSettings("Friends/SubPanelFindBuddyResults.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSubPanelFindBuddyResults::~CSubPanelFindBuddyResults()
{
	ServerSession().RemoveNetworkMessageWatch(this);
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the panel to move to next
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelFindBuddyResults::GetNextSubPanel()
{
//	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelFindBuddyRequestAuth"));

	// just skip the request auth dialog for now
	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelFindBuddyComplete"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyResults::OnDisplayAsNext()
{
	GetWizardPanel()->SetNextButtonEnabled(false);

	GetWizardPanel()->SetTitle("#TrackerUI_FriendsSearchingTitle", false);

	m_pInfoText->SetText("#TrackerUI_Searching");

	// reset count and clear table
	m_iFound = 0;
	m_pTable->DeleteAllItems();
	m_pTable->SetVisible(true);

	// post a message to ourselves to finish the search
	m_iAttemptID++;
	PostMessage(this, new KeyValues("NoFriends", "attempt", m_iAttemptID), 5.0f);

	// Start the searching by sending the network message
	KeyValues *doc = GetWizardData();
	ServerSession().SearchForFriend(0, doc->GetString("Email"), doc->GetString("UserName"), doc->GetString("FirstName"), doc->GetString("LastName"));
}

//-----------------------------------------------------------------------------
// Purpose: sets the finish button enabled whenever we're displayed
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyResults::OnDisplay()
{
	GetWizardPanel()->SetFinishButtonEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Network message handler, with details of another friend that has been found.
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyResults::OnFriendFound(KeyValues *friendData)
{
	if ((unsigned int)friendData->GetInt("UID") == GetDoc()->GetUserID())
	{
		// don't show ourselves in results list
		return;
	}

	// add the friend to drop down list
	m_pTable->AddItem(friendData->MakeCopy());

	m_iFound++;
	m_pInfoText->SetText("#TrackerUI_SelectFriendFromList");

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Network message handler, indicating no more friends have been found
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyResults::OnNoFriends(int attemptID)
{
	// make sure this message is from the right attempt
	if (attemptID != m_iAttemptID)
		return;

	if (m_iFound == 0)
	{
		m_iFound = -1;
		m_pInfoText->SetText("#TrackerUI_SearchFailed");
	}

	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyResults::PerformLayout()
{
	if (m_iFound == -1)
	{
		GetWizardPanel()->SetTitle("#TrackerUI_FriendsFinishedSearchTitle", false);
	}
	else if (m_iFound == 1)
	{
		GetWizardPanel()->SetTitle("#TrackerUI_FriendsFoundMatchTitle", false);
	}
	else if (m_iFound > 1)
	{
		char buf[256];
		sprintf(buf, "Friends - Found %d matches", m_iFound);
		GetWizardPanel()->SetTitle(buf, false);

//tagES
//		char number[32];
//		wchar_t unicode[256], unicodeFound[32];
//		itoa( m_iFound, number, 10 );
//		localize()->ConvertANSIToUnicode( number, unicodeFound, sizeof( unicodeFound ) / sizeof( wchar_t ) );
//		localize()->ConstructString(unicode, sizeof( unicode ) / sizeof( wchar_t ), localize()->Find("#TrackerUI_FriendsFoundMatchesTitle"), 1, unicodeFound );
//		GetWizardPanel()->SetTitle(unicode, false);
	}
	else
	{
		GetWizardPanel()->SetTitle("#TrackerUI_FriendsSearchingTitle", false);
		m_pInfoText->SetText("#TrackerUI_Searching");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSubPanelFindBuddyResults::OnNextButton()
{
	// don't advance unless there is a row selected
	if (!m_pTable->GetNumSelectedRows())
		return false;

	// write the data to the doc
	KeyValues *doc = GetWizardData();
	KeyValues *users = doc->FindKey("Users", true);
	users->Clear();

	// walk through
	for (int i = 0; i < m_pTable->GetNumSelectedRows(); i++)
	{
		int row = m_pTable->GetSelectedRow(i);

		KeyValues *user = m_pTable->GetItem(row);
		KeyValues *dest = users->CreateNewKey();

		dest->SetInt("UID", user->GetInt("UID"));
		dest->SetString("UserName", user->GetString("UserName"));
		dest->SetString("FirstName", user->GetString("FirstName"));
		dest->SetString("LastName", user->GetString("LastName"));
	}

	doc->SetInt("UserCount", i);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSubPanelFindBuddyResults::OnRowSelected(int startIndex, int endIndex)
{
	if (startIndex > -1)
	{
		// they've selected an item so allow the user to move forward
		GetWizardPanel()->SetNextButtonEnabled(true);
	}
	else
	{
		// nothing selected, so no next
		GetWizardPanel()->SetNextButtonEnabled(false);
	}

	GetWizardPanel()->ResetDefaultButton();
}


//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelFindBuddyResults::m_MessageMap[] =
{
// obseleted
//	MAP_MSGID( CSubPanelFindBuddyResults, TSVC_NOFRIENDS, OnNoFriends ),
	MAP_MSGID_PARAMS( CSubPanelFindBuddyResults, TSVC_FRIENDSFOUND, OnFriendFound ),

	MAP_MESSAGE_INT( CSubPanelFindBuddyResults, "NoFriends", OnNoFriends, "attempt" ),

	MAP_MESSAGE_INT_INT( CSubPanelFindBuddyResults, "RowSelected", OnRowSelected, "startIndex", "endIndex" ),
};
IMPLEMENT_PANELMAP( CSubPanelFindBuddyResults, Panel );

