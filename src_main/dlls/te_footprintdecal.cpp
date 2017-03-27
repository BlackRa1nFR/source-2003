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
// Purpose: Dispatches footprint decal tempentity
//-----------------------------------------------------------------------------

#define FOOTPRINT_DECAY_TIME 3.0f

class CTEFootprintDecal : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEFootprintDecal, CBaseTempEntity );

					CTEFootprintDecal( const char *name );
	virtual			~CTEFootprintDecal( void );

	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVector( m_vecDirection );											
	CNetworkVar( int, m_nEntity );
	CNetworkVar( int, m_nIndex );
	CNetworkVar( unsigned char, m_chMaterialType );
};

IMPLEMENT_SERVERCLASS_ST(CTEFootprintDecal, DT_TEFootprintDecal)
	SendPropVector( SENDINFO(m_vecOrigin),		-1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecDirection),	-1, SPROP_COORD),
	SendPropInt(  SENDINFO(m_nEntity),			11, SPROP_UNSIGNED ),
	SendPropInt(  SENDINFO(m_nIndex),			8,	SPROP_UNSIGNED ),
	SendPropInt(   SENDINFO(m_chMaterialType),	8,	SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEFootprintDecal objects
static CTEFootprintDecal g_TEFootprintDecal( "Footprint Decal" );

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CTEFootprintDecal::CTEFootprintDecal( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_nEntity = 0;
	m_nIndex = 0;
	m_chMaterialType = 'C';
}

CTEFootprintDecal::~CTEFootprintDecal( void )
{
}

//-----------------------------------------------------------------------------
// Creates decals
//-----------------------------------------------------------------------------

void CTEFootprintDecal::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

//-----------------------------------------------------------------------------
// places a footprint decal 
//-----------------------------------------------------------------------------
								    
void TE_FootprintDecal( IRecipientFilter& filter, float delay, 
					    const Vector *origin, const Vector *right, int entity, int index, 
					    unsigned char materialType )
{
	Assert( origin );
	g_TEFootprintDecal.m_vecOrigin = *origin;
	g_TEFootprintDecal.m_vecDirection	= *right;
	g_TEFootprintDecal.m_nEntity		= entity;	
	g_TEFootprintDecal.m_nIndex			= index;
	g_TEFootprintDecal.m_chMaterialType	= materialType;

	VectorNormalize(g_TEFootprintDecal.m_vecDirection.GetForModify());

	// Send it over the wire
	g_TEFootprintDecal.Create( filter, delay );
}