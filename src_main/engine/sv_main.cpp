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
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "decal.h"
#include "host_cmd.h"
#include "cmodel_engine.h"
#include "sv_log.h"
#include "zone.h"
#include "sound.h"
#include "vox.h"
#include "EngineSoundInternal.h"
#include "checksum_engine.h"
#include "master.h"
#include "host.h"
#include "keys.h"
#include "vengineserver_impl.h"
#include "sv_filter.h"
#include "pr_edict.h"
#include "screen.h"
#include "sys_dll.h"
#include "world.h"
#include "sv_main.h"
#include "networkstringtableserver.h"
#include "datamap.h"
#include "filesystem_engine.h"
#include "string_t.h"
#include "vstdlib/random.h"
#include "networkstringtablecontainerserver.h"
#include "networkstringtable.h"
#include "dt_send_eng.h"
#include "sv_packedentities.h"
#include "testscriptmgr.h"
#include "PlayerState.h"
#include "saverestoretypes.h"
#include "tier0/vprof.h"
#include "proto_oob.h"
#include "staticpropmgr.h"
#include "checksum_crc.h"
#include "console.h"
#include "vstdlib/ICommandLine.h"
#include "gameeventmanager.h"
#include "enginebugreporter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Used to point at the client_frame_t's as we encode the entities for each client.
#define CLIENT_FRAME_CACHE_SIZE 256

struct ClientFrameCache
{
	client_frame_t	*m_Frames[CLIENT_FRAME_CACHE_SIZE];
	int				m_nFrames;
};

extern ConVar host_name;
extern ConVar deathmatch;

// Server default maxplayers value
#define DEFAULT_SERVER_CLIENTS	6
// This many players on a Lan with same key, is ok.
#define MAX_IDENTICAL_CDKEYS	5

PackedDataAllocator g_PackedDataAllocator;

int SV_UPDATE_BACKUP = SINGLEPLAYER_BACKUP;
int SV_UPDATE_MASK   = (SINGLEPLAYER_BACKUP-1);
qboolean allow_cheats;
static int	g_userid = 1;

CServerState	sv;
server_static_t	svs;

CGlobalVars g_ServerGlobalVariables( false );

char	localmodels[MAX_MODELS][5];			// inline model names for precache

static int	current_skill;

ConVar	sv_contact( "sv_contact", "", FCVAR_SERVER, "Contact email for server sysop" );
ConVar	sv_maxupdaterate( "sv_maxupdaterate", "60", 0, "Maximum updates per second that the server will allow" );
ConVar	sv_minupdaterate( "sv_minupdaterate", "10", 0, "Minimum updates per second that the server will allow" );
ConVar	sv_failuretime( "sv_failuretime", "0.5", 0, "After this long without a packet from client, don't send any more until client starts sending again" );
ConVar	sv_minrate( "sv_minrate", "0", FCVAR_SERVER, "Min bandwidth rate allowed on server, 0 == unlimited" );
ConVar	sv_maxrate( "sv_maxrate", "0", FCVAR_SERVER, "Max bandwidth rate allowed on server, 0 == unlimited" );
ConVar  sv_logrelay( "sv_logrelay", "0", 0, "Allow log messages from remote machines to be logged on this server" );
ConVar	sv_language( "sv_language","0");
ConVar	violence_hblood( "violence_hblood","1", 0, "Draw human blood" );
ConVar	violence_ablood( "violence_ablood","1", 0, "Draw alien blood" );
ConVar	violence_hgibs( "violence_hgibs","1", 0, "Show human gib entities" );
ConVar	violence_agibs( "violence_agibs","1", 0, "Show alien gib entities" );
ConVar  sv_clienttrace( "sv_clienttrace","1", FCVAR_SERVER, "0 = big box(Quake), 0.5 = halfsize, 1 = normal (100%), otherwise it's a scaling factor" );
ConVar  sv_timeout( "sv_timeout", "65", FCVAR_SERVER, "After this many seconds without a message from a client, the client is dropped" );
ConVar  sv_cheats( "sv_cheats", "1", FCVAR_SERVER, "Allow cheats such as impulse 101" );
ConVar	sv_password( "sv_password", "", FCVAR_SERVER | FCVAR_PROTECTED, "Server password for entry into multiplayer games" );
ConVar  sv_lan( "sv_lan", "0", 0, "Server is a lan server ( no heartbeat, no authentication, no non-class C addresses, 9999.0 rate, etc." );
ConVar	sv_stressbots("sv_stressbots", "0", FCVAR_SERVER, "If set to 1, the server calculates data and fills packets to bots. Used for perf testing.");
ConVar	sv_CacheEncodedEnts("sv_CacheEncodedEnts", "1", FCVAR_SERVER, "If set to 1, does an optimization to prevent extra SendTable_Encode calls.");
ConVar  sv_VoiceCodec("sv_VoiceCodec", "vaudio_miles", 0, "Specifies which voice codec DLL to use in a game. Set to the name of the DLL without the extension.");
ConVar  sv_deltatrace( "sv_deltatrace", "0", 0, "For debugging, print entity creation/deletion info to console." );
ConVar  sv_packettrace( "sv_packettrace", "1", 0, "For debugging, print entity creation/deletion info to console." );


// Prints important entity creation/deletion events to console
#if defined( _DEBUG )
#define TRACE_DELTA( text ) if ( sv_deltatrace.GetInt() ) { Con_Printf( text ); };
#else
#define TRACE_DELTA( funcs )
#endif


#if defined( DEBUG_NETWORKING )

//-----------------------------------------------------------------------------
// Opens the recording file
//-----------------------------------------------------------------------------

static FILE* OpenRecordingFile()
{
	FILE* fp = 0;
	static bool s_CantOpenFile = false;
	static bool s_NeverOpened = true;
	if (!s_CantOpenFile)
	{
		fp = fopen( "svtrace.txt", s_NeverOpened ? "wt" : "at" );
		if (!fp)
		{
			s_CantOpenFile = true;
		}
		s_NeverOpened = false;
	}
	return fp;
}

//-----------------------------------------------------------------------------
// Records an argument for a command, flushes when the command is done
//-----------------------------------------------------------------------------

static void SpewToFile( char const* pFmt, ... )
{
	static CUtlVector<unsigned char> s_RecordingBuffer;

	char temp[2048];
	va_list args;

	va_start( args, pFmt );
	int len = Q_vsnprintf( temp, sizeof( temp ), pFmt, args );
	va_end( args );
	Assert( len < 2048 );

	int idx = s_RecordingBuffer.AddMultipleToTail( len );
	memcpy( &s_RecordingBuffer[idx], temp, len );
	if ( 1 ) //s_RecordingBuffer.Size() > 8192)
	{
		FILE* fp = OpenRecordingFile();
		fwrite( s_RecordingBuffer.Base(), 1, s_RecordingBuffer.Size(), fp );
		fclose( fp );

		s_RecordingBuffer.RemoveAll();
	}
}

#endif // #if defined( DEBUG_NETWORKING )

bool ServerHasPassword()
{
	const char * pw = sv_password.GetString();
	return pw[0] && stricmp(pw, "none");
}


// ---------------------------------------------------------------------------------------- //
// client_frame_t implementation.
// ---------------------------------------------------------------------------------------- //

client_frame_t::client_frame_t()
{
	m_pSnapshot = NULL;
}

client_frame_t::~client_frame_t()
{
	SetSnapshot( NULL );	// Release our reference to the snapshot.
}

void client_frame_t::SetSnapshot( CFrameSnapshot *pSnapshot )
{
	if ( m_pSnapshot )
		m_pSnapshot->ReleaseReference();

	if( pSnapshot )
		pSnapshot->AddReference();

	m_pSnapshot = pSnapshot;
}	


void CServerState::Clear( void )
{
	if ( networkStringTableContainerServer )
	{
		networkStringTableContainerServer->RemoveAllTables();
	}

	state = ss_dead;

	active = false;
	paused = false;
	loadgame = false;
	
	tickcount = 0;
	
	memset( name, 0, sizeof( name ) );
	memset( startspot, 0, sizeof( startspot ) );
	memset( modelname, 0, sizeof( modelname ) );

	host_state.worldmodel = NULL;	
	host_state.worldmapCRC = 0;  
	
	clientSideDllCRC = 0; 

	memset( lightstyles, 0, sizeof( lightstyles ) );

	num_edicts = 0;
	max_edicts = 0;
	edicts = NULL;
	
	baselines.entities = 0;
	baselines.max_entities = 0;
	baselines.num_entities = 0;

	datagram.Reset();
	memset( datagram_buf, 0, sizeof( datagram_buf ) );

	reliable_datagram.Reset();
	memset( reliable_datagram_buf, 0, sizeof( reliable_datagram_buf ) );

	multicast.Reset();
	memset( multicast_buf, 0, sizeof( multicast_buf ) );

	signon.Reset();
	memset( signon_buffer, 0, sizeof( signon_buffer ) );

	signon_fullsendtables.Reset();
	memset( signon_fullsendtables_buffer, 0, sizeof( signon_fullsendtables_buffer ) );

	serverclasses = 0;
	serverclassbits = 0;
	sound_sequence = 0;

	pending_logos.RemoveAll();

	m_hModelPrecacheTable = INVALID_STRING_TABLE;
	m_hGenericPrecacheTable = INVALID_STRING_TABLE;
	m_hSoundPrecacheTable = INVALID_STRING_TABLE;
	m_hDecalPrecacheTable = INVALID_STRING_TABLE;
	m_hInstanceBaselineTable = INVALID_STRING_TABLE;

	// Clear the instance baseline indices in the ServerClasses.
	if ( serverGameDLL )
	{
		for( ServerClass *pCur = serverGameDLL->GetAllServerClasses(); pCur; pCur=pCur->m_pNext )
		{
			pCur->m_InstanceBaselineIndex = INVALID_STRING_INDEX;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create any client/server string tables needed internally by the engine
//-----------------------------------------------------------------------------
void CServerState::CreateEngineStringTables( void )
{
	m_hModelPrecacheTable = networkStringTableContainerServer->CreateStringTable( 
		MODEL_PRECACHE_TABLENAME, 
		MAX_MODELS );

	m_hGenericPrecacheTable = networkStringTableContainerServer->CreateStringTable(
		GENERIC_PRECACHE_TABLENAME,
		MAX_GENERIC );

	m_hSoundPrecacheTable = networkStringTableContainerServer->CreateStringTable(
		SOUND_PRECACHE_TABLENAME,
		MAX_SOUNDS );

	m_hDecalPrecacheTable = networkStringTableContainerServer->CreateStringTable(
		DECAL_PRECACHE_TABLENAME,
		MAX_BASE_DECALS );

	m_hInstanceBaselineTable = networkStringTableContainerServer->CreateStringTable(
		INSTANCE_BASELINE_TABLENAME,
		MAX_DATATABLES );
}


//-----------------------------------------------------------------------------
// Purpose: Prepare for level transition, etc.
//-----------------------------------------------------------------------------
void SV_InactivateClients( void )
{
	int i;
	client_t *cl;

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		// Fake clients get killed in here.
		if ( cl->fakeclient )
		{
			SV_ClearClientStructure(cl);
			continue;
		}
		else if ( !cl->active && !cl->connected && !cl->spawned )
		{
			continue;
		}

		cl->netchan.message.Reset();

		cl->connected = true;
		cl->active = false;
		cl->spawned = false;
	}
}


int SV_GetClientIndex(client_t *cl)
{
	return cl - svs.clients;
}

/*
================
SV_DeleteClientFrames

================
*/
void SV_DeleteClientFrames( client_t *cl )
{
	int i;
	client_frame_t *pframe;

	if ( !cl->frames )
		return;

	for ( i = 0, pframe = cl->frames; i < cl->numframes; i++, pframe++ )
	{
		SV_ClearPacketEntities( &pframe->entities );
	}

	delete [] cl->frames;
	cl->frames = NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  : *cl -
//-----------------------------------------------------------------------------
void SV_ClearClientStructure(client_t *cl)
{
	if (!cl)
	{
		Sys_Error("SV_ClearClientStructure with NULL client!");
	}

	SV_DeleteClientFrames( cl );

	memset(cl, 0, sizeof(*cl));

	// Clear all entity_created bits for this client.
	int iBitMask = ~(1 << (cl - svs.clients));
	for ( int iEdict=0; iEdict < sv.max_edicts; iEdict++ )
	{
		sv.edicts[iEdict].entity_created &= iBitMask;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Maps over all client slots and called the passed in function
// Input  : Function to invoke
//-----------------------------------------------------------------------------
void SV_MapOverClients( void (*fn)(client_t *) )
{
	for( int i=0; i < svs.maxclientslimit; i++ )
	{
		fn ( &svs.clients[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *cl -
//-----------------------------------------------------------------------------
void SV_AllocClientFrames( client_t *cl )
{
	Assert( !cl->frames );

	client_frame_t *frameset;
	frameset = (client_frame_t *)new client_frame_t[ SV_UPDATE_BACKUP ];
	Assert( frameset );
	memset( frameset, 0, SV_UPDATE_BACKUP * sizeof( client_frame_t ) );
	cl->frames = frameset;
	cl->numframes = SV_UPDATE_BACKUP;

	// Indicate that the snapshots aren't currently being used....
	// NOTE: This is how all snapshots from the previous level are unloaded;
	// we're basically dereferencing all snapshots here...
	for (int i = 0; i < SV_UPDATE_BACKUP; ++i )
	{
		frameset[i].SetSnapshot( NULL );
	}
}

/*
================
SV_ClearPacketEntities( client_frame_t *frame )

================
*/
void SV_ClearPacketEntities( PackedEntities *pack )
{
	if(pack)
	{
		// Must clear out all dynamic data for each frame
		delete [] pack->entities;
		pack->entities		= NULL;
		pack->num_entities	= 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *frame -
//			numents -
//-----------------------------------------------------------------------------
void SV_AllocPacketEntities( client_frame_t *frame , int numents )
{
	PackedEntities *pack;

	if ( !frame )
		return;

	pack = &frame->entities;

	Assert( !pack->entities );

	pack->entities = new PackedEntity[ max( numents, 1 ) ];
	if (!pack->entities)
	{
		Sys_Error("Failed to allocate space for %i packet entities\n", numents );
	}

	pack->max_entities = pack->num_entities = numents;
}


//// Usercommands
/*
====================
SV_User_f

user <name or userid>

Dump userdata / masterdata for a user
====================
*/
void SV_User_f (void)
{
	int		uid;
	int		i;
	client_t *cl;

	if ( !sv.active )
	{
		Con_Printf( "Can't 'user', not running a server\n" );
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: user <username / userid>\n");
		return;
	}

	uid = atoi(Cmd_Argv(1));

	for (i=0, cl= svs.clients ; i<svs.maxclients ; i++, cl++)
	{
		if ( !cl->active && !cl->spawned && !cl->connected )
			continue;

		if ( !cl->name[0] )
			continue;

		if ( ( cl->userid == uid ) ||
			!strcmp( cl->name, Cmd_Argv(1)) )
		{
			Info_Print ( cl->userinfo);
			return;
		}
	}

	Con_Printf ("User not in server.\n");
}

/*
====================
SV_Users_f

Dump userids for all current players
====================
*/
void SV_Users_f (void)
{
	int		i;
	int		c;
	client_t *cl;

	if ( !sv.active )
	{
		Con_Printf( "Can't 'users', not running a server\n" );
		return;
	}

	c = 0;
	Con_Printf ("userid name\n");
	Con_Printf ("------ ----\n");
	for (i=0, cl= svs.clients ; i<svs.maxclients ; i++, cl++)
	{
		if ( !cl->active && !cl->spawned && !cl->connected )
			continue;

		if ( cl->name[0])
		{
			Con_Printf ("%6i %s\n", cl->userid, cl->name);
			c++;
		}
	}

	Con_Printf ( "%i users\n", c );
}

//-----------------------------------------------------------------------------
// Purpose: Determine the value of svs.maxclients
//-----------------------------------------------------------------------------
void SV_SetMaxClients( void )
{
	// Determine absolute limit
	svs.maxclientslimit = MAX_CLIENTS;

	// Assume default
	svs.maxclients = isDedicated ? DEFAULT_SERVER_CLIENTS : 1;

	// Check for command line override
	int nMaxPlayers = CommandLine()->ParmValue( "-maxplayers", -1 );
	if ( nMaxPlayers >= 0 )
	{
		svs.maxclients = nMaxPlayers;
	}

	svs.maxclients = max( 1, svs.maxclients );
	svs.maxclients = min( MAX_CLIENTS, svs.maxclients );

	// sanity check
	svs.maxclients = min( svs.maxclients, svs.maxclientslimit );
}

//-----------------------------------------------------------------------------
// Purpose: Changes the maximum # of players allowed on the server.
//  Server cannot be running when command is issued.
//-----------------------------------------------------------------------------
void SV_MaxPlayers_f (void)
{
	if ( Cmd_Argc () != 2 )
	{
		Con_Printf ("\"maxplayers\" is \"%u\"\n", svs.maxclients);
		return;
	}

	if ( sv.active )
	{
		Con_Printf( "Cannot change maxplayers while the server is running\n");
		return;
	}

	svs.maxclients = max( Q_atoi( Cmd_Argv( 1 ) ), 1 );
	svs.maxclients = min( svs.maxclients, svs.maxclientslimit );

	Con_Printf( "maxplayers set to %i\n", svs.maxclients );

	deathmatch.SetValue( ( svs.maxclients == 1 ) ? 0 : 1 );
}


int SV_BuildSendTablesArray( ServerClass *pClasses, SendTable **pTables, int nMaxTables )
{
	int nTables = 0;

	for( ServerClass *pCur=pClasses; pCur; pCur=pCur->m_pNext )
	{
		ErrorIfNot( nTables < nMaxTables, ("SV_BuildSendTablesArray: too many SendTables!") );
		pTables[nTables] = pCur->m_pTable;
		++nTables;
	}
	
	return nTables;
}


// Builds an alternate copy of the datatable for any classes that have datatables with props excluded.
void SV_InitSendTables( ServerClass *pClasses )
{
	SendTable *pTables[MAX_DATATABLES];
	int nTables = SV_BuildSendTablesArray( pClasses, pTables, ARRAYSIZE( pTables ) );

	SendTable_Init( pTables, nTables );

	svs.sendtable_crc = SendTable_ComputeCRC();
}


void SV_TermSendTables( ServerClass *pClasses )
{
	SendTable_Term();
}


//-----------------------------------------------------------------------------
// Purpose: Loads the game .dll
//-----------------------------------------------------------------------------
void SV_InitGameDLL( void )
{
	// Clear out the command buffer.
	Cbuf_Execute();

	// Don't initialize a second time
	if ( svs.dll_initialized )
	{
		return;
	}

	// Load in the game .dll
	LoadEntityDLLs( host_parms.basedir );

	if ( !serverGameDLL )
	{
		Warning( "Unabled to load server.dll\n" );
		return;
	}

	// Flag that we've started the game .dll
	svs.dll_initialized = true;

	// Tell the game DLL to start up
	if(!serverGameDLL->DLLInit(g_AppSystemFactory, physicsFactory, g_AppSystemFactory, &g_ServerGlobalVariables))
	{
		Host_Error("IDLLFunctions::GameDLLInit returned false.\n");
	}

	// Make extra copies of data tables if they have SendPropExcludes.
	SV_InitSendTables( serverGameDLL->GetAllServerClasses() );

	// Execute and server commands the game .dll added at startup
	Cbuf_Execute();
}

//
// Release resources associated with extension DLLs.
//
void SV_ShutdownGameDLL( void )
{
	if ( !svs.dll_initialized )
	{
		return;
	}

	// Delete any extra SendTable copies we've attached to the game DLL's classes, if any.
	SV_TermSendTables( serverGameDLL->GetAllServerClasses() );
	serverGameDLL->DLLShutdown();

	UnloadEntityDLLs();

	svs.dll_initialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: General initialization of the server
//-----------------------------------------------------------------------------
void SV_Init (void)
{
	int i;
	for ( i=0 ; i < MAX_MODELS ; i++ )
	{
		Q_snprintf( localmodels[i], sizeof( localmodels[i] ), "*%i", i );
	}

	SV_SetMaxClients();

	// This is the only place we ever allocate client slots, and they are on the hunk so they
	// are never freed
	svs.clients = ( client_t * )Hunk_AllocName( svs.maxclientslimit * sizeof( client_t ), "svs.clients" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SV_Shutdown( void )
{
	// TODO clear stuff out here
	SV_MapOverClients( SV_DeleteClientFrames );

	// Actually performs a shutdown.
	framesnapshot->LevelChanged();

	g_pGameEventManager->FireEvent( new KeyValues( "server_shutdown", "reason", "quit" ), NULL );

	// Log_Printf( "Server shutdown.\n" );
	g_Log.Close();
}

/*
==================
void SV_CountPlayers

Counts number of connections.  Clients includes regular connections
==================
*/
void SV_CountPlayers( int *clients )
{
	int i;
	client_t *cl;

	*clients	= 0;

	for (i=0, cl = svs.clients ; i < svs.maxclients ; i++, cl++ )
	{
		if ( cl->active || cl->spawned || cl->connected )
		{
			(*clients)++;
		}
	}
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/
bool SV_BuildSoundMsg( edict_t *pEntity, int iChannel, const char *pSample, int volume,
    soundlevel_t iSoundLevel, int iFlags, int iPitch, const Vector &vecOrigin, float soundtime, bf_write *pBuffer )
{

    int         sound_num;
    int			field_mask;
	int			ent;
	unsigned short us;
	unsigned char ub;

	if (volume < 0 || volume > 255)
		Sys_Error ("SV_StartSound: volume = %i", volume);

	if (iSoundLevel < 0 || iSoundLevel > 255)
		Sys_Error ("SV_StartSound: iSoundLevel = %d", iSoundLevel);

	if (iChannel < 0 || iChannel > 7)
		Sys_Error ("SV_StartSound: channel = %i", iChannel);

	if (iPitch < 0 || iPitch > 255)
		Sys_Error ("SV_StartSound: pitch = %i", iPitch);
	
	int iDelay = 0;
	if ( soundtime != 0.0f )
	{
		iDelay = clamp( (int)( ( soundtime - sv.gettime() ) * 1000.0f ), -10 * MAX_SOUND_DELAY_MSEC, MAX_SOUND_DELAY_MSEC );
	}

	// find precache number for sound

	// if this is a sentence, get sentence number
	if ( pSample && TestSoundChar(pSample, CHAR_SENTENCE) )
	{
		iFlags |= SND_SENTENCE;
		sound_num = atoi(PSkipSoundChars(pSample));
		if (sound_num >= VOX_SentenceCount() )
		{
			Con_Printf("invalid sentence number: %s", PSkipSoundChars(pSample));
			return false;
		}
	}
	else
	{
		sound_num = sv.LookupSoundIndex( pSample );
		if ( !sound_num || !sv.GetSound( sound_num ) )
		{
			Con_Printf ("SV_StartSound: %s not precached (%d)\n", pSample, sound_num);
			return false;
		}
    }

	ent = NUM_FOR_EDICT(pEntity);

	field_mask = iFlags;

	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (iSoundLevel != SNDLVL_NORM) 
		field_mask |= SND_SOUNDLEVEL;
	if (iPitch != DEFAULT_SOUND_PACKET_PITCH)
		field_mask |= SND_PITCH;
	if (iDelay != 0 )
		field_mask |= SND_DELAY;

	// directed messages go only to the entity the are targeted on
	pBuffer->WriteByte (svc_sound);

	// Keep sound_sequence as one byte
	sv.sound_sequence = ( sv.sound_sequence + 1 ) & 0xff;
	pBuffer->WriteUBitLong( sv.sound_sequence, 8 );

	us = ( unsigned short ) field_mask;
	pBuffer->WriteUBitLong ( us, SND_FLAG_BITS_ENCODE );

	if (field_mask & SND_VOLUME)
	{
		ub = ( unsigned char ) volume;
		pBuffer->WriteUBitLong ( ub, 8 );
	}

	if (field_mask & SND_SOUNDLEVEL)
	{
		ub = ( unsigned char ) iSoundLevel;	// keep in dB range
		pBuffer->WriteUBitLong ( ub, 8 );
	}

	ub = ( unsigned char ) iChannel;
	pBuffer->WriteUBitLong ( ub, 3 );

	us = ( unsigned short ) ent;
	pBuffer->WriteUBitLong( us, MAX_EDICT_BITS );

	us = ( unsigned short ) sound_num;
	pBuffer->WriteUBitLong( us, MAX_SOUND_INDEX_BITS );	// make sure sound doesn't exceed

	pBuffer->WriteBitCoord( (int)vecOrigin.x );
	pBuffer->WriteBitCoord( (int)vecOrigin.y );
	pBuffer->WriteBitCoord( (int)vecOrigin.z );

	if (field_mask & SND_PITCH)
	{
		ub = ( unsigned char ) iPitch;
		pBuffer->WriteUBitLong ( ub, 8 );
	}

	if (field_mask & SND_DELAY)
	{
		// Skipahead works in 10 msec increments
		if ( iDelay < 0 )
		{
			iDelay *= 0.1;
		}

		pBuffer->WriteBitLong( iDelay, MAX_SOUND_DELAY_MSEC_ENCODE_BITS, true );
	}

	return true;
}

/*
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

Pitch should be PITCH_NORM (100) for no pitch shift. Values over 100 (up to 255)
shift pitch higher, values lower than 100 lower the pitch.
==================
*/
void SV_StartSound ( IRecipientFilter& filter, edict_t *pSoundEmittingEntity, int iChannel, 
	const char *pSample, float flVolume, soundlevel_t iSoundLevel, int iFlags, 
	int iPitch, const Vector *pOrigin, float soundtime /*= 0.0f*/ )
{
	// Compute the sound origin
	Vector	vecSoundOrigin;
	if (pOrigin)
	{
		VectorCopy( *pOrigin, vecSoundOrigin );
	}
	else if (pSoundEmittingEntity)
	{
		IServerEntity *serverEntity = pSoundEmittingEntity->GetIServerEntity();
		if ( serverEntity )
		{
			CM_WorldSpaceCenter( serverEntity->GetCollideable(), &vecSoundOrigin );
		}
		else
		{
			vecSoundOrigin.Init();
		}
	}
	else
	{
		vecSoundOrigin.Init();
	}

	sv.multicast.Reset();

	int iVolume = 255 * flVolume;
	if (!SV_BuildSoundMsg( pSoundEmittingEntity, iChannel, pSample, iVolume, 
					iSoundLevel, iFlags, iPitch, vecSoundOrigin, soundtime, &sv.multicast ))
	{
		return;
	}

	bool reliable = filter.IsReliable();
	int c = filter.GetRecipientCount();
	int i;
	for ( i = 0; i < c; i++ )
	{
		int index = filter.GetRecipientIndex( i );

		if ( index < 1 || index > svs.maxclients )
			continue;

		client_t *cl = &svs.clients[ index - 1 ];
		if ( !cl->active )
			continue;

		bf_write *out = reliable ? &cl->netchan.message : &cl->datagram;
		if ( !out )
			continue;

		// Is there room in the buffer?
		if ( out->GetNumBitsLeft() <= sv.multicast.GetNumBitsWritten() )
			continue;

		out->WriteBits(sv.multicast.m_pData, sv.multicast.GetNumBitsWritten());
	}

	sv.multicast.Reset();
}

bool IsSinglePlayerGame( void )
{
#ifndef SWDS
	if (!sv.active)
		return cl.maxclients == 1;
#endif
	return svs.maxclients == 1;
}

//-----------------------------------------------------------------------------
// Purpose: Sets bits of playerbits based on valid multicast recipients
// Input  : usepas - 
//			origin - 
//			playerbits - 
//-----------------------------------------------------------------------------
void SV_DetermineMulticastRecipients( bool usepas, const Vector& origin, unsigned int& playerbits )
{
	// determine cluster for origin
	int cluster = CM_LeafCluster( CM_PointLeafnum( origin ) );
	unsigned char *pMask = usepas ? CM_ClusterPAS( cluster ) : CM_ClusterPVS( cluster );

	playerbits = 0;

	// Check for relevent clients
	client_t *pClient;
	int	j;
	for (j = 0, pClient = svs.clients; j < svs.maxclients; ++j, ++pClient)
	{
		if ( !pClient->active )
			continue;

		// HACK:  Should above also check pClient->spawned instead of this
		if ( !pClient->edict || pClient->edict->free || pClient->edict->m_pEnt == NULL )
			continue;

		Vector vecEarPosition;
		serverGameClients->ClientEarPosition( pClient->edict, &vecEarPosition );

		int iBitNumber = CM_LeafCluster( CM_PointLeafnum( vecEarPosition ) );
		if ( !(pMask[iBitNumber>>3] & (1<<(iBitNumber&7)) ) )
			continue;

		playerbits |= ( 1 << j );
	}
}

static bool AppearsNumeric( char const *in )
{
	char const *p = in;
	int special[ 3 ];
	Q_memset( special, 0, sizeof( special ) );

	for ( ; *p; p++ )
	{
		if ( *p == '-' )
		{
			special[0]++;
			continue;
		}

		if ( *p == '+' )
		{
			special[1]++;
			continue;
		}

		if ( *p >= '0' && *p <= '9' )
		{
			continue;
		}

		if ( *p == '.' )
		{
			special[2]++;
			continue;
		}

		return false;
	}

	// Can't have multiple +, -, or decimals
	for ( int i = 0; i < 3; i++ )
	{
		if ( special[ i ] > 1 )
			return false;
	}

	// can't be + and - at same time
	if ( special[ 0 ] && special[ 1 ] )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: If the value is numeric, remove unnecessary trailing zeros
// Input  : *invalue - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CleanupConVarStringValue( char const *invalue )
{
	static char clean[ 256 ];

	Q_snprintf( clean, sizeof( clean ), "%s", invalue );

	// Don't mess with empty string
	// Otherwise, if it appears numeric and has a decimal, try to strip all zeroes after decimal
	if ( Q_strlen( clean ) >= 1 && AppearsNumeric( clean ) && Q_strstr( clean, "." ) )
	{
		char *end = clean + strlen( clean ) - 1;
		while ( *end && end >= clean )
		{
			// Removing trailing zeros
			if ( *end != '0' )
			{
				// Remove decimal, zoo
				if ( *end == '.' )
				{
					if ( end == clean )
					{
						*end = '0';
					}
					else
					{
						*end = 0;
					}
				}
				break;
			}

			*end-- = 0;
		}
	}

	return clean;
}

//-----------------------------------------------------------------------------
// Purpose: Write single ConVar change to all connected clients
// Input  : *var - 
//			*newValue - 
//-----------------------------------------------------------------------------
void SV_ReplicateConVarChange( ConVar const *var, char const *newValue )
{
	Assert( var );
	Assert( var->IsBitSet( FCVAR_REPLICATED ) );
	Assert( newValue );

	char		data[ 256 ];
	int         i;
	client_t    *cl;

	bf_write msg( "SV_ReplicateConVarChange", data, sizeof( data ) );

	if (!sv.active)
		return;

	msg.WriteByte( svc_setconvar );
	// Don't reset all FCVAR_REPLICATED values to "default"
	msg.WriteByte( 0 );
	// Count
	msg.WriteByte( 1 );
	msg.WriteString( va( "%s", var->GetName() ) );
	msg.WriteString( va( "%s", CleanupConVarStringValue( newValue ) ) );

	Assert( !msg.IsOverflowed() );

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if ((!cl->active && !cl->connected && !cl->spawned) || cl->fakeclient)
			continue;

		// Never send to local client, since the ConVar is directly linked there
		if ( cl->netchan.remote_address.type == NA_LOOPBACK )
			continue;

		cl->netchan.message.WriteBytes( msg.GetData(), msg.GetNumBytesWritten() );
	}
}

static int SV_CountNonDefaultVariablesWithFlags( int flags )
{
	int i = 0;
	const ConCommandBase *var;
	
	for (var= ConCommandBase::GetCommands() ; var ; var=var->GetNext())
	{
		if ( var->IsCommand() )
			continue;

		const ConVar *cvar = ( const ConVar * )var;

		if ( !cvar->IsBitSet( flags ) )
			continue;

		// It's == to the default value, don't count
		if ( !Q_strcasecmp( cvar->GetDefault(), cvar->GetString() ) )
			continue;

		i++;
	}

	return i;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg - 
//-----------------------------------------------------------------------------
void SV_WriteReplicatedConVarsToBuffer( bf_write& msg )
{
	int count = SV_CountNonDefaultVariablesWithFlags( FCVAR_REPLICATED );
	// Nothing to send
	if ( count <= 0 )
		return;

	// Too many to send, error out and have mod author get a clue.
	if ( count > 255 )
	{
		Sys_Error( "Engine only supports 255 ConVars marked FCVAR_REPLICATED\n" );
		return;
	}

	// Write header
	msg.WriteByte( svc_setconvar );
	// Tell client to reset all vars to default since we're only sending changes
	msg.WriteByte( 1 );
	// Note how many we're sending
	msg.WriteByte( count );

	int i = 0;
	const ConCommandBase *var;
	
	for (var= ConCommandBase::GetCommands() ; var ; var=var->GetNext())
	{
		if ( var->IsCommand() )
			continue;

		const ConVar *cvar = ( const ConVar * )var;

		if ( !cvar->IsBitSet( FCVAR_REPLICATED ) )
			continue;

		// It's == to the default value, don't count
		if ( !Q_strcasecmp( cvar->GetDefault(), cvar->GetString() ) )
			continue;

		msg.WriteString( va( "%s", cvar->GetName() ) );
		msg.WriteString( va( "%s", CleanupConVarStringValue( cvar->GetString() ) ) );
		i++;
	}

	// Make sure this count matches original one!!!
	Assert( i == count );
}

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo (client_t *client)
{
	char			message[2048];
	int				playernum;

	byte buffer[ NET_MAX_PAYLOAD ];
	bf_write msg( "SV_SendServerinfo->msg", buffer, sizeof( buffer ) );

	// Only send this message to developer console, or multiplayer clients.
	if ( ( developer.GetInt() ) || (svs.maxclients > 1 ) )
	{
		char basemap[ 512 ];
		COM_FileBase( sv.modelname, basemap );
		int curplayers = 0;
		SV_CountPlayers( &curplayers );

		msg.WriteByte (svc_print);
		Q_snprintf (message, sizeof( message ), "\n%s\nMap: %s\nPlayers:  %i / %i\nBuild %d\nServer Number %i\n",
			serverGameDLL->GetGameDescription(),
			basemap,
			curplayers, svs.maxclients,
			build_number(),
			svs.spawncount );
		msg.WriteString (message);
	}

	msg.WriteByte( svc_serverinfo );
	msg.WriteShort( PROTOCOL_VERSION );
	msg.WriteLong( svs.spawncount );
	msg.WriteLong( host_state.worldmapCRC );       // To prevent cheating with hacked maps
	msg.WriteLong( sv.clientSideDllCRC );  // To prevent cheating with hacked client dlls
	msg.WriteByte( svs.maxclients );

	playernum = NUM_FOR_EDICT( client->edict ) - 1;
	msg.WriteByte (playernum);

	// Write the gamedir so the client can load the correct client.dll
	COM_FileBase( com_gamedir, message );
	msg.WriteString(message );

	// Write the map name so client can crc check it.
	msg.WriteString ( sv.modelname );

	ConVar const *skyname = cv->FindVar( "sv_skyname" );
	msg.WriteString( skyname ? skyname->GetString() : "" );
	
	// send game event system CRC to enforce using the same description file
	msg.WriteLong( g_pGameEventManager->GetEventTableCRC()	);

	// send music
	msg.WriteByte (svc_cdtrack);
	msg.WriteByte (g_ServerGlobalVariables.cdAudioTrack);

	// set view
	msg.WriteByte (svc_setview);
	msg.WriteShort (NUM_FOR_EDICT(client->edict));

	SV_WriteNetworkStringTablesToBuffer( &msg );
	client->tabledef_acknowledged_tickcount = host_tickcount;

	// Write replicated ConVars to non-listen server clients only
	if ( client->netchan.remote_address.type != NA_LOOPBACK )
	{
		SV_WriteReplicatedConVarsToBuffer( msg );
	}

	client->spawned = false;
	client->connected = true;

	Netchan_CreateFragments( true, &client->netchan, &msg );
}

// Returns the client_t for a player's edict.
// Returns NULL if the edict is not a player's edict.
client_t* SV_EdictToClient(edict_t *ed)
{
	int index = ed - sv.edicts - 1;
	if(index < 0 || index >= svs.maxclients)
		return NULL;
	else
		return &svs.clients[index];
}

//-----------------------------------------------------------------------------
// Purpose: Disconnects the client and cleans out the m_pEnt CBasePlayer container object
// Input  : *clientedict - 
//-----------------------------------------------------------------------------
void SV_DisconnectClient( edict_t *clientedict )
{
	serverGameClients->ClientDisconnect( clientedict );
	// release the DLL entity that's attached to this edict, if any
	serverGameEnts->FreeContainingEntity( clientedict);
}

/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_New_f (void)
{
	edict_t *ent;
	char szRejectReason[128];
	char szAddress[ 256 ];
	char szName[ 64 ];

	// not valid on client
	if ( cmd_source == src_command )
		return;

	if (host_client->spawned && !host_client->active)
		return;

	Assert( serverGameDLL );

	ent = host_client->edict;

	host_client->connected = true;
	host_client->connection_started = realtime;

	host_client->netchan.message.Reset();
	host_client->datagram.Reset();

	// Netchan_Clear( &host_client->netchan );

	SV_SendServerinfo (host_client);

	// sent userinfo to be sent
	host_client->sendinfo = true;

	// If the client was connected, tell the game .dll to disconnect him/her.
	if ( ( host_client->active || host_client->spawned ) && host_client->edict )
	{
		SV_DisconnectClient( host_client->edict );
	}

	Q_snprintf( szName, sizeof( szName ), host_client->name );
	Q_snprintf( szAddress, sizeof( szAddress ), NET_AdrToString( host_client->netchan.remote_address ) );
	Q_snprintf( szRejectReason, sizeof( szRejectReason ), "Connection rejected by game\n" );

	// Allow the game dll to reject this client.
	if ( !serverGameClients->ClientConnect( ent, szName, szAddress, szRejectReason ) )
	{
		// Reject the connection and drop the client.
		host_client->netchan.message.WriteByte (svc_stufftext);
		host_client->netchan.message.WriteString (va("echo %s\n", szRejectReason ) );
		SV_DropClient ( host_client, false, "game .dll rejected connection by %s", szName );
	}
}

/*
================
SV_RejectConnection

Rejects connection request and sends back a message
================
*/
void SV_RejectConnection(netadr_t *adr, char *fmt, ... )
{
	va_list		argptr;
	char	text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf ( text, sizeof( text ), fmt, argptr);
	va_end (argptr);

	SZ_Clear(&net_message);
	MSG_WriteLong   ( &net_message, 0xFFFFFFFF ); // -1 -1 -1 -1 signal
	MSG_WriteByte   ( &net_message, S2C_CONNREJECT );
	MSG_WriteString ( &net_message, text );
	NET_SendPacket( NS_SERVER, net_message.cursize, net_message.data, *adr );
	SZ_Clear(&net_message);
}

/*
================
SV_RejectConnectionForPassword(char *)

Rejects connection request for having a bad password and sends back a message
================
*/
void SV_RejectConnectionForPassword(netadr_t *adr)
{
	SZ_Clear(&net_message);
	MSG_WriteLong   (&net_message,     0xFFFFFFFF ); // -1 -1 -1 -1 signal
	MSG_WriteByte   (&net_message,     S2C_BADPASSWORD );
	MSG_WriteString (&net_message,     "BADPASSWORD" );
	NET_SendPacket( NS_SERVER, net_message.cursize, net_message.data, *adr );
	SZ_Clear(&net_message);
}

/*
================
SV_CheckProtocol

Make sure connecting client is using proper protocol
================
*/
int SV_CheckProtocol( netadr_t *adr, int nProtocol )
{
	if ( !adr )
	{
		Sys_Error( "SV_CheckProtocol:  Null address\n" );
		return 0;
	}

	if ( nProtocol != PROTOCOL_VERSION )
	{
		// Client is newer than server
		if ( nProtocol > PROTOCOL_VERSION )
		{
			SV_RejectConnection( adr, "This server is using an older protocol ( %i ) than your client ( %i ).  If you believe this server is outdated, you can contact the server administrator at %s\n",
				PROTOCOL_VERSION, nProtocol, Q_strlen( sv_contact.GetString() ) > 0 ? sv_contact.GetString() : "(no email address specified)" );
		}
		else
		// Server is newer than client
		{
			SV_RejectConnection( adr, "This server is using a newer protocol ( %i ) than your client ( %i ).  You should check for updates to your client\n",
				PROTOCOL_VERSION, nProtocol );
		}
		return 0;
	}

	// Success
	return 1;
}

// MAX_CHALLENGES is made large to prevent a denial
//  of service attack that could cycle all of them
//  out before legitimate users connected
#define	MAX_CHALLENGES	1024
typedef struct
{
	netadr_t    adr;       // Address where challenge value was sent to.
	int			challenge; // To connect, adr IP address must respond with this #
	int			time;      // # is valid for only a short duration.
} challenge_t;

challenge_t	g_rg_sv_challenges[MAX_CHALLENGES];	// to prevent spoofed IPs from connecting

/*
================
SV_CheckChallenge

Make sure connecting client is not spoofing
================
*/
int SV_CheckChallenge( netadr_t *adr, int nChallengeValue )
{
	int i;

	if ( !adr )
	{
		Sys_Error( "SV_CheckChallenge:  Null address\n" );
		return 0;
	}

	// See if the challenge is valid
	// Don't care if it is a local address.
	if ( NET_IsLocalAddress (*adr) )
		return 1;

	for (i=0 ; i<MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (net_from, g_rg_sv_challenges[i].adr) )
		{
			// FIXME:  Compare time gap and don't allow too long.
			if (nChallengeValue == g_rg_sv_challenges[i].challenge)
				break;		// good
			SV_RejectConnection( adr, "Bad challenge.\n");
			return 0;
		}
	}
	if ( i == MAX_CHALLENGES )
	{
		SV_RejectConnection( adr, "No challenge for your address.\n");
		return 0;
	}

	return 1;
}

/*
================
SV_CheckIPRestrictions

Determine if client is outside appropriate address range
================
*/
int SV_CheckIPRestrictions( netadr_t *adr )
{
	if ( !adr )
	{
		Sys_Error( "SV_CheckIPRestrictions:  Null address\n" );
		return 0;
	}

#if 0 // disabled during development
	if ( ( gfUseLANAuthentication || sv_lan.GetInt() ) && ( adr->type == NA_IP ) )
	{
		if ( !NET_CompareClassBAdr( *adr, net_local_adr ) && !NET_IsReservedAdr( *adr ) )
		{
			SV_RejectConnection( adr, "LAN servers are restricted to local clients (class C).\n" );
			return 0;
		}
	}
#endif

	return 1;
}

/*
================
SV_FinishCertificateCheck

For LAN connections, make sure we don't have too many people with same cd key hash
For Authenticated net connections, check the certificate and also double check won userid
 from that certificate
================
*/
int SV_FinishCertificateCheck( netadr_t *adr, int nAuthProtocol, char *szRawCertificate )
{
	int i;
	client_t *cl;
	int nHashCount = 0;

	// Now check auth information
	switch ( nAuthProtocol )
	{
	default:
	case PROTOCOL_AUTHCERTIFICATE:
		SV_RejectConnection( adr, "Authentication disabled!!!\n");
		return 0;
	case PROTOCOL_HASHEDCDKEY:
		if ( !gfUseLANAuthentication )
		{
			// User tried to sneak in with lan authentication, even though we are requring full
			// Now way
			//
			SV_RejectConnection( adr, "Invalid authentication message.\n");
			return 0;
		}

		if ( Q_strlen( szRawCertificate ) != 32 )
		{
			SV_RejectConnection( adr, "Invalid CD Key.\n" );
			return 0;
		}

		// Now make sure that this hash isn't "overused"
		for ( i=0,cl=svs.clients ; i<svs.maxclients; i++,cl++ )
		{
			if (!cl->active && !cl->spawned && !cl->connected)
				continue;

			if ( Q_strnicmp ( szRawCertificate, cl->hashedcdkey, 32 ) )
				continue;

			nHashCount++;
		}

		if ( nHashCount >= MAX_IDENTICAL_CDKEYS )
		{
			SV_RejectConnection( adr, "CD Key already in use.\n");
			return 0;
		}
		break;
	}

	return 1;
}

/*
================
SV_CheckKeyInfo

Determine if client is outside appropriate address range
================
*/
int SV_CheckKeyInfo( netadr_t *adr, char *protinfo, unsigned short *port, int *pAuthProtocol, char *pszRaw, CRC32_t& sendtable_crc )
{
	sendtable_crc = (CRC32_t)0;

	const char *s;
	int nAuthProtocol = -1;

	// Check protocol ID
	s = Info_ValueForKey( protinfo, "prot" );
	nAuthProtocol = Q_atoi( s );
	if ( ( nAuthProtocol <= 0 ) || ( nAuthProtocol > PROTOCOL_LASTVALID ) )
	{
		SV_RejectConnection( adr, "Invalid connection.\n");
		return 0;
	}

	s = Info_ValueForKey( protinfo, "raw" );
	if ( Q_strlen( s ) <= 0 ||
		 ( ( nAuthProtocol == PROTOCOL_HASHEDCDKEY ) && ( Q_strlen(s) != 32 ) ) )
	{
		SV_RejectConnection( adr, "Invalid authentication certificate length.\n" );
		return 0;
	}

	Q_strcpy( pszRaw, s );

	*port = (unsigned short)Q_atoi( PORT_CLIENT );

	s = Info_ValueForKey( protinfo, "tablecrc" );
	if ( Q_strlen( s ) > 0 )
	{
		 COM_HexConvert( s, 8, (byte *)&sendtable_crc );
	}

	/*
	s = Info_ValueForKey( protinfo, "port" );
	if ( Q_strlen( s ) > 0 )
	{
		*port = (unsigned short)Q_atoi( s );
	}
	*/
	// Store success
	*pAuthProtocol = nAuthProtocol;

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: See if the connecting client specified a logo file
// Input  : *protinfo - 
//-----------------------------------------------------------------------------
void SV_CheckLogoFile( char *protinfo )
{
	host_client->request_logo = false;

	const char *s = Info_ValueForKey( protinfo, "logo" );
	if ( !s || !s[0] )
	{
		// Player doesn't have a logo
		host_client->logo = (CRC32_t)0;
		return;
	}
	else
	{
		COM_HexConvert( s, 8, (byte *)&host_client->logo );
	}

	char crcfilename[ 512 ];
	Q_snprintf( crcfilename, sizeof( crcfilename ), "materials/decals/downloads/%s.vtf",
		COM_BinPrintf( (byte *)&host_client->logo, 4 ) );

	// Make sure other clients get told about the logo for this player
	Info_SetValueForStarKey( host_client->userinfo, "*logo", COM_BinPrintf( (byte *)&host_client->logo, 4 ), sizeof( host_client->userinfo ) );

	// See if the logo is resident in the server's cache
	if ( g_pFileSystem->FileExists( crcfilename ) )
	{
		return;
	}

	// Request logo after SV_New_f
	host_client->request_logo = true;
}

/*
================
SV_CheckUserInfo

Get user name
================
*/
int SV_CheckUserInfo( netadr_t *adr, char *userinfo, char *name )
{
	char *s;

	// Check connection password (don't verify local client)
	if ( !NET_IsLocalAddress ( *adr ) && ServerHasPassword() &&
		 strcmp(sv_password.GetString(), Info_ValueForKey( userinfo, "password" ) ) )  // And the user didn't match it
	{
		// failed
		Con_Printf ( "%s:  password failed\n", NET_AdrToString (*adr));

		// Special rejection handler.
		SV_RejectConnectionForPassword( adr );
		return 0;
	}

	Info_RemoveKey (userinfo, "password"); // remove password so other users don't see it

	Q_memset( name, 0, 32 );

	s = (char *)Info_ValueForKey( userinfo, "name" );;  // Username
	if (s)
	{
		Q_strncpy( name, s , 32 );
		name[31] = '\0';
	}
	else
	{
		Q_strcpy ( name, "unnamed" );
	}

	return 1;
}

/*
================
SV_FindEmptySlot

Get slot # and set client_t pointer for player, if possible
We don't do this search on a "reconnect, we just reuse the slot
================
*/
int SV_FindEmptySlot( netadr_t *adr, int *pslot, client_t **ppClient )
{
	client_t *client=0;
	int slot;
	int			clients;

	clients = 0;

	SV_CountPlayers( &clients );

	for ( slot = 0 ; slot < svs.maxclients ; slot++ )
	{
		client = &svs.clients[slot];
		if ( !client->active && !client->spawned && !client->connected )
			break;
	}

	if ( slot >= svs.maxclients )
	{
		SV_RejectConnection( adr, "Server is full.\n" );
		return 0;
	}

	*pslot = slot;
	*ppClient = client;
	// Success
	return 1;
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient ()
{
	edict_t			*ent;
	client_t		*client = NULL;
	netadr_t		adr;
	int				edictnum;
	int				i;
	char			userinfo[1024];
	char			protinfo[1024];
	char			*s;
	char			name[32];
	char			szRawCertificate[512];
	int				nAuthProtocol = -1;
	unsigned short	port;
	qboolean		reconnect = false;
	CRC32_t			sendtable_crc = (CRC32_t)0;

	adr = net_from;

	// Default port to use
	port = (unsigned short)Q_atoi( PORT_CLIENT );

	if (Cmd_Argc() < 5)
	{
		SV_RejectConnection( &adr, "Insufficient connection info\n");
		return;
	}

	// Make sure protocols match up
	if ( !SV_CheckProtocol( &adr, Q_atoi( Cmd_Argv( 1 ) ) ) )
	{
		return;
	}

	if ( !SV_CheckChallenge( &adr, Q_atoi( Cmd_Argv( 2 ) ) ) )
	{
		return;
	}


#if 1 // REENABLE FOR RELEASE
	// LAN servers restrict to class b IP addresses
	if ( !SV_CheckIPRestrictions( &adr ) )
	{
		return;
	}
#endif

	// Client connection has passed challenge response.
	// if there is allready a slot for this ip, drop it
	for (i=0,client=svs.clients ; i<svs.maxclients; i++,client++)
	{
		if ( !client->active && !client->spawned && !client->connected )
			continue;

		if ( NET_CompareAdr ( adr, client->netchan.remote_address ) )
		{
			Con_Printf ( "%s:reconnect\n", NET_AdrToString (adr) );
			reconnect = true;
			break;
		}
	}

	Q_memset ( szRawCertificate, 0, 512 );

	s = Cmd_Argv(3);
	Q_strncpy( protinfo, s, sizeof( protinfo ) - 1 );

	// HASHED CD KEY VALIDATION
	if ( !SV_CheckKeyInfo( &adr, protinfo, &port, &nAuthProtocol, szRawCertificate, sendtable_crc ) )
	{
		return;
	}

	/// CLIENT USERINFO
	s = Cmd_Argv(4);
	Q_strncpy( userinfo, s, sizeof(userinfo)-1 );

	if ( !SV_CheckUserInfo( &adr, userinfo, name ) )
	{
		return;
	}

	// A reconnecting client will re-use the slot found above when checking for reconnection.  The
	//  slot will be wiped clean.
	if ( !reconnect )
	{
		//   Connect the client if there are empty slots.
		if ( !SV_FindEmptySlot( &adr, &i, &client ) )
		{
			return;
		}
	}

	if ( !SV_FinishCertificateCheck( &adr, nAuthProtocol, szRawCertificate ) )
	{
		return;
	}

	host_client = client;

	// If the client was connected, tell the game .dll to disconnect him/her.
	if ( ( host_client->active || host_client->spawned ) && host_client->edict )
	{
		SV_DisconnectClient( host_client->edict );
	}

	SV_ClearClientStructure( client );

	// Fixup pointers
	Assert( !host_client->frames );

	SV_AllocClientFrames( host_client );

	// Store off the potential client number
	edictnum    = i+1;
	ent         = EDICT_NUM(edictnum);
	host_client->edict = ent;
	client->m_bUsedLocalNetworkBackdoor = false;

////////////////////////////////////////////////
// Client can connect
//
	// Set up the network channel.
	Netchan_Setup (NS_SERVER, &client->netchan, adr );

	// Will get reset from userinfo, but this value comes from sv_updaterate ( the default )
	host_client->next_messageinterval = 0.05;
	host_client->next_messagetime = realtime + host_client->next_messageinterval;
	// Force a full delta update on first packet.
	host_client->delta_sequence = -1;

	// Tell client connection worked.
	Netchan_OutOfBandPrint (NS_SERVER, adr, "%c0000000000000000", S2C_CONNECTION );

	// Display debug message.
	if ( host_client->netchan.remote_address.type != NA_LOOPBACK  )
	{
		Con_DPrintf ("Client %s connected\nAdr: %s\n", name, NET_AdrToString(host_client->netchan.remote_address ) );
	}

	// Set up client structure.
	if ( nAuthProtocol == PROTOCOL_HASHEDCDKEY )
	{
		Q_strncpy ( host_client->hashedcdkey, szRawCertificate, 32 );
		host_client->hashedcdkey[32] = '\0';
	}

	client->active = false;
	client->spawned = false;
	client->connected = true;

	client->edict = ent;

	client->userid = g_userid++;

	// Log_Printf( "\"%s<%i>\" connected, address %s\n", name, client->userid, NET_AdrToString( client->netchan.remote_address ) );

	KeyValues * event = new KeyValues( "player_connect" );
	event->SetInt( "userid", client->userid );
	event->SetString( "name", name );
	event->SetString( "address", NET_AdrToString( client->netchan.remote_address ) );
	g_pGameEventManager->FireEvent( event, NULL );
	

	Q_strncpy( client->userinfo, userinfo, MAX_INFO_STRING );
	// Set up or request any spray logos as needed
	SV_CheckLogoFile( protinfo );

	// Set up the datagram buffer for this client.
	client->datagram.StartWriting(client->datagram_buf, sizeof(client->datagram_buf));
	client->m_ForceWaitForAck = -1;
	client->m_bResendNoDelta = false;
	client->sendtable_crc = sendtable_crc;
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping (void)
{
	char	data[6];

	data[0] = -1;
	data[1] = -1;
	data[2] = -1;
	data[3] = -1;
	data[4] = A2A_ACK;
	data[5] = 0;

	NET_SendPacket (NS_SERVER, 6, data, net_from);
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
This message can be up to around 5k with worst case string lengths.
================
*/
void SVC_Status (void)
{
	// UNDONE:  Do we want a special protocol here.
	// TODO:  add this from QW source
}

/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge (void)
{
	int		i;
	int		oldest;
	int		oldestTime;
	char    data[128];
	int n;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (net_from, g_rg_sv_challenges[i].adr))
			break;
		if (g_rg_sv_challenges[i].time < oldestTime)
		{
			oldestTime = g_rg_sv_challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		g_rg_sv_challenges[oldest].challenge = (RandomInt(0,0xFFFF) << 16) | RandomInt(0,0xFFFF);
		g_rg_sv_challenges[oldest].adr = net_from;
		g_rg_sv_challenges[oldest].time = realtime;
		i = oldest;
	}

	// Also, tell the client what kind of connection protocol to use.
	if ( sv.active && ( svs.maxclients > 1 ) )
	{
		if ( !sv_lan.GetInt() )
		{
			COM_CheckAuthenticationType();
		}
		else
		{
			gfUseLANAuthentication = true;
		}
	}
	else
	{
		gfUseLANAuthentication = true;  // Use CD Key Hash
	}

	if ( gfUseLANAuthentication )  // We have a valid certificate.
	{
		n = PROTOCOL_HASHEDCDKEY;
	}
	else
	{
		Assert( 0 );
		// n = PROTOCOL_AUTHCERTIFICATE;
		n = PROTOCOL_HASHEDCDKEY;
		gfUseLANAuthentication = true;
	}

	Q_snprintf(data, sizeof( data ), "%c%c%c%c%c00000000 %u %i\n",
		255, 255, 255, 255,
		S2C_CHALLENGE,
		(int)g_rg_sv_challenges[i].challenge,
		n);

	// send it back
	NET_SendPacket ( NS_SERVER, Q_strlen( data ) + 1, data, net_from );
}

void SV_ResetModInfo( void )
{
	memset( &gmodinfo, 0, sizeof( modinfo_t ) );
	gmodinfo.version = 1;
	gmodinfo.svonly  = true;
}

qboolean SV_GetModInfo( char *pszInfo, char *pszDL, int *version, int *size, qboolean *svonly, qboolean *cldll, char *pszHLVersion )
{
	// Read in the liblist .gam file for this mod?	No, do this at the time we load the entity dll and share the structure.
	//
	if ( gmodinfo.bIsMod )
	{
		strcpy( pszInfo, gmodinfo.szInfo );
		strcpy( pszDL,   gmodinfo.szDL );
		strcpy ( pszHLVersion, gmodinfo.szHLVersion );
		*version = gmodinfo.version;
		*size    = gmodinfo.size;
		*svonly  = gmodinfo.svonly;
		*cldll   = gmodinfo.cldll;
	}
	else
	{
		strcpy( pszInfo, "" );
		strcpy( pszDL,   "" );
		strcpy( pszHLVersion, VERSION_STRING );
		*version = 1;
		*size    = 0;
		*svonly  = true;
		*cldll   = false;
	}
	return gmodinfo.bIsMod;
}

/*
================
SVC_Info

Responds with short or long info for broadcast scans
================
*/
void SVC_Info ( qboolean bDetailed )
{
	int		i, count;

	byte	data[1400];
	char szModURL_Info[512];
	char szModURL_DL[512];
	int mod_version;
	int mod_size;
	int isMod;
	char gd[MAX_OSPATH];
	int cldll, svonly;
	char szHLVersion[32];

	bf_write buf( "SVC_Info->buf", data, sizeof(data) );

	if (!sv.active)            // Must be running a server.
		return;

	if (svs.maxclients <= 1)   // ignore in single player
		return;

	count = 0;
	for (i=0 ; i<svs.maxclients ; i++)
		if (svs.clients[i].active)
			count++;

	buf.WriteLong( 0xffffffff);
	if ( bDetailed )
		buf.WriteByte( S2A_INFO_DETAILED );
	else
		buf.WriteByte( S2A_INFO);

	if ( !noip )  // Always try to send the IP address
		buf.WriteString( NET_AdrToString(net_local_adr));

	else // LOOP BACK
		buf.WriteString( "LOOPBACK" );

//!!!BETA mwhmwh
//	sprintf(gd, "BETA:%s", host_name.GetString());
	Q_snprintf(gd, sizeof( gd ), "BETA:  Valve Test" );
	buf.WriteString( gd);
//!!! end beta

	buf.WriteString( sv.name);

	COM_FileBase( com_gamedir, gd );

	buf.WriteString( gd );
	buf.WriteString( (char *)serverGameDLL->GetGameDescription());

	buf.WriteByte( count);
	buf.WriteByte( svs.maxclients);
	buf.WriteByte( PROTOCOL_VERSION);

	if ( bDetailed )
	{
		// Additional info....
		if ( cls.state == ca_dedicated )
			buf.WriteByte( 'd' );
		else
			buf.WriteByte( 'l' );

	#if defined(_WIN32)
		buf.WriteByte( 'w' );
	#else // LINUX?
		buf.WriteByte( 'l' );
	#endif

		// Password?
		if ( ServerHasPassword() )
		{
			buf.WriteByte( 1 );
		}
		else
		{
			buf.WriteByte( 0 );
		}
		// Determine if this is a modified game and, if so, send that info, too.
		isMod = SV_GetModInfo( szModURL_Info, szModURL_DL, &mod_version, &mod_size, &svonly, &cldll, szHLVersion );
		if ( isMod )
		{
			buf.WriteByte  ( 1 ); // Game requires having mod stuff.
			buf.WriteString( szModURL_Info );
			buf.WriteString( szModURL_DL );
			buf.WriteString( szHLVersion );
			buf.WriteLong  ( mod_version );
			buf.WriteLong  ( mod_size );
			buf.WriteByte  ( svonly );
			buf.WriteByte  ( cldll );
		}
		else
		{
			buf.WriteByte  ( 0 ); // Not a modified game
		}
	}

	NET_SendPacket (NS_SERVER, buf.GetNumBytesWritten(), buf.GetData(), net_from);
}

#define MAX_SINFO 2048
void SVC_InfoString ( void )
{
	int		i;
	int		count; // player count
	int		proxy;

	byte	data[1400];

	char	address[ 256 ];
	char	gd[MAX_OSPATH];
	char	info[ MAX_SINFO ];
	char	szOS[2];

	bf_write buf( "SVC_InfoString->buf", data, sizeof( data ) );

	if ( noip )
		return;

	if (!sv.active)            // Must be running a server.
		return;

	if (svs.maxclients <= 1)   // ignore in single player
		return;

	count = 0;
	proxy = 0;
	for (i=0 ; i<svs.maxclients ; i++)
	{
		if (svs.clients[i].active || svs.clients[i].spawned || svs.clients[i].connected )
		{
			if ( 0 /*svs.clients[i].proxy*/ )
			{
				proxy++;
			}
			else
			{
				count++;
			}
		}
	}

	address[0] = 0;

	if ( !noip )
	{
		 // Always try to send the IP address
		Q_strcpy( address, NET_AdrToString( net_local_adr ) );
	}

#if _WIN32
	strcpy( szOS, "w" );
#else
	strcpy( szOS, "l" );
#endif

	info[0]='\0';

	Info_SetValueForKey( info, "protocol", va( "%i", PROTOCOL_VERSION ), MAX_SINFO );
	Info_SetValueForKey( info, "address", va( "%i", PROTOCOL_VERSION ), MAX_SINFO );
	// Actual players
	Info_SetValueForKey( info, "players", va( "%i", count ), MAX_SINFO );
	// Proxy slots occupied
	if ( proxy >= 0 )
	{
		Info_SetValueForKey( info, "proxytarget", va( "%i", proxy ), MAX_SINFO );
	}
//	Info_SetValueForKey( info, "lan", va( "%i", Master_IsLanGame() ? 1 : 0 ), MAX_SINFO );
	Info_SetValueForKey( info, "lan", va( "%i", 0 ), MAX_SINFO );
	Info_SetValueForKey( info, "max", va( "%i", svs.maxclients ), MAX_SINFO );

	COM_FileBase( com_gamedir, gd );

	Info_SetValueForKey( info, "gamedir", gd, MAX_SINFO );
	Info_SetValueForKey( info, "description", (char *)serverGameDLL->GetGameDescription(), MAX_SINFO );

	Info_SetValueForKey( info, "hostname", host_name.GetString(), MAX_SINFO );
	Info_SetValueForKey( info, "map", sv.name, MAX_SINFO );

	Info_SetValueForKey( info, "dedicated", va( "%i", cls.state == ca_dedicated ? 1 : 0 ) , MAX_SINFO );
	Info_SetValueForKey( info, "password", va( "%i", ServerHasPassword()?1:0 ) , MAX_SINFO );
	Info_SetValueForKey( info, "os", szOS , MAX_SINFO );

	if ( gmodinfo.bIsMod )
	{
		Info_SetValueForKey( info, "mod", va( "%i", 1 ), MAX_SINFO );
		Info_SetValueForKey( info, "modversion", va( "%i", gmodinfo.version ), MAX_SINFO );
	}

	buf.WriteLong( 0xffffffff );
	buf.WriteString( "infostringresponse" );
	buf.WriteString( info );

	NET_SendPacket (NS_SERVER, buf.GetNumBytesWritten(), buf.GetData(), net_from);
}

int player_datacounts[32];  // # of bytes of player data for this slot
typedef struct
{
	int entdata;
	int playerdata;
} entcount_t;
entcount_t ent_datacounts[32];

/*
=================
SVC_PlayerInfo

Returns info about requested player.
=================
*/
#define PINFO_NORMAL     (1<<0)
#define PINFO_FAKECLIENT (1<<1)

void SVC_PlayerInfo (void)
{
	int		i, count;
	client_t	*client;
	//unsigned char cState;

	byte	data[2048];

	bf_write buf( "SVC_PlayerInfo", data, sizeof(data) );

	if (!sv.active)            // Must be running a server.
		return;

	if (svs.maxclients <= 1)   // ignore in single player
		return;

	// Find Player
	buf.WriteLong( -1);
	buf.WriteByte( S2A_PLAYER);  // All players coming now.

	count = 0;
	for (i=0, client = svs.clients; i<svs.maxclients ; i++, client++)
	{
		if (!svs.clients[i].active)
			continue;

		count++;
	}

	buf.WriteByte( count);
	count = 0;
	for (i=0, client = svs.clients; i<svs.maxclients ; i++, client++)
	{
		if (!svs.clients[i].active)
			continue;

		count++;

		buf.WriteByte( count);
		buf.WriteString( client->name);
		buf.WriteLong( (int)0);  // need to ask game dll for frags
		buf.WriteFloat( realtime - client->netchan.connect_time);
	}

	NET_SendPacket (NS_SERVER, buf.GetNumBytesWritten(), buf.GetData(), net_from);
}

/*
=================
SVC_RuleInfo

More detailed server information
=================
*/
void SVC_RuleInfo (void)
{
	int nNumRules;
	const ConCommandBase *var;

	byte data[2048];
	bf_write buf( "SVC_RuleInfo->buf", data, sizeof(data) );

	if (!sv.active)            // Must be running a server.
		return;

	if (svs.maxclients <= 1)   // ignore in single player
		return;

	nNumRules = cv->CountVariablesWithFlags( FCVAR_SERVER );
	if (nNumRules <= 0)        // No server rules active.
		return;

	// Find Player
	buf.WriteLong( -1);
	buf.WriteByte( S2A_RULES);  // All players coming now.
	buf.WriteShort( nNumRules);

	// Need to respond with game directory, game name, and any server variables that have been set that
	//  effect rules.  Also, probably need a hook into the .dll to respond with additional rule information.
	for ( var = ConCommandBase::GetCommands() ; var ; var=var->GetNext() )
	{
		if ( var->IsCommand() )
			continue;

		if (!(var->IsBitSet( FCVAR_SERVER ) ) )
			continue;

		buf.WriteString( var->GetName() );   // Cvar Name

		// For protected cvars, don't send the string
		if ( var->IsBitSet( FCVAR_PROTECTED ) )
		{
			// If it has a value string and the string is not "none"
			if ( ( strlen( ((ConVar*)var)->GetString() ) > 0 ) &&
				 stricmp( ((ConVar*)var)->GetString(), "none" ) )
			{
				buf.WriteString( "1" );
			}
			else
			{
				buf.WriteString( "0" );
			}
		}
		else
		{
			buf.WriteString( ((ConVar*)var)->GetString() ); // Value
		}
	}

	NET_SendPacket (NS_SERVER, buf.GetNumBytesWritten(), buf.GetData(), net_from);
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket (void)
{
	char    *args;
	char	*s;
	char	*c;

	MSG_BeginReading ();
	MSG_ReadLong ();		// skip the -1 marker

	s = MSG_ReadStringLine ();

	args = s;

	Cmd_TokenizeString (s);

	c = Cmd_Argv(0);

	if (!strcmp(c, "ping") ||
		( c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')) )
	{
		SVC_Ping ();
		return;
	}
	else if ( c[0] == A2C_PRINT )
	{
		MSG_BeginReading ();
		MSG_ReadLong ();        // skip the -1
		MSG_ReadByte (); // Skip a2c_print
		s = MSG_ReadString(); // grab the string
		Con_DPrintf( s );
	}
	else if (c[0] == A2A_ACK && (c[1] == 0 || c[1] == '\n') )
	{
		Con_Printf ("A2A_ACK from %s\n", NET_AdrToString (net_from));
		return;
	}
	else if (c[0] == M2A_CHALLENGE && (c[1] == 0 || c[1] == '\n') )
	{
		master->RespondToHeartbeatChallenge ();
		return;
	}
	else if ( !stricmp( c, "log" ) )
	{
		if ( sv_logrelay.GetFloat() && args && ( strlen( args ) > 4 ) )
		{
			s = args + strlen( "log " );
			if ( s && s[0] )
				Con_Printf( "%s\n", s );  // add back in the \n that read string line stripped off.
		}
		return;
	}
	else if (!strcmp(c,"status"))
	{
		SVC_Status ();
		return;
	}
	else if (!strcmp(c,"getchallenge"))
	{
		SVC_GetChallenge();
		return;
	}
	else if (!stricmp(c, "info"))
	{
		SVC_Info( false );
		return;
	}
	else if (!stricmp(c, "details"))
	{
		SVC_Info( true );
		return;
	}
	else if (!stricmp(c, "infostring"))
	{
		SVC_InfoString();
		return;
	}
	else if (!stricmp(c, "players")) // Player info request.
	{
		SVC_PlayerInfo();
		return;
	}
	else if (!stricmp(c, "rules"))   // Rule info request.
	{
		SVC_RuleInfo();
		return;
	}
	else if (!strcmp(c,"connect"))  // Must include correct challenge #
	{
		SV_ConnectClient();
		return;
	}
	else if (!strcmp(c, "rcon"))
	{
		SV_Rcom (&net_from);
	}
	else
	{
		// Just ignore it.
		Con_Printf ("bad connectionless packet from %s:\n%s\n"
		, NET_AdrToString (net_from), s);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove any logo requests from previous server ( Should never happen since
//  pending_logos is wiped by sv.Clear() )
//-----------------------------------------------------------------------------
void SV_PruneRequestList( void )
{
	int c = sv.pending_logos.Count();
	int i;
	for ( i = c - 1; i >= 0 ; i-- )
	{
		CLogoRequest *logo = &sv.pending_logos[ i ];
		if ( logo->servercount == svs.spawncount )
			continue;

		sv.pending_logos.Remove( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: User requested download of a logo that we don't have resident
//  We probably asked another user to send it
// If it ever shows up, then we'll send it on down to any players who requested it
// Input  : *cl - 
//			crc - 
//-----------------------------------------------------------------------------
void SV_StoreLogoRequest( client_t *cl, CRC32_t& crc )
{
	SV_PruneRequestList();

	// Don't actually send back down to listen server client since I am that client already
	//  so once the other player's logo shows up here, it's available to the local client immediately
	if ( cl->netchan.remote_address.type == NA_LOOPBACK )
	{
		return;
	}

	int playerslot = cl - svs.clients;

	// See if this slot already occupied and update userid and logo if so.
	// This should catch any case where one player drops and another takes his/her
	// slot during a game.
	int c = sv.pending_logos.Count();
	int i;
	for ( i = 0; i <  c ; i++ )
	{
		CLogoRequest *logo = &sv.pending_logos[ i ];
		if ( logo->playerslot != playerslot )
			continue;

		logo->playerid = cl->userid;
		logo->logo = crc;
		return;
	}

	// Add a new entry
	CLogoRequest r;
	r.playerslot	= playerslot;
	r.playerid		= cl->userid;
	r.servercount	= svs.spawncount;
	r.logo			= crc;

	sv.pending_logos.AddToTail( r );
}

//-----------------------------------------------------------------------------
// Purpose: A client has uploaded its logo to us
// Got through and see if any pending logo downloads to this new logo file exist
// Input  : logoCRC - 
//-----------------------------------------------------------------------------
void SV_SendLogo( CRC32_t& logoCRC )
{
	SV_PruneRequestList();

	char crcfilename[ 512 ];
	Q_snprintf( crcfilename, sizeof( crcfilename ), "materials/decals/downloads/%s.vtf",
		COM_BinPrintf( (byte *)&logoCRC, 4 ) );

	// TODO:  Could recheck CRC of disk file here if we're paranoid

	int c = sv.pending_logos.Count();
	int i;
	for ( i = c - 1; i >= 0 ; i-- )
	{
		CLogoRequest *logo = &sv.pending_logos[ i ];
		// Is it the correct logo?
		if ( logo->logo != logoCRC )
			continue;

		// Lookup player, etc.
		if ( logo->playerslot >= 0 && logo->playerslot < svs.maxclients )
		{
			client_t *cl = &svs.clients[ logo->playerslot ];
			if ( ( cl->active || cl->spawned || cl->connected ) && !cl->fakeclient )
			{
				// Still the same player?
				if ( cl->userid == logo->playerid )
				{
					// Ummmm....shouldn't ever have to file transfer to ourselves!
					Assert( cl->netchan.remote_address.type != NA_LOOPBACK );

					// Send the logo to the player who needs it
					Netchan_CreateFileFragments( true, &cl->netchan, crcfilename );
				}
			}
		}

		// Remove from list
		sv.pending_logos.Remove( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: A File has been received, if it's a logo, send it on to any other players who need it
//  and return true, otherwise, return false
// Input  : *cl - 
//			*filename - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool SV_ProcessIncomingLogo( client_t *cl, char *filename )
{
	char crcfilename[ 512 ];
	Q_snprintf( crcfilename, sizeof( crcfilename ), "materials/decals/downloads/%s.vtf",
		COM_BinPrintf( (byte *)&cl->logo, 4 ) );

	// It's not a logo file?
	if ( Q_strcasecmp( filename, crcfilename ) )
	{
		return false;
	}

	// First, make sure crc is valid
	CRC32_t check;
	CRC_File( &check, crcfilename );
	if ( check != cl->logo )
	{
		Con_Printf( "Incoming logo file didn't match player's logo CRC, ignoring\n" );
		// Still note that it was a logo!
		return true;
	}

	// Okay, looks good, see if any other players need this logo file
	SV_SendLogo( check );
	return true;
}



//-----------------------------------------------------------------------------
// Purpose: Process incoming logo file
// Input  : *cl - 
//			*filename - 
//-----------------------------------------------------------------------------
void SV_ProcessFile( client_t *cl, char *filename )
{
	if ( SV_ProcessIncomingLogo( cl, filename ) )
		return;

	// Some other file...
	Con_Printf( "Received file %s from %s\n", filename, cl->name );
}


void SV_HandleForceWaitForAck( client_t *cl )
{
	if ( cl->m_ForceWaitForAck != -1 )
	{
		// If we get in here, then we sent a packet that was uncompressed (ie: no entity deltas)
		// to the client, and we've been waiting to hear back from them.
		
		if ( cl->netchan.incoming_acknowledged == cl->m_ForceWaitForAck )
		{
			// If we get in here, then the client is acking the sequence number in which we sent 
			// them the uncompressed packet. In most cases, this also means they got the uncompressed data,
			// but if netchan decided to drop the unreliable data, it may not have gotten through.

			if ( cl->delta_sequence == (cl->m_ForceWaitForAck & DELTAFRAME_MASK) )
			{
				// Cool, they got it! Now we can proceed like normal.
				cl->m_ForceWaitForAck = -1;
			}
			else
			{
				// Oops. Netchan must have dropped the unreliable data.
				cl->m_bResendNoDelta = true;
			}
		}	 
		else if ( cl->netchan.incoming_acknowledged > cl->m_ForceWaitForAck )
		{
			// If we get in here, it means the client missed the uncompressed data we sent, 
			// so we force it to send a new unreliable one.

			cl->m_bResendNoDelta = true;
		}
	}
}


/*
=================
SV_ReadPackets

Read's packets from clients and executes messages as appropriate.
=================
*/
void SV_ReadPackets (void)
{
	VPROF_BUDGET( "SV_ReadPackets", VPROF_BUDGETGROUP_OTHER_NETWORKING );

	int			i;
	client_t	*cl;

	while (NET_GetPacket (NS_SERVER))
	{
		MSG_GetReadBuf()->Reset();

		if ( Filter_ShouldDiscard ( &net_from ) )
		{
			Filter_SendBan( &net_from );	// tell them we aren't listening...
			continue;
		}

		// check for connectionless packet (0xffffffff) first
		if (*(int *)net_message.data == -1)
		{
			SV_ConnectionlessPacket ();
			continue;
		}

		// check for packets from connected clients
		for (i=0, cl=svs.clients ; i<svs.maxclients ; i++,cl++)
		{
			if (!cl->connected && !cl->active && !cl->spawned) // (cl->state == cs_free)
				continue;

			if (!NET_CompareAdr (net_from, cl->netchan.remote_address))
				continue;

			if (Netchan_Process(&cl->netchan))
			{
				// In single player, always respond
				if ( svs.maxclients == 1 )
				{
					cl->send_message = true;	// reply at end of frame
				}

				// During connection, only respond if client sends a packet
				if ( !cl->active || !cl->spawned )
				{
					cl->send_message = true;
				}

				// Run it.
				SV_ExecuteClientMessage (cl);

				// Now that we know whether they sent a clc_delta on this sequence,
				// we can determine if we need to resend an uncompressed packet.
				SV_HandleForceWaitForAck( cl );

				// Fix up clock in case prediction/etc. code reset it.
				g_ServerGlobalVariables.frametime = TICK_RATE;
			}

			// Fragmentation/reassembly sending takes priority over all game messages, want this in the future?
			if ( Netchan_IncomingReady( &cl->netchan ) )
			{
				if ( Netchan_CopyNormalFragments( &cl->netchan ) )
				{
					MSG_BeginReading();
					SV_ExecuteClientMessage( cl );
				}
				if ( Netchan_CopyFileFragments( &cl->netchan ) )
				{
					host_client = cl;
					SV_ProcessFile( cl, cl->netchan.incomingfilename );
				}
			}

			break;
		}

		if (i != svs.maxclientslimit)
			continue;

		// packet is not from a known client
		//	Con_Printf ("%s:sequenced packet without connection\n"
		// ,NET_AdrToString(net_from));
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client in sv_timeout.GetFloat()
seconds, drop the conneciton.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts (void)
{
	VPROF( "SV_CheckTimeouts" );
	
	// Don't timeout in _DEBUG builds
#if !defined( _DEBUG )
	int		    i;
	client_t	*cl;
	float	    droptime;

	droptime = realtime - sv_timeout.GetFloat();    // If the last received message is the current time
											//  minus the timeout.GetFloat() amount of time, drop the client.

	for (i=0,cl=svs.clients ; i<svs.maxclients ; i++,cl++)
	{
		if ( cl->fakeclient )
			continue;

		if ( ( cl->connected || cl->active || cl->spawned ) &&
			 ( cl->netchan.last_received < droptime ))
		{
			SV_BroadcastPrintf ("%s timed out\n", cl->name);
			SV_DropClient (cl, false, "%s timed out", cl->name );
		}
	}
#endif
}

float SV_CalcPing( client_t *cl, int numsamples /*= 1*/ )
{
	float		ping;
	int			i;
	int			count;
	int back;
	register	client_frame_t *frame;
	int idx;

	// bots don't have a real ping
	if ( cl->fakeclient )
		return 5;

	idx = cl - svs.clients;

	ping  = 0;
	count = 0;

	if ( numsamples <= 0 )
	{
		back = min( SV_UPDATE_BACKUP / 2, 16 );
	}
	else
	{
		back = min( SV_UPDATE_BACKUP, numsamples );
	}

	for ( i=0 ; i< SV_UPDATE_BACKUP ; i++)
	{
		frame = &cl->frames[ ( cl->netchan.incoming_acknowledged - i - 1) & SV_UPDATE_MASK ];
		if (frame->raw_ping > 0)
		{
			ping += frame->raw_ping;
			count++;
		}

		if ( count >= back )
			break;
	}

	if ( !count )
		return 0.0f;
	ping /= count;

	if ( ping < 0 )
	{
		ping = 0.0f;
	}

	return ping * 1000.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
//			numsamples - 
// Output : float
//-----------------------------------------------------------------------------
float SV_CalcLatency( client_t *cl, int numsamples )
{
	float		ping;
	int			i;
	int			count;
	int back;
	register	client_frame_t *frame;
	int idx;

	// bots don't have a real ping
	if ( cl->fakeclient )
		return 5;

	idx = cl - svs.clients;

	ping  = 0;
	count = 0;

	if ( numsamples <= 0 )
	{
		back = min( SV_UPDATE_BACKUP / 2, 16 );
	}
	else
	{
		back = min( SV_UPDATE_BACKUP, numsamples );
	}

	for ( i=0 ; i< back ; i++)
	{
		frame = &cl->frames[ ( cl->netchan.incoming_acknowledged - i - 1) & SV_UPDATE_MASK ];
		if (frame->latency > 0)
		{
			ping += frame->latency;
			count++;
		}
	}
	if (!count)
		return 0.0f;
	ping /= count;

	if ( ping < 0 )
	{
		ping = 0.0f;
	}

	return ping * 1000.0f;
}

/*
===================
SV_CalcPacketLoss

  Determine % of packets that were not ack'd.
===================
*/
int SV_CalcPacketLoss (client_t *cl)
{
	int    		lost;
	int			i;
	int			count;
	float       losspercent;
	register	client_frame_t *frame;
	int         numsamples;

	lost  = 0;
	count = 0;

	if ( cl->fakeclient )
		return 0;

	numsamples = SV_UPDATE_BACKUP / 2;

	for ( i = 0 ; i < numsamples; i++ )
	{
		frame = &cl->frames[(cl->netchan.incoming_acknowledged-1-i) & SV_UPDATE_MASK];
		count++;
		if (frame->latency == -1)
		{
			lost++;
		}
	}
	if (!count)
		return 100;

	losspercent = 100.0 * ( float )lost/( float )count;

	return (int)losspercent;
}

/*
===================
SV_FullClientUpdate

sends all the info about *cl to *sb
===================
*/
void SV_FullClientUpdate(client_t *cl, bf_write *sb)
{
	int i = cl - svs.clients;

	sb->WriteByte ( svc_updateuserinfo );
	sb->WriteUBitLong( i, MAX_CLIENT_BITS );

	if ( cl->name[0] )
	{
		sb->WriteOneBit( 1 );

		// Send an MD5 hash of (the hex version of the MD5 hash of) their CD key.
		MD5Context_t ctx;
		char digest[16];
		MD5Init(&ctx);
		MD5Update(&ctx, (unsigned char*)cl->hashedcdkey, sizeof(cl->hashedcdkey));
		MD5Final((unsigned char*)digest, &ctx);

		sb->WriteBytes( digest, 16 );

		// Send the userinfo string
		char	info[MAX_INFO_STRING];
		Q_strncpy( info, cl->userinfo, sizeof( info ) );
		// Remove server passwords, etc.
		Info_RemovePrefixedKeys( info, '_' );

		sb->WriteString( info );
	}
	else 
	{
		sb->WriteOneBit( 0 );
	}
}

/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

//-----------------------------------------------------------------------------
// Purpose: Writes events to the client's network buffer
// Input  : *cl -
//			*pack -
//			*msg -
//-----------------------------------------------------------------------------
void SV_EmitEvents( client_t *cl, client_frame_t *pack, bf_write *msg )
{
	int i, ev;
	CEventState *es;
	CEventInfo *info;
	int ev_count = 0;
//	int etofind;
	int c;

	es = &cl->events;

	// Count events
	for ( ev = 0; ev < MAX_EVENT_QUEUE; ev++ )
	{
		info = &es->ei[ ev ];
		if ( info->index == 0 )
			continue;
		ev_count++;
	}

	// Nothing to send
	if ( !ev_count )
		return;

	if ( ev_count >= MAX_EVENT_QUEUE )
	{
		ev_count = MAX_EVENT_QUEUE - 1;
	}

	// Create message
	msg->WriteByte( svc_event );
	// Up to MAX_EVENT_QUEUE events
	msg->WriteBitLong( ev_count, Q_log2( MAX_EVENT_QUEUE ), false );

	c = 0;
	for ( i = 0 ; i < MAX_EVENT_QUEUE; i++ )
	{
		info = &es->ei[ i ];
		if ( info->index == 0 )
		{
			continue;
		}

		// Only send if there's room
		if ( c < ev_count )
		{
			// Event index
			msg->WriteBitLong( info->index, sv.serverclassbits, false );

			msg->WriteBitLong( info->bits, EVENT_DATA_LEN_BITS, false );
			msg->WriteBits( info->data, info->bits );

			// Delay
			if ( info->fire_time == 0.0 )
			{
				msg->WriteOneBit( 0 );
			}
			else
			{
				msg->WriteOneBit( 1 );
				msg->WriteBitLong( (int)(info->fire_time * 100.0), 16, false );
			}
		}

		info->index = 0;

		// Increment counter
		c++;
	}
}

/*
================
SV_HasEventsInQueue

================
*/
qboolean SV_HasEventsInQueue( client_t *client )
{
	int i;
	CEventState *es;
	CEventInfo *ei;

	if ( !client )
		return false;

	es = &client->events;
	for ( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &es->ei[ i ];
		if ( ei->index != 0 )
			return true;
	}

	return false;
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

static int		s_FatBytes;
static byte*	s_pFatPVS = 0;

CUtlVector<int> g_AreasNetworked;

static void SV_AddToFatPVS( const Vector& org )
{
	int		i;
	byte	*pvs;

	pvs = CM_ClusterPVS( CM_LeafCluster( CM_PointLeafnum( org ) ) );
	for (i=0 ; i<s_FatBytes ; i++)
		s_pFatPVS[i] |= pvs[i];
}

//-----------------------------------------------------------------------------
// Purpose: Zeroes out pvs, this way we can or together multiple pvs's for a player
//-----------------------------------------------------------------------------
void SV_ResetPVS( byte* pvs )
{
	s_pFatPVS = pvs;
	s_FatBytes = (CM_NumClusters()+7)>>3;
	Q_memset (s_pFatPVS, 0, s_FatBytes);
	g_AreasNetworked.RemoveAll();
}

/*
=============
Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
void SV_AddOriginToPVS( const Vector& origin )
{
	SV_AddToFatPVS( origin );
	int area = CM_LeafArea( CM_PointLeafnum( origin ) );
	int i;
	for( i = 0; i < g_AreasNetworked.Count(); i++ )
	{
		if( g_AreasNetworked[i] == area )
		{
			return;
		}
	}
	g_AreasNetworked.AddToTail( area );
}

//-----------------------------------------------------------------------------
// Purpose: Write current timestamp and clock drift value to specified client
// Input  : msg - 
//			*client - 
//-----------------------------------------------------------------------------
void SV_WriteServerTimestamp( bf_write& msg, client_t *client )
{
	// Send the client the current time stamp.
	msg.WriteByte( svc_time );
	msg.WriteLong( sv.tickcount );
}

//-----------------------------------------------------------------------------
// Writes the client datagram header
//-----------------------------------------------------------------------------

static void WriteClientDatagramHeader( bf_write& msg, client_t* client )
{
	// Send the client the current time stamp.
	SV_WriteServerTimestamp( msg, client );

	// add the client specific data to the datagram
	SV_WriteClientdataToMessage (client, &msg);

	// Update shared client/server string tables
	SV_UpdateStringTables( client, &msg );

//	COM_Log( "sv.log", "sending world state %i(%i), last command number executed %i(%i)\n",
//		client->netchan.outgoing_sequence, client->netchan.outgoing_sequence & SV_UPDATE_MASK,
//		client->netchan.incoming_sequence, client->netchan.incoming_sequence & SV_UPDATE_MASK );
}

void SV_SendRestoreMsg( bf_write *dest )
{
#ifndef SWDS
	int				i;
	CSaveRestoreData currentLevelData;
	char			name[256];

	byte			buf[NET_MAX_PAYLOAD];

	if ( !sv.loadgame )
		return;

	bf_write msg( "SV_SendRestoreMsg->msg", buf, sizeof(buf) );

	memset( &currentLevelData, 0, sizeof(currentLevelData) );

	g_ServerGlobalVariables.pSaveData = &currentLevelData;
	// Build the adjacent map list
	serverGameDLL->BuildAdjacentMapList();

	msg.WriteByte ( svc_restore);
	Q_snprintf( name, sizeof( name ), "%s%s.HL2", saverestore->GetSaveDir(), sv.name );
	COM_FixSlashes( name );
	msg.WriteString( name );
	msg.WriteByte( currentLevelData.levelInfo.connectionCount );
	for ( i = 0; i < currentLevelData.levelInfo.connectionCount; i++ )
	{
		msg.WriteString( currentLevelData.levelInfo.levelList[i].mapName );
	}

	g_ServerGlobalVariables.pSaveData = NULL;

	// See if there's room, not enough, wait one frame
	if ( msg.GetNumBytesWritten() > dest->GetNumBytesLeft() )
		return;

	dest->WriteBits( msg.m_pData, msg.GetNumBitsWritten() );

	// Reset
	sv.loadgame = false;
#endif
}

/*
=======================
SV_UpdateToReliableMessages

=======================
*/
void SV_UpdateToReliableMessages (void)
{
	int			i, j;
	client_t *client;

// check for changes to be sent over the reliable streams
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->edict)
			continue;

		// update any userinfo packets that have changed
		if (!host_client->sendinfo)
			continue;

		host_client->sendinfo = false;
		SV_FullClientUpdate (host_client, &sv.reliable_datagram);
	}

	// Clear the server datagram if it overflowed.
	if (sv.datagram.IsOverflowed())
	{
		Con_DPrintf("sv.datagram overflowed!\n");
		sv.datagram.Reset();
	}

	// Now send the reliable and server datagrams to all clients.
	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
	{
		if ( ( !client->active && !client->spawned ) || client->fakeclient)
			continue;

		client->netchan.message.WriteBits(sv.reliable_datagram.GetData(), sv.reliable_datagram.GetNumBitsWritten());
		client->datagram.WriteBits(sv.datagram.GetData(), sv.datagram.GetNumBitsWritten());
	}

	// Now clear the reliable and datagram buffers.
	sv.reliable_datagram.Reset();
	sv.datagram.Reset();
}


void SV_FakeCLCDeltaForBots( client_t *pClient, CFrameSnapshot *pSnapshot )
{
	if ( !pClient->fakeclient || !sv_stressbots.GetBool() )
		return;

	// Find a frame for this client that has a snapshot # previous to the specified snapshot.
	int iBest = -1;
	int bestDiff = 10000000;
	for ( int i=0; i < pClient->numframes; i++ )
	{
		CFrameSnapshot *pTest = pClient->frames[i].GetSnapshot();
		if ( pTest )
		{
			int diff = pSnapshot->m_nTickNumber - pTest->m_nTickNumber;
			if ( diff > 0 && diff < bestDiff )
			{
				iBest = i;
				bestDiff = diff;
			}
		}
	}

	pClient->delta_sequence = iBest;

	// Fake this so it starts setting up fullpacks for different frames.
	++pClient->netchan.outgoing_sequence;
}


void SV_HandleClientDatagramOverflow( client_t *pClient )
{
	Con_Printf ("WARNING: datagram overflowed for %s\n", pClient->name);

#if defined( _DEBUG )
	{
		char const *NET_GetDebugFilename( char const *prefix );
		void NET_StorePacket( char const *filename, byte const *buf, int len );
		
		char const *filename = NET_GetDebugFilename( "datagram" );
		if ( filename )
		{
			Con_Printf( "Saving overflowed datagram of %i bytes for %s to file %s\n",
				pClient->datagram.GetNumBytesWritten(), pClient->name, filename );
			
			NET_StorePacket( filename, (byte const *)pClient->datagram.GetBasePointer(), pClient->datagram.GetNumBytesWritten() );
			
		}
		else
		{
			Con_Printf( "Too many files in debug directory, clear out old data!\n" );
		}
	}
#endif
}



//-----------------------------------------------------------------------------
// Sends datagrams to all clients who need one
//-----------------------------------------------------------------------------

static void SV_SendClientDatagrams ( int clientCount, client_t** clients, CFrameSnapshot* pSnapshot )
{
	byte		buf[NET_MAX_PAYLOAD];
	bool		wrotedatagram;
	int i;
	client_frame_t *pPack[MAX_CLIENTS];

	bf_write msg;
	msg.SetDebugName( "SV_SendClientDatagrams->msg" );

	// Compute the client packs
	SV_ComputeClientPacks( clientCount, clients, pSnapshot, pPack );

	for (i = 0; i < clientCount; ++i)
	{
		client_t *pClient = clients[i];
		
		// Pretend the bot ack'd a packet so we're not always making huge deltas for it.
		SV_FakeCLCDeltaForBots( pClient, pSnapshot );
		
		TRACE_PACKET( ( "SV Send (%d)\n", pClient->netchan.outgoing_sequence ) );

		wrotedatagram = false;
		msg.StartWriting(buf, sizeof(buf));

		// If we've sent an uncompressed packet and don't know if the client has received it
		// or not, we don't send more entity data.
		// See the definition of m_ForceWaitForAck and the comments in SV_ForceWaitForAck
		// for info about why we do this.
		if ( pClient->m_ForceWaitForAck == -1 || pClient->m_bResendNoDelta )
		{
			WriteClientDatagramHeader( msg, pClient );

			// Encode the packet entities as a delta from the
			// last packetentities acknowledged by the client
			SV_EmitPacketEntities( pClient, pPack[i], pSnapshot, &msg );
			pClient->m_bResendNoDelta = false;

			SV_EmitEvents( pClient, pPack[i], &msg );
		}

		// Did we overflow the message buffer?  If so, just ignore it.
		if (pClient->datagram.IsOverflowed())
		{
			SV_HandleClientDatagramOverflow( pClient );
		}
		else  // Otherwise, copy the client->datagram into the message stream, too.
		{
			msg.WriteBits(pClient->datagram.m_pData, pClient->datagram.GetNumBitsWritten());
			wrotedatagram = true;
		}

		pClient->datagram.m_iCurBit = 0;

		// If this game was restored, send the restore data now
		// This is a hack, but in single player it's okay to do this
		if ( wrotedatagram && ( svs.maxclients == 1 ) )
		{
			// Must only write into the datagram if there is room
			SV_SendRestoreMsg( &msg );
		}

		// send deltas over reliable stream
		if ( msg.IsOverflowed() )
		{
			Con_Printf ("WARNING: msg overflowed for %s\n", pClient->name);
			msg.Reset();
		}

		// Send the datagram
		if ( !pClient->fakeclient )
		{
			Netchan_TransmitBits( &pClient->netchan, msg.GetNumBitsWritten(), buf );
		}
	}
}


//-----------------------------------------------------------------------------
// This function contains all the logic to determine if we should send a datagram
// to a particular client
//-----------------------------------------------------------------------------

static bool UpdateHostClientSendState( client_t* client )
{
	// If sv_stressbots is true, then treat a bot more like a regular client and do deltas and such for it.
	if(!sv_stressbots.GetInt())
	{
		if(client->fakeclient)
			return false;
	}

	if ( ( !client->active && !client->connected && !client->spawned ) )
		return false;

	if ( client->skip_message )
	{
		client->skip_message = false;
		return false;
	}

	if ( !host_limitlocal.GetFloat() && client->netchan.remote_address.type == NA_LOOPBACK )
	{
		client->send_message = true;
	}

	// During connection, only respond if client sent a packet
	if ( client->active && client->spawned )
	{
		// Try to send a message as soon as we can.  If the target time for sending is within the next frame interval ( based on last frame ),
		//  trigger the send now.
		// Note that in single player, send_message is also set to true any time a packet arrives from the client.

		float time_unti_next_message = client->next_messagetime - ( realtime + host_frametime );

		if ( time_unti_next_message <= 0.0f )
		{
			client->send_message = true;
		}

		// Something got hosed
		if ( time_unti_next_message > 2.0f )
		{
			client->send_message = true;
		}
	}

	// if the reliable message overflowed,
	// drop the client
	if (client->netchan.message.IsOverflowed())
	{
		client->netchan.message.Reset();
		client->datagram.Reset();
		SV_BroadcastPrintf ("%s overflowed\n", client->name);
		Con_Printf ("WARNING: reliable overflow for %s\n",client->name);

		SV_DropClient (client, false, "%s overflowed the reliable buffer", client->name );
		client->send_message = true;
		client->netchan.cleartime = 0;	// don't choke this message

	}
	else if ( client->send_message )
	{
		// If we haven't gotten a message in sv_failuretime seconds, then stop sending messages to this client
		//  until we get another packet in from the client.  This prevents crash/drop and reconnect where they are
		//  being hosed with "sequenced packet without connection" packets.
		if ( ( realtime - client->netchan.last_received ) > sv_failuretime.GetFloat() &&
			!client->fakeclient )
		{
			client->send_message = false;
		}
	}

	// only send messages if the client has sent one
	// and the bandwidth is not choked
	if ( !client->send_message )
		return false;

	// Bandwidth choke active?
	if (!Netchan_CanPacket (&client->netchan))
	{
		client->chokecount++;
		return false;
	}

	client->send_message = false;

	// Now that we were able to send, reset timer to point to next possible send time.
	client->next_messagetime = realtime + host_frametime + client->next_messageinterval;

	return true;
}


//-----------------------------------------------------------------------------
// A potential optimization of the client data sending; the optimization
// is based around the fact that we think that we're spending all our time in
// cache misses since we are accessing so much memory
//-----------------------------------------------------------------------------

void SV_SendClientMessages (void)
{
	VPROF_BUDGET( "SV_SendClientMessages", VPROF_BUDGETGROUP_OTHER_NETWORKING );
	
	int i;

	// Take new snapshot
	CFrameSnapshot* pSnapshot = framesnapshot->TakeTickSnapshot( host_tickcount );

	// update frags, names, etc
	SV_UpdateToReliableMessages ();

	// build individual updates
	int receivingClientCount = 0;
	client_t*	pReceivingClients[MAX_CLIENTS];
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		// Update Host client send state...
		if ( !UpdateHostClientSendState( host_client ))
			continue;

		// Append the unreliable data (player updates and packet entities)
		if ( host_client->active && host_client->spawned )
		{
			// Add this client to the list of clients we're gonna send to.
			pReceivingClients[receivingClientCount] = host_client;
			++receivingClientCount;
		}
		else
		{
			// Connected, but inactive, just send reliable, sequenced info.
			if(!host_client->fakeclient)
				Netchan_Transmit( &host_client->netchan, 0, NULL );
		}
	}

	if (receivingClientCount)
		SV_SendClientDatagrams( receivingClientCount, pReceivingClients, pSnapshot );

	// Allow game .dll to run code, including unsetting EF_MUZZLEFLASH and EF_NOINTERP on effects fields
	// etc.
	serverGameClients->PostClientMessagesSent();
	pSnapshot->ReleaseReference();

	// Always reset this after sending packets to clients
	sv.sound_sequence = 0;
}

/*
================
qboolean SV_ShouldUpdatePing( int clientindex )

================
*/
qboolean SV_ShouldUpdatePing( int clientindex )
{
	int curmodulo;

	// In single player, update about once every 30 frames or so.
	if ( svs.maxclientslimit == 1 )
	{
		return ( !( host_tickcount % 30 ) );
	}

	// Otherwise update based on modulo
	curmodulo = host_tickcount % svs.maxclientslimit;

	if ( clientindex == curmodulo )
		return true;
	else
		return false;
}

// Look for the specified entity in all the client frames.
// FIXME:  YWB  Seems like it would be better to store and search by entity number since we are
//  returning a PacketEntity *...
PackedEntity* SV_FindEntityInCache(ClientFrameCache *pCache, int iEntity)
{
	// For debugging..
	if(!sv_CacheEncodedEnts.GetInt())
		return NULL;

	for(int i=0; i < pCache->m_nFrames; i++)
	{
		client_frame_t *pFrame = pCache->m_Frames[i];

		for(int e=0; e < pFrame->entities.num_entities; e++)
		{
			if(pFrame->entities.entities[e].m_nEntityIndex == iEntity)
				return &pFrame->entities.entities[e];
		}
	}

	return NULL;
}



/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage (client_t *client, bf_write *msg)
{
	int		i;
	edict_t *ent;
	client_frame_t	*frame;

	ent = client->edict;

	// This is the frame we are creating
	frame = &client->frames[ client->netchan.outgoing_sequence & SV_UPDATE_MASK ];

	// Save current time for next ping calculation
	frame->senttime = realtime;
	frame->latency	= -1; 
	frame->raw_ping = -1;

	// send the chokecount for net usage graph
	if (client->chokecount)
	{
		msg->WriteByte (svc_choke);
		client->chokecount = 0;
	}

//
// send the current viewpos offset from the view entity
//
// a fixangle might get lost in a dropped packet.  Oh well.
	Assert( serverGameClients );
	CPlayerState *pl = serverGameClients->GetPlayerState( client->edict );
	Assert( pl );

	if ( pl && pl->fixangle )
	{
		if ( pl->fixangle == 2 )
		{
			msg->WriteByte (svc_addangle);
			msg->WriteBitAngle( pl->anglechangetotal, 16 );
			msg->WriteBitAngle( pl->anglechangefinal, 16 );
			pl->anglechangetotal = 0;
			pl->anglechangefinal = 0;
		}
		else
		{
			msg->WriteByte (svc_setangle);
			QAngle angles = vec3_angle;
			angles = pl->v_angle;
			for (i=0 ; i < 3 ; i++)
			{
				msg->WriteBitAngle( angles[i], 16 );
			}
		}
		pl->fixangle = 0;
	}

	// Send the data marker
	msg->WriteByte (svc_clientdata);
}


/*
================
SV_CheckUpdateRate

Get default updates / second value
================
*/
void SV_CheckUpdateRate( double *rate )
{
	Assert( rate );

	if ( (*rate) == 0.0 )
	{
		*rate = 1.0/20.0;
		return;
	}

	// 1000 fpx max update rate
	if ( sv_maxupdaterate.GetFloat() <= 0.001f && sv_maxupdaterate.GetFloat() != 0.0 )
	{
		sv_maxupdaterate.SetValue( 30.0f );
	}

	if ( sv_minupdaterate.GetFloat() <= 0.001f && sv_minupdaterate.GetFloat() != 0.0 )
	{
		sv_minupdaterate.SetValue( 1.0f );
	}

	if ( sv_maxupdaterate.GetFloat() != 0.0 )
	{
		if ( *rate < 1.0 / sv_maxupdaterate.GetFloat() )
			*rate = 1.0 / sv_maxupdaterate.GetFloat();
	}

	if ( sv_minupdaterate.GetFloat() != 0.0 )
	{
		if ( *rate > 1.0 / sv_minupdaterate.GetFloat() )
			*rate = 1.0 / sv_minupdaterate.GetFloat();
	}
}

/*
================
SV_CheckRate

Make sure channel rate for active client is within server bounds
================
*/
void SV_CheckRate( client_t *cl )
{
	if ( sv_maxrate.GetFloat() > 0 )
	{
		if ( cl->netchan.rate > sv_maxrate.GetFloat() )
		{
			cl->netchan.rate = min( sv_maxrate.GetFloat(), MAX_RATE );
		}
	}

	if ( sv_minrate.GetFloat() > 0 )
	{
		if ( cl->netchan.rate < sv_minrate.GetFloat() )
		{
			cl->netchan.rate = max( sv_minrate.GetFloat(), MIN_RATE );
		}
	}
}

char *SV_ExtractNameFromUserinfo( client_t *cl )
{
	char	*val;
	char	*p, *q;
	int		i;
	client_t	*client;
	int		dupc = 1;
	char	newname[80];

	// name for C code
	val = (char *)Info_ValueForKey (cl->userinfo, "name");

	// trim user name
	Q_strncpy(newname, val, sizeof(newname));

	for (p = newname; *p == ' ' && *p; p++)
		;
	if (p != newname && *p) {
		for (q = newname; *p; *q++ = *p++)
			;
		*q = 0;
	}
	for (p = newname + strlen(newname) - 1; p != newname && *p == ' '; p--)
		;
	p[1] = 0;

	if (strcmp(val, newname)) {
		Info_SetValueForKey (cl->userinfo, "name", newname, MAX_INFO_STRING);
		val = (char *)Info_ValueForKey (cl->userinfo, "name");
	}

	if (!val[0] || !stricmp(val, "console")) {
		Info_SetValueForKey (cl->userinfo, "name", "unnamed", MAX_INFO_STRING);
		val = (char *)Info_ValueForKey (cl->userinfo, "name");
	}

	// check to see if another user by the same name exists
	while (1) {
		for (i=0, client = svs.clients ; i<MAX_CLIENTS ; i++, client++) {
			if ( !client->active || !client->spawned || client == cl)
				continue;
			if (!stricmp(client->name, val))
				break;
		}
		if (i != MAX_CLIENTS) { // dup name
			if (strlen(val) > sizeof(cl->name) - 1)
				val[sizeof(cl->name) - 4] = 0;
			p = val;

			if (val[0] == '(')
				if (val[2] == ')')
					p = val + 3;
				else if (val[3] == ')')
					p = val + 4;

			Q_snprintf(newname, sizeof( newname ), "(%d)%-0.40s", dupc++, p);
			Info_SetValueForKey (cl->userinfo, "name", newname, MAX_INFO_STRING);
			val = (char *)Info_ValueForKey (cl->userinfo, "name");
		} else
			break;
	}

	return val;
}

/*
=================
SV_ExtractFromUserinfo

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_ExtractFromUserinfo (client_t *cl)
{
	char	*val;
	int		i;

	// give entity dll a chance to look at the new userinfo
	serverGameClients->ClientUserInfoChanged( cl->edict, cl->userinfo );

	Q_strncpy(cl->name, SV_ExtractNameFromUserinfo( cl ), sizeof(cl->name));

	// rate command
	val = (char *)Info_ValueForKey (cl->userinfo, "rate");
	if (strlen(val))
	{
		i = atoi(val);

		if (i < MIN_RATE)
			i = MIN_RATE;
		if (i > MAX_RATE)
			i = MAX_RATE;

		cl->netchan.rate = i;
	}

	val = (char *)Info_ValueForKey( cl->userinfo, "cl_updaterate" );
	if ( Q_strlen( val ) )
	{
		i = Q_atoi( val );
#if !defined( _DEBUG )
		if ( i < 10 )
		{
			i = 10;
		}
#endif
		if ( i > 0 )
		{
			cl->next_messageinterval = 1.0f/(float)i;
		}
	}

	// Bound to server min / max values
	SV_CheckUpdateRate( &cl->next_messageinterval );

	SV_CheckRate( cl );
}


/*
==============================================================================
SERVER SPAWNING

==============================================================================
*/

void SV_WriteVoiceCodec(bf_write *pBuf)
{
	pBuf->WriteByte( svc_voiceinit );

	// Only send the voice codec in multiplayer. Otherwise, we don't want voice.
	if( svs.maxclients <= 1 )
		pBuf->WriteString( "" );
	else
		pBuf->WriteString( sv_VoiceCodec.GetString() );
}


// UNDONE: "player.mdl" ???  This should be set by name in the DLL
/*
================
SV_CreateBaseline

================
*/
void SV_CreateBaseline (void)
{
	SV_WriteVoiceCodec( &sv.signon );

	ServerClass *pClasses = serverGameDLL->GetAllServerClasses();

	// Send SendTable info.
	SV_WriteSendTables(pClasses, &sv.signon_fullsendtables);

	// Send class descriptions.
	SV_WriteClassInfos(pClasses, &sv.signon_fullsendtables);


	for ( int entnum = 0; entnum < sv.num_edicts ; entnum++)
	{
		// If it would overflow the signon buffer, stop writing baselines. This means it'll be sending them
		// more data in the delta'd ents.
		if ( sv.signon.GetNumBytesLeft() < MAX_PACKEDENTITY_DATA )
		{
			Warning( "Exhausted signon buffer. Skipping remaining baselines (ents %d - %d).\n", entnum, sv.num_edicts );
			break;
		}

		// get the current server version
		edict_t *svent = sv.edicts + entnum;

		PackedEntity *svbaseline = &sv.baselines.entities[entnum];
		svbaseline->m_nEntityIndex = entnum;

		if ( svent->free || !svent->m_pEnt )
			continue;

		IServerEntity *serverEntity = svent->GetIServerEntity();
		if ( !serverEntity )
			continue;

		if ( entnum > svs.maxclients && !serverEntity->GetModelIndex() )
			continue;

		// Players aren't spawned until they enter the server so their baseline hasn't been setup yet.
		char const* pModelString;
		if (entnum > 0 && entnum <= svs.maxclients)
		{
			pModelString = "models/player.mdl";
		}
		else
		{
			pModelString = STRING(serverEntity->GetModelName());
		}

		serverEntity->SetModelIndex( SV_ModelIndex( pModelString ) );
		if (serverEntity->GetModelIndex() < 0)
		{
			Sys_Error("Model %s not precached", pModelString);
		}

		//
		// create entity baseline
		//
		ServerClass *pClass   = svent->m_pEnt ? svent->m_pEnt->GetServerClass() : 0;
		if ( pClass )
		{
			SendTable *pSendTable = pClass->m_pTable;
	
			char packedData[MAX_PACKEDENTITY_DATA];
			bf_write writeBuf( "SV_CreateBaseline->writeBuf", packedData, sizeof( packedData ) );
			
			if ( !SendTable_Encode(
				pSendTable, 
				svent->m_pEnt, 
				&writeBuf, 
				NULL,	// Pass in NULL for pDeltaBits so it writes all properties.
				entnum) )
			{
				Host_Error("SV_CreateBaseline: SendTable_Encode returned false (ent %d).\n", entnum);
			}
			
			svbaseline->AllocAndCopyPadded( packedData, writeBuf.GetNumBytesWritten(), &g_PackedDataAllocator );
			svbaseline->m_pSendTable = pSendTable;


			// Now put the baseline into the signon message.
			sv.signon.WriteByte(svc_spawnbaseline);
			sv.signon.WriteUBitLong( entnum, 10 );
			sv.signon.WriteUBitLong( pClass->m_ClassID, sv.serverclassbits );

			void *pBaselineData = svbaseline->LockData();
			sv.signon.WriteBits( pBaselineData, writeBuf.GetNumBitsWritten() );
			svbaseline->UnlockData();
		}
	}
}

void SV_ClearBaselines( void )
{
	edict_t			*svent;
	PackedEntity	*svbaseline;
	int				entnum;

	for (entnum = 0; entnum < sv.num_edicts ; entnum++)
	{
		// get the current server version
		svent = sv.edicts + entnum;

		svbaseline = &sv.baselines.entities[entnum];

		svbaseline->FreeData();
	}
}

/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void SV_BroadcastCommand (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	char	    data[128];
	int         i;
	client_t    *cl;

	bf_write msg( "SV_BroadcastCommand", data, sizeof(data) );

	if (!sv.active)
		return;

	va_start (argptr,fmt);
	Q_vsnprintf (string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	msg.WriteByte   ( svc_stufftext );
	msg.WriteString ( string );

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if ((!cl->active && !cl->connected && !cl->spawned) || cl->fakeclient)
			continue;

		cl->netchan.message.WriteBytes(msg.GetData(), msg.GetNumBytesWritten());
	}
}

/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect (void)
{
	SV_BroadcastCommand("reconnect\n");

	/*if (cls.state != ca_dedicated)
		Cbuf_InsertText ("reconnect\n");
	*/
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : runPhysics -
//-----------------------------------------------------------------------------
void SV_ActivateServer()
{
	int i;

	// Activate the DLL server code
	serverGameDLL->ServerActivate( sv.edicts, sv.num_edicts, svs.maxclients );

	sv.active = true;
	// all setup is completed, any further precache statements are errors
	sv.state = ss_active;
	
	// create a baseline for more efficient communications
	SV_CreateBaseline();

	// Send serverinfo to all connected clients
	for (i=0,host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (host_client->active || host_client->connected)
		{
			Netchan_Clear( &host_client->netchan );
			host_client->delta_sequence = -1;
		}
	}

	// Tell what kind of server has been started.
	if (svs.maxclients > 1)
	{
		Con_DPrintf ("%i player server started\n", svs.maxclients);
	}
	else
	{
		Con_DPrintf ("Game started\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SV_ClearMemory( void )
{
	StaticPropMgr()->LevelShutdown();

	SV_ClearBaselines();

	Host_FreeToLowMark( true );

	sv.Clear();

	SV_MapOverClients(SV_DeleteClientFrames);
}


#include "tier0/memdbgoff.h"

static void SV_AllocateEdicts()
{
	sv.edicts = (edict_t *)Hunk_AllocName( sv.max_edicts*sizeof(edict_t), "edicts" );

	// Invoke the constructor so the vtable is set correctly..
	for (int i = 0; i < sv.max_edicts; ++i)
	{
		new( &sv.edicts[i] ) edict_t;
	}

}

#include "tier0/memdbgon.h"


/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
int SV_SpawnServer( char *mapname, char *startspot )
{
	client_t		*cl;
	edict_t			*ent;
	int				i;
	char			szDllName[MAX_QPATH];
	unsigned int	tmpChecksum;

	Assert( serverGameClients );

	//
	// tell all connected clients that we are going to a new level
	//
	if ( sv.active )
	{
		SV_SendReconnect ();

		for (i=0, cl = svs.clients ; i < svs.maxclients ; i++, cl++ )
		{
			// now tell dll about disconnect
			if ( ( !cl->active && !cl->spawned && !cl->connected ) || 
				!cl->edict || cl->edict->free )
			{
				cl->connected = false;
				cl->active = false;
				cl->spawned = false;
				continue;
			}

			// call the prog function for removing a client
			SV_DisconnectClient( cl->edict );
			
			// Leave connected, client should send "new" to jumpstart connection state machine
			cl->connected = true;
			cl->active = false;
			cl->spawned = false;
		}
	}

	COM_SetupLogDir( mapname );

	g_Log.Open();
	g_Log.Printf( "Spawning server %s\n", mapname );
	g_Log.PrintServerVars();

	// init network stuff
	NET_Config ( svs.maxclients > 1 ? true : false );

	// let's not have any servers with no name
	if ( host_name.GetString()[0] == 0 )
	{
		host_name.SetValue( serverGameDLL->GetGameDescription() );
	}

#ifndef SWDS
	SCR_CenterStringOff();
#endif

	if ( startspot )
	{
		Con_DPrintf("Spawn Server %s: [%s]\n", mapname, startspot );
	}
	else
	{
		Con_DPrintf("Spawn Server %s\n", mapname );
	}

	// Any partially connected client will be restarted if the spawncount is not matched.
	gHostSpawnCount = ++svs.spawncount;

	//
	// make cvars consistant
	//
	deathmatch.SetValue( (svs.maxclients > 1) ? 1 : 0 );
	if ( coop.GetInt() )
	{
		deathmatch.SetValue( 0 );
	}

	current_skill = (int)(skill.GetFloat() + 0.5);
	current_skill = max( current_skill, 0 );
	current_skill = min( current_skill, 3 );

	skill.SetValue( (float)current_skill );

	//
	// set up the new server
	//
	// Clear out any old client frame data
	SV_ClearMemory();

	SV_UPDATE_BACKUP = ( svs.maxclients == 1 ) ? SINGLEPLAYER_BACKUP : MULTIPLAYER_BACKUP;
	SV_UPDATE_MASK   = ( SV_UPDATE_BACKUP - 1 );

	// Reallocate frames based on new server maxclients...
	SV_MapOverClients( SV_AllocClientFrames );

	// Clear out the state of the most recently sent packed entities from
	// the snapshot manager
	framesnapshot->LevelChanged();

	strcpy ( sv.name, mapname );
	if (startspot)
	{
		strcpy(sv.startspot, startspot);
	}
	else
	{
		sv.startspot[0] = 0;
	}

	// Force normal player collisions for single player
	if ( svs.maxclients == 1 )
	{
		sv_clienttrace.SetValue( 1 );
	}

	// Allocate server memory
	sv.max_edicts = COM_EntsForPlayerSlots( svs.maxclients );

	g_ServerGlobalVariables.maxEntities = sv.max_edicts;
	g_ServerGlobalVariables.maxClients = svs.maxclients;

	// Assume no entities beyond world and client slots
	sv.num_edicts = svs.maxclients+1;

	SV_AllocateEdicts();

	sv.baselines.max_entities = sv.baselines.num_entities = sv.max_edicts;
	sv.baselines.entities = (PackedEntity*)Hunk_AllocName( sv.baselines.num_entities*sizeof(PackedEntity), "baselines" );

	serverGameEnts->SetDebugEdictBase(sv.edicts);

	sv.datagram.StartWriting(sv.datagram_buf, sizeof(sv.datagram_buf));
	sv.datagram.SetDebugName( "sv.datagram" );

	sv.reliable_datagram.StartWriting(sv.reliable_datagram_buf, sizeof(sv.reliable_datagram_buf));
	sv.reliable_datagram.SetDebugName( "sv.reliable_datagram" );

	sv.multicast.StartWriting(sv.multicast_buf, sizeof(sv.multicast_buf));
	sv.multicast.SetDebugName( "sv.multicast_buf" );

	sv.signon.StartWriting(sv.signon_buffer, sizeof(sv.signon_buffer));
	sv.signon.SetDebugName( "sv.signon" );

	sv.signon_fullsendtables.StartWriting( sv.signon_fullsendtables_buffer, sizeof( sv.signon_fullsendtables_buffer ) );
	sv.signon_fullsendtables.SetDebugName( "sv.signon_fullsendtables" );

	sv.allowsignonwrites = true;

	sv.serverclasses = 0;		// number of unique server classes
	sv.serverclassbits = 0;		// log2 of serverclasses

	// allocate player data, and assign the values into the edicts
	for ( i=0 ; i<svs.maxclients ; i++ )
	{
		ent = sv.edicts + i + 1;
		// Setup up the edict
		InitializeEntityDLLFields( ent );
		svs.clients[i].edict = ent;
	}

	sv.state	= ss_loading;
	sv.paused	= false;

	// Set initial time values.
	sv.tickcount	= (int)( 1.0 / TICK_RATE ) + 1; // Start at appropximate 1
	g_ServerGlobalVariables.tickcount = sv.tickcount;
	g_ServerGlobalVariables.curtime = sv.gettime();

	// Load the world model.
	Q_snprintf (sv.modelname,sizeof( sv.modelname ), "maps/%s.bsp", mapname );

	g_pFileSystem->AddSearchPath( sv.modelname, "GAME", PATH_ADD_TO_HEAD );

	// JAYHL2: The GetModelForName shouldn't be necessary when we convert to cmodel across the board
	CM_LoadMap( sv.modelname, false, &tmpChecksum );

	host_state.worldmodel = modelloader->GetModelForName(sv.modelname, IModelLoader::FMODELLOADER_SERVER );
	if (!host_state.worldmodel)
	{
		Con_Printf ("Couldn't spawn server %s\n", sv.modelname);
		sv.active = false;
		return 0;
	}

	if ( svs.maxclients > 1 )
	{
		// Server map CRC check.
		CRC32_Init(&host_state.worldmapCRC);
		if ( !CRC_MapFile( &host_state.worldmapCRC, sv.modelname ) )
		{
			Con_Printf ("Couldn't CRC server map:  %s\n", sv.modelname);
			sv.active = false;
			return 0;
		}

		// DLL CRC check.
		Q_snprintf( szDllName, sizeof( szDllName ), "bin\\client.dll");
		COM_FixSlashes( szDllName );

		if ( !CRC_File( &sv.clientSideDllCRC, szDllName ) )
		{
			Con_Printf ("Couldn't CRC client side dll:  %s\n", szDllName);
			sv.active = false;
			return 0;
		}
	}
	else
	{
		host_state.worldmapCRC	= 0;
		sv.clientSideDllCRC = 0;
	}

	// Create network string tables ( including precache tables )
	SV_CreateNetworkStringTables();

	// Leave empty slots for models/sounds/generic (not for decals though)
	sv.PrecacheModel( "", 0 );
	sv.PrecacheGeneric( "", 0 );
	sv.PrecacheSound( "", 0 );

	// Add in world
	sv.PrecacheModel( sv.modelname, RES_FATALIFMISSING | RES_PRELOAD, host_state.worldmodel );
	// Add world submodels to the model cache
	for ( i = 1 ; i < host_state.worldmodel->brush.numsubmodels ; i++ )
	{
		// Add in world brush models
		sv.PrecacheModel( localmodels[ i ], RES_FATALIFMISSING | RES_PRELOAD, modelloader->GetModelForName( localmodels[ i ], IModelLoader::FMODELLOADER_SERVER ) );
	}

	//
	// clear world interaction links
	//
	SV_ClearWorld ();

	//
	// load the rest of the entities
	//
	ent = sv.edicts;
	InitializeEntityDLLFields( ent );
	ent->free = false;

	if (coop.GetFloat())
	{
		g_ServerGlobalVariables.coop = (coop.GetInt() != 0);
	}
	else
	{
		g_ServerGlobalVariables.deathmatch = (deathmatch.GetInt() != 0);
	}

	g_ServerGlobalVariables.mapname   = MAKE_STRING( sv.name );
	g_ServerGlobalVariables.startspot = MAKE_STRING( sv.startspot );

	allow_cheats = sv_cheats.GetInt();

	GetTestScriptMgr()->CheckPoint( "map_load" );

	// set game event

	KeyValues * event = new KeyValues( "server_spawn" );
	event->SetString( "hostname", host_name.GetString() );
	event->SetString( "address", "127.0.0.1" ); // TODO
	event->SetInt(    "port", 27015 );				// TODO
	event->SetString( "game", com_gamedir );
	event->SetString( "mapname", mapname );
	event->SetString( "startdate", "yy.mm.dd" );	// TODO
	event->SetString( "starttime", "hh.mm.ss" );	// TODO
	event->SetInt(    "maxplayers", svs.maxclients );
#if _WIN32
	event->SetString("os", "WIN32" );
#else
	event->SetString("os", "LINUX" );
#endif
	event->SetInt( "dedicated", (cls.state == ca_dedicated) ? 1 : 0 );
	g_pGameEventManager->FireEvent( event, NULL );

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Send text to host_client
// Input  : *fmt -
//			... -
//-----------------------------------------------------------------------------
void SV_ClientPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	if ( host_client->fakeclient )
		return;

	va_start (argptr,fmt);
	Q_vsnprintf (string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	host_client->netchan.message.WriteByte( svc_print );
	host_client->netchan.message.WriteString( string );
}

//-----------------------------------------------------------------------------
// Purpose: Sends text to all active clients
// Input  : *fmt -
//			... -
//-----------------------------------------------------------------------------
void SV_BroadcastPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	int			i;
	client_t	*cl;

	va_start (argptr,fmt);
	Q_vsnprintf (string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	for (i=0, cl = &svs.clients[0] ; i<svs.maxclients ; i++, cl++)
	{
		if ( cl->fakeclient )
			continue;

		if ( !cl->active && !cl->spawned )
			continue;

		cl->netchan.message.WriteByte (svc_print);
		cl->netchan.message.WriteString (string);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stuff commands to host_client's console buffer
// Input  : *fmt -
//			... -
//-----------------------------------------------------------------------------
void SV_SendClientCommands (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	if ( host_client->fakeclient )
		return;

	va_start (argptr,fmt);
	Q_vsnprintf (string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	host_client->netchan.message.WriteByte (svc_stufftext);
	host_client->netchan.message.WriteString (string);
}

//-----------------------------------------------------------------------------
// Purpose: Drops client from server, with explanation
// Input  : *cl -
//			crash -
//			*fmt -
//			... -
//-----------------------------------------------------------------------------
void SV_DropClient (client_t *cl, qboolean crash, char *fmt, ... )
{
	int		i = 0;
	//client_t *client;
	byte	final[20];

	va_list		argptr;
	char		string[1024];

	va_start (argptr,fmt);
	Q_vsnprintf (string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	Assert( serverGameClients );

	if (!crash)
	{
		// add the disconnect
		if ( !cl->fakeclient )
		{
			cl->netchan.message.WriteByte (svc_disconnect);
			final[0] = svc_disconnect;
			i = 1;
		}

		if (cl->edict && cl->spawned)
		{
			SV_DisconnectClient( cl->edict );
		}

		Sys_Printf ("Dropped %s from server\nReason:  %s\n",cl->name, string );

		// Send the remaining reliable buffer so the client finds out the server is shutting down.
		if ( !cl->fakeclient )
		{
			Netchan_Transmit ( &cl->netchan, i, final );	// just update reliable
		}
	}

	// Throw away any residual garbage in the channel.
	Netchan_Clear( &cl->netchan );

// free the client (the body stays around)
	cl->active = false;
	cl->connected = false;
	cl->spawned = false;
	cl->name[0] = 0;
	cl->connection_started = realtime;
	cl->edict = NULL;

	memset(cl->userinfo, 0, sizeof(cl->userinfo));

// send notification to all other clients
	SV_FullClientUpdate (cl, &sv.reliable_datagram);
}

/*
==============================
SV_IsSimulating

==============================
*/
bool SV_IsSimulating( void )
{
	if ( !sv.paused )
	{
		if ( ( svs.maxclients > 1 ) 
#ifndef SWDS
			|| ( !Con_IsVisible() && !bugreporter->ShouldPause() && (cls.state == ca_active || cls.state == ca_dedicated) ) 
#endif
			)
		{
			return true;
		}
	}
	return false;
}


/*
==================
SV_Frame

==================
*/

void SV_Frame( bool send_client_updates )
{
	VPROF( "SV_Frame" );
	
	if ( !sv.active || !Host_ShouldRun() )
	{
		return;
	}

	g_ServerGlobalVariables.frametime = TICK_RATE;

	// Run any commands from client and play client Think functions if it is time.
	SV_ReadPackets();

	// Move other things around and think
	SV_Physics( SV_IsSimulating() );

	// Send the results of movement and physics to the clients
	if ( send_client_updates )
	{
		// This causes network messages to be sent
		SV_GameRenderDebugOverlays();
		SV_SendClientMessages();
	}

	// Send a heartbeat to the master if needed
	master->CheckHeartbeat ();
}


static ConCommand user( "user", SV_User_f, "Show user data." );
static ConCommand users( "users", SV_Users_f, "Show user info for players on server." );
static ConCommand maxplayers("maxplayers", SV_MaxPlayers_f, "Change the maximum number of players allowed on this server." );
