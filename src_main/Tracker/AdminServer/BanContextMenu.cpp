//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BanContextMenu.h"

#include <VGUI_Controls.h>
#include <VGUI_IInput.h>
#include <VGUI_IPanel.h>
#include <VGUI_ISurface.h>
#include <VGUI_KeyValues.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBanContextMenu::CBanContextMenu(Panel *parent) : Menu(parent, "BanContextMenu")
{
	CBanContextMenu::parent=parent;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBanContextMenu::~CBanContextMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: Activates the menu
//-----------------------------------------------------------------------------
void CBanContextMenu::ShowMenu(Panel *target, unsigned int banID)
{
	ClearMenu();

	if(banID==-1) 
	{
		AddMenuItem("ban", "&Add Ban", new KeyValues("addban", "banID", banID), CBanContextMenu::parent);
	} 
	else
	{
		AddMenuItem("ban", "&Remove Ban", new KeyValues("removeban", "banID", banID), CBanContextMenu::parent);
		AddMenuItem("ban", "&Change Time", new KeyValues("changeban", "banID", banID), CBanContextMenu::parent);
	}

	MakePopup();
	int x, y, gx, gy;
	input()->GetCursorPos(x, y);
	ipanel()->GetPos(surface()->GetEmbeddedPanel(), gx, gy);
	SetPos(x - gx, y - gy);
	MoveToFront();
	RequestFocus();
	SetVisible(true);
}
