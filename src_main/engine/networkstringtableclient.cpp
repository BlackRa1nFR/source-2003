//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "networkstringtable.h"
#include "networkstringtableitem.h"
#include "inetworkstringtableclient.h"
#include "networkstringtablecontainerclient.h"
#include "utlvector.h"
#include "cdll_engine_int.h"
#include "precache.h"
#include "utlsymbol.h"
#include "utlrbtree.h"
#include "utlbuffer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *object - 
//			stringTable - 
//			stringNumber - 
//			*newString - 
//			*newData - 
//-----------------------------------------------------------------------------
void Callback_ModelChanged( void *object, TABLEID stringTable, int stringNumber, char const *newString, void const *newData )
{
	if ( stringTable == cl.GetModelPrecacheTable() )
	{
		// Index 0 is always NULL, just ignore it
		// Index 1 == the world, don't 
		if ( stringNumber >= 1 )
		{
			cl.SetModel( stringNumber );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *object - 
//			stringTable - 
//			stringNumber - 
//			*newString - 
//			*newData - 
//-----------------------------------------------------------------------------
void Callback_GenericChanged( void *object, TABLEID stringTable, int stringNumber, char const *newString, void const *newData )
{
	if ( stringTable == cl.GetGenericPrecacheTable() )
	{
		// Index 0 is always NULL, just ignore it
		if ( stringNumber >= 1 )
		{
			cl.SetGeneric( stringNumber );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *object - 
//			stringTable - 
//			stringNumber - 
//			*newString - 
//			*newData - 
//-----------------------------------------------------------------------------
void Callback_SoundChanged( void *object, TABLEID stringTable, int stringNumber, char const *newString, void const *newData )
{
	if ( stringTable == cl.GetSoundPrecacheTable() )
	{
		// Index 0 is always NULL, just ignore it
		if ( stringNumber >= 1 )
		{
			cl.SetSound( stringNumber );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *object - 
//			stringTable - 
//			stringNumber - 
//			*newString - 
//			*newData - 
//-----------------------------------------------------------------------------
void Callback_DecalChanged( void *object, TABLEID stringTable, int stringNumber, char const *newString, void const *newData )
{
	if ( stringTable == cl.GetDecalPrecacheTable() )
	{
		cl.SetDecal( stringNumber );
	}
}


void Callback_InstanceBaselineChanged( void *object, TABLEID stringTable, int stringNumber, char const *newString, void const *newData )
{
	Assert( stringTable == cl.GetInstanceBaselineTable() );
	cl.UpdateInstanceBaseline( stringNumber );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CL_InstallEngineStringTableCallback( char const *tableName )
{
	// Hook Model Precache table
	if ( !Q_strcasecmp( tableName, MODEL_PRECACHE_TABLENAME ) )
	{
		// Look up the id 
		TABLEID id = networkStringTableContainerClient->FindStringTable( tableName );
		// Cache the id
		cl.SetModelPrecacheTable( id );
		// Install the callback
		networkStringTableContainerClient->SetStringChangedCallback( NULL, id, Callback_ModelChanged );
		return true;
	}

	if ( !Q_strcasecmp( tableName, GENERIC_PRECACHE_TABLENAME ) )
	{
		// Look up the id 
		TABLEID id = networkStringTableContainerClient->FindStringTable( tableName );
		// Cache the id
		cl.SetGenericPrecacheTable( id );
		// Install the callback
		networkStringTableContainerClient->SetStringChangedCallback( NULL, id, Callback_GenericChanged );
		return true;
	}

	if ( !Q_strcasecmp( tableName, SOUND_PRECACHE_TABLENAME ) )
	{
		// Look up the id 
		TABLEID id = networkStringTableContainerClient->FindStringTable( tableName );
		// Cache the id
		cl.SetSoundPrecacheTable( id );
		// Install the callback
		networkStringTableContainerClient->SetStringChangedCallback( NULL, id, Callback_SoundChanged );
		return true;
	}

	if ( !Q_strcasecmp( tableName, DECAL_PRECACHE_TABLENAME ) )
	{
		// Look up the id 
		TABLEID id = networkStringTableContainerClient->FindStringTable( tableName );
		// Cache the id
		cl.SetDecalPrecacheTable( id );
		// Install the callback
		networkStringTableContainerClient->SetStringChangedCallback( NULL, id, Callback_DecalChanged );
		return true;
	}

	if ( !Q_strcasecmp( tableName, INSTANCE_BASELINE_TABLENAME ) )
	{
		// Look up the id 
		TABLEID id = networkStringTableContainerClient->FindStringTable( tableName );
		// Cache the id
		cl.SetInstanceBaselineTable( id );
		// Install the callback
		networkStringTableContainerClient->SetStringChangedCallback( NULL, id, Callback_InstanceBaselineChanged );
		return true;
	}

	// The the client.dll have a shot at it
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Client side implementation of a string table
//-----------------------------------------------------------------------------
class CNetworkStringTableClient : public CNetworkStringTable
{
	typedef CNetworkStringTable BaseClass;

public:
	// Construction
							CNetworkStringTableClient( TABLEID id, const char *tableName, int maxentries );

	virtual void			DataChanged( int stringNumber, CNetworkStringTableItem *item );
	// Print contents to console
	virtual void			Dump( void );

	// Parse changed field info
	void					ParseUpdate( void );

	void					SetStringChangedCallback( void *object, pfnStringChanged changeFunc );

	void					DirectUpdate( int entryIndex, char const *string,
								int userdatalength, const void *userdata );

	void					WriteStringTable( CUtlBuffer& buf );
	bool					ReadStringTable( CUtlBuffer& buf );

private:
	CNetworkStringTableClient( const CNetworkStringTableClient & ); // not implemented, not accessible

	// Change function callback
	pfnStringChanged		m_changeFunc;
	// Optional context/object
	void					*m_pObject;
};

//-----------------------------------------------------------------------------
// Purpose: Creates a string table on the client
// Input  : id - 
//			*tableName - 
//			maxentries - 
//-----------------------------------------------------------------------------
CNetworkStringTableClient::CNetworkStringTableClient( TABLEID id, const char *tableName, int maxentries )
: CNetworkStringTable( id, tableName, maxentries )
{
	m_changeFunc = NULL;
	m_pObject = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : changeFunc - 
//-----------------------------------------------------------------------------
void CNetworkStringTableClient::SetStringChangedCallback( void *object, pfnStringChanged changeFunc )
{
	m_changeFunc = changeFunc;
	m_pObject = object;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableClient::Dump( void )
{
	Con_Printf( "Client\n" );
	BaseClass::Dump();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableClient::DataChanged( int stringNumber, CNetworkStringTableItem *item )
{
	// Invoke callback if one was installed
	if ( m_changeFunc != NULL )
	{
		int userDataSize;
		const void *pUserData = item->GetUserData( &userDataSize );
		( *m_changeFunc )( m_pObject, GetTableId(), stringNumber, GetString( stringNumber ), pUserData );
	}
}

void CNetworkStringTableClient::DirectUpdate( int entryIndex, char const *string,
	int userdatalength, const void *userdata )
{
	char const *pName = string;

	// Read in the user data.
	const void *pUserData = NULL;
	int nBytes = 0;

	if ( userdatalength > 0 && userdata != NULL )
	{
		nBytes = userdatalength;
		pUserData = userdata;
	}

	/*
	char *netname = cl.m_pServerClasses[ atoi( pName ) ].m_ClassName;

	Con_Printf( "%s:  received %s, %i bytes = %s\n",
	   GetTableName(), netname, nBytes, pUserData ? COM_BinPrintf( (byte *)pUserData, nBytes ) : "NULL" );
	*/

	// Check if we are updating an old entry or adding a new one
	if ( entryIndex < GetNumStrings() )
	{
		SetString( entryIndex, pName );
		SetStringUserData( entryIndex, nBytes, pUserData );
	}
	else
	{
		// Grow the table (entryindex must be the next empty slot)
		Assert( entryIndex == GetNumStrings() );
		AddString( pName, nBytes, pUserData );
	}
}

void CNetworkStringTableClient::WriteStringTable( CUtlBuffer& buf )
{
	int numstrings = GetNumStrings();
	buf.PutInt( numstrings );
	for ( int i = 0 ; i < numstrings; i++ )
	{
		buf.PutString( GetString( i ) );
		int userDataSize;
		const void *pUserData = GetStringUserData( i, &userDataSize );
		if ( userDataSize > 0 )
		{
			buf.PutChar( 1 );
			buf.PutShort( (short)userDataSize );
			buf.Put( pUserData, userDataSize );
		}
		else
		{
			buf.PutChar( 0 );
		}
		
	}
}

bool CNetworkStringTableClient::ReadStringTable( CUtlBuffer& buf )
{
	DeleteAllStrings();

	int numstrings = buf.GetInt();
	for ( int i = 0 ; i < numstrings; i++ )
	{
		char stringname[4096];
		
		buf.GetString( stringname, sizeof( stringname ) );

		if ( buf.GetChar() == 1 )
		{
			int userDataSize = (int)buf.GetShort();
			Assert( userDataSize > 0 );
			byte *data = new byte[ userDataSize + 4 ];
			Assert( data );

			buf.Get( data, userDataSize );

			AddString( stringname, userDataSize, data );

			delete[] data;
			
		}
		else
		{
			AddString( stringname );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Parse string update
//-----------------------------------------------------------------------------
void CNetworkStringTableClient::ParseUpdate( void )
{
	while ( MSG_ReadOneBit() )
	{
		int entryIndex = MSG_ReadBitLong( GetEntryBits() );
		if ( entryIndex < 0 || entryIndex >= GetMaxEntries() )
		{
			Host_Error( "Server sent bogus string index %i for table %s\n", entryIndex, GetTableName() );
		}

		const char *pName = MSG_ReadString();

		// Read in the user data.
		unsigned char tempbuf[ CNetworkStringTableItem::MAX_USERDATA_SIZE ];
		const void *pUserData = NULL;
		int nBytes = 0;

		if ( MSG_ReadOneBit() )
		{
			nBytes = MSG_ReadBitLong( CNetworkStringTableItem::MAX_USERDATA_BITS );
			ErrorIfNot( nBytes <= sizeof( tempbuf ),
				("CNetworkStringTableClient::ParseUpdate: message too large (%d bytes).", nBytes)
			);

			MSG_GetReadBuf()->ReadBytes( tempbuf, nBytes );
			pUserData = tempbuf;
		}

		/*
		char *netname = cl.m_pServerClasses[ atoi( pName ) ].m_ClassName;

		Con_Printf( "%s:  received %s, %i bytes = %s\n",
		   GetTableName(), netname, nBytes, pUserData ? COM_BinPrintf( (byte *)pUserData, nBytes ) : "NULL" );
		*/

		// Check if we are updating an old entry or adding a new one
		if ( entryIndex < GetNumStrings() )
		{
			SetString( entryIndex, pName );
			SetStringUserData( entryIndex, nBytes, pUserData );
		}
		else
		{
			// Grow the table (entryindex must be the next empty slot)
			Assert( entryIndex == GetNumStrings() );
			AddString( pName, nBytes, pUserData );
		}
	}
}

static CNetworkStringTableContainerClient g_NetworkStringTableClient;
CNetworkStringTableContainerClient *networkStringTableContainerClient = &g_NetworkStringTableClient;
// Expose to client .dll
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CNetworkStringTableContainerClient, INetworkStringTableClient, INTERFACENAME_NETWORKSTRINGTABLECLIENT, g_NetworkStringTableClient );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetworkStringTableContainerClient::CNetworkStringTableContainerClient( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetworkStringTableContainerClient::~CNetworkStringTableContainerClient( void )
{
	RemoveAllTables();
}

void CNetworkStringTableContainerClient::SetStringChangedCallback( void *object, TABLEID stringTable, pfnStringChanged changeFunc )
{
	CNetworkStringTableClient *table = GetTable( stringTable );
	if ( table )
	{
		table->SetStringChangedCallback( object, changeFunc );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tableName - 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CNetworkStringTableContainerClient::FindStringTable( const char *tableName )
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
// Output : char const
//-----------------------------------------------------------------------------
char const *CNetworkStringTableContainerClient::GetTableName( TABLEID stringTable )
{
	CNetworkStringTableClient *table = GetTable( stringTable );
	assert( table );
	if ( !table )
	{
		assert( 0 );
		return "Unknown Table";
	}
	return table->GetTableName();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTableContainerClient::GetNumStrings( TABLEID stringTable )
{
	CNetworkStringTableClient *table = GetTable( stringTable );
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
int	CNetworkStringTableContainerClient::GetMaxStrings( TABLEID stringTable )
{
	CNetworkStringTableClient *table = GetTable( stringTable );
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
//			stringNumber - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CNetworkStringTableContainerClient::GetString( TABLEID stringTable, int stringNumber )
{
	CNetworkStringTableClient *table = GetTable( stringTable );
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
// Output : const char
//-----------------------------------------------------------------------------
const void *CNetworkStringTableContainerClient::GetStringUserData( TABLEID stringTable, int stringNumber, int *length /*=0*/ )
{
	CNetworkStringTableClient *table = GetTable( stringTable );
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
int CNetworkStringTableContainerClient::FindStringIndex( TABLEID stringTable, char const *string )
{
	CNetworkStringTableClient *table = GetTable( stringTable );
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
//			maxentries - 
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CNetworkStringTableContainerClient::AddTable( const char *tableName, int maxentries )
{
	TABLEID found = FindStringTable( tableName );
	if ( found != INVALID_STRING_TABLE )
	{
		assert( 0 );
		return INVALID_STRING_TABLE;
	}

	TABLEID id = m_Tables.Size();

	CNetworkStringTableClient *pTable = new CNetworkStringTableClient( id, tableName, maxentries );
	assert( pTable );
	m_Tables.AddToTail( pTable );

	return id;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
// Output : CNetworkStringTableClient
//-----------------------------------------------------------------------------
CNetworkStringTableClient *CNetworkStringTableContainerClient::GetTable( TABLEID stringTable )
{
	if ( stringTable < 0 || stringTable >= m_Tables.Size() )
		return NULL;

	return m_Tables[ stringTable ];
}

#define SUBSTRING_BITS	5

struct StringHistoryEntry
{
	char string[ (1<<SUBSTRING_BITS) ];
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerClient::ParseTableDefinitions( void )
{
	// Kill any existing ones
	RemoveAllTables();

	int numTables = MSG_ReadByte();
	assert( numTables >= 0 && numTables <= MAX_TABLES );

	CUtlVector< StringHistoryEntry > history;

	for ( int i = 0; i < numTables; i++ )
	{
		history.RemoveAll();

		char tableName[ 256 ];
		Q_strncpy( tableName, MSG_ReadString(), sizeof( tableName ) );
		
		int maxentries = MSG_ReadShort();
		assert( maxentries >= 0 );

		TABLEID id = AddTable( tableName, maxentries );
		assert( id != INVALID_STRING_TABLE );

		CNetworkStringTableClient *table = GetTable( id );
		assert( table );

		// Let engine hook callbacks before we read in any data values at all
		if ( !CL_InstallEngineStringTableCallback( tableName ) )
		{
			// If engine takes a pass, allow client dll to hook in its callbacks
			g_ClientDLL->InstallStringTableCallback( tableName );
		}

		int usedentries = MSG_ReadBitLong( table->GetEntryBits() );

		for ( int i = 0; i < usedentries; i++ )
		{
			char entry[ 1024 ];
			entry[ 0 ] =0;

			bool substringcheck = MSG_ReadOneBit() ? true : false;
			if ( substringcheck )
			{
				int recvbits = Q_log2( history.Count() ) + 1;
				int index = MSG_ReadBitLong( recvbits );
				int bytestocopy = MSG_ReadBitLong( SUBSTRING_BITS );
				Q_strncpy( entry, history[ index ].string, bytestocopy + 1 );
				Q_strcat( entry, MSG_ReadString() );
			}
			else
			{
				Q_strncpy( entry, MSG_ReadString(), sizeof( entry ) );
			}

			if ( MSG_ReadOneBit() )
			{
				byte data[ CNetworkStringTableItem::MAX_USERDATA_SIZE ];
				int nBytes = MSG_ReadBitLong( CNetworkStringTableItem::MAX_USERDATA_BITS );
				MSG_GetReadBuf()->ReadBytes( data, nBytes );
				
				table->AddString( entry, nBytes, data );

				//Con_Printf( "%s + %i\n", entry, nBytes );

			}
			else
			{
				table->AddString( entry );

				//Con_Printf( "%s\n", entry );
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
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerClient::ParseUpdate( void )
{
	TABLEID tableId;
	tableId = MSG_ReadBitLong( Q_log2( MAX_TABLES ) );
	if ( tableId < 0 || tableId >= m_Tables.Size() )
	{
		Host_Error( "Server sent bogus table id %i\n", tableId );
	}

	CNetworkStringTableClient *table = m_Tables[ tableId ];
	assert( table );

	table->ParseUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerClient::RemoveAllTables( void )
{
	while ( m_Tables.Size() > 0 )
	{
		CNetworkStringTableClient *table = m_Tables[ 0 ];
		m_Tables.Remove( 0 );
		delete table;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainerClient::Dump( void )
{
	for ( int i = 0; i < m_Tables.Size(); i++ )
	{
		m_Tables[ i ]->Dump();
	}
}

void CNetworkStringTableContainerClient::WriteStringTables( CUtlBuffer& buf )
{
	int numTables = m_Tables.Size();

	buf.PutInt( numTables );
	for ( int i = 0; i < numTables; i++ )
	{
		CNetworkStringTableClient *table = m_Tables[ i ];
		buf.PutString( table->GetTableName() );
		table->WriteStringTable( buf );
	}
}

bool CNetworkStringTableContainerClient::ReadStringTables( CUtlBuffer& buf )
{
	int numTables = buf.GetInt();
	for ( int i = 0 ; i < numTables; i++ )
	{
		char tablename[ 256 ];
		buf.GetString( tablename, sizeof( tablename ) );

		TABLEID id = FindStringTable( tablename );
		if ( id == INVALID_STRING_TABLE )
		{
			Host_Error( "Error reading string tables, no such table %s\n", tablename );
		}

		// Find this table by name
		CNetworkStringTableClient *table = GetTable( id );
		Assert( table );

		// Now read the data for the table
		if ( !table->ReadStringTable( buf ) )
		{
			Host_Error( "Error reading string table %s\n", tablename );
		}

	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_ParseStringTables( void )
{
	g_NetworkStringTableClient.ParseTableDefinitions();

	CL_RegisterResources();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_ParseUpdateStringTable( void )
{
	g_NetworkStringTableClient.ParseUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CL_PrintStringTables( void )
{
	g_NetworkStringTableClient.Dump();
}

void LocalNetworkBackDoor_DirectStringTableUpdate( int tableId, int entryIndex, char const *string,
	int userdatalength, const void *userdata )
{
	CNetworkStringTableClient *table = networkStringTableContainerClient->GetTable( tableId );
	if ( !table )
	{
		Sys_Error( "Bogus table id in network string table backdoor!(%i)", tableId );
	}

	table->DirectUpdate( entryIndex, string, userdatalength, userdata );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CL_WriteStringTables( CUtlBuffer& buf )
{
	g_NetworkStringTableClient.WriteStringTables( buf );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
bool CL_ReadStringTables( CUtlBuffer& buf )
{
	return g_NetworkStringTableClient.ReadStringTables( buf );
}