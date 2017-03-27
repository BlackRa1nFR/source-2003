//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "ServerInfoPanel.h"
#include "ServerList.h"

#include <VGUI_Controls.h>
#include <VGUI_IScheme.h>
#include <VGUI_TextEntry.h>

#include <stdio.h>

using namespace vgui;

// server list
extern CServerList *g_pServerList;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServerInfoPanel::CServerInfoPanel(Panel *parent, const char *name) : EditablePanel(parent, name)
{
	m_pText = new TextEntry(this, "ServerText");
	m_pText->SetMultiline(true);
	m_pText->SetRichEdit(true);
	m_pText->SetEditable(false);
	m_pText->SetVerticalScrollbar(true);
	m_iServerID = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServerInfoPanel::~CServerInfoPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : serverID - 
//-----------------------------------------------------------------------------
void CServerInfoPanel::SetServerID(int serverID)
{
	m_iServerID = serverID;
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Lays out controls
//-----------------------------------------------------------------------------
void CServerInfoPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	Color col1 = GetSchemeColor("DimBaseText");
	Color col2 = GetSchemeColor("BaseText");
	Color col3 = GetSchemeColor("BrightBaseText");

	SetBorder(scheme()->GetBorder(scheme()->GetDefaultScheme(), "BaseBorder"));

	int wide, tall;
	GetSize(wide, tall);

	m_pText->SetPos(5, 5);
	m_pText->SetSize(wide - 10, tall - 10);

	// reset text
	m_pText->SetText("");

	// rebuild text string from server info
	server_t &server = g_pServerList->GetServer(m_iServerID);

	// server name
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("Server: ");
	m_pText->DoInsertColorChange(col2);
	m_pText->DoInsertString(server.name);
	m_pText->DoInsertString("\n");

	// server status
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("Status: ");

	switch (server.state)
	{
	case SERVER_ACTIVE:
		if (server.underHeavyLoad)
		{
			m_pText->DoInsertColorChange(Color(0, 255, 0, 0));
			m_pText->DoInsertString("Under Heavy Load");
		}
		else
		{
			m_pText->DoInsertColorChange(col2);
			m_pText->DoInsertString("Active");
		}
		break;

	case SERVER_DOWN:
		m_pText->DoInsertColorChange(col3);
		m_pText->DoInsertString("Inactive");
		break;

	case SERVER_NOTRESPONDING:
		m_pText->DoInsertColorChange(Color(255, 100, 100, 0));
		m_pText->DoInsertString("NOT RESPONDING");
		break;

	case SERVER_SHUTTINGDOWN:
		m_pText->DoInsertColorChange(col3);
		m_pText->DoInsertString("Shutting down");
		break;

	case SERVER_UNKNOWN:
	default:
		m_pText->DoInsertColorChange(Color(255, 100, 100, 0));
		m_pText->DoInsertString("UNKNOWN");
		break;
	};

	m_pText->DoInsertString("\n");

	// only continue drawing if we have an active server
	if (server.state != SERVER_ACTIVE && server.state != SERVER_SHUTTINGDOWN)
		return;

	// primary
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("Primary: ");
	if (server.primary)
	{
		m_pText->DoInsertColorChange(col3);
		m_pText->DoInsertString("true");
	}
	else
	{
		m_pText->DoInsertColorChange(col2);
		m_pText->DoInsertString("false");
	}
	m_pText->DoInsertString("\n");

	// user count
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("Users: ");
	m_pText->DoInsertColorChange(col2);
	char buf[64];
	sprintf(buf, "%d / %d", server.users, server.maxUsers);
	m_pText->DoInsertString(buf);
	m_pText->DoInsertString("\n");

	// fps
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("Fps: ");
	m_pText->DoInsertColorChange(col2);
	if (server.fps > 0)
	{
		sprintf(buf, "%d", server.fps);
	}
	else
	{
		strcpy(buf, "sleeping");
	}
	m_pText->DoInsertString(buf);
	m_pText->DoInsertString("\n");


	// network buffers
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("Open network buffers: ");
	m_pText->DoInsertColorChange(col2);
	sprintf(buf, "%d    ", server.networkBuffers);
	m_pText->DoInsertString(buf);
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("Peak: ");
	m_pText->DoInsertColorChange(col2);
	sprintf(buf, "%d", server.peakNetworkBuffers);
	m_pText->DoInsertString(buf);
	m_pText->DoInsertString("\n");

	// sqldb queries
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("Open sql queries: ");
	m_pText->DoInsertColorChange(col2);
	sprintf(buf, "%d    ", server.dbOutBufs);
	m_pText->DoInsertString(buf);
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("finished but unproccesed: ");
	m_pText->DoInsertColorChange(col2);
	sprintf(buf, "%d", server.dbInBufs);
	m_pText->DoInsertString(buf);
	m_pText->DoInsertString("\n");

	// bandwidth
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("Bandwidth usage:  receiving ");
	m_pText->DoInsertColorChange(col2);
	sprintf(buf, "%d bytes/second    ", server.bytesReceivedPerSecond);
	m_pText->DoInsertString(buf);
	m_pText->DoInsertColorChange(col1);
	m_pText->DoInsertString("sending ");
	m_pText->DoInsertColorChange(col2);
	sprintf(buf, "%d bytes/second", server.bytesSentPerSecond);
	m_pText->DoInsertString(buf);
	m_pText->DoInsertString("\n");
}
