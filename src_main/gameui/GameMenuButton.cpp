//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "GameMenuButton.h"
// interface to engine
#include "EngineInterface.h"
#include "NewGameDialog.h"
#include "LoadGameDialog.h"
#include "SaveGameDialog.h"
#include "OptionsDialog.h"
#include "CreateMultiplayerGameDialog.h"

#include <vgui_controls/Menu.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>

//#include "GameUISurface.h"
#include "MultiplayerCustomizeDialog.h"
#include "MultiplayerAdvancedDialog.h"
#include "ContentControlDialog.h"
#include "VGuiSystemModuleLoader.h"

#include <assert.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CGameMenuButton *g_pGameMenuButton = NULL;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameMenuButton::CGameMenuButton(vgui::Panel *parent, const char *panelName, const char *text)
								: vgui::MenuButton( parent, panelName, text )
{
	g_pGameMenuButton = this;
	vgui::Menu *gameMenu = NULL;

	// load settings from config file
	KeyValues *datafile = new KeyValues("GameMenu");
	datafile->UsesEscapeSequences( true );
	if (datafile->LoadFromFile(filesystem(), "Resource/GameMenu.res"))
	{
		gameMenu = RecursiveLoadGameMenu(datafile);
		datafile->deleteThis();
	}
	else
	{
		// add a bunch of default stuff
		assert(!("Could not load file Resource/GameMenu.res"));
		gameMenu = new vgui::Menu(this, "GameMenu");
		gameMenu->AddMenuItem( "#GameUI_GameMenu_NewGame ", "OpenNewGameDialog", this );
		gameMenu->AddMenuItem( "#GameUI_GameMenu_LoadGame ", "OpenLoadGameDialog", this );
		gameMenu->AddMenuItem( "SaveGame", "#GameUI_GameMenu_SaveGame ", "OpenSaveGameDialog", this );
		gameMenu->AddMenuItem( "#GameUI_GameMenu_Multiplayer ", "OpenMultiplayerDialog", this );
		gameMenu->AddMenuItem( "#GameUI_GameMenu_Options ", "OpenOptionsDialog", this );
		gameMenu->AddMenuItem( "#GameUI_GameMenu_Quit ", "Quit", this );
	}

	m_pMenu = gameMenu;

	gameMenu->MakePopup();
	MenuButton::SetMenu(gameMenu);
	SetOpenDirection(MenuButton::UP);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameMenuButton::~CGameMenuButton()
{
}

//-----------------------------------------------------------------------------
// Purpose: configures the layout for the control
//-----------------------------------------------------------------------------
void CGameMenuButton::PerformLayout()
{
	BaseClass::PerformLayout();

	Panel *menuItem = m_pMenu->FindChildByName("SaveGame");
	if (!menuItem)
		return;

	const char *lvl = engine->GetLevelName();
	if (lvl && *lvl)
	{
		menuItem->SetEnabled(true);
	}
	else
	{
		menuItem->SetEnabled(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets up the game menu from the keyvalues
//			the game menu is hierarchial, so this is recursive
//-----------------------------------------------------------------------------
vgui::Menu *CGameMenuButton::RecursiveLoadGameMenu(KeyValues *datafile)
{
	Menu *menu = new vgui::Menu(this, datafile->GetName());

	// loop through all the data adding items to the menu
	for (KeyValues *dat = datafile->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		const char *label = dat->GetString("label", "<unknown>");
		const char *cmd = dat->GetString("command", NULL);
		const char *name = dat->GetString("name", label);

		KeyValues *subkeys = dat->FindKey("SubMenu", false);
		Menu *submenu = NULL;
		if (subkeys)
		{
			// it's a hierarchial menu
			submenu = RecursiveLoadGameMenu(subkeys);
		}

		menu->AddCascadingMenuItem(name, label, cmd, this, submenu);
	}

	return menu;
}

//-----------------------------------------------------------------------------
// Purpose: message handler for menu selections
//-----------------------------------------------------------------------------
void CGameMenuButton::OnCommand(const char *command)
{
	if (!stricmp(command, "OpenNewGameDialog"))
	{
		OnOpenNewGameDialog();
	}
	else if (!stricmp(command, "OpenLoadGameDialog"))
	{
		OnOpenLoadGameDialog();
	}
	else if (!stricmp(command, "OpenSaveGameDialog"))
	{
		OnOpenSaveGameDialog();
	}
	else if (!stricmp(command, "OpenOptionsDialog"))
	{
		OnOpenOptionsDialog();
	}
	else if (!stricmp(command, "OpenMultiplayerCustomizeDialog"))
	{
		OnOpenMultiplayerDialog();
	}
	else if (!stricmp(command, "OpenMultiplayerAdvancedDialog"))
	{
		OnOpenMultiplayerAdvancedDialog();
	}
	else if (!stricmp(command, "OpenContentControlDialog"))
	{
		OnOpenContentControlDialog();
	}
	else if (!stricmp(command, "OpenServerBrowser"))
	{
		OnOpenServerBrowser();
	}
	else if (!stricmp(command, "OpenCreateMultiplayerGameDialog"))
	{
		OnOpenCreateMultiplayerGameDialog();
	}
	else if (!stricmp(command, "Quit"))
	{
		engine->ClientCmd("quit\n" );
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: moves the game menu button to the right place on the taskbar
//-----------------------------------------------------------------------------
void CGameMenuButton::PositionDialog(vgui::PHandle dlg)
{
	if (!dlg.Get())
		return;

	int x, y, ww, wt, wide, tall;
	vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
	dlg->GetSize(wide, tall);

	// Center it, keeping requested size
	dlg->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMenuButton::OnOpenMultiplayerDialog()
{
	if ( !m_hMultiplayerCustomizeDialog.Get() )
	{
		m_hMultiplayerCustomizeDialog = new CMultiplayerCustomizeDialog( this );
		PositionDialog( m_hMultiplayerCustomizeDialog );
	}
	m_hMultiplayerCustomizeDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMenuButton::OnOpenMultiplayerAdvancedDialog()
{
	if ( !m_hMultiplayerAdvancedDialog.Get() )
	{
		m_hMultiplayerAdvancedDialog = new CMultiplayerAdvancedDialog();
		PositionDialog( m_hMultiplayerAdvancedDialog );
	}
	m_hMultiplayerAdvancedDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMenuButton::OnOpenContentControlDialog()
{
	if ( !m_hContentControlDialog.Get() )
	{
		m_hContentControlDialog = new CContentControlDialog();
		PositionDialog( m_hContentControlDialog );
	}
	m_hContentControlDialog->Activate();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMenuButton::OnOpenNewGameDialog()
{
	if ( !m_hNewGameDialog.Get() )
	{
		m_hNewGameDialog = new CNewGameDialog();
		PositionDialog( m_hNewGameDialog );
	}
	m_hNewGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMenuButton::OnOpenLoadGameDialog()
{
	if ( !m_hLoadGameDialog.Get() )
	{
		m_hLoadGameDialog = new CLoadGameDialog();
		PositionDialog( m_hLoadGameDialog );
	}
	m_hLoadGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMenuButton::OnOpenSaveGameDialog()
{
	if ( !m_hSaveGameDialog.Get() )
	{
		m_hSaveGameDialog = new CSaveGameDialog();
		PositionDialog( m_hSaveGameDialog );
	}
	m_hSaveGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMenuButton::OnOpenOptionsDialog()
{
	if ( !m_hOptionsDialog.Get() )
	{
		m_hOptionsDialog = new COptionsDialog();
		PositionDialog( m_hOptionsDialog );
	}
	m_hOptionsDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMenuButton::OnOpenServerBrowser()
{
	g_VModuleLoader.ActivateModule("#App_Servers");
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMenuButton::OnOpenCreateMultiplayerGameDialog()
{
	if (!m_hCreateMultiplayerGameDialog.Get())
	{
		m_hCreateMultiplayerGameDialog = new CCreateMultiplayerGameDialog();
		PositionDialog(m_hCreateMultiplayerGameDialog);
	}
	m_hCreateMultiplayerGameDialog->Activate();
}