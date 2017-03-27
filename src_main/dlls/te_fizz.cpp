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
// Purpose: Dispatches Fizz tempentity
//-----------------------------------------------------------------------------
class CTEFizz : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEFizz, CBaseTempEntity );

					CTEFizz( const char *name );
	virtual			~CTEFizz( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	virtual void	Precache( void );

	DECLARE_SERVERCLASS();

public:
	CNetworkVar( int, m_nEntity );
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nDensity );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEFizz::CTEFizz( const char *name ) :
	CBaseTempEntity( name )
{
	m_nEntity = 0;
	m_nModelIndex = 0;
	m_nDensity = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEFizz::~CTEFizz( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEFizz::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = engine->PrecacheModel( "sprites/bubble.vmt" );;
	m_nDensity = 200;
	m_nEntity = 1;

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTEFizz::Precache( void )
{
	engine->PrecacheModel( "sprites/bubble.vmt" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//-----------------------------------------------------------------------------
void CTEFizz::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

IMPLEMENT_SERVERCLASS_ST(CTEFizz, DT_TEFizz)
	SendPropInt( SENDINFO(m_nEntity), 11, SPROP_UNSIGNED ),
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropInt( SENDINFO(m_nDensity), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEFizz objects
static CTEFizz g_TEFizz( "Fizz" );

void TE_Fizz( IRecipientFilter& filter, float delay,
	const CBaseEntity *entity, int modelindex, int density )
{
	Assert( entity );

	g_TEFizz.m_nEntity		= ENTINDEX( (edict_t *)entity->edict() );
	g_TEFizz.m_nModelIndex	= modelindex;	
	g_TEFizz.m_nDensity		= density;

	// Send it over the wire
	g_TEFizz.Create( filter, delay );
}