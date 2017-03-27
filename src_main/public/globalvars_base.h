//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GLOBALVARS_BASE_H
#define GLOBALVARS_BASE_H
#ifdef _WIN32
#pragma once
#endif

class CSaveRestoreData;

//-----------------------------------------------------------------------------
// Purpose: Global variables used by shared code
//-----------------------------------------------------------------------------
class CGlobalVarsBase
{
public:

	CGlobalVarsBase( bool bIsClient );
	
	// This can be used to filter debug output or to catch the client or server in the act.
	// ONLY valid during debugging.
#ifdef _DEBUG
	bool IsClient() const;
#endif
	

public:
	
	// Absolute time (per frame still)
	float			realtime;
	// Absolute frmae counter
	int				framecount;
	// Non-paused frametime
	float			absoluteframetime;

	// Current time (usually tickcount * tick_interval, except during client rendering)
	float			curtime;
	// Time spent on last server or client frame (has nothing to do with think intervals)
	float			frametime;
	// current maxplayers
	int				maxClients;

	// Simulation ticks
	int				tickcount;
	// Simulation tick interval
	float			tick_interval;
	// interpolation amount ( client-only ) based on fraction of next tick that's pending
	float			interpolation_amount;

	// current saverestore data
	CSaveRestoreData *pSaveData;


private:
	// Set to true in client code. This can only be used for debugging code.
	bool			m_bClient;
};


inline CGlobalVarsBase::CGlobalVarsBase( bool bIsClient )
{
	m_bClient = bIsClient;
}


#ifdef _DEBUG
	inline bool CGlobalVarsBase::IsClient() const
	{
		return m_bClient;
	}
#endif


#endif // GLOBALVARS_BASE_H