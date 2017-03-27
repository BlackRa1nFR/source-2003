//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "ServerContextMenu.h"

#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServerContextMenu::CServerContextMenu(Panel *parent) : Menu(parent, "ServerContextMenu")
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServerContextMenu::~CServerContextMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: Activates the menu
//-----------------------------------------------------------------------------
void CServerContextMenu::ShowMenu(Panel *target, unsigned int serverID, bool showConnect, bool showRefresh, bool showAddToFavorites)
{
	if (showConnect)
	{
		AddMenuItem("ConnectToServer", "&Connect to server", new KeyValues("ConnectToServer", "serverID", serverID), target);
		AddMenuItem("ViewGameInfo", "&View server info", new KeyValues("ViewGameInfo", "serverID", serverID), target);
	}

	if (showRefresh)
	{
		AddMenuItem("RefreshServer", "&Refresh server", new KeyValues("RefreshServer", "serverID", serverID), target);
	}

	if (showAddToFavorites)
	{
		AddMenuItem("AddToFavorites", "&Add server to favorites", new KeyValues("AddToFavorites", "serverID", serverID), target);
	}

	int x, y, gx, gy;
	input()->GetCursorPos(x, y);
	ipanel()->GetPos(surface()->GetEmbeddedPanel(), gx, gy);
	SetPos(x - gx, y - gy);
	SetVisible(true);
}
