//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose: Simple, small, free-standing tools for building AIs
//
//=============================================================================

#ifndef AI_UTILS_H
#define AI_UTILS_H

#include "simtimer.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
//
// CAI_MoveMonitor
//
// Purpose: Watch an entity, trigger if moved more than a tolerance
//
//-----------------------------------------------------------------------------

class CAI_MoveMonitor
{
public:
	CAI_MoveMonitor()
	 : m_vMark( 0, 0, 0 ),
	   m_flMarkTolerance( NO_MARK )
	{
	}
	
	void SetMark( CBaseEntity *pEntity, float tolerance )
	{
		m_vMark = pEntity->GetAbsOrigin();
		m_flMarkTolerance = tolerance;
	}
	
	void ClearMark()
	{
	   m_flMarkTolerance = NO_MARK;
	}

	bool IsMarkSet()
	{
		return ( m_flMarkTolerance != NO_MARK );
	}

	bool TargetMoved( CBaseEntity *pEntity )
	{
		if ( IsMarkSet() && pEntity != NULL )
		{
			float distance = ( m_vMark - pEntity->GetAbsOrigin() ).Length();
			if ( distance > m_flMarkTolerance )
				return true;
		}
		return false;
	}
	
private:
	enum
	{
		NO_MARK = -1
	};
	
	Vector			   m_vMark;
	float			   m_flMarkTolerance;

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------
//
// CAI_ShotRegulator
//
// Purpose: Assists in creating non-constant bursty shooting style
//
//-----------------------------------------------------------------------------

class CAI_ShotRegulator
{
public:
	CAI_ShotRegulator()
	 :	m_nShotsToTake(0),
		m_nMinShots(1), 
		m_nMaxShots(1)
	{
	}

	void SetParameters( int minShotsPerBurst, int maxShotsPerBurst, float minRestTime, float maxRestTime = 0.0 )
	{
		m_NextShotTimer.Set( minRestTime, maxRestTime );
		m_nMinShots = minShotsPerBurst;
		m_nMaxShots = maxShotsPerBurst;
		m_nShotsToTake = m_nMinShots + random->RandomInt( 0, m_nMaxShots - m_nMinShots );
	}

	void Reset( bool bStartShooting = true )
	{
		if ( bStartShooting )
		{
			m_nShotsToTake = m_nMinShots + random->RandomInt( 0, m_nMaxShots - m_nMinShots );
		}
		else
		{
			m_NextShotTimer.Reset();
			m_nShotsToTake = 0;
		}
	}

	bool ShouldShoot()
	{ 
		return ( m_nShotsToTake != 0 ); 
	}

	void Update()
	{
		if ( m_nShotsToTake == 0 && m_NextShotTimer.Expired() )
			m_nShotsToTake = m_nMinShots + random->RandomInt( 0, m_nMaxShots - m_nMinShots );
	}

	int GetShots()				
	{ 
		return m_nShotsToTake; 
	}

	void SetShots( int shots )	
	{ 
		if ( m_nShotsToTake != 0 && shots == 0 )
			m_NextShotTimer.Reset();
		m_nShotsToTake = shots; 
	}

	void OnFiredWeapon()
	{
		Assert( m_nShotsToTake > 0 );
		m_nShotsToTake--;
		if ( m_nShotsToTake == 0 )
			m_NextShotTimer.Reset();
	}

private:
	CRandSimTimer	m_NextShotTimer;
	int				m_nShotsToTake;
	int				m_nMinShots, m_nMaxShots;

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------

#endif // AI_UTILS_H
