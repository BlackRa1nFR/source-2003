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

#include <vgui_controls/PropertyPage.h>

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
	int GetMaxPlayers();
	const char *GetPassword();
	const char *GetHostName();
	const char *GetMapName();

protected:
	virtual void OnApplyChanges();

private:
	void LoadMapList();
	vgui::ListPanel *m_pMapList;

	enum { DATA_STR_LENGTH = 64 };
	char m_szHostName[DATA_STR_LENGTH];
	char m_szPassword[DATA_STR_LENGTH];
	char m_szMapName[DATA_STR_LENGTH];
	int m_iMaxPlayers;
};


#endif // CREATEMULTIPLAYERGAMESERVERPAGE_H
