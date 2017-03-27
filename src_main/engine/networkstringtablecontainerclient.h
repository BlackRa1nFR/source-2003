//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef NETWORKSTRINGTABLECONTAINERCLIENT_H
#define NETWORKSTRINGTABLECONTAINERCLIENT_H
#ifdef _WIN32
#pragma once
#endif

#include "inetworkstringtableclient.h"

class CNetworkStringTableClient;
class CUtlBuffer;

//-----------------------------------------------------------------------------
// Purpose: Client implementation of string list
//-----------------------------------------------------------------------------
class CNetworkStringTableContainerClient : public INetworkStringTableClient
{
public:
	// Construction
							CNetworkStringTableContainerClient( void );
							~CNetworkStringTableContainerClient( void );

	// Implement INetworkStringTableClient
	virtual TABLEID			FindStringTable( const char *tableName );
	virtual int				GetNumStrings( TABLEID stringTable );
	virtual int				GetMaxStrings( TABLEID stringTable );
	virtual const char		*GetString( TABLEID stringTable, int stringNumber );
	virtual const void		*GetStringUserData( TABLEID stringTable, int stringNumber, int *length = 0 );
	virtual void			SetStringChangedCallback( void *object, TABLEID stringTable, pfnStringChanged changeFunc );

	virtual int				FindStringIndex( TABLEID stringTable, char const *string );

	virtual char const		*GetTableName( TABLEID stringTable );

	// Table accessors
	TABLEID					AddTable( const char *tableName, int maxentries );
	CNetworkStringTableClient *GetTable( TABLEID stringTable );

	// Parse initial definitions from server ( with initial values )
	void					ParseTableDefinitions( void );
	// Parse updated string data
	void					ParseUpdate( void );
	// Delete all table data
	void					RemoveAllTables( void );
	// Print contents to console
	void					Dump( void );

	void					WriteStringTables( CUtlBuffer& buf );
	bool					ReadStringTables( CUtlBuffer& buf );

private:
	CUtlVector < CNetworkStringTableClient * > m_Tables;
};

extern CNetworkStringTableContainerClient *networkStringTableContainerClient;

#endif // NETWORKSTRINGTABLECONTAINERCLIENT_H
