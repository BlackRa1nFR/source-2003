#include "cbase.h"
#include "recipientfilter.h"
#include "team.h"
#include "ipredictionsystem.h"

static IPredictionSystem g_RecipientFilterPredictionSystem;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecipientFilter::CRecipientFilter()
{
	m_pEntityMessageName = NULL;
	Reset();
}

CRecipientFilter::~CRecipientFilter()
{
	delete[] m_pEntityMessageName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecipientFilter::Reset( void )
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
void CRecipientFilter::EnsureTruePlayerCount( void )
{
	if ( m_nTruePlayerCount != -1 )
		return;

	m_nTruePlayerCount = 0;
	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
		{
			continue;
		}

		m_nTruePlayerCount++;
	}
}

void CRecipientFilter::MakeReliable( void )
{
	m_bReliable = true;
}

bool CRecipientFilter::IsReliable( void ) const
{
	return m_bReliable;
}

int CRecipientFilter::GetRecipientCount( void ) const
{
	return m_Recipients.Size();
}

int	CRecipientFilter::GetRecipientIndex( int slot ) const
{
	if ( slot < 0 || slot >= GetRecipientCount() )
		return -1;

	return m_Recipients[ slot ];
}

void CRecipientFilter::AddAllPlayers( void )
{
	m_Recipients.RemoveAll();

	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
		{
			continue;
		}

		AddRecipient( pPlayer );
	}
}

void CRecipientFilter::AddRecipient( CBasePlayer *player )
{
	Assert( player );

	int index = player->entindex();

	// If we're predicting and this is not the first time we've predicted this sound
	//  then don't send it to the local player again.
	if ( m_bUsingPredictionRules )
	{
		// Only add local player if this is the first time doing prediction
		if ( g_RecipientFilterPredictionSystem.GetSuppressHost() == player )
		{
			return;
		}
	}

	// Already in list
	if ( m_Recipients.Find( index ) != m_Recipients.InvalidIndex() )
		return;

	m_Recipients.AddToTail( index );
}

void CRecipientFilter::RemoveAllRecipients( void )
{
	m_Recipients.RemoveAll();
}

void CRecipientFilter::RemoveRecipient( CBasePlayer *player )
{
	Assert( player );

	int index = player->entindex();

	// Remove it if it's in the list
	m_Recipients.FindAndRemove( index );
}

void CRecipientFilter::AddRecipientsByTeam( CTeam *team )
{
	Assert( team );

	int i;
	int c = team->GetNumPlayers();
	for ( i = 0 ; i < c ; i++ )
	{
		CBasePlayer *player = team->GetPlayer( i );
		if ( !player )
			continue;

		AddRecipient( player );
	}
}

void CRecipientFilter::RemoveRecipientsByTeam( CTeam *team )
{
	Assert( team );

	int i;
	int c = team->GetNumPlayers();
	for ( i = 0 ; i < c ; i++ )
	{
		CBasePlayer *player = team->GetPlayer( i );
		if ( !player )
			continue;

		RemoveRecipient( player );
	}
}

void CRecipientFilter::AddPlayersFromBitMask( unsigned int playerbits )
{
	int i;
	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( !( playerbits & (1<<i) ) )
			continue;

		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i + 1 );
		if ( !pPlayer )
			continue;

		AddRecipient( pPlayer );
	}
}

void CRecipientFilter::AddRecipientsByPVS( const Vector& origin )
{
	if ( gpGlobals->maxClients == 1 )
	{
		AddAllPlayers();
	}
	else
	{
		unsigned int playerbits = 0;
		engine->Message_DetermineMulticastRecipients( false, origin, playerbits );
		AddPlayersFromBitMask( playerbits );
	}
}

void CRecipientFilter::AddRecipientsByPAS( const Vector& origin )
{
	if ( gpGlobals->maxClients == 1 )
	{
		AddAllPlayers();
	}
	else
	{
		unsigned int playerbits = 0;
		engine->Message_DetermineMulticastRecipients( true, origin, playerbits );
		AddPlayersFromBitMask( playerbits );
	}
}

void CRecipientFilter::SetEntityMessage( int entity, const char *msgname )
{
	m_Type = ENTITY;
	m_nEntityIndex = entity;
	delete[] m_pEntityMessageName;
	m_pEntityMessageName = new char[ Q_strlen( msgname ) + 1 ];
	Assert( m_pEntityMessageName );
	Q_strcpy( m_pEntityMessageName, msgname );
}

bool CRecipientFilter::IsEntityMessage( void ) const
{
	return ( m_Type == ENTITY ) ? true : false;
}

int CRecipientFilter::GetEntityIndex( void ) const
{
	return m_nEntityIndex;
}

char const *CRecipientFilter::GetEntityMessageName( void ) const
{
	Assert( IsEntityMessage() );
	Assert( m_pEntityMessageName );
	return m_pEntityMessageName ? m_pEntityMessageName : "";
}

bool CRecipientFilter::IsBroadcastMessage( void )
{
	EnsureTruePlayerCount();
	if ( m_nTruePlayerCount == m_Recipients.Count() )
		return true;

	return false;
}

bool CRecipientFilter::IsInitMessage( void ) const
{
	return m_bInitMessage;
}

void CRecipientFilter::MakeInitMessage( void )
{
	m_bInitMessage = true;
}

void CRecipientFilter::UsePredictionRules( void )
{
	if ( m_bUsingPredictionRules )
		return;

	m_bUsingPredictionRules = true;

	// Cull list now, if needed
	int c = GetRecipientCount();
	if ( c == 0 )
		return;

	int i;
	for( i = c - 1; i >= 0; i-- )
	{
		int idx = GetRecipientIndex( i );
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( idx );
		if ( !pPlayer )
			continue;

		if ( g_RecipientFilterPredictionSystem.GetSuppressHost() == pPlayer )
		{
			RemoveRecipient( pPlayer );
		}
	}
}

bool CRecipientFilter::IsUsingPredictionRules( void ) const
{
	return m_bUsingPredictionRules;
}

bool CRecipientFilter::	IgnorePredictionCull( void ) const
{
	return m_bIgnorePredictionCull;
}

void CRecipientFilter::SetIgnorePredictionCull( bool ignore )
{
	m_bIgnorePredictionCull = ignore;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			ATTN_NORM - 
//-----------------------------------------------------------------------------
void CPASAttenuationFilter::Filter( const Vector& origin, float attenuation /*= ATTN_NORM*/ )
{
	// Don't crop for attenuation in single player
	if ( gpGlobals->maxClients == 1 )
		return;

	// CPASFilter adds them by pure PVS in constructor
	if ( attenuation <= 0 )
		return;

	// Now remove recipients that are outside sound radius
	float distance, maxAudible;
	Vector vecRelative;

	int c = GetRecipientCount();
	int i;
	for ( i = c - 1; i >= 0; i-- )
	{
		int index = GetRecipientIndex( i );

		CBaseEntity *ent = CBaseEntity::Instance( index );
		if ( !ent || !ent->IsPlayer() )
		{
			Assert( 0 );
			continue;
		}

		CBasePlayer *player = ToBasePlayer( ent );
		if ( !player )
		{
			Assert( 0 );
			continue;
		}

		VectorSubtract( player->EarPosition(), origin, vecRelative );
		distance = VectorLength( vecRelative );
		maxAudible = ( 2 * SOUND_NORMAL_CLIP_DIST ) / attenuation;
		if ( distance <= maxAudible )
			continue;

		RemoveRecipient( player );
	}
}