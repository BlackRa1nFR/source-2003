//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#ifndef AI_MOVESHOOT_H
#define AI_MOVESHOOT_H

#include "ai_component.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// @TODO (toml 07-09-03): probably want to fold this into base NPC. evaluate when
// head above water

class CAI_MoveAndShootOverlay : public CAI_Component
{
	typedef CAI_Component BaseClass;

public:
	CAI_MoveAndShootOverlay()
	 :	m_bMovingAndShooting(false),
		m_nMoveShots(0),
		m_flNextMoveShootTime(0),
		m_minBurst(0),
		m_maxBurst(0),
		m_minPause(0),
		m_maxPause(0)
	{
	}

	void StartShootWhileMove( float minPause = 0.3, float maxPause = 0.5 );
	void NoShootWhileMove();
	void RunShootWhileMove();
	void EndShootWhileMove();

private:
	bool CanAimAtEnemy();
	void UpdateMoveShootActivity( bool bMoveAimAtEnemy );

	bool	m_bMovingAndShooting;
	int		m_nMoveShots;
	float	m_flNextMoveShootTime;
	
	int		m_minBurst;
	int		m_maxBurst;
	float	m_minPause;
	float	m_maxPause;

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------

#endif // AI_MOVESHOOT_H
