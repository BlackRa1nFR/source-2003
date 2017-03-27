//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMEMENUBUTTON_H
#define GAMEMENUBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/MenuButton.h>
#include <vgui_controls/PHandle.h>

namespace vgui
{
class Frame;
}

//-----------------------------------------------------------------------------
// Purpose: Button on the taskbar that has all the game-specific options
//-----------------------------------------------------------------------------
class CGameMenuButton : public vgui::MenuButton
{
public:
	CGameMenuButton( vgui::Panel *parent, const char *panelName, const char *text );
	~CGameMenuButton();

	void OnOpenNewGameDialog();
	void OnOpenLoadGameDialog();
	void OnOpenSaveGameDialog();
	void OnOpenMultiplayerDialog();
	void OnOpenMultiplayerAdvancedDialog();
	void OnOpenOptionsDialog();
	void OnOpenContentControlDialog();
	void OnOpenServerBrowser();
	void OnOpenCreateMultiplayerGameDialog();

protected:
	virtual void OnCommand(const char *command);
	virtual void PerformLayout();

private:
	vgui::Menu *RecursiveLoadGameMenu(KeyValues *datafile);

	typedef vgui::MenuButton BaseClass;
	vgui::Menu *m_pMenu;

	void				PositionDialog( vgui::PHandle dlg );

	vgui::DHANDLE<vgui::Frame> m_hNewGameDialog;
	vgui::DHANDLE<vgui::Frame> m_hLoadGameDialog;
	vgui::DHANDLE<vgui::Frame> m_hSaveGameDialog;
	vgui::DHANDLE<vgui::Frame> m_hOptionsDialog;
	vgui::DHANDLE<vgui::Frame> m_hMultiplayerCustomizeDialog;
	vgui::DHANDLE<vgui::Frame> m_hMultiplayerAdvancedDialog;
	vgui::DHANDLE<vgui::Frame> m_hContentControlDialog;
	vgui::DHANDLE<vgui::Frame> m_hCreateMultiplayerGameDialog;
};

// Singleton accessor
extern CGameMenuButton *g_pGameMenuButton;

#endif // GAMEMENUBUTTON_H
