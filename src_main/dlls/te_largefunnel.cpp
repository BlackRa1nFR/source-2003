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
#include "te_particlesystem.h"

extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud

//-----------------------------------------------------------------------------
// Purpose: Dispatches smoke tempentity
//-----------------------------------------------------------------------------
class CTELargeFunnel : public CTEParticleSystem
{
public:
	DECLARE_CLASS( CTELargeFunnel, CTEParticleSystem );
	DECLARE_SERVERCLASS();

					CTELargeFunnel( const char *name );
	virtual			~CTELargeFunnel( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

public:
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nReversed );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTELargeFunnel::CTELargeFunnel( const char *name ) :
	BaseClass( name )
{
	m_nModelIndex = 0;
	m_nReversed = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTELargeFunnel::~CTELargeFunnel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTELargeFunnel::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = g_sModelIndexSmoke;
	m_nReversed = 0;
	m_vecOrigin = current_origin;
	
	Vector forward, right;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward, &right, NULL );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin.Get(), 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin.Get(), 25.0, right, m_vecOrigin.GetForModify() );

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
void CTELargeFunnel::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

IMPLEMENT_SERVERCLASS_ST(CTELargeFunnel, DT_TELargeFunnel)
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropInt( SENDINFO(m_nReversed), 2, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TELargeFunnel objects
static CTELargeFunnel g_TELargeFunnel( "Large Funnel" );

void TE_LargeFunnel( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, int reversed )
{
	g_TELargeFunnel.m_vecOrigin		= *pos;
	g_TELargeFunnel.m_nModelIndex	= modelindex;	
	g_TELargeFunnel.m_nReversed		= reversed;

	// Send it over the wire
	g_TELargeFunnel.Create( filter, delay );
}