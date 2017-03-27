//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the player specific data that is sent only to the player
//			to whom it belongs.
//
// $NoKeywords: $
//=============================================================================

#ifndef C_PLAYERLOCALDATA_H
#define C_PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "vector.h"
#include "playernet_vars.h"

//-----------------------------------------------------------------------------
// Purpose: Player specific data ( sent only to local player, too )
//-----------------------------------------------------------------------------
class CPlayerLocalData
{
public:
	DECLARE_PREDICTABLE();
	DECLARE_CLASS_NOBASE( CPlayerLocalData );
	DECLARE_EMBEDDED_NETWORKVAR();

	CPlayerLocalData()
	{
		m_iv_vecPunchAngle.Setup( &m_vecPunchAngle.m_Value, LATCH_SIMULATION_VAR );
	}

	unsigned char			m_chAreaBits[32];	// Area visibility flags.

	int						m_iHideHUD;			// bitfields containing sections of the HUD to hide
	int						m_iFOV;				// field of view
	float					m_flFOVRate;		// rate at which the FOV changes
	int						m_iFOVStart;		// starting value of the FOV changing over time (client only)
	float					m_flFOVTime;		// starting time of the FOV zoom

	bool					m_bDucked;
	bool					m_bDucking;
	float					m_flDucktime;
	int						m_nStepside;
	float					m_flFallVelocity;
	int						m_nOldButtons;
	// Base velocity that was passed in to server physics so 
	//  client can predict conveyors correctly.  Server zeroes it, so we need to store here, too.
	Vector					m_vecClientBaseVelocity;  
	CNetworkQAngle( m_vecPunchAngle );		// auto-decaying view angle adjustment
	CInterpolatedVar< QAngle >	m_iv_vecPunchAngle;

	CNetworkQAngle( m_vecPunchAngleVel );		// velocity of auto-decaying view angle adjustment
	bool					m_bDrawViewmodel;
	bool					m_bWearingSuit;
	float					m_flStepSize;
	bool					m_bAllowAutoMovement;

	// 3d skybox
	sky3dparams_t			m_skybox3d;
	// wold fog
	fogparams_t				m_fog;
	// audio environment
	audioparams_t			m_audio;

};

#endif // C_PLAYERLOCALDATA_H
