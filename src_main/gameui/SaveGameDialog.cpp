//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SaveGameDialog.h"
#include "EngineInterface.h"
#include "GameUI_Interface.h"

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <KeyValues.h>
#include <vgui_controls/Label.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/ListPanel.h>
#include <stdio.h>
#include <stdlib.h>
#include "FileSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#include "BaseUI/IBaseUI.h"
extern IBaseUI *baseuifuncs;

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// Note, this must match engine version from host_cmd.h
#define	SAVEGAME_VERSION	0x0071		// Version 0.71

const int CSaveGameDialog::SAVEGAME_MAPNAME_LEN = 32;
const int CSaveGameDialog::SAVEGAME_COMMENT_LEN = 80;
const int CSaveGameDialog::SAVEGAME_ELAPSED_LEN = 32;

extern int TimeStampSortFunc(const void *elem1, const void *elem2);

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CSaveGameDialog::CSaveGameDialog(vgui::Panel *parent) : CTaskFrame(parent, "SaveGameDialog")
{
	SetBounds(0, 0, 512, 384);
	SetSizeable( true );

	MakePopup();
	g_pTaskbar->AddTask(GetVPanel());

	SetTitle("#GameUI_SaveGame", true);

	new vgui::Label( this, "HelpText", "#GameUI_SaveGameHelp" );

	vgui::Button *savegame = new vgui::Button( this, "Save", "#GameUI_Save" );
	savegame->SetCommand( "Save" );

	vgui::Button *cancel = new vgui::Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	CreateSavedGamesList();
	ScanSavedGames();

	LoadControlSettings("Resource\\SaveGameDialog.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSaveGameDialog::~CSaveGameDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CSaveGameDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "Save" )  )
	{
		KeyValues *item = m_pGameList->GetItem( m_pGameList->GetSelectedItem(0) );
		if ( item )
		{
			const char *shortName = item->GetString( "ShortName", "" );
			if ( shortName && shortName[ 0 ] )
			{
				// Load the game, return to top and switch to engine
				char sz[ 256 ];
				sprintf(sz, "save %s\n", shortName );

				engine->ClientCmd( sz );

				if(baseuifuncs)
				{
					baseuifuncs->HideGameUI();
				}

				
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
void CSaveGameDialog::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}


void CSaveGameDialog::CreateSavedGamesList( void )
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

#define MAKEID(d,c,b,a)	( ((int)(a) << 24) | ((int)(b) << 16) | ((int)(c) << 8) | ((int)(d)) )

int SaveReadNameAndComment( FileHandle_t f, char *name, char *comment )
{
	int i, tag, size, tokenSize, tokenCount;
	char *pSaveData, *pFieldName, **pTokenList;

	filesystem()->Read( &tag, sizeof(int), f );
	if ( tag != MAKEID('J','S','A','V') )
	{
		return 0;
	}
		
	filesystem()->Read( &tag, sizeof(int), f );
	if ( tag != SAVEGAME_VERSION )				// Enforce version for now
	{
		return 0;
	}

	name[0] = '\0';
	comment[0] = '\0';
	filesystem()->Read( &size, sizeof(int), f );
	
	filesystem()->Read( &tokenCount, sizeof(int), f );	// These two ints are the token list
	filesystem()->Read( &tokenSize, sizeof(int), f );
	size += tokenSize;

	// Sanity Check.
	if ( tokenCount < 0 || tokenCount > 1024*1024*32  )
	{
		return 0;
	}

	if ( tokenSize < 0 || tokenSize > 1024*1024*32  )
	{
		return 0;
	}

	pSaveData = (char *)new char[size];
	filesystem()->Read(pSaveData, size, f);

	int nNumberOfFields;

	char *pData;
	int nFieldSize;
	
	pData = pSaveData;

	// Allocate a table for the strings, and parse the table
	if ( tokenSize > 0 )
	{
		pTokenList = new char *[tokenCount];

		// Make sure the token strings pointed to by the pToken hashtable.
		for( i=0; i<tokenCount; i++ )
		{
			pTokenList[i] = *pData ? pData : NULL;	// Point to each string in the pToken table
			while( *pData++ );				// Find next token (after next null)
		}
	}
	else
		pTokenList = NULL;

	// short, short (size, index of field name)
	nFieldSize = *(short *)pData;
	pData += sizeof(short);
	pFieldName = pTokenList[ *(short *)pData ];

	if (stricmp(pFieldName, "GameHeader"))
	{
		delete[] pSaveData;
		return 0;
	};

	// int (fieldcount)
	pData += sizeof(short);
	nNumberOfFields = (int)*pData;
	pData += nFieldSize;

	// Each field is a short (size), short (index of name), binary string of "size" bytes (data)
	for (i = 0; i < nNumberOfFields; i++)
	{
		// Data order is:
		// Size
		// szName
		// Actual Data

		nFieldSize = *(short *)pData;
		pData += sizeof(short);

		pFieldName = pTokenList[ *(short *)pData ];
		pData += sizeof(short);

		if (!stricmp(pFieldName, "comment"))
		{
			strncpy(comment, pData, nFieldSize);
		}
		else if (!stricmp(pFieldName, "mapName"))
		{
			strncpy(name, pData, nFieldSize);
		};

		// Move to Start of next field.
		pData += nFieldSize;
	};

	// Delete the string table we allocated
	delete[] pTokenList;
	delete[] pSaveData;
	
	if (strlen(name) > 0 && strlen(comment) > 0)
		return 1;
	
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: sort function for timestamp column
//-----------------------------------------------------------------------------
int TimeStampSortFunc(const void *elem1, const void *elem2)
{
	vgui::ListPanelItem *p1, *p2;
	p1 = *(vgui::ListPanelItem **)elem1;
	p2 = *(vgui::ListPanelItem **)elem2;
	assert(p1 && p2);

	if (!p1)
		return -1;
	if (!p2)
		return 1;

	int t1 = p1->kv->GetInt("timestamp", 0);
	int t2 = p2->kv->GetInt("timestamp", 0);
	assert(t1 > 0);
	assert(t2 > 0);

	return (t1 < t2) ? 1 : -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSaveGameDialog::ParseSaveData( char const *pszFileName, char const* pszShortName, KeyValues *kv )
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
	kv->SetInt( "Quick", strstr(pszFileName, "quick") ? 1 : 0 );
	kv->SetInt( "Autosave", strstr(pszFileName, "autosave") ? 1 : 0 );

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
	kv->SetString("Time", szFileTime);
	kv->SetInt("timestamp", fileTime);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSaveGameDialog::ScanSavedGames( void )
{
	// Populate list box with all saved games on record:
	char	szFileName[_MAX_PATH];
	char	szDirectory[_MAX_PATH];
	sprintf( szDirectory, "save/*.sav" );

	// FIXME:  Reset list?

	// Iterate the saved files
	FileFindHandle_t handle;
	char const *pFileName = filesystem()->FindFirst( szDirectory, &handle );
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

	// create dummy item for current saved game
	KeyValues *dummy = new KeyValues("SavedGame");
	dummy->SetString("Game", "New saved game");
	dummy->SetString("Elapsed Time", "New");
	dummy->SetString("Time", "Current");
	dummy->SetInt("timestamp", 0x7FFFFFFF);
	dummy->SetString("ShortName", FindSaveSlot());
	m_pGameList->AddItem(dummy, 0, false, false);

	// sort the list
	m_pGameList->SortList();

	// set the first item to be selected
	m_pGameList->SetSingleSelectedItem( m_pGameList->GetItemIDFromRow(0) );
}

//-----------------------------------------------------------------------------
// Purpose: generates a new save game name
//-----------------------------------------------------------------------------
const char *CSaveGameDialog::FindSaveSlot()
{
	static char szFileName[512];
	for (int i = 0; i < 1000; i++)
	{
		sprintf(szFileName, "save/Half-Life-%03i.sav", i );

		FileHandle_t fp = filesystem()->Open( szFileName, "rb" );
		if (!fp)
		{
			return szFileName + 5;
		}
		filesystem()->Close(fp);
	}

	assert(!("Could not generate new save game file"));
	return "error.sav";
}