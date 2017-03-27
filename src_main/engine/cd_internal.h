//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( CD_INTERNAL_H )
#define CD_INTERNAL_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Implements cd audio handling
//-----------------------------------------------------------------------------
class CCDAudio : public ICDAudio
{
	static const float TRACK_EXTRA_TIME; 

public:
						CCDAudio( void );
	virtual				~CCDAudio( void );

	int					Init( void );
	void				Shutdown( void );

	void				Play( int track, bool looping );
	void				Pause( void );
	void				Resume( void );
	void				Frame( void );

	void				CD_f ( void );

private:
	enum
	{					
		MAX_REMAP = 100
	};

	// Length of the currently playing cd track 
	float				m_flPlayTime;  
	// Time when we started playing the cd
	double				m_dStartTime; 
	// Time when we entered pause
	double				m_dPauseTime; 
	bool				m_bIsValid;
	bool				m_bIsPlaying;
	bool				m_bWasPlaying;
	bool				m_bInitialized;
	bool				m_bEnabled;
	bool				m_bIsLooping;
	float				m_flVolume;
	int					m_nPlayTrack;
	int					m_nMaxTrack;
	// Special flag for resuming an actually playing 
	//  track when we switch to / from launcher.
	bool				m_bResumeOnSwitch;
	unsigned int		m_uiDeviceID;
	int 				m_rgRemapCD[ MAX_REMAP ];

private:
	void				ResetCDTimes( void );
	void				Stop( void );
	void				Eject( void );
	void				CloseDoor( void );
	void				GetAudioDiskInfo( void );
	void				SwitchToLauncher( void );
	void				SwitchToEngine( void );
	void				Reset( void );

private:
	// These functions are called from inside the thread processing loop
	void				_Stop( int, int );
	void				_Pause( int, int );
	void				_Eject( int, int );
	void				_CloseDoor( int, int );
	void				_GetAudioDiskInfo( int, int );
	void				_Play(int track, int looping);
	void				_Resume( int, int );
	void				_SwitchToLauncher( int, int );
	void				_SwitchToEngine( int, int );
	void				_CDUpdate( int, int );
	void				_CDReset( int, int );
};

CCDAudio *GetInteralCDAudio( void );

#endif // CD_INTERNAL_H