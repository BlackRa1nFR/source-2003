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
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "basetempentity.h"
#include "vstdlib/strtools.h"

//-----------------------------------------------------------------------------
// Purpose: Dispatches Text Message
//-----------------------------------------------------------------------------
class CTETextMessage : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTETextMessage, CBaseTempEntity );

					CTETextMessage( const char *name );
	virtual			~CTETextMessage( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	DECLARE_SERVERCLASS();

public:
	CNetworkVar( int, m_nChannel );
	CNetworkVar( float, x );
	CNetworkVar( float, y );
	CNetworkVar( int, m_nEffects );
	
	CNetworkVar( int, r1 );
	CNetworkVar( int, g1 );
	CNetworkVar( int, b1 );
	CNetworkVar( int, a1 );

	CNetworkVar( int, r2 );
	CNetworkVar( int, g2 );
	CNetworkVar( int, b2 );
	CNetworkVar( int, a2 );

	CNetworkVar( float, m_fFadeInTime );
	CNetworkVar( float, m_fFadeOutTime );
	CNetworkVar( float, m_fHoldTime );
	CNetworkVar( float, m_fFxTime );
	char			m_szMessage[ 150 ];
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTETextMessage::CTETextMessage( const char *name ) :
	CBaseTempEntity( name )
{
	m_nChannel = 0;
	x = 0.0;
	y = 0.0;
	m_nEffects = 0;
	r1 = g1 = b1 = a1 = 0;
	r2 = g2 = b2 = a2 = 0;
	m_fFadeInTime = 0.0;
	m_fFadeOutTime = 0.0;
	m_fHoldTime = 0.0;
	m_fFxTime = 0.0;
	memset( m_szMessage, 0, sizeof( m_szMessage ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTETextMessage::~CTETextMessage( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTETextMessage::Test( const Vector& current_origin, const QAngle& current_angles )
{
	m_nChannel = 0;
	x = 0.2;
	y = 0.5;
	m_nEffects = 0;
	r1 = 100;
	g1 = 100;
	b1 = 200;
	a1 = 127;
	r2 = 200;
	g2 = 0;
	b2 = 100;
	a2 = 127;
	m_fFadeInTime = 1.0;
	m_fFadeOutTime = 1.0;
	m_fHoldTime = 1.0;
	m_fFxTime = 2.0;

	Q_strncpy( m_szMessage, "Testing CTETextMessage" ,sizeof(m_szMessage));

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
void CTETextMessage::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

IMPLEMENT_SERVERCLASS_ST( CTETextMessage, DT_TETextMessage)
	SendPropInt( SENDINFO(m_nChannel), 8, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(x ), 16, SPROP_ROUNDDOWN, -1.0, 4.0 ),
	SendPropFloat( SENDINFO(y ), 16, SPROP_ROUNDDOWN, -1.0, 4.0 ),
	SendPropInt( SENDINFO(m_nEffects), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(r1), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(g1), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(b1), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(a1), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(r2), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(g2), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(b2), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(a2), 8, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_fFadeInTime ), 16, SPROP_ROUNDDOWN, 0.0, 256.0 ),
	SendPropFloat( SENDINFO(m_fFadeOutTime ), 16, SPROP_ROUNDDOWN, 0.0, 256.0 ),
	SendPropFloat( SENDINFO(m_fHoldTime ), 16, SPROP_ROUNDDOWN, 0.0, 256.0 ),
	SendPropFloat( SENDINFO(m_fFxTime ), 16, SPROP_ROUNDDOWN, 0.0, 256.0 ),
	SendPropString( SENDINFO_NOCHECK(m_szMessage)),
END_SEND_TABLE()


// Singleton to fire TETextMessage objects
static CTETextMessage g_TETextMessage( "TextMessage" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*tp - 
//			*pMessage - 
//-----------------------------------------------------------------------------
void TE_TextMessage( IRecipientFilter& filter, float delay,
	const struct hudtextparms_s *tp, const char *pMessage  )
{
	g_TETextMessage.m_nChannel = tp->channel & 0xFF;
	g_TETextMessage.x = tp->x;
	g_TETextMessage.y = tp->y;
	g_TETextMessage.m_nEffects = tp->effect;
	g_TETextMessage.r1 = tp->r1;
	g_TETextMessage.g1 = tp->g1;
	g_TETextMessage.b1 = tp->b1;
	g_TETextMessage.a1 = tp->a1;
	g_TETextMessage.r2 = tp->r2;
	g_TETextMessage.g2 = tp->g2;
	g_TETextMessage.b2 = tp->b2;
	g_TETextMessage.a2 = tp->a2;
	g_TETextMessage.m_fFadeInTime = tp->fadeinTime;
	g_TETextMessage.m_fFadeOutTime = tp->fadeoutTime;
	g_TETextMessage.m_fHoldTime = tp->holdTime;
	g_TETextMessage.m_fFxTime = tp->fxTime;
	
	Q_strncpy( g_TETextMessage.m_szMessage, pMessage, sizeof(g_TETextMessage.m_szMessage) );

	// Send it over the wire
	g_TETextMessage.Create( filter, delay );
}