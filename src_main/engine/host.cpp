//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Revision: $
// $NoKeywords: $
//
//=============================================================================
#include "tier0/fasttimer.h"
#include "winquake.h"

#ifdef _WIN32
#include <crtdbg.h>   // For getting at current heap size
#endif

#include "server.h"
#include "host_jmp.h"
#include "screen.h"
#include "kbutton.h"
#include "keys.h"
#include "cdll_int.h"
#include "eiface.h"
#include "sv_main.h"
#include "master.h"
#include "sv_log.h"
#include "zone.h"
#include "game_interface.h"
#include "sv_filter.h"
#include "vid.h"
#include "glquake.h"

#if defined _WIN32 && !defined SWDS
#include "voice.h"
#include "sound.h"
#include "vgui_int.h"
#endif

#include "measure_section.h"
#include "sys.h"
#include "cl_pred.h"
#include "console.h"
#include "view.h"
#include "host.h"
#include "decal.h"
#include "gl_matsysiface.h"
#include "sys_dll.h"
#include "cmodel_engine.h"
#ifndef SWDS
#include "con_nprint.h"
#endif
#include "filesystem.h"
#include "filesystem_engine.h"
#include "tier0/vcrmode.h"
#include "traceinit.h"
#include "host_saverestore.h"
#include "l_studio.h"
#include "demo.h"
#include "cdll_engine_int.h"
#include "host_cmd.h"
#include "host_state.h"
#include "dt_instrumentation.h"
#include "dt_instrumentation_server.h"
#include "const.h"
#include "bitbuf_errorhandler.h"
#include "soundflags.h"
#include "enginestats.h"
#include "vstdlib/strtools.h"
#include "testscriptmgr.h"
#include "tmessage.h"
#include "cl_ents.h"
#include "tier0/vprof.h"
#include "vstdlib/ICommandLine.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "glquake.h"
#include "staticpropmgr.h"
#include "gameeventmanager.h"


int host_frameticks = 0;
int host_tickcount = 0;
int host_currentframetick = 0;
float host_remainder = 0.0f;

//------------------------------------------
enum
{
	FRAME_SEGMENT_INPUT = 0,
	FRAME_SEGMENT_CLIENT,
	FRAME_SEGMENT_SERVER,
	FRAME_SEGMENT_RENDER,
	FRAME_SEGMENT_SOUND,
	FRAME_SEGMENT_CLDLL,
	FRAME_SEGMENT_CMD_EXECUTE,

	NUM_FRAME_SEGMENTS,
};

class CFrameTimer
{
public:
	void ResetDeltas();

	CFrameTimer() : swaptime(0)
	{
		ResetDeltas();
	}

	void MarkFrame();
	void StartFrameSegment( int i )
	{
		starttime[i] = Sys_FloatTime();
	}

	void EndFrameSegment( int i )
	{
		double dt = Sys_FloatTime() - starttime[i];
		deltas[ i ] += dt;
	}
	void MarkSwapTime( )
	{
		double newswaptime = Sys_FloatTime();
		frametime = newswaptime - swaptime;
		swaptime = newswaptime;

		ComputeFrameVariability();
		g_EngineStats.SetFrameTime( frametime );
		g_EngineStats.SetFPSVariability( m_flFPSVariability );
	}

private:
	enum
	{
		FRAME_HISTORY_COUNT = 50		
	};

	friend void Host_Speeds();
	void ComputeFrameVariability();

	double time_base;
	double times[9];
	double swaptime;
	double frametime;
	double m_flFPSVariability;
	double starttime[NUM_FRAME_SEGMENTS];
	double deltas[NUM_FRAME_SEGMENTS];

	float m_pFrameTimeHistory[FRAME_HISTORY_COUNT];
	int m_nFrameTimeHistoryIndex;
};


static CFrameTimer g_HostTimes;


//------------------------------------------


double host_time = 0.0;

extern ConVar	pausable;
extern ConVar	ent_developer;
extern ConVar	violence_hblood;
extern ConVar	violence_ablood;
extern ConVar	violence_hgibs;
extern ConVar	violence_agibs;
extern ConVar	sv_cheats;

// In other C files.
void Shader_Shutdown( void );
void R_Shutdown( void );

extern ConVar      scr_connectmsg;
extern char cl_moviename[];

bool g_bAbortServerSet = false;

static ConVar	fps_max( "fps_max", "300", 0, "Frame rate limiter" );
static ConVar	singlestep( "singlestep", "0", 0, "Run engine in single step mode ( set next to 1 to advance a frame )" );
static ConVar	next( "next", "0", 0, "Set to 1 to advance to next frame ( when singlestep == 1 )" );
// Print a debug message when the client or server cache is missed
ConVar host_showcachemiss( "host_showcachemiss", "0", 0, "Print a debug message when the client or server cache is missed." );
static ConVar mem_dumpstats( "mem_dumpstats", "0", 0, "Dump current and max heap usage info to console at end of frame ( set to 2 for continuous output )\n" );

extern qboolean gfBackground;

extern	int giActive;
extern qboolean	g_bInEditMode;

static bool host_checkheap = false;

CCommonHostState host_state;
/*
A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.
Memory is cleared / released when a server or client begins, not when they end.
*/


// Ear position + orientation
Vector h_origin( 0, 0, 0 );
Vector h_forward( 1, 0, 0 ), h_right( 0, 1, 0 ), h_up( 0, 0, 1 );

engineparms_t host_parms;

qboolean	host_initialized = false;		// true if into command execution

double		host_frametime;
double		realtime;				// without any filtering or bounding
static double	oldrealtime;			// last frame run

int			host_framecount;
static int	host_hunklevel;

client_t	*host_client;			// current client

jmp_buf 	host_abortserver;
jmp_buf     host_enddemo;

static ConVar	host_profile( "host_profile","0" );

ConVar	host_limitlocal( "host_limitlocal", "0", 0, "Apply cl_cmdrate and cl_updaterate to loopback connection" );
ConVar	host_framerate( "host_framerate","0" );	// set to lock per-frame time elapse
ConVar	host_timescale( "host_timescale","1.0" );	// set for slow motion
ConVar	host_speeds( "host_speeds","0" );		// set for running times
ConVar  mp_logfile( "mp_logfile", "1", FCVAR_SERVER );  // Log multiplayer frags to server logfile
ConVar  mp_logecho( "mp_logecho", "1", FCVAR_SERVER );  // Log multiplayer frags to console
#ifdef _DEBUG
ConVar	developer( "developer","1" );   // Dev by default is off except in debug builds
#else
ConVar  developer( "developer", "0" );
#endif
ConVar	ent_developer( "ent_developer", "0" );
ConVar	skill( "skill","1", FCVAR_ARCHIVE );			// 1 - 3
ConVar	deathmatch( "deathmatch","0", FCVAR_SERVER );	// 0, 1, or 2
ConVar	coop( "coop","0", FCVAR_SERVER );			// 0 or 1
ConVar	pausable( "pausable","1", FCVAR_SERVER );
ConVar	r_ForceRestore( "r_ForceRestore", "0" );


// For measure_section.cpp.
double GetRealTime()
{
	return realtime;
}

/*
================
Host_EndGame
================
*/
void Host_EndGame (char *message, ...)
{
	int oldn;
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,message);
	Q_vsnprintf (string,sizeof(string),message,argptr);
	va_end (argptr);
	Con_Printf ("Host_EndGame: %s\n",string);

#ifndef SWDS
	scr_disabled_for_loading = true;
#endif

	oldn = cls.demonum;
	
	Host_Disconnect();

	cls.demonum = oldn;

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_EndGame: %s\n",string);	// dedicated servers exit
	
	if (cls.demonum != -1)
	{
		oldn = cls.demonum;
		
		cls.demonum = oldn;
#ifndef SWDS
		CL_NextDemo ();
#endif		
		longjmp (host_enddemo, 1);
	}
	else
	{

#ifndef SWDS
		scr_disabled_for_loading = false;

		Cbuf_AddText("cd stop\n");
		Cbuf_Execute();
#endif
		if ( g_bAbortServerSet )
		{
			longjmp (host_abortserver, 1);
		}
	}

}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;

	if (inerror)
	{
		Sys_Error ("Host_Error: recursively entered");
	}
	inerror = true;

#ifndef SWDS
	CL_WriteMessageHistory();
#endif

	va_start (argptr,error);
	Q_vsnprintf(string,sizeof(string),error,argptr);
	va_end (argptr);

	if ( cls.state == ca_dedicated )
	{
		// dedicated servers just exit
		Sys_Error( "Host_Error: %s\n", string );	
	}

#ifndef SWDS
	// Reenable screen updates
	SCR_EndLoadingPlaque ();		
#endif
	Con_Printf( "\nHost_Error: %s\n\n", string );
	
	Host_Disconnect();
	
	cls.demonum = -1;

	inerror = false;

	if ( g_bAbortServerSet )
	{
		longjmp (host_abortserver, 1);
	}
}

#ifndef SWDS
//******************************************
// UseDefuaultBinding
//
// If the config.cfg file is not present, this
// function is called to set the default key
// bindings to match those defined in kb_def.lst
//******************************************
void UseDefaultBindings( void )
{
	FileHandle_t f;
	char szFileName[ _MAX_PATH ];
	char token[ 1024 ];
	char szKeyName[ 256 ];

	// read kb_def file to get default key binds
	sprintf( szFileName, "%skb_def.lst", SCRIPT_DIR );
	f = g_pFileSystem->Open( szFileName, "r");
	if ( !f )
	{
		Con_Printf( "Couldn't open kb_def.lst\n" );
		return;
	}

	// read file into memory
	int size = g_pFileSystem->Size(f);
	char *buf = new char[ size ];
	g_pFileSystem->Read( buf, size, f );
	g_pFileSystem->Close( f );

	while ( 1 )
	{
		buf = COM_ParseFile( buf, token );
		if ( strlen( token ) <= 0 )
			break;
		strcpy ( szKeyName, token );

		buf = COM_ParseFile( buf, token );
		if ( strlen( token ) <= 0 )  // Error
			break;
			
		// finally, bind key
		Key_SetBinding ( Key_StringToKeynum( szKeyName ), token );
	}
	delete buf;		// cleanup on the way out
}


static bool g_bConfigCfgExecuted = false;

void Host_ReadConfiguration( void )
{
	if ( cls.state == ca_dedicated )
		return;

	// Rebind keys and set cvars
	Cbuf_AddText( "exec config.cfg mod\n" );
	Cbuf_Execute();

	// check to see if we actually set any keys, if not, load defaults from kb_def.lst
	// so we at least have basics setup.
	int nNumBinds = Key_CountBindings();
	if ( nNumBinds == 0 )
	{
		UseDefaultBindings();
	}

	Key_SetBinding ( K_ESCAPE, "cancelselect" );
	// Make sure console key is always bound
	Key_SetBinding( '`', "toggleconsole" );
	g_bConfigCfgExecuted = true;
}

/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration( char const *filename )
{
	Assert( filename && filename [ 0 ] );

	FileHandle_t f;
	kbutton_t *ml, *jl;
	
	if ( !g_bConfigCfgExecuted )
	{
		return;
	}

	if ( !host_initialized )
	{
		return;
	}

	// If in map editing mode don't save configuration
	if (g_bInEditMode)
	{
		Con_Printf( "skipping %s output when in map edit mode\n", filename );
		return;
	}

	// dedicated servers initialize the host but don't parse and set the
	// config.cfg cvars
	if (  cls.state != ca_dedicated )
	{
		if ( Key_CountBindings() <= 1 )
		{
			Con_Printf( "skipping %s output, no keys bound\n", filename );
			return;
		}

		// Generate a new .cfg file.
		f = g_pFileSystem->Open( filename, "w");
		if (!f)
		{
			Con_Printf( "Couldn't write %s\n", filename );
			return;
		}
		
		// Always throw away all keys that are left over.
		g_pFileSystem->FPrintf( f, "unbindall\n" );

		Key_WriteBindings (f);
		cv->WriteVariables (f);
		Info_WriteVars( cls.userinfo, f );

		ml = g_ClientDLL->IN_FindKey( "in_mlook" );
		jl = g_ClientDLL->IN_FindKey( "in_jlook" );

		if ( ml && ( ml->state & 1 ) ) 
			g_pFileSystem->FPrintf( f, "+mlook\n" );

		if ( jl && ( jl->state & 1 ) )
			g_pFileSystem->FPrintf( f, "+jlook\n" );

		g_pFileSystem->Close (f);

		Con_Printf( "Wrote %s\n", filename );
	}
}


void Host_WriteConfig_f( void )
{
	if ( Cmd_Argc() > 2 )
	{
		Con_Printf( "Usage:  writeconfig <filename.cfg>\n" );
		return;
	}

	if ( Cmd_Argc() == 2 )
	{
		char const *filename = Cmd_Argv( 1 );
		if ( !filename || !filename[ 0 ] )
		{
			return;
		}

		char outfile[ MAX_QPATH ];
		// Strip path and extension from filename
		COM_FileBase( filename, outfile );
		Host_WriteConfiguration( va( "cfg/%s.cfg", outfile ) );
	}
	else
	{
		Host_WriteConfiguration( "cfg/config.cfg" );
	}
}


static ConCommand host_writeconfig( "host_writeconfig", Host_WriteConfig_f, "Store current settings to config.cfg (or specified .cfg file)." );
#endif

void Host_RecomputeSpeed_f( void )
{
	Con_Printf( "Recomputing clock speed...\n" );

	CClockSpeedInit::Init();
	Con_Printf( "Clock speed: %.0f Mhz\n", CFastTimer::GetClockSpeed() / 1000000.0 );
}

static ConCommand recompute_speed( "recompute_speed", Host_RecomputeSpeed_f, "Recomputes clock speed (for debugging purposes)." );


void DTI_Flush_f()
{
	DTI_Flush();
	ServerDTI_Flush();
}

static ConCommand dti_flush( "dti_flush", DTI_Flush_f, "Write out the datatable instrumentation files (you must run with -dti for this to work)." );

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CRC32_t
//-----------------------------------------------------------------------------
CRC32_t Host_GetServerSendtableCRC()
{
	if ( !svs.dll_initialized )
	{
		Assert( 0 );
		return (CRC32_t)0;
	}

	return svs.sendtable_crc;
}


/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer( void )
{
	if ( !sv.active )
		return;

	//  Tell master we are shutting down
	master->ShutdownConnection();

	sv.active = false;

	int i;
	// Only drop clients if we have not cleared out entity data prior to this.
	for ( i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if ( ( host_client->active || host_client->connected ) && !host_client->fakeclient )
		{
			SV_DropClient(host_client, false, "server shutting down" );
		}
	}

	// Let drop messages go out
#ifdef _WIN32
	Sleep( 100 );
#elif _LINUX
	usleep( 100 );
#endif

	//
	// clear structures
	//

	SV_ClearMemory();

	SV_MapOverClients(SV_ClearClientStructure);

	g_pGameEventManager->FireEvent( new KeyValues( "server_shutdown", "reason", "restart" ), NULL );

	// Log_Printf( "Server shutdown.\n" );

	g_Log.Close();

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
// Output : qboolean
//-----------------------------------------------------------------------------
qboolean Host_FilterTime( float time )
{
	// Accumulate some time
	realtime += time;

	// Dedicated's tic_rate regulates server frame rate.  Don't apply fps filter here.
	float fps = fps_max.GetFloat();
	if ( fps != 0 )
	{
		// Limit fps to withing tolerable range
		fps = max( MIN_FPS, fps );
		fps = min( MAX_FPS, fps );

		float minframetime = 1.0 / fps;

		if (
#ifndef SWDS
		    !demo->IsPlayingBack_TimeDemo() && 
#endif
			( realtime - oldrealtime ) < minframetime )
		{
			// framerate is too high
			return false;		
		}
	}

	if ( !demo->IsPlayingBack_TimeDemo() )
	{
		host_frametime	= realtime - oldrealtime;
	}
	else
	{
		// Used to help increase reproducability of timedemos
		host_frametime	= 0.015f;
	}

	oldrealtime		= realtime;

	if ( host_framerate.GetFloat() > 0 
#if !defined( _DEBUG )
		&& ( IsSinglePlayerGame() || Demo_IsPlayingBack() ) 
#endif
		)
	{
		float fps = host_framerate.GetFloat();
		if ( fps > 1 )
		{
			fps = 1.0f/fps;
		}
		host_frametime = fps;
	}
	else if (host_timescale.GetFloat() > 0 
#if !defined( _DEBUG )
		&& ( IsSinglePlayerGame() || Demo_IsPlayingBack() ) 
#endif
		)
	{
		float demoscale = 1.0f;
#ifndef SWDS
		if ( Demo_IsPlayingBack() && demo->GetPlaybackTimeScale() != 1.0f )
		{
			demoscale = demo->GetPlaybackTimeScale();
		}
#endif
		float fullscale = host_timescale.GetFloat() * demoscale;

		host_frametime *= fullscale;
		host_frametime = min( host_frametime, MAX_FRAMETIME * fullscale);
	}
	else
	{	// don't allow really long or short frames
		host_frametime = min( host_frametime, MAX_FRAMETIME );
		host_frametime = max( host_frametime, MIN_FRAMETIME );
	}
	
	return true;
}

#define FPS_AVG_FRAC 0.9f

static float g_fFramesPerSecond = 0.0f;

/*
==================
Host_PostFrameRate
==================
*/
void Host_PostFrameRate( float frameTime )
{
	frameTime = clamp( frameTime, 0.0001f, 1.0f );

	float fps = 1.0f / frameTime;
	g_fFramesPerSecond = g_fFramesPerSecond * FPS_AVG_FRAC + ( 1.0f - FPS_AVG_FRAC ) * fps;
}

/*
==================
Host_GetHostInfo
==================
*/
void Host_GetHostInfo(float *fps, int *nActive, int *nMaxPlayers, char *pszMap)
{
	int clients = 0;

	*fps = g_fFramesPerSecond;

	// Count clients, report 
	SV_CountPlayers( &clients );

	*nActive = clients;
	if (pszMap)
	{
		if (sv.name && sv.name[0])
			strcpy(pszMap, sv.name);
		else
			pszMap[0] = '\0';
	}

	*nMaxPlayers = svs.maxclients;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SV_ParseRcom( void )
{
	char	*s;
	char	*c;

	MSG_BeginReading ();
	MSG_ReadLong ();		// skip the -1 marker

	s = MSG_ReadStringLine ();
	Cmd_TokenizeString (s);
	c = Cmd_Argv(0);
	if ( !Q_strcmp(c, "rcon" ) )
	{
		SV_Rcom (&net_from);
	}
}

//-----------------------------------------------------------------------------
// Purpose:  An inactive dedicated server can still receive rcons
//-----------------------------------------------------------------------------
void SV_CheckRcom( void )
{
	VPROF( "SV_CheckRcom" );

	if ( sv.active || 
		( cls.state != ca_dedicated ) || 
		giActive == DLL_CLOSE || 
		!host_initialized )
		return;

	while ( NET_GetPacket( NS_SERVER ) )
	{
		if ( Filter_ShouldDiscard( &net_from ) )
		{
			Filter_SendBan( &net_from );	// tell them we aren't listening...
			continue;
		}

		// check for connectionless packet (0xffffffff) first
		if (*(int *)net_message.data == -1)
		{
			SV_ParseRcom();
			continue;
		}
	}
}


#if defined _WIN32 && !defined SWDS
// FIXME: move me somewhere more appropriate
void CL_SendVoicePacket(bool bFinal)
{
	if ( cls.state != ca_active || !Voice_IsRecording() )
		return;

	// Get whatever compressed data there is and and send it.
	char uchVoiceData[2048];
	int nDataLength = Voice_GetCompressedData(uchVoiceData, sizeof(uchVoiceData), bFinal);
	if(!nDataLength)
		return;

	bf_write &msg = cls.datagram;
	if(msg.GetNumBytesLeft() <= (16 + nDataLength))
		return;

	msg.WriteByte(clc_voicedata);
	msg.WriteShort(nDataLength);

	for(int i = 0; i < nDataLength; i++)
	{
		msg.WriteByte( (unsigned char)uchVoiceData[i] );
	}
}


void CL_ProcessVoiceData()
{
	Voice_Idle(host_frametime);
	CL_SendVoicePacket(false);
}
#endif

/*
=====================
Host_UpdateScreen

Refresh the screen
=====================
*/
void Host_UpdateScreen( void )
{
	if ( cls.state == ca_dedicated )
		return;

#if defined _WIN32 && !defined SWDS
	if( r_ForceRestore.GetInt() )
	{
		ForceMatSysRestore();
		r_ForceRestore.SetValue(0);
	}

	// Refresh the screen
	SCR_UpdateScreen ();

	// If recording movie and the console is totally up, then write out this frame to movie file.
	if ( cl_moviename[0] && !VGui_IsConsoleVisible() )
	{
		VID_WriteMovieFrame();
	}
#endif
}

/*
====================
Host_UpdateSounds

Update sound subsystem and cd audio
====================
*/
void Host_UpdateSounds( void )
{
	if ( cls.state == ca_dedicated )
		return;

#if defined _WIN32 && !defined SWDS
	// update audio
	if (cls.state == ca_active)
	{
		S_Update (h_origin, h_forward, h_right, h_up);	
	}
	else
	{
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);
	}
#endif
}

/*
==============================
Host_Speeds

==============================
*/
void CFrameTimer::ResetDeltas()
{
	for ( int i = 0; i < NUM_FRAME_SEGMENTS; i++ )
	{
		deltas[ i ] = 0.0f;
	}
}

void CFrameTimer::MarkFrame()
{
	double frameTime;
	double fps;

	// Con_DPrintf("%f %f %f\n", time1, time2, time3 );

	float fs_input = (deltas[FRAME_SEGMENT_INPUT])*1000.0;
	float fs_client = (deltas[FRAME_SEGMENT_CLIENT])*1000.0;
	float fs_server = (deltas[FRAME_SEGMENT_SERVER])*1000.0;
	float fs_render = (deltas[FRAME_SEGMENT_RENDER])*1000.0;
	float fs_sound = (deltas[FRAME_SEGMENT_SOUND])*1000.0;
	float fs_cldll = (deltas[FRAME_SEGMENT_CLDLL])*1000.0;
	float fs_exec = (deltas[FRAME_SEGMENT_CMD_EXECUTE])*1000.0;

	ResetDeltas();

	frameTime = host_frametime;
	//frameTime /= 1000.0;
	if ( frameTime < 0.0001 )
	{
		fps = 999.0;
	}
	else
	{
		fps = 1.0 / frameTime;
	}

	if (host_speeds.GetInt())
	{
		int ent_count = 0;
		int i;
		static int last_host_tickcount;

		int ticks = host_tickcount - last_host_tickcount;
		last_host_tickcount = host_tickcount;

		// count used entities
		for (i=0 ; i<sv.num_edicts ; i++)
		{
			if (!sv.edicts[i].free)
				ent_count++;
		}

		char sz[ 256 ];
		Q_snprintf( sz, sizeof( sz ),
			"%3i fps -- inp(%3.1f) sv(%3.1f) cl(%3.1f) render(%3.1f) snd(%3.1f) cl_dll(%3.1f) exec(%3.1f) ents(%d) ticks(%d)",
				(int)fps, 
				fs_input, 
				fs_server, 
				fs_client, 
				fs_render, 
				fs_sound, 
				fs_cldll, 
				fs_exec, 
				ent_count, 
				ticks );

#ifndef SWDS
		if ( host_speeds.GetInt() >= 2 )
		{
			Con_NPrintf ( 0, sz );
		}
		else
#endif
		{
			Con_DPrintf ( "%s\n", sz );
		}
	}

}

#define FRAME_TIME_FILTER_TIME 0.5f

void CFrameTimer::ComputeFrameVariability()
{
	m_pFrameTimeHistory[m_nFrameTimeHistoryIndex] = frametime;
	if ( ++m_nFrameTimeHistoryIndex >= FRAME_HISTORY_COUNT )
	{
		m_nFrameTimeHistoryIndex = 0;
	}

	// Compute a low-pass filter of the frame time over the last half-second
	// Count the number of samples that live within the last half-second
	int i = m_nFrameTimeHistoryIndex;
	int nMaxSamples = 0;
	float flTotalTime = 0.0f;
	while( (nMaxSamples < FRAME_HISTORY_COUNT) && (flTotalTime <= FRAME_TIME_FILTER_TIME) )
	{
		if ( --i < 0 )
		{
			i = FRAME_HISTORY_COUNT - 1;
		}		
		if ( m_pFrameTimeHistory[i] == 0.0f )
			break;

		flTotalTime += m_pFrameTimeHistory[i];
		++nMaxSamples;
	}

	if ( nMaxSamples == 0 )
	{
		m_flFPSVariability = 0.0f;
		return;
	}

	float flExponent = -2.0f / (int)nMaxSamples;

	i = m_nFrameTimeHistoryIndex;
	float flAverageTime = 0.0f;
	float flExpCurveArea = 0.0f;
	int n = 0;
	while( n < nMaxSamples )
	{
		if ( --i < 0 )
		{
			i = FRAME_HISTORY_COUNT - 1;
		}
		flExpCurveArea += exp( flExponent * n );
		flAverageTime += m_pFrameTimeHistory[i] * exp( flExponent * n );
		++n;
	}

	flAverageTime /= flExpCurveArea;

	float flAveFPS = 0.0f;
	if ( flAverageTime != 0.0f )
	{
		flAveFPS = 1.0f / flAverageTime;
	}

	float flCurrentFPS = 0.0f;
 	if ( frametime != 0.0f )
	{
		flCurrentFPS = 1.0f / frametime;
	}

	// Now subtract out the current fps to get variability in FPS
	m_flFPSVariability = fabs( flCurrentFPS - flAveFPS );
}


void Host_Speeds()
{
	g_HostTimes.MarkFrame();
	demo->MarkFrame( g_HostTimes.times[3], g_HostTimes.m_flFPSVariability );
}

//-----------------------------------------------------------------------------
// Purpose: When singlestep == 1, then you must set next == 1 to run to the 
//  next frame.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Host_ShouldRun( void )
{
	static int current_tick = -1;

	// See if we are single stepping
	if ( !singlestep.GetInt() )
	{
		return true;
	}

	// Did user set "next" to 1?
	if ( next.GetInt() )
	{
		// Did we finally finish this frame ( Host_ShouldRun is called in 3 spots to pause
		//  three different things ).
		if ( current_tick != (host_tickcount-1) )
		{
			// Okay, time to reset to halt execution again
			next.SetValue( 0 );
			return false;
		}

		// Otherwise, keep running this one frame since we are still finishing this frame
		return true;
	}
	else
	{
		// Remember last frame without "next" being reset ( time is locked )
		current_tick = host_tickcount;
		// Time is locked
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Host_DumpMemoryStats( void )
{
#ifdef _WIN32
	_CrtMemState state;
	memset( &state, 0, sizeof( state ) );
	_CrtMemCheckpoint( &state );

	unsigned int size = 0;

    for ( int use = 0; use < _MAX_BLOCKS; use++)
	{
		size += state.lSizes[ use ];
	}
	Msg("MEMORY:  Run-time Heap\n------------------------------------\n");

	Msg( "\tHigh water %s\n", Q_pretifymem( state.lHighWaterCount,4 ) );
	Msg( "\tCurrent mem %s\n", Q_pretifymem( size,4 ) );
	Msg("------------------------------------\n");
	int hunk = MEM_Summary_Console();
	Msg("\tAllocated outside hunk:  %s\n", Q_pretifymem( size - hunk ) );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Host_CheckDumpMemoryStats( void )
{
	if ( mem_dumpstats.GetInt() <= 0 )
		return;

	Host_DumpMemoryStats();

	if ( mem_dumpstats.GetInt() <= 1 )
	{
		mem_dumpstats.SetValue( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void _Host_SetGlobalTime()
{
	// Server
	g_ServerGlobalVariables.realtime			= realtime;
	g_ServerGlobalVariables.framecount			= host_framecount;
	g_ServerGlobalVariables.absoluteframetime	= host_frametime;
	g_ServerGlobalVariables.tick_interval		= TICK_RATE;

#ifndef SWDS
	// Client
	g_ClientGlobalVariables.realtime			= realtime;
	g_ClientGlobalVariables.framecount			= host_framecount;
	g_ClientGlobalVariables.absoluteframetime	= host_frametime;
	g_ClientGlobalVariables.tick_interval		= TICK_RATE;
#endif
}

/*
==================
_Host_RunFrame

Runs all active servers
==================
*/

void _Host_RunFrame_Input( float accumulated_extra_samples )
{
	VPROF( "_Host_RunFrame_Input" );


	// Run a test script?
	static bool bFirstFrame = true;
	if ( bFirstFrame )
	{
		bFirstFrame = false;
		const char *pScriptFilename = CommandLine()->ParmValue( "-TestScript" );
		if ( pScriptFilename )
		{
			if ( !GetTestScriptMgr()->StartTestScript( pScriptFilename ) )
			{
				Error( "StartTestScript( %s ) failed.", pScriptFilename );
			}
		}
	}

	g_HostTimes.StartFrameSegment( FRAME_SEGMENT_INPUT );

#ifndef SWDS
	// Client can process input
	ClientDLL_ProcessInput( );

	// Send any current movement commands to server and flush reliable buffer even if not moving yet.
	CL_Move ( accumulated_extra_samples );

	g_HostTimes.EndFrameSegment( FRAME_SEGMENT_INPUT );

	g_HostTimes.StartFrameSegment( FRAME_SEGMENT_CMD_EXECUTE );

	// process console commands
	Cbuf_Execute ();

	g_HostTimes.EndFrameSegment( FRAME_SEGMENT_CMD_EXECUTE );
#endif
}

void _Host_RunFrame_Server( bool finaltick )
{
	bool send_client_updates = finaltick;

	VPROF_BUDGET( "_Host_RunFrame_Server", VPROF_BUDGETGROUP_GAME );

	// Run the Server frame ( read, run phsyiscs, respond )
	g_HostTimes.StartFrameSegment( FRAME_SEGMENT_SERVER );
	SV_Frame ( send_client_updates );
	g_HostTimes.EndFrameSegment( FRAME_SEGMENT_SERVER );

	// Look for connectionless rcon packets on dedicated servers
	SV_CheckRcom();

	SV_CheckTimeouts();
	
}

void _Host_RunFrame_Client( bool framefinished )
{
#ifndef SWDS
	VPROF( "_Host_RunFrame_Client" );

	g_HostTimes.StartFrameSegment( FRAME_SEGMENT_CLIENT );

	// Get any current state update from server, etc.
	ClientDLL_FrameStageNotify( FRAME_NET_UPDATE_START );
	CL_ReadPackets( framefinished );
	ClientDLL_FrameStageNotify( FRAME_NET_UPDATE_END );

#if defined VOICE_OVER_IP && defined _WIN32
	// Send any enqueued voice data to the server
	CL_ProcessVoiceData();
#endif // VOICE_OVER_IP

	// Resend connection request if needed.
	CL_CheckForResend ();

	g_HostTimes.EndFrameSegment( FRAME_SEGMENT_CLIENT );
#endif
}

void _Host_RunFrame_Render()
{
#ifndef SWDS
	VPROF( "_Host_RunFrame_Render" );

	int nNoRendering = 0;
	if ( demo->IsTakingSnapshot() )
	{
		// If we started in no-rendering mode, get us out!
		nNoRendering = mat_norendering.GetInt();
		mat_norendering.SetValue( 0 );

		// FIXME!!  Cbuf_AddText doesn't accept a const char *?!?!!
		const char *pSnapshotFilename = demo->GetSnapshotFilename();
		char *pBuf = ( char * )_alloca( strlen( "snapshot"  ) + strlen( pSnapshotFilename ) + 2 );
		Q_strcpy( pBuf, "snapshot " );
		Q_strcat( pBuf, pSnapshotFilename );
		Q_strcat( pBuf, "\n" );
		Cbuf_AddText( pBuf );
		Cbuf_Execute();
	}

	// Used for netgaph.
//	CL_UpdateFrameLerp();

	// update video if not running in background
	g_HostTimes.StartFrameSegment( FRAME_SEGMENT_RENDER );

	{
		VPROF( "_Host_RunFrame_Render - UpdateScreen" );
		Host_UpdateScreen();
	}
	g_HostTimes.MarkSwapTime( );
	{
		VPROF( "_Host_RunFrame_Render - CL_DecayLights" );
		CL_DecayLights ();
	}

	g_HostTimes.EndFrameSegment( FRAME_SEGMENT_RENDER );


	// This is undoing the set in the check above
	if ( demo->IsTakingSnapshot() )
	{
		// If we ran this from the command-line, quit after taking the snapshot.
		if( CommandLine()->FindParm( "-benchframe" ) )
		{
			Cbuf_AddText( "quit\n" );
			Cbuf_Execute();
		}
		mat_norendering.SetValue( nNoRendering );
	}
#endif
}

void CL_ApplyAddAngle()
{
	float frametime = cl.getframetime();

	float amove = 0.0f;

	for ( int i = 0; i < cl.addangle.Count(); i++ )
	{
		AddAngle *add = &cl.addangle[ i ];
		
		float remainder = fabs( add->yawdelta - add->accum );
		if ( remainder <= 0.0001f )
		{
			cl.addangle.Remove( i );
			--i;
			continue;
		}

		// Apply some of the delta
		float f = frametime / TICK_RATE;
		f = clamp( f, 0.0f, 1.0f );

		float amount_to_add = f * add->yawdelta;

		if ( add->yawdelta > 0.0f )
		{
			amount_to_add = min( amount_to_add, remainder );
		}
		else
		{
			amount_to_add = max( amount_to_add, -remainder );
		}

		add->accum += amount_to_add;
		
		amove += amount_to_add;
	}

	cl.viewangles[ 1 ] += amove;
}

void _Host_RunFrame_Sound()
{
#ifndef SWDS
	VPROF_BUDGET( "_Host_RunFrame_Sound", VPROF_BUDGETGROUP_OTHER_SOUND );

	g_HostTimes.StartFrameSegment( FRAME_SEGMENT_SOUND );

	Host_UpdateSounds();

	g_HostTimes.EndFrameSegment( FRAME_SEGMENT_SOUND );
#endif
}

void _Host_RunFrame (float time)
{
	if ( host_checkheap )
	{
#ifdef _WIN32
		if ( _heapchk() != _HEAPOK )
		{
			Sys_Error( "_Host_RunFrame (top):  _heapchk() != _HEAPOK\n" );
		}
#endif
	}

	// Reset profiling stuff
	ResetTimeMeasurements();

	if ( setjmp ( host_enddemo ) )
		return;			// demo finished.

	// decide the simulation time
	if ( !Host_FilterTime ( time ) )
		return;			

	_Host_SetGlobalTime();

	// FIXME:  Could track remainder as fractional ticks instead of msec
	float prevremainder = host_remainder;
	float time_since_last_tick = host_remainder;
#ifndef SWDS
	if ( !demo->IsPlaybackPaused() )
	{
		time_since_last_tick += host_frametime;
	}
#endif
	float extramousetime = 0.0f;

	int numticks = 0;
	if ( time_since_last_tick >= TICK_RATE )
	{
		numticks = (int)( (time_since_last_tick / TICK_RATE ) );
		host_remainder = time_since_last_tick - numticks * TICK_RATE;
		extramousetime = 0;
	}
	else
	{
		host_remainder = time_since_last_tick;
		extramousetime = host_frametime;
	}

	{
	// Profile scope, protect from setjmp() problems
	VPROF( "_Host_RunFrame" );

	g_HostTimes.StartFrameSegment( FRAME_SEGMENT_CMD_EXECUTE );

	// process console commands
	Cbuf_Execute ();

	g_HostTimes.EndFrameSegment( FRAME_SEGMENT_CMD_EXECUTE );

	// Msg( "Running %i ticks (%f remainder) for frametime %f total %f tick %f delta %f\n", numticks, remainder, host_frametime, host_time );
	g_ServerGlobalVariables.interpolation_amount = 0.0f;
#ifndef SWDS
	g_ClientGlobalVariables.interpolation_amount = 0.0f;
	
	cl.tickremainder = 0.0f;
	cl.insimulation = true;
#endif

	host_frameticks = numticks;
	host_currentframetick = 0;

	for ( int tick = 0; tick < numticks; tick++ )
	{ 
		// NOTE:  Do we want do this at start or end of this loop?
		++host_tickcount;
		++host_currentframetick;

		g_ServerGlobalVariables.tickcount = sv.tickcount;
#ifndef SWDS
		g_ClientGlobalVariables.tickcount = cl.tickcount;

		// Make sure state is correct
		CL_CheckClientState();
#endif

		//-------------------
		// input processing
		//-------------------
		_Host_RunFrame_Input( prevremainder );
		prevremainder = 0;

		//-------------------
		//
		// server operations
		//
		//-------------------
		// Only send updates to clients on final tick so we don't reencode network data multiple times per frame unnecessarily
		bool finaltick = ( tick == numticks - 1 ) ? true : false;
		_Host_RunFrame_Server( finaltick );

		//-------------------
		//
		// client operations
		//
		//-------------------
#ifndef SWDS
		_Host_RunFrame_Client( finaltick );
#endif
	}

	// This is a hack to let timedemo pull messages from the queue faster than every 15 msec
	if ( numticks == 0 && 
		Demo_IsPlayingBack_TimeDemo() )
	{
		_Host_RunFrame_Client( true );
	}

#ifndef SWDS
	// This causes cl.gettime() to return the true clock being used for rendering (tickcount * rate + remainder)
	cl.tickremainder	= host_remainder;
	cl.insimulation		= false;

	if ( cl.getframetime() == 0.0f )
	{
		cl.tickremainder = 0.0f;
	}
	// Now allow for interpolation on client
	g_ClientGlobalVariables.interpolation_amount = ( host_remainder / TICK_RATE );

	// Compute absolute/render time stamp
	g_ClientGlobalVariables.curtime = cl.gettime();
	g_ClientGlobalVariables.frametime = cl.getframetime();

	//-------------------
	// Run prediction if it hasn't been run yet
	//-------------------
	// If we haven't predicted/simulated the player (multiplayer with prediction enabled and
	//  not a listen server with zero frame lag, then go ahead and predict now
	if ( !CL_HasRunPredictionThisFrame() )
	{
		CL_RunPrediction( PREDICTION_NORMAL );
	}
	
	CL_ApplyAddAngle();

	// Make sure mouse is not lagged regardless
	if ( extramousetime > 0.0f )
	{
		CL_ExtraMouseUpdate( extramousetime );
	}
#endif

	//-------------------
	// rendering
	//-------------------
	_Host_RunFrame_Render();
	
	//-------------------
	// sound
	//-------------------
	_Host_RunFrame_Sound();

	
	//-------------------
	// simulation
	//-------------------
#ifndef SWDS
	{
		VPROF( "_Host_RunFrame - ClientDLL_Update" );
		// Client-side simulation
		g_HostTimes.StartFrameSegment( FRAME_SEGMENT_CLDLL );

		ClientDLL_Update();

		g_HostTimes.EndFrameSegment( FRAME_SEGMENT_CLDLL );

	}
#endif

	//-------------------
	// time
	//-------------------
	
	Host_Speeds();

	Host_UpdateMapList();

	host_framecount++;
#ifndef SWDS
	if ( !demo->IsPlaybackPaused() )
#endif
	{
		host_time += host_frametime;
	}

	Host_PostFrameRate( host_frametime );

	if ( host_checkheap )
	{
#ifdef _WIN32
		if ( _heapchk() != _HEAPOK )
		{
			Sys_Error( "_Host_RunFrame (bottom):  _heapchk() != _HEAPOK\n" );
		}
#endif
	}

	Host_CheckDumpMemoryStats();

	GetTestScriptMgr()->CheckPoint( "frame_end" );
	
	} // Profile scope, protect from setjmp() problems
}

/*
==============================
Host_Frame

==============================
*/
int Host_RunFrame( float time, int iState )
{
	double	time1, time2;

	if ( setjmp (host_abortserver) )
	{
		return giActive;			// something bad happened, or the server disconnected
	}
	g_bAbortServerSet = true;

	giActive = iState;

	if ( host_profile.GetInt() )
	{
		time1 = Sys_FloatTime();
	}

	_Host_RunFrame( time );

	if ( host_profile.GetInt() )
	{
		time2 = Sys_FloatTime();
	}

	if ( host_profile.GetInt() )
	{
		static double	timetotal;
		static int		timecount;
		int		i, c, m;

		timetotal += time2 - time1;
		timecount++;
		
		if (timecount < 1000)
			return giActive;

		m = timetotal*1000/timecount;
		timecount = 0;
		timetotal = 0;
		c = 0;
		for (i=0 ; i<svs.maxclients ; i++)
		{
			if (svs.clients[i].active)
				c++;
		}
		Con_Printf ("host_profile: %2i clients %2i msec\n",  c,  m);
	}

	return giActive;
}


//============================================================================


/*extern FileHandle_t vcrFile;
#define	VCR_SIGNATURE	0x56435231
// "VCR1"

void Host_InitVCR (engineparms_t *parms)
{
	int		i, len, n;
	char	*p;
	
	if (COM_CheckParm("-playback"))
	{
		if (com_argc != 2)
			Sys_Error("No other parameters allowed with -playback\n");

		vcrFile = g_pFileSystem->OpenRead( "halflife.vcr", "rb" );
		if (vcrFile == -1)
			Sys_Error("playback file not found\n");

		g_pFileSystem->Read( &i, sizeof(int), vcrFile );
		if (i != VCR_SIGNATURE)
			Sys_Error("Invalid signature in vcr file\n");

		g_pFileSystem->Read( &com_argc, sizeof(int), vcrFile );
		com_argv = (char **)malloc(com_argc * sizeof(char *));
		com_argv[0] = parms->argv[0];
		for (i = 0; i < com_argc; i++)
		{
			g_pFileSystem->Read ( &len, sizeof(int), vcrFile );
			p = (char *)malloc(len);
			g_pFileSystem->Read ( p, len, vcrFile );
			com_argv[i+1] = p;
		}
		com_argc++; // add one for arg[0]
		parms->argc = com_argc;
		parms->argv = com_argv;
	}

	if ( (n = COM_CheckParm("-record")) != 0)
	{
		vcrFile = g_pFileSystem->Open( "halflife.vcr", "wb" );

		i = VCR_SIGNATURE;
		g_pFileSystem->Write(&i, sizeof(int), vcrFile);
		i = com_argc - 1;
		g_pFileSystem->Write(&i, sizeof(int), vcrFile);
		for (i = 1; i < com_argc; i++)
		{
			if (i == n)
			{
				len = 10;
				g_pFileSystem->Write(&len, sizeof(int), vcrFile);
				g_pFileSystem->Write("-playback", len,vcrFile);
				continue;
			}
			len = Q_strlen(com_argv[i]) + 1;
			g_pFileSystem->Write(&len, sizeof(int), vcrFile);
			g_pFileSystem->Write(com_argv[i], len, vcrFile);
		}
	}
	
}
*/

//-----------------------------------------------------------------------------
// Purpose: If "User Token 2" exists in HKEY_CURRENT_USER/Software/Valve/Half-Lfe/Settings
//   then we "disable" gore.  Obviously not very "secure"
//-----------------------------------------------------------------------------
void Host_CheckGore( void )
{
	char szSubKey[128];
	int nBufferLen;
	char szBuffer[128];
	qboolean bReducedGore = false;
	
	memset( szBuffer, 0, 128 );

#if defined(_WIN32)
	Q_snprintf(szSubKey, sizeof( szSubKey ), "Software\\Valve\\%s\\Settings", gpszAppName );

	nBufferLen = 127;
	strcpy( szBuffer, "" );

	Sys_GetRegKeyValue( szSubKey, "User Token 2", szBuffer,	nBufferLen, szBuffer );

	// Gore reduction active?
	bReducedGore = ( strlen( szBuffer ) > 0 ) ? true : false;
	if ( !bReducedGore )
	{
		Sys_GetRegKeyValue( szSubKey, "User Token 3", szBuffer, nBufferLen, szBuffer );

		bReducedGore = ( strlen( szBuffer ) > 0 ) ? true : false;
	}
#endif
	// Gore reduction active
	if ( bReducedGore )
	{
		violence_hblood.SetValue( 0 );
		violence_hgibs.SetValue( 0 );
		violence_ablood.SetValue( 0 );
		violence_agibs.SetValue( 0 );
	}
	else
	{
		violence_hblood.SetValue( 1 );
		violence_hgibs.SetValue( 1 );
		violence_ablood.SetValue( 1 );
		violence_agibs.SetValue( 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void Host_InitProcessor( void )
{
	const CPUInformation& pi = GetCPUInformation();

	// Compute Frequency in Mhz: 
	char* szFrequencyDenomination = "Mhz";
	double fFrequency = pi.m_Speed / 1000000.0;

	// Adjust to Ghz if nessecary:
	if( fFrequency > 1000.0 )
	{
		fFrequency /= 1000.0;
		szFrequencyDenomination = "Ghz";
	}

	char szFeatureString[256];
	Q_strcpy( szFeatureString, pi.m_szProcessorID );
	Q_strcat( szFeatureString, " " );
	
	if( pi.m_bSSE )
	{
		if( MathLib_SSEEnabled() ) Q_strcat(szFeatureString, "SSE " );
		else					   Q_strcat(szFeatureString, "(SSE) " );
	}
	
	if( pi.m_bSSE2 )
	{
		if( MathLib_SSE2Enabled() ) Q_strcat(szFeatureString, "SSE2 " );
		else					   Q_strcat(szFeatureString, "(SSE2) " );
	}
	
	if( pi.m_bMMX )
	{
		if( MathLib_MMXEnabled() ) Q_strcat(szFeatureString, "MMX " );
		else					   Q_strcat(szFeatureString, "(MMX) " );
	}

	if( pi.m_b3DNow )
	{
		if( MathLib_3DNowEnabled() ) Q_strcat(szFeatureString, "3DNow " );
		else					   Q_strcat(szFeatureString, "(3DNow) " );
	}

	if( pi.m_bRDTSC )	Q_strcat(szFeatureString, "RDTSC " );
	if( pi.m_bCMOV )	Q_strcat(szFeatureString, "CMOV " );
	if( pi.m_bFCMOV )	Q_strcat(szFeatureString, "FCMOV " );

	// Remove the trailing space.  There will always be one.
	szFeatureString[Q_strlen(szFeatureString)-1] = '\0';

	// Dump CPU information:
	if( pi.m_nLogicalProcessors == 1 )
	{
		Con_DPrintf( "1 CPU, Frequency: %.01f %s,  Features: %s\n", 
			fFrequency,
			szFrequencyDenomination,
			szFeatureString
			);
	} else
	{
		char buffer[256] = "";
		if( pi.m_nPhysicalProcessors != pi.m_nLogicalProcessors )
		{
			sprintf(buffer, " (%i physical)", (int) pi.m_nPhysicalProcessors );
		}

		Con_DPrintf( "%i CPUs%s, Frequency: %.01f %s,  Features: %s\n", 
			(int)pi.m_nLogicalProcessors,
			buffer,
			fFrequency,
			szFrequencyDenomination,
			szFeatureString
			);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Host_Init( void )
{
	realtime = 0;
	cls.state = ( isDedicated ) ? ca_dedicated : ca_disconnected;

	InstallBitBufErrorHandler();

	TRACEINIT( Memory_Init(), Memory_Shutdown() );

	TRACEINIT( Con_Init(), Con_Shutdown() );
	
	TRACEINIT( Cbuf_Init(), Cbuf_Shutdown() );

	TRACEINIT( Cmd_Init(), Cmd_Shutdown() );	

	TRACEINIT( cv->Init(), cv->Shutdown() ); // So we can list cvars with "cvarlst"

#ifndef SWDS
	TRACEINIT( V_Init(), V_Shutdown() );
#endif

	//TRACEINIT( Host_InitVCR(), Host_ShutdownVCR() );

	TRACEINIT( COM_Init(), COM_Shutdown() );

#ifndef SWDS
	TRACEINIT( saverestore->Init(), saverestore->Shutdown() );
#endif

	TRACEINIT( Filter_Init(), Filter_Shutdown() );

#ifndef SWDS
	TRACEINIT( Key_Init(), Key_Shutdown() );
#endif

	TRACEINIT( NET_Init(), NET_Shutdown() );     
	
	// Sequenced message stream layer.
	TRACEINIT( Netchan_Init(), Netchan_Shutdown() );  

	TRACEINIT( SV_Init(), SV_Shutdown() );

	// Allow master server interface to register its commands
	TRACEINIT( master->Init(), master->Shutdown() );
	
	TRACEINIT( g_pGameEventManager->Init(), g_pGameEventManager->Shutdown() );

	TRACEINIT( g_Log.Init(), g_Log.Shutdown() );
	
	Con_DPrintf( "Heap:  %5.2f Mb\n", host_parms.memsize/ (1024*1024.0) );
	
#if defined _WIN32 && !defined SWDS
	if ( cls.state != ca_dedicated )
	{
		TRACEINIT( VID_Init(), VID_Shutdown() );

		TRACEINIT( InitMaterialSystem(), ShutdownMaterialSystem() );

		TRACEINIT( modelloader->Init(), modelloader->Shutdown() );

		TRACEINIT( StaticPropMgr()->Init(), StaticPropMgr()->Shutdown() );

		TRACEINIT( InitStudioRender(), ShutdownStudioRender() );

		//startup vgui
		TRACEINIT( VGui_Startup(), VGui_Shutdown() );

		TRACEINIT( S_Init(), S_Shutdown() );

		TRACEINIT( TextMessageInit(), TextMessageShutdown() );

		TRACEINIT( ClientDLL_Init(), ClientDLL_Shutdown() );

		TRACEINIT( Draw_Init(), Draw_Shutdown() );

		TRACEINIT( SCR_Init(), SCR_Shutdown() );

		TRACEINIT( R_Init(), R_Shutdown() ); 

		TRACEINIT( Decal_Init(), Decal_Shutdown() );

		TRACEINIT( CL_Init(), CL_Shutdown() );
	}
	else
#endif
	{
		TRACEINIT( InitMaterialSystem(), ShutdownMaterialSystem() );

		TRACEINIT( modelloader->Init(), modelloader->Shutdown() );

		TRACEINIT( StaticPropMgr()->Init(), StaticPropMgr()->Shutdown() );

		TRACEINIT( InitStudioRender(), ShutdownStudioRender() );

		TRACEINIT( Decal_Init(), Decal_Shutdown() );
	}

#ifndef SWDS
	// Don't trace into this
	Host_ReadConfiguration();
#endif

	// Execute valve.rc
	Cbuf_InsertText( "exec valve.rc\n" );

	// Mark DLL as active
	giActive = DLL_ACTIVE;

#ifndef SWDS
	// Enable rendering
	scr_skipupdate = false;
#endif

	// Check for special -dev flag
	if ( CommandLine()->FindParm("-dev") )
	{
		sv_cheats.SetValue( 1 );
		developer.SetValue( 1 );
	}

	// Deal with Gore Settings
	Host_CheckGore();

	// Initialize processor subsystem, and print relevant information:
	Host_InitProcessor();

	// Mark hunklevel at end of startup
	Hunk_AllocName( 0, "-HOST_HUNKLEVEL-" );
	host_hunklevel = Hunk_LowMark ();

	// Finished initializing
	host_initialized = true;

	host_checkheap = CommandLine()->FindParm( "-heapcheck" ) ? true : false;

	if ( host_checkheap )
	{
#ifdef _WIN32
		if ( _heapchk() != _HEAPOK )
		{
			Sys_Error( "Host_Init:  _heapchk() != _HEAPOK\n" );
		}
#endif
	}

	// go directly to run state with no active game
	HostState_Init();

	// Initialize datatable instrumentation.
	if( CommandLine()->FindParm("-dti" ) )
	{
		char *pDataTraceFilename = 0;
		if ( CommandLine()->FindParm( "-dti_datatrace" ) )
		{
			pDataTraceFilename = "dti.datatrace";
		}

		DTI_Init( "dti_client.txt" );
		ServerDTI_Init( "dti_server.txt" );
	}
}

void Host_Changelevel( bool loadfromsavedgame, const char *mapname, const char *start )
{
	char			level[MAX_QPATH];
	char			_startspot[MAX_QPATH];
	char			*startspot;
	char			oldlevel[MAX_QPATH];
#ifndef SWDS
	CSaveRestoreData *pSaveData = NULL;
#endif

	if ( !sv.active )
	{
		Con_Printf ("Only the server may changelevel\n");
		return;
	}

	// FIXME:  Even needed?
	if ( Demo_IsPlayingBack() )
	{
		Con_Printf ("Changelevel invalid during demo playback\n");
		return;
	}
#ifndef SWDS
	SCR_BeginLoadingPlaque();
#endif
#if defined _WIN32 && !defined SWDS
	// stop sounds (especially looping!)
	S_StopAllSounds (true);
#endif

	strcpy( level, mapname );
	if ( !start )
		startspot = NULL;
	else
	{
		strcpy (_startspot, start );
		startspot = _startspot;
	}

	strcpy( oldlevel, sv.name );

#ifndef SWDS
	if ( loadfromsavedgame )
	{
		// save the current level's state
		pSaveData = saverestore->SaveGameState();
	}
#endif
	serverGameDLL->LevelShutdown();

	SV_InactivateClients();
	if ( !SV_SpawnServer( level, startspot ) )
	{
		return;
	}
	
#ifndef SWDS
	if ( loadfromsavedgame )
	{
		// Finish saving gamestate
		saverestore->Finish( pSaveData );

		g_ServerGlobalVariables.curtime = sv.gettime();
		serverGameDLL->LevelInit( level, CM_EntityString(), oldlevel, startspot, true );
		sv.paused = true;		// pause until all clients connect
		sv.loadgame = true;
	}
	else
#endif
	{
		g_ServerGlobalVariables.curtime = sv.gettime();
		serverGameDLL->LevelInit( level, CM_EntityString(), NULL, NULL, false );
	}

	SV_ActivateServer();
}

static void    StripExtension (char *path)
{
	int             length;

	COM_FixSlashes( path );

	length = strlen(path)-1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}


// There's a version of this in bsplib.cpp!!!  Make sure that they match.
void GetPlatformMapPath( const char *pMapPath, char *pPlatformMapPath, int maxLength )
{
	Q_strncpy( pPlatformMapPath, pMapPath, maxLength );

	// It's OK for this to be NULL on the dedicated server.
	if( g_pMaterialSystemHardwareConfig )
	{
		int dxlevel = g_pMaterialSystemHardwareConfig->GetDXSupportLevel();
		StripExtension( pPlatformMapPath );
		if( dxlevel <= 60 )
		{
			Q_strncat( pPlatformMapPath, "_dx60", maxLength );
		}
		Q_strncat( pPlatformMapPath, ".bsp", maxLength );
	}
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/
bool Host_NewGame( char *mapName, bool loadGame )
{
	extern char	*CM_EntityString( void );
	extern ConVar host_map;

	char dxMapName[MAX_PATH];
	GetPlatformMapPath( mapName, dxMapName, MAX_PATH );
	host_map.SetValue( dxMapName );

#ifndef SWDS
	CL_Disconnect();

	SCR_BeginLoadingPlaque ();
	// stop sounds (especially looping!)
	S_StopAllSounds (true);
#endif

	if ( !loadGame )
	{
		HostState_RunGameInit();
	}

	if ( !SV_SpawnServer ( mapName, NULL ) )
		return false;

	// make sure the time is set
	g_ServerGlobalVariables.curtime = sv.gettime();

	serverGameDLL->LevelInit( mapName, CM_EntityString(), NULL, NULL, loadGame );

	if ( loadGame )
	{
		sv.paused = true;		// pause until all clients connect
		sv.loadgame = true;
		g_ServerGlobalVariables.curtime = sv.gettime();
	}

	SV_ActivateServer();

	if ( !sv.active )
	{
		return false;
	}

	// Connect the local client when a "map" command is issued.
	if (cls.state != ca_dedicated)
	{
		Cmd_ExecuteString ("connect local", src_command);
	}

	// Dedicated server triggers map load here.
	if ( isDedicated )
	{
		GetTestScriptMgr()->CheckPoint( "FinishedMapLoad" );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Host_FreeToLowMark( bool server )
{
	assert( host_initialized );
	assert( host_hunklevel );

	// If called by the client and we are running a listen server, just ignore
	if ( !server && sv.active )
		return;

	// HACKHACK: You can't clear the hunk unless the client data is free
	// since this gets called by the server, it's necessary to wipe the client
	// in case we are on a listen server
#ifndef SWDS
	if ( server )
	{
		CL_ClearState();
	}
#endif

	// The world model relies on the hunk
	modelloader->ReleaseAllModels( IModelLoader::FMODELLOADER_SERVER );
	modelloader->ReleaseAllModels( IModelLoader::FMODELLOADER_CLIENT );

	modelloader->UnloadUnreferencedModels();

	if ( host_hunklevel )
	{
		// See if we are going to obliterate any malloc'd pointers
		Hunk_FreeToLowMark(host_hunklevel);
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Host_Shutdown(void)
{
	if ( host_checkheap )
	{
#ifdef _WIN32
		if ( _heapchk() != _HEAPOK )
		{
			Sys_Error( "Host_Shutdown (top):  _heapchk() != _HEAPOK\n" );
		}
#endif
	}

	// Check for recursive shutdown, should never happen
	static bool shutting_down = false;
	if ( shutting_down )
	{
		Sys_Printf( "Recursive shutdown!!!\n" );
		return;
	}
	shutting_down = true;

#ifndef SWDS
	// Store active configuration settings
	Host_WriteConfiguration( "cfg/config.cfg" ); 
#endif

	// Disconnect from server
	Host_Disconnect();

#ifndef SWDS
	// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true;
#endif

#if defined VOICE_OVER_IP && !defined SWDS
	Voice_Deinit();
#endif // VOICE_OVER_IP
	
	SV_ShutdownGameDLL();

	// TODO, Trace this
	CM_FreeMap();

	host_initialized = false;

#if defined _WIN32 && !defined SWDS
	if ( cls.state != ca_dedicated )
	{
		TRACESHUTDOWN( CL_Shutdown() );

		TRACESHUTDOWN( Decal_Shutdown() );

		TRACESHUTDOWN( R_Shutdown() );

		TRACESHUTDOWN( SCR_Shutdown() );

		TRACESHUTDOWN( Draw_Shutdown() );

		TRACESHUTDOWN( ClientDLL_Shutdown() );

		TRACESHUTDOWN( TextMessageShutdown() );

		TRACESHUTDOWN( S_Shutdown() );

		TRACESHUTDOWN( VGui_Shutdown() );

		TRACESHUTDOWN( StaticPropMgr()->Shutdown() );

		// Model loader must shutdown before StudioRender
		// because it calls into StudioRender
		TRACESHUTDOWN( modelloader->Shutdown() );

		TRACESHUTDOWN( ShutdownStudioRender() );
		
		TRACESHUTDOWN( ShutdownMaterialSystem() );

		TRACESHUTDOWN( VID_Shutdown() );
	}
	else
#endif
	{
		TRACESHUTDOWN( Decal_Shutdown() );

		TRACESHUTDOWN( ShutdownStudioRender() );

		TRACESHUTDOWN( StaticPropMgr()->Shutdown() );

		TRACESHUTDOWN( modelloader->Shutdown() );

		TRACESHUTDOWN( ShutdownMaterialSystem() );
	}

	TRACESHUTDOWN( g_Log.Shutdown() );

	TRACESHUTDOWN( g_pGameEventManager->Shutdown() );

	TRACESHUTDOWN( master->Shutdown() );

	TRACESHUTDOWN( SV_Shutdown() );

	TRACESHUTDOWN( Netchan_Shutdown() );

	TRACESHUTDOWN( NET_Shutdown() );

#ifndef SWDS
	TRACESHUTDOWN( Key_Shutdown() );
#endif

	TRACESHUTDOWN( Filter_Shutdown() );

#ifndef SWDS
	TRACESHUTDOWN( saverestore->Shutdown() );
#endif

	TRACESHUTDOWN( COM_Shutdown() );

	// TRACESHUTDOWN( Host_ShutdownVCR() );
#ifndef SWDS
	TRACESHUTDOWN( V_Shutdown() );
#endif

	TRACESHUTDOWN( cv->Shutdown() );

	TRACESHUTDOWN( Cmd_Shutdown() );

	TRACESHUTDOWN( Cbuf_Shutdown() );

	TRACESHUTDOWN( Con_Shutdown() );

	TRACESHUTDOWN( Memory_Shutdown() );

	DTI_Term();
	ServerDTI_Term();

	if ( host_checkheap )
	{
#ifdef _WIN32
		if ( _heapchk() != _HEAPOK )
		{
			Sys_Error( "Host_Shutdown (bottom):  _heapchk() != _HEAPOK\n" );
		}
#endif
	}
}
