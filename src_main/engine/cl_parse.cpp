//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "client.h"
#include "cdll_int.h"
#include "sound.h"
#include "decal.h"
#include "host_cmd.h"
#include "voice.h"
#include "profile.h"
#include "tmessage.h"
#include "server.h"
#include "checksum_engine.h"
#include "vox.h"
#include "client_class.h"
#include "debugoverlay.h"
#include "r_local.h"
#include "host.h"
#include "vgui_int.h"
#include "r_efx.h"
#include "icliententitylist.h"
#include "cl_ents.h"
#include "iprediction.h"
#include "demo.h"
#include "NetworkStringTableClient.h"
#include "cdll_engine_int.h"
#include "FileSystem.h"
#include "FileSystem_Engine.h"
#include "soundflags.h"
#include "EngineSoundInternal.h"
#include "NetworkStringTableContainerClient.h"
#include "precache.h"
#include "const.h"
#include "icliententity.h"
#include "dt_recv_eng.h"
#include "cl_pred.h"
#include "proto_version.h"
#include "UtlRBTree.h"
#include "screen.h"
#include "vid.h"
#include "cl_parse_event.h"
#include "cl_ents_parse.h"
#include "vstdlib/icommandline.h"
#include "dt_common_eng.h"
#include "decal_private.h"
#include "gameeventmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int CL_UPDATE_BACKUP = SINGLEPLAYER_BACKUP;
int CL_UPDATE_MASK   = ( SINGLEPLAYER_BACKUP - 1 );

extern ConVar cl_captions;
extern ConVar cl_showmessages;
extern ConVar snd_show;

extern vrect_t		scr_vrect;

static int starting_count;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pHead - 
//			*pClassName - 
// Output : static ClientClass*
//-----------------------------------------------------------------------------
ClientClass* FindClientClass(ClientClass *pHead, const char *pClassName)
{
	ClientClass *pCur;

	for(pCur=pHead; pCur; pCur=pCur->m_pNext)
	{
		if(stricmp(pCur->m_pNetworkName, pClassName) == 0)
			return pCur;
	}

	return NULL;
}



/*
================
Called when the server sends data table info during signon (so we know how to
unpack data tables it'll be sending).
================
*/
void CL_ParseSendTableInfo(CClientState *pState, bf_read *pBuf)
{
	bool bNeedsDecoder = pBuf->ReadOneBit() != 0;
	if ( !RecvTable_RecvInfo( pBuf, bNeedsDecoder ) )
		Host_Error( "RecvTable_RecvInfo failed.\n" );
}

void CL_ParseClassInfo_NumClasses(CClientState *pState, bf_read *pBuf)
{
	if(pState->m_pServerClasses)
	{
		delete [] pState->m_pServerClasses;
	}

	pState->m_nServerClasses = pBuf->ReadShort();
	if(!(pState->m_pServerClasses = new C_ServerClassInfo[pState->m_nServerClasses]))
	{
		Host_EndGame("CL_ParseClassInfo: can't allocate %d C_ServerClassInfos.\n", pState->m_nServerClasses);
		return;
	}

	pState->m_nServerClassBits = Q_log2( pState->m_nServerClasses ) + 1;
	Assert( pState->m_nServerClassBits > 0 );
	// Make sure it's bounded!!!
	Assert( pState->m_nServerClassBits <= 16 );
}


void CL_ParseClassInfo_ClassData(CClientState *pState, bf_read *pBuf)
{
	int classID; 

	classID = pBuf->ReadUBitLong( pState->m_nServerClassBits );
	if(classID >= pState->m_nServerClasses)
	{
		Host_EndGame("CL_ParseClassInfo: invalid class index (%d).\n", classID);
		return;
	}

	pState->m_pServerClasses[classID].m_ClassName = pBuf->ReadAndAllocateString();
	pState->m_pServerClasses[classID].m_DatatableName = pBuf->ReadAndAllocateString();
}


void CL_ParseClassInfo_EndClasses(CClientState *pState )
{
	// Verify that we have received info about all classes.
	for ( int i=0; i < pState->m_nServerClasses; i++ )
	{
		if ( !pState->m_pServerClasses[i].m_DatatableName )
		{
			Host_EndGame("CL_ParseClassInfo_EndClasses: class %d not initialized.\n", i);
			return;
		}
	}

	ClientClass *pClasses = g_ClientDLL->GetAllClasses();

	// Match the server classes to the client classes.
	for ( i=0; i < pState->m_nServerClasses; i++ )
	{
		C_ServerClassInfo *pServerClass = &pState->m_pServerClasses[i];

		// (this can be null in which case we just use default behavior).
		pServerClass->m_pClientClass = FindClientClass(pClasses, pServerClass->m_ClassName);

		if ( pServerClass->m_pClientClass )
		{
			// If the class names match, then their datatables must match too.
			// It's ok if the client is missing a class that the server has. In that case,
			// if the server actually tries to use it, the client will bomb out.
			const char *pServerName = pServerClass->m_DatatableName;
			const char *pClientName = pServerClass->m_pClientClass->m_pRecvTable->GetName();

			if ( stricmp( pServerName, pClientName ) != 0 )
			{
				Host_EndGame( "CL_ParseClassInfo_EndClasses: server and client classes for '%s' use different datatables (server: %s, client: %s)",
					pServerClass->m_ClassName, pServerName, pClientName );
				
				return;
			}
		}
		else
		{
			Msg( "Client missing DT class %s\n", pServerClass->m_ClassName );
		}
	}

	// Now build decoders.
	EndGameAssertMsg( RecvTable_CreateDecoders(), ("CL_ParseClassInfo_EndClasses: CreateDecoders failed.\n") );
}

void CL_ParseClassInfo_CreateFromSendTables(CClientState *pState)
{
	// Create all of the send tables locally
	DataTable_CreateClientTablesFromServerTables();

	// Now create all of the server classes locally, too
	DataTable_CreateClientClassInfosFromServerClasses( pState );

	CL_ParseClassInfo_EndClasses( pState );
}

/*
================
Reads in the list of classes.
================
*/
void CL_ParseClassInfo(CClientState *pState, bf_read *pBuf)
{
	byte subPacket;
	
	
	subPacket = pBuf->ReadByte();
	if(subPacket == CLASSINFO_NUMCLASSES)
	{
		CL_ParseClassInfo_NumClasses(pState, pBuf);
	}
	else if(subPacket == CLASSINFO_CLASSDATA)
	{
		CL_ParseClassInfo_ClassData(pState, pBuf);
	}
	else if(subPacket == CLASSINFO_ENDCLASSES)
	{
		CL_ParseClassInfo_EndClasses(pState);
	}
	else if ( subPacket == CLASSINFO_CREATEFROMSENDTABLES)
	{
		CL_ParseClassInfo_CreateFromSendTables(pState);
	}
	else
	{
		Host_EndGame("CL_ParseClassInfo: invalid svc_classinfo subpacket (%d).\n", subPacket);
	}
}

int DispatchDirectUserMsg(const char *pszName, int iSize, void *pBuf )
{
	if ( !g_ClientDLL->DispatchUserMessage( pszName, iSize, pBuf ) )
	{
		Con_Printf( "Problem Direct Dispatching User Message %s\n", pszName );
		return 0;
	}
	return 1;
}

///////////////////////////////////////////////////
//
// SOUND
//
//-----------------------------------------------------------------------------
// Purpose: A queued up sound, which is in a tree sorted by sequence_number so
//  the engine plays the messages in exactly the order encoded, which can be 
//  different from the receiving order due to the ordering of reliable data before unreliable
//  in the server to client packets
//-----------------------------------------------------------------------------
struct SoundPacket
{
public:
	// For sorting
	int				sequence_number;

    Vector			pos;
    int				channel;
	int				ent;
    int 			sound_num;
    float 			volume;
    int 			field_mask;
    soundlevel_t	soundlevel;  
	int				pitch;
	float			delay;
};

//-----------------------------------------------------------------------------
// Purpose: Used for sorting sounds
// Input  : &sound1 - 
//			&sound2 - 
// Output : static bool
//-----------------------------------------------------------------------------
static bool CL_SoundPacketLessFunc( SoundPacket const &sound1, SoundPacket const &sound2 )
{
	return sound1.sequence_number < sound2.sequence_number;
}

static CUtlRBTree< SoundPacket, int > g_SoundPackets( 0, 0, CL_SoundPacketLessFunc );

//-----------------------------------------------------------------------------
// Purpose: Add sound to queue
// Input  : sound - 
//-----------------------------------------------------------------------------
void CL_AddSoundPacket( const SoundPacket& sound )
{
	g_SoundPackets.Insert( sound );
}

//-----------------------------------------------------------------------------
// Purpose: Play sound packet
// Input  : sound - 
//-----------------------------------------------------------------------------
void CL_DispatchSoundPacket( const SoundPacket& sound )
{
	CSfxTable *pSfx;

	if ( sound.field_mask & SND_SENTENCE )
	{
		char name[ MAX_QPATH ];
		const char *pSentenceName;
		
		// make dummy sfx for sentences
		pSentenceName = VOX_SentenceNameFromIndex( sound.sound_num );

		Q_snprintf( name, sizeof( name ), "!%s", pSentenceName );
		
		pSfx = S_DummySfx( name );
		
		if ( cl_captions.GetInt() )
		{
			CL_HudMessage( pSentenceName );
		}
	}
	else
	{
		pSfx = cl.GetSound( sound.sound_num );
	}

	if ( snd_show.GetInt() >= 2 )
	{
		DevMsg( "%i Dispatch Sound (seq %i) %s : src %d : channel %d : %d dB : vol %.2f : time %.3f (%.4f delay)\n", 
			host_framecount,
			sound.sequence_number,
			cl.GetSoundName( sound.sound_num ), 
			sound.ent, 
			sound.channel, 
			sound.soundlevel, 
			sound.volume, 
			cl.gettime(),
			sound.delay );
	}

	if ( sound.channel == CHAN_STATIC )
	{
		S_StartStaticSound(
			sound.ent, 
			CHAN_STATIC, 
			pSfx, 
			sound.pos, 
			sound.volume, 
			sound.soundlevel, 
			sound.field_mask, 
			sound.pitch,
			true,
			sound.delay );
	}
	else
	{
		// Don't actually play non-static sounds if playing a demo and skipping ahead
		if ( demo->IsPlayingBack() && 
			( demo->IsSkippingAhead() || demo->IsFastForwarding() ) )
		{
			return;
		}

		S_StartDynamicSound(
			sound.ent, 
			sound.channel, 
			pSfx, 
			sound.pos, 
			sound.volume, 
			sound.soundlevel, 
			sound.field_mask, 
			sound.pitch,
			true,
			sound.delay );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called after reading network messages to play sounds encoded in the network packet
//-----------------------------------------------------------------------------
void CL_DispatchSounds( void )
{
	int i;
	// Walk list in sequence order
	i = g_SoundPackets.FirstInorder();
	while ( i != g_SoundPackets.InvalidIndex() )
	{
		SoundPacket const *packet = &g_SoundPackets[ i ];
		Assert( packet );
		if ( packet )
		{
			// Play the sound
			CL_DispatchSoundPacket( *packet );
		}
		i = g_SoundPackets.NextInorder( i );
	}

	// Reset the queue each time we empty it!!!
	g_SoundPackets.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_ParseStartSoundPacket(void)
{
	SoundPacket sound;

	// Read sequence number for sorting
	sound.sequence_number = MSG_ReadBitLong( 8 );

	sound.field_mask = MSG_ReadBitLong( SND_FLAG_BITS_ENCODE ); 
    if ( sound.field_mask & SND_VOLUME )
	{
		// reduce back to 0.0 - 1.0 range
		sound.volume = (float)MSG_ReadBitByte ( 8 ) / 255.0;			
	}
	else
	{
		sound.volume = DEFAULT_SOUND_PACKET_VOLUME / 255.0;
	}
	
    if ( sound.field_mask & SND_SOUNDLEVEL )
	{
		sound.soundlevel = (soundlevel_t)MSG_ReadBitByte ( 8 );		// keep in dB range
	}
	else
	{
		sound.soundlevel = SNDLVL_NORM;
	}
	
	sound.channel		= MSG_ReadBitByte ( 3 );
	sound.ent			= MSG_ReadBitShort( MAX_EDICT_BITS );
	sound.sound_num		= MSG_ReadBitShort( MAX_SOUND_INDEX_BITS );

	sound.pos.x			= MSG_ReadBitCoord();
	sound.pos.y			= MSG_ReadBitCoord();
	sound.pos.z			= MSG_ReadBitCoord();

	if ( sound.field_mask & SND_PITCH )
	{
		sound.pitch = MSG_ReadBitByte ( 8 );
	}
	else
	{
		sound.pitch = DEFAULT_SOUND_PACKET_PITCH;
	}

	if ( sound.field_mask & SND_DELAY )
	{
		// 0 to 4096 msec
		sound.delay = (float)MSG_ReadSBitLong( MAX_SOUND_DELAY_MSEC_ENCODE_BITS ) / 1000.0f;
		if ( sound.delay < 0 )
		{
			sound.delay *= 10.0f;
		}
	}
	else
	{
		sound.delay = 0.0f;
	}

	// Add to sorted queue
	CL_AddSoundPacket( sound );
}       



//////////////////////////////////////////////////////
//
// USERINFO
//
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : crc - 
//-----------------------------------------------------------------------------
void CL_CheckForLogo( CRC32_t& crc )
{
	// It's blank/unset... don't bother asking for it.
	if ( !crc )
	{
		return;
	}

	// See if we already have this logo in our cache...
	char crcfilename[ 512 ];

	Q_snprintf( crcfilename, sizeof( crcfilename ), "materials/decals/downloads/%s.vtf",
		COM_BinPrintf( (byte *)&crc, 4 ) );

	if ( g_pFileSystem->FileExists( crcfilename ) )
	{
		return;
	}

	// Request logo file download from server
	cls.netchan.message.WriteByte( clc_sendlogo );
	cls.netchan.message.WriteLong( (unsigned int)crc );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void CL_UpdateUserinfo
//-----------------------------------------------------------------------------
void CL_UpdateUserinfo (void)
{
	int		slot;
	player_info_t	*player;

	bf_read *buf = MSG_GetReadBuf();
	Assert( buf );

	slot = buf->ReadUBitLong( MAX_CLIENT_BITS );
	if (slot >= MAX_CLIENTS)
	{
		Host_EndGame ("CL_ParseServerMessage: svc_updateuserinfo > MAX_CLIENTS");
	}

	player = &cl.players[slot];

	bool active = buf->ReadOneBit() ? true : false;
	if ( active )
	{
		MSG_ReadBuf( 16, player->hashedcdkey );

		Q_strncpy(player->userinfo, MSG_ReadString(), sizeof(player->userinfo));

		Q_strncpy(player->name, Info_ValueForKey (player->userinfo, "name"), sizeof(player->name));
		Q_strncpy(player->model, Info_ValueForKey (player->userinfo, "model"), sizeof(player->model));
		player->trackerID = Q_atoi(Info_ValueForKey (player->userinfo, "*tracker"));

		// Did the server tell us about a player logo?
		char const *s = Info_ValueForKey( player->userinfo, "*logo" );
		if ( s && s[ 0 ] )
		{	
			// Get CRC32_t from raw data
			COM_HexConvert( s, 8, (byte *)&player->logo );
			// Remove the start key
			Info_RemoveKey( player->userinfo, "*logo" );
			// See if we need to request a download from server.
			CL_CheckForLogo( player->logo );
		}
	}
	else
	{
		Q_memset( player, 0, sizeof( *player ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flag - 
// Output : static void
//-----------------------------------------------------------------------------
static void CL_RevertConVarsWithFlag( int flag )
{
	const ConCommandBase *var;
	for (var= ConCommandBase::GetCommands() ; var ; var=var->GetNext())
	{
		if ( var->IsCommand() )
			continue;

		ConVar *cvar = ( ConVar * )var;

		if ( !cvar->IsBitSet( flag ) )
			continue;

		// It's == to the default value, don't count
		if ( !Q_strcasecmp( cvar->GetDefault(), cvar->GetString() ) )
			continue;

		cvar->Revert();

		DevMsg( "%s = \"%s\" (reverted)\n", cvar->GetName(), cvar->GetString() );

	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_Parse_SetConVar( void )
{
	bf_read *buf = MSG_GetReadBuf();
	Assert( buf );

	bool revert_replicated_convars = buf->ReadByte() == 1 ? true : false;
	if ( revert_replicated_convars )
	{
		CL_RevertConVarsWithFlag( FCVAR_REPLICATED );
	}

	int count = (int)buf->ReadByte();
	if ( count <= 0 || count > 255 )
	{
		Host_EndGame( "CL_Parse_SetConVar:  bogus count %i\n", (int)count );
		return;
	}

	int i = 0;
	for ( i = 0; i < count; i++ )
	{
		char cvarname[ 256 ];
		if ( !buf->ReadString( cvarname, sizeof( cvarname ) ) )
		{
			Host_EndGame( "CL_Parse_SetConVar:  error reading convar name at convar # %i\n", i );
			return;
		}

		char value[ 256 ];
		if ( !buf->ReadString( value, sizeof( value ) ) )
		{
			Host_EndGame( "CL_Parse_SetConVar:  error reading convar value at convar # %i(%s)\n", i, cvarname );
			return;
		}

		// De-constify
		ConVar *var = ( ConVar * )cv->FindVar( cvarname );
		if ( !var )
		{
			Con_Printf( "CL_Parse_SetConVar:  No such cvar ( %s set to %s), skipping\n",
				cvarname, value );
			continue;
		}

		// Make sure server is only setting replicated ConVars
		if ( !var->IsBitSet( FCVAR_REPLICATED ) )
		{
			Con_Printf( "CL_Parse_SetConVar:  Can't set cvar %s to %s, not marked FCVAR_REPLICATED on client\n",
				cvarname, value );
			continue;		
		}

		// Set value directly ( don't call through cv->DirectSet!!! )
		var->SetValue( value );

		DevMsg( "%s = \"%s\"\n", cvarname, value );
	}
}

/*
==================
qboolean CL_CheckCRCs( char *pszMap )


==================
*/
qboolean CL_CheckCRCs( const char *pszMap )
{
	CRC32_t mapCRC;        // If this is the worldmap, CRC agains server's map
	CRC32_t clientDllCRC;  // Also, make sure client DLL is not hacked.
	char szDllName[MAX_QPATH];    // Client side DLL being used.

	// Don't verify CRC if we are running a local server (i.e., we are playing single player, or we are the server in multiplay
	if ( sv.active ) // Single player
		return true;

	CRC32_Init(&mapCRC);
	if (!CRC_MapFile( &mapCRC, pszMap ) )
	{
		// Does the file exist?
		FileHandle_t fp = 0;
		int nSize = -1;

		nSize = COM_OpenFile( pszMap, &fp );
		if ( fp )
			g_pFileSystem->Close( fp );

		if ( nSize != -1 )
		{
			COM_ExplainDisconnection( true, "Couldn't CRC map %s, disconnecting\n", pszMap);
		}
		else
		{   
			COM_ExplainDisconnection( true, "Missing map %s,  disconnecting\n", pszMap);
		}

		Host_Error( "Disconnected" );
		return false;
	}

	// Hacked map
	if ( cl.serverCRC != mapCRC && 
		!demo->IsPlayingBack())
	{
		COM_ExplainDisconnection( true, "Your map [%s] differs from the server's.\n", pszMap );
		Host_Error ("Disconnected");
		return false;
	}

	// Check to see that our copy of the client side dll matches the server's.
	// Client side DLL  CRC check.
	Q_snprintf( szDllName, sizeof( szDllName ), "bin\\client.dll");

	if (!CRC_File( &clientDllCRC, szDllName) && !demo->IsPlayingBack() )
	{
		COM_ExplainDisconnection( true, "Couldn't CRC client side dll %s.\n", szDllName);
		Host_Error( "Disconnected" );
		return false;
	}

#if !defined( _DEBUG )
	// These must match.
	// Except during demo playback.  For that just put a warning.
	if (cl.serverClientSideDllCRC != clientDllCRC)
	{
		if ( !demo->IsPlayingBack() )
		{
			Warning( "Your .dll [%s] differs from the server's.\n", szDllName );
			//COM_ExplainDisconnection( true, "Your .dll [%s] differs from the server's.\n", szDllName);
			//Host_Error( "Disconnected" );
			//return false;
		}
	}
#endif

	return true;
}

/*
==================
CL_RegisterResources

Clean up and move to next part of sequence.
==================
*/
void CL_RegisterResources ( void )
{
	// All done precaching.
	host_state.worldmodel = cl.GetModel( 1 );
	if ( !host_state.worldmodel )
	{
		Host_Error( "CL_RegisterResources:  host_state.worldmodel/cl.GetModel( 1 )==NULL\n" );
	}

	// Tell rendering system we have a new set of models.
	R_NewMap ();                    
	
	// Done with all resources, issue prespawn command.
	// Include server count in case server disconnects and changes level during d/l
	cls.netchan.message.WriteByte   (clc_stringcmd);
	cls.netchan.message.WriteString (va("prespawn %i", cl.servercount));
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nMaxClients - 
//-----------------------------------------------------------------------------
void CL_ReallocateDynamicData( void )
{
	int maxents = COM_EntsForPlayerSlots( cl.maxclients );

	// Describes the entity's networking data (the RecvTable is in the baseline).
	AllocateEntityBaselines( maxents );
	
	assert( entitylist );
	if ( entitylist )
	{
		entitylist->SetMaxEntities( maxents );
	}
}


/*
==================
CL_CheckGameDirectory

Client side game directory change.
==================
*/
void CL_CheckGameDirectory( char *gamedir )
{
	// Switch game directories if needed, or abort if it's not good.
	char szGD[ MAX_OSPATH ];

	if ( !gamedir || !gamedir[0] )
	{
		Con_Printf( "Server didn't specify a gamedir, assuming no change\n" );
		return;
	}

	// Rip out the current gamedir.
	COM_FileBase( com_gamedir, szGD );

	if ( stricmp( szGD, gamedir ) )
	{
		// Changing game directories without restarting is not permitted any more
		Error( "CL_CheckGameDirectory: game directories don't match (%s / %s)", szGD, gamedir );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Read in server info packet.
// Output : void CL_ParseServerInfo
//-----------------------------------------------------------------------------
void CL_ParseServerInfo (void)
{
	char	*str;
	char    *gamedir;
	int		i;

	Con_DPrintf ("Serverinfo packet received.\n");

	// Reset client state
	CL_ClearState();

	demo->SetWaitingForUpdate( false );	

	// parse protocol version number
	i = MSG_ReadShort();
	if ( i != PROTOCOL_VERSION )
	{
		Con_Printf ( "Server returned version %i, expected %i\n", i, PROTOCOL_VERSION );
		return;
	}

	// Parse servercount (i.e., # of servers spawned since server .exe started)
	// So that we can detect new server startup during download, etc.
	cl.servercount              = MSG_ReadLong();

	// Because a server doesn't run during
	//  demoplayback, but the decal system relies on this...
	if ( demo->IsPlayingBack() )
	{
		cl.servercount = gHostSpawnCount;    
	}

	// The CRC of the server map must match the CRC of the client map. or else
	//  the client is probably cheating.
	cl.serverCRC                = MSG_ReadLong();
	// The client side DLL CRC check.
	cl.serverClientSideDllCRC   = MSG_ReadLong();

	cl.maxclients               = MSG_ReadByte ();

	if ( cl.maxclients < 1 || cl.maxclients > MAX_CLIENTS )
	{
		Host_Error("Bad maxclients (%u) from server\n", cl.maxclients);
		return;
	}

	// Multiplayer game?
	if ( cl.maxclients > 1 )	
	{
		if ( mp_decals.GetInt() < r_decals.GetInt() )
		{
			r_decals.SetValue( mp_decals.GetInt() );
		}
	}

	g_ClientGlobalVariables.maxClients = cl.maxclients;

	CL_ReallocateDynamicData();

	CL_UPDATE_BACKUP = ( cl.maxclients == 1 ) ? SINGLEPLAYER_BACKUP : MULTIPLAYER_BACKUP;
	CL_UPDATE_MASK   = ( CL_UPDATE_BACKUP - 1 );

	// Clear the client frames, if they are left over, that is...
	memset( cl.frames, 0, MULTIPLAYER_BACKUP * sizeof( frame_t ) );

	// Re-map parsecountmod if mask changed
	cl.parsecountmod &= CL_UPDATE_MASK;

	cl.playernum = MSG_ReadByte ();

	gamedir	= MSG_ReadString();

	CL_CheckGameDirectory( gamedir );

	// Re-init hud video, especially if we changed game directories
	ClientDLL_HudVidInit();

	str							= MSG_ReadString ();
	Q_strncpy( cl.levelname, str, sizeof( cl.levelname ) );

	str							= MSG_ReadString();
	ConVar *skyname = ( ConVar * )cv->FindVar( "sv_skyname" );
	if ( skyname )
	{
		skyname->SetValue( str );
	}

	// read server game event system CRC, 0 if no system used
	CRC32_t	serverGameEventsCRC = MSG_ReadLong();

	if ( serverGameEventsCRC != g_pGameEventManager->GetEventTableCRC() )
	{
		if ( serverGameEventsCRC == 0 )
		{
			g_pGameEventManager->Reset();	// disable game event system
		}
		else
		{
			Con_Printf ( "Server uses different game event table (%u).\n", serverGameEventsCRC );
			return;
		}
	}

	

	// Verify the map and player .mdl crc's before requesting rest of signon
	if ( !CL_CheckCRCs( cl.levelname ) )
	{
		Host_Error( "Unabled to verify map %s\n", ( cl.levelname && cl.levelname[0] ) ? cl.levelname : "unknown" );
		return;
	}

	gHostSpawnCount = cl.servercount;
	
	// During a level transition the client remained active which could cause problems.
	// knock it back down to 'connected'
	cls.state = ca_connected;
}

//-----------------------------------------------------------------------------
// Purpose: Parses an entity baseline.
// Input  : *pReadBuf - 
// Output : void CL_ParseSpawnBaseline
//-----------------------------------------------------------------------------
void CL_ParseSpawnBaseline ( bf_read *pReadBuf )
{
	int entnum = MSG_ReadBitLong( 10 );
	PackedEntity *baseline = GetStaticBaseline( entnum );
	Assert( baseline );

	Assert( cl.m_nServerClassBits > 0 && cl.m_nServerClassBits <= 16 );

	int classID = pReadBuf->ReadUBitLong( cl.m_nServerClassBits );
	EndGameAssertMsg( classID < cl.m_nServerClasses, ("CL_ParseBaseline: invalid classID (%d).\n", classID) );

	C_ServerClassInfo *pServerClass = &cl.m_pServerClasses[classID];
	EndGameAssertMsg( pServerClass->m_pClientClass, ("CL_ParseBaseline: no matching client class for %s.\n", (const char*)pServerClass->m_ClassName) );

	baseline->m_pRecvTable = pServerClass->m_pClientClass->m_pRecvTable;
	EndGameAssertMsg( baseline->m_pRecvTable, ("CL_ParseBaseline: missing decoder for %s.\n", (const char*)pServerClass->m_ClassName) );

	// Make space for the baseline data.
	char packedData[MAX_PACKEDENTITY_DATA];
	bf_write writeBuf( "CL_ParseSpawnBaseline", packedData, sizeof( packedData ) );
	RecvTable_CopyEncoding( baseline->m_pRecvTable, pReadBuf, &writeBuf, -1 );

	EndGameAssertMsg( !writeBuf.IsOverflowed(), ("Baseline for RecvTable %s overflowed.", baseline->m_pRecvTable->GetName()) );

	// Copy out the data we just decoded.
	baseline->AllocAndCopyPadded( packedData, writeBuf.GetNumBytesWritten(), &g_ClientPackedDataAllocator );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *frame - 
//-----------------------------------------------------------------------------
void CL_ResetFrameStats( frame_t *frame )
{
	frame->clientbytes = 0;
	frame->packetentitybytes = 0;
	frame->localplayerinfobytes   = 0;
	frame->otherplayerinfobytes   = 0;
	frame->soundbytes   = 0;
	frame->usrbytes = 0;
	frame->eventbytes = 0;
	frame->msgbytes = 0;
	frame->voicebytes = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Get rid of network profiling data for frames when we aren't fully connected
// Input  : *frame - 
//-----------------------------------------------------------------------------
void CL_ResetFrame( frame_t *frame )
{
	frame->invalid = false;
	frame->choked = false;
	frame->latency = 0.0;
	frame->receivedtime = realtime;          // Time now that we are parsing.
	frame->time = cl.mtime[ 0 ];

	CL_ResetFrameStats( frame );
}

//-----------------------------------------------------------------------------
// Purpose: Called after all network messages have been read from the network stream
//-----------------------------------------------------------------------------
void CL_PostReadMessages( void )
{
	// Did we get any messages this tick (i.e., did we call PreEntityPacketReceived)?
	if ( !cl.last_entity_message_received_tick ||
		cl.last_entity_message_received_tick != host_tickcount )
		return;

	// How many commands total did we run this frame
	int commands_acknowledged = cls.netchan.incoming_acknowledged - cl.last_command_ack;

//	COM_Log( "cl.log", "Server ack'd %i commands this frame\n", commands_acknowledged );

	//Msg( "%i/%i CL_PostReadMessages:  last ack %i most recent %i acked %i commands\n", 
	//	host_framecount, cl.tickcount,
	//	cl.last_command_ack, 
	//	cls.netchan.outgoing_sequence - 1,
	//	commands_acknowledged );

	// Highest command parsed from messages
	cl.last_command_ack	= cls.netchan.incoming_acknowledged;
	// Highest world state we know about, will be base for next round of predictions
	cl.last_prediction_start_state = cls.netchan.incoming_sequence;

	// Let prediction copy off pristine data and report any errors, etc.
	g_pClientSidePrediction->PostNetworkDataReceived( commands_acknowledged );
}

/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata ( void )
{
	int			i, j;
	int			command_ack;

	float		latency;   // Our latency (round trip message time to server and back)
	frame_t		*frame;    // The frame we are parsing in.

	// This is the last movement that the server ack'd
	command_ack = cls.netchan.incoming_acknowledged; 

	// Zero latency!!! (single player or listen server?)
	if ( cls.lastoutgoingcommand == command_ack &&
		!CL_HasRunPredictionThisFrame() )
	{
		//Msg( "%i/%i CL_ParseClientdata:  no latency server ack %i\n", 
		//	host_framecount, cl.tickcount,
		//	command_ack );
		CL_RunPrediction( PREDICTION_SIMULATION_RESULTS_ARRIVING_ON_SEND_FRAME );
	}

	// This is the frame update that this message corresponds to
	i = cls.netchan.incoming_sequence;  

	// Did we drop some frames?
	if ( i > cl.last_incoming_sequence + 1 )
	{
		// Mark as dropped
		for ( j = cl.last_incoming_sequence + 1;
		      j < i; 
			  j++ )
		{
			if ( cl.frames[j & CL_UPDATE_MASK ].receivedtime >= 0.0 )
			{
				cl.frames[j & CL_UPDATE_MASK ].receivedtime = -1;
				cl.frames[j & CL_UPDATE_MASK ].latency = 0;
			}
		}
	}

	// Ack'd incoming messages.
	cl.parsecount = i;  
	// Index into window.
	cl.parsecountmod = cl.parsecount & CL_UPDATE_MASK;     
	 // Frame at index.
	frame = &cl.frames[ cl.parsecountmod ];
	
	// Mark network received time
	frame->time = cl.mtime[0];

	// Send time for that frame.
	cl.parsecounttime = cl.commands[ command_ack & CL_UPDATE_MASK ].senttime;  

	// Current time that we got a response to the command packet.
	cl.commands[ command_ack & CL_UPDATE_MASK ].receivedtime = realtime;    

	// Time now that we are parsing.
	frame->receivedtime = realtime;          

	CL_ResetFrameStats( frame );

	// We've received entity data this frame
	cl.last_entity_message_received_tick = host_tickcount;

	// Copy some results from prediction back into right spot
	// Anything not sent over the network from server to client must be specified here.
	//if ( cl.last_command_ack  )
	{
		int number_of_commands_executed = ( command_ack - cl.last_command_ack );

#if 0
		COM_Log( "cl.log", "Receiving frame acknowledging %i commands\n",
			number_of_commands_executed );

		COM_Log( "cl.log", "  last command number executed %i(%i)\n",
			command_ack, command_ack & CL_UPDATE_MASK );

		COM_Log( "cl.log", "  previous last command number executed %i(%i)\n",
			cl.last_command_ack, cl.last_command_ack & CL_UPDATE_MASK );

		COM_Log( "cl.log", "  current world frame %i(%i)\n",
			cl.parsecount, cl.parsecountmod );

		COM_Log( "cl.log", "  previous world frame %i(%i)\n",
			cl.last_incoming_sequence, cl.last_incoming_sequence & CL_UPDATE_MASK );
#endif

		// Copy last set of changes right into current frame.
		g_pClientSidePrediction->PreEntityPacketReceived( number_of_commands_executed, cl.parsecount );
	}

	// Fixme, do this after all packets read for this frame?
	cl.last_incoming_sequence	= cls.netchan.incoming_sequence;

	if ( demo->IsPlayingBack() )
	{
		frame->latency = 0.0f;
	}
	else
	{
		// Calculate latency of this frame.
		//  Sent time is set when usercmd is sent to server in CL_Move
		//  this is the # of seconds the round trip took.
		latency = realtime - cl.parsecounttime;

		// Fill into frame latency
		frame->latency = latency;

		// Negative latency makes no sense.  Huge latency is a problem.
		if ( latency >= 0 && latency <= 2.0)
		{
			// drift the average latency towards the observed latency
			// If round trip was fastest so far, just use that for latency value
			// Otherwise, move in 1 ms steps toward observed channel latency.
			if ( latency < cls.latency )
			{
				cls.latency = latency;
			}
			else
			{
				// drift up, so corrections are needed
				cls.latency += 0.001f;	
			}
		}	
	}
}


// extern ConVar r_decals;

//-----------------------------------------------------------------------------
// Purpose: Parses a temporary entity from the network stream
//-----------------------------------------------------------------------------
void CL_ParseBSPDecal (void)
{
	Vector		pos;
	QAngle		angles;
	int			modelIndex;
	int			decalTextureIndex;
	int			flags;
	int			entnumber;
	model_t		*model;

	MSG_ReadBitVec3Coord(pos);
	modelIndex = 0;		// Apply to the world

	flags = FDECAL_PERMANENT;
	decalTextureIndex = MSG_ReadBitLong( MAX_DECAL_INDEX_BITS );		// Read decal type

	entnumber = MSG_ReadBitLong( MAX_EDICT_BITS );// Read entity index
	if ( entnumber  )
	{
		modelIndex = MSG_ReadBitLong( SP_MODEL_INDEX_BITS );	// If not the world, read brush model index
		model = cl.GetModel( modelIndex );
	}
	else
	{
		model = host_state.worldmodel;
	}

	if (r_decals.GetInt())
	{
		g_pEfx->DecalShoot( 
			decalTextureIndex, 
			entnumber, 
			model, 
			vec3_origin, 
			vec3_angle,
			pos, 
			NULL, 
			flags );
	}
}

/*
===================
CL_ParseStaticSound

===================
*/
void CL_ParseStaticSound (void)
{
	Vector		org;
	int			sound_num;
	float		vol;
	soundlevel_t soundlevel;
	int			ent;
	int			pitch;
	int			flags;
	CSfxTable	*pSfx;
	float		delay = 0.0f;

	flags = MSG_ReadBitLong( SND_FLAG_BITS_ENCODE );

	org.x = MSG_ReadBitCoord();
	org.y = MSG_ReadBitCoord();
	org.z = MSG_ReadBitCoord();

	sound_num = MSG_ReadBitLong( MAX_SOUND_INDEX_BITS );
	if ( flags & SND_VOLUME )
	{
		vol = MSG_ReadByte () / 255.0;		// reduce back to 0.0 - 1.0 range
	}
	else
	{
		vol = DEFAULT_SOUND_PACKET_VOLUME;
	}
	if ( flags & SND_SOUNDLEVEL )
	{
		soundlevel = (soundlevel_t)MSG_ReadByte();		// keep in dB range
	}
	else
	{
		soundlevel = SNDLVL_NORM;
	}
	ent = MSG_ReadBitLong( MAX_EDICT_BITS );
	if ( flags & SND_PITCH )
	{
		pitch = MSG_ReadByte();
	}
	else
	{
		pitch = DEFAULT_SOUND_PACKET_PITCH;
	}

	if (flags & SND_DELAY )
	{
		// Up to 4096 msec delay
		delay = (float)MSG_ReadSBitLong( MAX_SOUND_DELAY_MSEC_ENCODE_BITS ) / 1000.0f;
		if ( delay < 0 )
		{
			delay *= 10.0f;
		}
	}

	if (flags & SND_SENTENCE)
	{
		char name[ MAX_QPATH ];
		// make dummy sfx for sentences
		strcpy(name, "!");
		strcat(name, VOX_SentenceNameFromIndex( sound_num ));
		pSfx = S_DummySfx( name );
	}
	else
	{
		pSfx = cl.GetSound( sound_num );
	}

	if ( pSfx )
	{
		S_StartStaticSound( ent, CHAN_STATIC, pSfx, org, vol, soundlevel, flags, pitch, true, delay );
	}
}

/*
===============
CL_ParseSoundFade

===============
*/
void CL_ParseSoundFade()
{
	float percent;
	float inTime, holdTime, outTime;

	percent		= MSG_ReadFloat();
	holdTime	= MSG_ReadFloat();
	inTime		=  MSG_ReadFloat();
	outTime		=  MSG_ReadFloat();
	
	S_SoundFade( percent, holdTime, outTime, inTime );
}


//------------------ Copyright (c) 1999 Valve, LLC -----------------------
//Author  : DSpeyrer
//Purpose : Parses incoming voice data, submitting it to the sound engine.
//------------------------------------------------------------------------
void CL_ParseVoiceData(void)
{
	int iEntity, nDataLength, nChannel;
	char chReceived[8192];

	int startReadCount = MSG_GetReadBuf()->GetNumBytesRead();

	iEntity = MSG_ReadByte() + 1;
	nDataLength = MSG_ReadShort();
	nDataLength = min(nDataLength, sizeof(chReceived));
	MSG_ReadBuf( nDataLength, chReceived );

	cl.frames[cl.parsecountmod].voicebytes += MSG_GetReadBuf()->GetNumBytesRead() - startReadCount;

	if(iEntity == (cl.playernum+1))
	{
		Voice_LocalPlayerTalkingAck();
	}

	// Data length can be zero when the server is just acking a client's voice data.
	if(nDataLength == 0)
		return;

	// Have we already initialized the channels for this guy?
	nChannel = Voice_GetChannel(iEntity);
	if(nChannel == VOICE_CHANNEL_ERROR)
	{
		// Create a channel in the voice engine and a channel in the sound engine for this guy.
		nChannel = Voice_AssignChannel(iEntity);
		if (nChannel == VOICE_CHANNEL_ERROR)
		{
			Con_DPrintf("CL_ParseVoiceData: Voice_AssignChannel failed for client %d!\n", iEntity-1);
			return;
		}
	}

	if (nDataLength != 0)
	{
		// Give the voice engine the data (it in turn gives it to the mixer for the sound engine).
		Voice_AddIncomingData(nChannel, chReceived, nDataLength, cls.netchan.incoming_sequence);
	}
}


/*
===============
CL_Restore

Restores a saved game.
===============
*/
void CL_Restore( const char *fileName )
{
	int				i,mapCount;

	saverestore->RestoreClientState( fileName, false );

	mapCount = MSG_ReadByte();
	for ( i = 0; i < mapCount; i++ )
	{
		char mapname[ 256 ];
		Q_strncpy( mapname, MSG_ReadString(), sizeof( mapname ) );

		saverestore->RestoreAdjacenClientState( mapname );
		// JAY UNDONE:  Actually load decals that transferred through the transition!!!
		//Con_Printf("Loading decals from %s\n", pMapName );
	}

	saverestore->OnFinishedClientRestore();
}

//-----------------------------------------------------------------------------
// Purpose: Read in data from debug overlay and add to overlay text
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CL_ParseDebugGridOverlay()
{
	Vector origin;
	MSG_ReadBitVec3Coord(origin);
	CDebugOverlay::AddGridOverlay(origin);
}

//-----------------------------------------------------------------------------
// Purpose: Read in data from debug overlay and add to overlay text
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CL_ParseDebugTextOverlay()
{
	Vector origin;
	MSG_ReadBitVec3Coord(origin);
	float	duration =	MSG_ReadFloat();
	CDebugOverlay::AddTextOverlay(origin,duration,MSG_ReadString());
}

//-----------------------------------------------------------------------------
// Purpose: Read in data from debug overlay and add to overlay text
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CL_ParseDebugScreenTextOverlay()
{
	float	flXPos		=	MSG_ReadFloat();
	float	flYPos		=	MSG_ReadFloat();
	float	duration	=	MSG_ReadFloat();
	int		r			=	MSG_ReadShort();
	int		g			=	MSG_ReadShort();
	int		b			=	MSG_ReadShort();
	int		a			=	MSG_ReadShort();
	CDebugOverlay::AddScreenTextOverlay(flXPos,flYPos,duration,r,g,b,a,MSG_ReadString());
}

//-----------------------------------------------------------------------------
// Purpose: Read in data from debug overlay and add to overlay text
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CL_ParseDebugEntityTextOverlay()
{
	int		ent_index	=	MSG_ReadShort();
	int		text_offset =	MSG_ReadShort();
	float	duration	=	MSG_ReadFloat();
	int		r			=	MSG_ReadShort();
	int		g			=	MSG_ReadShort();
	int		b			=	MSG_ReadShort();
	int		a			=	MSG_ReadShort();
	CDebugOverlay::AddEntityTextOverlay(ent_index,text_offset,duration,r,g,b,a,MSG_ReadString());
}


//-----------------------------------------------------------------------------
// Purpose: Read in data from debug box overlay 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CL_ParseDebugBoxOverlay()
{
	Vector origin;
	Vector mins;
	Vector maxs;
	QAngle angles;
	MSG_ReadBitVec3Coord(origin);
	MSG_ReadBitVec3Coord(mins);
	MSG_ReadBitVec3Coord(maxs);
	MSG_ReadBitAngles(angles);
	int		r =			MSG_ReadShort();
	int		g =			MSG_ReadShort();
	int		b =			MSG_ReadShort();
	int		a =			MSG_ReadShort();
	float	duration =	MSG_ReadFloat();
	CDebugOverlay::AddBoxOverlay(origin,mins,maxs,angles,r,g,b,a,duration);
}

//-----------------------------------------------------------------------------
// Purpose: Read in data from debug line overlay 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CL_ParseDebugLineOverlay()
{
	Vector origin;
	Vector dest;
	MSG_ReadBitVec3Coord(origin);
	MSG_ReadBitVec3Coord(dest);
	int		r		=	MSG_ReadShort();
	int		g		=	MSG_ReadShort();
	int		b		=	MSG_ReadShort();
	int		noDepth	=	MSG_ReadShort();
	float	duration =	MSG_ReadFloat();
	CDebugOverlay::AddLineOverlay(origin,dest,r,g,b,noDepth ? true : false,duration);
}

//-----------------------------------------------------------------------------
// Purpose: Read in data from debug triangle overlay 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CL_ParseDebugTriangleOverlay()
{
	Vector p1, p2, p3;
	MSG_ReadBitVec3Coord(p1);
	MSG_ReadBitVec3Coord(p2);
	MSG_ReadBitVec3Coord(p3);
	int		r		=	MSG_ReadShort();
	int		g		=	MSG_ReadShort();
	int		b		=	MSG_ReadShort();
	int		a		=	MSG_ReadShort();
	int		noDepth	=	MSG_ReadShort();
	float	duration =	MSG_ReadFloat();
	CDebugOverlay::AddTriangleOverlay(p1, p2, p3, r, g, b, a, noDepth ? true : false, duration );
}

void CL_Parse_Disconnect( void )
{
	SCR_EndLoadingPlaque();
	Host_EndGame ("Server disconnected\n");
}

void CL_Parse_SetView( void )
{
	cl.viewentity = MSG_ReadShort ();
}

void CL_Parse_Time( void )
{
	int ticks = MSG_ReadLong();

	// Get server time stamp
	cl.mtime[1] = cl.mtime[0];
	cl.mtime[0] = ticks * TICK_RATE;

	// Single player == keep clocks in sync
	cl.tickcount = ticks;
	g_ClientGlobalVariables.tickcount = cl.tickcount;
	g_ClientGlobalVariables.curtime = cl.gettime();

	// Don't let client compute a "negative" frame time based on cl.tickcount lagging cl.oldtickcount
	if ( cl.tickcount < cl.oldtickcount )
	{
		cl.oldtickcount = cl.tickcount;
	}
	g_ClientGlobalVariables.frametime = cl.getframetime();
}



void CL_Parse_Print( void )
{
	Con_Print ( MSG_ReadString () );
}

void CL_Parse_CenterPrint( void )
{
	SCR_CenterPrint (MSG_ReadString ());
}

void CL_Parse_StuffText( void )
{
	char *s;

	s = MSG_ReadString();

	Cbuf_AddText ( s );
}

void CL_Parse_ServerInfo( void )
{
	CL_ParseServerInfo ();
	vid.recalc_refdef = true;	// leave intermission full screen	
}

void CL_Parse_SetAngle( void )
{
	int i;

	for (i=0 ; i<3 ; i++)
	{
		cl.viewangles[i] = MSG_ReadHiresAngle ();

		// Clamp between -180 and 180
		if (cl.viewangles[i]>180)
		{
			cl.viewangles[i] -= 360;
		}
	}
}

void CL_Parse_AddAngle( void )
{
	float ang_total;
	float ang_final;

	ang_total = MSG_ReadHiresAngle();
	ang_final = MSG_ReadHiresAngle();

	if ( ang_total > 180.0f )
	{
		ang_total -= 360.0f;
	}
	if ( ang_final > 180.0f )
	{
		ang_final -= 360.0f;
	}

	float apply_now = ang_total - ang_final;

	AddAngle a;
	a.yawdelta = ang_final;
	a.starttime = cl.gettime();
	a.accum = 0.0f;

	cl.addangle.AddToTail( a );

	cl.viewangles[1] += apply_now;
}

void CL_Parse_CrosshairAngle( void )
{
	QAngle angle( 0, 0, 0 );
	angle[ PITCH ] = MSG_ReadChar() * 0.2;
	angle[ YAW ]	= MSG_ReadChar() * 0.2;

	g_ClientDLL->SetCrosshairAngle( angle );
}

void CL_Parse_LightStyle( void )
{
	int readall = MSG_ReadOneBit();
	if ( readall )
	{
		for ( int i = 0; i < MAX_LIGHTSTYLES; i++ )
		{
			cl_lightstyle[i].map[ 0 ] = 0;

			if ( MSG_ReadOneBit() )
			{
				Q_strcpy (cl_lightstyle[i].map,  MSG_ReadString());
			}
			cl_lightstyle[i].length = Q_strlen(cl_lightstyle[i].map);
		}
	}
	else
	{
		int i = MSG_ReadBitLong( MAX_LIGHTSTYLE_INDEX_BITS );
		if (i >= MAX_LIGHTSTYLES)
		{
			Sys_Error ("svc_lightstyle(%i) > MAX_LIGHTSTYLES", i );
		}
		Q_strcpy (cl_lightstyle[i].map,  MSG_ReadString());
		cl_lightstyle[i].length = Q_strlen(cl_lightstyle[i].map);
	}
}

void CL_Parse_StopSound( void )
{
	int i;
	i = MSG_ReadShort();
	S_StopSound(i>>3, i&7);
}


void CL_Parse_SpawnBaseline( void )
{
	CL_ParseSpawnBaseline( MSG_GetReadBuf() );
}

void CL_Parse_SetPause( void )
{
	cl.paused = MSG_ReadByte ();
	if (cl.paused)
	{
		Cbuf_AddText( "cd pause\n" );
	}
	else
	{
		Cbuf_AddText( "cd resume\n" );
	}
}

void CL_Parse_SignonNum( void )
{
	int i;
	i = MSG_ReadByte ();
	if (i <= cls.signon)
	{
		Con_Printf ("Received signon %i when at %i\n", i, cls.signon);
		Host_Disconnect();
		return;
	}
	cls.signon = i;
	CL_SignonReply ();
}

void CL_Parse_CDTrack( void )
{
	cl.cdtrack = MSG_ReadByte ();

	if ( CommandLine()->FindParm("-nocdaudio") )
	{
		return;
	}

	Cbuf_AddText( va( "cd loop %i\n", cl.cdtrack ) );
}

void CL_Parse_Restore( void )
{
	CL_Restore( MSG_ReadString() );
}

void CL_Parse_RoomType( void )
{		
	extern ConVar dsp_room;
	dsp_room.SetValue( (float)MSG_ReadShort() );
}

void CL_Parse_Finale( void )
{
	vid.recalc_refdef = true;	// go to full screen
	SCR_CenterPrint (MSG_ReadString ());			
}

void CL_Parse_CutScene( void )
{
	vid.recalc_refdef = true;	// go to full screen
	SCR_CenterPrint (MSG_ReadString ());			
}

void CL_Parse_Choke( void )
{
	cl.frames[ cls.netchan.incoming_sequence & CL_UPDATE_MASK ].choked = true;
}

void CL_Parse_SkippedUpdate( void )
{
	int i;

	i = MSG_ReadByte(); // Skipped frames
	if ( cl.frames[ i & CL_UPDATE_MASK ].receivedtime == -1 ||
		 cl.frames[ i & CL_UPDATE_MASK ].receivedtime == -2 )
	{
		cl.frames[ i & CL_UPDATE_MASK ].receivedtime = -3;
	}
}

void CL_Parse_SendTable( void )
{
	CL_ParseSendTableInfo(&cl, MSG_GetReadBuf());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_Parse_ClassInfo( void )
{
	CL_ParseClassInfo(&cl, MSG_GetReadBuf());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_Parse_VoiceInit()
{
	char *pCodec = MSG_ReadString();
	if(pCodec[0] == 0)
	{
		Voice_Deinit();
	}
	else
	{
		Voice_Init(pCodec);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_Parse_EntityMessage( void )
{
	int		entnum = MSG_ReadBitLong( MAX_EDICT_BITS );
	char	messagename[ 128 ];
	
	Q_strncpy( messagename, MSG_ReadString(), sizeof( messagename ) );

	int		length = MSG_ReadByte();

	if ( length > 256 )
	{
		Host_Error( "CL_Parse_EntityMessage with bogus buffer size %i\n", length );
		return;
	}
		     
	// Route message to entity.
	// Gotta do this even if the entity doesn't exist so the next message is read ok
	byte tempBuf[256];
	MSG_ReadBuf( length, tempBuf );
	   
	// Look up entity
	IClientNetworkable *entity = entitylist->GetClientNetworkable( entnum );
	if ( entity )
	{
		// route to entity
		entity->ReceiveMessage( messagename, length, tempBuf );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server is requesting upload of logo file from our cache
//-----------------------------------------------------------------------------
void CL_ParseSendLogo( void )
{
	CRC32_t crc;
	crc = (CRC32_t)MSG_ReadLong();

	// Find file and send it
	char crcfilename[ 512 ];

	Q_snprintf( crcfilename, sizeof( crcfilename ), "materials/decals/downloads/%s.vtf",
		COM_BinPrintf( (byte *)&crc, 4 ) );

	// We don't have it, so just ignore
	if ( !g_pFileSystem->FileExists( crcfilename ) )
	{
		Con_Printf( "Can't upload %s, file missing!\n", crcfilename );
		return;
	}

	// Fragment and send the file.
	Netchan_CreateFileFragments( false, &cls.netchan, crcfilename );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//			bufEnd - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CL_ParseUserMessage( void )
{
	static char buf[MAX_USER_MSG_DATA];

	int bufStart = MSG_GetReadBuf()->GetNumBytesRead();

	// Try to read it int
	byte msg_type = MSG_ReadByte();

	int MsgSize = 0;

	char msgname[ 256 ];

	if ( !g_ClientDLL->GetUserMessageInfo( msg_type, msgname, MsgSize ) )
	{
		Host_Error( "Unable to find user message for index %i\n", msg_type );
		return;
	}

	if ( MsgSize == -1 )			// variable-length message
	{
		MsgSize = MSG_ReadByte();  // extract the length of the variable-length message from the stream
	}
		
	if ( MsgSize > MAX_USER_MSG_DATA )
	{
		Host_Error ("DispatchUserMsg:  User Msg %s/%d sent too much data (%i bytes), %i bytes max.\n",
			msgname, msg_type, MsgSize, MAX_USER_MSG_DATA );
	}

	MSG_ReadBuf( MsgSize, &buf );

	if ( !g_ClientDLL->DispatchUserMessage( msgname, MsgSize, (void *)&buf ) )
	{
		Host_Error ( "Problem Dispatching User Message %i:%s\n", msg_type, msgname );
	}

	int bufEnd = MSG_GetReadBuf()->GetNumBytesRead();
	
	cl.frames[ cl.parsecountmod ].usrbytes += (bufEnd - bufStart);

	if ( cl_showmessages.GetInt() )
	{
		Con_Printf( "Usr Msg %i:%s, offset(%i)\n", msg_type, msgname, bufStart );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//			bufEnd - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CL_Parse_GameEvent( void )
{
	g_pGameEventManager->ParseGameEvent( MSG_GetReadBuf() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
typedef struct svc_func_s
{
	unsigned char opcode;  // Opcode
	char *pszname;         // Display Name
	void ( *pfnParse )( void );    // Parse function
} svc_func_t;

// Dispatch table
static svc_func_t cl_parsefuncs[] =
{
	{ svc_bad, "svc_bad", NULL },   // 0
	{ svc_nop, "svc_nop", NULL },   // 1
	{ svc_disconnect, "svc_disconnect", CL_Parse_Disconnect }, // 2
	{ svc_event, "svc_event", CL_Parse_Event },				   // 3
	{ 4, "(unused)svc_version", NULL },  		   // 4
	{ svc_setview, "svc_setview", CL_Parse_SetView },  		   // 5
	{ svc_sound, "svc_sound", CL_ParseStartSoundPacket },		// 6
	{ svc_time, "svc_time", CL_Parse_Time },				   // 7
	{ svc_print, "svc_print", CL_Parse_Print },				   // 8
	{ svc_stufftext, "svc_stufftext", CL_Parse_StuffText },	   // 9
	{ svc_setangle, "svc_setangle", CL_Parse_SetAngle },	   // 10
	{ svc_serverinfo, "svc_serverinfo", CL_Parse_ServerInfo }, // 11
	{ svc_lightstyle, "svc_lightstyle", CL_Parse_LightStyle }, // 12
	{ svc_updateuserinfo, "svc_updateuserinfo", CL_UpdateUserinfo }, // 13
	{ svc_event_reliable, "svc_event_reliable", CL_Parse_ReliableEvent }, // 14
	{ svc_clientdata, "svc_clientdata", CL_ParseClientdata }, // 15
	{ svc_stopsound, "svc_stopsound", CL_Parse_StopSound },    // 16
	{ svc_createstringtables, "svc_createstringtables", CL_ParseStringTables }, // 17
	{ svc_updatestringtable, "svc_updatestringtable", CL_ParseUpdateStringTable },       // 18
	{ svc_entitymessage, "svc_entitymessage", CL_Parse_EntityMessage }, // 19
	{ svc_spawnbaseline, "svc_spawnbaseline", CL_Parse_SpawnBaseline },	// 20
	{ svc_bspdecal, "svc_bspdecal", CL_ParseBSPDecal },  // 21
	{ svc_setpause, "svc_setpause", CL_Parse_SetPause },	   // 22
	{ svc_signonnum, "svc_signonnum", CL_Parse_SignonNum },    // 23
	{ svc_centerprint, "svc_centerprint", CL_Parse_CenterPrint }, // 24
	{ svc_spawnstaticsound, "svc_spawnstaticsound", CL_ParseStaticSound },  // 25
	{ svc_gameevent, "svc_gameevent", CL_Parse_GameEvent },  // 26
	{ svc_finale, "svc_finale", CL_Parse_Finale },  // 27
	{ svc_cdtrack, "svc_cdtrack", CL_Parse_CDTrack },  // 28
	{ svc_restore, "svc_restore", CL_Parse_Restore },  // 29
	{ svc_cutscene, "svc_cutscene", CL_Parse_CutScene },  // 30
	{ 31, "(unused)svc_decalname", NULL },  // 31
	{ svc_roomtype, "svc_roomtype", CL_Parse_RoomType },  // 32
	{ svc_addangle, "svc_addangle", CL_Parse_AddAngle },  // 33
	{ svc_usermessage, "svc_usermessage", CL_ParseUserMessage },  // 34
	{ svc_packetentities, "svc_packetentities", CL_Parse_PacketEntities },  // 35
	{ svc_deltapacketentities, "svc_deltapacketentities", CL_Parse_DeltaPacketEntities },  // 36
	{ svc_choke, "svc_choke", CL_Parse_Choke },  // 37
	{ svc_setconvar, "svc_setconvar", CL_Parse_SetConVar },  // 38
	{ svc_sendlogo, "svc_sendlogo", CL_ParseSendLogo },  // 39
	{ svc_crosshairangle, "svc_crosshairangle", CL_Parse_CrosshairAngle },  // 40
	{ svc_soundfade, "svc_soundfade", CL_ParseSoundFade },  // 41
	{ svc_skippedupdate, "svc_skippedupdate", CL_Parse_SkippedUpdate },  // 42
	{ svc_voicedata, "svc_voicedata", CL_ParseVoiceData },  // 43
	{ svc_sendtable, "svc_sendtable", CL_Parse_SendTable },  // 44
	{ svc_classinfo, "svc_classinfo", CL_Parse_ClassInfo }, // 45
	{ svc_debugentityoverlay, "svc_debugentityoverlay", CL_ParseDebugEntityTextOverlay }, // 46
	{ svc_debugboxoverlay, "svc_debugboxoverlay", CL_ParseDebugBoxOverlay },	   // 47
	{ svc_debuglineoverlay, "svc_debuglineoverlay",	CL_ParseDebugLineOverlay },   // 48
	{ svc_debugtextoverlay, "svc_debugtextoverlay",	CL_ParseDebugTextOverlay },  // 49
	{ svc_debuggridoverlay, "svc_debuggridoverlay",	CL_ParseDebugGridOverlay },  // 50
	{ svc_voiceinit, "svc_voiceinit", CL_Parse_VoiceInit }, // 51
	{ svc_debugscreentext, "svc_debugscreentext",	CL_ParseDebugScreenTextOverlay },  // 52
	{ svc_debugtriangleoverlay, "svc_debugtriangleoverlay",	CL_ParseDebugTriangleOverlay },   // 53
	{ (unsigned char)-1, "End of List", NULL }
};

//-----------------------------------------------------------------------------
// Purpose: Describe a command by index
// Input  : cmd - 
// Output : char
//-----------------------------------------------------------------------------
char *CL_MsgInfo( int cmd )
{
	static char sz[ 256 ];

	Q_strcpy( sz, "???" );

	if ( cmd <= SVC_LASTMSG )
	{
		Q_strcpy( sz, cl_parsefuncs[ cmd ].pszname );
	}
	else
	{
		Assert( 0 );
	}

	return sz;
}

#define CMD_COUNT	32   // Must be power of two
#define CMD_MASK	( CMD_COUNT - 1 )

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : current_count - 
//			*msg - 
//-----------------------------------------------------------------------------
void CL_WriteErrorMessage( int current_count, sizebuf_t *msg )
{
	FileHandle_t fp;
	char name[ MAX_OSPATH ];
	const char *buffer_file = "buffer.dat";

	Q_snprintf( name, sizeof( name ), "%s", buffer_file );
	COM_FixSlashes( name );
	COM_CreatePath( name );

	fp = g_pFileSystem->Open( name, "wb" );
	if ( !fp )
		return;

	g_pFileSystem->Write( &starting_count, sizeof( int ), fp );
	g_pFileSystem->Write( &current_count, sizeof( int ), fp );
	g_pFileSystem->Write( msg->data, msg->cursize, fp );
	g_pFileSystem->Close( fp );

	Con_Printf( "Wrote erroneous message to %s\n", buffer_file );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct CommandDispatchDebug
{
	typedef struct
	{
		int command;
		int	starting_offset;
		int frame_number;
	} oldcmd_t;

	oldcmd_t oldcmd[ CMD_COUNT ];   
	int currentcmd;
	bool parsing;
};

static CommandDispatchDebug g_CommandDispatchDebug;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_WriteMessageHistory( void )
{
	int i;
	int thecmd;

	if ( cls.state == ca_disconnected ||
		 cls.state == ca_dedicated )
	{
		return;
	}

	if ( !g_CommandDispatchDebug.parsing )
		return;

	Con_Printf("Last %i messages parsed.\n", CMD_COUNT );
	// Finish here
	thecmd = g_CommandDispatchDebug.currentcmd - 1;
	// Back up to here
	thecmd -= ( CMD_COUNT - 1 );
	for ( i = 0; i < CMD_COUNT - 1; i++ )
	{
		thecmd &= CMD_MASK;

		CommandDispatchDebug::oldcmd_t *old = &g_CommandDispatchDebug.oldcmd[ thecmd ];

		Con_Printf ("%i %04i %s\n", 
			old->frame_number,  
			old->starting_offset,
			CL_MsgInfo( old->command ) );

		thecmd++;
	}

	CommandDispatchDebug::oldcmd_t *failcommand = &g_CommandDispatchDebug.oldcmd[ thecmd ];

	Con_Printf ("BAD:  %3i:%s\n",MSG_GetReadBuf()->GetNumBytesRead() - 1, CL_MsgInfo( failcommand->command ) );

	if ( developer.GetInt() )
	{
		CL_WriteErrorMessage( MSG_GetReadBuf()->GetNumBytesRead() - 1, &net_message );
	}

	g_CommandDispatchDebug.parsing = false;
}

#define SHOWNET(x) \
	if (cl_shownet.GetInt() == 2 && Q_strlen(x) > 1 ) \
		Con_Printf ("%3i:%s\n", MSG_GetReadBuf()->GetNumBytesRead()-1, x);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//-----------------------------------------------------------------------------
void CL_Parse_RecordCommand( byte cmd, int startoffset )
{
	if ( cmd == svc_nop )
		return;
	
	int slot = ( g_CommandDispatchDebug.currentcmd++ & CMD_MASK );

	g_CommandDispatchDebug.oldcmd[ slot ].command			= cmd;
	g_CommandDispatchDebug.oldcmd[ slot ].starting_offset	= startoffset;
	g_CommandDispatchDebug.oldcmd[ slot ].frame_number		= host_framecount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//			bufStart - 
//-----------------------------------------------------------------------------
void CL_DispatchRegularMessage( int cmd, int bufStart )
{
	SHOWNET( cl_parsefuncs[cmd].pszname );

	if ( cmd == svc_bad )
	{
		Host_Error ("CL_ParseServerMessage: Illegible server message\n" );
		return;
	}

	if ( cmd > SVC_LASTMSG )
	{
		Host_Error ("CL_ParseServerMessage: Invalid server message - %i (max == %i)\n", cmd, SVC_LASTMSG );
		return;
	}

	if ( cl_parsefuncs[ cmd ].pfnParse )
	{
		void ( *func )( void );
		func = cl_parsefuncs[ cmd ].pfnParse;
		// Dispatch parsing function

		if ( cl_showmessages.GetInt() )
		{
			Con_Printf( "%s, offset(%i)\n", cl_parsefuncs[ cmd ].pszname, bufStart );
		}

		(*func)();
	}
	else
	{
		if ( cl_showmessages.GetInt() )
		{
			Con_Printf( "No func for %s, offset(%i)\n", cl_parsefuncs[ cmd ].pszname, bufStart );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//			bytes_read - 
//-----------------------------------------------------------------------------
void CL_GatherBandwidthStatistics( int cmd, frame_t *frame, int bytes_read )
{
	switch ( cmd )
	{
	default:
		break;
	case svc_packetentities:
	case svc_deltapacketentities:
		{
			int otherplayerbytes	= ( cls.otherplayerbits >> 3 );
			int localbytes			= ( cls.localplayerbits >> 3 );

			// Aggregate all player bits
			int playerbytes			= ( cls.otherplayerbits + cls.localplayerbits ) >> 3;
			
			frame->packetentitybytes	+= bytes_read;
		
			// Move player bits over to packet entity section
			frame->packetentitybytes	-= playerbytes;

			// Update counters
			frame->otherplayerinfobytes	+= otherplayerbytes;
			frame->localplayerinfobytes += localbytes;
		}
		break;
	case svc_sound:
		{
			frame->soundbytes += bytes_read;
		}
		break;
	case svc_clientdata:
		{
			frame->clientbytes += bytes_read;
		}
		break;
	case svc_event:
	case svc_event_reliable:
		{
			frame->eventbytes += bytes_read;
		}
		break;
	}
}


extern bool g_bForceShowMessages;

//-----------------------------------------------------------------------------
// Purpose: Parse incoming message from server.
// Input  : normal_message - 
// Output : void CL_ParseServerMessage
//-----------------------------------------------------------------------------
void CL_ParseServerMessage( qboolean normal_message /* = true */ )
{
	// Index of svc_ or user command to issue.
	int			cmd;                       
	// For debugging errors in the network stream.
	starting_count = MSG_GetReadBuf()->GetNumBytesRead();
	// For determining data parse sizes
	int bufStart, bufEnd;  

	int oldShowMessages = cl_showmessages.GetInt();
	if ( g_bForceShowMessages )
	{
		cl_showmessages.SetValue( 1 );
	}

	if (cl_shownet.GetInt() == 1)
	{
		Con_Printf ("%i ",net_message.cursize);
	}
	else if (cl_shownet.GetInt() == 2)
	{
		Con_Printf ("------------------\n");
	}
	
	frame_t *frame = &cl.frames[ cls.netchan.incoming_sequence & CL_UPDATE_MASK ];

	//
	// parse the message
	//
	if ( normal_message )
	{
		// Assume no entity/player update this packet
		if ( cls.state == ca_active )
		{
			frame->invalid = true;   
			frame->choked = false;
		}
		else
		{
			CL_ResetFrame( frame );
		}
	}

	g_CommandDispatchDebug.parsing = true;

	bufEnd = 0;

	while (1)
	{
		// Bogus message?
		if ( MSG_IsOverflowed() )
		{
			Host_Error ("CL_ParseServerMessage: Bad server message");
		}

		// Mark start position
		bufStart = MSG_GetReadBuf()->GetNumBytesRead();

		if ( MSG_GetReadBuf()->GetNumBitsLeft() < 8 )
		{
			// end of message
			SHOWNET("END OF MESSAGE");
			break;		
		}

		cmd = MSG_ReadByte ();

		// Record command for debugging spew on parse problem
		CL_Parse_RecordCommand( cmd, bufStart );

		// Dispatch regular message
		CL_DispatchRegularMessage( cmd, bufStart );

		// Mark end position
		bufEnd = MSG_GetReadBuf()->GetNumBytesRead();

		// Gather bandwidth statistics for the netgraph
		CL_GatherBandwidthStatistics( cmd, frame, ( bufEnd - bufStart ) );
	}

	if ( cl_showmessages.GetInt() && bufEnd > 0 )
	{
		Con_Printf( "Packet end offset(%i)\n\n", bufEnd );
	}

	g_CommandDispatchDebug.parsing = false;

	// Latch in total message size
	if ( cls.state == ca_active )
	{
		frame->msgbytes += MSG_GetReadBuf()->m_nDataBytes;
	}

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (!demo->IsPlayingBack())
	{
		if ( cls.state != ca_active )
		{
			demo->WriteMessage ( true, starting_count, &net_message );
		}
		else if ( demo->IsRecording() && !demo->IsWaitingForUpdate() )
		{
			demo->WriteMessage ( false, starting_count, &net_message );
		}
	}

	// Play any sounds we received this packet
	CL_DispatchSounds();
	// Dispatch any events corresponding to this message in the .dem file
	demo->DispatchEvents();

	if ( g_bForceShowMessages )
	{
		cl_showmessages.SetValue( oldShowMessages );
		g_bForceShowMessages = false;
	}
}
