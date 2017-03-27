//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "networkstringtable.h"
#include "networkstringtableitem.h"
#include "inetworkstringtableserver.h"
#include "networkstringtablecontainerserver.h"
#include "utlvector.h"
#include "eiface.h"
#include "server.h"
#include "framesnapshot.h"
#include "utlsymbol.h"
#include "utlrbtree.h"
#include "host.h"
#include "LocalNetworkBackdoor.h"
#include "demo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Server string table implementation
//-----------------------------------------------------------------------------
class CNetworkStringTableServer : public CNetworkStringTable
{
	typedef CNetworkStringTable BaseClass;

public:
	// Construction
							CNetworkStringTableServer( TABLEID id, const char *tableName, int maxentries );

	virtual void			DataChanged( int stringNumber, CNetworkStringTableItem *item );
	// Print to console
	virtual void			Dump( void );

	// Zero out all string framecounts, since all clients will be up to date
	//  during connection
	void					MarkSignonStringsCurrent( void );
	// Return true if any entries have changed
	bool					NeedsUpdate( client_t *client );
	// Send updated data to client
	void					SendClientUpdate( client_t *client, bf_write *msg );


	void					CheckDirectUpdate( client_t *client );

private:
	CNetworkStringTableServer( const CNetworkStringTableServer & ); // not implemented, not accessible
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//			*tableName - 
//			maxentries - 
//-----------------------------------------------------------------------------
CNetworkStringTableServer::CNetworkStringTableServer( TABLEID id, const char *tableName, int maxentries )
: CNetworkStringTable( id, tableName, maxentries )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableServer::Dump( void )
{
	Con_Printf( "Server:\n" );
	BaseClass::Dump();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *item - 
//-----------------------------------------------------------------------------
void CNetworkStringTableServer::DataChanged( int stringNumber, CNetworkStringTableItem *item )
{
	Assert( item );
	if ( !item )
		return;

	// Mark changed frame
	item->SetTickCount( host_tickcount );
}

void CNetworkStringTableServer::CheckDirectUpdate( client_t *client )
{
	if ( !client->m_bUsedLocalNetworkBackdoor )
		return;

	assert( client );
	int client_ack = client->GetMaxAckTickCount();

	int count = GetNumStrings();
	for ( int i = 0; i < count; i++ )
	{
		CNetworkStringTableItem *p = GetItem( i );

		// Hasn't changed since signons created, don't send update
		if ( !p->GetTickCount() )
			continue;

		// Client is up to date
		if ( p->GetTickCount() <= client_ack )
			continue;

#ifndef SWDS
		// Oh yeah, gotta slam these strings directly for the backdoor thing to work...
		LocalNetworkBackDoor_DirectStringTableUpdate( GetTableId(),
			i, GetString( i ), p->GetUserDataLength(), p->GetUserData() );

		// Send the same update via networking if we're recording a demo
		if ( !demo->IsRecording() )
		{
			p->SetTickCount( client_ack );
		}

#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *client - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNetworkStringTableServer::NeedsUpdate( client_t *client )
{
	assert( client );
	int client_ack = client->GetMaxAckTickCount();

	int count = GetNumStrings();
	for ( int i = 0; i < count; i++ )
	{
		CNetworkStringTableItem *p = GetItem( i );

		// Hasn't changed since signons created, don't send update
		if ( !p->GetTickCount() )
			continue;

		// Client is up to date
		if ( p->GetTickCount() <= client_ack )
			continue;

#ifndef SWDS
		// Oh yeah, gotta slam these strings directly for the backdoor thing to work...
		if ( client->m_bUsedLocalNetworkBackdoor )
		{
			LocalNetworkBackDoor_DirectStringTableUpdate( GetTableId(),
				i, GetString( i ), p->GetUserDataLength(), p->GetUserData() );
			p->SetTickCount( client_ack );
			continue;
		}
#endif

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *client - 
//			*msg - 
//-----------------------------------------------------------------------------
void CNetworkStringTableServer::SendClientUpdate( client_t *client, bf_write *msg )
{
	assert( client );
	int client_ack = client->GetMaxAckTickCount();

	// Write out the header
	msg->WriteByte( svc_updatestringtable );
	msg->WriteBitLong( GetTableId(), Q_log2( MAX_TABLES ), false );
	
	int count = GetNumStrings();
	for ( int i = 0; i < count; i++ )
	{
		CNetworkStringTableItem *p = GetItem( i );

		// Hasn't changed since signons created, don't send update
		if ( !p->GetTickCount() )
			continue;

		// Client is up to date
		if ( p->GetTickCount() <= client_ack )
			continue;

		// Entry coming
		msg->WriteOneBit( 1 );
		// Entry index
		msg->WriteBitLong( i, GetEntryBits(), false );
		// 
		msg->WriteString( GetString( i ) );

		if ( p->GetUserDataLength() > 0 )
		{
			msg->WriteOneBit( 1 );
			
			int length = p->GetUserDataLength();
			msg->WriteUBitLong( length, CNetworkStringTableItem::MAX_USERDATA_BITS );
			msg->WriteBytes( p->GetUserData(), length );
		}
		else
		{
			msg->WriteOneBit( 0 );
		}
	}

	// No more entries
	msg->WriteOneBit( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableServer::MarkSignonStringsCurrent( void )
{
}

static CNetworkStringTableContainerServer g_NetworkStringTableServer;
CNetworkStringTableContainerServer *networkStringTableContainerServer = &g_NetworkStringTableServer;

// Expose interface
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CNetworkStringTableContainerServer, INetworkStringTableServer, INTERFACENAME_NETWORKSTRINGTABLESERVER, g_NetworkStringTableServer );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetworkStringTableContainerServer::CNetworkStringTableContainerServer( void )
{
	m_bAllowCreation = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetworkStringTableContainerServer::~CNetworkStringTableContainerServer( void )
{
	RemoveAllTables();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::AllowCreation( void )
{
	m_bAllowCreation = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::DisallowCreation( void )
{
	m_bAllowCreation = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tableName - 
//			maxentries - 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CNetworkStringTableContainerServer::CreateStringTable( const char *tableName, int maxentries )
{
	if ( !m_bAllowCreation )
	{
		Sys_Error( "Tried to create string table '%s' at wrong time\n", tableName );
	}

	TABLEID found = FindTable( tableName );
	if ( found != INVALID_STRING_TABLE )
	{
		assert( 0 );
		return INVALID_STRING_TABLE;
	}

	if ( m_Tables.Size() >= MAX_TABLES )
	{
		Sys_Error( "Only %i string tables allowed, can't create'%s'", MAX_TABLES, tableName);
	}

	TABLEID id = m_Tables.Size();

	CNetworkStringTableServer *pTable = new CNetworkStringTableServer( id, tableName, maxentries );
	assert( pTable );
	m_Tables.AddToTail( pTable );

	return id;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTableContainerServer::GetNumStrings( TABLEID stringTable )
{
	CNetworkStringTableServer *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return 0;
	}
	return table->GetNumStrings();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
//-----------------------------------------------------------------------------
int	CNetworkStringTableContainerServer::GetMaxStrings( TABLEID stringTable )
{
	CNetworkStringTableServer *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return 0;
	}
	return table->GetMaxEntries();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
//			*value - 
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTableContainerServer::AddString( TABLEID stringTable, const char *value, int length /*=0*/, const void *userdata /*=0*/ )
{
	CNetworkStringTableServer *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return INVALID_STRING_INDEX;
	}

	return table->AddString( value, length, userdata );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
//			stringNumber - 
//			*value - 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::SetString( TABLEID stringTable, int stringNumber, const char *value )
{
	CNetworkStringTableServer *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return;
	}

	table->SetString( stringNumber, value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
//			stringNumber - 
//			length - 
//			*userdata - 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::SetStringUserData( TABLEID stringTable, int stringNumber, int length /*=0*/, const void *userdata /*=0*/ )
{
	CNetworkStringTableServer *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return;
	}

	table->SetStringUserData( stringNumber, length, userdata );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
//			stringNumber - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CNetworkStringTableContainerServer::GetString( TABLEID stringTable, int stringNumber )
{
	CNetworkStringTableServer *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return NULL;
	}

	return table->GetString( stringNumber );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
//			stringNumber - 
//			length - 
// Output : const void
//-----------------------------------------------------------------------------
const void *CNetworkStringTableContainerServer::GetStringUserData( TABLEID stringTable, int stringNumber, int *length /*=0*/ )
{
	CNetworkStringTableServer *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return NULL;
	}

	return table->GetStringUserData( stringNumber, length );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
//			*string - 
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTableContainerServer::FindStringIndex( TABLEID stringTable, char const *string )
{
	CNetworkStringTableServer *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return -1;
	}

	return table->FindStringIndex( string );	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tableName - 
//-----------------------------------------------------------------------------
TABLEID	CNetworkStringTableContainerServer::FindTable( const char *tableName )
{
	for ( int i = 0; i < m_Tables.Size(); i++ )
	{
		if ( !stricmp( tableName, m_Tables[ i ]->GetTableName() ) )
			return (TABLEID)i;
	}

	return INVALID_STRING_TABLE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CNetworkStringTableContainerServer::GetTableName( TABLEID stringTable )
{
	CNetworkStringTableServer *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return NULL;
	}

	return table->GetTableName();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
// Output : CNetworkStringTableServer
//-----------------------------------------------------------------------------
CNetworkStringTableServer *CNetworkStringTableContainerServer::GetTable( TABLEID stringTable )
{
	if ( stringTable < 0 || stringTable >= m_Tables.Size() )
		return NULL;

	return m_Tables[ stringTable ];
}

#define SUBSTRING_BITS	5

int CountSimilarCharacters( char const *str1, char const *str2 )
{
	int c = 0;
	while ( *str1 && *str2 &&
		*str1 == *str2 && c < ((1<<SUBSTRING_BITS) -1 ))
	{
		str1++;
		str2++;
		c++;
	}

	return c;
}



struct StringHistoryEntry
{
	char string[ (1<<SUBSTRING_BITS) ];
};

static int GetBestPreviousString( CUtlVector< StringHistoryEntry >& history, char const *newstring, int& substringsize )
{
	int bestindex = -1;
	int bestcount = 0;
	int c = history.Count();
	for ( int i = 0; i < c; i++ )
	{
		char const *prev = history[ i ].string;
		int similar = CountSimilarCharacters( prev, newstring );
		
		if ( similar < 3 )
			continue;

		if ( similar > bestcount )
		{
			bestcount = similar;
			bestindex = i;
		}
	}

	substringsize = bestcount;
	return bestindex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::CreateTableDefinitions( bf_write *msg )
{
	msg->WriteByte( svc_createstringtables );

	// 256 tables max
	assert( m_Tables.Size() <= MAX_TABLES );

	msg->WriteByte( m_Tables.Size() );

	CUtlVector< StringHistoryEntry > history;

	for ( int i = 0 ; i < m_Tables.Size() ; i++ )
	{
		history.RemoveAll();

		CNetworkStringTableServer *table = GetTable( i );
		assert( table );

		msg->WriteString( table->GetTableName() );
		msg->WriteShort( table->GetMaxEntries() );
		int numStrings = table->GetNumStrings();
		int encodeBits = table->GetEntryBits();

		msg->WriteUBitLong( numStrings, encodeBits );

		for ( int j = 0; j < table->GetNumStrings(); j++ )
		{
			char const *entry = table->GetString( j );
			int substringsize = 0;
			int bestprevious = GetBestPreviousString( history, entry, substringsize );
			if ( bestprevious != -1 )
			{
				// As history gets longer, send bits grows
				// FIXME: Could put a cap on this or flush history per table
				int sendbits = Q_log2( history.Count() ) + 1;

				msg->WriteOneBit( 1 );
				msg->WriteUBitLong( bestprevious, sendbits );
				msg->WriteUBitLong( substringsize, SUBSTRING_BITS );
				msg->WriteString( entry + substringsize );
			}
			else
			{
				msg->WriteOneBit( 0 );
				msg->WriteString( entry  );
			}
			
			// Write the item's user data.
			int len;
			const void *pUserData = table->GetStringUserData( j, &len );
			if ( pUserData && len > 0 )
			{
				msg->WriteOneBit( 1 );
				msg->WriteUBitLong( len, CNetworkStringTableItem::MAX_USERDATA_BITS );
				msg->WriteBits( pUserData, len*8 );
			}
			else
			{
				msg->WriteOneBit( 0 );
			}

			if ( history.Count() >= 31 )
			{
				history.Remove( 0 );
			}

			StringHistoryEntry she;
			Q_strncpy( she.string, entry, sizeof( she.string ) );
			history.AddToTail( she );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
//			*msg - 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::CheckDirectUpdate( client_t *cl )
{
	// Determine if an update is needed
	for ( int i = 0; i < m_Tables.Size(); i++ )
	{
		CNetworkStringTableServer *table = GetTable( i );
		if ( !table )
			continue;

		table->CheckDirectUpdate( cl );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
//			*msg - 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::UpdateClient( client_t *cl, bf_write *msg )
{
	// Determine if an update is needed
	for ( int i = 0; i < m_Tables.Size(); i++ )
	{
		CNetworkStringTableServer *table = GetTable( i );
		if ( !table )
			continue;

		if ( !table->NeedsUpdate( cl ) )
			continue;

		table->SendClientUpdate( cl, msg );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::MarkSignonStringsCurrent( void )
{
	for ( int i = 0; i < m_Tables.Size(); i++ )
	{
		CNetworkStringTableServer *table = GetTable( i );
		if ( !table )
			continue;

		table->MarkSignonStringsCurrent();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::RemoveAllTables( void )
{
	while ( m_Tables.Size() > 0 )
	{
		CNetworkStringTableServer *table = m_Tables[ 0 ];
		m_Tables.Remove( 0 );
		delete table;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerServer::Dump( void )
{
	for ( int i = 0; i < m_Tables.Size(); i++ )
	{
		m_Tables[ i ]->Dump();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SV_CreateNetworkStringTables( void )
{
	// Remove any existing tables
	g_NetworkStringTableServer.RemoveAllTables();

	// Unset timing guard and create tables
	g_NetworkStringTableServer.AllowCreation();

	// Create engine tables
	sv.CreateEngineStringTables();

	// Create game code tables
	serverGameDLL->CreateNetworkStringTables();

	g_NetworkStringTableServer.DisallowCreation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SV_WriteNetworkStringTablesToBuffer( bf_write *msg )
{
	g_NetworkStringTableServer.CreateTableDefinitions( msg );
	g_NetworkStringTableServer.MarkSignonStringsCurrent();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
//-----------------------------------------------------------------------------
void SV_UpdateStringTables( client_t *cl, bf_write *msg )
{
	g_NetworkStringTableServer.UpdateClient( cl, msg );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
//-----------------------------------------------------------------------------
void SV_UpdateAcknowledgedFramecount( client_t *cl )
{
	Assert( cl->delta_sequence != -1 );

	client_frame_t *fromframe = &cl->frames[ cl->delta_sequence & SV_UPDATE_MASK ];
	CFrameSnapshot *pSnapshot = fromframe->GetSnapshot();
	if ( pSnapshot )
		cl->acknowledged_tickcount = pSnapshot->m_nTickNumber;
	else
		cl->acknowledged_tickcount = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SV_PrintStringTables( void )
{
	g_NetworkStringTableServer.Dump();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
//-----------------------------------------------------------------------------
void SV_CheckDirectUpdate( client_t *cl )
{
	g_NetworkStringTableServer.CheckDirectUpdate( cl );
}