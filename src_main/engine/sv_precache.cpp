//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "sv_main.h"
#include "sv_precache.h"
#include "networkstringtablecontainerserver.h"
#include "host.h"
#include "vstdlib/ICommandLine.h"


void SV_PrecacheInfo_f();
void SV_PrecacheGeneric_f();
void SV_PreacacheModel_f();
void SV_PreacacheSound_f();

static ConCommand sv_precacheinfo( "sv_precacheinfo", SV_PrecacheInfo_f, "Show precache info." );
static ConCommand sv_precachegeneric( "sv_precachegeneric", SV_PrecacheGeneric_f, "Usage:  sv_precachegeneric <name> [ preload ]\nAdd file to precache list." );
static ConCommand sv_precachemodel( "sv_precachemodel", SV_PreacacheModel_f, "Usage:  sv_precachemodel <name> [ preload ]\nAdd model to precache list." );
static ConCommand sv_precachesound( "sv_precachesound", SV_PreacacheSound_f, "Usage:  sv_precachesound <name> [ preload ]\nAdd sound to precache list." );
static ConVar sv_forcepreload( "sv_forcepreload", "0", FCVAR_ARCHIVE, "Force server side preloading.");

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int SV_ModelIndex
//-----------------------------------------------------------------------------
int SV_ModelIndex (const char *name)
{
	return sv.LookupModelIndex( name );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			preload - 
// Output : int
//-----------------------------------------------------------------------------
int SV_FindOrAddModel(const char *name, bool preload )
{
	// Look for a match or an empty slot...
	int flags = RES_FATALIFMISSING;
	if ( preload )
	{
		flags |= RES_PRELOAD;
	}

	return sv.PrecacheModel( name, flags );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int SV_SoundIndex(const char *name)
{
	return sv.LookupSoundIndex( name );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			preload - 
// Output : int
//-----------------------------------------------------------------------------
int SV_FindOrAddSound(const char *name, bool preload )
{
	// Look for a match or an empty slot...
	int flags = RES_FATALIFMISSING;
	if ( preload )
	{
		flags |= RES_PRELOAD;
	}

	return sv.PrecacheSound( name, flags );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int SV_GenericIndex(const char *name)
{
	return sv.LookupGenericIndex( name );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			preload - 
// Output : int
//-----------------------------------------------------------------------------
int SV_FindOrAddGeneric(const char *name, bool preload )
{
	// Look for a match or an empty slot...
	int flags = RES_FATALIFMISSING;
	if ( preload )
	{
		flags |= RES_PRELOAD;
	}

	return sv.PrecacheGeneric( name, flags );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int SV_DecalIndex(const char *name)
{
	return sv.LookupDecalIndex( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			preload - 
// Output : int
//-----------------------------------------------------------------------------
int SV_FindOrAddDecal(const char *name, bool preload )
{
	// Look for a match or an empty slot...
	int flags = RES_FATALIFMISSING;
	if ( preload )
	{
		flags |= RES_PRELOAD;
	}

	return sv.PrecacheDecal( name, flags );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CServerState::GetModelPrecacheTable( void ) const
{
	return m_hModelPrecacheTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			flags - 
//			*model - 
// Output : int
//-----------------------------------------------------------------------------
int CServerState::PrecacheModel( char const *name, int flags, model_t *model /*=NULL*/ )
{
	int idx = networkStringTableContainerServer->AddString( m_hModelPrecacheTable, name );
	if ( idx == INVALID_STRING_INDEX )
	{
		return -1;
	}

	CPrecacheUserData p;

	// first time, set file size & flags
	CPrecacheUserData const *pExisting = (CPrecacheUserData const *)networkStringTableContainerServer->GetStringUserData( m_hModelPrecacheTable, idx );
	if ( !pExisting )
	{
		p.flags = flags;
		if ( name[ 0 ] )
		{
			p.filesize = max( modelloader->GetModelFileSize( name ), 0 ) >> 10;
		}
		else
		{
			p.filesize = 0;
		}
	}
	else
	{
		// Just or in any new flags
		p = *pExisting;
		p.flags |= flags;
	}

	networkStringTableContainerServer->SetStringUserData( m_hModelPrecacheTable, idx, sizeof( p ), &p );

	CPrecacheItem *slot = &model_precache[ idx ];

	if ( model )
	{
		slot->SetModel( model );
	}

	bool loadnow = ( !slot->GetModel() && ( flags & RES_PRELOAD ) );
	
	if ( CommandLine()->FindParm( "-nopreload" ) )
	{
		loadnow = false;
	}
	else if ( sv_forcepreload.GetInt() || CommandLine()->FindParm( "-preload" ) )
	{
		loadnow = true;
	}

	if ( idx != 0 )
	{
		if ( loadnow )
		{
			slot->SetModel( modelloader->GetModelForName( name, IModelLoader::FMODELLOADER_SERVER ) );
		}
		else
		{
			modelloader->ReferenceModel( name, IModelLoader::FMODELLOADER_SERVER );
			slot->SetModel( NULL );
		}
	}

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : model_t
//-----------------------------------------------------------------------------
model_t *CServerState::GetModel( int index )
{
	if ( index <= 0 )
		return NULL;

	if ( index >= networkStringTableContainerServer->GetNumStrings( m_hModelPrecacheTable ) )
	{
		return NULL;
	}

	CPrecacheItem *slot = &model_precache[ index ];
	model_t *m = slot->GetModel();
	if ( m )
	{
		return m;
	}

	char const *modelname = networkStringTableContainerServer->GetString( m_hModelPrecacheTable, index );
	Assert( modelname );

	if ( host_showcachemiss.GetBool() )
	{
		Con_DPrintf( "server model cache miss on %s\n", modelname );
	}

	m = modelloader->GetModelForName( modelname, IModelLoader::FMODELLOADER_SERVER );
	slot->SetModel( m );

	return m;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CServerState::LookupModelIndex( char const *name )
{
	int idx = networkStringTableContainerServer->FindStringIndex( m_hModelPrecacheTable, name );
	return ( idx == INVALID_STRING_INDEX ) ? -1 : idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CServerState::GetSoundPrecacheTable( void ) const
{
	return m_hSoundPrecacheTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			flags - 
// Output : int
//-----------------------------------------------------------------------------
int CServerState::PrecacheSound( char const *name, int flags )
{
	int idx = networkStringTableContainerServer->AddString( m_hSoundPrecacheTable, name );
	if ( idx == INVALID_STRING_INDEX )
	{
		return -1;
	}

	CPrecacheUserData p;

	// first time, set file size & flags
	CPrecacheUserData const *pExisting = (CPrecacheUserData const *)networkStringTableContainerServer->GetStringUserData( m_hSoundPrecacheTable, idx );
	if ( !pExisting )
	{
		p.flags = flags;
		if ( name[ 0 ] )
		{
			p.filesize = max( COM_FileSize( va( "sound/%s", name ) ), 0 ) >> 10;
		}
		else
		{
			p.filesize = 0;
		}
	}
	else
	{
		// Just or in any new flags
		p = *pExisting;
		p.flags |= flags;
	}

	networkStringTableContainerServer->SetStringUserData( m_hSoundPrecacheTable, idx, sizeof( p ), &p );

	CPrecacheItem *slot = &sound_precache[ idx ];
	slot->SetName( name );
	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CServerState::GetSound( int index )
{
	if ( index <= 0 )
	{
		return NULL;
	}

	if ( index >= networkStringTableContainerServer->GetNumStrings( m_hSoundPrecacheTable ) )
	{
		return NULL;
	}

	CPrecacheItem *slot = &sound_precache[ index ];
	return slot->GetName();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CServerState::LookupSoundIndex( char const *name )
{
	int idx = networkStringTableContainerServer->FindStringIndex( m_hSoundPrecacheTable, name );
	return ( idx == INVALID_STRING_INDEX ) ? 0 : idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CServerState::GetGenericPrecacheTable( void ) const
{
	return m_hGenericPrecacheTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			flags - 
// Output : int
//-----------------------------------------------------------------------------
int CServerState::PrecacheGeneric( char const *name, int flags )
{
	int idx = networkStringTableContainerServer->AddString( m_hGenericPrecacheTable, name );
	if ( idx == INVALID_STRING_INDEX )
	{
		return -1;
	}

	CPrecacheUserData p;

	// first time, set file size & flags
	CPrecacheUserData const *pExisting = (CPrecacheUserData const *)networkStringTableContainerServer->GetStringUserData( m_hGenericPrecacheTable, idx );
	if ( !pExisting )
	{
		p.flags = flags;
		if ( name[ 0 ] )
		{
			p.filesize = max( COM_FileSize( name ), 0 ) >> 10;
		}
		else
		{
			p.filesize = 0;
		}
	}
	else
	{
		// Just or in any new flags
		p = *pExisting;
		p.flags |= flags;
	}

	networkStringTableContainerServer->SetStringUserData( m_hGenericPrecacheTable, idx, sizeof( p ), &p );

	CPrecacheItem *slot = &generic_precache[ idx ];
	slot->SetGeneric( name );
	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CServerState::GetGeneric( int index )
{
	// Bogus index
	if ( index < 0 || 
		 index >= networkStringTableContainerServer->GetNumStrings( m_hGenericPrecacheTable ) )
	{
		return "";
	}

	CPrecacheItem *slot = &generic_precache[ index ];
	return slot->GetGeneric();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CServerState::LookupGenericIndex( char const *name )
{
	int idx = networkStringTableContainerServer->FindStringIndex( m_hGenericPrecacheTable, name );
	return ( idx == INVALID_STRING_INDEX ) ? 0 : idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CServerState::GetDecalPrecacheTable( void ) const
{
	return m_hDecalPrecacheTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			flags - 
// Output : int
//-----------------------------------------------------------------------------
int CServerState::PrecacheDecal( char const *name, int flags )
{
	int idx = networkStringTableContainerServer->AddString( m_hDecalPrecacheTable, name );
	if ( idx == INVALID_STRING_INDEX )
	{
		return -1;
	}

	CPrecacheUserData p;

	// first time, set file size & flags
	CPrecacheUserData const *pExisting = (CPrecacheUserData const *)networkStringTableContainerServer->GetStringUserData( m_hDecalPrecacheTable, idx );
	if ( !pExisting )
	{
		p.flags = flags;
		if ( name[ 0 ] )
		{
			p.filesize = max( COM_FileSize( name ), 0 ) >> 10;
		}
		else
		{
			p.filesize = 0;
		}
	}
	else
	{
		// Just or in any new flags
		p = *pExisting;
		p.flags |= flags;
	}

	networkStringTableContainerServer->SetStringUserData( m_hDecalPrecacheTable, idx, sizeof( p ), &p );

	CPrecacheItem *slot = &decal_precache[ idx ];
	slot->SetDecal( name );
	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CServerState::GetInstanceBaselineTable() const
{
	return m_hInstanceBaselineTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CServerState::LookupDecalIndex( char const *name )
{
	int idx = networkStringTableContainerServer->FindStringIndex( m_hDecalPrecacheTable, name );
	return ( idx == INVALID_STRING_INDEX ) ? -1 : idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerState::DumpPrecacheStats( TABLEID table )
{
	if ( table == INVALID_STRING_TABLE )
	{
		Con_Printf( "Can only dump stats when active in a level\n" );
		return;
	}

	CPrecacheItem *items = NULL;
	if ( table == m_hModelPrecacheTable )
	{
		items = model_precache;
	}
	else if ( table == m_hGenericPrecacheTable )
	{
		items = generic_precache;
	}
	else if ( table == m_hSoundPrecacheTable )
	{
		items = sound_precache;
	}
	else if ( table == m_hDecalPrecacheTable )
	{
		items = decal_precache;
	}

	if ( !items )
		return;

	int count = networkStringTableContainerServer->GetNumStrings( table );
	int maxcount = networkStringTableContainerServer->GetMaxStrings( table );

	Con_Printf( "\n" );
	Con_Printf( "Precache table %s:  %i of %i slots used\n", networkStringTableContainerServer->GetTableName( table ),
		count, maxcount );

	int used = 0, total = 0;
	for ( int i = 0; i < count; i++ )
	{
		char const *name = networkStringTableContainerServer->GetString( table, i );
		CPrecacheItem *slot = &items[ i ];
		
		int testLength;
		const CPrecacheUserData *p = ( const CPrecacheUserData * )networkStringTableContainerServer->GetStringUserData( table, i, &testLength );
		ErrorIfNot( testLength == sizeof( *p ),
			("CServerState::DumpPrecacheStats: invalid CPrecacheUserData length (%d)", testLength)
		);

		if ( !name || !slot || !p )
			continue;

		Con_Printf( "%03i:  %s (%s):   ",
			i,
			name, 
			GetFlagString( p->flags ) );

		int filesize = p->filesize << 10;

		if ( p->filesize > 0 || name[ 0 ] == '*' )
		{
			Con_Printf( "%.2f kb,", (float)filesize / 1024.0f );
		}
		else
		{
			Con_Printf( "missing!," );
		}

		total += filesize;

		if ( slot->GetReferenceCount() == 0 )
		{
			Con_Printf( " never used\n" );
		}
		else
		{
			used += filesize;

			Con_Printf( " %i refs, first %.2f mru %.2f\n",
				slot->GetReferenceCount(), 
				slot->GetFirstReference(), 
				slot->GetMostRecentReference() );
		}
	}
	if ( total > 0 )
	{
		Con_Printf( " -- Total %.2f Mb, %.2f %% used\n",
			(float)( total ) / ( 1024.0f * 1024.0f ), ( 100.0f * (float)used ) / ( float ) total );
	}
	Con_Printf( "\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SV_PrecacheInfo_f( void )
{
	if ( Cmd_Argc() == 2 )
	{
		char const *table = Cmd_Argv( 1 );

		bool dumped = true;
		if ( !Q_strcasecmp( table, "generic" ) )
		{
			sv.DumpPrecacheStats( sv.GetGenericPrecacheTable() );
		}
		else if ( !Q_strcasecmp( table, "sound" ) )
		{
			sv.DumpPrecacheStats( sv.GetSoundPrecacheTable() );
		}
		else if ( !Q_strcasecmp( table, "decal" ) )
		{
			sv.DumpPrecacheStats( sv.GetDecalPrecacheTable() );
		}
		else if ( !Q_strcasecmp( table, "model" ) )
		{
			sv.DumpPrecacheStats( sv.GetModelPrecacheTable() );
		}
		else
		{
			dumped = false;
		}

		if ( dumped )
		{
			return;
		}
	}
	
	// Show all data
	sv.DumpPrecacheStats( sv.GetGenericPrecacheTable() );
	sv.DumpPrecacheStats( sv.GetDecalPrecacheTable() );
	sv.DumpPrecacheStats( sv.GetSoundPrecacheTable() );
	sv.DumpPrecacheStats( sv.GetModelPrecacheTable() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SV_PreacacheModel_f( void )
{
	if ( sv.GetModelPrecacheTable() == INVALID_STRING_TABLE )
	{
		Con_Printf( "Cant' precache, not running a server\n" );
		return;
	}
	if ( Cmd_Argc() < 2 )
	{
		Con_Printf( "Usage:  %s filename [preload]\n", Cmd_Argv(0) );
		return;
	}

	bool preload = false;
	if ( Cmd_Argc() == 3 && !Q_strcasecmp( Cmd_Argv(2), "preload" ) )
	{
		preload = true;
	}
	char const *model = Cmd_Argv(1);
	if ( !model || !model[ 0 ] )
	{
		Con_Printf( "sv_precachemodel:  empty model!\n" );
		return;
	}

	if ( sv.LookupModelIndex( model ) != 0 )
	{
		Con_Printf( "sv_precachemodel:  %s already in list.\n", model );
		return;
	}

	// Look for a match or an empty slot...
	int flags = RES_FATALIFMISSING;
	if ( preload )
	{
		flags |= RES_PRELOAD;
	}

	sv.PrecacheModel( model, flags, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SV_PreacacheSound_f( void )
{
	if ( sv.GetSoundPrecacheTable() == INVALID_STRING_TABLE )
	{
		Con_Printf( "Cant' precache, not running a server\n" );
		return;
	}
	if ( Cmd_Argc() < 2 )
	{
		Con_Printf( "Usage:  %s filename [preload]\n", Cmd_Argv(0) );
		return;
	}

	bool preload = false;
	if ( Cmd_Argc() == 3 && !Q_strcasecmp( Cmd_Argv(2), "preload" ) )
	{
		preload = true;
	}
	char const *sound = Cmd_Argv(1);
	if ( !sound || !sound[ 0 ] )
	{
		Con_Printf( "sv_precachesound:  empty sound!\n" );
		return;
	}

	if ( sv.LookupSoundIndex( sound ) != 0 )
	{
		Con_Printf( "sv_precachesound:  %s already in list.\n", sound );
		return;
	}

	// Look for a match or an empty slot...
	int flags = RES_FATALIFMISSING;
	if ( preload )
	{
		flags |= RES_PRELOAD;
	}

	sv.PrecacheSound( sound, flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SV_PrecacheGeneric_f( void )
{
	if ( sv.GetGenericPrecacheTable() == INVALID_STRING_TABLE )
	{
		Con_Printf( "Cant' precache, not running a server\n" );
		return;
	}
	if ( Cmd_Argc() < 2 )
	{
		Con_Printf( "Usage:  %s filename [preload]\n", Cmd_Argv(0) );
		return;
	}

	bool preload = false;
	if ( Cmd_Argc() == 3 && !Q_strcasecmp( Cmd_Argv(2), "preload" ) )
	{
		preload = true;
	}
	char const *name = Cmd_Argv(1);
	if ( !name || !name[ 0 ] )
	{
		Con_Printf( "sv_precachemodel:  empty model!\n" );
		return;
	}

	if ( sv.LookupGenericIndex( name ) != 0 )
	{
		Con_Printf( "sv_precachegeneric:  %s already in list.\n", name );
		return;
	}

	// Look for a match or an empty slot...
	int flags = RES_FATALIFMISSING;
	if ( preload )
	{
		flags |= RES_PRELOAD;
	}

	sv.PrecacheGeneric( name, flags );
}


