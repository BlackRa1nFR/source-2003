//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TASKBAR_H
#define TASKBAR_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "UtlVector.h"

class CTaskButton;
class CBackgroundMenuButton;
class CGameMenu;

//-----------------------------------------------------------------------------
// Purpose: Win9X-type taskbar for in the game
//-----------------------------------------------------------------------------
class CTaskbar : public vgui::EditablePanel
{
public:
	CTaskbar(vgui::Panel *parent, const char *panelName);
	~CTaskbar();

	// update the taskbar a frame
	void RunFrame();

	// sets the title of a task
	void SetTitle(vgui::VPANEL panel, const wchar_t *title);

	// adds a new task to the list
	void AddTask(vgui::VPANEL panel);

	// handles gameUI being shown
	void OnGameUIActivated();

	// game dialogs
	void OnOpenNewGameDialog();
	void OnOpenLoadGameDialog();
	void OnOpenSaveGameDialog();
	void OnOpenOptionsDialog();
	void OnOpenServerBrowser();
	void OnOpenDemoDialog();
	void OnOpenCreateMultiplayerGameDialog();
	void OnOpenQuitConfirmationDialog();
	void OnOpenChangeGameDialog();
	void OnOpenPlayerListDialog();

	void PositionDialog(vgui::PHandle dlg);

private:
	// menu manipulation
	void CreatePlatformMenu();
	void CreateGameMenu();
	void UpdateGameMenus();
	void UpdateTaskButtons();
	CGameMenu *RecursiveLoadGameMenu(KeyValues *datafile);

	CTaskButton *FindTask(vgui::VPANEL panel);

	virtual void OnCommand(const char *command);
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnActivateModule(int moduleIndex);


	DECLARE_PANELMAP();

	// menu buttons
	CBackgroundMenuButton *m_pGameMenuButton;
	CBackgroundMenuButton *m_pPlatformMenuButton;
	CGameMenu *m_pPlatformMenu;
	CGameMenu *m_pGameMenu;
	bool m_bShowPlatformMenu;
	bool m_bPlatformMenuInitialized;
	int m_iGameMenuInset;

	// base dialogs
	vgui::DHANDLE<vgui::Frame> m_hNewGameDialog;
	vgui::DHANDLE<vgui::Frame> m_hLoadGameDialog;
	vgui::DHANDLE<vgui::Frame> m_hSaveGameDialog;
	vgui::DHANDLE<vgui::Frame> m_hOptionsDialog;
	vgui::DHANDLE<vgui::Frame> m_hCreateMultiplayerGameDialog;
	//vgui::DHANDLE<vgui::Frame> m_hDemoPlayerDialog;
	vgui::DHANDLE<vgui::Frame> m_hChangeGameDialog;
	vgui::DHANDLE<vgui::Frame> m_hPlayerListDialog;

	// list of all the tasks
	CUtlVector<CTaskButton *> g_Tasks;

	typedef vgui::Panel BaseClass;
};

extern CTaskbar *g_pTaskbar;


#endif // TASKBAR_H
