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
// Purpose: Dispatches blood stream tempentity
//-----------------------------------------------------------------------------
class CTEStickyBolt : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEStickyBolt, CBaseTempEntity );

					CTEStickyBolt( const char *name );
	virtual			~CTEStickyBolt( void );
	
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	DECLARE_SERVERCLASS();

public:
	CNetworkVar( int, m_nEntIndex );
	CNetworkVector(	m_vecOrigin );
	CNetworkVector( m_vecDirection );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEStickyBolt::CTEStickyBolt( const char *name ) :
	CBaseTempEntity( name )
{
	m_nEntIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEStickyBolt::~CTEStickyBolt( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//-----------------------------------------------------------------------------
void CTEStickyBolt::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

IMPLEMENT_SERVERCLASS_ST(CTEStickyBolt, DT_TEStickyBolt)
	SendPropInt( SENDINFO(m_nEntIndex), 5, SPROP_UNSIGNED ),
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecDirection), -1, SPROP_COORD),
END_SEND_TABLE()


// Singleton to fire StickyBolt objects
static CTEStickyBolt g_TEStickyBolt( "StickyBolt" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			player - 
//-----------------------------------------------------------------------------
void TE_StickyBolt( IRecipientFilter& filter, float delay,
	int iIndex, Vector vecDirection, const Vector * origin )
{
	g_TEStickyBolt.m_nEntIndex = iIndex;
	g_TEStickyBolt.m_vecDirection = vecDirection;
	g_TEStickyBolt.m_vecOrigin = *origin;
	
	// Send it over the wire
	g_TEStickyBolt.Create( filter, delay );
}