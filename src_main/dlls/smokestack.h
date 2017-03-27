//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the server side of a steam jet particle system entity.
//
// $NoKeywords: $
//=============================================================================

#ifndef SMOKESTACK_H
#define SMOKESTACK_H
#pragma	once

#include "baseparticleentity.h"

//==================================================
// CSmokeStack
//==================================================

class CSmokeStackLightInfo
{
public:
	DECLARE_CLASS_NOBASE( CSmokeStackLightInfo );
	DECLARE_SIMPLE_DATADESC();
	DECLARE_NETWORKVAR_CHAIN();

	CNetworkVector( m_vPos );
	CNetworkVector( m_vColor );
	CNetworkVar( float, m_flIntensity );
};

class CSmokeStack : public CBaseParticleEntity
{
public:
	DECLARE_CLASS( CSmokeStack, CBaseParticleEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

					CSmokeStack();
					~CSmokeStack();

	virtual void	Spawn( void );
	virtual void	Activate();
	virtual bool	KeyValue( const char *szKeyName, const char *szValue );

protected:

	// Input handlers.
	void	InputTurnOn(inputdata_t &data);
	void	InputTurnOff(inputdata_t &data);
	void	InputToggle(inputdata_t &data);

	void	RecalcWindVector();


// Stuff from the datatable.
public:
	CNetworkVar( float, m_SpreadSpeed );
	CNetworkVar( float, m_Speed );
	CNetworkVar( float, m_StartSize );
	CNetworkVar( float, m_EndSize );
	CNetworkVar( float, m_Rate );
	CNetworkVar( float, m_JetLength );	// Length of the jet. Lifetime is derived from this.

	CNetworkVar( int, m_bEmit );		// Emit particles?
	CNetworkVar( float, m_flBaseSpread );
	
	CNetworkVar( int, m_DirLightSource ); // 0=bottom, 1=top
	CNetworkVector( m_DirLightColor );
	CNetworkVar( float, m_flTwist );

	int				m_WindAngle;
	int				m_WindSpeed;
	CNetworkVector( m_vWind );		// m_vWind is just calculated from m_WindAngle and m_WindSpeed.

	enum {NUM_LIGHTS=2};
	CSmokeStackLightInfo		m_Lights[NUM_LIGHTS];
	CNetworkVar( int, m_nLights );

	bool			m_InitialState;
};

#endif // SMOKESTACK_H

