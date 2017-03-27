//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef NETSTATEMGR_H
#define NETSTATEMGR_H
#ifdef _WIN32
#pragma once
#endif


#include "iservernetworkable.h"


#define AUTOUPDATE_MAX_TIME_LENGTH	5.0	// maximum of 5 seconds between autoupdates
#define AUTOUPDATE_FREQ_SCALE		(65535.0f / AUTOUPDATE_MAX_TIME_LENGTH)


class CNetStateMgr
{
public:
					
					CNetStateMgr();
	
	// This is useful for entities that don't change frequently or that the client
	// doesn't need updates on very often. If you use this mode, the server will only try to
	// detect state changes every N seconds, so it will save CPU cycles and bandwidth.
	//
	// Note: N must be less than AUTOUPDATE_MAX_TIME_LENGTH.
	//
	// Set back to zero to disable the feature.
	//
	// This feature works on top of manual mode. 
	// - If you turn it on and manual mode is off, it will autodetect changes every N seconds.
	// - If you turn it on and manual mode is on, then every N seconds it will only say there
	//   is a change if you've called NetworkStateChanged.
	void			SetUpdateInterval( float N );

	// Call this with true to put the entity into a state where it 
	// explicitly indicates its networking state has changed
	// Call it with false to put it back into auto-detect mode
	void			EnableManualMode( bool bEnable );

	// Tells if manual mode is enabled.
	bool			IsUsingManualMode();

	// Returns whether or not the entity has changed (since the last call).
	EntityChange_t	DetectStateChanges();

	// Called by the engine to indicate that its processed the state changes
	void			ResetStateChanges();

	// Force a change in the state. If bForceUpdate is true, then even if the 
	// entity is using an update interval, it will return a change next frame.
	void			StateChanged( bool bForceUpdate = false );

private:
	bool			m_bUsingManualMode;
	bool			m_bChanged;
	bool			m_bTimerElapsed;

	// Counters for SetUpdateInterval.
	unsigned short	m_NSUpdateInterval;	// Real value is m_AutoUpdateFrequency * AUTOUPDATE_FREQ_SCALE
	unsigned short	m_NSUpdateCounter;	// Counts down to zero. When zero, it triggers an auto update.
};



// ----------------------------------------------------------------------------- //
// Inlines.
// ----------------------------------------------------------------------------- //

inline bool CNetStateMgr::IsUsingManualMode()
{
	return m_bUsingManualMode != 0;
}

inline void CNetStateMgr::StateChanged( bool bForceUpdate )
{
	m_bChanged = true;
	
	if ( bForceUpdate )
		m_NSUpdateCounter = 0;
}


#endif // NETSTATEMGR_H
