//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "glquake.h"
#include "server.h"
#include "host_cmd.h"
#include "keys.h"
#include "screen.h"
#include "vengineserver_impl.h"
#include "game_interface.h"
#include "host_saverestore.h"
#include "sv_filter.h"
#include "gl_matsysiface.h"
#include "pr_edict.h"
#include "world.h"
#include "checksum_engine.h"
#include "const.h"
#include "sv_main.h"
#include "host.h"
#include "demo.h"
#include "cdll_int.h"
#include "networkstringtableserver.h"
#include "networkstringtableclient.h"
#include "host_state.h"
#include "string_t.h"
#include "tier0/dbg.h"
#include "testscriptmgr.h"
#include "r_local.h"
#include "PlayerState.h"
#include "enginesingleuserfilter.h"
#include "profile.h"
#include "proto_version.h"
#include "cmd.h"

#ifdef _WIN32
#include "sound.h"
#include "voice.h"
#endif

#ifndef SWDS
// In other C files.
void VideoMode_SetVideoMode( int width, int height, int bpp );
void VideoMode_ChangeFullscreenMode( bool fullscreen );

extern qboolean	g_bInEditMode;
#endif

extern qboolean allow_cheats;
extern ConVar	pausable;

Vector	r_origin(0,0,0);

ConVar	host_name		( "hostname", "Half-Life 2" );
ConVar	host_map		( "host_map", "" );
ConVar  rcon_password	( "rcon_password", "" );

ConVar voice_recordtofile("voice_recordtofile", "0", 0, "Record mic data and decompressed voice data into 'voice_micdata.wav' and 'voice_decompressed.wav'");
ConVar voice_inputfromfile("voice_inputfromfile", "0", 0, "Get voice input from 'voice_input.wav' rather than from the microphone.");

static ConVar sv_fullsendtables( "sv_fullsendtables", "0", 0, "Force full sendtable sending path." );


// Globals
int	gHostSpawnCount = 0;

/*
==================
Host_Quit_f
==================
*/
void Host_Quit_f (void)
{
	HostState_Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Host_Quit_Restart_f( void )
{
	HostState_Restart();
}

#ifndef SWDS
//-----------------------------------------------------------------------------
// Purpose: Change to hardware renderer
//-----------------------------------------------------------------------------
void Host_SetRenderer_f( void )
{
	bool fullscreen = true;

	if ( cls.state == ca_dedicated )
		return;

	if ( Cmd_Argc() != 2 )
	{
		return;
	}

	if ( !stricmp( Cmd_Argv(1), "windowed" ) )
	{
		fullscreen = false;
	}

	VideoMode_ChangeFullscreenMode( fullscreen );
}

void Host_SetVideoMode_f( void )
{
	int w, h, bpp = 16;

	if ( cls.state == ca_dedicated )
		return;

	if ( Cmd_Argc() != 3 &&
		 Cmd_Argc() != 4 )
	{
		return;
	}

	w = Q_atoi( Cmd_Argv(1) );
	h = Q_atoi( Cmd_Argv(2) );
	if ( Cmd_Argc() == 4 )
	{
		bpp = Q_atoi( Cmd_Argv(3) );
	}

	VideoMode_SetVideoMode( w, h, bpp );
}
#endif

#ifndef SWDS
//-----------------------------------------------------------------------------
// A console command to spew out driver information
//-----------------------------------------------------------------------------
void Host_LightCrosshair (void);

static ConCommand light_crosshair( "light_crosshair", Host_LightCrosshair );

void Host_LightCrosshair (void)
{
	Vector endPoint;
	Vector lightmapColor;

	// max_range * sqrt(3)
	VectorMA( r_origin, COORD_EXTENT * 1.74f, vpn, endPoint );
	
	R_LightVec( r_origin, endPoint, true, lightmapColor );
	int r = LinearToTexture( lightmapColor.x );
	int g = LinearToTexture( lightmapColor.y );
	int b = LinearToTexture( lightmapColor.z );

	Con_Printf( "Luxel Value: %d %d %d\n", r, g, b );
}
#endif

/*
==================
Host_Status_PrintClient

Print client info to console 
==================
*/
void Host_Status_PrintClient( char *pszStatus, qboolean bShowAddress, void (*print) (const char *fmt, ...), int j, client_t *client )
{
	int			seconds;
	int			minutes;
	int			hours = 0;

	seconds = (int)(realtime - client->netchan.connect_time);
	minutes = seconds / 60;
	if (minutes)
	{
		seconds -= (minutes * 60);
		hours = minutes / 60;
		if (hours)
			minutes -= (hours * 60);
	}
	else
		hours = 0;

	print ("#%-2u %-12.12s\n", j + 1, client->name  );
				
	if ( hours != 0 )
		print ( "   time :  %2i:%02i:%02i\n", hours, minutes, seconds );
	else
		print ( "   time :  %02i:%02i\n", minutes, seconds );

	print ( "   ping :  %4i\n", (int)SV_CalcLatency( client ) );
//	print ( "   drop :  %5.2f %%\n", (100.0*(float)client->netchan.drop_count) / (float)client->netchan.incoming_sequence );
	
	if ( pszStatus && pszStatus[0] )
		print( "   %s\n", pszStatus );

	if ( client->fakeclient )
		print ( "  (fake)\n" );
	else if ( bShowAddress )
		print ("   %s\n", NET_BaseAdrToString(client->netchan.remote_address));

	print( "\n" );

}

/*
==================
Host_Status_f
==================
*/
void Host_Status_f (void)
{
	client_t	*client;

	int			j;
	int nClients;
	void		(*print) (const char *fmt, ...);

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer ();
			return;
		}
		print = Con_Printf;
	}
	else
		print = SV_ClientPrintf;

// ============================================================
// Server status information.
	print ( "hostname:  %s\n", host_name.GetString() );
	print ( "version :  %s\n", gpszVersionString );
	print ( "build   :  %d\n", build_number() );
	
	if ( !noip )
	{
		print ( "tcp/ip  :  %s\n", NET_AdrToString (net_local_adr) );
	}

	print (" map     :  %s at: %d x, %d y, %d z\n", sv.name, (int)r_origin[0], (int)r_origin[1], (int)r_origin[2]);

	SV_CountPlayers( &nClients );

	print (" players: %i active (%i max)\n\n", nClients, svs.maxclients);
// ============================================================

	// Early exit for this server.
	if ( Cmd_Argc() == 2 )
	{
		if ( !stricmp( Cmd_Argv(1), "short" ) )
		{
			for ( j=0, client = svs.clients ; j < svs.maxclients ; j++, client++)
			{
				if ( !client->active )
					continue;

				print( "#%i - %s\n" , j + 1, client->name );
			}
			return;
		}
	}

	for ( j=0, client = svs.clients ; j < svs.maxclients ; j++, client++)
	{
		if (!client->active)
		{
			if ( client->connected )
			{
				Host_Status_PrintClient
				( 
					"CONNECTING" ,
					(cmd_source == src_command),
					print,
					j,
					client
				);
			}
			continue;
		}
		
		Host_Status_PrintClient
		(
			NULL, 
			(cmd_source == src_command),
			print,
			j,
			client
		);
	}
}


/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f (void)
{
	int		i;
	client_t	*client;
	
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	SV_ClientPrintf ("Client ping times:\n");
	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		if (!client->active)
			continue;
		SV_ClientPrintf ("%4i %s\n", (int)SV_CalcLatency(client), client->name);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : editmode - 
//-----------------------------------------------------------------------------
extern void GetPlatformMapPath( const char *pMapPath, char *pPlatformMapPath, int maxLength );
void Host_Map_Helper( bool editmode )
{
	int		i;
	char	name[MAX_QPATH];

	if (cmd_source != src_command)
		return;

	GetPlatformMapPath( Cmd_Argv(1), name, sizeof( name ) );
	
	// If I was in edit mode reload config file
	// to overwrite WC edit key bindings
#ifndef SWDS
	if ( !editmode )
	{
		if (g_bInEditMode)
		{
			// Re-read config from disk
			Host_ReadConfiguration();
			g_bInEditMode = false;
		}
	}
	else
	{
		g_bInEditMode = true;
	}
#endif

	// If there is a .bsp on the end, strip it off!
	i = strlen(name);
	if ( i > 4 && !Q_strcasecmp( name + i - 4, ".bsp" ) )
	{
		name[i-4] = 0;
	}

	if ( !g_pVEngineServer->IsMapValid( name ) )
	{
		Warning( "map load failed: %s not found or invalid\n", name );
		return;
	}

	// Update whether we have a valid authentication cert
	if ( ( svs.maxclients > 1 ) && !sv_lan.GetInt() )
	{
		COM_CheckAuthenticationType();
	}

	// JAY: UNDONE: Remove this?
	HostState_NewGame( name );

	// If any test scripts are running, make them wait until the map is loaded.
	GetTestScriptMgr()->SetWaitCheckPoint( "FinishedMapLoad" );
}

/*
======================
Host_Map_f

handle a 
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f (void)
{
	Host_Map_Helper( false );
}

/* 
UNDONE: protect this from use if not in dev. mode
======================
Host_Map_Edit_f

handle a 
map_edit <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_Edit_f (void)
{
	Host_Map_Helper( true );
}


/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f (void)
{
	if (Demo_IsPlayingBack() || !sv.active)
		return;

	if (cmd_source != src_command)
		return;

	HostState_NewGame( sv.name );
}

/*
==================
Host_Reload_f

Restarts the current server for a dead player
==================
*/
void Host_Reload_f (void)
{
#ifndef SWDS
	const char *pSaveName;
	char name[MAX_OSPATH];
#endif

	if (Demo_IsPlayingBack() || !sv.active)
		return;

	if (cmd_source != src_command)
		return;

	// See if there is a most recently saved game
	// Restart that game if there is
	// Otherwise, restart the starting game map
#ifndef SWDS
	pSaveName = saverestore->FindRecentSave( name, sizeof( name ) );
	if ( pSaveName )
	{
		HostState_LoadGame( pSaveName );
	}
	else
#endif
	{
		HostState_NewGame( host_map.GetString() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Goes to a new map, taking all clients along
// Output : void Host_Changelevel_f
//-----------------------------------------------------------------------------
void Host_Changelevel_f (void)
{
	if ( Cmd_Argc() < 2 )
	{
		Msg ("changelevel <levelname> : continue game on a new level\n");
		return;
	}

	if ( !sv.active )
	{
		Msg( "Can't changelevel, not running server\n" );
		return;
	}

	if ( !g_pVEngineServer->IsMapValid( Cmd_Argv(1) ) )
	{
		Warning( "changelevel failed: %s not found\n", Cmd_Argv(1) );
		return;
	}

	HostState_ChangeLevelMP( Cmd_Argv(1), Cmd_Argv(2) );
}

//-----------------------------------------------------------------------------
// Purpose: Changing levels within a unit, uses save/restore
//-----------------------------------------------------------------------------
void Host_Changelevel2_f( void )
{
	if ( Cmd_Argc() < 2 )
	{
		Msg ("changelevel2 <levelname> : continue game on a new level in the unit\n");
		return;
	}

	if ( !sv.active )
	{
		Msg( "Can't changelevel2, not in a map\n" );
		return;
	}

	if ( !g_pVEngineServer->IsMapValid( Cmd_Argv(1) ) )
	{
		Warning( "changelevel2 failed: %s not found\n", Cmd_Argv(1) );
		return;
	}
	
	HostState_ChangeLevelSP( Cmd_Argv(1), Cmd_Argv(2) );
}

//-----------------------------------------------------------------------------
// Purpose: Shut down client connection and any server
//-----------------------------------------------------------------------------
void Host_Disconnect( void )
{
#ifndef SWDS
	CL_Disconnect();
#endif
	HostState_GameShutdown();
}

/*
=======================
Host_Disconnect_f

Kill the client and any local server.
=======================
*/
void Host_Disconnect_f (void)
{
	Host_Disconnect();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Host_Version_f
//-----------------------------------------------------------------------------
void Host_Version_f (void)
{
	Msg( "Exe version %s (protocol %i)\n", gpszVersionString, PROTOCOL_VERSION );
	Msg ("Build: "__TIME__" "__DATE__" (%i)\n", build_number() );
}

/*
==================
Host_FullInfo_f

Allow clients to change userinfo
==================
*/
void Host_FullInfo_f (void)
{
	char	key[512];
	char	value[512];
	char	*o;
	char	*s;

	if (Cmd_Argc() != 2)
	{
		Msg ("fullinfo <complete info string>\n");
		return;
	}

	s = Cmd_Argv(1);
	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!*s)
		{
			Warning ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;

		if (cmd_source == src_command)
		{
			Info_SetValueForKey (cls.userinfo, key, value, MAX_INFO_STRING);
			Cmd_ForwardToServer();
			return;
		}

		Info_SetValueForKey (host_client->userinfo, key, value, MAX_INFO_STRING);
		SV_ExtractFromUserinfo (host_client);  // parse some info from the info strings
	}
}

/*
==================
Host_SetInfo_f

Allow clients to change userinfo
==================
*/
void Host_SetInfo_f (void)
{
	if (Cmd_Argc() == 1)
	{
		Info_Print (cls.userinfo);
		return;
	}
	if (Cmd_Argc() != 3)
	{
		Msg ("usage: setinfo [ <key> <value> ]\n");
		return;
	}


	if (cmd_source == src_command)
	{
		Info_SetValueForKey (cls.userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING);
		Cmd_ForwardToServer ();
		return;
	}

	Info_SetValueForKey (host_client->userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING);
	host_client->sendinfo = true;

	SV_ExtractFromUserinfo (host_client);  // parse some info from the info strings
}

void Host_Say(qboolean teamonly)
{
	client_t *client;
	client_t *save;
	int		j;
	char	*p;
	char	text[64];

	if (cls.state != ca_dedicated)
	{
		if (cmd_source != src_command)
			return;  // this will only happen if the game engine handles the say command instead of the entity dll;  that shouldn't happen
		
		Cmd_ForwardToServer ();
		return;
	}

	if (Cmd_Argc () < 2)
		return;

	p = Cmd_Args();
	if (!p)
		return;

	save = host_client;

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[Q_strlen(p)-1] = 0;
	}

	Q_snprintf(text, sizeof( text ), "<%s> ", host_name.GetString() );

	j = sizeof(text) - 2 - Q_strlen(text);  // -2 for /n and null terminator
	if (Q_strlen(p) > j)
		p[j] = 0;

	strcat (text, p);
	strcat (text, "\n");

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client || !client->active || !client->spawned)
			continue;

		host_client = client;

		CEngineSingleUserFilter singleUserFilter( j + 1 );

		host_client->netchan.message.WriteByte (svc_stufftext);
		host_client->netchan.message.WriteString (va("echo %s\n", text ) );
	}

	host_client = save;

	Sys_Printf( "%s", &text[1] );
}

void Host_Say_f(void)
{
	Host_Say(false);
}

void Host_Say_Team_f(void)
{
	Host_Say(true);
}

void Host_Tell_f(void)
{
	client_t *client;
	client_t *save;
	int		j;
	char	*p;
	char	text[64];

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	if (Cmd_Argc () < 3)
		return;

	Q_strcpy(text, host_client->name);
	Q_strcat(text, ": ");

	p = Cmd_Args();
	if (!p)
		return;

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[Q_strlen(p)-1] = 0;
	}

// check length & truncate if necessary
	j = sizeof(text) - 2 - Q_strlen(text);  // -2 for /n and null terminator
	if (Q_strlen(p) > j)
		p[j] = 0;

	strcat (text, p);
	strcat (text, "\n");

	save = host_client;
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active && !client->spawned)
			continue;
		if (Q_strcasecmp(client->name, Cmd_Argv(1)))
			continue;
		host_client = client;
		SV_ClientPrintf("%s", text);
		break;
	}
	host_client = save;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsPausable( void )
{
	if ( svs.maxclients > 1 )
		return false;

	return pausable.GetInt() ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Host_TogglePause_f (void)
{
#ifndef SWDS
	if ( !cl.levelname[ 0 ] )
		return;
#endif

	if ( cmd_source == src_command )
	{
		Cmd_ForwardToServer ();
		return;
	}
	if ( !IsPausable() )
	{
		return;
	}


	sv.paused = !sv.paused;

	Assert( serverGameClients );
	CPlayerState *pl = serverGameClients->GetPlayerState( sv_player );
	if ( pl )
	{
		SV_BroadcastPrintf( "%s %s the game\n", STRING( pl->netname ), !sv.paused ? "paused" : "unpaused" );
	}

	// send notification to all clients
	sv.reliable_datagram.WriteByte ( svc_setpause );
	sv.reliable_datagram.WriteByte ( sv.paused );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Host_SetPause_f
//-----------------------------------------------------------------------------
void Host_SetPause_f (void)
{
#ifndef SWDS
	if ( !cl.levelname[ 0 ] )
		return;
#endif

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}
	if ( !IsPausable() )
	{
		return;
	}

	sv.paused = true;

	// send notification to all clients
	sv.reliable_datagram.WriteByte ( svc_setpause );
	sv.reliable_datagram.WriteByte ( sv.paused );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void Host_UnPause_f
//-----------------------------------------------------------------------------
void Host_UnPause_f (void)
{
#ifndef SWDS
	if ( !cl.levelname[ 0 ] )
		return;
#endif

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}
	if ( !IsPausable() )
	{
		return;
	}

	sv.paused = false;

	// send notification to all clients
	sv.reliable_datagram.WriteByte ( svc_setpause );
	sv.reliable_datagram.WriteByte ( sv.paused );
}


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f (void)
{
	if (cmd_source == src_command)
	{
		Warning ("prespawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Warning ("prespawn not valid -- already spawned\n");
		return;
	}
	
	if ( Cmd_Argc() != 2 )
	{
		Warning ("prespawn without level tag!\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (!Demo_IsPlayingBack() &&
	 ( atoi(Cmd_Argv(1)) != svs.spawncount ) )
	{
		Warning ("SV_PreSpawn_f from different level\n");
		SV_New_f();
		return;
	}

	if ( sv.signon.IsOverflowed() )
	{
		Host_Error( "Signon buffer overflowed %i bytes!!!\n", sv.signon.GetNumBytesWritten() );
		return;
	}

	if ( sv.signon_fullsendtables.IsOverflowed() )
	{
		Host_Error( "Send Table signon buffer overflowed %i bytes!!!\n", sv.signon_fullsendtables.GetNumBytesWritten() );
		return;
	}

	// Full packet of buffers
	byte buffer[ NET_MAX_PAYLOAD ];
	bf_write msg( "Host_PreSpawn_f", buffer, sizeof( buffer ) );

	bool send_full_tables = true;
	if ( svs.sendtable_crc != (CRC32_t)0 )
	{
		if ( host_client->sendtable_crc == svs.sendtable_crc )
		{
			send_full_tables = false;
		}
	}

	// Write the send tables if needed
	if ( send_full_tables || sv_fullsendtables.GetBool() )
	{
		msg.WriteBits( sv.signon_fullsendtables.GetData(), sv.signon_fullsendtables.GetNumBitsWritten() );
	}
	else
	{
		// Write the shortcut command
		msg.WriteByte(svc_classinfo);
		msg.WriteByte(CLASSINFO_CREATEFROMSENDTABLES);
	}

	// Write the regular signon now
	msg.WriteBits( sv.signon.GetData(), sv.signon.GetNumBitsWritten() );

	sv.allowsignonwrites = false;

	msg.WriteByte ( svc_signonnum );
	msg.WriteByte ( 1 );

	Netchan_CreateFragments( true, &host_client->netchan, &msg );
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f (void)
{
	int			i;
	client_t	*client;
	edict_t		*ent;
	int			spawncount;

	if (cmd_source == src_command)
	{
		Warning ("spawn is not valid from the console\n");
		return;
	}

	if (!host_client->connected)
	{
		Warning ("Spawn not valid -- already spawned\n");
		return;
	}

	spawncount = atoi(Cmd_Argv(1));

// handle the case of a level changing while a client was connecting
   if (!Demo_IsPlayingBack() && 
   ( spawncount != svs.spawncount ) )
	{
		Warning ("Spawn from different level\n");
		SV_New_f();
		return;
	}

	host_client->netchan.message.Reset();
	// Set client clock to match server's

	SV_WriteServerTimestamp( host_client->netchan.message, host_client );

	// Grab name from userinfo string
	Q_strncpy( host_client->name, SV_ExtractNameFromUserinfo( host_client ), sizeof( host_client->name ) );

	// Spawned into server, not fully active, though
	host_client->spawned = true;

// run the entrance script
	if (sv.loadgame)
	{	// loaded games are fully inited already
		// if this is the last client to be connected, unpause
		sv.paused = false;
	}
	else
	{
		// set up the edict
		ent = host_client->edict;
		sv.state = ss_loading;

		serverGameEnts->FreeContainingEntity(ent);

		InitializeEntityDLLFields(ent);
		Assert( serverGameClients );
		// call the spawn function
		g_ServerGlobalVariables.curtime = sv.gettime();
		serverGameClients->ClientPutInServer( sv_player, host_client->name );
		sv.state = ss_active;
	}

	// Do we need to upload our logo
	if ( host_client->request_logo )
	{
		host_client->request_logo = false;
		// Don't have it locally, so we need to request logo file upload
		host_client->netchan.message.WriteByte( svc_sendlogo );
		host_client->netchan.message.WriteLong( (unsigned int)host_client->logo );
	}

	// Parse some other info from the info strings and tell other players

	SV_ExtractFromUserinfo( host_client );  
	
	// send other clients userinfo 
	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		SV_FullClientUpdate( client, &host_client->netchan.message );
	}

	host_client->netchan.message.WriteByte( svc_lightstyle );
	host_client->netchan.message.WriteOneBit( 1 );
	// send all current light styles
	for ( i=0 ; i< MAX_LIGHTSTYLES ; i++ )
	{
		if ( sv.lightstyles[i] && sv.lightstyles[ i ][ 0 ] )
		{
			host_client->netchan.message.WriteOneBit( 1 );
			host_client->netchan.message.WriteString( sv.lightstyles[i] );
		}
		else
		{
			host_client->netchan.message.WriteOneBit( 0 );
		}
	}

	//
	// send a fixangle
	// Never send a roll angle, because savegames can catch the server
	// in a state where it is expecting the client to correct the angle
	// and it won't happen if the game was just loaded, so you wind up
	// with a permanent head tilt
	ent = sv.edicts + ( 1 + (host_client - svs.clients) );
 	host_client->netchan.message.WriteByte ( svc_setangle);

	Assert( serverGameClients );
	CPlayerState *pl = serverGameClients->GetPlayerState( sv_player );
	Assert( pl );

	for ( i=0 ; i < 2 ; i++ )
	{
		host_client->netchan.message.WriteBitAngle( pl->v_angle[i] , 16 );
	}
	host_client->netchan.message.WriteBitAngle( 0, 16 );

	SV_WriteClientdataToMessage (host_client, &host_client->netchan.message);

	host_client->netchan.message.WriteByte ( svc_signonnum );
	host_client->netchan.message.WriteByte ( 2 );
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f (void)
{
	int spawncount;

	if (cmd_source == src_command)
	{
		Warning ("begin is not valid from the console\n");
		return;
	}

	spawncount = atoi(Cmd_Argv(1));

// handle the case of a level changing while a client was connecting
    if ( !Demo_IsPlayingBack() && 
	( spawncount != svs.spawncount ) )
	{
		Warning ("Begin from different level\n");
		SV_New_f();
		return;
	}
	
	host_client->active = true;
	// host_client->spawned = true;
	host_client->connected = false; // done d'l ing

	serverGameClients->ClientActive( sv_player );

	GetTestScriptMgr()->CheckPoint( "client_connected" );
}

/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f (void)
{
	char		*who;
	char		*message = NULL;
	client_t	*save;
	int			i;
	qboolean	byNumber = false;

	if ( Cmd_Argc() <= 1 )
	{
		Msg( "usage:  kick < name > | < # userid >\n" );
		return;
	}

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer ();
			return;
		}
	}
	else if (g_ServerGlobalVariables.deathmatch)
		return;

	save = host_client;

	if (Cmd_Argc() > 2 && Q_strcmp(Cmd_Argv(1), "#") == 0)
	{
		i = Q_atof(Cmd_Argv(2)) - 1;
		if (i < 0 || i >= svs.maxclients)
			return;
		if (!svs.clients[i].active)
			return;
		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (!host_client->active && !host_client->connected)
				continue;
			if (Q_strcasecmp(host_client->name, Cmd_Argv(1)) == 0)
				break;
		}
	}

	if (i < svs.maxclients)
	{
		
		if (cmd_source == src_command)
		{
#ifdef SWDS
                        who = "Console";
#else
                        if ( cls.state == ca_dedicated )
                        {
                                who = "Console";
                        }
                        else
                        {
				who =  const_cast<char *>(cl_name.GetString());
                        }
#endif
		}
		else
		{
			who = save->name;
		}

		// can't kick yourself!
		if (host_client == save)
			return;

		if (Cmd_Argc() > 2)
		{
			message = COM_Parse(Cmd_Args());
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += Q_strlen(Cmd_Argv(2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
			SV_ClientPrintf ("Kicked by %s: %s\n", who, message);
		else
			SV_ClientPrintf ("Kicked by %s\n", who);
		SV_DropClient (host_client, false, "kicked by %s", who );
	}

	host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

/*
==================
Host_ShowMaterials_f
==================
*/
void Host_ShowMaterials_f (void)
{
	if( Cmd_Argc() == 1 )
	{
		materialSystemInterface->DebugPrintUsedMaterials( NULL, false );
	}
	else
	{
		materialSystemInterface->DebugPrintUsedMaterials( Cmd_Argv( 1 ), false );
	}
}

/*
==================
Host_ShowMaterials_f
==================
*/
void Host_ShowMaterialsVerbose_f (void)
{
	if( Cmd_Argc() == 1 )
	{
		materialSystemInterface->DebugPrintUsedMaterials( NULL, true );
	}
	else
	{
		materialSystemInterface->DebugPrintUsedMaterials( Cmd_Argv( 1 ), true );
	}
}

/*
==================
Host_ShowTextures_f
==================
*/
void Host_ShowTextures_f (void)
{
	materialSystemInterface->DebugPrintUsedTextures();
}

/*
==================
Host_ReloadMaterials_f
==================
*/

void ReleaseMaterialSystemObjects();
void RestoreMaterialSystemObjects();

void Host_ReloadAllMaterials_f (void)
{
	materialSystemInterface->ReloadMaterials( NULL );
}

void Host_ReloadMaterial_f (void)
{
	if( Cmd_Argc() != 2 )
	{
		Warning( "Usage: mat_reloadmaterials material_name_substring\n" );
		return;
	}
	materialSystemInterface->ReloadMaterials( Cmd_Argv( 1 ) );
}

void Host_ReloadTextures_f( void )
{
	materialSystemInterface->ReloadTextures();
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


#ifndef SWDS
/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f (void)
{
	int		i, c;

	if (cls.state == ca_dedicated)
	{
		if (!sv.active)
			Msg("Cannot play demos on a dedicated server.\n");
			//Cbuf_AddText ("map hldm1\n");
		return;
	}

	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		Msg ("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	Msg ("%i demo(s) in loop\n", c);

	for (i=1 ; i<c+1 ; i++)
		Q_strncpy(cls.demos[i-1], Cmd_Argv(i), sizeof(cls.demos[0]));

	cls.demonum = 0;

	if (!sv.active 
		&& !Demo_IsPlayingBack() )
	{
		cls.demonum = 0;
		CL_NextDemo ();
	}
	else
		cls.demonum = -1;
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f (void)
{
	if (cls.state == ca_dedicated)
		return;
	if (cls.demonum == -1)
		cls.demonum = 0;
	Host_Disconnect();
	CL_NextDemo ();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f (void)
{
	if (cls.state == ca_dedicated)
		return;

	if (!Demo_IsPlayingBack())
		return;
	
	Host_Disconnect ();
}

//-----------------------------------------------------------------------------
// Purpose: Skip to next demo
//-----------------------------------------------------------------------------
void Host_NextDemo_f( void )
{
	Host_EndGame( "Moving to next demo..." );
}

/*
==================
Host_Soundfade_f

==================
*/
void Host_Soundfade_f(void)
{
	float percent;
	float inTime, holdTime, outTime;

	if (Cmd_Argc() != 3 && Cmd_Argc() != 5)
	{
		Msg("soundfade <percent> <hold> [<out> <int>]\n");
		return;
	}

	percent = clamp( atof(Cmd_Argv(1)), 0.0f, 100.0f );
	
	holdTime = max( 0.0f, atof(Cmd_Argv(2)) );

	inTime = 0.0f;
	outTime = 0.0f;
	if (Cmd_Argc() == 5)
	{
		outTime = max( 0.0f, atof(Cmd_Argv(3)) );
		inTime = max( 0.0f, atof( Cmd_Argv(4)) );
	}

	S_SoundFade( percent, holdTime, outTime, inTime );
}
#endif

/*
==================
Host_KillServer_f

==================
*/
void Host_KillServer_f(void)
{
	Host_Disconnect();
	if ( cls.state != ca_dedicated )
	{
		// close network sockets
		NET_Config ( false );
	}
}

#ifndef SWDS

void Host_VoiceRecordStart_f(void)
{
	if (cls.state == ca_active)
	{
		const char *pUncompressedFile = NULL;
		const char *pDecompressedFile = NULL;
		const char *pInputFile = NULL;
		
		if(voice_recordtofile.GetInt())
		{
			pUncompressedFile = "voice_micdata.wav";
			pDecompressedFile = "voice_decompressed.wav";
		}

		if(voice_inputfromfile.GetInt())
		{
			pInputFile = "voice_input.wav";
		}

		if (Voice_RecordStart(pUncompressedFile, pDecompressedFile, pInputFile))
		{
		}
	}
}


void Host_VoiceRecordStop_f(void)
{
	if (cls.state == ca_active)
	{
		if (Voice_IsRecording())
		{
			CL_SendVoicePacket(true);
			Voice_RecordStop();
		}
	}
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Wrapper for modelloader->Print() function call
//-----------------------------------------------------------------------------
void Mod_Print( void )
{
	modelloader->Print();
}

/*
==================
Host_IncrementCVar
==================
*/
void Host_IncrementCVar_f( void )
{
	if( Cmd_Argc() != 5 )
	{
		Warning( "Usage: incrementvar varName minValue maxValue delta" );
		return;
	}

	char const *varName = Cmd_Argv( 1 );
	if( !varName )
	{
		Con_DPrintf( "Host_IncrementCVar_f without a varname\n" );
		return;
	}

	ConVar *var = ( ConVar * )cv->FindVar( varName );
	if( !var )
	{
		Con_DPrintf( "cvar \"%s\" not found\n", varName );
		return;
	}

	float currentValue = var->GetFloat();
	float startValue = atof( Cmd_Argv( 2 ) );
	float endValue = atof( Cmd_Argv( 3 ) );
	float delta = atof( Cmd_Argv( 4 ) );
	float newValue = currentValue + delta;
	if( newValue > endValue )
	{
		newValue = startValue;
	}
	else if ( newValue < startValue )
	{
		newValue = endValue;
	}

	// Conver incrementvar command to direct sets to avoid any problems with state in a demo loop.
	Cbuf_InsertText( va("%s %f", varName, newValue) );

	Con_DPrintf( "%s = %f\n", var->GetName(), newValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Host_DumpStringTables_f( void )
{
	SV_PrintStringTables();
#ifndef SWDS
	CL_PrintStringTables();
#endif
}

// Register shared commands
static ConCommand dumpstringtables( "dumpstringtables", Host_DumpStringTables_f );
static ConCommand killserver("killserver", Host_KillServer_f);
static ConCommand status("status", Host_Status_f);
static ConCommand quit("quit", Host_Quit_f);
static ConCommand cmd_exit("exit", Host_Quit_f);
static ConCommand quti("quti", Host_Quit_f);
static ConCommand _restart("_restart", Host_Quit_Restart_f );

#ifndef SWDS
static ConCommand _setvideomode( "_setvideomode", Host_SetVideoMode_f );
static ConCommand _setrenderer( "_setrenderer", Host_SetRenderer_f );
static ConCommand map_edit("map_edit", Host_Map_Edit_f);
static ConCommand soundfade("soundfade", Host_Soundfade_f);
#ifdef VOICE_OVER_IP
static ConCommand startvoicerecord("+voicerecord", Host_VoiceRecordStart_f);
static ConCommand endvoicerecord("-voicerecord", Host_VoiceRecordStop_f);
#endif // VOICE_OVER_IP
static ConCommand startdemos("startdemos", Host_Startdemos_f);
static ConCommand nextdemo( "nextdemo", Host_NextDemo_f, "Play next demo in sequence." );
static ConCommand demos("demos", Host_Demos_f);
static ConCommand stopdemo("stopdemo", Host_Stopdemo_f);
static ConCommand mat_showmaterials("mat_showmaterials", Host_ShowMaterials_f);
static ConCommand mat_showmaterialsverbose("mat_showmaterialsverbose", Host_ShowMaterialsVerbose_f);
static ConCommand mat_showtextures("mat_showtextures", Host_ShowTextures_f);
#endif

static ConCommand restart("restart", Host_Restart_f);
static ConCommand disconnect("disconnect", Host_Disconnect_f);
static ConCommand reload("reload", Host_Reload_f);
static ConCommand changelevel("changelevel", Host_Changelevel_f);
static ConCommand changelevel2("changelevel2", Host_Changelevel2_f);
static ConCommand version("version", Host_Version_f);
static ConCommand say("say", Host_Say_f);
static ConCommand say_team("say_team", Host_Say_Team_f);
static ConCommand tell("tell", Host_Tell_f);
static ConCommand setpause("setpause", Host_SetPause_f);
static ConCommand unpause("unpause", Host_UnPause_f);
static ConCommand conpause("pause", Host_TogglePause_f);
static ConCommand spawn("spawn", Host_Spawn_f);
static ConCommand begin("begin", Host_Begin_f);
static ConCommand prespawn("prespawn", Host_PreSpawn_f);
static ConCommand kick("kick", Host_Kick_f);
static ConCommand ping("ping", Host_Ping_f);
static ConCommand setinfo("setinfo", Host_SetInfo_f);
static ConCommand fullinfo("fullinfo", Host_FullInfo_f);
static ConCommand mat_reloadmaterial("mat_reloadmaterial", Host_ReloadMaterial_f);
static ConCommand mat_reloadallmaterials("mat_reloadallmaterials", Host_ReloadAllMaterials_f);
static ConCommand mat_reloadtextures("mat_reloadtextures", Host_ReloadTextures_f);
static ConCommand mcache("mcache", Mod_Print);
static ConCommand cmd_new("new", SV_New_f);
static ConCommand incrementvar( "incrementvar", Host_IncrementCVar_f );
