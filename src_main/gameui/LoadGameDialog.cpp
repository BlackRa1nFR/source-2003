//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "LoadGameDialog.h"
#include "EngineInterface.h"

#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ListPanel.h>

#include <stdio.h>
#include <stdlib.h>
#include "FileSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Note, this must match engine version from host_cmd.h
#define	SAVEGAME_VERSION	0x0071		// Version 0.71

using namespace vgui;

const int CLoadGameDialog::SAVEGAME_MAPNAME_LEN = 32;
const int CLoadGameDialog::SAVEGAME_COMMENT_LEN = 80;
const int CLoadGameDialog::SAVEGAME_ELAPSED_LEN = 32;


int SaveReadNameAndComment( FileHandle_t f, char *name, char *comment );
int TimeStampSortFunc(const void *elem1, const void *elem2);

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CLoadGameDialog::CLoadGameDialog(vgui::Panel *parent) : CTaskFrame(parent, "LoadGameDialog")
{
	SetBounds(0, 0, 512, 384);
	SetMinimumSize( 256, 300 );
	SetSizeable( true );

	MakePopup();
	g_pTaskbar->AddTask(GetVPanel());

	SetTitle("#GameUI_LoadGame", true);

	new vgui::Label( this, "HelpText", "#GameUI_LoadGameHelp" );

	vgui::Button *load = new vgui::Button( this, "Load", "#GameUI_Load" );
	load->SetCommand( "Load" );

	vgui::Button *cancel = new vgui::Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	CreateSavedGamesList();
	ScanSavedGames();

	LoadControlSettings("Resource\\LoadGameDialog.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLoadGameDialog::~CLoadGameDialog()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CLoadGameDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "Load" )  )
	{
		KeyValues *item = m_pGameList->GetItem( m_pGameList->GetSelectedItem(0) );
		if ( item )
		{
			const char *shortName = item->GetString( "ShortName", "" );
			if ( shortName && shortName[ 0 ] )
			{
				// Load the game, return to top and switch to engine
				char sz[ 256 ];
				sprintf(sz, "load %s\n", shortName );
				
				engine->ClientCmd( sz );
				
				// Close this dialog
				OnClose();
			}
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadGameDialog::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}


void CLoadGameDialog::CreateSavedGamesList( void )
{
	// List
	m_pGameList = new vgui::ListPanel( this, "listpanel_loadgame" );

	// Add the column headers
	m_pGameList->AddColumnHeader( 0, "Game", "#GameUI_SavedGame", 175, true, RESIZABLE, RESIZABLE );
	m_pGameList->AddColumnHeader( 1, "Type", "#GameUI_Type", 100, true, RESIZABLE, RESIZABLE );
	m_pGameList->AddColumnHeader( 2, "Elapsed Time", "#GameUI_ElapsedTime", 75, true, RESIZABLE );
	m_pGameList->AddColumnHeader( 3, "Time", "#GameUI_TimeStamp", 175, true, RESIZABLE );

	m_pGameList->SetSortFunc(3, TimeStampSortFunc);
	m_pGameList->SetSortColumn(3);
}

bool CLoadGameDialog::ParseSaveData( char const *pszFileName, char const* pszShortName, KeyValues *kv )
{
	char    szMapName[SAVEGAME_MAPNAME_LEN];
	char    szComment[SAVEGAME_COMMENT_LEN];
	char    szElapsedTime[SAVEGAME_ELAPSED_LEN];

	if ( !pszFileName || !pszShortName || !kv )
		return false;

	kv->SetString( "ShortName", pszShortName );
	kv->SetString( "FileName", pszFileName );

	FileHandle_t fh = filesystem()->Open( pszFileName, "rb" );
	if (fh == FILESYSTEM_INVALID_HANDLE)
		return false;

	int readok = SaveReadNameAndComment( fh, szMapName, szComment );
	
	filesystem()->Close(fh);

	if ( !readok )
	{
		return false;
	}

	kv->SetString( "Map", szMapName );

	// Elapsed time is the last 5 characters in comment.
	int i;
	i = strlen( szComment );
	sprintf( szElapsedTime, "??" );
	if (i >= 6)
	{
		strncpy( szElapsedTime, (char *)&szComment[i - 5], 5 );
		szElapsedTime[5] = '\0';

		// Chop elpsed out of comment.
		int n;

		n = i - 5;
		szComment[n] = '\0';
	
		n--;

		// Strip back the spaces at the end.
		while ((n >= 1) &&
			szComment[n] &&
			szComment[n] == ' ')
		{
			szComment[n--] = '\0';
		}
	}

	// calculate the file name to print
	const char *pszType = NULL;
	if (strstr(pszFileName, "quick"))
	{
		pszType = "#GameUI_QuickSave";
	}
	else if (strstr(pszFileName, "autosave"))
	{
		pszType = "#GameUI_AutoSave";
	}

	kv->SetString( "Type", pszType );
	kv->SetString( "Game", szComment );

	kv->SetString( "Elapsed Time", szElapsedTime );

	// Now get file time stamp.
	long fileTime = filesystem()->GetFileTime(pszFileName);
	char szFileTime[32];
	filesystem()->FileTimeToString(szFileTime, sizeof(szFileTime), fileTime);
	char *newline = strstr(szFileTime, "\n");
	if (newline)
	{
		*newline = 0;
	}
	kv->SetString( "Time", szFileTime );
	kv->SetInt("timestamp", fileTime);

	return true;
}

void CLoadGameDialog::ScanSavedGames( void )
{
	// Populate list box with all saved games on record:
	char	szFileName[_MAX_PATH];
	char	szDirectory[_MAX_PATH];
	sprintf( szDirectory, "save/*.sav" );

	// FIXME:  Reset list?

	// Iterate the saved files
	FileFindHandle_t handle;
	char const* pFileName = filesystem()->FindFirst( szDirectory, &handle );
	while (pFileName)
	{
		if ( !strnicmp(pFileName, "HLSave", strlen( "HLSave" ) ) )
		{
			pFileName = filesystem()->FindNext( handle );
			continue;
		}

		sprintf(szFileName, "save/%s", pFileName);
		
		// Parse in data.
		KeyValues *item = new KeyValues( "SavedGame" );

		if ( ParseSaveData ( szFileName, pFileName, item ) )
		{
			m_pGameList->AddItem( item, 0, false, false );
		}
		
		item->deleteThis();

		pFileName = filesystem()->FindNext( handle );
	}
	
	filesystem()->FindClose( handle );

	// sort the list
	m_pGameList->SortList();

	// set the first item to be selected
	m_pGameList->SetSingleSelectedItem( m_pGameList->GetItemIDFromRow(0) );
}
