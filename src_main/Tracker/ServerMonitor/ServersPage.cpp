//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "ServersPage.h"
#include "ServerList.h"

using namespace vgui;

extern CServerList *g_pServerList;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServersPage::CServersPage()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServersPage::~CServersPage()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets up control positions
//-----------------------------------------------------------------------------
void CServersPage::PerformLayout()
{
	BaseClass::PerformLayout();

	int x = 10, y = 10, wide, tall;
	GetSize(wide, tall);
	tall -= (y * 2);
	int serverCount = g_pServerList->ServerCount();
	int paneTall = serverCount > 0 ? tall / serverCount : 0;

	// loop through the servers in the server list, create UI elements for each
	for (int i = 0; i < serverCount; i++)
	{
		CServerInfoPanel *pane;
		if (m_Panels.Size() > i)
		{
			// panel already exists
			pane = m_Panels[i];
		}
		else
		{
			// create a new panel
			int index = m_Panels.AddToTail(new CServerInfoPanel(this, "ServerInfo"));
			pane = m_Panels[index];
		}

		pane->SetServerID(i);

		pane->SetPos(x, y);
		pane->SetSize(wide - 20, paneTall - 4);

		y += paneTall;
	}
}

