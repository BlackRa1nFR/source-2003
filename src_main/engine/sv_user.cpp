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
// sv_user.c -- server code for moving users
#include "quakedef.h"
#include "world.h"
#include "eiface.h"
#include "server.h"
#include "checksum_engine.h"
#include "keys.h"
#include "enginestats.h"
#include "measure_section.h"
#include "cmodel_engine.h"
#include "gl_model_private.h"
#include "sv_main.h"
#include "networkstringtableserver.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "console.h"
#include "host.h"

edict_t	*sv_player = NULL;


ConVar sv_voiceenable( "sv_voiceenable", "1", FCVAR_ARCHIVE|FCVAR_SERVER ); // set to 0 to disable all voice forwarding.
extern ConVar	sv_cheats;

// Gets voice data from a client and forwards it to anyone who can hear this client.
void SV_ParseVoiceData(client_t *cl)
{
	int i, iClient;
	int	nDataLength;
	char chReceived[4096];
	client_t *pDestClient;
	
	iClient = cl - svs.clients;

	// Read in the data.	
	nDataLength = MSG_ReadShort();
	if( nDataLength > sizeof(chReceived) )
	{
		Host_Error("SV_ParseVoiceData: invalid incoming packet.\n");
		return;
	}

	MSG_ReadBuf( nDataLength, chReceived );


	// Disable voice?
	if( !sv_voiceenable.GetInt() )
		return;

	for(i=0; i < MAX_CLIENTS; i++)
	{
		qboolean bLocal;
		int nSendLength;

		pDestClient = &svs.clients[i];
		bLocal = (pDestClient == cl);

		// Does the game code want cl sending to this client?
		if(!(cl->m_VoiceStreams[i>>5] & (1 << (i & 31))) && !bLocal)
			continue;
		
		// Is this client even on the server?
		if(!pDestClient->active && !pDestClient->connected && !bLocal)
			continue;

		// Is loopback enabled?
		nSendLength = nDataLength;
		if(bLocal && !pDestClient->m_bLoopback)
		{
			nSendLength = 0;	// Still send something, just zero length (this is so the client 
								// can display something that shows knows the server knows it's talking).
		}

		// Is there room to write this data in?
		if( pDestClient->datagram.GetNumBytesLeft() >= (6 + nDataLength) )
		{
			pDestClient->datagram.WriteByte( svc_voicedata );
			pDestClient->datagram.WriteByte( (byte)iClient );
			pDestClient->datagram.WriteShort( nSendLength );
			pDestClient->datagram.WriteBytes( chReceived, nSendLength );
		}
	}
}

void SV_StoreLogoRequest( client_t *cl, CRC32_t& crc );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SV_ParseSendLogo( client_t *cl )
{
	CRC32_t crc;
	crc = (CRC32_t)MSG_ReadLong();

	// It's a blank logo, ignore...
	if ( crc == 0 )
	{
		return;
	}

	// Find file and send it
	char crcfilename[ 512 ];

	Q_snprintf( crcfilename, sizeof( crcfilename ), "materials/decals/downloads/%s.vtf",
		COM_BinPrintf( (byte *)&crc, 4 ) );

	if ( g_pFileSystem->FileExists( crcfilename ) )
	{
		Netchan_CreateFileFragments( true, &cl->netchan, crcfilename );
	}
	else
	{
		// Still waiting for logo to upload?
		SV_StoreLogoRequest( cl, crc );
	}
}

typedef struct
{
	char *command;
} command_t;

static command_t clcommands[] = 
{
	"status",
	"god",
	"notarget",
	"fly",
	"name",
	"noclip",
	"tell",
	"color",
	"pause",
	"setpause",
	"unpause",
	"spawn",
	"new",
	"dropclient",
	"begin",
	"prespawn",
	"kick",
	"ping",
	"ban",
	"setinfo",
	"showinfo",
	NULL,
};

/*
===================
SV_ValidateClientCommand

Determine if passed in user command is valid.
===================
*/
int	SV_ValidateClientCommand( char *pszCommand )
{
	char *p;
	int i = 0;

	COM_Parse( pszCommand );

	p = clcommands[i].command;
	while ( p != NULL )
	{
		if ( Q_strcasecmp( com_token, p ) == 0 )
		{
			return 1;
		}

		i++;
		p = clcommands[i].command;
	}

	return 0;
}

/*
==================
SV_ParseStringCommand

Client command string
==================
*/
void SV_ParseStringCommand(client_t *cl)
{
	char *s;
	int ret;

	s = MSG_ReadString();
	
	// Determine whether the command is appropriate
	ret = SV_ValidateClientCommand( s );
	switch ( ret )
	{
	case 2:
		Cbuf_InsertText (s);
		break;
	case 1:
		Cmd_ExecuteString (s, src_client);
		break;
	default:

		Cmd_TokenizeString(s);

		ConCommandBase const *pCommand = ConCommandBase::FindCommand( Cmd_Argv( 0 ) );
		if ( pCommand && pCommand->IsCommand() && pCommand->IsBitSet( FCVAR_EXTDLL ) )
		{
			// Allow cheat commands in singleplayer, debug, or multiplayer with sv_cheats on
#ifndef _DEBUG
			if ( pCommand->IsBitSet( FCVAR_CHEAT ) )
			{
				if ( svs.maxclients > 1 && sv_cheats.GetInt() == 0 )
					break;
			}
#endif

			serverGameClients->SetCommandClient( host_client - svs.clients );
			( ( ConCommand * )pCommand )->Dispatch();
		}
		else
		{
			serverGameClients->ClientCommand( sv_player );
		}
		break;
	}
}

/*
==============================
SV_ParseDelta

==============================
*/
void SV_ParseDelta(client_t *cl)
{
	cl->delta_sequence = MSG_GetReadBuf()->ReadUBitLong( DELTAFRAME_NUMBITS );

	SV_UpdateAcknowledgedFramecount( cl );
}

static qboolean g_lastmovementtick = -1;

/*
==============================
SV_ParseMove

==============================
*/
void SV_ParseMove(client_t *cl)
{
	int				numbackup = 2;
	int				numcmds;
	int				totalcmds;
	client_frame_t	*frame;
	bf_read *pBuf = MSG_GetReadBuf();

	if ( g_lastmovementtick == sv.tickcount )  // Only one movement command per frame, someone is cheating.
	{
		pBuf->SetOverflowFlag();
		return;
	}

	g_lastmovementtick = sv.tickcount;

	// Calculate ping time for client based on last frame that the client ack'd
	frame = &host_client->frames[ host_client->netchan.incoming_acknowledged & SV_UPDATE_MASK ];

	cl->m_bLoopback = !!pBuf->ReadOneBit();

	numbackup	= pBuf->ReadUBitLong( NUM_BACKUP_COMMAND_BITS );;
	numcmds		= pBuf->ReadByte();

	totalcmds	= numbackup + numcmds;

	// Decrement drop count by held back packet count
	net_drop -= ( numcmds - 1 );

	bool ignore = !sv.active || ( !host_client->active && !host_client->spawned );
#ifdef SWDS
	bool paused = ( sv.paused || ( (svs.maxclients <= 1) ) );
#else
	bool paused = ( sv.paused || ( (svs.maxclients <= 1) && (Con_IsVisible()) ) );
#endif

	// Make sure player knows of correct server time
	g_ServerGlobalVariables.curtime = sv.gettime();
	g_ServerGlobalVariables.frametime = TICK_RATE;

//	COM_Log( "sv.log", "  executing %i move commands from client starting with command %i(%i)\n",
//		numcmds, 
//		host_client->netchan.incoming_sequence,
//		host_client->netchan.incoming_sequence & SV_UPDATE_MASK );

	float lastcmdtime = serverGameClients->ProcessUsercmds
	( 
		sv_player,							// Player edict
		pBuf,
		numcmds,
		totalcmds,							// Commands in packet
		net_drop,							// Number of dropped commands
		ignore,								// Don't actually run anything
		paused								// Run, but don't actually do any movement
	);

	// Adjust latency time by 1/2 last client frame since the message probably arrived 1/2 through client's frame loop
	frame->latency -= 0.5 * lastcmdtime;
	frame->latency = max( 0.0f, frame->latency );

	if ( pBuf->IsOverflowed() )
	{
		SV_DropClient( host_client, false, "ProcessUsercmds:  Overflowed reading usercmd data (check sending and receiving code for mismatches)!\n" );
		return;
	}

	unsigned int tag = pBuf->ReadUBitLong( 32 );
	if ( tag != 0xffffffff )
	{
		SV_DropClient( host_client, false, "ProcessUsercmds:  Incorrect reading frame (check sending and receiving code for mismatches)!\n" );
		pBuf->SetOverflowFlag();
		return;
	}
}

typedef struct clc_func_s
{
	// Opcode
	unsigned char opcode;  
	// Display Name
	char *pszname;         
	// Parse function
	void ( *pfnParse )(client_t *cl);
} clc_func_t;

static clc_func_t sv_clcfuncs[] =
{
	{ clc_bad, "clc_bad", NULL },   // 0
	{ clc_nop, "clc_nop", NULL },   // 1
	{ clc_sendlogo, "clc_disconnect", SV_ParseSendLogo }, // 2
	{ clc_move, "clc_move", SV_ParseMove }, // 3
	{ clc_stringcmd, "clc_stringcmd", SV_ParseStringCommand }, // 4
	{ clc_delta, "clc_delta", SV_ParseDelta }, // 5
	{ clc_voicedata, "clc_voicedata", SV_ParseVoiceData }, // 6
	{ (unsigned char)-1, "End of List", NULL }
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
//-----------------------------------------------------------------------------
void SV_LogBadPacket( client_t *cl )
{
	FileHandle_t fp;
	int i = 0;
	char filename[ MAX_OSPATH ];
	bool done = false;

	while ( i < 1000 && !done )
	{
		Q_snprintf( filename, sizeof( filename ), "error%03i.dat", i );
		fp = g_pFileSystem->Open( filename, "rb" );
		if ( !fp )
		{
			fp = g_pFileSystem->Open( filename, "wb" );
			g_pFileSystem->Write( net_message.data, net_message.cursize, fp );
			done = true;
		}
		if ( fp )
		{
			g_pFileSystem->Close( fp );
		}
		i++;
	}

	if ( i < 1000 )
	{
		Con_Printf( "Error buffer for %s written to %s\n", cl->name, filename );
	}
	else
	{
		Con_Printf( "Couldn't write error buffer, delete error###.dat files to make space\n" );
	}
}

/*
==================
SV_ExecuteClientMessage

Process client command packet
==================
*/
void SV_ExecuteClientMessage (client_t *cl)
{
	int				c;
	client_frame_t	*frame;

	g_lastmovementtick = ( sv.tickcount - 1 );

	// Calculate ping time.
	// Frame client is one.
	frame = &cl->frames[cl->netchan.incoming_acknowledged & SV_UPDATE_MASK];

	// Remove message interval gap, too.
	frame->latency = realtime - frame->senttime - cl->next_messageinterval;
	// Raw ping doesn't factor in message interval, either
	frame->raw_ping = realtime - frame->senttime;

	// On first frame ( no senttime ) don't skew ping
	if ( frame->senttime == 0.0f )
	{
		frame->latency = 0.0f;
		frame->raw_ping = 0.0f;
	}

	// Don't skew ping based on signon stuff either
	if ( ( realtime - cl->connection_started ) < 2.0 && ( frame->latency > 0.0 ) )
	{
		frame->latency = 0.0f;
		frame->raw_ping = 0.0f;
	}

	host_client = cl;
	sv_player   = host_client->edict;
	
	cl->delta_sequence = -1;	// no delta unless requested

	// This will cause the string table to send the entire
	// string table in the case of recording a demo
	cl->acknowledged_tickcount = 0;

	while ( 1 )
	{
		if ( MSG_IsOverflowed() )
		{
			Con_Printf ("SV_ReadClientMessage: badread\n");

			SV_LogBadPacket( cl );

			SV_DropClient (cl, false, "Bad parse in client command" );
			return;
		}	

		// Are we at the end?
		if ( MSG_GetReadBuf()->GetNumBitsLeft() < 8 )
		{
			break;
		}
		c = MSG_ReadByte ();
		
		// Msg out of range
		if ( c < 0 || c > clc_voicedata )
		{
			Con_Printf ( "SV_ReadClientMessage: unknown command %i\n", c );

			SV_LogBadPacket( cl );

			SV_DropClient (cl, false, "Bad command character in client command" );
			break;
		}

		// See if there is a handler
		if ( sv_clcfuncs[ c ].pfnParse )
		{
			void ( *func )(client_t*);
			func = sv_clcfuncs[ c ].pfnParse;
			
			// Dispatch parsing function
			(*func)(cl);
		}

		if ( !cl->active && !cl->connected && !cl->spawned )
		{
			// Execution of the client's message forced the client
			//  to be dropped
			break;
		}
	}
}

/*
=================
SV_Drop_f

The client is going to disconnect, so remove the connection immediately
=================
*/
void SV_Drop_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	SV_RedirectEnd ();
	SV_BroadcastPrintf ("%s dropped\n", host_client->name);
	SV_DropClient (host_client, false, "%s issued 'drop' command", host_client->name );	
}

/*
==================
SV_SetPlayer

Sets host_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer( int idnum )
{
	client_t	*cl;
	int			i;

	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if ( !cl->spawned || !cl->active || !cl->connected )
			continue;
		if (cl->userid == idnum)
		{
			host_client = cl;
			sv_player = host_client->edict;
			return true;
		}
	}
	Con_Printf ("Userid %i is not on the server\n", idnum);
	return false;
}

void SV_Info_f( void )
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: info <userid>\n");
		return;
	}

	if ( !SV_SetPlayer(atoi(Cmd_Argv(1))) ) // sets the host_client
		return;

	Info_Print( host_client->userinfo );
}

static ConCommand dropclient("dropclient", SV_Drop_f);
static ConCommand info( "info", SV_Info_f );
