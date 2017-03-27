//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
// Save / Restore System
#include "glquake.h"
#include <ctype.h>
#include "server.h"
#include "vengineserver_impl.h"
#include "host_cmd.h"
#include "sys.h"
#include "cdll_int.h"
#include "tmessage.h"
#include "screen.h"
#include "game_interface.h"
#include "decal.h"
#include "zone.h"
#include "sv_main.h"
#include "host.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "host_state.h"
#include "datamap.h"
#include "string_t.h"
#include "PlayerState.h"
#include "saverestoretypes.h"
#include "demo.h"
#include "icliententity.h"
#include "r_efx.h"
#include "icliententitylist.h"
#include "cdll_int.h"
#include "utldict.h"
#include "decal_private.h"
#include "engine/IEngineTrace.h"
#include "enginetrace.h"
#include "baseautocompletefilelist.h"

extern IBaseClientDLL *g_ClientDLL;

extern ConVar	deathmatch;
extern ConVar	skill;

// Keep the last 1 autosave / quick saves
#define		SAVE_AGED_COUNT		1			
#define		MAX_SAVEGAMES		12

//-----------------------------------------------------------------------------
// Purpose: Alloc/free memory for save games
// Input  : num - 
//			size - 
//-----------------------------------------------------------------------------
void *SaveAllocMemory( size_t num, size_t size )
{
	return calloc( num, size );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSaveMem - 
//-----------------------------------------------------------------------------
void SaveFreeMemory( void *pSaveMem )
{
	if ( pSaveMem )
	{
		free( pSaveMem );
	}
}

struct GAME_HEADER
{
	DECLARE_SIMPLE_DATADESC();

	char	mapName[32];
	char	comment[80];
	int		mapCount;
};

struct SAVE_HEADER 
{
	DECLARE_SIMPLE_DATADESC();

	int		saveId;
	int		version;
	int		skillLevel;
	int		connectionCount;
	int		lightStyleCount;
	float	time;
	char	mapName[32];
	char	skyName[32];
//	int		skyColor_r;
//	int		skyColor_g;
//	int		skyColor_b;
//	float	skyVec_x;
//	float	skyVec_y;
//	float	skyVec_z;
//	char	comment[80];
};

struct SAVELIGHTSTYLE 
{
	DECLARE_SIMPLE_DATADESC();

	int		index;
	char	style[64];
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSaveRestore : public ISaveRestore
{
public:
	CSaveRestore()
	{
		m_bClearSaveDir = false;
	}

	void					Init( void );
	void					Shutdown( void );
	bool					LoadGame( const char *pName );
	bool					LoadGameNoScreen(const char *pName);
	int						SaveGame( const char *pszSlot, const char *pszComment );
	char					*GetSaveDir(void);
	void					ClearSaveDir( void );
	void					RequestClearSaveDir( void );
	int						LoadGameState( char const *level, bool createPlayers );
	void					LoadAdjacentEnts( const char *pOldLevel, const char *pLandmarkName );
	const char				*FindRecentSave( char *pNameBuf, int nameBufLen );
	int						SaveGameSlot( const char *pSaveName, const char *pSaveComment );
	CSaveRestoreData			*SaveGameState( void );
	void					RestoreClientState( char const *fileName, bool adjacent );
	void					RestoreAdjacenClientState( char const *map );
	int						IsValidSave( void );
	void					Finish( CSaveRestoreData *save );
	void					ClearRestoredIndexTranslationTables();
	void					OnFinishedClientRestore();

private:
	void					SaveClientState( CSaveRestoreData *serverData, const char *name );

	void					EntityPatchWrite( CSaveRestoreData *pSaveData, const char *level );
	void					EntityPatchRead( CSaveRestoreData *pSaveData, const char *level );
	int						DirectoryCount( const char *pPath );
	void					DirectoryCopy( const char *pPath, FileHandle_t pFile );
	void					DirectoryExtract( FileHandle_t pFile, int mapCount );
	void					DirectoryClear( const char *pPath );

	void					AgeSaveList( const char *pName, int count );
	int						SaveReadHeader( FileHandle_t pFile, GAME_HEADER *pHeader, int readGlobalState );
	CSaveRestoreData			*LoadSaveData( const char *level );
	void					ParseSaveTables( CSaveRestoreData *pSaveData, SAVE_HEADER *pHeader, int updateGlobals );
	int						FileSize( FileHandle_t pFile );
	void					FileCopy( FileHandle_t pOutput, FileHandle_t pInput, int fileSize );

	bool					CalcSaveGameName( const char *pName, char *output, int outputStringLength );

	CSaveRestoreData *		SaveGameStateInit( void );
	FileHandle_t 			OpenGameStateSaveFile( void );
	void 					SaveGameStateGlobals( CSaveRestoreData *pSaveData );

	void					BuildRestoredIndexTranslationTable( char const *mapname, CSaveRestoreData *pSaveData, bool verbose );

	struct SaveRestoreTranslate
	{
		string_t classname;
		int savedindex;
		int restoredindex;
	};

	struct RestoreLookupTable
	{
		RestoreLookupTable()
		{
		}

		void Clear()
		{
			lookup.RemoveAll();
		}

		RestoreLookupTable( const RestoreLookupTable& src )
		{
			int c = src.lookup.Count();
			for ( int i = 0 ; i < c; i++ )
			{
				lookup.AddToTail( src.lookup[ i ] );
			}

			m_vecLandMarkOffset = src.m_vecLandMarkOffset;
		}

		RestoreLookupTable& operator=( const RestoreLookupTable& src )
		{
			if ( this == &src )
				return *this;

			int c = src.lookup.Count();
			for ( int i = 0 ; i < c; i++ )
			{
				lookup.AddToTail( src.lookup[ i ] );
			}

			m_vecLandMarkOffset = src.m_vecLandMarkOffset;

			return *this;
		}

		CUtlVector< SaveRestoreTranslate >	lookup;
		Vector								m_vecLandMarkOffset;
	};

	RestoreLookupTable		*FindOrAddRestoreLookupTable( char const *mapname );
	int						LookupRestoreSpotSaveIndex( RestoreLookupTable *table, int save );
	void					ReapplyDecal( bool adjacent, RestoreLookupTable *table, decallist_t *entry );


	CUtlDict< RestoreLookupTable, int >	m_RestoreLookup;

	bool	m_bClearSaveDir;
};

CSaveRestore g_SaveRestore;
ISaveRestore *saverestore = (ISaveRestore *)&g_SaveRestore;

bool SaveRestore_LoadGame( const char *pName )
{
	return saverestore->LoadGameNoScreen( pName );
}

int	SaveRestore_SaveGame( const char *pszSlot, const char *pszComment )
{
	return saverestore->SaveGame( pszSlot, pszComment );
}

BEGIN_SIMPLE_DATADESC( GAME_HEADER )

	DEFINE_FIELD( GAME_HEADER, mapCount, FIELD_INTEGER ),
	DEFINE_ARRAY( GAME_HEADER, mapName, FIELD_CHARACTER, 32 ),
	DEFINE_ARRAY( GAME_HEADER, comment, FIELD_CHARACTER, 80 ),

END_DATADESC()


// The proper way to extend the file format (add a new data chunk) is to add a field here, and use it to determine
// whether your new data chunk is in the file or not.  If the file was not saved with your new field, the chunk 
// won't be there either.
// Structure members can be added/deleted without any problems, new structures must be reflected in an existing struct
// and not read unless actually in the file.  New structure members will be zeroed out when reading 'old' files.

BEGIN_SIMPLE_DATADESC( SAVE_HEADER )

//	DEFINE_FIELD( SAVE_HEADER, saveId, FIELD_INTEGER ),
//	DEFINE_FIELD( SAVE_HEADER, version, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, skillLevel, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, connectionCount, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, lightStyleCount, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, time, FIELD_TIME ),
	DEFINE_ARRAY( SAVE_HEADER, mapName, FIELD_CHARACTER, 32 ),
	DEFINE_ARRAY( SAVE_HEADER, skyName, FIELD_CHARACTER, 32 ),

//	DEFINE_FIELD( SAVE_HEADER, skyColor_r, FIELD_INTEGER ),
//	DEFINE_FIELD( SAVE_HEADER, skyColor_g, FIELD_INTEGER ),
//	DEFINE_FIELD( SAVE_HEADER, skyColor_b, FIELD_INTEGER ),
//	DEFINE_FIELD( SAVE_HEADER, skyVec_x, FIELD_FLOAT ),
//	DEFINE_FIELD( SAVE_HEADER, skyVec_y, FIELD_FLOAT ),
//	DEFINE_FIELD( SAVE_HEADER, skyVec_z, FIELD_FLOAT ),
//	DEFINE_ARRAY( SAVE_HEADER, comment, FIELD_CHARACTER, 80 ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( levellist_t )
	DEFINE_ARRAY( levellist_t, mapName, FIELD_CHARACTER, 32 ),
	DEFINE_ARRAY( levellist_t, landmarkName, FIELD_CHARACTER, 32 ),
	DEFINE_FIELD( levellist_t, pentLandmark, FIELD_EDICT ),
	DEFINE_FIELD( levellist_t, vecLandmarkOrigin, FIELD_VECTOR ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( SAVELIGHTSTYLE )
	DEFINE_FIELD( SAVELIGHTSTYLE, index, FIELD_INTEGER ),
	DEFINE_ARRAY( SAVELIGHTSTYLE, style, FIELD_CHARACTER, 64 ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pNameBuf - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CSaveRestore::FindRecentSave( char *pNameBuf, int nameBufLen )
{
	char const*	findfn;
	char basefilename[ MAX_OSPATH ];
	int				found;
	long	newest = 0, ft;
	char			szPath[ MAX_OSPATH ];

	Q_snprintf( pNameBuf, nameBufLen, "%s*.sav", GetSaveDir() );
	Q_snprintf( szPath, sizeof( szPath ), "%s", GetSaveDir() );

	findfn = Sys_FindFirst(pNameBuf, basefilename);
	
	found = 0;

	while ( findfn )
	{
		// Don't load HLSave.sav -- it's a temporary file used by the launcher when switching video modes
		if ( Q_strlen( findfn ) != 0 &&
			 Q_strcasecmp(findfn , "HLSave.sav" ) )
		{
			Q_snprintf( szPath, sizeof( szPath ), "%s%s", GetSaveDir() , findfn );
			ft = g_pFileSystem->GetFileTime( szPath );
			// Found a match?
			if ( ft > 0 )
			{
				// Should we use the matche?
				if ( !found || Sys_CompareFileTime( newest, ft ) < 0 )
				{
					newest = ft;
					strcpy( pNameBuf, findfn );
					found = 1;
				}
			}
		}

		// Any more save files
		findfn = Sys_FindNext( basefilename );
	}

	Sys_FindClose();

	if ( found )
		return pNameBuf;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the save game directory for the current player profile
// Output : char
//-----------------------------------------------------------------------------
char *CSaveRestore::GetSaveDir(void)
{
	static char szDirectory[MAX_OSPATH];
	memset(szDirectory, 0, MAX_OSPATH);
	strcpy(szDirectory, "SAVE/" );

	return szDirectory;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//			count - 
//-----------------------------------------------------------------------------
void CSaveRestore::AgeSaveList( const char *pName, int count )
{
	char newName[MAX_OSPATH], oldName[MAX_OSPATH];

	// Delete last quick/autosave (e.g. quick05.sav)


	Q_snprintf( newName, sizeof( newName ), "%s%s%02d.sav", GetSaveDir(), pName, count );
	COM_FixSlashes( newName );
	g_pFileSystem->RemoveFile( newName, "GAME" );

	while ( count > 0 )
	{
		if ( count == 1 )
			Q_snprintf( oldName, sizeof( oldName ), "%s%s.sav", GetSaveDir(), pName );// quick.sav
		else
			Q_snprintf( oldName, sizeof( oldName ), "%s%s%02d.sav", GetSaveDir(), pName, count-1 );	// quick04.sav, etc.

		COM_FixSlashes( oldName );
		Q_snprintf( newName, sizeof( newName ), "%s%s%02d.sav", GetSaveDir(), pName, count );
		COM_FixSlashes( newName );

		// Scroll the name list down (rename quick04.sav to quick05.sav)
		g_pFileSystem->RenameFile( oldName, newName, "GAME" );
		count--;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CSaveRestore::IsValidSave( void )
{
	if (cmd_source != src_command)
		return 0;

	// Don't parse autosave/transition save/restores during playback!
	if ( Demo_IsPlayingBack() )
	{
		return 0;
	}

	if (!sv.active)
	{
		Con_Printf ("Not playing a local game.\n");
		return 0;
	}

	if (cls.state != ca_active)
	{
		Con_Printf ("Can't save if not active.\n");
		return 0;
	}

	if (svs.maxclients != 1)
	{
		Con_Printf ("Can't save multiplayer games.\n");
		return 0;
	}

	if ( svs.clients[0].active )
	{
		Assert( serverGameClients );
		CPlayerState *pl = serverGameClients->GetPlayerState( svs.clients[0].edict );
		if ( !pl )
		{
			Con_Printf ("Can't savegame without a player!\n");
			return 0;
		}
			
		if ( pl->deadflag != false )
		{
			Con_Printf ("Can't savegame with a dead player\n");
			return 0;
		}
	}
	
	// Passed all checks, it's ok to save
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSaveName - 
//			*pSaveComment - 
// Output : int
//-----------------------------------------------------------------------------
int CSaveRestore::SaveGameSlot( const char *pSaveName, const char *pSaveComment )
{
	char			hlPath[256], name[256], *pTokenData;
	int				tag, i, tokenSize;
	FileHandle_t pFile;
	CSaveRestoreData	*pSaveData;
	GAME_HEADER		gameHeader;

	pSaveData = SaveGameState();
	if (!pSaveData)
		return 0;

	Finish( pSaveData );

	pSaveData = serverGameDLL->SaveInit( 0 );

	Q_snprintf( hlPath, sizeof( hlPath ), "%s*.HL?", GetSaveDir() );
	COM_FixSlashes( hlPath );
	gameHeader.mapCount = DirectoryCount( hlPath );
	strcpy( gameHeader.mapName, sv.name );
	strcpy( gameHeader.comment, pSaveComment );

	serverGameDLL->SaveWriteFields( pSaveData, "GameHeader", &gameHeader, NULL, GAME_HEADER::m_DataMap.dataDesc, GAME_HEADER::m_DataMap.dataNumFields );
	serverGameDLL->SaveGlobalState( pSaveData );

	// Write entity string token table
	pTokenData = pSaveData->AccessCurPos();
	for( i = 0; i < pSaveData->SizeSymbolTable(); i++ )
	{
		const char *pszToken = (pSaveData->StringFromSymbol( i )) ? pSaveData->StringFromSymbol( i ) : "";
		if ( !pSaveData->Write( pszToken, strlen(pszToken) + 1 ) )
		{
			Con_Printf( "Token Table Save/Restore overflow!" );
			break;
		}
	}	

	tokenSize = pSaveData->AccessCurPos() - pTokenData;
	pSaveData->Rewind( tokenSize );

	Q_snprintf( name, sizeof( name ), "%s%s", GetSaveDir(), pSaveName );
	COM_DefaultExtension( name, ".sav", sizeof( name ) );
	COM_FixSlashes( name );
	Con_DPrintf( "Saving game to %s...\n", name );
	// Output to disk
	if ( stricmp(pSaveName, "quick") || stricmp(pSaveName,"autosave") )
		AgeSaveList( pSaveName, SAVE_AGED_COUNT );

	pFile = g_pFileSystem->Open( name, "wb" );
	// Write the header -- THIS SHOULD NEVER CHANGE STRUCTURE, USE SAVE_HEADER FOR NEW HEADER INFORMATION
	// THIS IS ONLY HERE TO IDENTIFY THE FILE AND GET IT'S SIZE.
	tag = MAKEID('J','S','A','V');
	g_pFileSystem->Write( &tag, sizeof(int), pFile );
	tag = SAVEGAME_VERSION;
	g_pFileSystem->Write( &tag, sizeof(int), pFile );
	tag = pSaveData->GetCurPos();
	g_pFileSystem->Write( &tag, sizeof(int), pFile ); // Does not include token table

	// Write out the tokens first so we can load them before we load the entities
	tag = pSaveData->SizeSymbolTable();
	g_pFileSystem->Write( &tag, sizeof(int), pFile );
	g_pFileSystem->Write( &tokenSize, sizeof(int), pFile );
	g_pFileSystem->Write( pTokenData, tokenSize, pFile );

	g_pFileSystem->Write( pSaveData->GetBuffer(), pSaveData->GetCurPos(), pFile );

	DirectoryCopy( hlPath, pFile );
	g_pFileSystem->Close( pFile );
	Finish( pSaveData );

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszSlot - 
//			*pszComment - 
// Output : int
//-----------------------------------------------------------------------------
int CSaveRestore::SaveGame(const char *pszSlot, const char *pszComment)
{
	int iret;
#ifdef SWDS
	qboolean q = scr_skipupdate;
	scr_skipupdate = true;
	iret = SaveGameSlot(pszSlot, pszComment);
	scr_skipupdate = q;
#else
	iret = 0;
#endif
	return iret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pHeader - 
//			readGlobalState - 
// Output : int
//-----------------------------------------------------------------------------
int CSaveRestore::SaveReadHeader( FileHandle_t pFile, GAME_HEADER *pHeader, int readGlobalState )
{
	int					i, tag, size, tokenCount, tokenSize;
	char				*pszTokenList;
	CSaveRestoreData		*pSaveData;

	g_pFileSystem->Read( &tag, sizeof(int), pFile );
	if ( tag != MAKEID('J','S','A','V') )
	{
		g_pFileSystem->Close( pFile );
		return 0;
	}
		
	g_pFileSystem->Read( &tag, sizeof(int), pFile );
	if ( tag != SAVEGAME_VERSION )				// Enforce version for now
	{
		g_pFileSystem->Close( pFile );
		return 0;
	}

	g_pFileSystem->Read( &size, sizeof(int), pFile );


	g_pFileSystem->Read( &tokenCount, sizeof(int), pFile );
	g_pFileSystem->Read( &tokenSize, sizeof(int), pFile );

	pSaveData = MakeSaveRestoreData( SaveAllocMemory( sizeof(CSaveRestoreData) + tokenSize + size, sizeof(char) ) );

	pSaveData->levelInfo.connectionCount = 0;

	pszTokenList = (char *)(pSaveData + 1);

	if ( tokenSize > 0 )
	{
		g_pFileSystem->Read( pszTokenList, tokenSize, pFile );

		pSaveData->InitSymbolTable( (char**)SaveAllocMemory( tokenCount, sizeof(char *) ), tokenCount );

		// Make sure the token strings pointed to by the pToken hashtable.
		for( i=0; i<tokenCount; i++ )
		{
			if ( *pszTokenList )
			{
				Verify( pSaveData->DefineSymbol( pszTokenList, i ) );
			}
			while( *pszTokenList++ );				// Find next token (after next null)
		}
	}
	else
		pSaveData->InitSymbolTable( NULL, 0 );


	pSaveData->levelInfo.fUseLandmark = false;
	pSaveData->levelInfo.time = 0;

	// pszTokenList now points after token data
	pSaveData->Init( pszTokenList, size ); 
	g_pFileSystem->Read( pSaveData->GetBuffer(), size, pFile );
	
	serverGameDLL->SaveReadFields( pSaveData, "GameHeader", pHeader, NULL, GAME_HEADER::m_DataMap.dataDesc, GAME_HEADER::m_DataMap.dataNumFields );
	if ( readGlobalState )
		serverGameDLL->RestoreGlobalState( pSaveData );
	Finish( pSaveData );
	
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//			*output - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSaveRestore::CalcSaveGameName( const char *pName, char *output, int outputStringLength )
{
	if (!pName || !pName[0])
		return false;

	Q_snprintf( output, outputStringLength, "%s%s", GetSaveDir(), pName );
	COM_DefaultExtension( output, ".sav", outputStringLength );
	COM_FixSlashes( output );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
// Output : int
//-----------------------------------------------------------------------------
bool CSaveRestore::LoadGame( const char *pName )
{
	FileHandle_t	pFile;
	GAME_HEADER		gameHeader;
	char			name[256];
	bool			validload = false;

	if ( !CalcSaveGameName( pName, name, sizeof( name ) ) )
		return false;

	Con_Printf( "Loading game from %s...\n", name );
	ClearSaveDir();

	pFile = g_pFileSystem->Open( name, "rb" );
	if ( pFile )
	{
		if ( SaveReadHeader( pFile, &gameHeader, 1 ) )
		{
			DirectoryExtract( pFile, gameHeader.mapCount );
			validload = true;
		}
		g_pFileSystem->Close( pFile );
	}
	else
		Con_Printf( "File not found or failed to open.\n" );

	if ( !validload )
	{
		Host_Disconnect();
		return false;
	}

	// Put up loading plaque
	SCR_BeginLoadingPlaque ();

	// stop demo loop in case this fails
	cls.demonum = -1;		

	deathmatch.SetValue( 0 );
	coop.SetValue( 0 );

	return Host_NewGame( gameHeader.mapName, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
// Output : int
//-----------------------------------------------------------------------------
bool CSaveRestore::LoadGameNoScreen( const char *pName )
{
	qboolean q = scr_skipupdate;
	scr_skipupdate = true;
	bool ret = LoadGame( pName );
	scr_skipupdate = q;
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CSaveRestoreData
//-----------------------------------------------------------------------------

struct SaveFileHeaderTag_t
{
	int id;
	int version;
	
	bool operator==(const SaveFileHeaderTag_t &rhs) const { return ( memcmp( this, &rhs, sizeof(SaveFileHeaderTag_t) ) == 0 ); }
	bool operator!=(const SaveFileHeaderTag_t &rhs) const { return ( memcmp( this, &rhs, sizeof(SaveFileHeaderTag_t) ) != 0 ); }
};

const struct SaveFileHeaderTag_t CURRENT_SAVEFILE_HEADER_TAG = { MAKEID('V','A','L','V'), SAVEGAME_VERSION };

struct SaveFileSectionsInfo_t
{
	int nBytesSymbols;
	int nSymbols;
	int nBytesDataHeaders;
	int nBytesData;
	
	int SumBytes() const
	{
		return ( nBytesSymbols + nBytesDataHeaders + nBytesData );
	}
};

struct SaveFileSections_t
{
	char *pSymbols;
	char *pDataHeaders;
	char *pData;
};

FileHandle_t CSaveRestore::OpenGameStateSaveFile( void )
{
	char			name[256];
	FileHandle_t	pFile;
	
	Q_snprintf( name, sizeof( name ), "%s%s.HL1", GetSaveDir(), sv.name);
	COM_FixSlashes( name );
	COM_CreatePath( name );

	pFile = g_pFileSystem->Open( name, "wb" );
	
	if ( !pFile )
		Con_Printf("Unable to open save game file %s.", name);
	
	return pFile;
}

void CSaveRestore::SaveGameStateGlobals( CSaveRestoreData *pSaveData )
{
	SAVE_HEADER header;
	
	// Write global data
	header.version 			= build_number( );
	header.skillLevel 		= skill.GetInt();	// This is created from an int even though it's a float
	header.connectionCount 	= pSaveData->levelInfo.connectionCount;
	header.time 			= sv.gettime();
	ConVar const *skyname = cv->FindVar( "sv_skyname" );
	if ( skyname )
	{
		strcpy( header.skyName, skyname->GetString() );
	}
	else
	{
		strcpy( header.skyName, "unknown" );
	}

	strcpy( header.mapName, sv.name );
	header.lightStyleCount 	= 0;
	int i;
	for ( i = 0; i < MAX_LIGHTSTYLES; i++ )
		if ( sv.lightstyles[i] )
			header.lightStyleCount++;

	pSaveData->levelInfo.time = 0; // prohibits rebase of header.time (why not just save time as a field_float and ditch this hack?)
	serverGameDLL->SaveWriteFields( pSaveData, "Save Header", &header, NULL, SAVE_HEADER::m_DataMap.dataDesc, SAVE_HEADER::m_DataMap.dataNumFields );
	pSaveData->levelInfo.time = header.time;

	// Write adjacency list
	for ( i = 0; i < pSaveData->levelInfo.connectionCount; i++ )
		serverGameDLL->SaveWriteFields( pSaveData, "ADJACENCY", pSaveData->levelInfo.levelList + i, NULL, levellist_t::m_DataMap.dataDesc, levellist_t::m_DataMap.dataNumFields );

	// Write the lightstyles
	SAVELIGHTSTYLE	light;
	for ( i = 0; i < MAX_LIGHTSTYLES; i++ )
	{
		if ( sv.lightstyles[i] )
		{
			light.index = i;
			strcpy( light.style, sv.lightstyles[i] );
			serverGameDLL->SaveWriteFields( pSaveData, "LIGHTSTYLE", &light, NULL, SAVELIGHTSTYLE::m_DataMap.dataDesc, SAVELIGHTSTYLE::m_DataMap.dataNumFields );
		}
	}
}

CSaveRestoreData *CSaveRestore::SaveGameStateInit( void )
{
	CSaveRestoreData *pSaveData = serverGameDLL->SaveInit( 0 );

	return pSaveData;
}

CSaveRestoreData *CSaveRestore::SaveGameState( void )
{
	SaveFileSectionsInfo_t sectionsInfo;
	SaveFileSections_t sections;
	
	CSaveRestoreData *pSaveData = SaveGameStateInit();
	
	//---------------------------------
	// Save the data
	sections.pData = pSaveData->AccessCurPos();
	
	//---------------------------------
	// Pre-save

	serverGameDLL->PreSave( pSaveData );
	// Build the adjacent map list (after entity table build by game in presave)
	serverGameDLL->BuildAdjacentMapList();

	//---------------------------------

	SaveGameStateGlobals( pSaveData );

	serverGameDLL->Save( pSaveData );
	
	sectionsInfo.nBytesData = pSaveData->AccessCurPos() - sections.pData;

	
	//---------------------------------
	// Save necessary tables/dictionaries/directories
	sections.pDataHeaders = pSaveData->AccessCurPos();
	
	serverGameDLL->WriteSaveHeaders( pSaveData );
	
	sectionsInfo.nBytesDataHeaders = pSaveData->AccessCurPos() - sections.pDataHeaders;

	//---------------------------------
	// Write the save file symbol table
	sections.pSymbols = pSaveData->AccessCurPos();
	for( int i = 0; i < pSaveData->SizeSymbolTable(); i++ )
	{
		const char *pszToken = ( pSaveData->StringFromSymbol( i ) ) ? pSaveData->StringFromSymbol( i ) : "";
		if ( !pSaveData->Write( pszToken, strlen(pszToken) + 1 ) )
		{
			break;
		}
	}	

	sectionsInfo.nBytesSymbols = pSaveData->AccessCurPos() - sections.pSymbols;
	sectionsInfo.nSymbols = pSaveData->SizeSymbolTable();

	//---------------------------------
	// Output to disk
	FileHandle_t pFile = OpenGameStateSaveFile();
	if ( !pFile )
		return NULL;

	// Write the header -- THIS SHOULD NEVER CHANGE STRUCTURE, USE SAVE_HEADER FOR NEW HEADER INFORMATION
	// THIS IS ONLY HERE TO IDENTIFY THE FILE AND GET IT'S SIZE.
	g_pFileSystem->Write( &CURRENT_SAVEFILE_HEADER_TAG, sizeof(CURRENT_SAVEFILE_HEADER_TAG), pFile );

	// Write out the tokens and table FIRST so they are loaded in the right order, then write out the rest of the data in the file.
	g_pFileSystem->Write( &sectionsInfo, sizeof(sectionsInfo), pFile );
	g_pFileSystem->Write( sections.pSymbols, sectionsInfo.nBytesSymbols, pFile );
	g_pFileSystem->Write( sections.pDataHeaders, sectionsInfo.nBytesDataHeaders, pFile );
	g_pFileSystem->Write( sections.pData, sectionsInfo.nBytesData, pFile );
	g_pFileSystem->Close( pFile );
	
	//---------------------------------
	
	EntityPatchWrite( pSaveData, sv.name );

	char name[256];
	Q_snprintf(name, sizeof( name ), "%s%s.HL2", GetSaveDir(), sv.name);
	COM_FixSlashes( name );
	// Let the client see the server entity to id lookup tables, etc.
	SaveClientState( pSaveData, name );

	return pSaveData;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *save - 
//-----------------------------------------------------------------------------
void CSaveRestore::Finish( CSaveRestoreData *save )
{
	char **pTokens = save->DetachSymbolTable();
	if ( pTokens )
		SaveFreeMemory( pTokens );

	entitytable_t *pEntityTable = save->DetachEntityTable();
	if ( pEntityTable )
		SaveFreeMemory( pEntityTable );

	if ( save )
		SaveFreeMemory( save );

	g_ServerGlobalVariables.pSaveData = NULL;
}

BEGIN_SIMPLE_DATADESC( decallist_t )

	DEFINE_FIELD( decallist_t, position, FIELD_POSITION_VECTOR ),
	DEFINE_ARRAY( decallist_t, name, FIELD_CHARACTER, 128 ),
	DEFINE_FIELD( decallist_t, entityIndex, FIELD_SHORT ),
	//	DEFINE_FIELD( decallist_t, depth, FIELD_CHARACTER ),
	DEFINE_FIELD( decallist_t, flags, FIELD_CHARACTER ),
	DEFINE_FIELD( decallist_t, impactPlaneNormal, FIELD_VECTOR ),

END_DATADESC()

struct baseclientsections_t
{
	int entitysize;
	int headersize;
	int decalsize;
	int symbolsize;

	int decalcount;
	int symbolcount;

	int SumBytes()
	{
		return entitysize + headersize + decalsize + symbolsize;
	}
};

struct clientsections_t : public baseclientsections_t
{
	char	*symboldata;
	char	*entitydata;
	char	*headerdata;
	char	*decaldata;
};

int CSaveRestore::LookupRestoreSpotSaveIndex( RestoreLookupTable *table, int save )
{
	int c = table->lookup.Count();
	for ( int i = 0; i < c; i++ )
	{
		SaveRestoreTranslate *slot = &table->lookup[ i ];
		if ( slot->savedindex == save )
			return slot->restoredindex;
	}
	
	return -1;
}

void CSaveRestore::ReapplyDecal( bool adjacent, RestoreLookupTable *table, decallist_t *entry )
{
	int flags = entry->flags;
	if ( adjacent )
	{
		flags |= FDECAL_DONTSAVE;
	}

	int decalIndex = Draw_DecalIndexFromName( entry->name );
	
	if ( adjacent )
	{
		// These entities might not exist over transitions, so we'll use the saved plane and do a traceline instead
		Vector testspot = entry->position;
		VectorMA( testspot, 5.0f, entry->impactPlaneNormal, testspot );

		Vector testend = entry->position;
		VectorMA( testend, -5.0f, entry->impactPlaneNormal, testend );

		CTraceFilterHitAll traceFilter;
		trace_t tr;
		Ray_t ray;
		ray.Init( testspot, testend );
		g_pEngineTraceServer->TraceRay( ray, MASK_OPAQUE, &traceFilter, &tr );

		if ( tr.fraction != 1.0f && !tr.allsolid )
		{
			// Check impact plane normal
			float dot = entry->impactPlaneNormal.Dot( tr.plane.normal );
			if ( dot >= 0.99 )
			{
				// Hack, have to use server traceline stuff to get at an actuall index here
				edict_t *hit = tr.GetEdict();
				if ( hit != NULL )
				{
					// Looks like a good match for original splat plane, reapply the decal
					int entityToHit = NUM_FOR_EDICT( hit );
					if ( entityToHit >= 0 )
					{
						IClientEntity *clientEntity = entitylist->GetClientEntity( entityToHit );
						if ( !clientEntity )
							return;
						
						g_pEfx->DecalShoot( 
							decalIndex, 
							entityToHit, 
							clientEntity->GetModel(), 
							clientEntity->GetAbsOrigin(), 
							clientEntity->GetAbsAngles(),
							entry->position, 0, flags );
					}
				}
			}
		}
	}
	else
	{
		int entityToHit = entry->entityIndex != 0 ? LookupRestoreSpotSaveIndex( table, entry->entityIndex ) : entry->entityIndex;
		if ( entityToHit >= 0 )
		{
			IClientEntity *clientEntity = entitylist->GetClientEntity( entityToHit );
			if ( clientEntity )
			{
				g_pEfx->DecalShoot( 
					decalIndex, 
					entityToHit, 
					clientEntity->GetModel(), 
					clientEntity->GetAbsOrigin(), 
					clientEntity->GetAbsAngles(),
					entry->position, 0, flags );
			}
		}
	}
}

void CSaveRestore::RestoreClientState( char const *fileName, bool adjacent )
{
	FileHandle_t pFile;

	pFile = g_pFileSystem->Open( fileName, "rb" );
	if ( !pFile )
		return;

	SaveFileHeaderTag_t tag;
	g_pFileSystem->Read( &tag, sizeof(tag), pFile );
	if ( tag != CURRENT_SAVEFILE_HEADER_TAG )
	{
		g_pFileSystem->Close( pFile );
		return;
	}

	baseclientsections_t sections;
	g_pFileSystem->Read( &sections, sizeof(baseclientsections_t), pFile );

	CSaveRestoreData *pSaveData = MakeSaveRestoreData( SaveAllocMemory( sizeof(CSaveRestoreData) + sections.SumBytes(), sizeof(char) ) );
	// Needed?
	strcpy( pSaveData->levelInfo.szCurrentMapName, fileName );

	g_pFileSystem->Read( (char *)(pSaveData + 1), sections.SumBytes(), pFile );
	g_pFileSystem->Close( pFile );

	char *pszTokenList = (char *)(pSaveData + 1);

	if ( sections.symbolsize > 0 )
	{
		pSaveData->InitSymbolTable( (char**)SaveAllocMemory( sections.symbolcount, sizeof(char *) ), sections.symbolcount );

		// Make sure the token strings pointed to by the pToken hashtable.
		for( int i=0; i<sections.symbolcount; i++ )
		{
			if ( *pszTokenList )
			{
				Verify( pSaveData->DefineSymbol( pszTokenList, i ) );
			}
			while( *pszTokenList++ );				// Find next token (after next null)
		}
	}
	else
	{
		pSaveData->InitSymbolTable( NULL, 0 );
	}

	Assert( pszTokenList - (char *)(pSaveData + 1) == sections.symbolsize );

	//---------------------------------
	// Set up the restore basis
	int size = sections.SumBytes() - sections.symbolsize;

	pSaveData->Init( (char *)(pszTokenList), size );	// The point pszTokenList was incremented to the end of the tokens

	g_ClientDLL->ReadRestoreHeaders( pSaveData );
	
	pSaveData->Rebase();

	char name[256];
	COM_FileBase( fileName, name );
	_strlwr( name );

	RestoreLookupTable *table = FindOrAddRestoreLookupTable( name );

	pSaveData->levelInfo.fUseLandmark = adjacent;
	if ( adjacent )
	{
		pSaveData->levelInfo.vecLandmarkOffset = table->m_vecLandMarkOffset;
	}

	// Fixup restore indices based on what server re-created for us
	int c = pSaveData->NumEntities();
	for ( int i = 0 ; i < c; i++ )
	{
		entitytable_t *entry = pSaveData->GetEntityInfo( i );
		
		entry->restoreentityindex = LookupRestoreSpotSaveIndex( table, entry->saveentityindex );
	}

	g_ClientDLL->Restore( pSaveData, false );

	if ( r_decals.GetInt() )
	{
		for ( int i = 0; i < sections.decalcount; i++ )
		{
			decallist_t entry;
			g_ClientDLL->SaveReadFields( pSaveData, "DECALLIST", &entry, NULL, decallist_t::m_DataMap.dataDesc, decallist_t::m_DataMap.dataNumFields );
			ReapplyDecal( adjacent, table, &entry );
		}
	}

	Finish( pSaveData );
}

void CSaveRestore::RestoreAdjacenClientState( char const *map )
{
	char name[256];

	Q_snprintf( name, sizeof( name ), "%s%s.HL2", GetSaveDir(), map );
	COM_FixSlashes( name );
	COM_CreatePath( name );

// FIXME:  Need to get this working
	RestoreClientState( name, true );
}

// Map an entity index back to a server entity and use it to lookup an ordinal id into the save restore data lump
int TranslatedEntityIndex( CSaveRestoreData *serverData, int entityindex )
{
	int i;
	int c = serverData->NumEntities();
	entitytable_t *entry;
	for ( i = 0 ; i < c; i++ )
	{
		entry = serverData->GetEntityInfo( i );
		Assert( entry );

		if ( entry->saveentityindex == entityindex )
		{
			return entry->id;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CSaveRestore::SaveClientState( CSaveRestoreData *serverData, const char *name )
{
#ifndef SWDS
	decallist_t		*decalList;
	int				i;
	FileHandle_t	pFile;

	clientsections_t	sections;

	CSaveRestoreData *pSaveData = g_ClientDLL->SaveInit( 0 );
	
	sections.entitydata = pSaveData->AccessCurPos();

	// Now write out the client .dll entities to the save file, too
	g_ClientDLL->PreSave( pSaveData );
	g_ClientDLL->Save( pSaveData );

	sections.entitysize = pSaveData->AccessCurPos() - sections.entitydata;

	sections.headerdata = pSaveData->AccessCurPos();

	g_ClientDLL->WriteSaveHeaders( pSaveData );

	sections.headersize = pSaveData->AccessCurPos() - sections.headerdata;

	sections.decaldata = pSaveData->AccessCurPos();

	decalList = (decallist_t*)malloc( sizeof(decallist_t) * Draw_DecalMax() );
	sections.decalcount = DecalListCreate( decalList );

	for ( i = 0; i < sections.decalcount; i++ )
	{
		decallist_t *entry = &decalList[ i ];

		// Translate entity index from server to save/restore relative
		//entry->entityIndex = TranslatedEntityIndex( serverData, entry->entityIndex );

		g_ClientDLL->SaveWriteFields( pSaveData, "DECALLIST", entry, NULL, decallist_t::m_DataMap.dataDesc, decallist_t::m_DataMap.dataNumFields );
	}

	sections.decalsize = pSaveData->AccessCurPos() - sections.decaldata;

	// Write string token table
	sections.symboldata = pSaveData->AccessCurPos();

	for( i = 0; i < pSaveData->SizeSymbolTable(); i++ )
	{
		const char *pszToken = (pSaveData->StringFromSymbol( i )) ? pSaveData->StringFromSymbol( i ) : "";
		if ( !pSaveData->Write( pszToken, strlen(pszToken) + 1 ) )
		{
			Con_Printf( "Token Table Save/Restore overflow!" );
			break;
		}
	}	

	sections.symbolcount = pSaveData->SizeSymbolTable();
	sections.symbolsize = pSaveData->AccessCurPos() - sections.symboldata;

	pFile = g_pFileSystem->Open( name, "wb" );
	if ( pFile )
	{
		g_pFileSystem->Write(  &CURRENT_SAVEFILE_HEADER_TAG, sizeof(CURRENT_SAVEFILE_HEADER_TAG), pFile );

		g_pFileSystem->Write( (baseclientsections_t * )&sections, sizeof( baseclientsections_t ), pFile );

		g_pFileSystem->Write( sections.symboldata, sections.symbolsize, pFile );
		g_pFileSystem->Write( sections.headerdata, sections.headersize, pFile );
		g_pFileSystem->Write( sections.entitydata, sections.entitysize, pFile );
		g_pFileSystem->Write( sections.decaldata, sections.decalsize, pFile );

		g_pFileSystem->Close( pFile );
	}

	Finish( pSaveData );

	free( decalList );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *level - 
// Output : CSaveRestoreData
//-----------------------------------------------------------------------------
CSaveRestoreData *CSaveRestore::LoadSaveData( const char *level )
{
	char			name[MAX_OSPATH];
	FileHandle_t	pFile;

	Q_snprintf( name, sizeof( name ), "%s%s.HL1", GetSaveDir(), level);
	COM_FixSlashes( name );
	Con_Printf ("Loading game from %s...\n", name);

	pFile = g_pFileSystem->Open( name, "rb" );
	if (!pFile)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return NULL;
	}

	//---------------------------------
	// Read the header
	SaveFileHeaderTag_t tag;
	g_pFileSystem->Read( &tag, sizeof(tag), pFile );
	// Is this a valid save?
	if ( tag != CURRENT_SAVEFILE_HEADER_TAG )
		return NULL;

	//---------------------------------
	// Read the sections info and the data
	//
	SaveFileSectionsInfo_t sectionsInfo;
	
	g_pFileSystem->Read( &sectionsInfo, sizeof(sectionsInfo), pFile );

	CSaveRestoreData *pSaveData = MakeSaveRestoreData( SaveAllocMemory( sizeof(CSaveRestoreData) + sectionsInfo.SumBytes(), sizeof(char) ) );
	strcpy( pSaveData->levelInfo.szCurrentMapName, level );
	
	g_pFileSystem->Read( (char *)(pSaveData + 1), sectionsInfo.SumBytes(), pFile );
	g_pFileSystem->Close( pFile );
	
	//---------------------------------
	// Parse the symbol table
	char *pszTokenList = (char *)(pSaveData + 1);// Skip past the CSaveRestoreData structure

	if ( sectionsInfo.nBytesSymbols > 0 )
	{
		pSaveData->InitSymbolTable( (char**)SaveAllocMemory( sectionsInfo.nSymbols, sizeof(char *) ), sectionsInfo.nSymbols );

		// Make sure the token strings pointed to by the pToken hashtable.
		for( int i = 0; i<sectionsInfo.nSymbols; i++ )
		{
			if ( *pszTokenList )
			{
				Verify( pSaveData->DefineSymbol( pszTokenList, i ) );
			}
			while( *pszTokenList++ );				// Find next token (after next null)
		}
	}
	else
	{
		pSaveData->InitSymbolTable( NULL, 0 );
	}

	Assert( pszTokenList - (char *)(pSaveData + 1) == sectionsInfo.nBytesSymbols );

	//---------------------------------
	// Set up the restore basis
	int size = sectionsInfo.SumBytes() - sectionsInfo.nBytesSymbols;

	pSaveData->levelInfo.connectionCount = 0;
	pSaveData->Init( (char *)(pszTokenList), size );	// The point pszTokenList was incremented to the end of the tokens
	pSaveData->levelInfo.fUseLandmark = true;
	pSaveData->levelInfo.time = 0;
	VectorCopy( vec3_origin, pSaveData->levelInfo.vecLandmarkOffset );
	g_ServerGlobalVariables.pSaveData = (CSaveRestoreData*)pSaveData;

	return pSaveData;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSaveData - 
//			*pHeader - 
//			updateGlobals - 
//-----------------------------------------------------------------------------
void CSaveRestore::ParseSaveTables( CSaveRestoreData *pSaveData, SAVE_HEADER *pHeader, int updateGlobals )
{
	int				i;
	SAVELIGHTSTYLE	light;

	// Re-base the savedata since we re-ordered the entity/table / restore fields
	pSaveData->Rebase();
	// Process SAVE_HEADER
	serverGameDLL->SaveReadFields( pSaveData, "Save Header", pHeader, NULL, SAVE_HEADER::m_DataMap.dataDesc, SAVE_HEADER::m_DataMap.dataNumFields );
//	header.version = ENGINE_VERSION;
	pSaveData->levelInfo.connectionCount = pHeader->connectionCount;
	pSaveData->levelInfo.time = pHeader->time;
	pSaveData->levelInfo.fUseLandmark = true;
	VectorCopy( vec3_origin, pSaveData->levelInfo.vecLandmarkOffset );

	// Read adjacency list
	for ( i = 0; i < pSaveData->levelInfo.connectionCount; i++ )
		serverGameDLL->SaveReadFields( pSaveData, "ADJACENCY", pSaveData->levelInfo.levelList + i, NULL, levellist_t::m_DataMap.dataDesc, levellist_t::m_DataMap.dataNumFields );

	if ( updateGlobals )
	{
		for ( i = 0; i < MAX_LIGHTSTYLES; i++ )
			sv.lightstyles[i] = NULL;
	}
	for ( i = 0; i < pHeader->lightStyleCount; i++ )
	{
		serverGameDLL->SaveReadFields( pSaveData, "LIGHTSTYLE", &light, NULL, SAVELIGHTSTYLE::m_DataMap.dataDesc, SAVELIGHTSTYLE::m_DataMap.dataNumFields );
		if ( updateGlobals )
		{
			sv.lightstyles[light.index] = (char *)Hunk_Alloc( strlen(light.style) + 1 );
			strcpy( sv.lightstyles[light.index], light.style );
			
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Write out the list of entities that are no longer in the save file for this level
//  (they've been moved to another level)
// Input  : *pSaveData - 
//			*level - 
//-----------------------------------------------------------------------------
void CSaveRestore::EntityPatchWrite( CSaveRestoreData *pSaveData, const char *level )
{
	char			name[MAX_OSPATH];
	FileHandle_t	pFile;
	int				i, size;

	Q_snprintf( name, sizeof( name ), "%s%s.HL3", GetSaveDir(), level);
	COM_FixSlashes( name );

	pFile = g_pFileSystem->Open( name, "wb" );
	if ( pFile )
	{
		size = 0;
		for ( i = 0; i < pSaveData->NumEntities(); i++ )
		{
			if ( pSaveData->GetEntityInfo(i)->flags & FENTTABLE_REMOVED )
				size++;
		}
		// Patch count
		g_pFileSystem->Write( &size, sizeof(int), pFile );
		for ( i = 0; i < pSaveData->NumEntities(); i++ )
		{
			if ( pSaveData->GetEntityInfo(i)->flags & FENTTABLE_REMOVED )
				g_pFileSystem->Write( &i, sizeof(int), pFile );
		}
		g_pFileSystem->Close( pFile );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Read the list of entities that are no longer in the save file for this level (they've been moved to another level)
//   and correct the table
// Input  : *pSaveData - 
//			*level - 
//-----------------------------------------------------------------------------
void CSaveRestore::EntityPatchRead( CSaveRestoreData *pSaveData, const char *level )
{
	char			name[MAX_OSPATH];
	FileHandle_t	pFile;
	int				i, size, entityId;

	Q_snprintf(name, sizeof( name ), "%s%s.HL3", GetSaveDir(), level);
	COM_FixSlashes( name );

	pFile = g_pFileSystem->Open( name, "rb" );
	if ( pFile )
	{
		// Patch count
		g_pFileSystem->Read( &size, sizeof(int), pFile );
		for ( i = 0; i < size; i++ )
		{
			g_pFileSystem->Read( &entityId, sizeof(int), pFile );
			pSaveData->GetEntityInfo(entityId)->flags = FENTTABLE_REMOVED;
		}
		g_pFileSystem->Close( pFile );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *level - 
//			createPlayers - 
// Output : int
//-----------------------------------------------------------------------------
int CSaveRestore::LoadGameState( char const *level, bool createPlayers )
{
	SAVE_HEADER		header;
	CSaveRestoreData *pSaveData;
	pSaveData = LoadSaveData( level );
	if ( !pSaveData )		// Couldn't load the file
		return 0;

	serverGameDLL->ReadRestoreHeaders( pSaveData );

	ParseSaveTables( pSaveData, &header, 1 );
	EntityPatchRead( pSaveData, level );
	skill.SetValue( header.skillLevel );
	strcpy( sv.name, header.mapName );
	ConVar *skyname = ( ConVar * )cv->FindVar( "sv_skyname" );
	if ( skyname )
	{
		skyname->SetValue( header.skyName );
	}
	
	// Create entity list
	serverGameDLL->Restore( pSaveData, createPlayers );

	BuildRestoredIndexTranslationTable( level, pSaveData, false );

	Finish( pSaveData );

	sv.tickcount = (int)( header.time / TICK_RATE );
	// SUCCESS!
	return 1;
}

CSaveRestore::RestoreLookupTable *CSaveRestore::FindOrAddRestoreLookupTable( char const *mapname )
{
	int idx = m_RestoreLookup.Find( mapname );
	if ( idx == m_RestoreLookup.InvalidIndex() )
	{
		idx = m_RestoreLookup.Insert( mapname );
	}
	return &m_RestoreLookup[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSaveData - 
// Output : int
//-----------------------------------------------------------------------------
void CSaveRestore::BuildRestoredIndexTranslationTable( char const *mapname, CSaveRestoreData *pSaveData, bool verbose )
{
	char name[ 256 ];
	COM_FileBase( mapname, name );
	_strlwr( name );

	// Build Translation Lookup
	RestoreLookupTable *table = FindOrAddRestoreLookupTable( name );
	table->Clear();

	int c = pSaveData->NumEntities();
	for ( int i = 0; i < c; i++ )
	{
		entitytable_t *entry = pSaveData->GetEntityInfo( i );
		SaveRestoreTranslate slot;

		slot.classname		= entry->classname;
		slot.savedindex		= entry->saveentityindex;
		slot.restoredindex	= entry->restoreentityindex;

		table->lookup.AddToTail( slot );
	}

	table->m_vecLandMarkOffset = pSaveData->levelInfo.vecLandmarkOffset;
}

void CSaveRestore::ClearRestoredIndexTranslationTables()
{
	m_RestoreLookup.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Find all occurances of the map in the adjacency table
// Input  : *pSaveData - 
//			*pMapName - 
//			index - 
// Output : int
//-----------------------------------------------------------------------------
int EntryInTable( CSaveRestoreData *pSaveData, const char *pMapName, int index )
{
	int i;

	index++;
	for ( i = index; i < pSaveData->levelInfo.connectionCount; i++ )
	{
		if ( !strcmp( pSaveData->levelInfo.levelList[i].mapName, pMapName ) )
			return i;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSaveData - 
//			output - 
//			*pLandmarkName - 
//-----------------------------------------------------------------------------
void LandmarkOrigin( CSaveRestoreData *pSaveData, Vector& output, const char *pLandmarkName )
{
	int i;

	for ( i = 0; i < pSaveData->levelInfo.connectionCount; i++ )
	{
		if ( !strcmp( pSaveData->levelInfo.levelList[i].landmarkName, pLandmarkName ) )
		{
			VectorCopy( pSaveData->levelInfo.levelList[i].vecLandmarkOrigin, output );
			return;
		}
	}

	VectorCopy( vec3_origin, output );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOldLevel - 
//			*pLandmarkName - 
//-----------------------------------------------------------------------------
void CSaveRestore::LoadAdjacentEnts( const char *pOldLevel, const char *pLandmarkName )
{
	CSaveRestoreData currentLevelData, *pSaveData;
	int				i, test, flags, index, movedCount = 0;
	SAVE_HEADER		header;
	Vector			landmarkOrigin;

	memset( &currentLevelData, 0, sizeof(CSaveRestoreData) );
	g_ServerGlobalVariables.pSaveData = &currentLevelData;
	// Build the adjacent map list
	serverGameDLL->BuildAdjacentMapList();
	bool foundprevious = false;

	for ( i = 0; i < currentLevelData.levelInfo.connectionCount; i++ )
	{
		// make sure the previous level is in the connection list so we can
		// bring over the player.
		if ( !strcmpi( currentLevelData.levelInfo.levelList[i].mapName, pOldLevel ) )
		{
			foundprevious = true;
		}

		for ( test = 0; test < i; test++ )
		{
			// Only do maps once
			if ( !strcmp( currentLevelData.levelInfo.levelList[i].mapName, currentLevelData.levelInfo.levelList[test].mapName ) )
				break;
		}
		// Map was already in the list
		if ( test < i )
			continue;

//		Con_Printf("Merging entities from %s ( at %s )\n", currentLevelData.levelList[i].mapName, currentLevelData.levelList[i].landmarkName );
		pSaveData = LoadSaveData( currentLevelData.levelInfo.levelList[i].mapName );

		if ( pSaveData )
		{
			serverGameDLL->ReadRestoreHeaders( pSaveData );

			ParseSaveTables( pSaveData, &header, 0 );
			EntityPatchRead( pSaveData, currentLevelData.levelInfo.levelList[i].mapName );
			pSaveData->levelInfo.time = sv.gettime();// - header.time;
			pSaveData->levelInfo.fUseLandmark = true;
			flags = 0;
			LandmarkOrigin( &currentLevelData, landmarkOrigin, pLandmarkName );
			LandmarkOrigin( pSaveData, pSaveData->levelInfo.vecLandmarkOffset, pLandmarkName );
			VectorSubtract( landmarkOrigin, pSaveData->levelInfo.vecLandmarkOffset, pSaveData->levelInfo.vecLandmarkOffset );
			if ( !strcmp( currentLevelData.levelInfo.levelList[i].mapName, pOldLevel ) )
				flags |= FENTTABLE_PLAYER;

			index = -1;
			while ( 1 )
			{
				index = EntryInTable( pSaveData, sv.name, index );
				if ( index < 0 )
					break;
				flags |= 1<<index;
			}
			
			if ( flags )
				movedCount = serverGameDLL->CreateEntityTransitionList( pSaveData, flags );

			// If ents were moved, rewrite entity table to save file
			if ( movedCount )
				EntityPatchWrite( pSaveData, currentLevelData.levelInfo.levelList[i].mapName );

			BuildRestoredIndexTranslationTable( currentLevelData.levelInfo.levelList[i].mapName, pSaveData, true );

			Finish( pSaveData );
		}
	}
	g_ServerGlobalVariables.pSaveData = NULL;
	if ( !foundprevious )
	{
		Host_Error( "Level transition ERROR\nCan't find connection to %s from %s\n", pOldLevel, sv.name );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : int
//-----------------------------------------------------------------------------
int CSaveRestore::FileSize( FileHandle_t pFile )
{
	if ( !pFile )
		return 0;

	return g_pFileSystem->Size(pFile);
}

#define FILECOPYBUFSIZE 1024

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOutput - 
//			*pInput - 
//			fileSize - 
//-----------------------------------------------------------------------------
void CSaveRestore::FileCopy( FileHandle_t pOutput, FileHandle_t pInput, int fileSize )
{
	char	buf[FILECOPYBUFSIZE];		// A small buffer for the copy
	int		size;

	while ( fileSize > 0 )
	{
		if ( fileSize > FILECOPYBUFSIZE )
			size = FILECOPYBUFSIZE;
		else
			size = fileSize;
		g_pFileSystem->Read( buf, size, pInput );
		g_pFileSystem->Write( buf, size, pOutput );
		
		fileSize -= size;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPath - 
//			*pFile - 
//-----------------------------------------------------------------------------
void CSaveRestore::DirectoryCopy( const char *pPath, FileHandle_t pFile )
{
	char const		*findfn;
	char			basefindfn[ MAX_OSPATH ];
	int				fileSize;
	FileHandle_t	pCopy;
	char			szName[MAX_OSPATH];

	findfn = Sys_FindFirst(pPath, basefindfn);
	while ( findfn )
	{
		Q_snprintf( szName, sizeof( szName ), "%s%s", GetSaveDir(), findfn );
		COM_FixSlashes( szName );
		pCopy = g_pFileSystem->Open( szName, "rb" );
		fileSize = FileSize( pCopy );
		g_pFileSystem->Write( findfn, MAX_OSPATH, pFile );		// Filename can only be as long as a map name + extension
		g_pFileSystem->Write( &fileSize, sizeof(int), pFile );
		FileCopy( pFile, pCopy, fileSize );
		g_pFileSystem->Close( pCopy );

		// Any more save files?
		findfn = Sys_FindNext( basefindfn );
	}

	Sys_FindClose();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			fileCount - 
//-----------------------------------------------------------------------------
void CSaveRestore::DirectoryExtract( FileHandle_t pFile, int fileCount )
{
	int				i, fileSize;
	FileHandle_t pCopy;
	char			szName[MAX_OSPATH], fileName[MAX_OSPATH];

	for ( i = 0; i < fileCount; i++ )
	{
		g_pFileSystem->Read( fileName, MAX_OSPATH, pFile );		// Filename can only be as long as a map name + extension
		g_pFileSystem->Read( &fileSize, sizeof(int), pFile );
		Q_snprintf( szName, sizeof( szName ), "%s%s", GetSaveDir(), fileName );
		COM_FixSlashes( szName );
		pCopy = g_pFileSystem->Open( szName, "wb" );
		FileCopy( pCopy, pFile, fileSize );
		g_pFileSystem->Close( pCopy );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPath - 
// Output : int
//-----------------------------------------------------------------------------
int CSaveRestore::DirectoryCount( const char *pPath )
{
	int				count;
	char const *findfn;


	count = 0;
	findfn = Sys_FindFirst( pPath, NULL );

	while ( findfn != NULL )
	{
		count++;
		// Any more save files
		findfn = Sys_FindNext(NULL);
	}
	Sys_FindClose();

	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPath - 
//-----------------------------------------------------------------------------
void CSaveRestore::DirectoryClear( const char *pPath )
{
	char const	*findfn;
	char			szPath[ MAX_OSPATH ];

	findfn = Sys_FindFirst( pPath, NULL );

	while ( findfn != NULL )
	{
		Q_snprintf( szPath, sizeof( szPath ), "%s%s", GetSaveDir(), findfn );

		// Delete the temporary save file
		g_pFileSystem->RemoveFile( szPath, "GAME" );

		// Any more save files
		findfn = Sys_FindNext(NULL);
	}
	Sys_FindClose();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSaveRestore::ClearSaveDir( void )
{
	char			szName[MAX_OSPATH];

	Q_snprintf(szName, sizeof( szName ), "%s", GetSaveDir() );
	COM_FixSlashes( szName );
	// Create save directory if it doesn't exist
	Sys_mkdir( szName );

	strcat(szName, "*.HL?");
	DirectoryClear( szName );
}

void CSaveRestore::RequestClearSaveDir( void )
{
	m_bClearSaveDir = true;
}

void CSaveRestore::OnFinishedClientRestore()
{
	ClearRestoredIndexTranslationTables();

	if ( m_bClearSaveDir )
	{
		m_bClearSaveDir = false;
		ClearSaveDir();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Host_Savegame_f
//-----------------------------------------------------------------------------
void Host_Savegame_f (void)
{
	char comment[80];

	// Can we save at this point?
	if ( !saverestore->IsValidSave() )
		return;

	if (Cmd_Argc() != 2)
	{
		Con_DPrintf ("save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_DPrintf ("Relative pathnames are not allowed.\n");
		return;
	}

	serverGameDLL->GetSaveComment( comment, sizeof( comment ) );
	saverestore->SaveGameSlot( Cmd_Argv(1), comment );
#ifndef SWDS
	CL_HudMessage( "GAMESAVED" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Host_AutoSave_f
//-----------------------------------------------------------------------------
void Host_AutoSave_f (void)
{
	char comment[80];

	// Can we save at this point?
	if ( !saverestore->IsValidSave() )
		return;

	serverGameDLL->GetSaveComment( comment, sizeof( comment ) );
	saverestore->SaveGameSlot( "autosave", comment );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Host_Loadgame_f
//-----------------------------------------------------------------------------
void Host_Loadgame_f (void)
{
	if (cmd_source != src_command)
		return;

	if (svs.maxclients != 1)
	{
		Con_Printf ("Can't load in multiplayer games.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("load <savename> : load a game\n");
		return;
	}

	HostState_LoadGame( Cmd_Argv(1) );
#if 0
	if ( !saverestore->LoadGame( Cmd_Argv(1) ) )
	{
		Con_Printf("Error loading saved game\n" );
	}
#endif
}

CON_COMMAND_AUTOCOMPLETEFILE( load, Host_Loadgame_f, "Load a saved game.", "save", sav );

static ConCommand save("save", Host_Savegame_f);
static ConCommand autosave("autosave", Host_AutoSave_f);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSaveRestore::Init( void )
{
	ClearSaveDir();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSaveRestore::Shutdown( void )
{
}
