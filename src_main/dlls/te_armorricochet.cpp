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
// Purpose: Dispatches sparks
//-----------------------------------------------------------------------------
class CTEMetalSparks : public CBaseTempEntity
{
DECLARE_CLASS( CTEMetalSparks, CBaseTempEntity );

public:
					CTEMetalSparks( const char *name );
	virtual			~CTEMetalSparks( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecPos );
	CNetworkVector( m_vecDir );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEMetalSparks::CTEMetalSparks( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecPos.Init();
	m_vecDir.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEMetalSparks::~CTEMetalSparks( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEMetalSparks::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_vecPos = current_origin;
	
	AngleVectors( current_angles, &m_vecDir.GetForModify() );
	
	Vector forward;

	m_vecPos += Vector( 0, 0, 24 );

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecPos, 100.0, forward, m_vecPos.GetForModify() );
	
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
void CTEMetalSparks::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEMetalSparks, DT_TEMetalSparks)
	SendPropVector( SENDINFO(m_vecPos), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecDir), -1, SPROP_COORD),
END_SEND_TABLE()

// Singleton to fire TEMetalSparks objects
static CTEMetalSparks g_TEMetalSparks( "Metal Sparks" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			scale - 
//-----------------------------------------------------------------------------
void TE_MetalSparks( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir )
{
	g_TEMetalSparks.m_vecPos = *pos;
	g_TEMetalSparks.m_vecDir = *dir;

	// Send it over the wire
	g_TEMetalSparks.Create( filter, delay );
}

class CTEArmorRicochet : public CTEMetalSparks
{
DECLARE_CLASS( CTEArmorRicochet, CTEMetalSparks );

public:
	CTEArmorRicochet( const char *name ) : CTEMetalSparks(name) {}
	DECLARE_SERVERCLASS();
};

IMPLEMENT_SERVERCLASS_ST( CTEArmorRicochet, DT_TEArmorRicochet)
END_SEND_TABLE()

static CTEArmorRicochet g_TEArmorRicochet( "Armor Ricochet" );
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			scale - 
//-----------------------------------------------------------------------------
void TE_ArmorRicochet( IRecipientFilter& filter, float delay,
	const Vector* pos, const Vector* dir )
{
	g_TEArmorRicochet.m_vecPos = *pos;
	g_TEArmorRicochet.m_vecDir = *dir;

	// Send it over the wire
	g_TEArmorRicochet.Create( filter, delay );
}
