//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifdef _WIN32
#include <windows.h>
#endif
#include "quakedef.h"
#include "cdll_int.h"
#include "tmessage.h"
#include "screen.h"
#include "shake.h"
#include "iprediction.h"
#include "host.h"
#include "keys.h"
#include "demo.h"
#include "cdll_engine_int.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "sys.h"
#include "enginestats.h"
#if defined _WIN32 && !defined SWDS
#include "enginesoundinternal.h"
#endif
#include "host_state.h"
#include "proto_version.h"
#include "cl_demoactionmanager.h"
#include "cl_pred.h"
#include "baseautocompletefilelist.h"
#include "cl_demosmoothing.h"
#include "gl_matsysiface.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "vstdlib/icommandline.h"
#include "vengineserver_impl.h"
#include "console.h"
#include "utlbuffer.h"
#include "networkstringtableclient.h"

#ifdef SWDS
#include "server.h"
#endif

void CL_CopyChunk( FileHandle_t dst, FileHandle_t src, int nSize );

// Game Gauge profiling support
static ConVar demo_recordcommands( "demo_recordcommands", "1", 0, "Record commands typed at console into .dem files." );
static ConVar demo_debug( "demo_debug", "0", 0, "Demo debug info." );

static char g_pStatsFile[MAX_OSPATH] = { NULL };

extern ConVar cl_showmessages;

// This is the number of units under which we are allowed to interpolate, otherwise pop.
// This fixes problems with in-level transitions.
#define DEMO_INTERP_LIMIT	64.0f

// Fast forward convars
static ConVar demo_fastforwardstartspeed( "demo_fastforwardstartspeed", "2", 0, "Go this fast when starting to hold FF button." );
static ConVar demo_fastforwardfinalspeed( "demo_fastforwardfinalspeed", "20", 0, "Go this fast when starting to hold FF button." );
static ConVar demo_fastforwardramptime( "demo_fastforwardramptime", "5", 0, "How many seconds it takes to get to full FF speed." );

extern float src_demo_override_fov;

struct DemoCommandQueue
{
	DemoCommandQueue()
	{
		t = 0.0;
	}
	float			t;
	democmdinfo_t	info;
};

//-----------------------------------------------------------------------------
// Purpose: Implements IDemo and handles demo file i/o
// Demos are more or less driven off of network traffic, but there are a few
//  other kinds of data items that are also included in the demo file:  specifically
//  commands that the client .dll itself issued to the engine are recorded, though they
//  probably were not the result of network traffic.
// At the start of a connection to a map/server, all of the signon, etc. network packets
//  are buffered.  This allows us to actually decide to start recording the demo at a later
//  time.  Once we actually issue the recording command, we don't actually start recording 
//  network traffic, but instead we ask the server for an "uncompressed" packet (otherwise
//  we wouldn't be able to deal with the incoming packets during playback because we'd be missing the
//  data to delta from ) and go into a waiting state.  Once an uncompressed packet is received, 
//  we unset the waiting state and start recording network packets from that point forward.
// Demo's record the elapsed time based on the current client clock minus the time the demo was started
// During playback, the elapsed time for playback ( based on the host_time, which is subject to the
//  host_frametime cvar ) is compared with the elapsed time on the message from the demo file.  
// If it's not quite time for the message yet, the demo input stream is rewound
// The demo system sits at the point where the client is checking to see if any network messages
//  have arrived from the server.  If the message isn't ready for processing, the demo system
//  just responds that there are no messages waiting and the client continues on
// Once a true network message with entity data is read from the demo stream, a couple of other
//  actions occur.  First, the timestamp in the demo file and the view origin/angles corresponding
//  to the message are cached off.  Then, we search ahead (into the future) to find out the next true network message
//  we are going to read from the demo file.  We store of it's elapsed time and view origin/angles
// Every frame that the client is rendering, even if there is no data from the demo system,
//  the engine asks the demo system to compute an interpolated origin and view angles.  This
//  is done by taking the current time on the host and figuring out how far that puts us between
//  the last read origin from the demo file and the time when we'll actually read out and use the next origin
// We use Quaternions to avoid gimbel lock on interpolating the view angles
// To make a movie recorded at a fixed frame rate you would simply set the host_framerate to the
//  desired playback fps ( e.g., 0.02 == 50 fps ), then issue the startmovie command, and then
//  play the demo.  The demo system will think that the engine is running at 50 fps and will pull
//  messages accordingly, even though movie recording kills the actually framerate.
// It will also render frames with render origin/angles interpolated in-between the previous and next origins
//  even if the recording framerate was not 50 fps or greater.  The interpolation provides a smooth visual playback 
//  of the demo information to the client without actually adding latency to the view position (because we are
//  looking into the future for the position, not buffering the past data ).
//-----------------------------------------------------------------------------
class CDemo : public IDemo
{
public:
					CDemo( void );
					~CDemo( void );

	void			Init( void );
	void			Shutdown( void );

	void			StopPlayback( void );
	void			StopRecording( void );

	bool			IsPlayingBack( void ) const;
	bool			IsPlayingBack_TimeDemo( void );
	bool			IsTimeDemoTimerStarted( void );

	bool			IsRecording( void );

	void			RecordCommand( const char *cmdname );

	void			MarkFrame( float flCurTime, float flFPSVariability );

	void			StartupDemoHeader( void );

	bool			ReadMessage( void );

	void			WriteMessage( bool startup, int start, sizebuf_t *msg );

	void			SetWaitingForUpdate( bool waiting );
	bool			IsWaitingForUpdate( void );

	void			GetInterpolatedViewpoint( void );

	void			PausePlayback( float autoresumeafter = 0.0f );
	void			ResumePlayback();
	bool			IsPlaybackPaused() const;

	void			SkipToFrame( int whentoresume, bool pauseafterskip = false );
	void			SkipToTime( float whentoresume, bool pauseafterskip = false );

	float			GetProgress( void );
	void			GetProgressFrame( int &frame, int& totalframes );
	void			GetProgressTime( float& current, float& totaltime );

	void			SetAutoPauseAfterInit( bool pause );
	float			GetPlaybackTimeScale( void ) const;
	void			SetFastForwarding( bool ff );
	void			AdvanceOneFrame( void );

	void			WriteUserCmd( int cmdnumber );
	bool			IsFastForwarding( void );
	bool			IsSkippingAhead( void );

	void			QuitAfterTimeDemo()					{ m_bQuitAfterTimeDemo = true; }
	void			SnapshotAfterFrame( int nFrame )	{ m_nSnapshotFrame = nFrame; }
	void			SetSnapshotFilename( const char *pFileName );
	const char *		GetSnapshotFilename( void );

	virtual float			GetPlaybackRateModifier( void ) const;
	virtual void			SetPlaybackRateModifier( float rate );

	virtual void			DispatchEvents( void );

	virtual void			DrawDebuggingInfo( void );

	virtual void			LoadSmoothingInfo( const char *filename, CSmoothingContext& smoothing );
	virtual void			ClearSmoothingInfo( CSmoothingContext& smoothing );
	virtual void			SaveSmoothingInfo( char const *filename, CSmoothingContext& smoothing );

	virtual void			SetAutoRecord( char const *mapname, char const *demoname );
	virtual void			CheckAutoRecord( char const *mapname );

	virtual void			StartRendering( void );
 	virtual bool			IsTakingSnapshot();

public:

	bool			IsValidDemoHeader( void );


	void			Play( const char *name );
	void			Record( const char *name );
	int				GetNumFrames( const char *name );
	void			List( const char *name );

	void			Play_TimeDemo( const char *name );

private:
	bool			_ReadMessage( void );

	void			FinishTimeDemo( void );

	bool			MoveToNextSection( void );

	void			ReadSequenceInfo( FileHandle_t demofile, bool discard );
	void			ReadCmdInfo( FileHandle_t demofile, democmdinfo_t& info );
	void			ReadCmdHeader( FileHandle_t demofile, byte& cmd, float& dt, int& dframe );
	void			ReadUserCmd( FileHandle_t demofile, bool discard );

	void			WriteGetCmdInfo( democmdinfo_t& info );

	void			WriteSequenceInfo ( FileHandle_t fp );
	void			WriteCmdHeader( byte cmd, FileHandle_t fp );
	void			WriteCmdInfo( FileHandle_t fp, democmdinfo_t& info );

	void			WriteStringTables( FileHandle_t& demofile );
	void			ReadStringTables( int expected_length, FileHandle_t& demofile );

	float			GetPlaybackClock( void );
	float			GetRecordingClock( void );

	void			RewindMessageHeader( int oldposition );
	bool			ReadRawNetworkData( FileHandle_t demofile, int framecount, sizebuf_t& msg );

	bool			ParseAheadForInterval( float interval, CUtlVector< DemoCommandQueue >& list );
	void			InterpolateDemoCommand( CUtlVector< DemoCommandQueue >& list, float t, float& frac, democmdinfo_t& prev, democmdinfo_t& next );

	bool			ShouldSkip( float elapsed, int frame );

	bool			OverrideViewSetup( democmdinfo_t& info );
	void			ParseSmoothingInfo( FileHandle_t demofile, demodirectory_t& directory, CUtlVector< demosmoothing_t >& smooth );

	void			ClearAutoRecord( void );

	void			WriteTimeDemoResults( void );

private:
	struct SkipContext
	{
		bool		active;
		bool		useframe;
		bool		pauseafterskip;
		int			skiptoframe;
		float		skiptotime;

		float		skipstarttime;
		int			skipstartframe;

		int			skipframesremaining;
	};

	enum
	{
		GG_MAX_FRAMES = 300,
	};

	// Demo lump types
	enum
	{
		// This lump contains startup info needed to spawn into the server
		DEMO_STARTUP	= 0,    
		// This lump contains playback info of messages, etc., needed during playback.
		DEMO_NORMAL		= 1    
	};

	// Demo messages
	enum
	{
		// it's a startup message, process as fast as possible
		dem_norewind	= 1,
		// it's a normal network packet that we stored off
		dem_read,
		// move the demostart time value forward by this amount.
		dem_jumptime,
		// it's a command that the hud issued
		dem_clientdll,
		// usercmd
		dem_usercmd,
		// client string table state
		dem_stringtables,
		// end of time.
		dem_stop,

		// Last command
		dem_lastcmd		= dem_stop
	};

	// to meter out one message a frame
	int				m_nTimeDemoLastFrame;		
	// host_tickcount at start
	int				m_nTimeDemoStartFrame;		
	// realtime at second frame of timedemo
	float			m_flTimeDemoStartTime;		
	// Frame rate variability
	float			m_flTotalFPSVariability;
	// # of demo frames in this segment.
	int				m_nFrameCount;     

	// For synchronizing playback and recording.
	float			m_flStartTime;     
	// For synchronizing playback during timedemo.
	int				m_nStartTick;     
	// Name of demo file we are appending onto.
	char			m_szDemoFileName[ MAX_OSPATH ];  
	// For recording demos.
	FileHandle_t	m_pDemoFile;          
	// For saving startup data to start playing a demo midstream.
	FileHandle_t	m_pDemoFileHeader;  

	bool			m_bRecording;
	bool			m_bPlayingBack;
	bool			m_bTimeDemo;
	// I.e., demo is waiting for first nondeltacompressed message to arrive.
	//  We don't actually start to record until a non-delta message is received
	bool			m_bRecordWaiting;
	   
	// Game Gauge Stuff
	int				m_nGGMaxFPS;
	int				m_nGGMinFPS;
	int				m_nGGFrameCount;
	float			m_flGGTime;
	int				m_rgGGFPSFrames[ GG_MAX_FRAMES ];
	int				m_nGGSeconds;

	// Demo file i/o stuff
	demoentry_t*	m_pEntry;
	int				m_nEntryIndex;
	demodirectory_t	m_DemoDirectory;
	demoheader_t    m_DemoHeader;

	CUtlVector< DemoCommandQueue > m_DestCmdInfo;

	float			m_flLastFrameElapsed;
	democmdinfo_t	m_LastCmdInfo;

	bool			m_bInterpolateView;

	bool			m_bTimeDemoTimerStarted;
	int				m_nStartShowMessages;

	bool			m_bPlaybackPaused;
	float			m_flAutoResumeTime;
	SkipContext		m_Skip;
	bool			m_bAutoPause;
	bool			m_bFastForwarding;
	int				m_nAutoPauseFrame;

	float			m_flFastForwardStartTime;
	float			m_flPlaybackRateModifier;

	bool			m_bAutoRecord;
	char			m_szAutoRecordMap[ MAX_OSPATH ];
	char			m_szAutoRecordDemoName[ MAX_OSPATH ];

	bool			m_bQuitAfterTimeDemo;
	int				m_nSnapshotFrame;
	int				m_nFrameStartedRendering;
	char			m_pSnapshotFilename[MAX_OSPATH];
};

// Create object
static CDemo g_Demo;
IDemo *demo = ( IDemo * )&g_Demo;

bool Demo_IsPlayingBack()
{
	return demo->IsPlayingBack();
}
bool Demo_IsPlayingBack_TimeDemo()
{
	return demo->IsPlayingBack_TimeDemo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDemo::CDemo( void )
{
	m_bQuitAfterTimeDemo	= false;
	m_nSnapshotFrame		= -1;
	m_nFrameStartedRendering = -9999;
	m_nTimeDemoLastFrame	= 0;		
	m_nTimeDemoStartFrame	= 0;		
	m_flTimeDemoStartTime	= 0.0f;
	m_flTotalFPSVariability = 0.0f;
	m_bTimeDemoTimerStarted = false;

	m_nFrameCount			= 0;     
	m_flStartTime			= 0.0f;      
	m_nStartTick			= 0;     
	m_szDemoFileName[ 0 ]	= 0;  
	m_pDemoFile				= NULL;          
	m_pDemoFileHeader		= NULL;        

	m_bRecording			= false;
	m_bPlayingBack			= false;
	m_bPlaybackPaused		= false;
	m_bTimeDemo				= false;
	m_bRecordWaiting		= false;
	m_flAutoResumeTime		= 0.0f;

	// Game Gauge support stuff
	m_nGGMaxFPS				= 0;
	m_nGGMinFPS				= 0;
	m_nGGFrameCount			= 0;
	m_flGGTime				= 0.0f;
	m_nGGSeconds			= 0;

	m_pEntry				= NULL;
	m_nEntryIndex			= 0;
	memset( &m_DemoDirectory, 0, sizeof( m_DemoDirectory ) );
	memset( &m_DemoHeader, 0, sizeof( m_DemoHeader ) );

	m_flLastFrameElapsed	= 0.0f;
	memset( &m_LastCmdInfo, 0, sizeof( m_LastCmdInfo ) );

	m_bInterpolateView		= false;
	m_nStartShowMessages	= 0;
	memset( &m_Skip, 0, sizeof( m_Skip ) );
	m_bAutoPause			= false;
	m_bFastForwarding		= false;
	m_nAutoPauseFrame		= -1;

	m_flPlaybackRateModifier = 1.0f;

	m_bAutoRecord				= false;
	m_szAutoRecordMap[ 0 ]		= 0;
	m_szAutoRecordDemoName[ 0 ] = 0;
	m_pSnapshotFilename[ 0 ]	= 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDemo::~CDemo( void )
{
	// TODO:  Close any open file handles?
	Assert( !m_pDemoFile );
	if ( m_pDemoFileHeader )
	{
		g_pFileSystem->Close( m_pDemoFileHeader );
		m_pDemoFileHeader = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Mark whether we are waiting for the first uncompressed update packet
// Input  : waiting - 
//-----------------------------------------------------------------------------
void CDemo::SetWaitingForUpdate( bool waiting )
{
	m_bRecordWaiting = waiting;
}

//-----------------------------------------------------------------------------
// Purpose: Are we waiting for a full/uncompressed update to write into the demo file?
//-----------------------------------------------------------------------------
bool CDemo::IsWaitingForUpdate( void )
{
	return m_bRecordWaiting;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CDemo::GetRecordingClock( void )
{
#ifdef SWDS
	return host_time;
#else
	return cl.gettime();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CDemo::GetPlaybackClock( void ) 
{
	if ( !cl.insimulation )
	{
		return host_time + host_frametime;
	}
	else
	{
		return host_time + ( host_currentframetick - 1 ) * TICK_RATE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//			*fp - 
//-----------------------------------------------------------------------------
void CDemo::WriteCmdHeader( byte cmd, FileHandle_t fp )
{
	Assert( fp );
	Assert( cmd >= 1 && cmd <= dem_lastcmd );

	// Command
	g_pFileSystem->Write( &cmd, sizeof(byte), fp );

	// Time offset
	float	dt = LittleFloat((float)(GetRecordingClock() - m_flStartTime));
	int		dtick = LittleLong(host_tickcount - m_nStartTick);

	g_pFileSystem->Write (&dt, sizeof(float), fp);
	g_pFileSystem->Write(&dtick, sizeof(int), fp );
}

int CDemo::GetNumFrames( const char *name )
{
	int nCurrent;
	demoheader_t    header;
	demodirectory_t directory;
	demoentry_t     *pentry;

	m_pDemoFile = g_pFileSystem->Open (name, "rb");
	if (!m_pDemoFile)
	{
		Con_Printf ("ERROR: couldn't open %s.\n", name );
		return 0;
	}
	
	// Read in the header
	g_pFileSystem->Read( &header, sizeof(demoheader_t), m_pDemoFile );

	if ( strcmp ( header.demofilestamp, "HLDEMO" ) )
	{
		Con_Printf( "%s is not a demo file\n", name);
		g_pFileSystem->Close(m_pDemoFile);
		m_pDemoFile = NULL;
		return 0;
	}

	// Now read in the directory structure.
	g_pFileSystem->Seek( m_pDemoFile, header.directory_offset, FILESYSTEM_SEEK_HEAD );

	g_pFileSystem->Read( &directory.numentries, sizeof(int), m_pDemoFile );

	if (directory.numentries < 1 ||
		directory.numentries > 1024 )
	{
		Con_Printf ("CDemo::GetNumFrames: demo had bogus # of directory entries:  %i\n",
			directory.numentries);
		g_pFileSystem->Close(m_pDemoFile);
		m_pDemoFile = NULL;
		return 0;
	}

	demoentry_t entry;
	for ( int i = 0; i <  directory.numentries; i++ )
	{
		memset( &entry, 0, sizeof( entry ) );
		g_pFileSystem->Read( &entry, sizeof( entry ), m_pDemoFile );

		directory.entries.AddToTail( entry );
	}

	int numFrames = 0;
	
	for (nCurrent = 0; nCurrent < directory.numentries; nCurrent++)
	{
		pentry = &directory.entries[nCurrent];
		numFrames += pentry->playback_frames;
	}

	g_pFileSystem->Close(m_pDemoFile);
	m_pDemoFile = NULL;

	directory.entries.Purge();
	directory.numentries    = 0;
	return numFrames;
}

//-----------------------------------------------------------------------------
// Purpose: List the contents of a demo file.
// Input  : *name - 
//-----------------------------------------------------------------------------
void CDemo::List( const char *name )
{
	int nCurrent;
	demoheader_t    header;
	demodirectory_t directory;
	demoentry_t     *pentry;

	char type[32];

	Con_Printf ("Demo contents for %s.\n", name);
	m_pDemoFile = g_pFileSystem->Open (name, "rb");
	if (!m_pDemoFile)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}
	
	// Read in the header
	g_pFileSystem->Read( &header, sizeof(demoheader_t), m_pDemoFile );

	if ( strcmp ( header.demofilestamp, "HLDEMO" ) )
	{
		Con_Printf( "%s is not a demo file\n", name);
		g_pFileSystem->Close(m_pDemoFile);
		m_pDemoFile = NULL;
		return;
	}

	Con_Printf ("Demo file protocols Network(%i), Demo(%i)\nServer protocols at Network(%i), Demo(%i)\n",
		header.networkprotocol, 
		header.demoprotocol,
		PROTOCOL_VERSION,
		DEMO_PROTOCOL
	);

	Con_Printf("Map name :  %s\n", header.mapname);
	Con_Printf("Game .dll:  %s\n", header.gamedirectory);

	// Now read in the directory structure.
	g_pFileSystem->Seek( m_pDemoFile, header.directory_offset, FILESYSTEM_SEEK_HEAD );

	g_pFileSystem->Read( &directory.numentries, sizeof(int), m_pDemoFile );

	if (directory.numentries < 1 ||
		directory.numentries > 1024 )
	{
		Con_Printf ("CDemo::List: demo had bogus # of directory entries:  %i\n",
			directory.numentries);
		g_pFileSystem->Close(m_pDemoFile);
		m_pDemoFile = NULL;
		return;
	}

	demoentry_t entry;
	for ( int i = 0; i <  directory.numentries; i++ )
	{
		memset( &entry, 0, sizeof( entry ) );
		g_pFileSystem->Read( &entry, sizeof( entry ), m_pDemoFile );

		directory.entries.AddToTail( entry );
	}

	Con_Printf("\n");

	for (nCurrent = 0; nCurrent < directory.numentries; nCurrent++)
	{
		pentry = &directory.entries[nCurrent];
		
		if (pentry->entrytype == DEMO_STARTUP)
			sprintf(type, "Start segment");
		else
			sprintf(type, "Normal segment");

		Con_Printf("%i:  %s\nTime:  %.2f s.\nFrames:  %i\nSize:  %.2fK\n",
			nCurrent+1, type, pentry->playback_time, pentry->playback_frames, ((float)pentry->length/1024.0));
	}

	g_pFileSystem->Close(m_pDemoFile);
	m_pDemoFile = NULL;

	directory.entries.Purge();
	directory.numentries    = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::MoveToNextSection( void )
{
	if ( ++m_nEntryIndex >= m_DemoDirectory.numentries )
	{
		// Done
		Host_EndGame("End of demo.\n");
		return false;
	}

	// Switch to next section, we got a dem_stop
	m_pEntry = &m_DemoDirectory.entries[m_nEntryIndex];
	
	// Ready to continue reading, reset clock.
	g_pFileSystem->Seek( m_pDemoFile, m_pEntry->offset, FILESYSTEM_SEEK_HEAD ); 

	// Time is now relative to this chunk's clock.
	m_flStartTime = GetPlaybackClock();
	m_nStartTick = host_tickcount;
	m_nFrameCount = 0;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Save state of cls.netchan sequences so that
// we can play the demo correctly.
// Input  : *fp - 
//-----------------------------------------------------------------------------
void CDemo::WriteSequenceInfo ( FileHandle_t fp )
{
	g_pFileSystem->Write( &cls.netchan.incoming_sequence, sizeof(int), fp );
	g_pFileSystem->Write( &cls.netchan.incoming_acknowledged, sizeof(int), fp );
	g_pFileSystem->Write( &cls.netchan.incoming_reliable_acknowledged, sizeof(int), fp );
	g_pFileSystem->Write( &cls.netchan.incoming_reliable_sequence, sizeof(int), fp );
	g_pFileSystem->Write( &cls.netchan.outgoing_sequence, sizeof(int), fp );
	g_pFileSystem->Write( &cls.netchan.reliable_sequence, sizeof(int), fp );
	g_pFileSystem->Write( &cls.netchan.last_reliable_sequence, sizeof(int), fp );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::ReadSequenceInfo( FileHandle_t demofile, bool discard )
{
	int incoming_sequence;
	int incoming_acknowledged;
	int incoming_reliable_acknowledged;
	int incoming_reliable_sequence;
	int outgoing_sequence;
	int reliable_sequence;
	int last_reliable_sequence;

	g_pFileSystem->Read( &incoming_sequence, sizeof(int), demofile );
	g_pFileSystem->Read( &incoming_acknowledged, sizeof(int), demofile );
	g_pFileSystem->Read( &incoming_reliable_acknowledged, sizeof(int), demofile );
	g_pFileSystem->Read( &incoming_reliable_sequence, sizeof(int), demofile );
	g_pFileSystem->Read( &outgoing_sequence, sizeof(int), demofile );
	g_pFileSystem->Read( &reliable_sequence, sizeof(int), demofile );
	g_pFileSystem->Read( &last_reliable_sequence, sizeof(int), demofile );

	if ( discard )
		return;

	cls.netchan.incoming_sequence				= incoming_sequence;
	cls.netchan.incoming_acknowledged			=  incoming_acknowledged;
	cls.netchan.incoming_reliable_acknowledged	=  incoming_reliable_acknowledged;
	cls.netchan.incoming_reliable_sequence		=  incoming_reliable_sequence;
	cls.netchan.outgoing_sequence				= outgoing_sequence;
	cls.netchan.reliable_sequence				= reliable_sequence;
	cls.netchan.last_reliable_sequence			= last_reliable_sequence;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : info - 
//-----------------------------------------------------------------------------
void CDemo::WriteGetCmdInfo( democmdinfo_t& info )
{
	info.flags		= FDEMO_NORMAL;

	g_pClientSidePrediction->GetViewOrigin( info.viewOrigin );
#ifndef SWDS
	info.viewAngles = cl.viewangles;
#endif
	g_pClientSidePrediction->GetLocalViewAngles( info.localViewAngles );

	// Nothing by default
	info.viewOrigin2.Init();
	info.viewAngles2.Init();
	info.localViewAngles2.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fp - 
//-----------------------------------------------------------------------------
void CDemo::WriteCmdInfo( FileHandle_t fp, democmdinfo_t& info )
{
	g_pFileSystem->Write( &info, sizeof( democmdinfo_t ), fp );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//			dt - 
//			frame - 
//-----------------------------------------------------------------------------
void CDemo::ReadCmdHeader( FileHandle_t demofile, byte& cmd, float& dt, int& dframe )
{
	int r;
	// Read the command
	r = g_pFileSystem->Read ( &cmd, sizeof(byte), demofile );
	Assert( r == sizeof(byte) );
	Assert( cmd >= 1 && cmd <= dem_lastcmd );

	// Read the timestamp
	r = g_pFileSystem->Read ( &dt, sizeof(float), demofile );
	Assert( r == sizeof(float) );
	dt = LittleFloat( dt );

	// Read the framestamp
	r = g_pFileSystem->Read( &dframe, sizeof( int ), demofile );
	Assert( r == sizeof( int ) );
	dframe = LittleLong( dframe );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::ReadCmdInfo( FileHandle_t demofile, democmdinfo_t& info )
{
	g_pFileSystem->Read( &info, sizeof( democmdinfo_t ), demofile );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cmdname - 
//-----------------------------------------------------------------------------
void CDemo::RecordCommand( const char *cmdstring )
{
	if ( !demo->IsRecording() )
	{
		return;
	}

	if ( !demo_recordcommands.GetInt() )
		return;

	// Don't record console toggling and don't record the record command itself!
	if ( Q_strstr( cmdstring, "toggleconsole" ) ||
		 Q_strstr( cmdstring, "record " ) ||
		 Q_strstr( cmdstring, "incrementvar" ) ||
		 Q_strstr( cmdstring, "stopdemo" ) )
	{
		return;
	}

	WriteCmdHeader( dem_clientdll, m_pDemoFile );

	int len = strlen( cmdstring ) + 1;
	
	Assert( len < 2048 );

	g_pFileSystem->Write( &len, sizeof( int ), m_pDemoFile );
	g_pFileSystem->Write ( cmdstring, len, m_pDemoFile );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::StartupDemoHeader( void )
{
	if ( m_pDemoFileHeader )
	{
		g_pFileSystem->Close(m_pDemoFileHeader);
	}

	// Note: this is replacing tmpfile()
	m_pDemoFileHeader = g_pFileSystem->Open( "hl2.tmp", "w+b" );
	if ( !m_pDemoFileHeader )
	{
		Con_DPrintf ("ERROR: couldn't open temporary header file.\n");
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			*msg - 
//-----------------------------------------------------------------------------
void CDemo::WriteMessage( bool startup, int start, sizebuf_t *msg )
{
	byte c;
	int len, swlen;
	long curpos;
	FileHandle_t file = startup ? m_pDemoFileHeader : m_pDemoFile;

	if ( !file )
	{
		return;
	}

	// Past the start but not recording a demo.
	if ( !startup && !IsRecording() )
	{
		return;
	}

	len = msg->cursize - start;
	swlen = LittleLong(len);

	if (len <= 0)
		return;

	if ( !startup )
	{
		m_nFrameCount++;
	}

	curpos = g_pFileSystem->Tell( file );

	// Demo playback should read this as an incoming message.
	c = startup ? dem_norewind : dem_read;

	WriteCmdHeader( c, file );
	{
		democmdinfo_t info;
		// Snag current info
		WriteGetCmdInfo( info );
		// Store it
		WriteCmdInfo( file, info );
	}

	WriteSequenceInfo ( file );

	// Write the length out.
	g_pFileSystem->Write ( &swlen, sizeof(int), file );

	// Output the buffer.  Skip the network packet stuff.
	g_pFileSystem->Write ( msg->data + start, len, file );

	if ( demo_debug.GetInt() >= 1 )
	{
		Msg( "Writing demo message %i bytes, starting with %i at offset %i\n",
			len, (int) *(byte *)( msg->data + start ), start );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a demo file runs out, or the user starts a game
// Output : void CDemo::StopPlayback
//-----------------------------------------------------------------------------
void CDemo::StopPlayback( void )
{
	if ( !IsPlayingBack() )
		return;

	demoaction->StopPlaying();

	g_pFileSystem->Close (m_pDemoFile);
	m_bPlayingBack = false;
	m_bPlaybackPaused = false;
	m_flAutoResumeTime = 0.0f;
	m_pDemoFile = NULL;
	m_nFrameCount = 0;

	if ( demo_debug.GetInt() >= 1 )
	{
		cl_showmessages.SetValue( m_nStartShowMessages );
	}

	m_DemoDirectory.entries.Purge();
	m_DemoDirectory.numentries    = 0;

	FinishTimeDemo();

	m_pEntry = NULL;
	m_bAutoPause = false;
	m_bFastForwarding = false;
	m_nAutoPauseFrame = -1;
	m_flPlaybackRateModifier = 1.0f;

	src_demo_override_fov = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Rewind from the current spot by the time stamp, byte code and frame counter offsets
//-----------------------------------------------------------------------------
void CDemo::RewindMessageHeader( int oldposition )
{
	Assert( m_pDemoFile );
	g_pFileSystem->Seek( m_pDemoFile, oldposition, FILESYSTEM_SEEK_HEAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::ReadRawNetworkData( FileHandle_t demofile, int framecount, sizebuf_t& msg )
{
	int		msglen = 0;	
	byte	msgbuf[NET_MAX_MESSAGE];

	msg.cursize = 0;

	// get the next message
	int r;
	
	r = g_pFileSystem->Read (&msglen, sizeof(int), demofile);
	if ( r != sizeof(int) )
	{
		Host_EndGame("Bad demo length.\n");
		return false;
	}
	
	msglen = LittleLong (msglen);
	
	if (msglen < 0)
	{
		Host_EndGame("Demo message length < 0.\n");
		return false;
	}
	
	if ( msglen < 8 )
	{
		Con_DPrintf("read runt demo message\n");
	}

	if ( msglen > NET_MAX_MESSAGE )
	{
		Host_EndGame("Demo message %i > NET_MAX_MESSAGE ", msglen );
		return false;
	}

	if ( msglen > 0 )
	{
		r = g_pFileSystem->Read (msgbuf, msglen, demofile);
		if ( r != msglen )
		{
			Host_EndGame("Error reading demo message data.\n");
			return false;
		}
	}

	// Simulate receipt in the message queue
	memcpy(msg.data, msgbuf, msglen);
	msg.cursize = msglen;

	if ( demo_debug.GetInt() >= 1 )
	{
		Msg( "Demo message %i %i bytes\n", framecount, msglen );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Read in next demo message and send to local client over network channel, if it's time.
// Output : qboolean 
//-----------------------------------------------------------------------------
bool CDemo::ParseAheadForInterval( float interval, CUtlVector< DemoCommandQueue >& list )
{
	float		f;
	int			dframe;
	byte		cmd;
	long		curpos;
	float		fElapsedTime = 0.0f;
	int			nElapsedTicks;

	democmdinfo_t	nextinfo;

	byte dummymessage_buffer[ NET_MAX_MESSAGE ];
	sizebuf_t dummymessage;

	dummymessage.maxsize = sizeof(dummymessage_buffer);
	dummymessage.data = dummymessage_buffer;

	list.RemoveAll();

	if ( m_bTimeDemo )
		return false;

	long		starting_position = g_pFileSystem->Tell( m_pDemoFile );

	while ( 1 )
	{
		bool swallowmessages = true;
		do
		{
			curpos = g_pFileSystem->Tell(m_pDemoFile);

			ReadCmdHeader( m_pDemoFile, cmd, f, dframe );

			fElapsedTime	= GetPlaybackClock() - m_flStartTime;
			nElapsedTicks	= host_tickcount - m_nStartTick;

			// COMMAND HANDLERS
			switch ( cmd )
			{
			case dem_jumptime:
			case dem_stop:
				{
					g_pFileSystem->Seek( m_pDemoFile, starting_position, FILESYSTEM_SEEK_HEAD );
					return false;
				}
				break;
			case dem_clientdll:
				{
					int length;
					g_pFileSystem->Read( &length, sizeof( int ), m_pDemoFile );
					Assert( length >= 1 && length <= 2048 );

					char szCmdName[ 2048 ];
					g_pFileSystem->Read ( szCmdName, length, m_pDemoFile );
				}
				break;
			case dem_stringtables:
				{
					int length;
					g_pFileSystem->Read( &length, sizeof( int ), m_pDemoFile );
					int curpos = g_pFileSystem->Tell( m_pDemoFile );
					// Skip ahead
					g_pFileSystem->Seek( m_pDemoFile, curpos + length, FILESYSTEM_SEEK_HEAD );
				}
				break;
			case dem_usercmd:
				{
					ReadUserCmd( m_pDemoFile, true );
				}
				break;
			default:
				{
					swallowmessages = false;
				}
				break;
			}
		}
		while ( swallowmessages );

		ReadCmdInfo( m_pDemoFile, nextinfo );

				int			dframe = 0;

		ReadSequenceInfo( m_pDemoFile, true );
		ReadRawNetworkData( m_pDemoFile, dframe, dummymessage );

		DemoCommandQueue entry;
		entry.info = nextinfo;
		entry.t = f;

		list.AddToTail( entry );

		if ( ( f - fElapsedTime ) > interval )
			break;
	}

	g_pFileSystem->Seek( m_pDemoFile, starting_position, FILESYSTEM_SEEK_HEAD );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::ReadMessage( void )
{
	bool bret = _ReadMessage();
	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: Read in next demo message and send to local client over network channel, if it's time.
// Output : qboolean 
//-----------------------------------------------------------------------------
bool CDemo::_ReadMessage( void )
{
	float		f = 0.0f;
	int			dframe;
	byte		cmd;
	long		curpos = 0;
	float		fElapsedTime = 0.0f;
	int			nElapsedTicks;

	static int timedemo_currentframe = 0;

	Assert( m_pDemoFile );
	if ( !m_pDemoFile )
	{
		m_bPlayingBack = false;
		Host_EndGame( "Tried to read a demo message with no demo file\n" );
		return false;
	}

	// If game is still shutting down, then don't read any demo messages from file quite yet
	if ( HostState_IsGameShuttingDown() )
	{
		return false;
	}

	Assert( IsPlayingBack() );
	// External editor has paused playback
	if ( IsPlaybackPaused() )
	{
		// cl.gettime() and host_time are frozen by calls back to demo->IsPlaybackPaused()
		if ( m_flAutoResumeTime != 0.0f &&
			( Sys_FloatTime() >= m_flAutoResumeTime ) )
		{
			ResumePlayback();
		}
		else
		{
			return false;
		}
	}

	bool swallowmessages = true;
	bool forceskip = false;
	do
	{
		if ( !m_pDemoFile )
			break;

		curpos = g_pFileSystem->Tell(m_pDemoFile);

		ReadCmdHeader( m_pDemoFile, cmd, f, dframe );

		fElapsedTime	= GetPlaybackClock() - m_flStartTime;
		nElapsedTicks	= host_tickcount - m_nStartTick;

		bool bSkipMessage = false;

		// Time demo ignores clocks and tries to synchronize frames to what was recorded
		//  I.e., while frame is the same, read messages, otherwise, skip out.
		if ( !m_bTimeDemo )
		{
			bSkipMessage = (f >= fElapsedTime) ? true : false;
		}
		else
		{
			bSkipMessage = false;
		}

		if ( cmd == dem_read &&
			ShouldSkip( f, nElapsedTicks ) )
		{
			bSkipMessage = false;
			forceskip = true;
		}

		if (cmd != dem_norewind &&   // Startup messages don't require looking at the timer.
			cmd != dem_stop &&
			bSkipMessage)   // Not ready for a message yet, put it back on the file.
		{
			// Never skip first message
			if ( m_nFrameCount != 0 )
			{
				demoaction->Update( false, m_nFrameCount, fElapsedTime );

				RewindMessageHeader( curpos );
				return false;   // Not time yet.
			}
		}

		// COMMAND HANDLERS
		switch ( cmd )
		{
		case dem_jumptime:
			{
				m_flStartTime = GetPlaybackClock();
				m_nStartTick = host_tickcount;
			}
			break;
		case dem_stop:
			{
				MoveToNextSection();
			}
			break;
		case dem_clientdll:
			{
				int length;
				g_pFileSystem->Read( &length, sizeof( int ), m_pDemoFile );
				Assert( length >= 1 && length <= 2048 );

				char szCmdName[ 2048 ];
				g_pFileSystem->Read ( szCmdName, length, m_pDemoFile );
				
				Cbuf_AddText(szCmdName);
				Cbuf_AddText("\n");
				Cbuf_Execute();
			}
			break;
		case dem_stringtables:
			{
				int length;
				g_pFileSystem->Read( &length, sizeof( int ), m_pDemoFile );
				int curpos = g_pFileSystem->Tell( m_pDemoFile );
				ReadStringTables( length, m_pDemoFile );
				int bytesRead = g_pFileSystem->Tell( m_pDemoFile ) - curpos;
				if ( bytesRead != length )
				{
					Host_Error( "Error reading string table data lump!!! read %i expecting %i bytes\n",
						bytesRead, length );
				}
			}
			break;
		case dem_usercmd:
			{
				ReadUserCmd( m_pDemoFile, false );
			}
			break;
		default:
			{
				swallowmessages = false;

				if ( forceskip )
				{
					float dfelapsed = clamp( f - m_flLastFrameElapsed, 0.0f, 1.0f );
					host_time += dfelapsed;
				}
			}
			break;
		}
	}
	while ( swallowmessages );

	if ( !m_pDemoFile )
		return false;

	// If we are playing back a timedemo, and we've already passed on a 
	//  frame update for this host_frame tag, then we'll just skip this message.
	if ( m_bTimeDemo && 
		( timedemo_currentframe == host_framecount ) &&
		!forceskip )
	{
		RewindMessageHeader( curpos );
		return false;
	}
	
	timedemo_currentframe = host_framecount;

	// If not on "LOADING" section, check a few things
	if ( m_nEntryIndex )
	{
		// Auto pause after loading and skipping LOADING section?
		if ( !m_nFrameCount && m_bAutoPause )  
		{
			PausePlayback();
			m_bAutoPause = false;
		}

		// We are now on the second frame of a new section, if so, reset start time (unless
		//  in a timedemo
		if ( m_nFrameCount == 1 &&
			!m_bTimeDemo )
		{
			// Cheat by moving the relative start time forward.
			m_flStartTime = GetPlaybackClock();
		}	

		demoaction->Update( true, m_nFrameCount, fElapsedTime );
	}

	// m_flTimeDemoStartTime will be grabbed at the second frame of the demo, so
	// all the loading time doesn't get counted

	m_nFrameCount++;

	m_flLastFrameElapsed = f;
	ReadCmdInfo( m_pDemoFile, m_LastCmdInfo );
	ReadSequenceInfo( m_pDemoFile, false );

	bool bret = ReadRawNetworkData( m_pDemoFile, m_nFrameCount, net_message );

	// Try and jump ahead one frame
	m_bInterpolateView = ParseAheadForInterval( MAX_FRAMETIME, m_DestCmdInfo );

	// Con_Printf( "Reading message for %i : %f skip %i\n", m_nFrameCount, fElapsedTime, forceskip ? 1 : 0 );

	if ( m_nEntryIndex && 
		m_nAutoPauseFrame != -1 && 
		m_nFrameCount == m_nAutoPauseFrame )
	{
		m_nAutoPauseFrame = -1;
		PausePlayback();
	}

	// Skip a few frames before doing any timing
	if ( m_bTimeDemo && (m_nFrameCount == m_nFrameStartedRendering + 60) )
	{
		m_nTimeDemoStartFrame = host_framecount;
		m_bTimeDemoTimerStarted = true;
		m_flTimeDemoStartTime = realtime;
		m_flTotalFPSVariability = 0.0f;
		g_EngineStats.BeginRun();
	}

	return bret;
}

static ConVar demo_interpolateview( "demo_interpolateview", "1", 0, "Do view interpolation during dem playback." );

void CDemo::InterpolateDemoCommand( CUtlVector< DemoCommandQueue >& list, float t, float& frac, democmdinfo_t& prev, democmdinfo_t& next )
{
	int c = list.Count();

	prev.Reset();
	next.Reset();

	DemoCommandQueue *pentry = NULL;
	for ( int i = 0; i < c; i++ )
	{
		DemoCommandQueue *entry = &list[ i ];

		next = entry->info;

		// Time is earlier
		if ( t < entry->t )
		{
			if ( i == 0 )
			{
				prev = next;
				frac = 0.0f;
				return;
			}

			// Avoid div by zero
			if ( entry->t == pentry->t )
			{
				frac = 0.0f;
				return;
			}

			// Time spans the two entries
			frac = ( t - pentry->t ) / ( entry->t - pentry->t );
			frac = clamp( frac, 0.0f, 1.0f );
			return;
		}

		prev = next;
		pentry = entry;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::GetInterpolatedViewpoint( void )
{
	democmdinfo_t outinfo;
	outinfo.Reset();

	if ( !demo->IsPlayingBack() )
		return;

	if ( m_bInterpolateView && !m_Skip.active && demo_interpolateview.GetBool() )
	{
		democmdinfo_t last, next;
		float frac = 0.0f;

		// Add last to dest list
		DemoCommandQueue entry;
		entry.t = m_flLastFrameElapsed;
		entry.info = m_LastCmdInfo;

		m_DestCmdInfo.AddToHead( entry );

		// Determine current time slice
		
		float elapsed = GetPlaybackClock() - m_flStartTime;

		InterpolateDemoCommand( m_DestCmdInfo, elapsed, frac, last, next );

		// Pull entry from head
		m_DestCmdInfo.Remove( 0 );
		
		// Now interpolate
		Vector delta;

		Vector startorigin = last.GetViewOrigin();
		Vector destorigin = next.GetViewOrigin();

		VectorSubtract( destorigin, startorigin, delta );
		float lenSquared = DotProduct( delta, delta );
		if( lenSquared > ( DEMO_INTERP_LIMIT * DEMO_INTERP_LIMIT ) )
		{
			frac = 1.0f;
		}

		outinfo.viewOrigin = startorigin + frac * ( destorigin - startorigin );

		Quaternion src, dest;
		Quaternion result;

		AngleQuaternion( last.GetViewAngles(), src );
		AngleQuaternion( next.GetViewAngles(), dest );
		QuaternionSlerp( src, dest, frac, result );

		QuaternionAngles( result, outinfo.viewAngles );

		AngleQuaternion( last.GetLocalViewAngles(), src );
		AngleQuaternion( next.GetLocalViewAngles(), dest );
		QuaternionSlerp( src, dest, frac, result );

		QuaternionAngles( result, outinfo.localViewAngles );

		g_pClientSidePrediction->DemoReportInterpolatedPlayerOrigin( outinfo.GetViewOrigin() );
	}
	else
	{
		outinfo.viewOrigin = m_LastCmdInfo.GetViewOrigin();
		outinfo.viewAngles = m_LastCmdInfo.GetViewAngles();
		outinfo.localViewAngles = m_LastCmdInfo.GetLocalViewAngles();
	}

	g_pClientSidePrediction->SetViewOrigin( outinfo.viewOrigin );
	g_pClientSidePrediction->SetViewAngles( outinfo.viewAngles );
	g_pClientSidePrediction->SetLocalViewAngles( outinfo.localViewAngles );
#ifndef SWDS
	VectorCopy( outinfo.viewAngles, cl.viewangles );
#endif

	OverrideViewSetup( outinfo );
}

//-----------------------------------------------------------------------------
// Purpose: stop recording a demo
//-----------------------------------------------------------------------------
void CDemo::StopRecording( void )
{
	int curpos;
	int i;

	if ( !IsRecording() )
	{
		return;
	}

	ClearAutoRecord();

	// Demo playback should read this as an incoming message.
	WriteCmdHeader( dem_stop, m_pDemoFile );

	float stoptime = GetRecordingClock();

	// Close down the hud for now.
	g_ClientDLL->HudReset();
	
	curpos = g_pFileSystem->Tell(m_pDemoFile);
	m_pEntry->length = curpos - m_pEntry->offset;
	m_pEntry->playback_time = stoptime - m_flStartTime;
	
	m_pEntry->playback_frames = m_nFrameCount;

	//  Now write out the directory and free it and touch up the demo header.
	g_pFileSystem->Write( &m_DemoDirectory.numentries, sizeof(int), m_pDemoFile );
	for (i = 0; i < m_DemoDirectory.numentries; i++)
	{
		g_pFileSystem->Write( &m_DemoDirectory.entries[i], sizeof(demoentry_t), m_pDemoFile );
	}

	m_DemoDirectory.entries.Purge();
	m_DemoDirectory.numentries    = 0;

	m_DemoHeader.directory_offset = curpos;
	g_pFileSystem->Seek( m_pDemoFile, 0, FILESYSTEM_SEEK_HEAD );
	g_pFileSystem->Write( &m_DemoHeader, sizeof(demoheader_t), m_pDemoFile );
	
	g_pFileSystem->Close (m_pDemoFile);
	m_pDemoFile = NULL;
	m_bRecording = false;
	
	m_nTimeDemoLastFrame = host_framecount;

	Con_Printf ("Completed demo\n");
	Con_DPrintf("Recording time %.2f\nHost frames %i\n", stoptime - m_flStartTime, m_nTimeDemoLastFrame - m_nTimeDemoStartFrame);

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			track - 
//-----------------------------------------------------------------------------
void CDemo::Record( const char *name )
{
	char    mapname[MAX_OSPATH];
	char    szDll[MAX_OSPATH];
	long    copysize;
	long    savepos;
	long    curpos;

	//
	// open the demo file
	//
	Con_Printf ("recording to %s.\n", name);
	m_pDemoFile = g_pFileSystem->Open (name, "wb");
	if (!m_pDemoFile)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}

	memset(&m_DemoHeader, 0, sizeof(demoheader_t));
	strcpy(m_DemoHeader.demofilestamp, "HLDEMO");

	COM_FileBase( modelloader->GetName( host_state.worldmodel ), mapname );

	strcpy(m_DemoHeader.mapname  , mapname );

	strcpy(szDll, com_gamedir);
	COM_FileBase ( szDll, m_DemoHeader.gamedirectory );

	m_DemoHeader.demoprotocol			= DEMO_PROTOCOL;
	m_DemoHeader.networkprotocol		= PROTOCOL_VERSION;
	m_DemoHeader.directory_offset		= 0;   // Temporary.  We rewrite the structure at the end.
	
	// Demos must match current server/client protocol.
	g_pFileSystem->Write( &m_DemoHeader, sizeof(demoheader_t), m_pDemoFile);

	m_DemoDirectory.numentries = 2;

	demoentry_t entry;
	for ( int i = 0; i < m_DemoDirectory.numentries; i++ )
	{
		memset( &entry, 0, sizeof( entry ) );
		m_DemoDirectory.entries.AddToTail( entry );
	}

	// DIRECTORY ENTRY # 0
	m_pEntry = &m_DemoDirectory.entries[0]; // Only one here.
	m_pEntry->entrytype       = DEMO_STARTUP;
	m_pEntry->playback_time       = 0.0f; // Startup takes 0 time.
	m_pEntry->offset          = g_pFileSystem->Tell(m_pDemoFile);  // Position for this chunk.

	// don't start saving messages until a non-delta compressed message is received
	m_bRecordWaiting	= true;
	m_bRecording		= true;

	curpos = g_pFileSystem->Tell(m_pDemoFile);

	// Finish off the startup info.
	WriteCmdHeader( dem_stop, m_pDemoFileHeader );

	g_pFileSystem->Flush( m_pDemoFileHeader );

	// Now copy the stuff we cached from the server.
	copysize = g_pFileSystem->Tell(m_pDemoFileHeader);
	savepos = copysize;

	g_pFileSystem->Seek(m_pDemoFileHeader, 0, FILESYSTEM_SEEK_HEAD);

	COM_CopyFileChunk ( m_pDemoFile, m_pDemoFileHeader, copysize );

	// Jump back to end, in case we record another demo for this session.
	g_pFileSystem->Seek(m_pDemoFileHeader, savepos, FILESYSTEM_SEEK_HEAD);

	m_flStartTime = GetRecordingClock();
	m_nFrameCount = 0;
	m_nStartTick = host_tickcount;

	// Now move on to entry # 2, the first data chunk.
	curpos = g_pFileSystem->Tell(m_pDemoFile);

	m_pEntry->length = curpos - m_pEntry->offset;

// DIRECTORY ENTRY # 1

	// Now we are writing the first real lump.
	m_pEntry = &m_DemoDirectory.entries[1]; // First real data lump
	m_pEntry->entrytype = DEMO_NORMAL;
	m_pEntry->playback_time = 0.0f; // Startup takes 0 time.

	m_pEntry->offset = g_pFileSystem->Tell(m_pDemoFile);
	// Demo playback should read this as an incoming message.
	// Write the client's realtime value out so we can synchronize the reads.
	WriteCmdHeader( dem_jumptime, m_pDemoFile );

	m_nTimeDemoStartFrame = host_framecount;
	m_nTimeDemoLastFrame = -1;		// get a new message this frame

	g_ClientDLL->HudReset();

	Cbuf_InsertText( va( "impulse %i\n", DEMO_RESTART_HUD_IMPULSE ) );
	Cbuf_Execute();

	WriteCmdHeader( dem_stringtables, m_pDemoFile );
	WriteStringTables( m_pDemoFile );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::IsPlayingBack_TimeDemo( void )
{
	return m_bTimeDemo;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::IsPlayingBack( void ) const
{
	return m_bPlayingBack;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::IsTimeDemoTimerStarted( void )
{
	return m_bTimeDemoTimerStarted;
}


 //-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDemo::IsTakingSnapshot()
{
	return IsPlayingBack_TimeDemo() && (m_nFrameCount == m_nSnapshotFrame);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::IsRecording( void )
{
	return m_bRecording;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::IsValidDemoHeader( void )
{
	return m_pDemoFileHeader ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: Start's demo playback
// Input  : *name - 
//-----------------------------------------------------------------------------
void CDemo::Play( const char *name )
{
	// Disconnect from server or stop running one
	Host_Disconnect();

	Con_Printf ("Playing demo from %s.\n", name);
	COM_OpenFile (name, &m_pDemoFile);
	if (!m_pDemoFile)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		cls.demonum = -1;		// stop demo loop
		return;
	}
	
	// Read in the m_DemoHeader
	g_pFileSystem->Read( &m_DemoHeader, sizeof(demoheader_t), m_pDemoFile );

	if ( strcmp ( m_DemoHeader.demofilestamp, "HLDEMO" ) )
	{
		Con_Printf( "%s is not a demo file\n", name);
		g_pFileSystem->Close(m_pDemoFile);
		m_pDemoFile = NULL;
		cls.demonum = -1;
		return;
	}

	if ( m_DemoHeader.networkprotocol != PROTOCOL_VERSION ||
		 m_DemoHeader.demoprotocol       != DEMO_PROTOCOL )
	{
		Con_Printf (
			"ERROR: demo protocol outdated\n"
			"Demo file protocols Network(%i), Demo(%i)\n"
			"Server protocol is at Network(%i), Demo(%i)\n",
			m_DemoHeader.networkprotocol, 
			m_DemoHeader.demoprotocol,
			PROTOCOL_VERSION,
			DEMO_PROTOCOL
		);
		g_pFileSystem->Close(m_pDemoFile);
		m_pDemoFile = NULL;
		cls.demonum = -1;		// stop demo loop
		return;
	}

	// Now read in the directory structure.
	g_pFileSystem->Seek( m_pDemoFile, m_DemoHeader.directory_offset, FILESYSTEM_SEEK_HEAD );
	g_pFileSystem->Read( &m_DemoDirectory.numentries, sizeof(int), m_pDemoFile );

	if (m_DemoDirectory.numentries < 1 ||
		m_DemoDirectory.numentries > 1024 )
	{
		Con_Printf ("CDemo::Play: demo had bogus # of directory entries:  %i\n",
			m_DemoDirectory.numentries);
		g_pFileSystem->Close(m_pDemoFile);
		m_pDemoFile = NULL;
		cls.demonum = -1;		// stop demo loop
		Host_Disconnect();
		return;
	}


	demoentry_t entry;
	for ( int i = 0; i <  m_DemoDirectory.numentries; i++ )
	{
		memset( &entry, 0, sizeof( entry ) );
		g_pFileSystem->Read( &entry, sizeof( entry ), m_pDemoFile );

		m_DemoDirectory.entries.AddToTail( entry );
	}

	m_nFrameStartedRendering = -9999;
	m_nEntryIndex = 0;
	m_pEntry = &m_DemoDirectory.entries[m_nEntryIndex];

	g_pFileSystem->Seek( m_pDemoFile, m_pEntry->offset, FILESYSTEM_SEEK_HEAD );

	m_bPlayingBack = true;
	cls.state = ca_connected;

	m_flStartTime = GetPlaybackClock();  // For determining whether to read another message
	m_nStartTick = host_tickcount;

	Netchan_Setup( NS_CLIENT, &cls.netchan, net_from );

	m_nFrameCount = 0;

	cls.lastoutgoingcommand = -1;
 	cls.nextcmdtime = realtime;

	strcpy( m_szDemoFileName, name );

	m_nStartShowMessages = cl_showmessages.GetInt();
	if ( demo_debug.GetInt() >= 1 )
	{
		cl_showmessages.SetValue( 1 );
	}

	demoaction->StartPlaying( m_szDemoFileName );

	m_bAutoPause = false;
	m_bFastForwarding = false;
	m_nAutoPauseFrame = -1;
	m_flPlaybackRateModifier = 1.0f;

	src_demo_override_fov = 0.0f;
}

static const char *DXLevelToString( int dxlevel )
{
	bool bHalfPrecision = false;

	const char *pShaderDLLName = g_pMaterialSystemHardwareConfig->GetShaderDLLName();
	if( pShaderDLLName && Q_stristr( pShaderDLLName, "nvfx" ) )
	{
		bHalfPrecision = true;
	}
	
	if( CommandLine()->CheckParm( "-dxlevel" ) )
	{
		switch( dxlevel )
		{
		case 0:
			return "default";
		case 60:
			return "6.0";
		case 70:
			return "7.0";
		case 80:
			return "8.0";
		case 81:
			return "8.1";
		case 82:
			if( bHalfPrecision )
			{
				return "8.1 with some 9.0 (half-precision)";
			}
			else
			{
				return "8.1 with some 9.0 (full-precision)";
			}
		case 90:
			if( bHalfPrecision )
			{
				return "9.0 (half-precision)";
			}
			else
			{
				return "9.0 (full-precision)";
			}
		default:
			return "UNKNOWN";
		}
	}
	else
	{
		switch( dxlevel )
		{
		case 60:
			return "gamemode - 6.0";
		case 70:
			return "gamemode - 7.0";
		case 80:
			return "gamemode - 8.0";
		case 81:
			return "gamemode - 8.1";
		case 82:
			if( bHalfPrecision )
			{
				return "gamemode - 8.1 with some 9.0 (half-precision)";
			}
			else
			{
				return "gamemode - 8.1 with some 9.0 (full-precision)";
			}
		case 90:
			if( bHalfPrecision )
			{
				return "gamemode - 9.0 (half-precision)";
			}
			else
			{
				return "gamemode - 9.0 (full-precision)";
			}
		default:
			return "gamemode";
		}
	}
}

void CDemo::WriteTimeDemoResults( void )
{
	int		frames;
	float	time;
	frames = (host_framecount - m_nTimeDemoStartFrame) - 1;
	time = Sys_FloatTime() - m_flTimeDemoStartTime;
	if (!time)
	{
		time = 1;
	}
	float flVariability = (m_flTotalFPSVariability / (float)frames);
	Con_Printf ("%i frames %5.3f seconds %5.2f fps %5.3f fps variability\n", frames, time, frames/time, flVariability );
	bool bFileExists = g_pFileSystem->FileExists( "SourceBench.csv" );
	FileHandle_t fileHandle = g_pFileSystem->Open( "SourceBench.csv", "a+" );
	int width, height;
	materialSystemInterface->GetBackBufferDimensions( width, height );
	ImageFormat backBufferFormat = materialSystemInterface->GetBackBufferFormat();
	if( !bFileExists )
	{
		g_pFileSystem->FPrintf( fileHandle, "Half-Life 2 Benchmark Results\n\n" );
		g_pFileSystem->FPrintf( fileHandle, "demofile," );
		g_pFileSystem->FPrintf( fileHandle, "frame data spreadsheet," );
		g_pFileSystem->FPrintf( fileHandle, "frames/sec," );
		g_pFileSystem->FPrintf( fileHandle, "framerate variability," );
		g_pFileSystem->FPrintf( fileHandle, "totaltime (sec)," );
		g_pFileSystem->FPrintf( fileHandle, "width," );
		g_pFileSystem->FPrintf( fileHandle, "height," );
		g_pFileSystem->FPrintf( fileHandle, "numframes," );
		g_pFileSystem->FPrintf( fileHandle, "dxlevel," );
		g_pFileSystem->FPrintf( fileHandle, "backbuffer format," );
		g_pFileSystem->FPrintf( fileHandle, "cmdline," );
		g_pFileSystem->FPrintf( fileHandle, "driver name," );
		g_pFileSystem->FPrintf( fileHandle, "driver desc," );
		g_pFileSystem->FPrintf( fileHandle, "driver version," );
		g_pFileSystem->FPrintf( fileHandle, "vendor id," );
		g_pFileSystem->FPrintf( fileHandle, "device id," );
		g_pFileSystem->FPrintf( fileHandle, "subsys id," );
		g_pFileSystem->FPrintf( fileHandle, "revision," );
		g_pFileSystem->FPrintf( fileHandle, "whqllevel," );
		g_pFileSystem->FPrintf( fileHandle, "forcetrilinear," );
		g_pFileSystem->FPrintf( fileHandle, "forcebilinear," );
		g_pFileSystem->FPrintf( fileHandle, "zfill," );
		g_pFileSystem->FPrintf( fileHandle, "fastclip," );
		g_pFileSystem->FPrintf( fileHandle, "shaderdll," );
		g_pFileSystem->FPrintf( fileHandle, "sound," );
		g_pFileSystem->FPrintf( fileHandle, "vsync," );
		g_pFileSystem->FPrintf( fileHandle, "\n" );
	}
	// HACK HACK HACK!
	extern ConVar mat_trilinear, mat_bilinear, r_fastzreject;
	g_pFileSystem->Seek( fileHandle, 0, FILESYSTEM_SEEK_TAIL );
	Material3DDriverInfo_t info;
	materialSystemInterface->Get3DDriverInfo( info );
	g_pFileSystem->FPrintf( fileHandle, "%s,", m_szDemoFileName );
	g_pFileSystem->FPrintf( fileHandle, "%s,", g_pStatsFile );
	g_pFileSystem->FPrintf( fileHandle, "%5.1f,", frames/time );
	g_pFileSystem->FPrintf( fileHandle, "%5.1f,", flVariability );
	g_pFileSystem->FPrintf( fileHandle, "%5.1f,", time );
	g_pFileSystem->FPrintf( fileHandle, "%i,", width );
	g_pFileSystem->FPrintf( fileHandle, "%i,", height );
	g_pFileSystem->FPrintf( fileHandle, "%i,", frames );
	g_pFileSystem->FPrintf( fileHandle, "%s,", DXLevelToString( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() ) );
	g_pFileSystem->FPrintf( fileHandle, "%s,", ImageLoader::GetName( backBufferFormat ) );
	g_pFileSystem->FPrintf( fileHandle, "%s,", CommandLine()->GetCmdLine() );
	g_pFileSystem->FPrintf( fileHandle, "%s,", info.m_pDriverName );
	g_pFileSystem->FPrintf( fileHandle, "%s,", info.m_pDriverDescription );
	g_pFileSystem->FPrintf( fileHandle, "%s,", info.m_pDriverVersion );
	g_pFileSystem->FPrintf( fileHandle, "0x%x,", info.m_VendorID );
	g_pFileSystem->FPrintf( fileHandle, "0x%x,", info.m_DeviceID );
	g_pFileSystem->FPrintf( fileHandle, "0x%x,", info.m_SubSysID );
	g_pFileSystem->FPrintf( fileHandle, "0x%x,", info.m_Revision );
	g_pFileSystem->FPrintf( fileHandle, "%i,", info.m_WHQLLevel );
	g_pFileSystem->FPrintf( fileHandle, "%s,", mat_trilinear.GetBool() ? "enabled" : "disabled" );
	g_pFileSystem->FPrintf( fileHandle, "%s,", mat_bilinear.GetBool() ? "enabled" : "disabled" );
	g_pFileSystem->FPrintf( fileHandle, "%s,", g_pMaterialSystemHardwareConfig->UseFastZReject() ? "enabled" : "disabled" );
	g_pFileSystem->FPrintf( fileHandle, "%s,", g_pMaterialSystemHardwareConfig->UseFastClipping() ? "enabled" : "disabled" );

	g_pFileSystem->FPrintf( fileHandle, "%s,", g_pMaterialSystemHardwareConfig->GetShaderDLLName() );
	g_pFileSystem->FPrintf( fileHandle, "%s,", CommandLine()->CheckParm( "-nosound" ) ? "disabled" : "enabled" );
	g_pFileSystem->FPrintf( fileHandle, "%s,", CommandLine()->CheckParm( "-mat_novsync" ) ? "disabled" : "enabled" );
	g_pFileSystem->FPrintf( fileHandle, "\n" );
/*
	g_pFileSystem->FPrintf( fileHandle, "%s,%5.1f,%i,%i,%i,%s,%5.1f,", 
		m_szDemoFileName, frames/time, frames, width, height, ImageLoader::GetName( backBufferFormat ), time );
	g_pFileSystem->FPrintf( fileHandle, "%s,", com_cmdline );
	g_pFileSystem->FPrintf( fileHandle, "%s,%s,%s,0x%x,0x%x,0x%x,0x%x,%d,",
		info.m_pDriverName, info.m_pDriverDescription, info.m_pDriverVersion,
		info.m_VendorID, info.m_DeviceID, info.m_SubSysID, info.m_Revision, info.m_WHQLLevel );
	g_pFileSystem->FPrintf( fileHandle, "%d,", g_pMaterialSystemHardwareConfig->GetDXSupportLevel() );
	extern ConVar mat_trilinear, mat_bilinear, r_fastzreject;
	g_pFileSystem->FPrintf( fileHandle, "%s,", mat_trilinear.GetBool() ? "enabled" : "disabled" );
	g_pFileSystem->FPrintf( fileHandle, "%s,", mat_bilinear.GetBool() ? "enabled" : "disabled" );
	g_pFileSystem->FPrintf( fileHandle, "%s,", r_fastzreject.GetBool() ? "enabled" : "disabled" );
	g_pFileSystem->FPrintf( fileHandle, "%s,", CommandLine()->ParmValue( "-shaderdll", "default!!!!" ) );
	g_pFileSystem->FPrintf( fileHandle, "%s,", CommandLine()->CheckParm( "-nosound" ) ? "disabled" : "enabled" );
	g_pFileSystem->FPrintf( fileHandle, "%s,", CommandLine()->CheckParm( "-mat_novsync" ) ? "disabled" : "enabled" );
	g_pFileSystem->FPrintf( fileHandle, "%s,", g_pStatsFile );
*/
	g_pFileSystem->Close( fileHandle );



}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::FinishTimeDemo( void )
{
	if ( !m_bTimeDemo )
		return;

	m_bTimeDemo = false;
	
	m_bTimeDemoTimerStarted = false;
	if( g_EngineStats.UsingLogFile() )
	{
		g_EngineStats.CloseLogFile();
		g_EngineStats.LogAllCountedStats( true );
		g_EngineStats.LogAllTimedStats( true );
		g_EngineStats.LogClientStats( true );
	}
	g_EngineStats.EndRun();
	if( !CommandLine()->FindParm( "-benchframe" ) )
	{
		WriteTimeDemoResults();
	}
	if( m_bQuitAfterTimeDemo )
	{
		Cbuf_AddText( "quit\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CDemo::Play_TimeDemo( const char *name )
{
	// Make sure random #s are the same for timedemo
	SeedRandomNumberGenerator( true );
	Play( name );
	
	// m_flTimeDemoStartTime will be grabbed at the second frame of the demo, so
	// all the loading time doesn't get counted
	
	m_bTimeDemo = true;
	m_nTimeDemoLastFrame = -1;		// get a new message this frame
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flCurTime - 
//-----------------------------------------------------------------------------
void CDemo::MarkFrame( float flCurTime, float flFPSVariability )
{
	if ( m_bTimeDemo )
	{
		m_flTotalFPSVariability += flFPSVariability;
	}

	// We skip the first frame 
	if (m_flGGTime == 0)
	{
		m_flGGTime = flCurTime;
	}
	// if a second has passed, check mins/max, etc
	else if ((flCurTime - m_flGGTime) >= 1)
	{
		// only count if the console is gone
		if (!Con_IsVisible())
		{
			// get the # of frames that have happened in the last second
			int frames = host_framecount - m_nGGFrameCount;

			if (frames > m_nGGMaxFPS)
				m_nGGMaxFPS = frames;

			if (frames < m_nGGMinFPS)
				m_nGGMinFPS = frames;
			
			if (m_nGGSeconds < GG_MAX_FRAMES)
				m_rgGGFPSFrames[m_nGGSeconds++] = frames;
		}
		else
		{
			// reset our starting point to start at the end of the 
			// last frame that the console was visible
			m_flTimeDemoStartTime = realtime;
			m_flTotalFPSVariability = 0.0f;
			m_nTimeDemoStartFrame = host_framecount;
		}

		m_nGGFrameCount = host_framecount;
		m_flGGTime = flCurTime;
	}

}

//-----------------------------------------------------------------------------
// Purpose: List the contents of a demo file.
//-----------------------------------------------------------------------------
void CL_ListDemo_f( void )
{
	if (cmd_source != src_command)
		return;

	if ( demo->IsRecording() || demo->IsPlayingBack() )
	{
		Con_Printf ("Listdemo only available when not running or recording.\n");
		return;
	}

	// Find the file
	char name[MAX_OSPATH];

	sprintf (name, "%s", Cmd_Argv(1));
	
	COM_DefaultExtension( name, ".dem", sizeof( name ) );

	g_Demo.List( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_StopRecording_f( void )
{
	if (cmd_source != src_command)
		return;

	if ( !demo->IsRecording() )
	{
		Con_DPrintf ("Not recording a demo.\n");
		return;
	}

	demo->StopRecording();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_Record_f( void )
{
	if ( Cmd_Argc() != 2 )
	{
		Con_Printf ("record <demoname>\n");
		return;
	}

	if ( demo->IsRecording() )
	{
		Con_Printf ("Already recording.\n");
		return;
	}


	if ( demo->IsPlayingBack() )
	{
		Con_Printf ("Can't record during demo playback.\n");
		return;
	}

	if ( !g_Demo.IsValidDemoHeader() || 
		( cls.state != ca_active ) )
	{
		Con_Printf("You must be in an active map spawned on a machine that allows creation of files in %s\n", com_gamedir );
		return;
	}

	if ( strstr(Cmd_Argv(1), "..") )
	{
		Con_Printf ("record:  relative pathnames are not allowed.\n");
		return;
	}

	char	name[ MAX_OSPATH ];
	sprintf (name, "%s", Cmd_Argv(1) );
	COM_DefaultExtension( name, ".dem", sizeof( name ) );

	// Record it
	g_Demo.Record( name );

#if !defined( SWDS )
	// Set up the initial state of ambients so the playback is not eeirily silent
	Host_RestartAmbientSounds();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_PlayDemo_f( void )
{
	if ( cmd_source != src_command )
	{
		return;
	}

	if ( Cmd_Argc() < 2 )
	{
		Con_Printf ("play <demoname> [startframe] : plays a demo, optionally starting at specified frame\n");
		return;
	}

	int skipframe = 0;
	if ( Cmd_Argc() == 3 )
	{
		skipframe = max( 0, atoi( Cmd_Argv(2) ) );
	}
	//
	// open the demo file
	//
	char name[ MAX_OSPATH ];

	strcpy (name, Cmd_Argv(1));

	COM_DefaultExtension( name, ".dem", sizeof( name ) );

	g_Demo.Play( name );

	if ( demo->IsPlayingBack() && skipframe > 0 )
	{
		g_Demo.SkipToFrame( skipframe );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void TimeDemo( const char *pDemoFile, const char *pStatsFile = NULL, int nSkipFrame = 0 )
{
	// Disconnect from server or stop running one
	Host_Disconnect();

	// open the demo file
	char name[ MAX_OSPATH ];
	strcpy (name, pDemoFile);
	COM_DefaultExtension( name, ".dem", sizeof( name ) );

	if ( pStatsFile )
	{
		int numFrames = g_Demo.GetNumFrames( name );
		if( numFrames > 0 )
		{
			g_EngineStats.OpenLogFile( pStatsFile, numFrames );
			g_EngineStats.LogAllCountedStats( false );
			g_EngineStats.LogAllTimedStats( false );
			g_EngineStats.LogClientStats( false );
			g_EngineStats.LogTimedStat( ENGINE_STATS_FPS, true );
			g_EngineStats.LogTimedStat( ENGINE_STATS_FPS_VARIABILITY, true );
		}
	}

	g_Demo.Play_TimeDemo( name );

	if ( demo->IsPlayingBack() && nSkipFrame > 0 )
	{
		g_Demo.SkipToFrame( nSkipFrame, true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_TimeDemo_f( void )
{
	if ( cmd_source != src_command )
	{
		return;
	}

	if ( Cmd_Argc() < 2 || Cmd_Argc() > 4 )
	{
		Con_Printf ("timedemo <demoname> <optional stats.txt> <optional startframe>: gets demo speeds, starting from optional frame\n");
		return;
	}

	if( Cmd_Argc() >= 3 )
	{
		Q_strncpy( g_pStatsFile, Cmd_Argv( 2 ), sizeof( g_pStatsFile ) );
	}
	else
	{
		Q_strcpy( g_pStatsFile, "UNKNOWN" );
	}
	int skipframe = 0;
	if ( Cmd_Argc() >= 4 )
	{
		skipframe = max( 0, atoi( Cmd_Argv(3) ) );
	}
	TimeDemo( Cmd_Argv(1), g_pStatsFile, skipframe );
}

void CL_TimeDemoQuit_f( void )
{
	CL_TimeDemo_f();
	g_Demo.QuitAfterTimeDemo();
}

void CL_BenchFrame_f( void )
{
	if ( cmd_source != src_command )
	{
		return;
	}

	if ( Cmd_Argc() != 4 )
	{
		Con_Printf ("benchframe <demoname> <frame> <tgafilename>: takes a snapshot of a particular frame in a demo\n");
		return;
	}

	TimeDemo( Cmd_Argv(1) ); 

	int nBenchFrame = max( 0, atoi( Cmd_Argv(2) ) );
	g_Demo.SnapshotAfterFrame( nBenchFrame );
	g_Demo.SetSnapshotFilename( Cmd_Argv(3) );
	g_Demo.QuitAfterTimeDemo();
}

void CL_DemoSkip_f( void )
{
	if ( cmd_source != src_command )
	{
		return;
	}

	if ( Cmd_Argc() < 2  )
	{
		Con_Printf ( "demoskip <frame> <optional: pause>:  during demo playback, skip to this frame\n");
		return;
	}

	if ( !demo->IsPlayingBack() )
	{
		Con_Printf( "demoskip only valid during playback\n" );
		return;
	}

	bool pause = false;
	if ( Cmd_Argc() == 3 && !Q_strcasecmp( Cmd_Argv(2), "pause" ) )
	{
		pause = true;
	}

	int frame = atoi( Cmd_Argv( 1 ) );
	int demoframe, demototalframes;
	demo->GetProgressFrame( demoframe, demototalframes );

	if ( frame < demoframe )
	{
		Con_Printf( "demoskip: can't skip backwards\n" );
		return;
	}

	if ( frame == demoframe )
		return;

	// Issue skip
	demo->SkipToFrame( frame, pause );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_VTune_f( void )
{
 	if (Cmd_Argc() != 2)
	{
		Con_Printf ("vtune \"pause\" | \"resume\" : Suspend or resume VTune's sampling.\n");
		return;
	}

	
	if( !Q_strcasecmp( Cmd_Argv(1), "pause" ) )
	{
		if(!vtune(false))
		{
			Con_Printf("Failed to find \"VTPause()\" in \"vtuneapi.dll\".\n");
			return;
		}

		Con_Printf("VTune sampling paused.\n");
	}

	else if( !Q_strcasecmp( Cmd_Argv(1), "resume" ) )
	{
		if(!vtune(true))
		{
			Con_Printf("Failed to find \"VTResume()\" in \"vtuneapi.dll\".\n");
			return;
		}
		
		Con_Printf("VTune sampling resumed.\n");
	}

	else
	{
		Con_Printf("Unknown vtune option.\n");
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::Init( void )
{
	demoaction->Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::Shutdown( void )
{
	demoaction->Shutdown();
}

void CDemo::PausePlayback( float autoresumeafter /*=0.0f*/ )
{
	Assert( IsPlayingBack() );
	m_bPlaybackPaused = true;
	if ( autoresumeafter > 0.0f )
	{
		// Use true clock since everything else is frozen
		m_flAutoResumeTime = Sys_FloatTime() + autoresumeafter;
	}
}

void CDemo::ResumePlayback()
{
	Assert( IsPlayingBack() );
	m_bPlaybackPaused = false;
	m_flAutoResumeTime = 0.0f;
}

bool CDemo::IsPlaybackPaused() const
{
	if ( IsPlayingBack() )
	{
		return m_bPlaybackPaused;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : whentoresume - 
//-----------------------------------------------------------------------------
void CDemo::SkipToFrame( int whentoresume, bool pauseafterskip /*=false*/ )
{
	Assert( IsPlayingBack() );

	memset( &m_Skip, 0, sizeof( m_Skip ) );
	m_Skip.active = true;
	m_Skip.useframe = true;
	m_Skip.pauseafterskip = pauseafterskip;
	m_Skip.skiptoframe = whentoresume;
	m_Skip.skipstarttime = m_flLastFrameElapsed;
	m_Skip.skipstartframe = m_nFrameCount;

	m_Skip.skipframesremaining = whentoresume - m_nFrameCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : whentoresume - 
//-----------------------------------------------------------------------------
void CDemo::SkipToTime( float whentoresume, bool pauseafterskip /*= false*/ )
{
	Assert( IsPlayingBack() );

	memset( &m_Skip, 0, sizeof( m_Skip ) );
	m_Skip.active = true;
	m_Skip.useframe = false;
	m_Skip.pauseafterskip = pauseafterskip;
	m_Skip.skiptotime = whentoresume;
	m_Skip.skipstarttime = m_flLastFrameElapsed;
	m_Skip.skipstartframe = m_nFrameCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : elapsed - 
//			frame - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::ShouldSkip( float elapsed, int frame )
{
	if ( !m_Skip.active )
		return false;

	if ( m_Skip.useframe )
	{
		if ( --m_Skip.skipframesremaining <= 0 )
		{
			m_Skip.active = false;
			SCR_EndLoadingPlaque ();

			if ( m_Skip.pauseafterskip )
			{
				PausePlayback();
			}

			if ( m_DestCmdInfo.Count() > 0 )
			{
				m_LastCmdInfo = m_DestCmdInfo[ 0 ].info;
			}

			return false;
		}
	}
	else 
	{
		if ( elapsed >= m_Skip.skiptotime )
		{
			m_Skip.active = false;
			SCR_EndLoadingPlaque ();

			if ( m_Skip.pauseafterskip )
			{
				PausePlayback();
			}
			
			if ( m_DestCmdInfo.Count() > 0 )
			{
				m_LastCmdInfo = m_DestCmdInfo[ 0 ].info;
			}

			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CDemo::GetProgress( void )
{
	float t, totalt;
	GetProgressTime( t, totalt );
	if ( totalt > 0.0f )
	{
		return clamp( t / totalt, 0.0f, 1.0f );
	}
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &frame - 
//			totalframes - 
//-----------------------------------------------------------------------------
void CDemo::GetProgressFrame( int &frame, int& totalframes )
{
	frame = totalframes = 0;
	if ( !IsPlayingBack() )
		return;

	if ( m_nEntryIndex == 0 )
		return;

	frame = m_nFrameCount;
	totalframes = 0;
	if ( m_pEntry )
	{
		totalframes = m_pEntry->playback_frames;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : current - 
//			totaltime - 
//-----------------------------------------------------------------------------
void CDemo::GetProgressTime( float& current, float& totaltime )
{
	current = totaltime = 0.0f;
	if ( !IsPlayingBack() )
		return;

	if ( m_nEntryIndex == 0 )
		return;

	current = m_flLastFrameElapsed;
	totaltime = 0.0f;
	if ( m_pEntry )
	{
		totaltime = m_pEntry->playback_time;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pause - 
//-----------------------------------------------------------------------------
void CDemo::SetAutoPauseAfterInit( bool pause )
{
	m_bAutoPause = pause;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CDemo::GetPlaybackTimeScale( void ) const
{
	float scale = 1.0f;

	if ( !m_bFastForwarding )
	{
		scale = demoaction->GetPlaybackScale();
	}
	else
	{
		float dt = fabsf( Sys_FloatTime() - m_flFastForwardStartTime );
		float ramptime = max( demo_fastforwardramptime.GetFloat(), 0.001f );
		float frac = clamp( dt / ramptime, 0.0f, 1.0f );
		float scaleramp = demo_fastforwardfinalspeed.GetFloat() - demo_fastforwardstartspeed.GetFloat();
		scale = demo_fastforwardstartspeed.GetFloat() + frac * scaleramp;
	}

	scale *= GetPlaybackRateModifier();

	return scale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::AdvanceOneFrame( void )
{
	if ( !IsPlayingBack() )
		return;

	if ( !IsPlaybackPaused() )
		return;

	m_nAutoPauseFrame = m_nFrameCount + 1;
	ResumePlayback();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ff - 
//-----------------------------------------------------------------------------
void CDemo::SetFastForwarding( bool ff )
{
	bool changed = ff != m_bFastForwarding;

	m_bFastForwarding = ff;
	if ( changed )
	{
		m_flFastForwardStartTime = (float)Sys_FloatTime();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::IsFastForwarding( void )
{
	return m_bFastForwarding;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDemo::IsSkippingAhead( void )
{
	return m_Skip.active;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output :
//-----------------------------------------------------------------------------
void CDemo::SetSnapshotFilename( const char *pFileName )
{
	Q_strncpy( m_pSnapshotFilename, pFileName, sizeof( m_pSnapshotFilename ) - 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output :
//-----------------------------------------------------------------------------
const char *CDemo::GetSnapshotFilename( void )
{
	return m_pSnapshotFilename;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmdnumber - 
//-----------------------------------------------------------------------------
void CDemo::WriteUserCmd( int cmdnumber )
{
	if ( !IsRecording() )
		return;

	if ( !m_pDemoFile )
		return;

	WriteCmdHeader( dem_usercmd, m_pDemoFile );

	g_pFileSystem->Write( &cls.netchan.outgoing_sequence, sizeof( int ), m_pDemoFile );
	g_pFileSystem->Write( &cmdnumber, sizeof( int ), m_pDemoFile );

	byte buf[1024];
	bf_write msg( "CDemo::WriteUserCmd", buf, sizeof(buf) );

	g_ClientDLL->EncodeUserCmdToBuffer( msg, sizeof( buf ), cmdnumber );

	unsigned short bytes = (unsigned short)msg.GetNumBytesWritten();

	g_pFileSystem->Write( &bytes, sizeof( unsigned short ), m_pDemoFile );
	g_pFileSystem->Write( msg.GetBasePointer(), msg.GetNumBytesWritten(), m_pDemoFile );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : discard - 
//-----------------------------------------------------------------------------
void CDemo::ReadUserCmd( FileHandle_t demofile, bool discard )
{
	Assert( demofile );

	byte buf[1024];

	int slot;
	unsigned short bbytes;
	int outgoing_sequence;
	g_pFileSystem->Read( &outgoing_sequence, sizeof( int ), demofile );
	g_pFileSystem->Read( &slot, sizeof( int ), demofile );
	g_pFileSystem->Read( &bbytes, sizeof( short ), demofile );

	int bytes = (unsigned int)bbytes;

	g_pFileSystem->Read( buf, bytes, demofile );

#ifndef SWDS
	if ( !discard )
	{
		bf_read msg( "CDemo::ReadUserCmd", buf, sizeof(buf) );
		msg.StartReading( buf, bytes );
		g_ClientDLL->DecodeUserCmdFromBuffer( msg, sizeof( buf ), slot );

		cmd_t *cmd = &cl.commands[ slot & CL_UPDATE_MASK ];

		// Make sure we run this command
		cmd->processedfuncs = false;
		// Fill in some dummy data for demo playback purposes
		cmd->senttime		= 0.0f;
		cmd->receivedtime	= 0.1f;
		cmd->frame_lerp		= 0.1f;
		cmd->heldback		= false;
		cmd->sendsize		= 1;

		// Note, we need to have the current outgoing sequence correct so we can do prediction
		//  correctly during playback
		cls.netchan.outgoing_sequence = outgoing_sequence;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CDemo::GetPlaybackRateModifier( void ) const
{
	return m_flPlaybackRateModifier;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rate - 
//-----------------------------------------------------------------------------
void CDemo::SetPlaybackRateModifier( float rate )
{
	m_flPlaybackRateModifier = rate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::DispatchEvents( void )
{
	demoaction->DispatchEvents();
}

CON_COMMAND_AUTOCOMPLETEFILE( playdemo, CL_PlayDemo_f, "Play a recorded demo file (.dem ).", NULL, dem );
CON_COMMAND_AUTOCOMPLETEFILE( timedemo, CL_TimeDemo_f, "Play a demo and report performance info.", NULL, dem );
CON_COMMAND_AUTOCOMPLETEFILE( timedemoquit, CL_TimeDemoQuit_f, "Play a demo, report performance info, and then exit", NULL, dem );
CON_COMMAND_AUTOCOMPLETEFILE( listdemo, CL_ListDemo_f, "List demo file contents.", NULL, dem );
CON_COMMAND_AUTOCOMPLETEFILE( benchframe, CL_BenchFrame_f, "Takes a snapshot of a particular frame in a time demo.", NULL, dem );

static ConCommand record( "record", CL_Record_f, "Record a demo." );
static ConCommand stop( "stop", CL_StopRecording_f, "Finish recording demo." );
static ConCommand cl_vtune( "vtune", CL_VTune_f, "Controls VTune's sampling." );
static ConCommand demoskip( "demoskip", CL_DemoSkip_f, "During demo playback, skip to this frame." );

CON_COMMAND( demopause, "Pauses demo playback." )
{
	if ( demo->IsPlayingBack() )
	{
		demo->PausePlayback();
	}
}

CON_COMMAND( demoresume, "Resumes demo playback." )
{
	if ( demo->IsPlayingBack() )
	{
		demo->ResumePlayback();
	}
}

CON_COMMAND( demotogglepause, "Toggles demo playback." )
{
	if ( demo->IsPlayingBack() )
	{
		if ( demo->IsPlaybackPaused() )
		{
			demo->ResumePlayback();
		}
		else
		{
			demo->PausePlayback();
		}
	}
}

CON_COMMAND( demopauseafterinit, "Auto-pause demo after loading through init phase." )
{
	if ( demo->IsPlayingBack() )
	{
		demo->SetAutoPauseAfterInit( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::DrawDebuggingInfo( void )
{
	if ( !demo->IsPlayingBack() )
		return;

	demoaction->DrawDebuggingInfo( m_nFrameCount, m_flLastFrameElapsed );
}

bool CDemo::OverrideViewSetup( democmdinfo_t& info )
{
	if ( !IsPlayingBack() )
		return false;

	if ( demoaction->OverrideView( info, m_nFrameCount, m_flLastFrameElapsed ) )
	{
		return true;
	}

	return false;
}

void CDemo::ParseSmoothingInfo( FileHandle_t demofile, demodirectory_t& directory, CUtlVector< demosmoothing_t >& smooth )
{
	// Go to data section
	int entryindex = 1;
	demoentry_t *pentry = &directory.entries[ entryindex ];
	g_pFileSystem->Seek( demofile, pentry->offset, FILESYSTEM_SEEK_HEAD );

	democmdinfo_t info;

	byte dummymessage_buffer[ NET_MAX_MESSAGE ];
	sizebuf_t dummymessage;

	dummymessage.maxsize = sizeof(dummymessage_buffer);
	dummymessage.data = dummymessage_buffer;

	bool demofinished = false;
	while ( !demofinished )
	{
		float		f = 0.0f;
		int			dframe = 0;
		byte		cmd;

		bool swallowmessages = true;
		do
		{
			ReadCmdHeader( demofile, cmd, f, dframe );

			// COMMAND HANDLERS
			switch ( cmd )
			{
			case dem_jumptime:
				break;
			case dem_stop:
				{
					demofinished = true;
				}
				break;
			case dem_clientdll:
				{
					int length;
					g_pFileSystem->Read( &length, sizeof( int ), demofile );
					Assert( length >= 1 && length <= 2048 );
					char szCmdName[ 2048 ];
					g_pFileSystem->Read ( szCmdName, length, demofile );
				}
				break;
			case dem_stringtables:
				{
					int length;
					g_pFileSystem->Read( &length, sizeof( int ), demofile );
					int curpos = g_pFileSystem->Tell( demofile );
					// Skip ahead
					g_pFileSystem->Seek( demofile, curpos + length, FILESYSTEM_SEEK_HEAD );
				}
				break;
			case dem_usercmd:
				{
					ReadUserCmd( demofile, true );
				}
				break;
			default:
				{
					swallowmessages = false;
				}
				break;
			}
		}
		while ( swallowmessages );

		if ( demofinished )
			break;

		int curpos = g_pFileSystem->Tell( demofile );

		ReadCmdInfo( demofile, info );

		ReadSequenceInfo( demofile, true );
		ReadRawNetworkData( demofile, dframe, dummymessage );

		// Add to end of list
		demosmoothing_t smoothing_entry;

		smoothing_entry.file_offset = curpos;
		smoothing_entry.frametime = f;
		smoothing_entry.framenumber = dframe;
		smoothing_entry.info = info;
		smoothing_entry.samplepoint = false;
		smoothing_entry.vecmoved = 	smoothing_entry.info.GetViewOrigin();
		smoothing_entry.angmoved = 	smoothing_entry.info.GetViewAngles();
		smoothing_entry.targetpoint = false;
		smoothing_entry.vectarget = smoothing_entry.info.GetViewOrigin();

		smooth.AddToTail( smoothing_entry );
	}
}

void CDemo::LoadSmoothingInfo( const char *filename, CSmoothingContext& smoothing )
{
	int i;
	FileHandle_t demofile;

	//
	// open the demo file
	//
	char name[ MAX_OSPATH ];
	strcpy (name, filename);
	COM_DefaultExtension( name, ".dem", sizeof( name ) );

	Con_Printf ("Smoothing demo from %s.\n", name);
	COM_OpenFile(name, &demofile);
	if (!demofile)
	{
		Con_Printf( "ERROR: couldn't open %s.\n", name );
		return;
	}
	
	demoheader_t header;
	// Read in the m_DemoHeader
	g_pFileSystem->Read( &header, sizeof(header), demofile );
	if ( strcmp ( header.demofilestamp, "HLDEMO" ) )
	{
		Con_Printf( "%s is not a demo file\n", name);
		g_pFileSystem->Close(demofile);
		demofile = NULL;
		return;
	}

	if ( header.networkprotocol != PROTOCOL_VERSION ||
		 header.demoprotocol       != DEMO_PROTOCOL )
	{
		Con_Printf (
			"ERROR: demo protocol outdated\n"
			"Demo file protocols Network(%i), Demo(%i)\n"
			"Server protocol is at Network(%i), Demo(%i)\n",
			header.networkprotocol, 
			header.demoprotocol,
			PROTOCOL_VERSION,
			DEMO_PROTOCOL
		);
		g_pFileSystem->Close(demofile);
		demofile = NULL;
		return;
	}

	demodirectory_t directory;

	// Now read in the directory structure.
	g_pFileSystem->Seek( demofile, header.directory_offset, FILESYSTEM_SEEK_HEAD );
	g_pFileSystem->Read( &directory.numentries, sizeof(int), demofile );

	if (directory.numentries < 1 ||
		directory.numentries > 1024 )
	{
		Con_Printf ("CDemo::LoadSmoothingInfo: demo had bogus # of directory entries:  %i\n",
			directory.numentries);
		g_pFileSystem->Close(demofile);
		demofile = NULL;
		return;
	}

	demoentry_t entry;
	for ( i = 0; i <  directory.numentries; i++ )
	{
		memset( &entry, 0, sizeof( entry ) );
		g_pFileSystem->Read( &entry, sizeof( entry ), demofile );
		directory.entries.AddToTail( entry );
	}

	smoothing.active = true;
	strcpy( smoothing.filename, name );

	smoothing.smooth.RemoveAll();

	ClearSmoothingInfo( smoothing );

	ParseSmoothingInfo( demofile, directory, smoothing.smooth );
	
	g_pFileSystem->Close(demofile);

	//Performsmoothing( smooth );
	//SaveSmoothedDemo( name, smooth );

	Con_Printf ( "...done.\n" );
}

void CDemo::ClearSmoothingInfo( CSmoothingContext& smoothing )
{
	int c = smoothing.smooth.Count();
	int i;

	for ( i = 0; i < c; i++ )
	{
		demosmoothing_t	*p = &smoothing.smooth[ i ];
		p->info.Reset();
		p->vecmoved = p->info.GetViewOrigin();
		p->angmoved = p->info.GetViewAngles();
		p->samplepoint = false;
		p->vectarget = p->info.GetViewOrigin();
		p->targetpoint = false;
	}
}

void CDemo::SaveSmoothingInfo( char const *filename, CSmoothingContext& smoothing )
{
	// Nothing to do
	int c = smoothing.smooth.Count();
	if ( !c )
		return;

	IFileSystem *fs = g_pFileSystem;

	FileHandle_t infile, outfile;

	COM_OpenFile( filename, &infile );
	if ( infile == FILESYSTEM_INVALID_HANDLE )
		return;

	int filesize = fs->Size( infile );

	char outfilename[ 512 ];
	COM_StripExtension( filename, outfilename, sizeof( outfilename ) );
	strcat( outfilename, "_smooth" );
	COM_DefaultExtension( outfilename, ".dem", sizeof( outfilename ) );
	outfile = fs->Open( outfilename, "wb" );
	if ( outfile == FILESYSTEM_INVALID_HANDLE )
	{
		fs->Close( infile );
		return;
	}

	int i;

	int lastwritepos = 0;
	for ( i = 0; i < c; i++ )
	{
		demosmoothing_t	*p = &smoothing.smooth[ i ];

		//fs->Seek( infile, p->file_offset, FILESYSTEM_SEEK_HEAD );

		int copyamount = p->file_offset - lastwritepos;

		CL_CopyChunk( outfile, infile, copyamount );

		fs->Seek( infile, p->file_offset, FILESYSTEM_SEEK_HEAD );

		WriteCmdInfo( outfile, p->info );
		lastwritepos = fs->Tell( outfile );
		fs->Seek( infile, p->file_offset + sizeof( democmdinfo_t ), FILESYSTEM_SEEK_HEAD );
	}

	int final = filesize - lastwritepos;

	CL_CopyChunk( outfile, infile, final );

	fs->Close( outfile );
	fs->Close( infile );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mapname - 
//-----------------------------------------------------------------------------
void CDemo::CheckAutoRecord( char const *mapname )
{
	if ( demo->IsPlayingBack() )
		return;

	if ( !m_bAutoRecord )
		return;

	if ( !m_szAutoRecordMap[ 0 ] || !m_szAutoRecordDemoName[ 0 ] )
	{
		ClearAutoRecord();
		return;
	}

	if ( Q_strcasecmp( mapname, m_szAutoRecordMap ) )
	{
		ClearAutoRecord();
		return;
	}

	char	name[ MAX_OSPATH ];
	sprintf (name, "%s", m_szAutoRecordDemoName );
	COM_DefaultExtension( name, ".dem", sizeof( name ) );

	Record( name );
	ClearAutoRecord();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDemo::ClearAutoRecord( void )
{
	m_bAutoRecord = false;
	m_szAutoRecordMap[ 0 ] = 0;
	m_szAutoRecordDemoName[ 0 ] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mapname - 
//			*demoname - 
//-----------------------------------------------------------------------------
void CDemo::SetAutoRecord( char const *mapname, char const *demoname )
{
	if ( demo->IsRecording() )
		return;

	if ( demo->IsPlayingBack() )
		return;

	m_bAutoRecord = true;;
	Q_strncpy( m_szAutoRecordMap, mapname, sizeof( m_szAutoRecordMap ) );
	Q_strncpy( m_szAutoRecordDemoName, demoname, sizeof( m_szAutoRecordDemoName ) );
}

void CDemo::StartRendering( void )
{
	if( !IsPlayingBack() )
	{
		return;
	}

	if( !IsPlayingBack_TimeDemo() )
	{
		return;
	}

	m_nFrameStartedRendering = m_nFrameCount;
}

void CDemo::WriteStringTables( FileHandle_t& demofile )
{
	CUtlBuffer buf;
	CL_WriteStringTables( buf );

	int length = buf.TellPut();

	g_pFileSystem->Write( &length, sizeof( int ), demofile );
	g_pFileSystem->Write( buf.Base(), buf.TellPut(), demofile );
}

void CDemo::ReadStringTables( int expected_length, FileHandle_t& demofile )
{
	CUtlBuffer buf;
	byte data[ 1024 ];
	while( expected_length > 0 )
	{
		int chunk = min( expected_length, 1024 );
		g_pFileSystem->Read( data, chunk, demofile );
		expected_length -= chunk;
		buf.Put( data, chunk );
	}

	bool success = CL_ReadStringTables( buf );
	if ( !success )
	{
		Host_Error( "Error parsing string tables during demo playback." );
	}
}