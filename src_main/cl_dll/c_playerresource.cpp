//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_playerresource.h"

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_PlayerResource, DT_PlayerResource, CPlayerResource)
	RecvPropArray( RecvPropInt( RECVINFO(m_iPing[0])), m_iPing),
	RecvPropArray( RecvPropInt( RECVINFO(m_iPacketloss[0])), m_iPacketloss),
	RecvPropArray( RecvPropInt( RECVINFO(m_iScore[0])), m_iScore),
	RecvPropArray( RecvPropInt( RECVINFO(m_iDeaths[0])), m_iDeaths),
	RecvPropArray( RecvPropInt( RECVINFO(m_bConnected[0])), m_bConnected),
END_RECV_TABLE()

C_PlayerResource *g_PR;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::C_PlayerResource()
{
	memset( m_szName, 0, sizeof( m_szName ) );
	memset( m_iPing, 0, sizeof( m_iPing ) );
	memset( m_iPacketloss, 0, sizeof( m_iPacketloss ) );
	memset( m_iScore, 0, sizeof( m_iScore ) );
	memset( m_iDeaths, 0, sizeof( m_iDeaths ) );
	memset( m_bConnected, 0, sizeof( m_bConnected ) );

	g_PR = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::~C_PlayerResource()
{
	g_PR = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PlayerResource::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: Update global map state based on data received
//-----------------------------------------------------------------------------
void C_PlayerResource::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// HACK: Get the player's names from the engine. These should be moved into the datatable.
	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		if ( m_bConnected[i] )
		{
			hud_player_info_t sPlayerInfo;
			engine->GetPlayerInfo( i, &sPlayerInfo );
			if( sPlayerInfo.name )
			{
				Q_strncpy( m_szName[i], sPlayerInfo.name, MAX_PLAYERNAME_LENGTH );
			}
			else
			{
				m_szName[i][0] = 0;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char *C_PlayerResource::Get_Name( int iIndex )
{
	if ( !Get_Connected( iIndex ) )
		return "unconnected";

	// Yuck, make sure it's up to date
	hud_player_info_t sPlayerInfo;
	engine->GetPlayerInfo( iIndex, &sPlayerInfo );
	if( sPlayerInfo.name )
	{
		Q_strncpy( m_szName[iIndex], sPlayerInfo.name, MAX_PLAYERNAME_LENGTH );
	}
	return m_szName[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::Get_Ping( int iIndex )
{
	if ( !Get_Connected( iIndex ) )
		return 0;

	return m_iPing[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::Get_Packetloss( int iIndex )
{
	if ( !Get_Connected( iIndex ) )
		return 0;

	return m_iPacketloss[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::Get_Score( int iIndex )
{
	if ( !Get_Connected( iIndex ) )
		return 0;

	return m_iScore[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::Get_Deaths( int iIndex )
{
	if ( !Get_Connected( iIndex ) )
		return 0;

	return m_iDeaths[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::Get_Connected( int iIndex )
{
	return m_bConnected[iIndex];
}
