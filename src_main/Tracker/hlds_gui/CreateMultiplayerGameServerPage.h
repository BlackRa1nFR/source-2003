//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CREATEMULTIPLAYERGAMESERVERPAGE_H
#define CREATEMULTIPLAYERGAMESERVERPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyPage.h>
#include <VGUI_ComboBox.h>
#include <VGUI_Button.h>

namespace vgui
{
class ListPanel;
}

//-----------------------------------------------------------------------------
// Purpose: server options page of the create game server dialog
//-----------------------------------------------------------------------------
class CCreateMultiplayerGameServerPage : public vgui::PropertyPage
{
public:
	CCreateMultiplayerGameServerPage(vgui::Panel *parent, const char *name);
	~CCreateMultiplayerGameServerPage();

	// returns currently entered information about the server
	int GetMaxPlayers()			{ return m_iMaxPlayers; }
	const char *GetPassword()	{ return m_szPassword; }
	const char *GetHostName()	{ return m_szHostName; }
	const char *GetMapName();//	{ return m_szMapName; }
	void LoadMapList();

protected:
	virtual void OnApplyChanges();

private:

	void OnCommand(const char *text);
	void LoadMODList();
	vgui::ListPanel *m_pMapList;
	vgui::TextEntry *m_pExtraCmdLine;
	vgui::Button *m_pGoButton;

	
	class CModCombo: public vgui::ComboBox
	{
	public:
		CModCombo(CCreateMultiplayerGameServerPage *parent, const char *panelName, int numLines, bool allowEdit): vgui::ComboBox(parent,panelName,numLines,allowEdit) {m_pParent=parent;};
		~CModCombo() {};

		virtual void OnHideMenu(vgui::Menu *menu);

	private:
		CCreateMultiplayerGameServerPage *m_pParent;
	};

	CModCombo *m_pMODList;

	enum { DATA_STR_LENGTH = 64 };
	char m_szHostName[DATA_STR_LENGTH];
	char m_szPassword[DATA_STR_LENGTH];
	char m_szMapName[DATA_STR_LENGTH];
	char m_szMod[DATA_STR_LENGTH];
	char m_szExtra[DATA_STR_LENGTH*2];
	int m_iMaxPlayers;
};


#endif // CREATEMULTIPLAYERGAMESERVERPAGE_H
