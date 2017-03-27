//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "FavoriteGames.h"

#include "proto_oob.h"
#include "ServerContextMenu.h"
#include "ServerListCompare.h"
#include "Socket.h"
#include "util.h"
#include "ServerBrowserDialog.h"
#include "DialogAddServer.h"
#include "InternetGames.h"
#include "FileSystem.h"
#include "UtlBuffer.h"

#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>

#include <vgui_controls/ListPanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/MessageBox.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFavoriteGames::CFavoriteGames(vgui::Panel *parent) : CBaseGamesPage(parent, "FavoriteGames")
{
	m_iServerRefreshCount = 0;
	
	m_pImportFavoritesdlg = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CFavoriteGames::~CFavoriteGames()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFavoriteGames::LoadFavoritesList(KeyValues *favoritesData)
{
	// load in favorites
	for (KeyValues *dat = favoritesData->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		serveritem_t server;
		memset(&server, 0, sizeof(server));
		
		const char *addr = dat->GetString("address");
		int ip1, ip2, ip3, ip4, port;
		sscanf(addr, "%d.%d.%d.%d:%d", &ip1, &ip2, &ip3, &ip4, &port);
		server.ip[0] = ip1;
		server.ip[1] = ip2;
		server.ip[2] = ip3;
		server.ip[3] = ip4;
		server.port = port;
		server.players = 0;
		v_strncpy(server.name, dat->GetString("name"), sizeof(server.name));
		v_strncpy(server.map, dat->GetString("map"), sizeof(server.map));
		v_strncpy(server.gameDir, dat->GetString("gamedir"), sizeof(server.gameDir));
		server.players = dat->GetInt("players");
		server.maxPlayers = dat->GetInt("maxplayers");
		
		// add to main list
		AddNewServer(server);
	}

	// set empty message
	m_pGameList->SetEmptyListText("#ServerBrowser_NoFavoriteServers");
}

//-----------------------------------------------------------------------------
// Purpose: saves the current list of servers to the favorites section of the data file
//-----------------------------------------------------------------------------
void CFavoriteGames::SaveFavoritesList(KeyValues *favoritesData)
{
	favoritesData->Clear();
	
	// loop through all the servers writing them into the doc
	for (int i = 0; i < m_Servers.ServerCount(); i++)
	{
		serveritem_t &server = m_Servers.GetServer(i);
		if (server.doNotRefresh)
			continue;
		
		KeyValues *dat = favoritesData->CreateNewKey();
		
		dat->SetString("name", server.name);
		dat->SetString("gamedir", server.gameDir);
		dat->SetInt("players", server.players);
		dat->SetInt("maxplayers", server.maxPlayers);
		dat->SetString("map", server.map);
		
		char buf[64];
		sprintf(buf, "%d.%d.%d.%d:%d", server.ip[0], server.ip[1], server.ip[2], server.ip[3], server.port);
		dat->SetString("address", buf);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Activates the page, starts refresh
//-----------------------------------------------------------------------------
void CFavoriteGames::OnPageShow()
{
	StartRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: Called on page hide, stops any refresh
//-----------------------------------------------------------------------------
void CFavoriteGames::OnPageHide()
{
	StopRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the game list supports the specified ui elements
//-----------------------------------------------------------------------------
bool CFavoriteGames::SupportsItem(InterfaceItem_e item)
{
	switch (item)
	{
	case FILTERS:
	case ADDSERVER:
		return true;
	
	case GETNEWLIST:
	default:
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: starts the servers refreshing
//-----------------------------------------------------------------------------
void CFavoriteGames::StartRefresh()
{
	// Stop current refresh
	m_Servers.StopRefresh();
	
	// build the refresh list from the server list
	for (int i = 0; i < m_Servers.ServerCount(); i++)
	{
		if (m_Servers.GetServer(i).doNotRefresh)
			continue;
		
		m_Servers.AddServerToRefreshList(i);
	}
	
	m_Servers.StartRefresh();
	
	SetRefreshing(IsRefreshing());
}

//-----------------------------------------------------------------------------
// Purpose: gets a new server list
//-----------------------------------------------------------------------------
void CFavoriteGames::GetNewServerList()
{
	StartRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: stops current refresh/GetNewServerList()
//-----------------------------------------------------------------------------
void CFavoriteGames::StopRefresh()
{
	// Stop the server list refreshing
	m_Servers.StopRefresh();
	
	// clear update states
	m_iServerRefreshCount = 0;
	
	// update UI
	RefreshComplete();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the list is currently refreshing servers
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFavoriteGames::IsRefreshing()
{
	return m_Servers.IsRefreshing();
}

//-----------------------------------------------------------------------------
// Purpose: adds a new server to list
// Input  : &server - 
//-----------------------------------------------------------------------------
void CFavoriteGames::AddNewServer(serveritem_t &newServer)
{
	// copy server into main server list
	unsigned int index = m_Servers.AddNewServer(newServer);
	
	// reget the server
	serveritem_t &server = m_Servers.GetServer(index);
	server.hadSuccessfulResponse = true;
	server.doNotRefresh = false;
	server.listEntryID = GetInvalidServerListID();
	server.serverID = index;
}

//-----------------------------------------------------------------------------
// Purpose: Continues the refresh
// Input  : moreAvailable - 
//			lastUnique - 
//-----------------------------------------------------------------------------
void CFavoriteGames::ListReceived(bool moreAvailable, int lastUnique)
{
	m_Servers.StartRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: called when Connect button is pressed
//-----------------------------------------------------------------------------
void CFavoriteGames::OnBeginConnect()
{
	if (!m_pGameList->GetSelectedItemsCount())
		return;
	
	// get the server
	int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(0));
		
	// Stop the current refresh
	StopRefresh();
		
	// join the game
	ServerBrowserDialog().JoinGame(this, serverID);
}

//-----------------------------------------------------------------------------
// Purpose: Displays the current game info without connecting
//-----------------------------------------------------------------------------
void CFavoriteGames::OnViewGameInfo()
{
	if (!m_pGameList->GetSelectedItemsCount())
		return;
	
	// get the server
	int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(0));
	
	// Stop the current refresh
	StopRefresh();
	
	// join the game
	ServerBrowserDialog().OpenGameInfoDialog(this, serverID);
}

//-----------------------------------------------------------------------------
// Purpose: reapplies filters (does nothing with this)
//-----------------------------------------------------------------------------
void CFavoriteGames::ApplyFilters()
{
	ApplyGameFilters();
}

//-----------------------------------------------------------------------------
// Purpose: called when a server response has timed out, treated just like a normal server
//-----------------------------------------------------------------------------
void CFavoriteGames::ServerFailedToRespond(serveritem_t &server)
{
	ServerResponded(server);
}

//-----------------------------------------------------------------------------
// Purpose: called when the current refresh list is complete
//-----------------------------------------------------------------------------
void CFavoriteGames::RefreshComplete()
{
	SetRefreshing(false);
	m_pGameList->SortList();
	m_iServerRefreshCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: opens context menu (user right clicked on a server)
//-----------------------------------------------------------------------------
void CFavoriteGames::OnOpenContextMenu(int itemID)
{
	CServerContextMenu *menu = ServerBrowserDialog().GetContextMenu(m_pGameList);
	if (m_pGameList->GetSelectedItemsCount())
	{
		// get the server
		int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(0));
		serveritem_t &server = m_Servers.GetServer(serverID);
		
		// Activate context menu
		menu->ShowMenu(this, serverID, true, true, false);
		menu->AddMenuItem("RemoveServer", "#ServerBrowser_RemoveServerFromFavorites", new KeyValues("RemoveFromFavorites"), this);
	}
	else
	{
		// no selected rows, so don't display default stuff in menu
		menu->ShowMenu(this, -1, false, false, false);
	}
	
	menu->AddMenuItem("AddServerByName", "#ServerBrowser_AddServerByIP", new KeyValues("AddServerByName"), this);
}

//-----------------------------------------------------------------------------
// Purpose: refreshes a single server
//-----------------------------------------------------------------------------
void CFavoriteGames::OnRefreshServer(int serverID)
{
	// walk the list of selected servers refreshing them
	for (int i = 0; i < m_pGameList->GetSelectedItemsCount(); i++)
	{
		serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(i));

		// refresh this server
		m_Servers.AddServerToRefreshList(serverID);
	}

	m_Servers.StartRefresh();
	SetRefreshing(IsRefreshing());
}

//-----------------------------------------------------------------------------
// Purpose: adds a server to the favorites
// Input  : serverID - 
//-----------------------------------------------------------------------------
void CFavoriteGames::OnRemoveFromFavorites()
{
	// iterate the selection
	while (m_pGameList->GetSelectedItemsCount() > 0)
	{
		int itemID = m_pGameList->GetSelectedItem(0);
		int serverID = m_pGameList->GetItemData(itemID)->userData;
		
		if (serverID >= m_Servers.ServerCount())
			continue;
		
		serveritem_t &server = m_Servers.GetServer(serverID);
		
		// remove from favorites list
		if ( m_pGameList->IsValidItemID(server.listEntryID) )
		{
			// find the row in the list and kill
			m_pGameList->RemoveItem(server.listEntryID);
			server.listEntryID = GetInvalidServerListID();
		}
		
		server.doNotRefresh = true;
	}
	
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Adds a server by IP address
//-----------------------------------------------------------------------------
void CFavoriteGames::OnAddServerByName()
{
	// open the add server dialog
	CDialogAddServer *dlg = new CDialogAddServer(this, this);
	dlg->MoveToCenterOfScreen();
	dlg->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Load a file into a CUtlBuffer class
//-----------------------------------------------------------------------------
int LoadFileIntoBuffer(CUtlBuffer &buf, char *pszFilename)
{
	// Open the file
	FileHandle_t fh = filesystem()->Open( pszFilename, "rb" );
	if (fh == 0)
	{
		MessageBox *dlg = new MessageBox ("#ServerBrowser_UnableToOpenDataFile", false);
		dlg->DoModal();
		return 0;	//file didn't load
	}

	int nFileSize = filesystem()->Size(fh);
	
	// Read the file in one gulp
	buf.EnsureCapacity( nFileSize );
	int result = filesystem()->Read( buf.Base(), nFileSize, fh );
	filesystem()->Close( fh );
	
	return nFileSize;	
}


//-----------------------------------------------------------------------------
// Purpose: Import Favorite Servers from previous installations of
//			Halflife 
//-----------------------------------------------------------------------------
void CFavoriteGames::ImportFavorites()
{
	// disabled for now due to filesystem not accepting non-relative paths
	return;

	char name[512];
	// check if halflife is installed
	if (vgui::system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Valve\\Half-life\\InstallPath", name, sizeof(name)))
	{
		// attach name of favorites file
		strcat(name, "\\favsvrs.dat");
	}
	// check if counterstrike is installed
	else if (vgui::system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Sierra OnLine\\Setup\\CSTRIKE\\Directory", name, sizeof(name)))
	{
		// attach name of favorites file
		strcat(name, "\\favsvrs.dat");
	}
	else // no hl installation, no fav servers?
	{
		return;
	}
	
	// see if the favorite server list file exists. 
	FileHandle_t fh = filesystem()->Open(name, "rb");
	if ( !fh )
	{
		return;
	}
	filesystem()->Close(fh);
	

	// it exists! yay lets transfer the servers over into the server browser

	// pop up a message about what we are doing
	m_pImportFavoritesdlg = new MessageBox ("#ServerBrowser_UpdatingFavsTitle", "#ServerBrowser_UpdatingFavs", this);
	m_pImportFavoritesdlg->SetCommand("OnImportFavoritesFile");
	m_pImportFavoritesdlg->AddActionSignalTarget(this);
	// hide the ok button since they dont have to do anything
	m_pImportFavoritesdlg->SetOKButtonVisible(false);
	// don't let them close this window
	m_pImportFavoritesdlg->DisableCloseButton(false);
	// dont let them put this window on the menubar
	m_pImportFavoritesdlg->SetMenuButtonVisible(false);
	// Pop up the box 
	m_pImportFavoritesdlg->DoModal();
	// execute the message box's command
	KeyValues *command = new KeyValues("Command");
	command->SetString("Command", "OnImportFavoritesFile");
	// post the message with a delay so that the box will display before the command is executed
	PostMessage(this, command, (float).1);
}

//-----------------------------------------------------------------------------
// Purpose: Parse posted messages
//			 
//-----------------------------------------------------------------------------
void CFavoriteGames::OnCommand(const char *command)
{
	if (!stricmp(command, "OnImportFavoritesFile"))
	{
		OnImportFavoritesFile();
	}
	else if (!stricmp(command, "AddServerByName"))
	{
		OnAddServerByName();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Import Favorite Servers from previous installations of
//			Halflife 
//-----------------------------------------------------------------------------
void CFavoriteGames::OnImportFavoritesFile()
{	
	char name[512];
	// check if they have halflife
	if (vgui::system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Valve\\Half-life\\InstallPath", name, sizeof(name)))
	{
		// add filename
		strcat(name, "\\favsvrs.dat");
	}
	// check if they have counterstrike
	else if (vgui::system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Sierra OnLine\\Setup\\CSTRIKE\\Directory", name, sizeof(name)))
	{
		// add filename
		strcat(name, "\\favsvrs.dat");
	}
	else // no hl installation, no fav servers?	// should never hit this!
	{
		return;
	}
		
	// load the file in one gulp
	CUtlBuffer buf ( 0, 1024, true);
	int fileSize = LoadFileIntoBuffer(buf, name);
	if ( fileSize == 0)
	{
		MessageBox *dlg = new MessageBox ("#ServerBrowser_UnableToLoadFavsTitle", "#ServerBrowser_ErrorLoadingFile", this);
		dlg->DoModal();
		return;	//file didn't load
	}
	
	// scan for the favorite servers
	char data[255];
	buf.GetString(data);
	while (buf.TellGet() < fileSize)
	{
		// store the Start of the server description so we can go back 
		// if its a favorite
		int serverStart;
		if ( strstr (data, "server") != NULL)
		{
			buf.GetString(data); // get the '{'
			serverStart = buf.TellGet(); 
		}
		
		// check if server is a favorite
		if ( strstr (data, "favorite") != NULL)
		{
			buf.GetString(data);
			if ( strstr(data, "1") )// server is a favorite
			{
				// go back to the Start and load the details
				buf.SeekGet( CUtlBuffer::SEEK_HEAD, serverStart);
				// try and add this server
				if (!LoadAFavoriteServer (buf))
				{
					// file may be corrupt, had trouble parsing it
					break;
				};
			}
		}
		buf.EatWhiteSpace();
		buf.GetString(data);
	}

	// we are done. Hide the message box.
	m_pImportFavoritesdlg->SetVisible(false);
}


//-----------------------------------------------------------------------------
// Purpose: Check to make sure we are reading what is expected
//			
//-----------------------------------------------------------------------------
bool CFavoriteGames::CheckForCorruption(char *data, const char *checkString)
{
	if (strcmp(data, checkString) != 0)
	{
		MessageBox *dlg = new MessageBox ("#ServerBrowser_UnableToLoadFavsTitle", "#ServerBrowser_ErrorLoadingFileCorrupt");
		dlg->DoModal();
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Parse throught the details of a server and populate a serveritem_t structure
//			Add it to the favorites list at the end. 
//-----------------------------------------------------------------------------
bool CFavoriteGames::LoadAFavoriteServer (CUtlBuffer &buf)
{
	serveritem_t fav;
	char data[255];
	buf.GetString(data);
	if (CheckForCorruption(data, "\"address\""))
		return false;;
	buf.GetString(data);
	int temp[4];
	sscanf (data, "\"%d.%d.%d.%d\"\n", &temp[0], &temp[1], &temp[2], &temp[3]);
	fav.ip[0] = temp[0];
	fav.ip[1] = temp[1];
	fav.ip[2] = temp[2];
	fav.ip[3] = temp[3];
	
	buf.EatWhiteSpace();
	buf.Scanf ("\"port\" \"%d\"", &fav.port );
	if (fav.port < 0)
		fav.port += 65536;
	buf.EatWhiteSpace();
	buf.Scanf ("\"name\" \"%s\"", fav.name );
	fav.name[strlen(fav.name)-1]='\0';	// remove trailing quote
	buf.EatWhiteSpace();
	buf.Scanf ("\"map\" \"%s\"", fav.map );
	fav.map[strlen(fav.map)-1]='\0';	// remove trailing quote
	
	buf.EatWhiteSpace();
	buf.Scanf ("\"game\" \"%s\"", fav.gameDescription );
	fav.gameDescription[strlen(fav.gameDescription)-1]='\0';	// remove trailing quote
	buf.EatWhiteSpace();
	buf.Scanf ("\"dir\" \"%s\"", fav.gameDir );
	fav.gameDir[strlen(fav.gameDir)-1
		]='\0';	// remove trailing quote
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"url\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"dl\""))
		return false;;
	buf.GetString(data);
	
	buf.EatWhiteSpace();
	buf.Scanf ("\"maxplayers\" \"%d\"", &fav.maxPlayers );
	buf.EatWhiteSpace();
	buf.Scanf ("\"currentplayers\" \"%d\"", &fav.players );
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"protocol\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"favorite\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"ipx\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"mod\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"version\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"size\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"svtype\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"svos\""))
		return false;;
	buf.GetString(data);
	
	buf.EatWhiteSpace();
	buf.Scanf ("\"password\" \"%d\"\n", &fav.password );
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"svside\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"cldll\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"lan\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"svping\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"noresponse\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"packetloss\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"status\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"filtered\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"fullmax\""))
		return false;;
	buf.GetString(data);
	
	buf.GetString(data);
	if (CheckForCorruption(data, "\"hlversion\""))
		return false;;
	buf.GetString(data);
	
	AddNewServer(fav);

	return true;	
}



//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CFavoriteGames::m_MessageMap[] =
{
	MAP_MESSAGE( CFavoriteGames, "ConnectToServer", OnBeginConnect ),
	MAP_MESSAGE( CFavoriteGames, "ViewGameInfo", OnViewGameInfo ),
	MAP_MESSAGE( CFavoriteGames, "RemoveFromFavorites", OnRemoveFromFavorites ),
	MAP_MESSAGE( CFavoriteGames, "AddServerByName", OnAddServerByName ),
	MAP_MESSAGE_INT( CFavoriteGames, "RefreshServer", OnRefreshServer, "serverID" ),
	MAP_MESSAGE_INT( CFavoriteGames, "OpenContextMenu", OnOpenContextMenu, "itemID" ),
};
IMPLEMENT_PANELMAP(CFavoriteGames, BaseClass);

