//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "DemoPage.h"

#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include <VGUI_MenuBar.h>
#include <VGUI_MenuButton.h>
#include <VGUI_Menu.h> 

using namespace vgui;

//-----------------------------------------------------------------------------
// A MenuBar
//-----------------------------------------------------------------------------

class MenuBarDemo: public DemoPage
{
	public:
		MenuBarDemo(Panel *parent, const char *name);
		~MenuBarDemo();
		
	private:
		MenuBar *m_pMenuBar;
		
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
MenuBarDemo::MenuBarDemo(Panel *parent, const char *name) : DemoPage(parent, name)
{
	m_pMenuBar = new MenuBar(this, "AMenuBar");
	m_pMenuBar->SetPos(0, 20);

	// Make a couple menus and attach them in.

	// A menu 
	MenuButton *pMenuButton = new MenuButton(this, "FileMenuButton", "&File");
	Menu *pMenu = new Menu(pMenuButton, "AMenu");
	pMenu->AddMenuItem("&New",  new KeyValues ("NewFile"), this);
	pMenu->AddMenuItem("&Open",  new KeyValues ("OpenFile"), this);
	pMenu->AddMenuItem("&Save",  new KeyValues ("SaveFile"), this);
	pMenuButton->SetMenu(pMenu);

	m_pMenuBar->AddButton(pMenuButton);

	// A menu 
	pMenuButton = new MenuButton(this, "EditMenuButton", "&Edit");
	pMenu = new Menu(pMenuButton, "AMenu");
	pMenu->AddMenuItem("&Undo",  new KeyValues ("Undo"), this);
	pMenu->AddMenuItem("&Find",  new KeyValues ("Find"), this);
	pMenu->AddMenuItem("Select&All",  new KeyValues ("SelectAll"), this);
	pMenuButton->SetMenu(pMenu);

	m_pMenuBar->AddButton(pMenuButton);

	// A menu 
	pMenuButton = new MenuButton(this, "ViewMenuButton", "&View");
	pMenu = new Menu(pMenuButton, "AMenu");
	pMenu->AddMenuItem("&FullScreen",  new KeyValues ("FullScreen"), this);
	pMenu->AddMenuItem("&SplitScreen",  new KeyValues ("SplitScreen"), this);
	pMenu->AddMenuItem("&Properties",  new KeyValues ("Properties"), this);
	pMenu->AddMenuItem("&Output",  new KeyValues ("Output"), this);
	pMenuButton->SetMenu(pMenu);

	m_pMenuBar->AddButton(pMenuButton);

	int bwide, btall;
	pMenuButton->GetSize( bwide, btall);
	int wide, tall;
	GetParent()->GetSize(wide, tall);
	m_pMenuBar->SetSize(wide - 2, btall + 8);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
MenuBarDemo::~MenuBarDemo()
{
}


Panel* MenuBarDemo_Create(Panel *parent)
{
	return new MenuBarDemo(parent, "MenuBarDemo");
}


