//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef INETWORKSTRINGTABLECLIENT_H
#define INETWORKSTRINGTABLECLIENT_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "networkstringtabledefs.h"

typedef void (*pfnStringChanged)( void *object, TABLEID stringTable, int stringNumber, char const *newString, void const *newData );

//-----------------------------------------------------------------------------
// Purpose: Client DLL interface to shared string tables
//-----------------------------------------------------------------------------
class INetworkStringTableClient
{
public:
	// Find TABLEID for named table
	virtual TABLEID			FindStringTable( const char *tableName ) = 0;

	virtual char const		*GetTableName( TABLEID stringTable ) = 0;
	// Accessors
	virtual int				GetNumStrings( TABLEID stringTable ) = 0;
	virtual int				GetMaxStrings( TABLEID stringTable ) = 0;

	// Retrieve the value of a string in the specified string table
	virtual const char		*GetString( TABLEID stringTable, int stringNumber ) = 0;
	virtual const void		*GetStringUserData( TABLEID stringTable, int stringNumber, int *length = 0 ) = 0;

	virtual void			SetStringChangedCallback( void *object, TABLEID stringTable, pfnStringChanged changeFunc ) = 0;

	virtual int				FindStringIndex( TABLEID stringTable, char const *string ) = 0;
};

#define INTERFACENAME_NETWORKSTRINGTABLECLIENT "VEngineClientStringTable001"

#endif // INETWORKSTRINGTABLECLIENT_H
