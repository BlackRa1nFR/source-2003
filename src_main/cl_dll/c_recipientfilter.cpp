//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "prediction.h"

static IPredictionSystem g_RecipientFilterPredictionSystem;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_RecipientFilter::C_RecipientFilter()
{
	m_pEntityMessageName = NULL;
	Reset();
}

C_RecipientFilter::~C_RecipientFilter()
{
	delete[] m_pEntityMessageName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_RecipientFilter::Reset( void )
{
	m_bReliable			= false;
	m_bInitMessage		= false;
	m_Type				= NORMAL;
	m_nEntityIndex		= -1;
	m_nTruePlayerCount	= -1;
	m_Recipients.RemoveAll();
	m_bUsingPredictionRules = false;
	m_bIgnorePredictionCull = false;
	delete[] m_pEntityMessageName;
	m_pEntityMessageName = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get a count of all possible players
//-----------------------------------------------------------------------------
void C_RecipientFilter::EnsureTruePlayerCount( void )
{
	if ( m_nTruePlayerCount != -1 )
		return;

	m_nTruePlayerCount = 1;
}

void C_RecipientFilter::MakeReliable( void )
{
	m_bReliable = true;
}

bool C_RecipientFilter::IsReliable( void ) const
{
	return m_bReliable;
}

int C_RecipientFilter::GetRecipientCount( void ) const
{
	return m_Recipients.Size();
}

int	C_RecipientFilter::GetRecipientIndex( int slot ) const
{
	if ( slot < 0 || slot >= GetRecipientCount() )
		return -1;

	return m_Recipients[ slot ];
}

void C_RecipientFilter::AddAllPlayers( void )
{
	if ( !C_BasePlayer::GetLocalPlayer() )
		return;

	m_Recipients.RemoveAll();
	AddRecipient( C_BasePlayer::GetLocalPlayer() );
}

void C_RecipientFilter::AddRecipient( C_BasePlayer *player )
{
	Assert( player );

	int index = player->index;

	// If we're predicting and this is not the first time we've predicted this sound
	//  then don't send it to the local player again.
	if ( m_bUsingPredictionRules )
	{
		Assert( player == C_BasePlayer::GetLocalPlayer() );
		Assert( prediction->InPrediction() );

		// Only add local player if this is the first time doing prediction
		if ( !g_RecipientFilterPredictionSystem.CanPredict() )
		{
			return;
		}
	}

	// Already in list
	if ( m_Recipients.Find( index ) != m_Recipients.InvalidIndex() )
		return;

	if ( index != engine->GetPlayer() )
		return;

	m_Recipients.AddToTail( index );
}

void C_RecipientFilter::RemoveRecipient( C_BasePlayer *player )
{
	if ( !player )
		return;

	int index = player->index;

	// Remove it if it's in the list
	m_Recipients.FindAndRemove( index );
}

void C_RecipientFilter::AddRecipientsByTeam( C_Team *team )
{
	AddAllPlayers();
}

void C_RecipientFilter::RemoveRecipientsByTeam( C_Team *team )
{
}

void C_RecipientFilter::AddPlayersFromBitMask( unsigned int playerbits )
{
	if ( !( playerbits & (1<<engine->GetPlayer()) ) )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	AddRecipient( pPlayer );
}

void C_RecipientFilter::AddRecipientsByPVS( const Vector& origin )
{
	AddAllPlayers();
}

void C_RecipientFilter::AddRecipientsByPAS( const Vector& origin )
{
	AddAllPlayers();
}

void C_RecipientFilter::SetEntityMessage( int entity, char const *msgname )
{
	Assert( msgname );
	m_Type = ENTITY;
	m_nEntityIndex = entity;
	delete[] m_pEntityMessageName;
	m_pEntityMessageName = new char[ Q_strlen( msgname ) + 1 ];
	Assert( m_pEntityMessageName );
	Q_strcpy( m_pEntityMessageName, msgname );
}

bool C_RecipientFilter::IsEntityMessage( void ) const
{
	return ( m_Type == ENTITY ) ? true : false;
}

int C_RecipientFilter::GetEntityIndex( void ) const
{
	return m_nEntityIndex;
}

char const *C_RecipientFilter::GetEntityMessageName( void ) const
{
	Assert( IsEntityMessage() );
	Assert( m_pEntityMessageName );
	return m_pEntityMessageName ? m_pEntityMessageName : "";
}

bool C_RecipientFilter::IsBroadcastMessage( void )
{
	EnsureTruePlayerCount();
	if ( m_nTruePlayerCount == m_Recipients.Count() )
		return true;

	return false;
}

bool C_RecipientFilter::IsInitMessage( void ) const
{
	return m_bInitMessage;
}

void C_RecipientFilter::MakeInitMessage( void )
{
	m_bInitMessage = true;
}

void C_RecipientFilter::UsePredictionRules( void )
{
	if ( m_bUsingPredictionRules )
		return;

	if ( !prediction->InPrediction() )
	{
		Assert( 0 );
		return;
	}

	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( !local )
	{
		Assert( 0 );
		return;
	}

	m_bUsingPredictionRules = true;

	// Cull list now, if needed
	int c = GetRecipientCount();
	if ( c == 0 )
		return;

	if ( !g_RecipientFilterPredictionSystem.CanPredict() )
	{
		RemoveRecipient( local );
	}
}

bool C_RecipientFilter::IsUsingPredictionRules( void ) const
{
	return m_bUsingPredictionRules;
}

bool C_RecipientFilter::	IgnorePredictionCull( void ) const
{
	return m_bIgnorePredictionCull;
}

void C_RecipientFilter::SetIgnorePredictionCull( bool ignore )
{
	m_bIgnorePredictionCull = ignore;
}

CLocalPlayerFilter::CLocalPlayerFilter()
{
	if ( C_BasePlayer::GetLocalPlayer() )
	{
		AddRecipient( C_BasePlayer::GetLocalPlayer() );
	}
}