#pragma warning( disable : 4201 )
#include <windows.h>
#include "basetypes.h"
#include <mmsystem.h>
#include "cd.h"
#include "cd_internal.h"
#include <stdlib.h>

#include "exefuncs.h"
#include "engine_launcher_api.h"
#include "vstdlib/icommandline.h"
#include "igame.h"
#include "ithread.h"
#include "convar.h"
#include "cmd.h"

static ConVar bgmvolume( "bgmvolume", "1", FCVAR_ARCHIVE );

static CCDAudio g_CDAudio;
ICDAudio *cdaudio = ( ICDAudio * )&g_CDAudio;

// Output : 
// Wait 2 extra seconds before stopping playback.
const float CCDAudio::TRACK_EXTRA_TIME = 2.0f;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CCDAudio
//-----------------------------------------------------------------------------
CCDAudio *GetInteralCDAudio( void )
{
	return &g_CDAudio;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CD_f( void )
{
	g_CDAudio.CD_f();
}

static ConCommand cmd_cd( "cd", CD_f, "Play or stop a cd track." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCDAudio::CCDAudio( void )
{
	ResetCDTimes();

	m_bIsValid		= false;
	m_bIsPlaying	= false;
	m_bWasPlaying	= false;
	m_bInitialized	= false;
	m_bEnabled		= false;
	m_bIsLooping	= false;
	m_flVolume		= 0.0f;
	m_nPlayTrack	= 0;
	m_nMaxTrack		= 0;
	m_bResumeOnSwitch = false;
	m_uiDeviceID	= 0;
	memset( m_rgRemapCD, 0, sizeof( m_rgRemapCD ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCDAudio::~CCDAudio( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Reset clock
//-----------------------------------------------------------------------------
void CCDAudio::ResetCDTimes( void )
{
	m_flPlayTime  = 0.0f;
	m_dStartTime = 0.0;
	m_dPauseTime = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: Stop playing cd
//-----------------------------------------------------------------------------
void CCDAudio::_Stop( int, int )
{
	if (!m_bEnabled)
		return;

	if ((!m_bIsPlaying) && (!m_bWasPlaying))
		return;

	m_bIsPlaying = false;
	m_bWasPlaying = false;

    mciSendCommand(m_uiDeviceID, MCI_STOP, 0, (DWORD)NULL);
		
	ResetCDTimes();
}

//-----------------------------------------------------------------------------
// Purpose: Pause playback
//-----------------------------------------------------------------------------
void CCDAudio::_Pause( int, int )
{
	MCI_GENERIC_PARMS	mciGenericParms;

	if (!m_bEnabled)
		return;

	if (!m_bIsPlaying)
		return;

	mciGenericParms.dwCallback = (DWORD)game->GetMainWindow();
    mciSendCommand(m_uiDeviceID, MCI_PAUSE, 0, (DWORD)(LPVOID) &mciGenericParms);

	m_bWasPlaying = m_bIsPlaying;
	m_bIsPlaying = false;

	m_dPauseTime = Sys_FloatTime();
}

//-----------------------------------------------------------------------------
// Purpose: Eject the disk
//-----------------------------------------------------------------------------
void CCDAudio::_Eject( int, int )
{
	DWORD	dwReturn;

	_Stop( 0, 0 );

    dwReturn = mciSendCommand(m_uiDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD)NULL);

	ResetCDTimes();
}

//-----------------------------------------------------------------------------
// Purpose: Close the cd drive door
//-----------------------------------------------------------------------------
void CCDAudio::_CloseDoor( int, int )
{
	DWORD	dwReturn;

    dwReturn = mciSendCommand(m_uiDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, (DWORD)NULL);

	ResetCDTimes();
}

//-----------------------------------------------------------------------------
// Purpose: Retrieve validity and max track data
//-----------------------------------------------------------------------------
void CCDAudio::_GetAudioDiskInfo( int, int )
{
	DWORD				dwReturn;
	MCI_STATUS_PARMS	mciStatusParms;


	m_bIsValid = false;

	mciStatusParms.dwItem = MCI_STATUS_READY;
    dwReturn = mciSendCommand(m_uiDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		return;
	}
	if (!mciStatusParms.dwReturn)
	{
		return;
	}

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    dwReturn = mciSendCommand(m_uiDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		return;
	}
	if (mciStatusParms.dwReturn < 1)
	{
		return;
	}

	m_bIsValid = true;
	m_nMaxTrack = (int)mciStatusParms.dwReturn;
}

//-----------------------------------------------------------------------------
// Purpose: Play cd
// Input  : track - 
//			looping - 
//-----------------------------------------------------------------------------
void CCDAudio::_Play( int track, int looping )
{
	int mins, secs;
	DWORD               dwm_flPlayTime;
	DWORD				dwReturn;
    MCI_PLAY_PARMS		mciPlayParms;
	MCI_STATUS_PARMS	mciStatusParms;

	if (!m_bEnabled)
		return;
	
	if (!m_bIsValid)
	{
		GetAudioDiskInfo();
		if (!m_bIsValid)
			return;
	}

	track = m_rgRemapCD[track];

	if (track < 1 || track > m_nMaxTrack)
	{
		return;
	}

	// don't try to play a non-audio track
	mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = track;
    dwReturn = mciSendCommand(m_uiDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		char szErr[256];
		int nErr = 256;

		mciGetErrorString(dwReturn, szErr, nErr);
		return;
	}
	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO)
	{
		return;
	}

	// get the length of the track to be played
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
    dwReturn = mciSendCommand(m_uiDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		return;
	}

	if (m_bIsPlaying)
	{
		if (m_nPlayTrack == track)
			return;
		_Stop( 0, 0 );
	}

	dwm_flPlayTime = mciStatusParms.dwReturn;

    mciPlayParms.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = (mciStatusParms.dwReturn << 8) | track;
    mciPlayParms.dwCallback = (DWORD)game->GetMainWindow();
    dwReturn = mciSendCommand(m_uiDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)(LPVOID) &mciPlayParms);
	if (dwReturn)
	{
		return;
	}

	// Clear any old data.
	ResetCDTimes();

	m_dStartTime = Sys_FloatTime();

	mins = MCI_MSF_MINUTE(dwm_flPlayTime);
	secs = MCI_MSF_SECOND(dwm_flPlayTime);

	m_flPlayTime = (float)(60.0f * mins + (float) secs );

	m_bIsLooping = looping ? true : false;
	m_nPlayTrack = track;
	m_bIsPlaying = true;

	if (m_flVolume == 0.0)
		_Pause ( 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Resume playing cd
//-----------------------------------------------------------------------------
void CCDAudio::_Resume( int, int )
{
	double curTime;
	DWORD			dwReturn;
    MCI_PLAY_PARMS	mciPlayParms;

	if (!m_bEnabled)
		return;
	
	if (!m_bIsValid)
		return;

	if (!m_bWasPlaying)
		return;

    mciPlayParms.dwFrom = MCI_MAKE_TMSF(m_nPlayTrack, 0, 0, 0);
    mciPlayParms.dwTo = MCI_MAKE_TMSF(m_nPlayTrack + 1, 0, 0, 0);
    mciPlayParms.dwCallback = (DWORD)game->GetMainWindow();
    dwReturn = mciSendCommand(m_uiDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID) &mciPlayParms);
	if (dwReturn)
	{
		ResetCDTimes();
		return;
	}
	m_bIsPlaying = true;

	curTime = Sys_FloatTime();

	// Subtract the elapsed time from the current playback time. (i.e., add it to the start time).

	m_dStartTime += (curTime - m_dPauseTime);

	m_dPauseTime = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: Save state when going into launcher
//-----------------------------------------------------------------------------
void CCDAudio::_SwitchToLauncher( int, int )
{
	if (m_bEnabled && ( m_bIsPlaying != false ) ) 
	{
		m_bResumeOnSwitch = true;
		// Pause device
		_Pause( 0, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Restore cd playback if needed
//-----------------------------------------------------------------------------
void CCDAudio::_SwitchToEngine( int, int )
{
	if (m_bResumeOnSwitch)
	{
		m_bResumeOnSwitch = false;
		_Resume( 0, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle "cd" console command
//-----------------------------------------------------------------------------
void CCDAudio::CD_f ( void )
{
	char	*command;
	int		ret;
	int		n;

	if ( Cmd_Argc() < 2 )
		return;

	command = Cmd_Argv (1);

	if (stricmp(command, "on") == 0)
	{
		m_bEnabled = true;
		return;
	}

	if (stricmp(command, "off") == 0)
	{
		if (m_bIsPlaying)
			Stop();
		m_bEnabled = false;
		return;
	}

	if (stricmp(command, "reset") == 0)
	{
		m_bEnabled = true;
		if (m_bIsPlaying)
			Stop();
		for (n = 0; n < 100; n++)
			m_rgRemapCD[n] = n;
		GetAudioDiskInfo();
		return;
	}

	if (stricmp(command, "remap") == 0)
	{
		ret = Cmd_Argc() - 2;
		if ( ret > 0)
		{
			for (n = 1; n <= ret; n++)
			{
				m_rgRemapCD[n] = atoi(Cmd_Argv (n+1));
			}
		}
		return;
	}

	if (stricmp(command, "close") == 0)
	{
		CloseDoor();
		return;
	}

	if (!m_bIsValid)
	{
		GetAudioDiskInfo();
		if (!m_bIsValid)
		{
			return;
		}
	}

	if (stricmp(command, "play") == 0)
	{
		Play( atoi(Cmd_Argv (2)), false );
		return;
	}

	if (stricmp(command, "loop") == 0)
	{
		Play( atoi(Cmd_Argv (2)), true );
		return;
	}

	if (stricmp(command, "stop") == 0)
	{
		Stop();
		return;
	}

	if (stricmp(command, "pause") == 0)
	{
		Pause();
		return;
	}

	if (stricmp(command, "resume") == 0)
	{
		Resume();
		return;
	}

	if (stricmp(command, "eject") == 0)
	{
		if (m_bIsPlaying)
			Stop();
		Eject();
		m_bIsValid = false;
		return;
	}

	if (stricmp(command, "info") == 0)
	{
		Msg("%u tracks\n", m_nMaxTrack);
		if ( m_bIsPlaying )
		{
			Msg("Currently %s track %u\n", m_bIsLooping ? "looping" : "playing", m_nPlayTrack);
		}
		else if ( m_bWasPlaying )
		{
			Msg("Paused %s track %u\n", m_bIsLooping ? "looping" : "playing", m_nPlayTrack);
		}
		Msg("Volume is %f\n", m_flVolume);
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Frame update
//-----------------------------------------------------------------------------
void CCDAudio::_CDUpdate( int, int )
{
	if (!m_bIsPlaying)
		return;

	// Make sure we set a valid track length.
	if ( !m_flPlayTime || ( m_dStartTime == 0.0 ) )
		return;

	double current;

	current = Sys_FloatTime();

	if (((float)(current - m_dStartTime) >= (m_flPlayTime + TRACK_EXTRA_TIME) ))
	{
		if (m_bIsPlaying)
		{
			m_bIsPlaying = false;
			if (m_bIsLooping)
				Play(m_nPlayTrack, true);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initialize cd audio device handling
// Output : int
//-----------------------------------------------------------------------------
int CCDAudio::Init( void )
{
	DWORD	dwReturn;
	MCI_OPEN_PARMS	mciOpenParms;
    MCI_SET_PARMS	mciSetParms;
	int				n;

	if ( CommandLine()->CheckParm( "-nocdaudio" ) )
	{
		return 0;
	}

	ResetCDTimes();

	mciOpenParms.lpstrDeviceType = "cdaudio";
	if (dwReturn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD) (LPVOID) &mciOpenParms))
	{
		return -1;
	}
	m_uiDeviceID = mciOpenParms.wDeviceID;

    // Set the time format to track/minute/second/frame (TMSF).
    mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
    if (dwReturn = mciSendCommand(m_uiDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID) &mciSetParms))
    {
        mciSendCommand(m_uiDeviceID, MCI_CLOSE, 0, (DWORD)NULL);
		return -1;
    }

	for (n = 0; n < 100; n++)
	{
		m_rgRemapCD[n] = n;
	}
	m_bInitialized	= true;
	m_bEnabled		= true;

	// set m_bIsValid flag
	GetAudioDiskInfo();

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown cd audio device
//-----------------------------------------------------------------------------
void CCDAudio::Shutdown( void )
{
	if (!m_bInitialized)
		return;
	Stop();
	mciSendCommand(m_uiDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)NULL);

	ResetCDTimes();
}

//-----------------------------------------------------------------------------
// Purpose: Shutown and restart system
//-----------------------------------------------------------------------------
void CCDAudio::_CDReset( int, int )
{
	Shutdown();
	Sleep(1000);
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::Reset( void )
{
	thread->AddThreadItem( _CDReset, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::Stop( void )
{
	thread->AddThreadItem(  _Stop, 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::Pause( void )
{
	thread->AddThreadItem( _Pause, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::Eject( void )
{
	thread->AddThreadItem( _Eject, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::CloseDoor( void )
{
	thread->AddThreadItem( _CloseDoor, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::GetAudioDiskInfo( void )
{
	thread->AddThreadItem( _GetAudioDiskInfo, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : track - 
//			looping - 
//-----------------------------------------------------------------------------
void CCDAudio::Play( int track, bool looping )
{
	thread->AddThreadItem( _Play, track, looping ? 1 : 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::Resume( void )
{
	thread->AddThreadItem( _Resume, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::SwitchToLauncher( void )
{
	thread->AddThreadItem( _SwitchToLauncher, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::SwitchToEngine( void )
{
	thread->AddThreadItem( _SwitchToEngine, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCDAudio::Frame( void )
{
	if (!m_bEnabled)
		return;

	if ( m_flVolume != bgmvolume.GetFloat() )
	{
		if ( m_flVolume )
		{
			m_flVolume = 0.0f;
			Pause ();
		}
		else
		{
			m_flVolume = 1.0f;
			Resume ();
		}
		bgmvolume.SetValue( m_flVolume );
	}

	thread->AddThreadItem( _CDUpdate, 0, 0 );
}