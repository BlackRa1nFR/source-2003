//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef NETWORKSTRINGTABLE_H
#define NETWORKSTRINGTABLE_H
#ifdef _WIN32
#pragma once
#endif

#include "networkstringtabledefs.h"
#include "networkstringtableitem.h"

#include "utldict.h"

//-----------------------------------------------------------------------------
// Purpose: Client/Server shared string table definition
//-----------------------------------------------------------------------------
class CNetworkStringTable
{
public:
	// Construction
							CNetworkStringTable( TABLEID id, const char *tableName, int maxentries );
	virtual					~CNetworkStringTable( void );

	// Accessors
	virtual int				AddString( const char *value, int length = 0, const void *userdata = 0 );
	virtual void			SetString( int stringNumber, const char *value );
	virtual const char		*GetString( int stringNumber );
	virtual void			SetStringUserData( int stringNumber, int length = 0, const void *userdata = 0 );
	virtual const void		*GetStringUserData( int stringNumber, int *length );
	virtual int				FindStringIndex( char const *string );
	virtual int				GetNumStrings( void );

	virtual void			DataChanged( int stringNumber, CNetworkStringTableItem *item ) = 0;

	virtual	CNetworkStringTableItem *GetItem( int i );

	virtual void			Dump( void );

	// Destroy string table
	void					DeleteAllStrings( void );

	// Table info
	TABLEID					GetTableId( void );
	int						GetMaxEntries( void );
	const char				*GetTableName( void );
	int						GetEntryBits( void );

private:
	CNetworkStringTable( const CNetworkStringTable & ); // not implemented, not allowed

	TABLEID					m_id;
	char					*m_pszTableName;
	// Must be a power of 2, so encoding can determine # of bits to use based on log2
	int						m_nMaxEntries;
	int						m_nEntryBits;

	CUtlDict< CNetworkStringTableItem, int > m_Items;
};

#endif // NETWORKSTRINGTABLE_H
