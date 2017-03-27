//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef DEMO_H
#define DEMO_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

typedef struct sizebuf_s sizebuf_t;

// NOTE:  Change this any time the file version of the demo changes
#define DEMO_PROTOCOL 2

#include "cl_demosmoothing.h"

bool Demo_IsPlayingBack();  // C wrapper function so demo object isn't included in server side only code
bool Demo_IsPlayingBack_TimeDemo();  // C wrapper function so demo object isn't included in server side only code

struct demoentry_t
{
	int      entrytype;			// DEMO_STARTUP or DEMO_NORMAL
	float    playback_time;		// Time of track
	int      playback_frames;	// # of frames in track
	int      offset;			// File offset of track data
	int      length;			// Length of track
};

struct demodirectory_t
{
	// Number of tracks
	int							numentries;
	// Track entry info
	CUtlVector < demoentry_t >	entries;
};

struct demoheader_t
{
	char demofilestamp[6];				// Should be HLDEMO
	int  demoprotocol;					// Should be DEMO_PROTOCOL
	int  networkprotocol;				// Should be PROTOCOL_VERSION
	char mapname[ MAX_OSPATH ];			// Name of map
	char gamedirectory[ MAX_OSPATH ];	// Name of game directory (com_gamedir)
	int  directory_offset;				// Offset of Entry Directory.
};

//-----------------------------------------------------------------------------
// Purpose: Demo recording and playback
//-----------------------------------------------------------------------------
class IDemo
{
public:
	// Initialize demo system
	virtual	void			Init( void ) = 0;
	virtual	void			Shutdown( void ) = 0;
	// Force playback to stop
	virtual	void			StopPlayback( void ) = 0;
	// Force recording to stop
	virtual void			StopRecording( void ) = 0;
	// Check whether we are playing a demo
	virtual bool			IsPlayingBack( void ) const = 0;
	// Are we playing a demo with timedemo set
	virtual bool			IsPlayingBack_TimeDemo( void ) = 0;
	// Is the timer started for timedemo recording?
	virtual bool			IsTimeDemoTimerStarted( void ) = 0;
	// Are we recording a demo
	virtual bool			IsRecording( void ) = 0;
	// Allow the client .dll commands to write themselves into the demo format
	virtual void			RecordCommand( const char *cmdstring ) = 0;
	// Let demo mark time
	virtual void			MarkFrame( float flCurTime, float flFPSVariability ) = 0;
	// Reset the info we spool at every level load
	virtual void			StartupDemoHeader( void ) = 0;
	// See if there is a demo message ready for reading
	virtual bool			ReadMessage( void ) = 0;
	// Write a message to the demo output stream
	virtual void			WriteMessage( bool startup, int start, sizebuf_t *msg ) = 0;
	// Change whether we are waiting for an uncompressed update
	virtual void			SetWaitingForUpdate( bool waiting ) = 0;
	// Are we waiting for an uncompressed update
	virtual bool			IsWaitingForUpdate( void ) = 0;
	// Allow demo system to provide interpolated view origin/angles during playback
	virtual void			GetInterpolatedViewpoint( void ) = 0;
	// Get the name of the current demo file
	virtual void			PausePlayback( float autoresumeafter = 0.0f ) = 0;
	virtual void			ResumePlayback() = 0;
	virtual bool			IsPlaybackPaused() const = 0;
	virtual void			SkipToFrame( int whentoresume, bool pauseafterskip = false ) = 0;
	virtual void			SkipToTime( float whentoresume, bool pauseafterskip = false ) = 0;


	virtual float			GetProgress( void ) = 0;
	virtual void			GetProgressFrame( int &frame, int& totalframes ) = 0;
	virtual void			GetProgressTime( float& current, float& totaltime ) = 0;

	virtual void			SetAutoPauseAfterInit( bool pause ) = 0;
	virtual float			GetPlaybackTimeScale( void ) const = 0;
	virtual void			SetFastForwarding( bool ff ) = 0;
	// If paused, advance a single frame
	virtual void			AdvanceOneFrame( void ) = 0;

	virtual void			WriteUserCmd( int cmdnumber ) = 0;

	virtual bool			IsFastForwarding( void ) = 0;
	virtual bool			IsSkippingAhead( void ) = 0;

	virtual float			GetPlaybackRateModifier( void ) const = 0;
	virtual void			SetPlaybackRateModifier( float rate ) = 0;

	virtual void			DispatchEvents( void ) = 0;

	// Smoothing helpers
	virtual void			DrawDebuggingInfo( void ) = 0;
	
	virtual void			LoadSmoothingInfo( const char *filename, CSmoothingContext& smoothing ) = 0;
	virtual void			ClearSmoothingInfo( CSmoothingContext& smoothing ) = 0;
	virtual void			SaveSmoothingInfo( char const *filename, CSmoothingContext& smoothing ) = 0;

	virtual void			CheckAutoRecord( char const *mapname ) = 0;
	virtual void			SetAutoRecord( char const *mapname, char const *demoname ) = 0;

	virtual void			StartRendering( void ) = 0;

	virtual bool			IsTakingSnapshot() = 0;
	virtual void			SetSnapshotFilename( const char * ) = 0;
	virtual const char *	GetSnapshotFilename( void ) = 0;
};

// expose the interface to the rest of the engine as needed
extern IDemo *demo;

#endif // DEMO_H
