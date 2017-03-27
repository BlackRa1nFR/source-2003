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
// $NoKeywords: $
//=============================================================================
#include "glquake.h"
#include "utllinkedlist.h"
#include "quakedef.h"
#include "cdll_int.h"
#include "r_local.h"
#include "sound.h"
#include "screen.h"
#include "vid.h"
#include "server.h"
#include "checksum_engine.h"
#include "cl_servercache.h"
#include "host.h"
#include "con_nprint.h"
#include "cl_pred.h"
#include "gl_texture.h"
#include "gl_lightmap.h"
#include "client_class.h"
#include "console.h"
#include "icliententitylist.h"
#include "cl_ents.h"
#include "con_nprint.h"
#include "measure_section.h"
#include "traceinit.h"
#include "demo.h"
#include "cdll_engine_int.h"
#include "voice.h"
#include "DebugOverlay.h"
#include "FileSystem.h"
#include "FileSystem_Engine.h"
#include "NetworkStringTableContainerClient.h"
#include "precache.h"
#include "icliententity.h"
#include "decal.h"
#include "dt_recv_eng.h"
#include "iregistry.h"
#include "vgui_int.h"
#include "testscriptmgr.h"
#include "tier0/vprof.h"
#include "proto_oob.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "gl_matsysiface.h"
#include "cl_ents_parse.h"
#include "staticpropmgr.h"
#include "ISpatialPartitionInternal.h"
#include "cl_localnetworkbackdoor.h"
#include "vstdlib/icommandline.h"
#include "enginebugreporter.h"

void R_UnloadSkys( void );
void CL_ResetEntityBits( void );
void COM_CopyFile (char *netpath, char *cachepath);

// Mininum time gap (in seconds) before a subsequent connection request is sent.
#define CL_MIN_RESEND_TIME			1.5    
// Max time.  The cvar cl_resend is bounded by these.
#define CL_MAX_RESEND_TIME			20.0    
 // Only send this many requests before timing out.
#define CL_CONNECTION_RETRIES		4   
// If we get more than 250 messages in the incoming buffer queue, dump any above this #
#define MAX_INCOMING_MESSAGES		250
// Largest # of commands to send in a packet
#define MAX_TOTAL_CMDS				16
// Size of command send buffer
#define MAX_CMD_BUFFER				4000
// In release, send commands at least this many times per second
#define MIN_CMD_RATE				10.0
// If the network connection hasn't been active in this many seconds, display some warning text.
#define CONNECTION_PROBLEM_TIME		15.0

CGlobalVarsBase g_ClientGlobalVariables( true );

extern ConVar rcon_password;

static server_cache_t	cached_servers[MAX_LOCAL_SERVERS];
static int		num_servers;

// Local server cache request timeout
static float       slist_time;         // Time we started listening for broadcast server info responses.
class CConnectionInfo
{
public:
	int         retrynumber;      // After CL_CONNECTION_RETRIES, give up...
	int			challenge;			// from the server to use for connecting
	byte		authprotocol;       // Connect using Hashed CD key, or auth certificate
};

static CConnectionInfo s_connection;

ConVar	cl_interp			( "cl_interp", "0.1", 0, "Interpolate object positions starting this many seconds in past" );  
ConVar	cl_cmdrate			( "cl_cmdrate", "30", FCVAR_ARCHIVE, "Max number of command packets sent to server per second" );
ConVar	cl_cmdbackup		( "cl_cmdbackup", "2", FCVAR_ARCHIVE, "For each command packet, how many additional history commands are sent ( helps in case of packet loss )" );
ConVar	cl_showmessages		( "cl_showmessages", "0", 0, "Dump network traffic to debug console" );
ConVar  cl_captions			( "cl_captions", "0", FCVAR_ARCHIVE, "Show captions for voice" );
ConVar	cl_timeout			( "cl_timeout", "305", FCVAR_ARCHIVE, "After this many seconds without receiving a packet from the server, the client will disconnect itself" );
ConVar	cl_shownet			( "cl_shownet","0", 0, "Show network traffic ( 0, 1 or 2 )" );
ConVar	rcon_address		( "rcon_address", "", 0, "Address of remote server if sending unconnected rcon commands" );
ConVar  rcon_port			( "rcon_port"   , "0", 0, "Port of remote server if sending unconnected rcon commands" );
ConVar	cl_resend			( "cl_resend","6", 0, "Delay in seconds before the client will resend the 'connect' attempt", true, CL_MIN_RESEND_TIME, true, CL_MAX_RESEND_TIME );
ConVar  cl_slisttimeout		( "cl_slist", "10", 0, "Number of seconds to wait for server ping responses when checking for server on your lan" );
// Thes vars are also userinfo mirrors
ConVar	password( "password", "", FCVAR_USERINFO, "Current server access password" );
ConVar	cl_name( "name","unnamed", FCVAR_ARCHIVE | FCVAR_USERINFO | FCVAR_PRINTABLEONLY, "Current user name" );
ConVar	team( "team","", FCVAR_ARCHIVE | FCVAR_USERINFO, "Selected team" );
ConVar	skin( "skin","", FCVAR_ARCHIVE | FCVAR_USERINFO, "Selected skin" );
ConVar	cl_model( "model", "", FCVAR_ARCHIVE | FCVAR_USERINFO, "Current model name" );
ConVar	rate( "rate","10000" /*2500*/, FCVAR_ARCHIVE | FCVAR_USERINFO, "Bytes per second max that the server can send you data" );
ConVar	cl_updaterate( "cl_updaterate","20", FCVAR_ARCHIVE | FCVAR_USERINFO, "Number of packets per second of updates you are requesting from the server" );


ConVar  cl_forcepreload( "cl_forcepreload", "0", FCVAR_ARCHIVE, "Whether we should force preloading.");
static ConVar cl_logofile( "cl_logofile", "smscorch1", FCVAR_ARCHIVE, "Spraypoint logo decal." );
ConVar cl_LocalNetworkBackdoor( "cl_LocalNetworkBackdoor", "1", 0, "Enable network optimizations for single player games." );

client_static_t	cls;

char cl_moviename[256] = "\0";
int cl_movieframe = 0;

// FIXME: put these on hunk?
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];
dlight_t		cl_elights[MAX_ELIGHTS];

static int cl_snapshotnum;

// Must match game .dll definition
// HACK HACK FOR E3 -- Remove this after E3
#define	HIDEHUD_ALL			( 1<<2 )

const CPrecacheUserData* CL_GetPrecacheUserData( TABLEID tableID, int index )
{
	int testLength;
	const CPrecacheUserData *data = ( CPrecacheUserData * )networkStringTableContainerClient->GetStringUserData( tableID, index, &testLength );
	if ( data )
	{
		ErrorIfNot( 
			testLength == sizeof( *data ),
			("CL_GetPrecacheUserData(%d,%d) - length (%d) invalid.", (int)tableID, index, testLength)
		);

	}
	return data;
}


void CL_HideHud_f( void )
{
	unsigned char c;

	if ( Cmd_Argc() != 2 )
	{
		Con_Printf( "Usage:  hud <on | off>\nShows or hides the hud\n" );
	}

	if ( !stricmp( Cmd_Argv(1), "off" ) )
	{
		Con_Printf( "hiding hud\n" );
		c = HIDEHUD_ALL;
		DispatchDirectUserMsg("HideWeapon", 1, (void *)&c);
	}
	else if ( !stricmp( Cmd_Argv(1), "on" ) )
	{
		c = 0;
		Con_Printf( "showing hud\n" );
		DispatchDirectUserMsg("HideWeapon", 1, (void *)&c);
	}
	else
	{
		Con_Printf( "Unknown hud option %s\n", Cmd_Argv(1) );	
	}
}

//-----------------------------------------------------------------------------
// Purpose: If the client is in the process of connecting and the cls.signon hits
//  is complete, make sure the client thinks its totally connected.
//-----------------------------------------------------------------------------
void CL_CheckClientState( void )
{
	// Setup the local network backdoor (we do this each frame so it can be toggled on and off).
	CL_SetupLocalNetworkBackDoor( 
		cl_LocalNetworkBackdoor.GetInt() && 
		cls.netchan.remote_address.type == NA_LOOPBACK &&
		(sv.active && svs.maxclients == 1) &&
		!demo->IsRecording() );


	if ( ( cls.state == ca_connected ) &&
		 ( cls.signon == SIGNONS ) )
	{	
		// first update is the final signon stage
		cls.state = ca_active;

		// communicate to tracker that we're in a game
		{
			unsigned char *IPPtr = cls.netchan.remote_address.ip;
			short port = cls.netchan.remote_address.port;
			if (!port)
			{
				IPPtr = net_local_adr.ip;
				port = net_local_adr.port;
			}

			VGui_NotifyOfServerConnect(com_gamedir, *((int *)IPPtr), NET_NetToHostShort(port));
		}

		GetTestScriptMgr()->CheckPoint( "FinishedMapLoad" );

		char mapname[ MAX_OSPATH ];
		COM_FileBase( modelloader->GetName( host_state.worldmodel ), mapname );

		demo->CheckAutoRecord( mapname );
		demo->StartRendering();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Broadcast pings to any servers that we can see on our LAN
//-----------------------------------------------------------------------------
void CL_PingServers_f(void)
{
	netadr_t	adr;
	int			i;

	// send a broadcast packet
	Con_Printf ("Searching for local servers...\n");

	slist_time = realtime;

	NET_Config (true);		// allow remote	

	Q_memset( &adr, 0, sizeof( adr ) );

	for ( i = 0; i < 10; i++ )
	{
		adr.port = BigShort((unsigned short)(Q_atoi(PORT_SERVER) + i));

		if ( !noip )
		{
			adr.type = NA_BROADCAST;
			Netchan_OutOfBandPrint( NS_CLIENT, adr, "infostring" );
		}
	}
}

/*
==================
CL_Slist_f (void)

Populates the client's server_cache_t structrue
Replaces Slist command
==================
*/
void CL_Slist_f (void)
{
	num_servers = 0;
	memset(cached_servers, 0, MAX_LOCAL_SERVERS * sizeof(server_cache_t));

	// send out info packets
	// FIXME:  Record realtime when sent and determine ping times for responsive servers.
	CL_PingServers_f();
}

void CL_PrintCachedServer( int slot )
{
	server_cache_t *p;

	if ( slot < 0 || slot >= MAX_LOCAL_SERVERS )
		return;

	p = &cached_servers[ slot ];

	if ( !p->inuse )
		return;

	Con_Printf( "%i. ", slot + 1 );

	Con_Printf( "%s on map %s (%i/%i) at %s\n",
		Info_ValueForKey( p->info, "gamedir" ),
		Info_ValueForKey( p->info, "map" ),
		Q_atoi( Info_ValueForKey( p->info, "players" ) ),
		Q_atoi( Info_ValueForKey( p->info, "max" ) ),
		NET_AdrToString(p->adr) );

	Con_Printf ("    hostname:  \"%s\"\n",
		Info_ValueForKey( p->info, "hostname" )
		 );

	char const *desc = Info_ValueForKey( p->info, "desciption" );
	if ( desc && desc[ 0 ] )
	{
		Con_Printf( "%s\n", Info_ValueForKey( p->info, "desciption" ) );
	}

	bool indent = true;

	if ( *Info_ValueForKey( p->info, "type") == 'p' )	// is it a proxy ??
	{
		Con_Printf( "    proxy for %s - ",
			Info_ValueForKey( p->info, "proxyaddress" ) );

		indent = false;
	}
	else if ( Q_atoi( Info_ValueForKey( p->info, "proxytarget" ) ) )
	{
		Con_Printf( "    proxy target - " );
		indent = false;
	}

	if ( indent )
	{
		Con_Printf( "    " );
		indent = false;
	}
		
	switch ( *Info_ValueForKey( p->info, "type" ) )
	{
	case 'l':
		Con_Printf( "listen - " ); 
		break;
	case 'd':
		Con_Printf( "dedicated - " ); 
		break;
	case 'p':
		Con_Printf( "proxy - " ); 
		break;
	default:
		Con_Printf( "unknown server type - " ); 
		break;
	}

	if ( *Info_ValueForKey( p->info, "os") == 'w' )
	{
		Con_Printf( "win32 - " );
	}
	else
	{
		Con_Printf( "linux - " );
	}

	if ( Q_atoi( Info_ValueForKey( p->info, "password" ) ) == 1 )
	{
		Con_Printf( "password - " );
	}
	else
	{
		Con_Printf( "nopasswd - " );
	}

	if ( Q_atoi( Info_ValueForKey( p->info, "secure" ) ) == 1 )
	{
		Con_Printf( "secure" );
	}
	else
	{
		Con_Printf( "insecure" );
	}

	Con_Printf( "\n" );
}

/*
=================
CL_AddToServerCache

Adds the address, name to the server cache
=================
*/
void CL_AddToServerCache (netadr_t adr, char *info )
{
	int		i;

	// Too many servers.
	if ( num_servers >= MAX_LOCAL_SERVERS )   
	{
		return;
	}
	
	if ( cl_slisttimeout.GetFloat() > 0.0 )
	{
		if ( ( realtime - slist_time ) >= cl_slisttimeout.GetFloat())
		{
			return;
		}
	}
	
	 // Already cached by address
	for (i=0 ; i<num_servers ; i++)        
	{
		if ( NET_CompareAdr( cached_servers[i].adr, adr) )
		{
			return;
		}
	}

	// Display it.
	cached_servers[num_servers].inuse	= 1;
	cached_servers[num_servers].adr		= adr;

	Q_strncpy(cached_servers[num_servers].info, info, MAX_LOCAL_SERVERINFOSTRING );
	num_servers++;

	CL_PrintCachedServer( num_servers - 1 );
}

/*
==================
CL_ServiceInfoStringResponse

Handle a reply from a ping request that we broadcast via IP
=================
*/
void CL_ServiceInfoStringResponse ( void )
{
	char		*info;

	info = MSG_ReadString();

	if ( !info || !info[0] )
		return;

	CL_AddToServerCache ( net_from, info );
}

/*
===================
CL_ListCachedServers_f()

===================
*/
void CL_ListCachedServers_f (void)
{
	int i;
	server_cache_t *p;

	if (num_servers == 0)
	{
		Con_Printf("No local servers in list.\nTry 'slist' to search again.\n");
		return;
	}

	for (i=0 ; i<num_servers ; i++)
	{
		p = &cached_servers[i];
		if ( !p->inuse )
			continue;

		Con_Printf ("------------------\n");
		CL_PrintCachedServer( i );
	}

}

/*
================
CL_ConnectClient

Received connection initiation response from server, set up connection
================
*/
void CL_ConnectClient( void )
{
	// Already connected?
	if ( cls.state == ca_connected )
	{
		return;
	}

	// Initiate the network channel
	Netchan_Setup (NS_CLIENT, &cls.netchan, net_from );

	// Clear remaining lagged packets to prevent problems
	NET_ClearLagData( true, false );

	// Report connection success.
	if ( stricmp("loopback", NET_AdrToString (net_from) ) )
	{
		Con_Printf( "Connected to %s\n", NET_AdrToString (net_from) );
	}

	// Signon process will commence now that server ok'd connection.
	cls.netchan.message.WriteByte(clc_stringcmd);
	cls.netchan.message.WriteString ("new");	

	// Not in the demo loop now
	cls.demonum = -1;			

	// Mark client as connected
	cls.state = ca_connected;
	// Need all the signon messages before playing ( or drawing first frame )
	cls.signon  = 0;			
	
	// Bump connection time to now so we don't resend a connection
	// Request	
	cls.connect_time = realtime; 

	// Haven't gotten a valid frame update yet
	cl.validsequence = 0;

	// We'll request a full delta from the baseline
	cl.delta_sequence = -1;

	// We don't have a backed up cmd history yet
	cls.lastoutgoingcommand = -1;

	// We can send a cmd right away
	cls.nextcmdtime = realtime;

	// Start spooling demo info in case we record a demo on this level
	demo->StartupDemoHeader();
}

/*
=======================
CL_ClientDllQuery

=======================
*/
int CL_ClientDllQuery ( const char *s )
{
	int len;
	int maxsize = 2048;
	byte	data[2048];
	int valid;
	
	if ( cl.maxclients == 1 )   // ignore in single player
		return 0;

	Q_memset( data, 0, maxsize );

	// Allow game .dll to parse string
	len = maxsize - sizeof( int );
	valid = g_ClientDLL->ClientConnectionlessPacket( &net_from, s, (char *)( data + sizeof( int ) ), &len );
	if ( len && ( len <= ( maxsize - 4 ) ) )
	{
		// Always prepend connectionless / oob signal to this packet
		*(int *)&data[0] = -1;
		NET_SendPacket ( NS_CLIENT, len + 4, data, net_from );
	}

	return valid;
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket (void)
{
	char	*s;
	char    *cmd;
	char	*args;
	unsigned char c = 0;
	char	data[6];

	MSG_BeginReading ();
	// skip the -1 marker
	MSG_ReadLong ();		

	s = MSG_ReadStringLine ();

	args = s;

	Cmd_TokenizeString (s);

	cmd = Cmd_Argv(0);
	if ( cmd )
	{
		c = (unsigned char)cmd[0];
	}

	if ( !stricmp( cmd, "infostringresponse" ) )
	{
		CL_ServiceInfoStringResponse();
		return;
	}

	// FIXME:  For some of these, we should confirm that the sender of 
	// the message is what we think the server is...

	switch ( c )
	{
	case S2C_CONNECTION:
		if ( cls.state == ca_connecting )
		{
			CL_ConnectClient();
		}
		break;
		
	case S2C_CHALLENGE:
		// Response from getchallenge we sent to the server we are connecting to
		// Blow it off if we are not connected.
		if ( cls.state != ca_connecting )
			break;

		if ( Cmd_Argc() != 3 )
			return;

		s_connection.challenge = ( unsigned int )Q_atoi( Cmd_Argv(1) );
		s_connection.authprotocol = ( int )Q_atoi( Cmd_Argv(2) ) ;
		if ( s_connection.authprotocol == (unsigned char)-1 )
		{
			s_connection.authprotocol = PROTOCOL_HASHEDCDKEY;
		}
		
		if ( s_connection.authprotocol == PROTOCOL_AUTHCERTIFICATE )
		{
			// Make sure we have a certificate
			COM_CheckAuthenticationType();
			if ( !gfUseLANAuthentication )
			{
				Con_Printf( "The server requires that you authenticated.\nAuth not supported in this build.\n" );
				Host_Disconnect();
			}
			else
			{
				Con_Printf( "The server requires that you authenticated.\nCould not obtain authentication.\n" );
				Host_Disconnect();
			}
		}
		else
		{
			//Con_Printf( "Using Normal\n");
			CL_SendConnectPacket ();
		}
		break;
	case A2C_PRINT:
		MSG_BeginReading ();
		MSG_ReadLong ();		// skip the -1 marker
		MSG_ReadByte();

		Con_Printf ( "%s\n", MSG_ReadString() );
		break;
	case S2C_BADPASSWORD:
		if ( cls.state != ca_connecting )  // Spoofed?
			return;

		s++;

		if ( !Q_strncasecmp( s, "BADPASSWORD", Q_strlen( "BADPASSWORD" ) ) )
			s += Q_strlen( "BADPASSWORD" );

		Con_Printf (s);

		// Force bad pw dialog to come up now.
		COM_ExplainDisconnection( false, "BADPASSWORD" );
		Con_Printf( "Invalid server password.\n" );
		Host_Disconnect();
		break;
	case S2C_CONNREJECT:
		if ( cls.state != ca_connecting )  // Spoofed?
			return;
		
		s++;

		// Force failure dialog to come up now.
		COM_ExplainDisconnection( true, s );
		Host_Disconnect();
		break;
	case A2A_PING:
		data[0] = -1;
		data[1] = -1;
		data[2] = -1;
		data[3] = -1;
		data[4] = A2A_ACK;
		data[5] = 0;
		NET_SendPacket (NS_CLIENT, 6, &data, net_from);
		break;
	case A2A_ACK:
		break;
	/*
	case S2A_INFO_DETAILED:
		MSG_BeginReading ();
		MSG_ReadLong ();		// skip the -1 marker
		MSG_ReadByte();
		CL_ParseServerInfoResponse();
		break;
	*/
	// Unknown?
	default:
		// Does the client .dll want to accept it?
		if ( CL_ClientDllQuery( args ) )
		{
			break;
		}

		// Otherwise, don't do anything.
		Con_DPrintf ( "CL_ConnectionlessPacket:  Unknown command:\n%c\n", c );
		break;
	}
}

/*
====================
CL_ProcessFile

A file has been received via the fragmentation/reassembly layer, put it in the right spot and
 see if we have finished downloading files.
====================
*/
void CL_ProcessFile( bool successfully_received, const char *filename )
{
	// Con_Printf( "Received %s, but file processing is not hooked up!!!\n", filename );
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
int CL_GetMessage (void)
{
	if ( demo->IsPlayingBack() )
	{
		if ( demo->ReadMessage() )
			return true;

		return false;
	}

	if (!NET_GetPacket (NS_CLIENT))
		return false;
	
	return true;
}

/*
=================
CL_ReadPackets

Updates the local time and reads/handles messages on client net connection.
=================
*/
void CL_ReadPackets ( bool framefinished )
{
	if( cls.state == ca_dedicated )
		return;

	MEASURECODE( "CL_ReadPackets" );

	int msg_count = 0;

	if ( !Host_ShouldRun() )
		return;
	
	cl.oldtickcount = cl.tickcount;
	if ( !demo->IsPlaybackPaused() )
	{
		cl.tickcount++;
		g_ClientGlobalVariables.tickcount = cl.tickcount;
		g_ClientGlobalVariables.curtime = cl.gettime();
	}
	// 0 or tick_rate if simulating
	g_ClientGlobalVariables.frametime = cl.getframetime();

	cl.continue_reading_packets = true;

	while ( CL_GetMessage() )
	{	
		if ( !demo->IsPlayingBack() )
		{
			// Swallow and discard all remaining incoming packets from stream (and don't write them into .dem file either!!!)
			if ( !cl.continue_reading_packets )
				continue;

			msg_count++;
			if ( msg_count > MAX_INCOMING_MESSAGES )
			{
				// Ignore rest of messages this frame, we won't be able to uncompress the delta
				//  if there is one!!!
				continue;
			}
		}
		
		//
		// remote command packet
		//
		if (*(int *)net_message.data == -1)
		{
			CL_ConnectionlessPacket ();
			continue;
		}

		// Sequenced message before fully connected, throw away.
		if ( cls.state == ca_disconnected || 
			 cls.state == ca_connecting )
			continue;		// dump sequenced message from server if not connected yet

		if (net_message.cursize < 8)
		{
			Con_Printf ("%s: Runt packet\n",NET_AdrToString(net_from));
			continue;
		}

		//
		// packet from server, verify source IP address.
		//
		if (!demo->IsPlayingBack() && 
			!NET_CompareAdr (net_from, cls.netchan.remote_address))
		{
			Con_Printf ("%s:sequenced packet without connection\n"
				,NET_AdrToString(net_from));
			continue;
		}

		// Do we have the bandwidth to process this packet and are we
		//  up to date on the reliable message stream.
		if (!demo->IsPlayingBack() && !Netchan_Process(&cls.netchan))
			continue;		// wasn't accepted for some reason

		if (demo->IsPlayingBack())
		{
			MSG_BeginReading();
		}

		// Parse out the commands.
		CL_ParseServerMessage();
	}

	if ( msg_count > MAX_INCOMING_MESSAGES )
	{
		Con_DPrintf( "Ignored %i messages\n", msg_count - MAX_INCOMING_MESSAGES );
	}

	// Check for fragmentation/reassembly related packets.
	if ( cls.state != ca_dedicated &&
		 cls.state != ca_disconnected && 
		 Netchan_IncomingReady( &cls.netchan )  )
	{

		// Process the incoming buffer(s)
		if ( Netchan_CopyNormalFragments( &cls.netchan) )
		{
			MSG_BeginReading();
			CL_ParseServerMessage( false );
		}
		
		if ( Netchan_CopyFileFragments( &cls.netchan ) )
		{
			// Remove from resource request stuff.
			CL_ProcessFile( true, cls.netchan.incomingfilename );
		}
	}

	// check timeout, but not if running _DEBUG engine
#if !defined( _DEBUG )
	// Only check on final frame because that's when the server might send us a packet in single player.  This avoids
	//  a bug where if you sit in the game code in the debugger then you get a timeout here on resuming the engine
	//  because the timestep is > 1 tick because of the debugging delay but the server hasn't sent the next packet yet.  ywb 9/5/03
	if ( framefinished &&   
		( cls.state >= ca_connected ) &&
		 ( !demo->IsPlayingBack() ) && 
		 ( (realtime - cls.netchan.last_received ) > cl_timeout.GetFloat() ) )
	{
		Con_Printf ("\nServer connection timed out.\n");
		Host_Disconnect ();
		return;
	}
#endif

	if (cl_shownet.GetInt())
		Con_Printf ("\n");

	Netchan_UpdateProgress( &cls.netchan );

	CL_PostReadMessages();
}

/*
=====================
Clear cl.frames.
=====================
*/
void CL_ClearClientFrames()
{
	int i;
	CRecvEntList *pClientPack;

	for (i = 0; i < CL_UPDATE_BACKUP; i++)
	{
		pClientPack = &cl.frames[i].packet_entities; 
		pClientPack->Term();
	}

	memset( cl.frames, 0, MULTIPLAYER_BACKUP * sizeof( frame_t ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_ClearState ( void )
{
	if ( cls.state == ca_dedicated )
		return;

	CL_ResetEntityBits();

	R_UnloadSkys();

	StaticPropMgr()->LevelShutdownClient();

	// shutdown this level in the client DLL
	if ( g_ClientDLL )
	{
		g_ClientDLL->LevelShutdown();
	}

	cls.signon = 0;

	cls.netchan.message.Reset();
	
	// clear other arrays	
	memset (cl_dlights, 0, sizeof(cl_dlights));
	memset (cl_elights, 0, sizeof(cl_elights));
	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));

	FreeEntityBaselines();

	CL_ClearClientFrames();

	// Free datatable stuff.
	delete[] cl.m_pServerClasses;
	cl.m_pServerClasses = NULL;
	
	RecvTable_Term( false );

	// Wipe the hunk ( unless the server is active )
	Host_FreeToLowMark( false );

	// Wipe the remainder of the structure.
	cl.Clear();
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
	if ( cls.state == ca_dedicated )
	{
		return;
	}

	cls.connect_time  = -99999;
	s_connection.retrynumber = 0;

	// stop sounds (especially looping!)
	S_StopAllSounds (true);
	
	demo->StopPlayback();
	demo->StopRecording();

	if ( cls.state == ca_disconnected )
		return;

	if ( cls.netchan.remote_address.type != NA_UNUSED )
	{
		byte	final[20];

		// Send a drop command.
		final[0] = clc_stringcmd;                         
		strcpy ((char*)(final+1), "dropclient\n");
		Netchan_Transmit( &cls.netchan, 13, final );
		Netchan_Transmit( &cls.netchan, 13, final );
		Netchan_Transmit( &cls.netchan, 13, final );
	}

	cls.state = ca_disconnected;
	cls.signon = 0;

	CL_ClearState();
	
	// Clear the network channel, too.
	Netchan_Clear( &cls.netchan );

	// notify game ui dll of out-of-in-game status
	VGui_NotifyOfServerDisconnect();
}

// HL1 CD Key
#define GUID_LEN 13

/*
=======================
CL_GetCDKeyHash()

Connections will now use a hashed cd key value
A LAN server will know not to allows more then xxx users with the same CD Key
=======================
*/
char *CL_GetCDKeyHash( void )
{
	char szKeyBuffer[256]; // Keys are about 13 chars long.	
	static char szHashedKeyBuffer[256];
	int nKeyLength;
	qboolean bDedicated = false;
	MD5Context_t ctx;
	unsigned char digest[16]; // The MD5 Hash

	nKeyLength = Q_snprintf( szKeyBuffer, sizeof( szKeyBuffer ), "%s", registry->ReadString( "key", "" ) );

	if (bDedicated)
	{
		Con_Printf("Key has no meaning on dedicated server...\n");
		return "";
	}

// HACK HACK:  This all goes away on Steam, but we need to have a non-zero cd key if the registry is
//  blank so just fill in a fake one
#if 1
	if ( nKeyLength == 0 )
	{
		nKeyLength = 13;
		strcpy( szKeyBuffer, "1234567890123" );
		Assert( Q_strlen( szKeyBuffer ) == nKeyLength );

		Con_Printf( "Missing CD Key from registry, using fake cd key 1234567890123 for now...\n" );

		registry->WriteString( "key", szKeyBuffer );
	}
#endif

	if (nKeyLength <= 0 ||
		nKeyLength >= 256 )
	{
		Con_Printf("Bogus key length on CD Key...\n");
		return "";
	}

	// Now get the md5 hash of the key
	memset( &ctx, 0, sizeof( ctx ) );
	memset( digest, 0, sizeof( digest ) );
	
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*)szKeyBuffer, nKeyLength);
	MD5Final(digest, &ctx);
	memset ( szHashedKeyBuffer, 0, 256 );
	strcpy ( szHashedKeyBuffer, MD5_Print ( digest ) );
	return szHashedKeyBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *protinfo - 
//			length - 
//-----------------------------------------------------------------------------
void CL_CheckLogoFile( char *protinfo, int length )
{
	char logotexture[ 512 ];
	Q_snprintf( logotexture, sizeof( logotexture ), "materials/decals/%s.vtf", cl_logofile.GetString() );
	if ( !g_pFileSystem->FileExists( logotexture ) )
		return;

	// Compute checksum of texture
	CRC32_t crc;
	CRC_File( &crc, logotexture );

	char crcfilename[ 512 ];

	Q_snprintf( crcfilename, sizeof( crcfilename ), "materials/decals/downloads/%s.vtf",
		COM_BinPrintf( (byte *)&crc, 4 ) );

	if ( !g_pFileSystem->FileExists( crcfilename ) )
	{
		// Copy it over under the new name
		COM_CopyFile( logotexture, crcfilename );
	}
	
	if ( !g_pFileSystem->FileExists( crcfilename ) )
	{
		return;
	}

	CRC32_t test;
	CRC_File( &test, crcfilename );

	Assert( test == crc );

	// FIXME: Validate the .vtf file!!!

	// Send the CRC of the logo file
	Info_SetValueForKey( protinfo, "logo", COM_BinPrintf( (byte *)&crc, 4 ), length );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *protinfo - 
//			length - 
//-----------------------------------------------------------------------------
void CL_CheckSendTableCRC( char *protinfo, int length )
{
	// Game .dll wasn't loaded...?
	CRC32_t crc = Host_GetServerSendtableCRC();
	if ( crc == (CRC32_t)0 )
	{
		Assert( 0 );
		return;
	}

	Info_SetValueForKey( protinfo, "tablecrc", COM_BinPrintf( (byte *)&crc, 4 ), length );
}

//-----------------------------------------------------------------------------
// Purpose: called by CL_Connect and CL_CheckResend
// If we are in ca_connecting state and we have gotten a challenge
//   response before the timeout, send another "connect" request.
// Output : void CL_SendConnectPacket
//-----------------------------------------------------------------------------
void CL_SendConnectPacket (void)
{
	netadr_t adr;
	char	data[2048];
	char szServerName[MAX_OSPATH];
	int len;
	unsigned char buffer[1024];
	char protinfo[ 1024 ];

	Q_memset( protinfo, 0, sizeof( protinfo ) );

	Q_strncpy(szServerName, cls.servername, MAX_OSPATH);

	// Deal with local connection.
	if ( !stricmp( cls.servername, "local" ) )
	{
		Q_snprintf(szServerName, sizeof( szServerName ), "%s", "localhost");
	}

	if (!NET_StringToAdr (szServerName, &adr))
	{
		Con_Printf ("Bad server address\n");
		Host_Disconnect();
		return;
	}

	if ( adr.port == (unsigned short)0 )
	{
		adr.port = BigShort( (unsigned short)atoi(PORT_SERVER) );
	}

	switch ( s_connection.authprotocol )
	{
	default:
		s_connection.authprotocol = PROTOCOL_HASHEDCDKEY;
		// Fall through, bogus protocol type, use CD key hash.
	case PROTOCOL_HASHEDCDKEY:
		len = strlen( CL_GetCDKeyHash() );
		strcpy( (char*)buffer, CL_GetCDKeyHash() );
		break;
	case PROTOCOL_AUTHCERTIFICATE:
		Host_Error( "PROTOCOL_AUTHCERTIFICATE, unexepected!\n" );
		return;
	}

	Info_SetValueForKey( protinfo, "prot", va( "%i", s_connection.authprotocol ), 1024 );
	Info_SetValueForKey( protinfo, "raw", (char *)buffer, 1024 );

	CL_CheckLogoFile( protinfo, sizeof( protinfo ) );
	CL_CheckSendTableCRC( protinfo, sizeof( protinfo ) );

	Q_snprintf (data, sizeof( data ), "%c%c%c%cconnect %i %i \"%s\" \"%s\"\n", 255, 255, 255, 255, 
		PROTOCOL_VERSION, s_connection.challenge, protinfo, cls.userinfo );  // Send protocol and challenge value

	// Mark time of this attempt for retransmit requests
	cls.connect_time = realtime;
	
	NET_SendPacket (NS_CLIENT, strlen(data), data, adr);
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend (void)
{
	netadr_t	adr;
	char	data[2048];
	char szServerName[MAX_OSPATH];

	// resend if we haven't gotten a reply yet
	// We only resend during the connection process.
	if (cls.state != ca_connecting)
		return;

	// Wait at least the resend # of seconds.
	if ( ( realtime - cls.connect_time ) < cl_resend.GetFloat())
		return;

	Q_strncpy(szServerName, cls.servername, MAX_OSPATH);

	// Deal with local connection.
	if (!stricmp(cls.servername, "local"))
		Q_snprintf(szServerName, sizeof( szServerName ), "%s", "localhost");

	if (!NET_StringToAdr (szServerName, &adr))
	{
		Con_Printf ("Bad server address\n");
		Host_Disconnect();
		return;
	}
	if (adr.port == 0)
	{
		adr.port = BigShort ((unsigned short)atoi(PORT_SERVER));
	}

	// Only retry so many times before failure.
	if ( s_connection.retrynumber >= CL_CONNECTION_RETRIES )
	{
		COM_ExplainDisconnection( true, "Connection failed after %i retries.\n", CL_CONNECTION_RETRIES );
		Host_Disconnect();
		return;
	}
	
	// Mark time of this attempt.
	cls.connect_time = realtime;	// for retransmit requests

	// Display appropriate message
	if (stricmp(szServerName, "localhost"))
	{
		if (s_connection.retrynumber == 0)
			Con_Printf ("Connecting to %s...\n", szServerName);
		else
			Con_Printf ("Retrying %s...\n", szServerName);
	}

	s_connection.retrynumber++;

	// Request another challenge value.
	Q_snprintf (data, sizeof( data ), "%c%c%c%cgetchallenge\n",
		255, 255, 255, 255);
	
	// Send the request.
	NET_SendPacket (NS_CLIENT, strlen(data), data, adr);
}


/*
================
CL_Retry_f

  Retry last connection (e.g., after we enter a password)

================
*/
void CL_Retry_f ( void )
{
	if ( !cls.retry_address[ 0 ] )
	{
		Con_Printf( "Can't retry, no previous connection\n" );
		return;
	}

	Con_Printf( "Commencing connection retry to %s\n", cls.retry_address );
	Cbuf_AddText( va( "connect %s\n", cls.retry_address ) );
}

/*
=====================
CL_Connect_f

User command to connect to server
=====================
*/
void CL_Connect_f (void)
{
	if ( Cmd_Argc() < 2 )
	{
		Con_Printf( "Usage:  connect <server>\n" );
		return;
	}

	// Disconnect from current server
	// Don't call Host_Disconnect, because we don't want to shutdown the listen server!
	CL_Disconnect();

	// Get new server name
	Q_strncpy( cls.servername, Cmd_Args(), sizeof(cls.servername) );

	int num = atoi( cls.servername );  // In case it's an index.

	// Check the cached_servers list for a text match or index match:
	if ( ( num > 0 ) && ( num <= num_servers ) && !Q_strstr( cls.servername , "." ) )
	{
		Q_strncpy( cls.servername, NET_AdrToString(cached_servers[num - 1].adr), sizeof(cls.servername)-1 );
	}

	// Store off the last address we tried to get to
	strcpy ( cls.retry_address, cls.servername );

	// For the check for resend timer to fire a connection / getchallenge request.
	cls.state = ca_connecting;
	// Force connection request to fire.
	cls.connect_time = -99999;  

	s_connection.retrynumber = 0;

	// Reset error conditions
	gfExtendedError = false;

	// If it's not a single player connection, initialize networking
	if ( strnicmp( cls.servername, "local", 5 ) )
	{
		// allow remote
		NET_Config (true);		
	}
}

//-----------------------------------------------------------------------------
// Purpose: This command causes the client to wait for the signon messages again.
// This is sent just before a server changes levels
// Output : void CL_Reconnect_f
//-----------------------------------------------------------------------------
void CL_Reconnect_f (void)
{
	if (cls.state == ca_dedicated )
	{
		return;
	}
	if ( cls.state == ca_disconnected ||
		cls.state == ca_connecting )
	{
		if ( cls.retry_address[ 0 ] )
		{
			CL_Retry_f();
		}
		return;
	}

	SCR_BeginLoadingPlaque ();

	cls.signon = 0;		// need new connection messages
	cls.state = ca_connected;	// not active anymore, but not disconnected

	// Clear channel and stuff
	Netchan_Clear( &cls.netchan );
	
	cls.netchan.message.Reset();

	// Write a "new" command into client's buffer.
	cls.netchan.message.WriteChar (clc_stringcmd);
	cls.netchan.message.WriteString ("new");
}

//-----------------------------------------------------------------------------
// Takes the map name, strips path and extension
//-----------------------------------------------------------------------------

static void CL_SetupMapName( char const* pName, char* pFixedName )
{
	const char* pSlash = strrchr( pName, '\\' );
	const char* pSlash2 = strrchr( pName, '/' );
	if (pSlash2 > pSlash)
		pSlash = pSlash2;
	if (pSlash)
		++pSlash;
	else 
		pSlash = pName;

	strcpy( pFixedName, pSlash );
	char* pExt = strchr( pFixedName, '.' );
	if (pExt)
		*pExt = 0;
}

//-----------------------------------------------------------------------------
// Purpose: A svc_signonnum has been received, perform a client side setup
// Output : void CL_SignonReply
//-----------------------------------------------------------------------------
void CL_SignonReply (void)
{
	char 	str[8192];

//	Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		{
			cls.netchan.message.WriteByte (clc_stringcmd);
			Q_snprintf ( str, sizeof( str ), "spawn %i", cl.servercount );
			cls.netchan.message.WriteString (str);
		}
		break;
		
	case 2:	
		{
			cls.netchan.message.WriteByte (clc_stringcmd);
			Q_snprintf( str, sizeof( str ), "begin %i", cl.servercount );
			cls.netchan.message.WriteString ( str );
			Cache_Report ();		        // print remaining memory
			// Tell client .dll about the transition
			char mapname[256];

			CL_SetupMapName( host_state.worldmodel->name, mapname );
			g_ClientDLL->LevelInitPreEntity(mapname);
		}

		break;
		
	case 3:
		{
			SpatialPartition()->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

			// This has to happen here, in phase 3, because it is in this phase
			// that raycasts against the world is supported (owing to the fact
			// that the world entity has been created by this point)
			StaticPropMgr()->LevelInitClient();
 			g_ClientDLL->LevelInitPostEntity();

			SpatialPartition()->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );

			SCR_EndLoadingPlaque ();		// allow normal screen updates

			if ( developer.GetInt() > 0 )
			{
				Netchan_ReportFlow( &cls.netchan );
			}
		}
		break;
	}
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	SCR_BeginLoadingPlaque ();

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
			scr_disabled_for_loading = false;

			Con_Printf ("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	Q_snprintf (str,sizeof( str ), "playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}

/*
===============
CL_TakeSnapshot_f

Generates a filename and calls the vidwin.c function to write a .bmp file
===============
*/

static bool cl_takesnapshot = false;

static char cl_snapshotname[MAX_OSPATH];

void CL_TakeSnapshot_f( void )
{
	// We'll take a snapshot at the next available opportunity
	cl_takesnapshot = true;
	if( Cmd_Argc() == 2 )
	{
		Q_strncpy( cl_snapshotname, Cmd_Argv( 1 ), sizeof( cl_snapshotname ) );		
	}
	else
	{
		cl_snapshotname[0] = 0;
	}
}

void CL_TakeSnapshotAndSwap()
{
	bool bReadPixelsFromFrontBuffer = g_pMaterialSystemHardwareConfig->ReadPixelsFromFrontBuffer();
	if( bReadPixelsFromFrontBuffer )
	{
		Shader_SwapBuffers();
	}
	
	if (cl_takesnapshot)
	{
		char base[ MAX_OSPATH ];
		char filename[ MAX_OSPATH ];
		IClientEntity *world = entitylist->GetClientEntity( 0 );

		g_pFileSystem->CreateDirHierarchy( "screenshots", "GAME" );
		if( cl_snapshotname[0] )
		{
			Q_strncpy( base, cl_snapshotname, sizeof( base ) );
			Q_snprintf( filename, sizeof( filename ), "screenshots/%s.tga", base );
		}
		else
		{
			if( world && world->GetModel() )
			{
				COM_FileBase( modelloader->GetName( ( model_t *)world->GetModel() ), base );
			}
			else
			{
				strcpy( base, "Snapshot" );
			}
			while( 1 )
			{
				Q_snprintf( filename, sizeof( filename ), "screenshots/%s%04d.tga", base, cl_snapshotnum++ );
				if( !g_pFileSystem->GetFileTime( filename ) )
				{
					// woo hoo!  The file doesn't exist already, so use it.
					break;
				}
			}
		}

		VID_TakeSnapshot( filename );

		cl_takesnapshot = false;
	}

	// If recording movie and the console is totally up, then write out this frame to movie file.
	if ( cl_moviename[0] && Con_IsVisible() )
	{
		VID_WriteMovieFrame();
	}

	if( !bReadPixelsFromFrontBuffer )
	{
		Shader_SwapBuffers();
	}
}


/*
===============
CL_StartMovie_f

Sets the engine up to dump frames
===============
*/
void CL_StartMovie_f( void )
{
	if (cmd_source != src_command)
		return;

	if( Cmd_Argc() != 2 )
	{
		Con_Printf ("startmovie <filename>\n");
		return;
	}

	strcpy( cl_moviename, Cmd_Argv( 1 ) );
	cl_movieframe = 0;

	SND_MovieStart();
	Con_Printf( "Started recording movie, frames will record after console is cleared...\n" );
}


/*
===============
CL_EndMovie_f

Ends frame dumping
===============
*/

void CL_EndMovie_f( void )
{
	if( !cl_moviename[0] )
	{
		Con_Printf( "No movie started.\n" );
	}
	else
	{
		SND_MovieEnd();
		cl_moviename[0] = 0;
		cl_movieframe = 0;
		Con_Printf( "Stopped recording movie...\n" );
	}
}

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f (void)
{
	char	message[1024];   // Command message
	char    szParam[ 256 ];
	int		i;
	netadr_t to;    // Command destination
	unsigned short nPort;    // Outgoing port.

	if (!rcon_password.GetString())
	{
		Con_Printf ("You must set 'rcon_password' before\n"
					"issuing an rcon command.\n");
		return;
	}

	NET_Config (true);		// allow remote	

	// If we are connected to a server, send rcon to it, otherwise
	//  send rcon commaand to whereever we specify in rcon_address, if possible.
	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address.GetString()))
		{
			Con_Printf ("You must either be connected,\n"
						"or set the 'rcon_address' cvar\n"
						"to issue rcon commands\n");

			return;
		}
		if ( !NET_StringToAdr( rcon_address.GetString(), &to) )
		{
			Con_Printf( "Unable to resolve rcon address %s\n", rcon_address.GetString() );
			return;
		}
	}

	nPort = (unsigned short)rcon_port.GetInt();  // ?? BigShort
	if (!nPort)
		nPort = atoi(PORT_SERVER); //DEFAULTnet_hostport;

	to.port = BigShort(nPort);

	Q_snprintf(message, sizeof( message ), "rcon \"%s\" ", rcon_password.GetString());

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		Q_snprintf( szParam, sizeof( szParam ), "\"%s\"", Cmd_Argv(i) );
		strcat( message, szParam );
		if ( i != ( Cmd_Argc() - 1 ) )
			strcat (message, " ");
	}

	Netchan_OutOfBandPrint( NS_CLIENT, to, "%s", message);
}

void CL_DebugBox_f()
{
	if( Cmd_Argc() != 7 )
	{
		Con_Printf ("box x1 y1 z1 x2 y2 z2\n");
		return;
	}

	Vector mins, maxs;
	for (int i = 0; i < 3; ++i)
	{
		mins[i] = atof(Cmd_Argv(i + 1)); 
		maxs[i] = atof(Cmd_Argv(i + 4)); 
	}
	CDebugOverlay::AddBoxOverlay( vec3_origin, mins, maxs, vec3_angle, 255, 0, 0, 0, 100 );
}

/*
==============
CL_View_f  

Debugging changes the view entity to the specified index
===============
*/
void CL_View_f (void)
{
	int nNewView;

	if( Cmd_Argc() != 2 )
	{
		Con_Printf ("cl_view entity#\nCurrent %i\n", cl.viewentity);
		return;
	}

	nNewView = atoi(Cmd_Argv(1));
	if (!nNewView)
		return;

	if ( nNewView > entitylist->GetHighestEntityIndex() )
		return;

	cl.viewentity = nNewView;
	vid.recalc_refdef = true; // Force recalculation
	Con_Printf("View entity set to %i\n", nNewView);
}

/*
===============
CL_AllocDlight

===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				memset (dl, 0, sizeof(*dl));
				dl->key = key;
				r_dlightchanged |= (1 << i);
				r_dlightactive |= (1 << i);
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.gettime())
		{
			memset (dl, 0, sizeof(*dl));
			dl->key = key;
			r_dlightchanged |= (1 << i);
			r_dlightactive |= (1 << i);
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset (dl, 0, sizeof(*dl));
	dl->key = key;
	r_dlightchanged |= (1 << 0);
	r_dlightactive |= (1 << 0);
	return dl;
}


/*
===============
CL_AllocElight

===============
*/
dlight_t *CL_AllocElight (int key)
{
	int		i;
	dlight_t	*el;

// first look for an exact key match
	if (key)
	{
		el = cl_elights;
		for (i=0 ; i<MAX_ELIGHTS ; i++, el++)
		{
			if (el->key == key)
			{
				memset (el, 0, sizeof(*el));
				el->key = key;
				return el;
			}
		}
	}

// then look for anything else
	el = cl_elights;
	for (i=0 ; i<MAX_ELIGHTS ; i++, el++)
	{
		if (el->die < cl.gettime())
		{
			memset (el, 0, sizeof(*el));
			el->key = key;
			return el;
		}
	}

	el = &cl_elights[0];
	memset (el, 0, sizeof(*el));
	el->key = key;
	return el;
}


/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;
	float		time;
	
	time = cl.getframetime();
	if ( time <= 0.0f )
		return;

	dl = cl_dlights;

	r_dlightchanged = 0;
	r_dlightactive = 0;

	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->radius != 0)
		{
			if (dl->die < cl.gettime())
			{
				r_dlightchanged |= (1 << i);
				dl->radius = 0;
			}
			else if (dl->decay)
			{
				r_dlightchanged |= (1 << i);

				dl->radius -= time*dl->decay;
				if (dl->radius < 0)
					dl->radius = 0;
			}
			if (dl->radius != 0)
			{
				r_dlightactive |= (1 << i);
			}
		}
	}

	dl = cl_elights;
	for (i=0 ; i<MAX_ELIGHTS ; i++, dl++)
	{
		if (!dl->radius)
			continue;
		if (dl->die < cl.gettime())
		{
			dl->radius = 0;
			continue;
		}
		
		dl->radius -= time*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}

#define CL_PACKETLOSS_RECALC 1.0
/*
=================
CL_ComputePacketLoss

=================
*/
void CL_ComputePacketLoss( void )
{
	int i;
	int frm;
	frame_t *frame;
	int count = 0;
	int lost = 0;

	if ( realtime < cls.packet_loss_recalc_time )
		return;

	cls.packet_loss_recalc_time = realtime + CL_PACKETLOSS_RECALC;

	// Compuate packet loss
	// Fill in frame data
	for ( i = cls.netchan.incoming_sequence-CL_UPDATE_BACKUP+1
		; i <= cls.netchan.incoming_sequence
		; i++)
	{
		frm = i;
		frame = &cl.frames[ frm &CL_UPDATE_MASK];

		if (frame->receivedtime == -1)
		{
			lost++;
		}	
		count++;
	}

	if ( count <= 0 )
	{
		cls.packet_loss = 0.0;
	}
	else
	{
		cls.packet_loss = ( 100.0 * (float)lost ) / (float)count;
	}
}

void CL_ExtraMouseUpdate( float frametime )
{
	// Not ready for commands yet.
	if (cls.state == ca_dedicated || 
		cls.state == ca_disconnected ||
		cls.state == ca_connecting )
		return;

	if ( !Host_ShouldRun() )
		return;

	// Don't create usercmds here during playback, they were encoded into the packet already
	if ( demo->IsPlayingBack() )
		return;

	// Are we fully signed on?
	int active = !demo->IsPlayingBack() && ( cls.signon == SIGNONS ) ? 1 : 0;

	// Have client .dll create and store usercmd structure
	g_ClientDLL->ExtraMouseSample( frametime, active );

}

/*
=================
CL_Move

Constructs the movement command and sends it to the server if it's time.
=================
*/
bool SV_IsSimulating( void );
void CL_Move( float accumulated_extra_samples )
{
	VPROF( "CL_Move" );
	
	byte			data[ MAX_CMD_BUFFER ];
	int				i;
	int				from, to;
	cmd_t			*pcmd;
	int				numbackup = 2;
	int				numcmds;
	int				newcmds;
	int				cmdnumber;
	int				active = 0;

	// Not ready for commands yet.
	if (cls.state == ca_dedicated || 
		cls.state == ca_disconnected ||
		cls.state == ca_connecting )
		return;

	if ( !Host_ShouldRun() )
		return;

	// Don't create usercmds here during playback, they were encoded into the packet already
	if ( demo->IsPlayingBack() )
		return;

	/*
	// This is a bit of a hack/cluttering of server and client code, but we don't want to accumulate usercmds when we
	//  are paused with the console showing
	if ( sv.active && sv.paused )
	{
		cls.lastoutgoingcommand++;
		Netchan_Transmit( &cls.netchan, NULL, NULL );	
		return;
	}
	*/

	CL_ComputePacketLoss();

	bf_write buf( "CL_Move", data, sizeof( data ) );

	// Message we are constructing.
	i		= cls.netchan.outgoing_sequence & CL_UPDATE_MASK;   

	pcmd = &cl.commands[ i ];

	pcmd->senttime			= demo->IsPlayingBack() ? 0.0 : realtime;      
	pcmd->receivedtime		= -1;
	pcmd->processedfuncs	= false;
	// Assume we'll transmit this command this frame ( rather than queueing it for next frame )
	pcmd->heldback			= false;
	pcmd->sendsize			= 0;

	// Are we fully signed on?
	active =  !demo->IsPlayingBack() && ( cls.signon == SIGNONS ) ? 1 : 0;

	// Have client .dll create and store usercmd structure
	g_ClientDLL->CreateMove( 
		cls.netchan.outgoing_sequence, 
		CL_UPDATE_BACKUP, 
		TICK_RATE, 
		TICK_RATE - accumulated_extra_samples,
		active, 
		cls.packet_loss );

	// Determine number of backup commands to send along
	numbackup = cl_cmdbackup.GetInt();
	if ( numbackup < 0 )
	{
		numbackup = 0;
	}
	else if ( numbackup > MAX_BACKUP_COMMANDS )
	{
		numbackup = MAX_BACKUP_COMMANDS;
	}


	// Write a clc_move message
	buf.WriteByte( clc_move );
	buf.WriteOneBit( Voice_GetLoopback() ); // tell the server if we want loopback on.

#if !defined( _DEBUG )
	// In release build, validate the cl_cmdrate
	if ( cl_cmdrate.GetInt() < MIN_CMD_RATE )
	{
		cl_cmdrate.SetValue( (float)MIN_CMD_RATE );
	}
#endif

	// Check to see if we can actually send this command
	//
	// In single player, send commands as fast as possible
	// Otherwise, only send when ready and when not choking bandwidth
	if ( cl.force_send_usercmd || 
		( cl.maxclients == 1 ) || 
		 ( ( cls.netchan.remote_address.type == NA_LOOPBACK ) && !host_limitlocal.GetInt() ) ||
		( ( realtime >= cls.nextcmdtime ) && Netchan_CanPacket( &cls.netchan ) ) )
	{
		cl.force_send_usercmd = false;

		if ( cl_cmdrate.GetInt() > 0 )
		{
			cls.nextcmdtime = realtime + ( 1.0f / cl_cmdrate.GetFloat() );
		}
		else
		{
			// Always able to send right away
			cls.nextcmdtime = realtime;
		}

		int outgoing_sequence;
		if ( cls.lastoutgoingcommand == -1 )
		{
			outgoing_sequence		= cls.netchan.outgoing_sequence;
			cls.lastoutgoingcommand = cls.netchan.outgoing_sequence;
		}
		else
		{
			outgoing_sequence		= cls.lastoutgoingcommand + 1;
		}
		
		// Say how many backups we'll be sending
		buf.WriteUBitLong( numbackup, NUM_BACKUP_COMMAND_BITS );

		// How many real commands have queued up
		newcmds = ( cls.netchan.outgoing_sequence - cls.lastoutgoingcommand );

		// Put an upper/lower bound on this
		newcmds = clamp( newcmds, 0, MAX_TOTAL_CMDS );

		buf.WriteByte( newcmds );

		numcmds = newcmds + numbackup;

		from = -1;
		for ( i = numcmds - 1; i >= 0; i-- )
		{
			cmdnumber = ( cls.netchan.outgoing_sequence - i ) & CL_UPDATE_MASK;
			to = cmdnumber;

			int startbit = buf.GetNumBitsWritten();

			bool isnewcmd = ( i < newcmds ) ? true : false;

			g_ClientDLL->WriteUsercmdDeltaToBuffer( &buf, from, to, isnewcmd );
		
			int endbit = buf.GetNumBitsWritten();

			if ( buf.IsOverflowed() )
			{
				Host_Error( "WriteUsercmdDeltaToBuffer (%i %i) Overflowed %i byte buffer, last cmd was %i bits long\n",
					from, to, MAX_CMD_BUFFER, endbit - startbit );
			}

			from = to;
		}

		// Write a tag to see if there is a mis-parse on the server
		buf.WriteUBitLong( 0xffffffff, 32 );

		// Request delta compression of entities
		if ( ( cls.state == ca_active ) &&
			 ( cls.signon == SIGNONS ) &&
			 ( ( cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged ) >= CL_UPDATE_MASK ) &&
			 ( ( realtime - cls.netchan.last_received ) > CONNECTION_PROBLEM_TIME ) &&
			 !demo->IsPlayingBack() )
		{
			con_nprint_t np;
			np.time_to_live = 1.0;
			np.index = 0;
			np.fixed_width_font = false;
			np.color[ 0 ] = 1.0;
			np.color[ 1 ] = 0.2;
			np.color[ 2 ] = 0.0;
			Con_NXPrintf( &np, "WARNING:  Connection Problem" );
			cl.validsequence = 0;
		}

		// Determine if we need to ask for a new set of delta's.
		if ( cl.validsequence &&
			( cls.state == ca_active ) &&
			!( demo->IsRecording() && demo->IsWaitingForUpdate() ) )
		{
			cl.delta_sequence = cl.validsequence;

			buf.WriteByte( clc_delta );
			buf.WriteUBitLong( cl.validsequence & DELTAFRAME_MASK, DELTAFRAME_NUMBITS );
		}
		else
		{
			// Tell server to send us an uncompressed update
			cl.delta_sequence = -1;
		}

		if ( buf.IsOverflowed() )
		{
			Host_Error( "CL_Move, overflowed command buffer (%i bytes)\n", MAX_CMD_BUFFER );
		}

		// Remember outgoing command that we are sending
		cls.lastoutgoingcommand = cls.netchan.outgoing_sequence;

		// Update size counter for netgraph
		cl.commands[ cls.netchan.outgoing_sequence & CL_UPDATE_MASK ].sendsize = buf.GetNumBytesWritten();

		// Composite the rest of the datagram..
		if( cls.datagram.GetNumBitsWritten() <= buf.GetNumBitsLeft() )
		{
			buf.WriteBits( cls.datagram.GetBasePointer(), cls.datagram.GetNumBitsWritten() );
		}
		cls.datagram.Reset();

		//COM_Log( "cl.log", "Sending command number %i(%i) to server\n", cls.netchan.outgoing_sequence, cls.netchan.outgoing_sequence & CL_UPDATE_MASK );
		// Send the message
		Netchan_Transmit( &cls.netchan, buf.GetNumBytesWritten(), buf.GetBasePointer() );	
	}
	else
	{
		// Mark command as held back so we'll send it next time
		cl.commands[ cls.netchan.outgoing_sequence & CL_UPDATE_MASK ].heldback = true;
		// Increment sequence number so we can detect that we've held back packets.
		cls.netchan.outgoing_sequence++;
	}

	// Store new usercmd to dem file
	if ( demo->IsRecording() )
	{
		// Back up one because we've incremented outgoing_sequence each frame by 1 unit
		cmdnumber = ( cls.netchan.outgoing_sequence - 1 ) & CL_UPDATE_MASK;
		demo->WriteUserCmd( cmdnumber );
	}

	// Update download/upload slider.
	Netchan_UpdateProgress( &cls.netchan );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMessage - 
//-----------------------------------------------------------------------------
void CL_HudMessage( const char *pMessage )
{
	DispatchDirectUserMsg( "HudText", strlen(pMessage) + 1, (void *)pMessage );
}
void CL_ShowEnts_f()
{
	for ( int i = 0; i < entitylist->GetMaxEntities(); i++ )
	{
		char entStr[256], classStr[256];
		IClientNetworkable *pEnt;

		if(pEnt = entitylist->GetClientNetworkable(i))
		{
			entStr[0] = 0;
			Q_snprintf(classStr, sizeof( classStr ), "'%s'", pEnt->GetClientClass()->m_pNetworkName);
		}
		else
		{
			Q_snprintf(entStr, sizeof( entStr ), "(missing), ");
			Q_snprintf(classStr, sizeof( classStr ), "(missing)");
		}

		if ( pEnt )
			Con_Printf("Ent %3d: %s class %s\n", i, entStr, classStr);
	}
}

//
// register commands
//
static ConCommand cl_showents( "cl_showents", CL_ShowEnts_f );
static ConCommand cl_hud( "cl_hud", CL_HideHud_f );
static ConCommand retry("retry", CL_Retry_f); 
static ConCommand connect("connect", CL_Connect_f);
static ConCommand reconnect("reconnect", CL_Reconnect_f);
static ConCommand snapshot( "snapshot", CL_TakeSnapshot_f );
static ConCommand screenshot( "screenshot", CL_TakeSnapshot_f );
static ConCommand startmovie( "startmovie", CL_StartMovie_f );
static ConCommand endmovie( "endmovie", CL_EndMovie_f );
static ConCommand rcon("rcon", CL_Rcon_f);
static ConCommand cl_view("cl_view", CL_View_f);
static ConCommand pingservers("pingservers", CL_PingServers_f );
static ConCommand slist("slist", CL_Slist_f );     
static ConCommand list("list", CL_ListCachedServers_f );
static ConCommand box( "box", CL_DebugBox_f );

/*
=================
CL_Init
=================
*/
void CL_Init (void)
{	
	cls.datagram.StartWriting(cls.datagram_buf, sizeof(cls.datagram_buf));
	cls.datagram.SetDebugName( "cls.datagram" );
	cls.netchan.message.SetDebugName( "cls.netchan.message" );

	// set default userinfo values
	Info_SetValueForKey( cls.userinfo, "name", "unnamed", MAX_INFO_STRING );
	Info_SetValueForKey( cls.userinfo, "rate", "2500", MAX_INFO_STRING);
	Info_SetValueForKey( cls.userinfo, "cl_updaterate", "20", MAX_INFO_STRING );

	TRACEINIT( demo->Init(), demo->Shutdown() );

	 // Don't use memset here because there are vtables.
	cl.Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_Shutdown( void )
{
	TRACESHUTDOWN( demo->Shutdown() );
}

float CClientState::gettime() const
{
	// Out
	if ( !insimulation )
	{
		return tickcount * TICK_RATE + tickremainder;
	}
	return tickcount * TICK_RATE;
}

float CClientState::getframetime() const
{
	if ( !insimulation )
	{
		return cl.paused || bugreporter->ShouldPause() ? 0 : host_frametime;
	}
	if ( tickcount == oldtickcount )
	{
		return 0.0f;
	}
	return ( tickcount - oldtickcount) * TICK_RATE;
}

//-----------------------------------------------------------------------------
// Purpose: // Clear all the variables in the CClientState.
//-----------------------------------------------------------------------------
void CClientState::Clear( void )
{
	if ( networkStringTableContainerClient )
		networkStringTableContainerClient->RemoveAllTables();
	
	m_hModelPrecacheTable = INVALID_STRING_TABLE;
	m_hGenericPrecacheTable = INVALID_STRING_TABLE;
	m_hSoundPrecacheTable = INVALID_STRING_TABLE;
	m_hDecalPrecacheTable = INVALID_STRING_TABLE;
	m_hInstanceBaselineTable = INVALID_STRING_TABLE;

	servercount = 0;
	validsequence = 0;
	parsecount = 0;
	viewangles.Init();
	paused = 0;
	mtime[0] = mtime[1] = 0;
	tickcount = 0;
	oldtickcount = 0;
	tickremainder = 0;
	insimulation = false;


	addangle.RemoveAll();

	memset( frames, 0, MULTIPLAYER_BACKUP * sizeof( frame_t ) );
	playernum = 0;
	memset(model_precache, 0, sizeof(model_precache));
	memset(sound_precache, 0, sizeof(sound_precache));
	levelname[0] = 0;
	maxclients = 0;
	viewentity = 0;
	cdtrack = 0;
	serverCRC = 0;
	serverClientSideDllCRC = 0;
	memset(players, 0, sizeof(players));
	m_pServerClasses = 0;
	m_nServerClasses = 0;
	m_nServerClassBits = 0;

	last_incoming_sequence = 0;
	last_command_ack = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : model_t
//-----------------------------------------------------------------------------
model_t *CClientState::GetModel( int index )
{
	if ( index <= 0 )
	{
		return NULL;
	}

	if ( index >= networkStringTableContainerClient->GetNumStrings( m_hModelPrecacheTable ) )
	{
		return NULL;
	}

	CPrecacheItem *p = &model_precache[ index ];
	model_t *m = p->GetModel();
	if ( m )
	{
		return m;
	}

	char const *name = networkStringTableContainerClient->GetString( m_hModelPrecacheTable, index );

	if ( host_showcachemiss.GetBool() )
	{
		Con_DPrintf( "client model cache miss on %s\n", name );
	}

	m = modelloader->GetModelForName( name, IModelLoader::FMODELLOADER_CLIENT );
	if ( !m )
	{
		const CPrecacheUserData *data = CL_GetPrecacheUserData( m_hModelPrecacheTable, index );
		if ( data && ( data->flags & RES_FATALIFMISSING ) )
		{
			COM_ExplainDisconnection( true, "Cannot continue without model %s, disconnecting\n", name );
			Host_Disconnect();
		}
	}

	p->SetModel( m );
	return m;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int -- note -1 if missing
//-----------------------------------------------------------------------------
int CClientState::LookupModelIndex( char const *name )
{
	int idx = networkStringTableContainerClient->FindStringIndex( m_hModelPrecacheTable, name );
	return ( idx == INVALID_STRING_INDEX ) ? -1 : idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			*name - 
//-----------------------------------------------------------------------------
void CClientState::SetModel( int tableIndex )
{
	// Bogus index
	if ( tableIndex < 0 || 
		 tableIndex >= networkStringTableContainerClient->GetNumStrings( m_hModelPrecacheTable ) )
	{
		return;
	}

	CPrecacheItem *p = &model_precache[ tableIndex ];
	const CPrecacheUserData *data = CL_GetPrecacheUserData( m_hModelPrecacheTable, tableIndex );

	bool loadnow = ( data && ( data->flags & RES_PRELOAD ) );

	if ( CommandLine()->FindParm( "-nopreload" ) )
	{
		loadnow = false;
	}
	else if ( cl_forcepreload.GetInt() || CommandLine()->FindParm( "-preload" ) )
	{
		loadnow = true;
	}

	if ( loadnow )
	{
		char const *name = networkStringTableContainerClient->GetString( m_hModelPrecacheTable, tableIndex );
		p->SetModel( modelloader->GetModelForName( name, IModelLoader::FMODELLOADER_CLIENT ) );
	}
	else
	{
		p->SetModel( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CClientState::GetModelPrecacheTable( void ) const
{
	return m_hModelPrecacheTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//-----------------------------------------------------------------------------
void CClientState::SetModelPrecacheTable( TABLEID id )
{
	m_hModelPrecacheTable = id;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : model_t
//-----------------------------------------------------------------------------
char const *CClientState::GetGeneric( int index )
{
	if ( index <= 0 )
		return "";

	if ( index >= networkStringTableContainerClient->GetNumStrings( m_hGenericPrecacheTable ) )
	{
		return "";
	}

	CPrecacheItem *p = &generic_precache[ index ];
	char const *g = p->GetGeneric();
	return g;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int -- note -1 if missing
//-----------------------------------------------------------------------------
int CClientState::LookupGenericIndex( char const *name )
{
	int idx = networkStringTableContainerClient->FindStringIndex( m_hGenericPrecacheTable, name );
	return ( idx == INVALID_STRING_INDEX ) ? -1 : idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			*name - 
//-----------------------------------------------------------------------------
void CClientState::SetGeneric( int tableIndex )
{
	// Bogus index
	if ( tableIndex < 0 || 
		 tableIndex >= networkStringTableContainerClient->GetNumStrings( m_hGenericPrecacheTable ) )
	{
		return;
	}

	char const *name = networkStringTableContainerClient->GetString( m_hGenericPrecacheTable, tableIndex );
	CPrecacheItem *p = &generic_precache[ tableIndex ];
	p->SetGeneric( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CClientState::GetGenericPrecacheTable( void ) const
{
	return m_hGenericPrecacheTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//-----------------------------------------------------------------------------
void CClientState::SetGenericPrecacheTable( TABLEID id )
{
	m_hGenericPrecacheTable = id;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CClientState::GetSoundName( int index )
{
	if ( index <= 0 )
		return "";

	if ( index >= networkStringTableContainerClient->GetNumStrings( m_hSoundPrecacheTable ) )
	{
		return "";
	}

	char const *name = networkStringTableContainerClient->GetString( m_hSoundPrecacheTable, index );
	return name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : model_t
//-----------------------------------------------------------------------------
CSfxTable *CClientState::GetSound( int index )
{
	if ( index <= 0 )
		return NULL;

	if ( index >= networkStringTableContainerClient->GetNumStrings( m_hSoundPrecacheTable ) )
	{
		return NULL;
	}

	CPrecacheItem *p = &sound_precache[ index ];
	CSfxTable *s = p->GetSound();
	if ( s )
		return s;

	char const *name = networkStringTableContainerClient->GetString( m_hSoundPrecacheTable, index );

	if ( host_showcachemiss.GetBool() )
	{
		Con_DPrintf( "client sound cache miss on %s\n", name );
	}

	s = S_PrecacheSound( name );

	p->SetSound( s );
	return s;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int -- note -1 if missing
//-----------------------------------------------------------------------------
int CClientState::LookupSoundIndex( char const *name )
{
	int idx = networkStringTableContainerClient->FindStringIndex( m_hSoundPrecacheTable, name );
	return ( idx == INVALID_STRING_INDEX ) ? -1 : idx;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			*name - 
//-----------------------------------------------------------------------------
void CClientState::SetSound( int tableIndex )
{
	// Bogus index
	if ( tableIndex < 0 || 
		 tableIndex >= networkStringTableContainerClient->GetNumStrings( m_hSoundPrecacheTable ) )
	{
		return;
	}

	CPrecacheItem *p = &sound_precache[ tableIndex ];
	const CPrecacheUserData *data = CL_GetPrecacheUserData( m_hSoundPrecacheTable, tableIndex );

	bool loadnow = ( data && ( data->flags & RES_PRELOAD ) );

	if ( CommandLine()->FindParm( "-nopreload" ) )
	{
		loadnow = false;
	}
	else if ( cl_forcepreload.GetInt() || CommandLine()->FindParm( "-preload" ) )
	{
		loadnow = true;
	}

	if ( loadnow )
	{
		char const *name = networkStringTableContainerClient->GetString( m_hSoundPrecacheTable, tableIndex );
		p->SetSound( S_PrecacheSound( name ) );
	}
	else
	{
		p->SetSound( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CClientState::GetSoundPrecacheTable( void ) const
{
	return m_hSoundPrecacheTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//-----------------------------------------------------------------------------
void CClientState::SetSoundPrecacheTable( TABLEID id )
{
	m_hSoundPrecacheTable = id;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : model_t
//-----------------------------------------------------------------------------
char const *CClientState::GetDecalName( int index )
{
	if ( index <= 0 )
	{
		return NULL;
	}

	if ( index >= networkStringTableContainerClient->GetNumStrings( m_hDecalPrecacheTable ) )
	{
		return NULL;
	}

	CPrecacheItem *p = &decal_precache[ index ];
	char const *d = p->GetDecal();
	return d;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			*name - 
//-----------------------------------------------------------------------------
void CClientState::SetDecal( int tableIndex )
{
	// Bogus index
	if ( tableIndex < 0 || 
		 tableIndex >= networkStringTableContainerClient->GetNumStrings( m_hDecalPrecacheTable ) )
	{
		return;
	}

	char const *name = networkStringTableContainerClient->GetString( m_hDecalPrecacheTable, tableIndex );
	CPrecacheItem *p = &decal_precache[ tableIndex ];
	p->SetDecal( name );

	Draw_DecalSetName( tableIndex, (char *)name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CClientState::GetDecalPrecacheTable( void ) const
{
	return m_hDecalPrecacheTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//-----------------------------------------------------------------------------
void CClientState::SetDecalPrecacheTable( TABLEID id )
{
	m_hDecalPrecacheTable = id;
}

TABLEID CClientState::GetInstanceBaselineTable() const
{
	return m_hInstanceBaselineTable;
}

void CClientState::SetInstanceBaselineTable( TABLEID id )
{
	m_hInstanceBaselineTable = id;
}

void CClientState::UpdateInstanceBaseline( int iStringID )
{
	// We used to update C_ServerClassInfo::m_InstanceBaselineIndex in here but we
	// do it lazily now. See GetDynamicBaseline for an explanation.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientState::DumpPrecacheStats( TABLEID table )
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

	int count = networkStringTableContainerClient->GetNumStrings( table );
	int maxcount = networkStringTableContainerClient->GetMaxStrings( table );

	Con_Printf( "\n" );
	Con_Printf( "Precache table %s:  %i of %i slots used\n", networkStringTableContainerClient->GetTableName( table ),
		count, maxcount );

	int used = 0, total = 0;
	for ( int i = 0; i < count; i++ )
	{
		char const *name = networkStringTableContainerClient->GetString( table, i );
		CPrecacheItem *slot = &items[ i ];
		const CPrecacheUserData *p = CL_GetPrecacheUserData( table, i );

		if ( !name || !slot || !p )
			continue;

		Con_Printf( "%03i:  %s (%s):   ",
			i,
			name, 
			GetFlagString( p->flags ) );

		int filesize = p->filesize << 10;

		if ( p->filesize > 0 || name[ 0 ] == '*' )
		{
			Con_Printf( "%.2f kb,", (float)( filesize ) / 1024.0f );
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

void CL_PrecacheInfo_f( void )
{
	if ( Cmd_Argc() == 2 )
	{
		char const *table = Cmd_Argv( 1 );

		bool dumped = true;
		if ( !Q_strcasecmp( table, "generic" ) )
		{
			cl.DumpPrecacheStats( cl.GetGenericPrecacheTable() );
		}
		else if ( !Q_strcasecmp( table, "sound" ) )
		{
			cl.DumpPrecacheStats( cl.GetSoundPrecacheTable() );
		}
		else if ( !Q_strcasecmp( table, "decal" ) )
		{
			cl.DumpPrecacheStats( cl.GetDecalPrecacheTable() );
		}
		else if ( !Q_strcasecmp( table, "model" ) )
		{
			cl.DumpPrecacheStats( cl.GetModelPrecacheTable() );
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
	cl.DumpPrecacheStats( cl.GetGenericPrecacheTable() );
	cl.DumpPrecacheStats( cl.GetDecalPrecacheTable() );
	cl.DumpPrecacheStats( cl.GetSoundPrecacheTable() );
	cl.DumpPrecacheStats( cl.GetModelPrecacheTable() );
}

static ConCommand cl_precacheinfo( "cl_precacheinfo", CL_PrecacheInfo_f, "Show precache info (client)." );

// Singleton client state
CClientState	cl;
