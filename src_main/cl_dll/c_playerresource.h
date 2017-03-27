//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================

#ifndef C_PLAYERRESOURCE_H
#define C_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "c_baseentity.h"

class C_PlayerResource : public C_BaseEntity
{
	DECLARE_CLASS( C_PlayerResource, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

					C_PlayerResource();
	virtual			~C_PlayerResource();
	virtual void	PreDataUpdate( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	// Player data access
	virtual char	*Get_Name( int iIndex );
	virtual int		Get_Ping( int iIndex );
	virtual int		Get_Packetloss( int iIndex );
	virtual int		Get_Score( int iIndex );
	virtual int		Get_Deaths( int iIndex );
	virtual bool	Get_Connected( int iIndex );

	// TF specific data access
	virtual int		Get_XPos( int iIndex )	{ return 0; };
	virtual int		Get_YPos( int iIndex )  { return 0; };
	virtual float	Get_KillTime( int iIndex )  { return 0.0; };
	virtual void	Clear_KillTime( int iIndex ) { return; };

public:
	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	char	m_szName[MAX_PLAYERS+1][ MAX_PLAYERNAME_LENGTH ];
	int		m_iPing[MAX_PLAYERS+1];
	int		m_iPacketloss[MAX_PLAYERS+1];
	int		m_iScore[MAX_PLAYERS+1];
	int		m_iDeaths[MAX_PLAYERS+1];
	bool	m_bConnected[MAX_PLAYERS+1];
};

extern C_PlayerResource *g_PR;

#endif // C_PLAYERRESOURCE_H
