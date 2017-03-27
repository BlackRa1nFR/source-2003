#include "quakedef.h"
#include "server.h"
#include "enginesingleuserfilter.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEngineRecipientFilter::CEngineRecipientFilter()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineRecipientFilter::Reset( void )
{
	m_bReliable			= false;
	m_nTruePlayerCount	= -1;
	m_Recipients.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Get a count of all possible players
//-----------------------------------------------------------------------------
void CEngineRecipientFilter::EnsureTruePlayerCount( void )
{
	if ( m_nTruePlayerCount != -1 )
		return;

	m_nTruePlayerCount = 0;
	int i;
	for ( i = 0; i <= svs.maxclients; i++ )
	{
		client_t *cl = &svs.clients[ i ];
		if ( !cl->active )
			continue;

		m_nTruePlayerCount++;
	}
}

void CEngineRecipientFilter::MakeReliable( void )
{
	m_bReliable = true;
}

bool CEngineRecipientFilter::IsReliable( void ) const
{
	return m_bReliable;
}

int CEngineRecipientFilter::GetRecipientCount( void ) const
{
	return m_Recipients.Size();
}

int	CEngineRecipientFilter::GetRecipientIndex( int slot ) const
{
	if ( slot < 0 || slot >= GetRecipientCount() )
		return -1;

	return m_Recipients[ slot ];
}

void CEngineRecipientFilter::AddAllPlayers( void )
{
	m_Recipients.RemoveAll();

	int i;
	for ( i = 0; i <= svs.maxclients; i++ )
	{
		client_t *cl = &svs.clients[ i ];
		if ( !cl->active )
			continue;

		m_Recipients.AddToTail( i + 1 );
	}
}

void CEngineRecipientFilter::AddRecipient( int index )
{
	// Already in list
	if ( m_Recipients.Find( index ) != m_Recipients.InvalidIndex() )
		return;

	m_Recipients.AddToTail( index );
}

void CEngineRecipientFilter::RemoveRecipient( int index )
{
	// Remove it if it's in the list
	m_Recipients.FindAndRemove( index );
}

void CEngineRecipientFilter::AddPlayersFromBitMask( unsigned int playerbits )
{
	int i;
	for( i = 0; i < svs.maxclients; i++ )
	{
		if ( !( playerbits & (1<<i) ) )
			continue;

		client_t *cl = &svs.clients[ i ];
		if ( !cl->active )
			continue;

		AddRecipient( i + 1 );
	}
}

void CEngineRecipientFilter::AddRecipientsByPVS( const Vector& origin )
{
	if ( svs.maxclients == 1 )
	{
		AddAllPlayers();
	}
	else
	{
		unsigned int playerbits = 0;
		SV_DetermineMulticastRecipients( false, origin, playerbits );
		AddPlayersFromBitMask( playerbits );
	}
}

void CEngineRecipientFilter::AddRecipientsByPAS( const Vector& origin )
{
	if ( svs.maxclients == 1 )
	{
		AddAllPlayers();
	}
	else
	{
		unsigned int playerbits = 0;
		SV_DetermineMulticastRecipients( true, origin, playerbits );
		AddPlayersFromBitMask( playerbits );
	}
}

bool CEngineRecipientFilter::IsBroadcastMessage( void )
{
	EnsureTruePlayerCount();
	if ( m_nTruePlayerCount == m_Recipients.Count() )
		return true;

	return false;
}
