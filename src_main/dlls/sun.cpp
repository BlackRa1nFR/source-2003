//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "baseentity.h"
#include "sendproxy.h"


class CSun : public CBaseEntity
{
public:
	DECLARE_CLASS( CSun, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CSun();

	virtual void	Activate();

	// Input handlers
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputSetColor( inputdata_t &inputdata );

public:
	CNetworkVector( m_vDirection );
	
	CNetworkVar( int, m_nLayers );

	CNetworkVar( int, m_HorzSize );
	CNetworkVar( int, m_VertSize );

	CNetworkVar( int, m_bOn );
};



IMPLEMENT_SERVERCLASS_ST_NOBASE( CSun, DT_Sun )
	SendPropInt( SENDINFO(m_clrRender), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ),
	SendPropVector( SENDINFO(m_vDirection), 0, SPROP_NORMAL ),
	
	SendPropInt( SENDINFO(m_nLayers),  3,  SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_bOn), 1, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO(m_HorzSize), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_VertSize), 10, SPROP_UNSIGNED )
END_SEND_TABLE()


LINK_ENTITY_TO_CLASS( env_sun, CSun );


BEGIN_DATADESC( CSun )

	DEFINE_FIELD( CSun,	m_vDirection,		FIELD_VECTOR ),
	
	DEFINE_KEYFIELD( CSun, m_nLayers,		FIELD_INTEGER,	"NumLayers" ),
	DEFINE_KEYFIELD( CSun, m_HorzSize,		FIELD_INTEGER,	"HorzSize" ),
	DEFINE_KEYFIELD( CSun, m_VertSize,		FIELD_INTEGER,	"VertSize" ),

	DEFINE_FIELD( CSun, m_bOn, FIELD_INTEGER ),

	DEFINE_INPUTFUNC( CSun, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CSun, FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( CSun, FIELD_COLOR32, "SetColor", InputSetColor )

END_DATADESC()



CSun::CSun()
{
	m_vDirection.Init( 0, 0, 1 );
	NetworkStateManualMode( true );
	m_bOn = true;
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}


void CSun::Activate()
{
	BaseClass::Activate();
	
	// Find our target.
	CBaseEntity *pEnt = gEntList.FindEntityByName( 0, m_target, NULL );
	if( pEnt )
	{
		Vector vDirection = GetAbsOrigin() - pEnt->GetAbsOrigin();
		VectorNormalize( vDirection );
		m_vDirection = vDirection;
	}

	NetworkStateChanged();
}


void CSun::InputTurnOn( inputdata_t &inputdata )
{
	if( !m_bOn )
	{
		m_bOn = true;
		NetworkStateChanged();
	}
}


void CSun::InputTurnOff( inputdata_t &inputdata )
{
	if( m_bOn )
	{
		m_bOn = false;
		NetworkStateChanged();
	}
}


void CSun::InputSetColor( inputdata_t &inputdata )
{
	m_clrRender = inputdata.value.Color32();
	NetworkStateChanged();
}



