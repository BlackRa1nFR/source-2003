//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_cs_player.h"
#include "c_user_message_register.h"


#if defined( CCSPlayer )
	#undef CCSPlayer
#endif


void __MsgFunc_BarTime( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int theTime = READ_BYTE();
	
	// Reset the clock.
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( pPlayer )
	{
		pPlayer->m_ProgressBarTime = theTime;
		pPlayer->m_ProgressBarStartTime = engine->Time();
	}
}
USER_MESSAGE_REGISTER( BarTime );


IMPLEMENT_CLIENTCLASS_DT( C_CSPlayer, DT_CSPlayer, CCSPlayer )
	RecvPropInt( RECVINFO( m_iPlayerState ) ),
	RecvPropInt( RECVINFO( m_iAccount ) ),
	RecvPropInt( RECVINFO( m_bInBombZone ) ),
	RecvPropInt( RECVINFO( m_bInBuyZone ) ),
	RecvPropInt( RECVINFO( m_iClass ) ),
	RecvPropInt( RECVINFO( m_ArmorValue ) ),
	RecvPropFloat( RECVINFO( m_flEyeAnglesPitch ) )
END_RECV_TABLE()


C_CSPlayer::C_CSPlayer() : m_PlayerAnimState( this )
{
	m_ProgressBarTime = 0;
	m_ArmorValue = 0;
}


int C_CSPlayer::DrawModel( int flags )
{
	return BaseClass::DrawModel( flags );
}


void C_CSPlayer::AddEntity()
{
	// Update the animation data. It does the local check here so this works when using
	// a third-person camera (and we don't have valid player angles).
	if ( this == C_CSPlayer::GetLocalCSPlayer() )
		m_PlayerAnimState.Update( EyeAngles()[YAW], m_flEyeAnglesPitch );
	else
		m_PlayerAnimState.Update( GetAbsAngles()[YAW], m_flEyeAnglesPitch );

	BaseClass::AddEntity();
}


bool C_CSPlayer::IsShieldDrawn() const
{
	return false;
}


bool C_CSPlayer::HasShield() const
{
	return false;
}


C_CSPlayer* C_CSPlayer::GetLocalCSPlayer()
{
	return (C_CSPlayer*)C_BasePlayer::GetLocalPlayer();
}


CSPlayerState C_CSPlayer::State_Get() const
{
	return m_iPlayerState;
}


float C_CSPlayer::GetMinFOV() const
{
	// Min FOV for AWP.
	return 10;
}


int C_CSPlayer::GetAccount() const
{
	return m_iAccount;
}


int C_CSPlayer::PlayerClass() const
{
	return m_iClass;
}


bool C_CSPlayer::CanShowBuyMenu() const
{
	return m_bInBuyZone && State_Get() == STATE_JOINED;
}


bool C_CSPlayer::CanShowTeamMenu() const
{
	return true;
}


int C_CSPlayer::ArmorValue() const
{
	return m_ArmorValue;
}

const QAngle& C_CSPlayer::GetRenderAngles()
{
	return m_PlayerAnimState.GetRenderAngles();
}

