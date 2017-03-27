//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "player.h"
#include "player_resource.h"

// Datatable
IMPLEMENT_SERVERCLASS_ST_NOBASE(CPlayerResource, DT_PlayerResource)
	//SendPropArray( SendPropString( SENDINFO(m_szName[0]) ), SENDARRAYINFO(m_szName) ),
	SendPropArray( SendPropInt( SENDINFO_ARRAY(m_iPing), 10, SPROP_UNSIGNED ), m_iPing ),
	SendPropArray( SendPropInt( SENDINFO_ARRAY(m_iPacketloss), 8, SPROP_UNSIGNED ), m_iPacketloss ),
	SendPropArray( SendPropInt( SENDINFO_ARRAY(m_iScore), 12 ), m_iScore ),
	SendPropArray( SendPropInt( SENDINFO_ARRAY(m_iDeaths), 12 ), m_iDeaths ),
	SendPropArray( SendPropInt( SENDINFO_ARRAY(m_bConnected), 1, SPROP_UNSIGNED ), m_bConnected ),
END_SEND_TABLE()

BEGIN_DATADESC( CPlayerResource )

	// DEFINE_ARRAY( CPlayerResource, m_iPing, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( CPlayerResource, m_iPacketloss, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( CPlayerResource, m_iScore, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( CPlayerResource, m_iDeaths, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( CPlayerResource, m_bConnected, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_FIELD( CPlayerResource, m_flNextPingUpdate, FIELD_FLOAT ),

	// Function Pointers
	DEFINE_FUNCTION( CPlayerResource, ResourceThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( player_manager, CPlayerResource );

CPlayerResource *g_pPlayerResource;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::Spawn( void )
{
	for ( int i=0; i < MAX_PLAYERS+1; i++ )
	{
		m_iPing.Set( i, 0 );
		m_iPacketloss.Set( i, 0 );
		m_iScore.Set( i, 0 );
		m_iDeaths.Set( i, 0 );
		m_bConnected.Set( i, 0 );
	}

	SetThink( &CPlayerResource::ResourceThink );
	SetNextThink( gpGlobals->curtime + 1.0f );
	m_flNextPingUpdate = gpGlobals->curtime + 1.0;
}

//-----------------------------------------------------------------------------
// Purpose: The Player resource is always transmitted to clients
//-----------------------------------------------------------------------------
bool CPlayerResource::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper for the virtual GrabPlayerData Think function
//-----------------------------------------------------------------------------
void CPlayerResource::ResourceThink( void )
{
	GrabPlayerData();

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::GrabPlayerData( void )
{
	// Test Hack
	// Should use a dirty flag instead of polling (although ping/packetloss should poll)
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		// Assume gone
		m_bConnected.Set( i, false );

		CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer  )
		{
			m_iScore.Set( i, pPlayer->FragCount() );
			m_iDeaths.Set( i, pPlayer->DeathCount() );
			m_bConnected.Set( i, pPlayer->IsConnected() );

			// Don't update ping / packetloss everytime
			if ( m_flNextPingUpdate < gpGlobals->curtime )
			{
				UTIL_GetPlayerConnectionInfo( i, m_iPing.GetForModify( i ), m_iPacketloss.GetForModify( i ) );
				// Don't overflow
				m_iPing.Set( i, min( m_iPing[ i ], 1023 ) );
			}
		}
	}

	// Update ping / packetloss every 2 seconds
	if ( m_flNextPingUpdate < gpGlobals->curtime )
	{
		m_flNextPingUpdate = gpGlobals->curtime + 2.0;
	}
}
