//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "netstatemgr.h"
#include "tier0/dbg.h"
#include "util.h"


CNetStateMgr::CNetStateMgr()
{
	m_bTimerElapsed = false;
	m_bChanged = false;
	m_bUsingManualMode = false;
	m_NSUpdateInterval = 0;
	m_NSUpdateCounter = 0;
}


void CNetStateMgr::SetUpdateInterval( float val )
{
	Assert( val >= 0 );

	if( val > AUTOUPDATE_MAX_TIME_LENGTH )
	{
		Warning( "Value passed to NetworkStateSetUpdateInterval (%.2f) too large.\n", val );
		val = AUTOUPDATE_MAX_TIME_LENGTH;
	}
	
	m_NSUpdateInterval = (unsigned short)( val * AUTOUPDATE_FREQ_SCALE ); 
	m_NSUpdateCounter = 0;
	
	// trigger an update immediately.
	m_bTimerElapsed = true;
}


void CNetStateMgr::EnableManualMode( bool bEnable )
{
	m_bUsingManualMode = !!bEnable;
}


EntityChange_t CNetStateMgr::DetectStateChanges()
{
	// No flags? It's in auto detect mode.
	if( m_NSUpdateInterval > 0 )
	{
		unsigned short elapsed = (unsigned short)( gpGlobals->frametime * AUTOUPDATE_FREQ_SCALE );
		if( elapsed >= m_NSUpdateCounter )
		{
			m_NSUpdateCounter = m_NSUpdateInterval;
			m_bTimerElapsed = true;
		}
		else
		{
			m_NSUpdateCounter -= elapsed;
		}
	}

	if ( g_bUseNetworkVars || m_bUsingManualMode )
	{
		if (m_bChanged)
			return ENTITY_CHANGE_AUTODETECT;
	}
	else
	{
		// If manual mode is off, then we always fire an event when the timer ticks.
		if (( m_NSUpdateInterval <= 0 ) || m_bTimerElapsed)
			return ENTITY_CHANGE_AUTODETECT;
	}

	return ENTITY_CHANGE_NONE;
}

// Called by the engine to indicate that its processed the state changes
void CNetStateMgr::ResetStateChanges()
{
	m_bTimerElapsed = false;
	m_bChanged = false;
}

