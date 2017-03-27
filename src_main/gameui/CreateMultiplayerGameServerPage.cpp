//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "CreateMultiplayerGameServerPage.h"

using namespace vgui;

#include <KeyValues.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/RadioButton.h>
#include "FileSystem.h"
#include "EngineInterface.h"

#include "ModInfo.h"

// for SRC
#include <vstdlib/random.h>
//#include "Random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define RANDOM_MAP "< Random Map >"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameServerPage::CCreateMultiplayerGameServerPage(vgui::Panel *parent, const char *name) : PropertyPage(parent, name)
{
	// we can use this if we decide we want to put "listen server" at the end of the game name
//	static char szHostName[256];
//	_snprintf( szHostName, sizeof( szHostName ) - 1, "%s %s", ModInfo().GetGameDescription(), "Listen Server" );
//	szHostName[sizeof( szHostName ) - 1] = '\0';

	m_pMapList = new ListPanel(this, "MapList");

	LoadControlSettings("Resource/CreateMultiplayerGameServerPage.res");

	m_pMapList->AddColumnHeader(0, "mapname", "#GameUI_Map", m_pMapList->GetWide(), true, RESIZABLE, RESIZABLE);
	LoadMapList();

	m_szMapName[0]  = 0;
	m_szHostName[0] = 0;
	m_szPassword[0] = 0;
	m_iMaxPlayers = engine->GetMaxClients();

	// make sure this will be a multiplayer game
	if ( m_iMaxPlayers <= 1 )
	{
		m_iMaxPlayers = 20; // this was the default for the old launcher
	}

	// initialize hostname
	SetControlString("ServerNameEdit", ModInfo().GetGameDescription());//szHostName);

	// initialize password
//	SetControlString("PasswordEdit", engine->pfnGetCvarString("sv_password"));
	ConVar const *var = cvar->FindVar( "sv_password" );
	if ( var )
	{
		SetControlString("PasswordEdit", var->GetString() );
	}


//	int maxPlayersEdit = atoi( GetControlString( "MaxPlayersEdit", "-1" ) );
//	if ( maxPlayersEdit <= 1 )
	{
		// initialize maxplayers
		char szBuffer[4];
		_snprintf(szBuffer, sizeof(szBuffer)-1, "%d", m_iMaxPlayers);
		szBuffer[sizeof(szBuffer)-1] = '\0';
		SetControlString("MaxPlayersEdit", szBuffer);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameServerPage::~CCreateMultiplayerGameServerPage()
{
}

//-----------------------------------------------------------------------------
// Purpose: called to get the info from the dialog
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameServerPage::OnApplyChanges()
{
	strncpy(m_szHostName, GetControlString("ServerNameEdit", "Half-Life"), DATA_STR_LENGTH);
	strncpy(m_szPassword, GetControlString("PasswordEdit", ""), DATA_STR_LENGTH);
	m_iMaxPlayers = atoi(GetControlString("MaxPlayersEdit", "8"));

	int selectedItemID = m_pMapList->GetSelectedItem(0);
	if (selectedItemID >= 0)
	{
		KeyValues *kv = m_pMapList->GetItem(selectedItemID);
		strncpy(m_szMapName, kv->GetString("mapname", ""), DATA_STR_LENGTH);
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads the list of available maps into the map list
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameServerPage::LoadMapList()
{
	// clear the current list (if any)
	m_pMapList->DeleteAllItems();

	// add special "name" to represent loading a randomly selected map
	m_pMapList->AddItem( new KeyValues( "data", "mapname", RANDOM_MAP ), 0, false, false );

	// iterate the filesystem getting the list of all the files
	// UNDONE: steam wants this done in a special way, need to support that
	FileFindHandle_t findHandle = NULL;
	const char *pathID = "GAME";
	if ( !stricmp(ModInfo().GetGameDescription(), "Half-Life" ) ) 
	{
		pathID = NULL; // hl is the base dir
	}

	const char *filename = filesystem()->FindFirst("maps/*.bsp", &findHandle);
	while (filename)
	{
		// remove the text 'maps/' and '.bsp' from the file name to get the map name
		char mapname[256];
		
		char *str = strstr(filename, "maps");
		if (str)
		{
			strncpy(mapname, str + 5, sizeof(mapname) - 1);	// maps + \\ = 5
		}
		else
		{
			strncpy(mapname, filename, sizeof(mapname) - 1);
		}
		str = strstr(mapname, ".bsp");
		if (str)
		{
			*str = 0;
		}

		//!! hack: strip out single player HL maps
		// this needs to be specified in a seperate file
		if (!stricmp(ModInfo().GetGameDescription(), "Half-Life" ) && (mapname[0] == 'c' || mapname[0] == 't') && mapname[2] == 'a' && mapname[1] >= '0' && mapname[1] <= '5')
		{
			goto nextFile;
		}



		// add to the map list
		m_pMapList->AddItem(new KeyValues("data", "mapname", mapname), 0, false, false);

		// get the next file
	nextFile:
		filename = filesystem()->FindNext(findHandle);
	}
	filesystem()->FindClose(findHandle);

	// set the first item to be selected
	if (m_pMapList->GetItemCount() > 0)
	{
		m_pMapList->SetSingleSelectedItem(m_pMapList->GetItemIDFromRow(0));
	}
}

const char *CCreateMultiplayerGameServerPage::GetMapName()
{
	int count = m_pMapList->GetItemCount();

	// if there is only one entry it's the special "select random map" entry
	if( count <= 1 )
		return NULL;

	const char *mapname = m_pMapList->GetItem(m_pMapList->GetSelectedItem(0))->GetString("mapname");
	if (!strcmp( mapname, RANDOM_MAP ))
	{
		int which = RandomInt( 1, count-1 );
		mapname = m_pMapList->GetItem( which )->GetString("mapname");
	}

	return mapname;
}

int CCreateMultiplayerGameServerPage::GetMaxPlayers()
{
	return atoi(GetControlString("MaxPlayersEdit", "8"));
}

const char *CCreateMultiplayerGameServerPage::GetPassword()
{
	return GetControlString("PasswordEdit", "");
}

const char *CCreateMultiplayerGameServerPage::GetHostName()
{
	return GetControlString("ServerNameEdit", "Half-Life");	
}