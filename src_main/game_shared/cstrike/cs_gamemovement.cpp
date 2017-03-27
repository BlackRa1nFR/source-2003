//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "gamemovement.h"
#include "cs_gamerules.h"

#ifdef CLIENT_DLL
	#include "c_cs_player.h"
#else
	#include "cs_player.h"
#endif


class CCSGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS( CCSGameMovement, CGameMovement );

	virtual float GetSpeedCropFraction() const;
	virtual void ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove );
	virtual bool CanAccelerate();


public:
	CCSPlayer *m_pCSPlayer;
};


// Expose our interface.
static CCSGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = ( IGameMovement * )&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement,INTERFACENAME_GAMEMOVEMENT, g_GameMovement );


// ---------------------------------------------------------------------------------------- //
// CCSGameMovement.
// ---------------------------------------------------------------------------------------- //

float CCSGameMovement::GetSpeedCropFraction() const
{
	return 0.52;
}


void CCSGameMovement::ProcessMovement( CBasePlayer *pBasePlayer, CMoveData *pMove )
{
	m_pCSPlayer = ToCSPlayer( pBasePlayer );
	Assert( m_pCSPlayer );

	if ( m_pCSPlayer->CanMove() )
	{
		BaseClass::ProcessMovement( pBasePlayer, pMove );
	}
}


bool CCSGameMovement::CanAccelerate()
{
	// Only allow the player to accelerate when in certain states.
	CSPlayerState curState = m_pCSPlayer->State_Get();
	if ( curState == STATE_JOINED || curState == STATE_OBSERVER_MODE )
	{
		return player->GetWaterJumpTime() == 0;
	}
	else
	{	
		return false;
	}
}

