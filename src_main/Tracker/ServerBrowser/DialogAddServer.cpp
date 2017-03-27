//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "DialogAddServer.h"
#include "INetAPI.h"
#include "IGameList.h"
#include "Server.h"

#include <KeyValues.h>
#include <vgui_controls/MessageBox.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *gameList - game list to add specified server to
//-----------------------------------------------------------------------------
CDialogAddServer::CDialogAddServer(vgui::Panel *parent, IGameList *gameList) : Frame(parent, "DialogAddServer")
{
	SetSizeable(false);

	m_pGameList = gameList;

	SetTitle("#ServerBrowser_AddServersTitle", true);

	LoadControlSettings("Servers/DialogAddServer.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogAddServer::~CDialogAddServer()
{
}

//-----------------------------------------------------------------------------
// Purpose: button command handler
//-----------------------------------------------------------------------------
void CDialogAddServer::OnCommand(const char *command)
{
	if (!stricmp(command, "OK"))
	{
		OnOK();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles the OK button being pressed; adds the server to the game list
//-----------------------------------------------------------------------------
void CDialogAddServer::OnOK()
{
	// try and parse out IP address
	const char *address = GetControlString("ServerNameText", "");
	netadr_t netaddr;
	if (net->StringToAdr(address, &netaddr))
	{
		// net address successfully parsed, add the server to the game list
		serveritem_t server;
		memset(&server, 0, sizeof(server));
		for (int i = 0; i < 4; i++)
		{
			server.ip[i] = netaddr.ip[i];
		}
		server.port = (netaddr.port & 0xff) << 8 | (netaddr.port & 0xff00) >> 8;;
		if (!server.port)
		{
			// use the default port since it was not entered
			server.port = 27015;
		}
		m_pGameList->AddNewServer(server);
		m_pGameList->StartRefresh();
	}
	else
	{
		// could not parse the ip address, popup an error
		MessageBox *dlg = new MessageBox("#ServerBrowser_AddServerErrorTitle", "#ServerBrowser_AddServerError");
		dlg->DoModal();
	}

	// mark ourselves to be closed
	PostMessage(this, new KeyValues("Close"));
}

//-----------------------------------------------------------------------------
// Purpose: Deletes dialog on close
//-----------------------------------------------------------------------------
void CDialogAddServer::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}
