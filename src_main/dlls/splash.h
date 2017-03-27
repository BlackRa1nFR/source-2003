//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SPLASH_H
#define SPLASH_H
#ifdef _WIN32
#pragma once
#endif

#include "baseparticleentity.h"


class CSplash : public CBaseParticleEntity
{
	DECLARE_CLASS( CSplash, CBaseParticleEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CSplash(void);
	~CSplash(void);
	void				Spawn(void);
	void				SetEmit(bool bVal);
	static	CSplash*		CreateSplash(const Vector &vPosition);

	// Input handlers
	void InputSetSpawnRate( inputdata_t &inputdata );
	void InputSetSpeed( inputdata_t &inputdata );
	void InputSetNoise( inputdata_t &inputdata );
	void InputSetParticleLifetime( inputdata_t &inputdata );
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );

	int					DrawDebugTextOverlays(void);
public:
	// Effect parameters. These will assume default values but you can change them.
	color32			m_rgbaStartColor;		// Raw color values from WC
	color32			m_rgbaEndColor;
	CNetworkVector( m_vStartColor );			// Fade between these colors.
	CNetworkVector( m_vEndColor );

	CNetworkVar( float, m_flSpawnRate );			// How many particles per second.
	CNetworkVar( float, m_flParticleLifetime );	// How long do the particles live?
	CNetworkVar( float, m_flStopEmitTime );		// When do I stop emitting particles?
	CNetworkVar( float, m_flSpeed );				// Speed range.
	CNetworkVar( float, m_flSpeedRange );
	CNetworkVar( float, m_flWidthMin );			// Width range
	CNetworkVar( float, m_flWidthMax );	
	CNetworkVar( float, m_flNoise );
	CNetworkVar( int, m_nNumDecals );
	CNetworkVar( bool, m_bEmit );
};

#endif // SPLASH_H
