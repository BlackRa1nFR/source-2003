//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <VGUI_WizardPanel.h>
#include <VGUI_KeyValues.h>

#include "SubPanelConnectionTest.h"
#include "ServerSession.h"
#include "TrackerProtocol.h"

using namespace vgui;

#define PING_TIMEOUT_TIME_SECONDS 8.1f

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelConnectionTest::CSubPanelConnectionTest(vgui::Panel *parent, const char *panelName) : WizardSubPanel(parent, panelName)
{
	m_iCurrentPingID = 0;
	m_bConnectFailed = false;
	m_bServerFound = false;

	ServerSession().AddNetworkMessageWatch(this, TSVC_PINGACK);

	LoadControlSettings("Friends/SubPanelConnectionTest.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelConnectionTest::~CSubPanelConnectionTest()
{
}

//-----------------------------------------------------------------------------
// Purpose: returns the next panel to let the user Start logging in
//-----------------------------------------------------------------------------
WizardSubPanel *CSubPanelConnectionTest::GetNextSubPanel()
{
	return dynamic_cast<WizardSubPanel *>(GetWizardPanel()->FindChildByName("SubPanelSelectLoginOption"));
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the to Start
//-----------------------------------------------------------------------------
void CSubPanelConnectionTest::OnDisplayAsNext()
{
	// Start the refresh
	StartServerSearch();
}

//-----------------------------------------------------------------------------
// Purpose: sets up layout
//-----------------------------------------------------------------------------
void CSubPanelConnectionTest::PerformLayout()
{
	BaseClass::PerformLayout();

	GetWizardPanel()->SetTitle("#TrackerUI_TestingNetworkConnectionTitle", true);

	GetWizardPanel()->SetFinishButtonEnabled(false);
	GetWizardPanel()->SetCancelButtonEnabled(true);

	// set the text that we've succeeded	
	GetWizardPanel()->SetNextButtonEnabled(m_bServerFound);
	FindChildByName("RetryButton")->SetEnabled(true);
	if (m_bServerFound)
	{
		SetControlString("InfoText", "#TrackerUI_AttemptingToConnect_Success");
		SetControlString("DescriptionText", "#TrackerUI_SuccessfullyConnectedClickNext");
	}
	else if (m_bConnectFailed)
	{
		SetControlString("InfoText", "#TrackerUI_AttemptingToConnect_Failure");
		SetControlString("DescriptionText", "#TrackerUI_FailedToConnect_CheckFirewall");
	}
	else
	{
		SetControlString("InfoText", "#TrackerUI_AttemptingToConnect");
		SetControlString("DescriptionText", "");
		FindChildByName("RetryButton")->SetEnabled(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Starts the searching for a server, and updates the UI appropriately
//-----------------------------------------------------------------------------
void CSubPanelConnectionTest::StartServerSearch()
{
	// Start looking for a server to connect to while the user enters their info
	ServerSession().UnconnectedSearchForServer();

	m_iCurrentPingID++;
	m_bConnectFailed = false;
	m_bServerFound = false;

	// post a timeout message to ourselves
	PostMessage(this, new KeyValues("PingTimeout", "pingID", m_iCurrentPingID), PING_TIMEOUT_TIME_SECONDS);

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Handles ping acknowledged message
//-----------------------------------------------------------------------------
void CSubPanelConnectionTest::OnPingAck()
{
	// increment the ping id
	m_iCurrentPingID++;
	m_bServerFound = true;

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Handles a command from a button
//-----------------------------------------------------------------------------
void CSubPanelConnectionTest::OnCommand(const char *command)
{
	if (!stricmp(command, "RetryConnect"))
	{
		StartServerSearch();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Checks to see if the attempt at connection timed out
// Input  : pingID - identifier of the ping
//-----------------------------------------------------------------------------
void CSubPanelConnectionTest::OnPingTimeout(int pingID)
{
	if (pingID == m_iCurrentPingID)
	{
		m_bConnectFailed = true;
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CSubPanelConnectionTest::m_MessageMap[] =
{
	MAP_MSGID( CSubPanelConnectionTest, TSVC_PINGACK, OnPingAck ),		// network message
	MAP_MESSAGE_INT( CSubPanelConnectionTest, "PingTimeout", OnPingTimeout, "pingID" ),		// network message
};
IMPLEMENT_PANELMAP(CSubPanelConnectionTest, BaseClass);
