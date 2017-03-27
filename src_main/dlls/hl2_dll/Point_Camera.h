//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CAMERA_H
#define CAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointCamera : public CBaseEntity
{
public:
	DECLARE_CLASS( CPointCamera, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	CPointCamera();

	void Spawn( void );

	// Tell the client that this camera needs to be rendered
	void SetActive( bool bActive );
	bool ShouldTransmit( const edict_t *recipient, 
						           const void *pvs, int clientArea );

	void ChangeFOVThink( void );

	void InputChangeFOV( inputdata_t &inputdata );
	void InputSetOnAndTurnOthersOff( inputdata_t &inputdata );
	void InputSetOn( inputdata_t &inputdata );
	void InputSetOff( inputdata_t &inputdata );

private:
	float m_TargetFOV;
	float m_DegreesPerSecond;

	CNetworkVar( float, m_FOV );
	CNetworkVar( float, m_Resolution );
	CNetworkVar( bool, m_bActive );
	bool	m_bLastActive;

	// Allows the mapmaker to control whether a camera is active or not
	bool	m_bIsOn;
};

#endif // CAMERA_H
