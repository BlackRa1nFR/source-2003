//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "Taskbar.h"
#include "TaskButton.h"
#include "VGuiSystemModuleLoader.h"
#include "GameUI_Interface.h"
#include "EngineInterface.h"
#include "GameConsole.h"

#include "NewGameDialog.h"
#include "LoadGameDialog.h"
#include "SaveGameDialog.h"
#include "OptionsDialog.h"
#include "CreateMultiplayerGameDialog.h"
//#include "DemoPlayerDialog.h"
#include "ChangeGameDialog.h"
#include "BackgroundMenuButton.h"
#include "PlayerListDialog.h"

#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/QueryBox.h>
#include "vstdlib/icommandline.h"

#include "GameConsole.h"

#include "igameuifuncs.h"
#include "keydefs.h"

using namespace vgui;

#include "BaseUI/IBaseUI.h"
extern IBaseUI *baseuifuncs;

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define BLACK_BAR_SIZE	64
#define BLACK_BAR_COLOR	Color(0, 0, 0, 128)
//!! testing black bars being disabled
// #define BLACK_BAR_COLOR	Color(0, 0, 0, 0)

#define MENU_BUTTON_WIDE 240
#define TASK_BUTTON_HEIGHT 24
#define TASK_BUTTON_INSET 6

#define MENU_ITEM_HEIGHT 64

#define MENU_ITEM_VISIBILITY_RATE 0.05f

//-----------------------------------------------------------------------------
// Purpose: Transparent menu item designed to sit on the background ingame
//-----------------------------------------------------------------------------
class CGameMenuItem : public vgui::MenuItem
{
public:
	CGameMenuItem(vgui::Panel *parent, const char *name)  : BaseClass(parent, name, "GameMenuItem") 
	{
		m_bRightAligned = false;
		SetPaintEnabled(false);
	}

	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		// make fully transparent
		SetFgColor(GetSchemeColor("InGameDesktop/MenuColor", pScheme));
		SetBgColor(Color(0, 0, 0, 0));
		SetDefaultColor(GetSchemeColor("InGameDesktop/MenuColor", pScheme), Color(0, 0, 0, 0));
		SetArmedColor(GetSchemeColor("InGameDesktop/ArmedMenuColor", pScheme), Color(0, 0, 0, 0));
		SetDepressedColor(GetSchemeColor("InGameDesktop/DepressedMenuColor", pScheme), Color(0, 0, 0, 0));
		SetContentAlignment(Label::a_west);
		SetBorder(NULL);
		SetDefaultBorder(NULL);
		SetDepressedBorder(NULL);
		SetKeyFocusBorder(NULL);
		SetFont(pScheme->GetFont( "MenuLarge", IsProportional()));
		SetTextInset(0, 0);
		SetArmedSound("sound/UI/buttonrollover.wav");
		SetDepressedSound("sound/UI/buttonclick.wav");
		SetReleasedSound("sound/UI/buttonclickrelease.wav");
		SetButtonActivationType(Button::ACTIVATE_ONPRESSED);

		if (m_bRightAligned)
		{
			SetContentAlignment(Label::a_east);
		}
	}

	void SetRightAlignedText(bool state)
	{
		m_bRightAligned = state;
	}

private:
	bool m_bRightAligned;

	typedef vgui::MenuItem BaseClass;
};

//-----------------------------------------------------------------------------
// Purpose: invisible menu
//-----------------------------------------------------------------------------
class CGameMenu : public vgui::Menu
{
public:
	CGameMenu(vgui::Panel *parent, const char *name) : vgui::Menu(parent, name) 
	{
		m_bRestartListUpdate = false;
		m_bUpdatingList = false;
		m_flListUpdateStartTime = 0.0f;
	}

	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		// make fully transparent
		SetMenuItemHeight(atoi(pScheme->GetResourceString("InGameDesktop/MenuItemHeight")));
		SetBgColor(Color(0, 0, 0, 0));
		SetBorder(NULL);

		m_flMenuItemVisibilityRate = (float)atof(pScheme->GetResourceString("InGameDesktop/MenuItemVisibilityRate"));
		if (m_flMenuItemVisibilityRate < 0.0000001)
		{
			m_flMenuItemVisibilityRate = 0.0000001f;
		}
	}

	virtual void LayoutMenuBorder()
	{
	}

	virtual void SetVisible(bool state)
	{
		// force to be always visible
		BaseClass::SetVisible(true);
		// move us to the back instead of going invisible
		if (!state)
		{
			ipanel()->MoveToBack(GetVPanel());
		}
	}

	virtual int AddMenuItem(const char *itemName, const char *itemText, const char *command, Panel *target, KeyValues *userData = NULL)
	{
		MenuItem *item = new CGameMenuItem(this, itemName);
		item->AddActionSignalTarget(target);
		item->SetCommand(command);
		item->SetText(itemText);
		item->SetUserData(userData);
		return BaseClass::AddMenuItem(item);
	}

	virtual int AddMenuItem(const char *itemName, const char *itemText, KeyValues *command, Panel *target, KeyValues *userData = NULL)
	{
		CGameMenuItem *item = new CGameMenuItem(this, itemName);
		item->AddActionSignalTarget(target);
		item->SetCommand(command);
		item->SetText(itemText);
		item->SetRightAlignedText(true);
		item->SetUserData(userData);
		return BaseClass::AddMenuItem(item);
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();

		if (m_bRestartListUpdate)
		{
			m_bUpdatingList = true;
			m_flListUpdateStartTime = (float)system()->GetFrameTime();
			m_bRestartListUpdate = false;
		}

		if (!m_bUpdatingList)
			return;

		int menuItemsVisible = (int)((system()->GetFrameTime() - m_flListUpdateStartTime) / m_flMenuItemVisibilityRate);
		if (menuItemsVisible >= GetChildCount())
		{
			menuItemsVisible = GetChildCount();
			m_bUpdatingList = false;
		}

		// loop through all the children setting their visibility appropriately
		for (int i = 0; i < GetChildCount(); i++)
		{
			Panel *child = GetChild(i);
			if (child)
			{
				if (i >= GetChildCount() - menuItemsVisible)
				{
					child->SetPaintEnabled(true);
				}
				else
				{
					child->SetPaintEnabled(false);
				}
			}
		}
		InvalidateLayout();
		Repaint();
	}

	virtual void OnCommand(const char *command)
	{
		if (!stricmp(command, "Open"))
		{
			MoveToFront();
			RequestFocus();
			StartMenuOpeningAnimation();
		}
		else
		{
			BaseClass::OnCommand(command);
		}
	}

	virtual void OnKeyCodePressed( KeyCode code )
	{
		BaseClass::OnKeyCodePressed( code );
		// HACK: Allow F key bindings to operate even here
		if ( code >= KEY_F1 && code <= KEY_F12 )
		{
			// See if there is a binding for the FKey
			int translated = ( code - KEY_F1 ) + K_F1;
			const char *binding = gameuifuncs->Key_BindingForKey( translated );
			if ( binding && binding[0] )
			{
				// submit the entry as a console commmand
				char szCommand[256];
				Q_strncpy( szCommand, binding, sizeof( szCommand ) );
				engine->ClientCmd( szCommand );
			}
		}
		// allow the console to appear
		else if ( code == KEY_BACKQUOTE )
		{
			GameConsole().Activate();
		}
	}

	void StartMenuOpeningAnimation()
	{
		m_bRestartListUpdate = true;
	}

	virtual void OnKillFocus()
	{
		BaseClass::OnKillFocus();
		
		// force us to the rear when we lose focus (so it looks like the menu is always on the background)
		surface()->MovePopupToBack(GetVPanel());
	}

	void UpdateMenuItemState(bool isInGame, bool isMultiplayer)
	{
		bool isSteam = CommandLine()->FindParm("-steam") != 0;

		// disabled save button if we're not in a game
		for (int i = 0; i < GetChildCount(); i++)
		{
			Panel *child = GetChild(i);
			MenuItem *menuItem = dynamic_cast<MenuItem *>(child);
			if (menuItem)
			{
				bool shouldBeVisible = true;
				// filter the visibility
				KeyValues *kv = menuItem->GetUserData();
				if (!kv)
					continue;

				if (!isInGame && kv->GetInt("OnlyInGame") )
				{
					shouldBeVisible = false;
				}
				else if (isMultiplayer && kv->GetInt("notmulti"))
				{
					shouldBeVisible = false;
				}
				else if (isInGame && !isMultiplayer && kv->GetInt("notsingle"))
				{
					shouldBeVisible = false;
				}
				else if (isSteam && kv->GetInt("notsteam"))
				{
					shouldBeVisible = false;
				}
				menuItem->SetVisible(shouldBeVisible);
			}
		}
		InvalidateLayout();
	}

private:
	bool m_bRestartListUpdate;
	bool m_bUpdatingList;
	float m_flListUpdateStartTime;
	float m_flMenuItemVisibilityRate;

	typedef vgui::Menu BaseClass;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTaskbar::CTaskbar(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
	m_iGameMenuInset = 24;
	m_bPlatformMenuInitialized = false;
	m_bShowPlatformMenu = false;
	if (CommandLine()->FindParm("-steam") || CommandLine()->FindParm("-showplatform"))
	{
		m_bShowPlatformMenu = true;
	}

	m_pGameMenuButton = new CBackgroundMenuButton(this, "GameMenuButton");
	m_pGameMenuButton->SetCommand("OpenGameMenu");
	m_pPlatformMenuButton = new CBackgroundMenuButton(this, "PlatformMenuButton");
	m_pPlatformMenuButton->SetCommand("OpenPlatformMenu");

	if (!m_bShowPlatformMenu)
	{
		m_pPlatformMenuButton->SetVisible(false);
	}

	m_pPlatformMenu = NULL;
	m_pGameMenu = NULL;

	CreateGameMenu();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTaskbar::~CTaskbar()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::CreatePlatformMenu()
{
	// load the platform menu
	m_pPlatformMenu = new CGameMenu(m_pPlatformMenuButton, "PlatformMenu");

	for (int i = 0; i < g_VModuleLoader.GetModuleCount(); i++)
	{
		m_pPlatformMenu->AddMenuItem(g_VModuleLoader.GetModuleLabel(i), g_VModuleLoader.GetModuleLabel(i), new KeyValues("ActivateModule", "module", i), this, NULL);
	}

	m_pPlatformMenuButton->SetVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::CreateGameMenu()
{
	// load settings from config file
	KeyValues *datafile = new KeyValues("GameMenu");
	datafile->UsesEscapeSequences( true );	// VGUI uses escape sequences
	if (datafile->LoadFromFile((IBaseFileSystem*)filesystem(), "Resource/GameMenu.res" ))
	{
		m_pGameMenu = RecursiveLoadGameMenu(datafile);
	}
	else
	{
		// add a bunch of default stuff
		assert(!("Could not load file Resource/GameMenu.res"));
	}
	datafile->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if menu items need to be enabled/disabled
//-----------------------------------------------------------------------------
void CTaskbar::UpdateGameMenus()
{
	int wide, tall;
	
	vgui::surface()->GetScreenSize(wide, tall);

	if (m_pPlatformMenu)
	{

		m_pPlatformMenu->SetPos(wide - m_iGameMenuInset - m_pPlatformMenu->GetWide(), tall - m_pPlatformMenu->GetTall() - BLACK_BAR_SIZE);
		m_pPlatformMenu->SetVisible(true);
	}

	// check our current state
	bool isInGame = false, isMulti = false;
	const char *lvl = engine->GetLevelName();
	if (lvl && *lvl)
	{
		isInGame = true;
		if (engine->GetMaxClients() > 1)
		{
			isMulti = true;
		}
	}

	// iterate all the menu items
	m_pGameMenu->UpdateMenuItemState(isInGame, isMulti);

	// position the menu
	m_pGameMenu->SetPos(m_iGameMenuInset, tall - m_pGameMenu->GetTall() - BLACK_BAR_SIZE);
	m_pGameMenu->SetVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: Lays out taskbar tasks
//-----------------------------------------------------------------------------
void CTaskbar::UpdateTaskButtons()
{
	// Validate button list
	VPANEL focus = input()->GetFocus();
	int validTasks = 0;
	int reorderedButtons = 0;
	for (int i = 0; i < g_Tasks.Size(); i++)
	{
		if (focus && ipanel()->HasParent(focus, g_Tasks[i]->GetTaskPanel()))
		{
			g_Tasks[i]->SetSelected(true);
		}
		else
		{
			g_Tasks[i]->SetSelected(false);
		}

		if (!g_Tasks[i]->GetTaskPanel())
		{
			// panel has been deleted, remove the task
			g_Tasks[i]->SetVisible(false);
			g_Tasks[i]->MarkForDeletion();

			// remove from list
			g_Tasks.Remove(i);

			// restart validation
			i = -1;
			validTasks = 0;
			continue;
		}
		
		if (g_Tasks[i]->ShouldDisplay())
		{
			validTasks++;

			if (!g_Tasks[i]->IsVisible() && reorderedButtons < 100)
			{
				// task has just about to become visible
				// needs to be moved to the end of the task list (if it isn't already)
				if (i != g_Tasks.Size() - 1)
				{
					// swap with the next entry
					CTaskButton *tmp = g_Tasks[i];
					g_Tasks[i] = g_Tasks[i+1];
					g_Tasks[i+1] = tmp;

					// restart validation
					i = -1;
					validTasks = 0;
					reorderedButtons++;
					continue;
				}
			}
		}
	}

	// calculate button size
	int buttonWide = (GetWide() - 256) / (validTasks > 0 ? validTasks : 1);
	int buttonTall = 24; //(GetTall() - 4);

	// cap the size
	if (buttonWide > 128)
	{
		buttonWide = 128;
	}

	int x = 40 + MENU_BUTTON_WIDE, y = 20;

	// layout buttons
	for (i = 0; i < g_Tasks.Size(); i++)
	{
		if (!g_Tasks[i]->ShouldDisplay())
		{
			g_Tasks[i]->SetVisible(false);
			continue;
		}

		g_Tasks[i]->SetVisible(true);
		g_Tasks[i]->SetBounds(x, y, buttonWide - 8, buttonTall);
		x += buttonWide;
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets up the game menu from the keyvalues
//			the game menu is hierarchial, so this is recursive
//-----------------------------------------------------------------------------
CGameMenu *CTaskbar::RecursiveLoadGameMenu(KeyValues *datafile)
{
	CGameMenu *menu = new CGameMenu(this, datafile->GetName());

	// loop through all the data adding items to the menu
	for (KeyValues *dat = datafile->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		const char *label = dat->GetString("label", "<unknown>");
		const char *cmd = dat->GetString("command", NULL);
		const char *name = dat->GetString("name", label);

		menu->AddMenuItem(name, label, cmd, this, dat);
	}

	return menu;
}

//-----------------------------------------------------------------------------
// Purpose: update the taskbar a frame
//-----------------------------------------------------------------------------
void CTaskbar::RunFrame()
{
	InvalidateLayout();

	if (!m_bPlatformMenuInitialized)
	{
		// check to see if the platform is ready to load yet
		if (g_VModuleLoader.IsPlatformReady())
		{
			// open the menus
			if (m_bShowPlatformMenu)
			{
				CreatePlatformMenu();
				PostMessage(m_pPlatformMenu, new KeyValues("Command", "command", "Open"));
			}

			m_bPlatformMenuInitialized = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the title of a task
//-----------------------------------------------------------------------------
void CTaskbar::SetTitle(VPANEL panel, const wchar_t *title)
{
	CTaskButton *task = FindTask(panel);
	if (task)
	{
		task->SetTitle(title);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Lays out the position of the taskbar
//-----------------------------------------------------------------------------
void CTaskbar::PerformLayout()
{
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	//!! hack, check the cd key
	// SRC: FIXME!!! (implement me!!)
	static int count = 2;
	if (count-- == 0)
	{
		GameUI().ValidateCDKey();
	}

	// position self along bottom of screen
	SetPos(0, tall - BLACK_BAR_SIZE);
	SetSize(wide, BLACK_BAR_SIZE);

	// place menu buttons
	m_pGameMenuButton->SetPos(m_iGameMenuInset, (BLACK_BAR_SIZE - m_iGameMenuInset) / 2);
	m_pGameMenuButton->SetSize(MENU_BUTTON_WIDE, m_iGameMenuInset + 4);

	m_pPlatformMenuButton->SetPos(wide - (m_pPlatformMenuButton->GetWide() + m_iGameMenuInset), TASK_BUTTON_INSET);
	m_pPlatformMenuButton->SetSize(MENU_BUTTON_WIDE, m_iGameMenuInset + 4);
	m_pPlatformMenuButton->SetContentAlignment(Label::a_east);

	UpdateTaskButtons();
	UpdateGameMenus();
}

//-----------------------------------------------------------------------------
// Purpose: Loads scheme information
//-----------------------------------------------------------------------------
void CTaskbar::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBorder(NULL);
	
	m_iGameMenuInset = atoi(pScheme->GetResourceString("InGameDesktop/GameMenuInset"));

	// set the button images
	m_pGameMenuButton->SetImages(scheme()->GetImage("resource/game_menu", false), scheme()->GetImage("resource/game_menu_mouseover", false));
	m_pPlatformMenuButton->SetImages(scheme()->GetImage("resource/steam_menu", false), scheme()->GetImage( "resource/steam_menu_mouseover", false));

	// work out current focus - find the topmost panel
	SetBgColor(GetSchemeColor("InGameDesktop/WidescreenBarColor", pScheme));
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new task to the bar
//-----------------------------------------------------------------------------
void CTaskbar::AddTask(VPANEL panel)
{
	//!! tasks disabled
	return;

	CTaskButton *task = FindTask(panel);
	if (task)
		return;	// task already exists

	if (panel == GetVPanel())
		return; // don't put the taskbar on the taskbar!

	// create a new task
	task = new CTaskButton(this, panel);
	g_Tasks.AddToTail(task);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Finds a task that handles the specified panel
//-----------------------------------------------------------------------------
CTaskButton *CTaskbar::FindTask(VPANEL panel)
{
	// walk the buttons
	for (int i = 0; i < g_Tasks.Size(); i++)
	{
		if (g_Tasks[i]->GetTaskPanel() == panel)
		{
			return g_Tasks[i];
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: message handler for platform menu; activates the selected module
//-----------------------------------------------------------------------------
void CTaskbar::OnActivateModule(int moduleIndex)
{
	g_VModuleLoader.ActivateModule(moduleIndex);
}

//-----------------------------------------------------------------------------
// Purpose: Animates menus on gameUI being shown
//-----------------------------------------------------------------------------
void CTaskbar::OnGameUIActivated()
{
	if (m_pGameMenu)
	{
		m_pGameMenu->StartMenuOpeningAnimation();
	}
	if (m_pPlatformMenu)
	{
		m_pPlatformMenu->StartMenuOpeningAnimation();
	}
}

//-----------------------------------------------------------------------------
// Purpose: message handler for menu selections
//-----------------------------------------------------------------------------
void CTaskbar::OnCommand(const char *command)
{
	if (!stricmp(command, "OpenGameMenu"))
	{
		if (m_pGameMenu)
		{
			PostMessage(m_pGameMenu, new KeyValues("Command", "command", "Open"));
		}
	}
	else if (!stricmp(command, "OpenPlatformMenu"))
	{
		if (m_pPlatformMenu && m_bShowPlatformMenu)
		{
			PostMessage(m_pPlatformMenu, new KeyValues("Command", "command", "Open"));
		}
	}
	else if (!stricmp(command, "OpenPlayerListDialog"))
	{
		OnOpenPlayerListDialog();
	}
	else if (!stricmp(command, "OpenNewGameDialog"))
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
	else if (!stricmp(command, "OpenServerBrowser"))
	{
		OnOpenServerBrowser();
	}
	else if (!stricmp(command, "OpenLoadDemoDialog"))
	{
		OnOpenDemoDialog();
	}
	else if (!stricmp(command, "OpenCreateMultiplayerGameDialog"))
	{
		OnOpenCreateMultiplayerGameDialog();
	}
	else if (!stricmp(command, "OpenChangeGameDialog"))
	{
		OnOpenChangeGameDialog();
	}
	else if (!stricmp(command, "Quit"))
	{
		OnOpenQuitConfirmationDialog();
	}
	else if (!stricmp(command, "QuitNoConfirm"))
	{
		engine->ClientCmd("quit\n");
		// hide everything while we quit
		SetVisible(false);
		vgui::surface()->RestrictPaintToSinglePanel(GetVPanel());
	}
	else if (!stricmp(command, "ResumeGame"))
	{
		if (baseuifuncs)
		{
			baseuifuncs->HideGameUI();
		}	
	}
	else if (!stricmp(command, "Disconnect"))
	{
		engine->ClientCmd("disconnect\n" );
	}
	else if (!stricmp(command, "ReleaseModalWindow"))
	{
		vgui::surface()->RestrictPaintToSinglePanel(NULL);
	}
	else if ( strstr(command, "engine " ) )
	{
		const char *engineCMD = strstr(command, "engine " ) + strlen("engine ");
		if ( strlen( engineCMD ) > 0 )
		{
			engine->ClientCmd( const_cast<char *>( engineCMD ));
		}
		if (baseuifuncs)
		{
			baseuifuncs->HideGameUI();
		}	
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenQuitConfirmationDialog()
{
	QueryBox *box = new QueryBox("#GameUI_QuitConfirmationTitle", "#GameUI_QuitConfirmationText", this);
	box->SetOKButtonText("#GameUI_Quit");
	box->SetOKCommand(new KeyValues("Command", "command", "QuitNoConfirm"));
	box->SetCancelCommand(new KeyValues("Command", "command", "ReleaseModalWindow"));
	box->AddActionSignalTarget(this);
	box->DoModal();
	vgui::surface()->RestrictPaintToSinglePanel(box->GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenNewGameDialog()
{
	if ( !m_hNewGameDialog.Get() )
	{
		m_hNewGameDialog = new CNewGameDialog(this);
		PositionDialog( m_hNewGameDialog );
	}
	m_hNewGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenLoadGameDialog()
{
	if ( !m_hLoadGameDialog.Get() )
	{
		m_hLoadGameDialog = new CLoadGameDialog(this);
		PositionDialog( m_hLoadGameDialog );
	}
	m_hLoadGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenSaveGameDialog()
{
	if ( !m_hSaveGameDialog.Get() )
	{
		m_hSaveGameDialog = new CSaveGameDialog(this);
		PositionDialog( m_hSaveGameDialog );
	}
	m_hSaveGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenOptionsDialog()
{
	if ( !m_hOptionsDialog.Get() )
	{
		m_hOptionsDialog = new COptionsDialog(this);
		PositionDialog( m_hOptionsDialog );
	}
	m_hOptionsDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenServerBrowser()
{
	g_VModuleLoader.ActivateModule("Servers");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenDemoDialog()
{
/*	if ( !m_hDemoPlayerDialog.Get() )
	{
		m_hDemoPlayerDialog = new CDemoPlayerDialog(this);
		PositionDialog( m_hDemoPlayerDialog );
	}
	m_hDemoPlayerDialog->Activate();*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenCreateMultiplayerGameDialog()
{
	if (!m_hCreateMultiplayerGameDialog.Get())
	{
		m_hCreateMultiplayerGameDialog = new CCreateMultiplayerGameDialog(this);
		PositionDialog(m_hCreateMultiplayerGameDialog);
	}
	m_hCreateMultiplayerGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenChangeGameDialog()
{
	if (!m_hChangeGameDialog.Get())
	{
		m_hChangeGameDialog = new CChangeGameDialog(this);
		PositionDialog(m_hChangeGameDialog);
	}
	m_hChangeGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTaskbar::OnOpenPlayerListDialog()
{
	if (!m_hPlayerListDialog.Get())
	{
		m_hPlayerListDialog = new CPlayerListDialog(this);
		PositionDialog(m_hPlayerListDialog);
	}
	m_hPlayerListDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: moves the game menu button to the right place on the taskbar
//-----------------------------------------------------------------------------
void CTaskbar::PositionDialog(vgui::PHandle dlg)
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
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t CTaskbar::m_MessageMap[] =
{
	MAP_MESSAGE_INT( CTaskbar, "ActivateModule", OnActivateModule, "module" ),
};

IMPLEMENT_PANELMAP( CTaskbar, EditablePanel );
