//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "networkstringtable.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//			*tableName - 
//			maxentries - 
//-----------------------------------------------------------------------------
CNetworkStringTable::CNetworkStringTable( TABLEID id, const char *tableName, int maxentries )
{
	m_id = id;
	m_pszTableName = new char[ strlen( tableName ) + 1 ];
	assert( m_pszTableName );
	assert( tableName );
	strcpy( m_pszTableName, tableName );

	m_nMaxEntries = maxentries;
	m_nEntryBits = Q_log2( m_nMaxEntries );

	// Make sure maxentries is power of 2
	if ( ( 1 << m_nEntryBits ) != maxentries )
	{
		Host_Error( "String tables must be powers of two in size!, %i is not a power of 2\n", maxentries );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetworkStringTable::~CNetworkStringTable( void )
{
	delete[] m_pszTableName;

	DeleteAllStrings();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTable::DeleteAllStrings( void )
{
	m_Items.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : i - 
// Output : CNetworkStringTableItem
//-----------------------------------------------------------------------------
CNetworkStringTableItem *CNetworkStringTable::GetItem( int i )
{
	return &m_Items[ i ];
}

//-----------------------------------------------------------------------------
// Purpose: Returns the table identifier
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CNetworkStringTable::GetTableId( void )
{
	return m_id;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the max size of the table
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTable::GetMaxEntries( void )
{
	return m_nMaxEntries;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a table, by name
// Output : const char
//-----------------------------------------------------------------------------
const char *CNetworkStringTable::GetTableName( void )
{
	return m_pszTableName;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of bits needed to encode an entry index
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTable::GetEntryBits( void )
{
	return m_nEntryBits;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTable::AddString( const char *value, int length /*= 0*/, const void *userdata /*= 0*/ )
{
	if ( !value )
	{
		Con_Printf( "Warning:  Can't add NULL string to table %s\n", GetTableName() );
		return INVALID_STRING_INDEX;
	}

	// See if it's already there
	int i = m_Items.Find( value );
	if ( m_Items.IsValidIndex( i ) )
	{
		return i;
	}

	if ( m_Items.Count() >= (unsigned int)GetMaxEntries() )
	{
		// Too many strings, FIXME: Print warning message
		Con_Printf( "Warning:  Table %s is full, can't add %s\n", GetTableName(), value );
		return INVALID_STRING_INDEX;
	}

	CNetworkStringTableItem	newItem;
	
	i = m_Items.Insert( value, newItem );

	CNetworkStringTableItem *temp = &m_Items[ i ];

	temp->SetUserData( length, userdata );

	DataChanged( i, temp );

	return i;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringNumber - 
//			*value - 
//-----------------------------------------------------------------------------
void CNetworkStringTable::SetString( int stringNumber, const char *value )
{
	Assert( stringNumber >= 0 && stringNumber < (int)m_Items.Count() );
	CNetworkStringTableItem *p = &m_Items[ stringNumber ];

	if ( !stricmp( m_Items.GetElementName( stringNumber ), value ) )
		return;

	m_Items.SetElementName( stringNumber, value );

	DataChanged( stringNumber, p );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringNumber - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CNetworkStringTable::GetString( int stringNumber )
{
	assert( stringNumber >= 0 && stringNumber < (int)m_Items.Count() );
	return m_Items.GetElementName( stringNumber );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringNumber - 
//			length - 
//			*userdata - 
//-----------------------------------------------------------------------------
void CNetworkStringTable::SetStringUserData( int stringNumber, int length /*=0*/, const void *userdata /*= 0*/ )
{
	CNetworkStringTableItem *p;

	assert( stringNumber >= 0 && stringNumber < (int)m_Items.Count() );
	p = &m_Items[ stringNumber ];
	assert( p );

	if ( p->SetUserData( length, userdata ) )
	{
		// Mark changed
		DataChanged( stringNumber, p );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringNumber - 
//			length - 
// Output : const void
//-----------------------------------------------------------------------------
const void *CNetworkStringTable::GetStringUserData( int stringNumber, int *length )
{
	CNetworkStringTableItem *p;

	assert( stringNumber >= 0 && stringNumber < (int)m_Items.Count() );
	p = &m_Items[ stringNumber ];
	assert( p );
	return p->GetUserData( length );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTable::GetNumStrings( void )
{
	return m_Items.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringTable - 
//			*string - 
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTable::FindStringIndex( char const *string )
{
	int i = m_Items.Find( string );
	if ( m_Items.IsValidIndex( i ) )
		return i;
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetworkStringTable::Dump( void )
{
	Con_Printf( "Table %s\n", GetTableName() );
	Con_Printf( "  %i/%i items\n", GetNumStrings(), GetMaxEntries() );
	for ( int i = 0; i < GetNumStrings() ; i++ )
	{
		Con_Printf( "  %i : %s\n", i, GetString( i ) );
	}
	Con_Printf( "\n" );
}
