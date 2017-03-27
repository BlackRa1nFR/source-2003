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

//-----------------------------------------------------------------------------
// Purpose: Dispatches line
//-----------------------------------------------------------------------------
class CTEShowLine : public CTEParticleSystem
{
public:
	DECLARE_CLASS( CTEShowLine, CTEParticleSystem );
	DECLARE_SERVERCLASS();

					CTEShowLine( const char *name );
	virtual			~CTEShowLine( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );


public:
	CNetworkVector( m_vecEnd );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEShowLine::CTEShowLine( const char *name ) :
	BaseClass( name )
{
	m_vecEnd.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEShowLine::~CTEShowLine( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEShowLine::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_vecOrigin = current_origin;
	
	Vector forward, right;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward, &right, NULL );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 100.0, forward, m_vecEnd.GetForModify() );

	m_vecOrigin = m_vecEnd + right * -128;
	m_vecEnd += right * 128;

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
void CTEShowLine::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

IMPLEMENT_SERVERCLASS_ST( CTEShowLine, DT_TEShowLine)
	SendPropVector( SENDINFO(m_vecEnd), -1, SPROP_COORD),
END_SEND_TABLE()


// Singleton to fire TEShowLine objects
static CTEShowLine g_TEShowLine( "Show Line" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*start - 
//			*end - 
//-----------------------------------------------------------------------------
void TE_ShowLine( IRecipientFilter& filter, float delay,
	const Vector* start, const Vector* end )
{
	g_TEShowLine.m_vecOrigin = *start;
	g_TEShowLine.m_vecEnd = *end;	

	// Send it over the wire
	g_TEShowLine.Create( filter, delay );
}