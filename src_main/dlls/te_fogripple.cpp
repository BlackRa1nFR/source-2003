//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "basetempentity.h"

//-----------------------------------------------------------------------------
// Purpose: Dispatches fog ripple tempentity
//-----------------------------------------------------------------------------
class CTEFogRipple : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEFogRipple, CBaseTempEntity );

					CTEFogRipple( const char *name );
	virtual			~CTEFogRipple( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVector( m_vecVelocity );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEFogRipple::CTEFogRipple( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_vecVelocity.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEFogRipple::~CTEFogRipple( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEFogRipple::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_vecOrigin = current_origin;
	
	Vector forward, right;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward, &right, NULL );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin, 25.0, right, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//-----------------------------------------------------------------------------
void CTEFogRipple::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

IMPLEMENT_SERVERCLASS_ST(CTEFogRipple, DT_TEFogRipple)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecVelocity), 0, SPROP_NOSCALE),
END_SEND_TABLE()


// Singleton to fire TEFogRipple objects
static CTEFogRipple g_TEFogRipple( "FogRipple" );

void TE_FogRipple( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* velocity)
{
	g_TEFogRipple.m_vecOrigin	= *pos;
	g_TEFogRipple.m_vecVelocity	= *velocity;

	// Send it over the wire
	g_TEFogRipple.Create( filter, delay );
}