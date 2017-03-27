/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  hud_msg.cpp
//
#include "cbase.h"
#include "parsemsg.h"
#include "hudelement.h"

/// USER-DEFINED SERVER MESSAGE HANDLERS

void CHud::MsgFunc_ResetHUD( const char *pszName, int iSize, void *pbuf )
{
	// clear all hud data
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->Reset();
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;
	m_flMouseSensitivityFactor = 0;
}

void CHud::MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
}

void CHud::MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_bTeamplay = READ_BYTE() ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHud::MsgFunc_TeamChange(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	int iNewTeam = READ_BYTE();

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		pLocalPlayer->TeamChange( iNewTeam );
	}
}
