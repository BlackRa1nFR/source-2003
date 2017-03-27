//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PLAYERPANEL_H
#define PLAYERPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>
#include <VGUI_PHandle.h>
#include <VGUI_ListPanel.h>
#include <VGUI_KeyValues.h>
#include <VGUI_PropertyPage.h>

#include "PlayerContextMenu.h"
#include "player.h"

namespace vgui
{
class Button;
class ToggleButton;
class RadioButton;
class Label;
class TextEntry;
class KeyValues;
class ListPanel;
class KeyValues;
class MessageBox;
class ComboBox;
class Panel;
class PropertySheet;
};


//-----------------------------------------------------------------------------
// Purpose: Dialog for displaying information about a game server
//-----------------------------------------------------------------------------
class CPlayerPanel: public vgui::PropertyPage
{
public:
	CPlayerPanel(vgui::Panel *parent, const char *name);
	~CPlayerPanel();

	// sets whether to show admin mod specific options
	void SetAdminMod(bool state);

	// inputs a new player list
	void NewPlayerList(CUtlVector<Players_t> *players);

	// returns the keyvalues for the currently selected row
	vgui::KeyValues *GetSelected(); 

	//property page handlers
	virtual void OnPageShow();
	virtual void OnPageHide();


	// vgui overrides
	virtual void PerformLayout();
	virtual void OnCommand(const char *command);

	void SetHasRcon(bool state);

private:

	// msg handlers
	void OnOpenContextMenu(int row);
	void OnEffectPlayer(vgui::KeyValues *data);

	// an inner class
	class PlayerListPanel: public vgui::ListPanel
	{
	public:
		PlayerListPanel(vgui::Panel *parent, const char *panelName): vgui::ListPanel(parent,panelName) { m_pParent=parent;};
		
		virtual void	OnMouseDoublePressed( vgui::MouseCode code );
	private:
		vgui::Panel *m_pParent;

	};


	void OnTextChanged(vgui::Panel *panel, const char *text);

	PlayerListPanel *m_pPlayerListPanel;
	vgui::Button *m_pKickButton;
	vgui::Button *m_pBanButton;
	vgui::Button *m_pSlapButton;
	vgui::Button *m_pChatButton;

	CPlayerContextMenu *m_pPlayerContextMenu;


	bool m_bHasAdminMod;
	bool m_bHasRcon;

	vgui::Panel *m_pParent;
		
	DECLARE_PANELMAP();
	typedef vgui::PropertyPage BaseClass;
};

#endif // PLAYERPANEL_H