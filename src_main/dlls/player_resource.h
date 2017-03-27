//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================

#ifndef PLAYER_RESOURCE_H
#define PLAYER_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class CPlayerResource : public CBaseEntity
{
	DECLARE_CLASS( CPlayerResource, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void ResourceThink( void );
	virtual void GrabPlayerData( void );
	virtual bool ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );

public:
	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	//char	m_szName[MAX_PLAYERS][ MAX_PLAYERNAME_LENGTH ];
	CNetworkArray( int, m_iPing, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iPacketloss, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iScore, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iDeaths, MAX_PLAYERS+1 );
	CNetworkArray( int, m_bConnected, MAX_PLAYERS+1 );

	float	m_flNextPingUpdate;
};

extern CPlayerResource *g_pPlayerResource;

#endif // PLAYER_RESOURCE_H
