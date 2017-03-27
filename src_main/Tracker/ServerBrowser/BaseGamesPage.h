//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEGAMESPAGE_H
#define BASEGAMESPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include "ServerList.h"
#include "IServerRefreshResponse.h"
#include "server.h"
#include "IGameList.h"

//-----------------------------------------------------------------------------
// Purpose: Base property page for all the games lists (internet/favorites/lan/etc.)
//-----------------------------------------------------------------------------
class CBaseGamesPage : public vgui::PropertyPage, public IServerRefreshResponse, public IGameList
{
public:
	CBaseGamesPage(vgui::Panel *parent, const char *name);
	~CBaseGamesPage();

	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	// gets information about specified server
	virtual serveritem_t &GetServer(unsigned int serverID);

	// filtering methods
	// returns true if filters passed; false if failed
	virtual bool CheckPrimaryFilters(serveritem_t &server);
	virtual bool CheckSecondaryFilters(serveritem_t &server);

	// returns filter string
	virtual const char *GetFilterString();

	virtual void SetRefreshing(bool state);

	// loads filter settings from disk
	virtual void LoadFilterSettings();

	virtual int GetInvalidServerListID();

protected:
	virtual void OnTick();
	virtual void OnCommand(const char *command);
	virtual void OnKeyCodePressed(vgui::KeyCode code);

	// applies games filters to current list
	void ApplyGameFilters();
	// updates server count UI
	void UpdateStatus();

	// server response
	virtual void ServerResponded(serveritem_t &server);

	vgui::ListPanel *m_pGameList;
	CServerList	m_Servers;
//	vgui::ImagePanel *m_pPasswordIcon; // password icon

private:
	void CreateFilters();
	void OnButtonToggled(vgui::Panel *panel, int state);
	void OnTextChanged(vgui::Panel *panel, const char *text);
	void UpdateFilterSettings();
	void RecalculateFilterString();
	void OnItemSelected();

	// command buttons
	vgui::Button *m_pConnect;
	vgui::Button *m_pRefreshAll;
	vgui::Button *m_pRefreshQuick;
	vgui::Button *m_pAddServer;

	// filter controls
	vgui::ComboBox *m_pGameFilter;
	vgui::ComboBox *m_pLocationFilter;
	vgui::TextEntry *m_pMapFilter;
	vgui::ComboBox *m_pPingFilter;
	vgui::CheckButton *m_pNoFullServersFilterCheck;
	vgui::CheckButton *m_pNoEmptyServersFilterCheck;
	vgui::CheckButton *m_pNoPasswordFilterCheck;
	vgui::ToggleButton *m_pFilter;
	vgui::Label *m_pFilterString;
	
	bool m_bFiltersVisible;	// true if filter section is currently visible

	KeyValues *m_pFilters; // base filter data

	// filter data
	char m_szGameFilter[32];
	char m_szMapFilter[32];
	char m_szLocationFilter[32];
	int	m_iPingFilter;
	bool m_bFilterNoFullServers;
	bool m_bFilterNoEmptyServers;
	bool m_bFilterNoPasswordedServers;

	enum { MAX_FILTERSTRING = 512 };
	char m_szMasterServerFilterString[MAX_FILTERSTRING];

	DECLARE_PANELMAP();
	typedef vgui::PropertyPage BaseClass;
};


#endif // BASEGAMESPAGE_H
