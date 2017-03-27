//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseGamesPage.h"
#include "ServerListCompare.h"
#include "util.h"
#include "ModList.h"
#include "ServerBrowserDialog.h"

#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>

#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ListPanel.h>

#include <stdio.h>

using namespace vgui;

static int s_PasswordImage[2] = { 0 };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBaseGamesPage::CBaseGamesPage(vgui::Panel *parent, const char *name) : PropertyPage(parent, name), m_Servers(this)
{
	ivgui()->AddTickSignal(GetVPanel());

	SetSize(500, 500);
	m_szGameFilter[0] = 0;
	m_szMapFilter[0]  = 0;
	m_szLocationFilter[0] = 0;
	m_iPingFilter = 0;
	m_bFilterNoFullServers = false;
	m_bFilterNoEmptyServers = false;
	m_bFilterNoPasswordedServers = false;

	// Init UI
	m_pConnect = new Button(this, "ConnectButton", "#ServerBrowser_Connect");
	m_pConnect->SetEnabled(false);
	m_pRefreshAll = new Button(this, "RefreshButton", "#ServerBrowser_Refresh");
	m_pRefreshQuick = new Button(this, "RefreshQuickButton", "#ServerBrowser_RefreshQuick");
	m_pAddServer = new Button(this, "AddServerButton", "#ServerBrowser_AddServer");
//	m_pRefreshMenu->AddMenuItem("StopRefresh", "#ServerBrowser_StopRefreshingList", "stoprefresh", this);
	m_pGameList = new ListPanel(this, "gamelist");

	// Add the column headers
	m_pGameList->AddColumnHeader(0, "Password", util->GetString(""), 20, false, NOT_RESIZABLE, NOT_RESIZABLE );
	m_pGameList->AddColumnHeader(1, "Name", util->GetString("#ServerBrowser_Servers"), 50, true,  RESIZABLE, RESIZABLE);
	m_pGameList->AddColumnHeader(2, "GameDesc", util->GetString("#ServerBrowser_Game"), 80, true, RESIZABLE, NOT_RESIZABLE);
	m_pGameList->AddColumnHeader(3, "Players", util->GetString("#ServerBrowser_Players"), 55, true, RESIZABLE, NOT_RESIZABLE);
	m_pGameList->AddColumnHeader(4, "Map", util->GetString("#ServerBrowser_Map"), 90, true, RESIZABLE, NOT_RESIZABLE);
	m_pGameList->AddColumnHeader(5, "Ping", util->GetString("#ServerBrowser_Latency"), 55, true, RESIZABLE, NOT_RESIZABLE);

	// setup fast sort functions
	m_pGameList->SetSortFunc(0, PasswordCompare);
	m_pGameList->SetSortFunc(1, ServerNameCompare);
	m_pGameList->SetSortFunc(2, GameCompare);
	m_pGameList->SetSortFunc(3, PlayersCompare);
	m_pGameList->SetSortFunc(4, MapCompare);
	m_pGameList->SetSortFunc(5, PingCompare);

	// Sort by ping time by default
	m_pGameList->SetSortColumn(5);

//	m_pPasswordIcon = new ImagePanel(NULL, NULL);
//	m_pPasswordIcon->SetImage(scheme()->GetImage(scheme()->GetDefaultScheme(), "servers/icon_password"));

	CreateFilters();

	LoadControlSettings("Servers/DialogServerPage.res");

	m_pGameList->SetVisible( false );

	LoadFilterSettings();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBaseGamesPage::~CBaseGamesPage()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseGamesPage::GetInvalidServerListID()
{
	return m_pGameList->InvalidItemID();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseGamesPage::PerformLayout()
{
	BaseClass::PerformLayout();

	if (m_pGameList->GetSelectedItemsCount() < 1)
	{
		m_pConnect->SetEnabled(false);
	}
	else
	{
		m_pConnect->SetEnabled(true);
	}

	// game list in middle
	int x = 0, y = 0, wide, tall;
	GetSize(wide, tall);
	m_pGameList->SetBounds(8, 8, wide - 16, tall - 46);

	int xstart = 12, x2 = 60, x3 = 210, x4 = 270, x5 = 360;
	int ystart = tall - 110, y2 = tall - 80, y3 = tall - 32;
	int yl2 = tall - 88, yl3 = tall - 66;
	
	m_pFilter->SetVisible(true);
	m_pFilter->SetPos(xstart, y3);
	m_pFilterString->SetPos(xstart + m_pFilter->GetWide() + 8, y3);

	if (SupportsItem(IGameList::GETNEWLIST) || SupportsItem(IGameList::ADDSERVER))
	{
		m_pFilterString->SetSize(wide - 372, 24);
	}
	else
	{
		m_pFilterString->SetSize(wide - 280, 24);
	}

	int xpos = wide - 72;
	m_pConnect->SetPos(xpos, y3);
	m_pConnect->SetSize(64, 24);
	xpos -= (160 - 72);
	m_pRefreshAll->SetPos(xpos, y3);
	m_pRefreshAll->SetSize(84, 24);
	xpos -= (256 - 160);
	m_pRefreshQuick->SetPos(xpos, y3);
	m_pRefreshQuick->SetSize(92, 24);

	if (SupportsItem(IGameList::GETNEWLIST))
	{
		m_pRefreshQuick->SetVisible(true);
		m_pRefreshAll->SetText("#ServerBrowser_RefreshAll");
		xpos -= 96;
	}
	else
	{
		m_pRefreshQuick->SetVisible(false);
		m_pRefreshAll->SetText("#ServerBrowser_Refresh");
	}

	if (SupportsItem(IGameList::ADDSERVER))
	{
		m_pAddServer->SetPos(xpos, y3);
		m_pAddServer->SetSize(92, 24);
		m_pAddServer->SetVisible(true);
	}
	else
	{
		m_pAddServer->SetVisible(false);
	}

	if (IsRefreshing())
	{
		m_pRefreshAll->SetText("#ServerBrowser_StopRefreshingList");
	}

	if (m_pGameList->GetItemCount() > 0)
	{
		m_pRefreshQuick->SetEnabled(true);
	}
	else
	{
		m_pRefreshQuick->SetEnabled(false);
	}

	// setup filter settings controls
	Panel *label = FindChildByName("GameFilterLabel");
	label->SetVisible(m_bFiltersVisible);
	label->SetPos(xstart, ystart);
	label = FindChildByName("LocationFilterLabel");
	label->SetVisible(false);
	label->SetPos(x3, y2);
	label = FindChildByName("MapFilterLabel");
	label->SetVisible(m_bFiltersVisible);
	label->SetPos(xstart, y2);
	label = FindChildByName("PingFilterLabel");
	label->SetVisible(m_bFiltersVisible);
	label->SetPos(x3, ystart);
	m_pNoFullServersFilterCheck->SetVisible(m_bFiltersVisible);
	m_pNoFullServersFilterCheck->SetPos(x5, ystart);
	m_pNoEmptyServersFilterCheck->SetVisible(m_bFiltersVisible);
	m_pNoEmptyServersFilterCheck->SetPos(x5, yl2);
	m_pNoPasswordFilterCheck->SetVisible(m_bFiltersVisible);
	m_pNoPasswordFilterCheck->SetPos(x5, yl3);
	m_pGameFilter->SetVisible(m_bFiltersVisible);
	m_pGameFilter->SetPos(x2, ystart);
	m_pLocationFilter->SetVisible(false);
	m_pLocationFilter->SetPos(x3, y2);
	m_pMapFilter->SetVisible(m_bFiltersVisible);
	m_pMapFilter->SetPos(x2, y2);
	m_pPingFilter->SetVisible(m_bFiltersVisible);
	m_pPingFilter->SetPos(x4, ystart);
	if (m_bFiltersVisible)
	{
		// reduce the size of the game list
		int gx, gy, gw, gt;
		m_pGameList->GetBounds(gx, gy, gw, gt);
		gt -= 80;
		m_pGameList->SetBounds(gx, gy, gw, gt);
	}

	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseGamesPage::OnTick()
{
	m_Servers.RunFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseGamesPage::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	m_pGameList->SetFont(pScheme->GetFont("DefaultSmall", IsProportional()));
		// load the password icon
	ImageList *imageList = new ImageList(false);
	s_PasswordImage[0] = 0;	// index 0 is always blank
	s_PasswordImage[1] = imageList->AddImage(scheme()->GetImage("servers/icon_password", false));
	m_pGameList->SetImageList(imageList, true);
	m_pGameList->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: gets information about specified server
//-----------------------------------------------------------------------------
serveritem_t &CBaseGamesPage::GetServer(unsigned int serverID)
{
	return m_Servers.GetServer(serverID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseGamesPage::CreateFilters()
{
	m_bFiltersVisible = false;
	m_szMasterServerFilterString[0] = 0;
	m_pFilter = new ToggleButton(this, "Filter", "#ServerBrowser_Filter");
	m_pFilterString = new Label(this, "FilterString", "");
	m_pFilterString->SetTextColorState(Label::CS_DULL);

	// filter controls
	m_pGameFilter = new ComboBox(this, "GameFilter", 6, false);
	m_pGameFilter->AddItem("#ServerBrowser_All", NULL);

	m_pLocationFilter = new ComboBox(this, "LocationFilter", 6, false);
	m_pLocationFilter->AddItem("", NULL);
	m_pLocationFilter->AddItem("#ServerBrowser_US_East", NULL);
	m_pLocationFilter->AddItem("#ServerBrowser_US_West", NULL);
	m_pLocationFilter->AddItem("#ServerBrowser_SouthAmerica", NULL);
	m_pLocationFilter->AddItem("#ServerBrowser_Europe", NULL);
	m_pLocationFilter->AddItem("#ServerBrowser_Asia", NULL);
	m_pLocationFilter->AddItem("#ServerBrowser_Australia", NULL);
	m_pLocationFilter->AddItem("#ServerBrowser_MiddleEast", NULL);
	m_pLocationFilter->AddItem("#ServerBrowser_Africa", NULL);

	m_pMapFilter = new TextEntry(this, "MapFilter");
	m_pPingFilter = new ComboBox(this, "PingFilter", 6, false);
	m_pPingFilter->AddItem("#ServerBrowser_All", NULL);
	m_pPingFilter->AddItem("#ServerBrowser_LessThan50", NULL);
	m_pPingFilter->AddItem("#ServerBrowser_LessThan100", NULL);
	m_pPingFilter->AddItem("#ServerBrowser_LessThan150", NULL);
	m_pPingFilter->AddItem("#ServerBrowser_LessThan250", NULL);
	m_pPingFilter->AddItem("#ServerBrowser_LessThan350", NULL);
	m_pPingFilter->AddItem("#ServerBrowser_LessThan600", NULL);

	m_pNoEmptyServersFilterCheck = new CheckButton(this, "ServerEmptyFilterCheck", "");
	m_pNoFullServersFilterCheck = new CheckButton(this, "ServerFullFilterCheck", "");
	m_pNoPasswordFilterCheck = new CheckButton(this, "NoPasswordFilterCheck", "");

	for (int i = 0; i < ModList().ModCount(); i++)
	{
		m_pGameFilter->AddItem(ModList().GetModDir(i), NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads filter settings from the keyvalues
//-----------------------------------------------------------------------------
void CBaseGamesPage::LoadFilterSettings()
{
	KeyValues *filter = ServerBrowserDialog().GetFilterSaveData(GetName());

	if (ServerBrowserDialog().GetActiveModName())
	{
		v_strncpy(m_szGameFilter, ServerBrowserDialog().GetActiveModName(), sizeof(m_szGameFilter));
	}
	else
	{
		v_strncpy(m_szGameFilter, filter->GetString("game"), sizeof(m_szGameFilter));
	}
	v_strncpy(m_szMapFilter, filter->GetString("map"), sizeof(m_szMapFilter));
	v_strncpy(m_szLocationFilter, filter->GetString("location"), sizeof(m_szLocationFilter));
	m_iPingFilter = filter->GetInt("ping");
	m_bFilterNoFullServers = filter->GetInt("NoFull");
	m_bFilterNoEmptyServers = filter->GetInt("NoEmpty");
	m_bFilterNoPasswordedServers = filter->GetInt("NoPassword");

	// apply to the controls
	m_pGameFilter->SetText(m_szGameFilter);
	m_pMapFilter->SetText(m_szMapFilter);
	m_pLocationFilter->SetText(m_szLocationFilter);
	
	if (m_iPingFilter)
	{
		char buf[32];
		sprintf(buf, "< %d", m_iPingFilter);
		m_pPingFilter->SetText(buf);
	}

	m_pNoFullServersFilterCheck->SetSelected(m_bFilterNoFullServers);
	m_pNoEmptyServersFilterCheck->SetSelected(m_bFilterNoEmptyServers);
	m_pNoPasswordFilterCheck->SetSelected(m_bFilterNoPasswordedServers);

	UpdateFilterSettings();
}

//-----------------------------------------------------------------------------
// Purpose: Handles incoming server refresh data
//			updates the server browser with the refreshed information from the server itself
//-----------------------------------------------------------------------------
void CBaseGamesPage::ServerResponded(serveritem_t &server)
{
	// check filters
	bool removeItem = false;
	if ( !CheckPrimaryFilters(server) )
	{
		// server has been filtered at a primary level
		// remove from lists
		server.doNotRefresh = true;

		// remove from UI list
		removeItem = true;
	}
	else if (!CheckSecondaryFilters(server))
	{
		// we still ping this server in the future; however it is removed from UI list
		removeItem = true;
	}

	if (removeItem)
		return;

	// update UI
	KeyValues *kv;
	if ( m_pGameList->IsValidItemID(server.listEntryID) )
	{
		// we're updating an existing entry
		kv = m_pGameList->GetItem(server.listEntryID);
		m_pGameList->SetUserData(server.listEntryID, server.serverID);
	}
	else
	{
		// new entry
		kv = new KeyValues("Server");
	}

	kv->SetString("name", server.name);
	kv->SetString("map", server.map);
	kv->SetString("GameDir", server.gameDir);
	kv->SetString("GameDesc", server.gameDescription);
	kv->SetInt("password", server.password ? 1 : 0); //m_pPasswordIcon : NULL);

	char buf[256];
	sprintf(buf, "%d / %d", server.players, server.maxPlayers);
	kv->SetString("Players", buf);
	
	kv->SetInt("Ping", server.ping);

	if ( !m_pGameList->IsValidItemID(server.listEntryID) )
	{
		// new server, add to list
		server.listEntryID = m_pGameList->AddItem(kv, server.serverID, false, false);
	}
	else
	{
		// tell the list that we've changed the data
		m_pGameList->ApplyItemChanges(server.listEntryID);
	}

	UpdateStatus();
	m_pGameList->InvalidateLayout();
	m_pGameList->Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Handles filter dropdown being toggled
//-----------------------------------------------------------------------------
void CBaseGamesPage::OnButtonToggled(Panel *panel, int state)
{
	if (panel == m_pFilter)
	{
		if (state)
		{
			// drop down
			m_bFiltersVisible = true;
		}
		else
		{
			// hide filter area
			m_bFiltersVisible = false;
		}

		InvalidateLayout();
		Repaint();
	}
	else if (panel == m_pNoFullServersFilterCheck || panel == m_pNoEmptyServersFilterCheck || panel == m_pNoPasswordFilterCheck)
	{
		// treat changing these buttons like any other filter has changed
		OnTextChanged(panel, "");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the game dir combo box is changed
//-----------------------------------------------------------------------------
void CBaseGamesPage::OnTextChanged(Panel *panel, const char *text)
{
	if (!stricmp(text, "<All>"))
	{
		ComboBox *box = dynamic_cast<ComboBox *>(panel);
		if (box)
		{
			box->SetText("");
			text = "";
		}
	}

	// get filter settings from controls
	UpdateFilterSettings();

	// apply settings
	ApplyFilters();
}

//-----------------------------------------------------------------------------
// Purpose: applies only the game filter to the current list
//-----------------------------------------------------------------------------
void CBaseGamesPage::ApplyGameFilters()
{
	// loop through all the servers checking filters
	for (int i = 0; i < m_Servers.ServerCount(); i++)
	{
		serveritem_t &server = m_Servers.GetServer(i);
		if (!CheckPrimaryFilters(server) || !CheckSecondaryFilters(server))
		{
			// server failed filtering, remove it
			server.doNotRefresh = true;
			if ( m_pGameList->IsValidItemID(server.listEntryID) )
			{
				// don't remove the server from list, just hide since this is a lot faster
				m_pGameList->SetItemVisible(server.listEntryID, false);
			}
		}
		else if (server.hadSuccessfulResponse)
		{
			// server passed filters, so it can be refreshed again
			server.doNotRefresh = false;

			// re-add item to list
			if ( !m_pGameList->IsValidItemID(server.listEntryID) )
			{
				KeyValues *kv = new KeyValues("Server");
				kv->SetString("name", server.name);
				kv->SetString("map", server.map);
				kv->SetString("GameDir", server.gameDir);
				kv->SetString("GameDesc", server.gameDescription);

				char buf[256];
				sprintf(buf, "%d / %d", server.players, server.maxPlayers);
				kv->SetString("Players", buf);
				kv->SetInt("Ping", server.ping);
				kv->SetInt("password", server.password ? 1 : 0); //m_pPasswordIcon : NULL);

				server.listEntryID = m_pGameList->AddItem(kv, i, false, false);
			}
			
			// make sure the server is visible
			m_pGameList->SetItemVisible(server.listEntryID, true);
		}
	}

	UpdateStatus();
	m_pGameList->SortList();
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Resets UI server count
//-----------------------------------------------------------------------------
void CBaseGamesPage::UpdateStatus()
{
	if (m_pGameList->GetItemCount() > 1)
	{
		char buf[64];
		sprintf(buf, "Servers (%d)", m_pGameList->GetItemCount());
		m_pGameList->SetColumnHeaderText(1, buf);
	}
	else
	{
		m_pGameList->SetColumnHeaderText(1, "Servers");
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets filter settings from controls
//-----------------------------------------------------------------------------
void CBaseGamesPage::UpdateFilterSettings()
{
	// game
	if (ServerBrowserDialog().GetActiveModName())
	{
		// overriding the game filter
		v_strncpy(m_szGameFilter, ServerBrowserDialog().GetActiveModName(), sizeof(m_szGameFilter));
		m_pGameFilter->SetEnabled(false);
	}
	else
	{
		m_pGameFilter->GetText(m_szGameFilter, sizeof(m_szGameFilter) - 1);
		m_pGameFilter->SetEnabled(true);
	}
	strlwr(m_szGameFilter);

	// map
	m_pMapFilter->GetText(m_szMapFilter, sizeof(m_szMapFilter) - 1);
	strlwr(m_szMapFilter);
	
	// ping
	char buf[256];
	m_pPingFilter->GetText(buf, sizeof(buf));
	if (buf[0])
	{
		m_iPingFilter = atoi(buf + 2);
	}
	else
	{
		m_iPingFilter = 0;
	}

	// location
	m_pLocationFilter->GetText(m_szLocationFilter, sizeof(m_szLocationFilter) - 1); 

	// players
	m_bFilterNoFullServers = m_pNoFullServersFilterCheck->IsSelected();
	m_bFilterNoEmptyServers = m_pNoEmptyServersFilterCheck->IsSelected();
	m_bFilterNoPasswordedServers = m_pNoPasswordFilterCheck->IsSelected();

	// update master filter string text
	buf[0] = 0;
	if (m_szGameFilter[0])
	{
		strcat(buf, "\\gamedir\\");
		strcat(buf, m_szGameFilter);
	}
	if (m_bFilterNoEmptyServers)
	{
		strcat(buf, "\\empty\\1");
	}
	if (m_bFilterNoFullServers)
	{
		strcat(buf, "\\full\\1");
	}
//	strcat(buf, "\\dedicated\\1");

	// copy in the new filter
	strcpy(m_szMasterServerFilterString, buf);

	// copy filter settings into filter file
	KeyValues *filter = ServerBrowserDialog().GetFilterSaveData(GetName());

	// only save the game filter if we're not overriding it
	if (!ServerBrowserDialog().GetActiveModName())
	{
		filter->SetString("game", m_szGameFilter);
	}
	filter->SetString("map", m_szMapFilter);
	filter->SetInt("ping", m_iPingFilter);
	filter->SetString("location", m_szLocationFilter);
	filter->SetInt("NoFull", m_bFilterNoFullServers);
	filter->SetInt("NoEmpty", m_bFilterNoEmptyServers);
	filter->SetInt("NoPassword", m_bFilterNoPasswordedServers);

	RecalculateFilterString();
}

//-----------------------------------------------------------------------------
// Purpose: reconstructs the filter description string from the current filter settings
//-----------------------------------------------------------------------------
void CBaseGamesPage::RecalculateFilterString()
{
	wchar_t unicode[2048], tempUnicode[128], spacerUnicode[8];
	unicode[0] = 0;
	int iTempUnicodeSize = sizeof( tempUnicode );

	localize()->ConvertANSIToUnicode( "; ", spacerUnicode, sizeof( spacerUnicode ) );

	if (m_szGameFilter[0])
	{
		localize()->ConvertANSIToUnicode( m_szGameFilter, tempUnicode, iTempUnicodeSize );
		wcscat( unicode, tempUnicode );
		wcscat( unicode, spacerUnicode );
	}

	if (m_iPingFilter)
	{
		char tmpBuf[16];
		itoa( m_iPingFilter, tmpBuf, 10 );

		wcscat( unicode, localize()->Find( "#ServerBrowser_FilterDescLatency" ) );
		localize()->ConvertANSIToUnicode( " < ", tempUnicode, iTempUnicodeSize );
		wcscat( unicode, tempUnicode );
		localize()->ConvertANSIToUnicode(tmpBuf, tempUnicode, iTempUnicodeSize );
		wcscat( unicode, tempUnicode );	
		wcscat( unicode, spacerUnicode );
	}

	if (m_bFilterNoFullServers)
	{
		wcscat( unicode, localize()->Find( "#ServerBrowser_FilterDescNotFull" ) );
		wcscat( unicode, spacerUnicode );
	}

	if (m_bFilterNoEmptyServers)
	{
		wcscat( unicode, localize()->Find( "#ServerBrowser_FilterDescNotEmpty" ) );
		wcscat( unicode, spacerUnicode );
	}

	if (m_bFilterNoPasswordedServers)
	{
		wcscat( unicode, localize()->Find( "#ServerBrowser_FilterDescNoPassword" ) );
		wcscat( unicode, spacerUnicode );
	}

	if (m_szMapFilter[0])
	{
		localize()->ConvertANSIToUnicode( m_szMapFilter, tempUnicode, iTempUnicodeSize );
		wcscat( unicode, tempUnicode );
	}

	m_pFilterString->SetText(unicode);
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the server passes the primary filters
//			if the server fails the filters, it will not be refreshed again
//-----------------------------------------------------------------------------
bool CBaseGamesPage::CheckPrimaryFilters(serveritem_t &server)
{
	if (m_szGameFilter[0] && stricmp(m_szGameFilter, server.gameDir))
	{
		return false;
	}

	//!! check location filter

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if a server passes the secondary set of filters
//			server will be continued to be pinged if it fails the filter, since
//			the relvent server data is dynamic
//-----------------------------------------------------------------------------
bool CBaseGamesPage::CheckSecondaryFilters(serveritem_t &server)
{
	if (m_bFilterNoEmptyServers && server.players < 1)
	{
		return false;
	}

	if (m_bFilterNoFullServers && server.players >= server.maxPlayers)
	{
		return false;
	}

	if (m_iPingFilter && server.ping > m_iPingFilter)
	{
		return false;
	}

	if (m_bFilterNoPasswordedServers && server.password)
	{
		return false;
	}

	// compare the first few characters of the filter name
	int count = strlen(m_szMapFilter);
	if (count && strnicmp(server.map, m_szMapFilter, count))
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseGamesPage::GetFilterString()
{
	return m_szMasterServerFilterString;
}

//-----------------------------------------------------------------------------
// Purpose: call to let the UI now whether the game list is currently refreshing
//-----------------------------------------------------------------------------
void CBaseGamesPage::SetRefreshing(bool state)
{
	if (state)
	{
		ServerBrowserDialog().UpdateStatusText("#ServerBrowser_RefreshingServerList");

		// clear message in panel
		m_pGameList->SetEmptyListText("");
		m_pRefreshAll->SetText("#ServerBrowser_StopRefreshingList");
		m_pRefreshAll->SetCommand("stoprefresh");
		m_pRefreshQuick->SetEnabled(false);
	}
	else
	{
		ServerBrowserDialog().UpdateStatusText("");
		if (SupportsItem(IGameList::GETNEWLIST))
		{
			m_pRefreshAll->SetText("#ServerBrowser_RefreshAll");
		}
		else
		{
			m_pRefreshAll->SetText("#ServerBrowser_Refresh");
		}
		m_pRefreshAll->SetCommand("GetNewList");

		// 'refresh quick' button is only enabled if there are servers in the list
		if (m_pGameList->GetItemCount() > 0)
		{
			m_pRefreshQuick->SetEnabled(true);
		}
		else
		{
			m_pRefreshQuick->SetEnabled(false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseGamesPage::OnCommand(const char *command)
{
	if (!stricmp(command, "Connect"))
	{
		OnBeginConnect();
	}
	else if (!stricmp(command, "stoprefresh"))
	{
		// cancel the existing refresh
		StopRefresh();
	}
	else if (!stricmp(command, "refresh"))
	{
		// Start a new refresh
		StartRefresh();
	}
	else if (!stricmp(command, "GetNewList"))
	{
		GetNewServerList();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: called when a row gets selected in the list
//-----------------------------------------------------------------------------
void CBaseGamesPage::OnItemSelected()
{
	if (m_pGameList->GetSelectedItemsCount() < 1)
	{
		m_pConnect->SetEnabled(false);
	}
	else
	{
		m_pConnect->SetEnabled(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: refreshes server list on F5
//-----------------------------------------------------------------------------
void CBaseGamesPage::OnKeyCodePressed(vgui::KeyCode code)
{
	if (code == KEY_F5)
	{
		StartRefresh();
	}
	else
	{
		BaseClass::OnKeyCodePressed(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CBaseGamesPage::m_MessageMap[] =
{
	MAP_MESSAGE( CBaseGamesPage, "ItemSelected", OnItemSelected ),
	MAP_MESSAGE_PTR_INT( CBaseGamesPage, "ButtonToggled", OnButtonToggled, "panel", "state" ),
	MAP_MESSAGE_PTR_CONSTCHARPTR( CBaseGamesPage, "TextChanged", OnTextChanged, "panel", "text" ),
};
IMPLEMENT_PANELMAP(CBaseGamesPage, BaseClass);
