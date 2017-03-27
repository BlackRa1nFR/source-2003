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
// Purpose: Dispatches decal tempentity
//-----------------------------------------------------------------------------
class CTEDecal : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEDecal, CBaseTempEntity );

					CTEDecal( const char *name );
	virtual			~CTEDecal( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVector( m_vecStart );
	CNetworkVar( int, m_nEntity );
	CNetworkVar( int, m_nHitbox );
	CNetworkVar( int, m_nIndex );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEDecal::CTEDecal( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_nEntity = 0;
	m_nIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEDecal::~CTEDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEDecal::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nEntity = 0;
	m_nIndex = 0;
	m_vecOrigin = current_origin;

	Vector vecEnd;
	
	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin, 1024.0, forward, vecEnd );

	trace_t tr;

	UTIL_TraceLine( m_vecOrigin, vecEnd, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	m_vecOrigin = tr.endpos;

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
void CTEDecal::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

IMPLEMENT_SERVERCLASS_ST(CTEDecal, DT_TEDecal)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecStart), -1, SPROP_COORD),
	SendPropInt( SENDINFO(m_nEntity), 11, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nHitbox), 11, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nIndex), 9, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEDecal objects
static CTEDecal g_TEDecal( "Entity Decal" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			entity - 
//			index - 
//-----------------------------------------------------------------------------
void TE_Decal( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* start, int entity, int hitbox, int index )
{
	Assert( pos && start );
	g_TEDecal.m_vecOrigin	= *pos;
	g_TEDecal.m_vecStart	= *start;
	g_TEDecal.m_nEntity		= entity;	
	g_TEDecal.m_nHitbox		= hitbox;
	g_TEDecal.m_nIndex		= index;

	// Send it over the wire
	g_TEDecal.Create( filter, delay );
}