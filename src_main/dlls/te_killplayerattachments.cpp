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
class CTEKillPlayerAttachments : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEKillPlayerAttachments, CBaseTempEntity );

					CTEKillPlayerAttachments( const char *name );
	virtual			~CTEKillPlayerAttachments( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	DECLARE_SERVERCLASS();

public:
	CNetworkVar( int, m_nPlayer );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEKillPlayerAttachments::CTEKillPlayerAttachments( const char *name ) :
	CBaseTempEntity( name )
{
	m_nPlayer = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEKillPlayerAttachments::~CTEKillPlayerAttachments( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEKillPlayerAttachments::Test( const Vector& current_origin, const QAngle& current_angles )
{
	m_nPlayer = 1;

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
void CTEKillPlayerAttachments::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}

IMPLEMENT_SERVERCLASS_ST(CTEKillPlayerAttachments, DT_TEKillPlayerAttachments)
	SendPropInt( SENDINFO(m_nPlayer), 5, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEKillPlayerAttachments objects
static CTEKillPlayerAttachments g_TEKillPlayerAttachments( "KillPlayerAttachments" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			player - 
//-----------------------------------------------------------------------------
void TE_KillPlayerAttachments( IRecipientFilter& filter, float delay,
	int player )
{
	g_TEKillPlayerAttachments.m_nPlayer = player;

	// Send it over the wire
	g_TEKillPlayerAttachments.Create( filter, delay );
}