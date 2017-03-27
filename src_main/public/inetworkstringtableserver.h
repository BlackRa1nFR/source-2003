//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef INETWORKSTRINGTABLESERVER_H
#define INETWORKSTRINGTABLESERVER_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "networkstringtabledefs.h"

//-----------------------------------------------------------------------------
// Purpose: Game .dll shared string table interfaces
//-----------------------------------------------------------------------------
class INetworkStringTableServer
{
public:
	// Create table with specified name and max size
	virtual TABLEID			CreateStringTable( const char *tableName, int maxentries ) = 0;
	// Access data about the created tables
	virtual int				GetNumStrings( TABLEID stringTable ) = 0;
	virtual int				GetMaxStrings( TABLEID stringTable ) = 0;

	// Add a string to a table
	virtual int				AddString( TABLEID stringTable, const char *value, int length = 0, void const *userdata = 0 ) = 0;
	// Set the value of a string already in a table
	virtual void			SetString( TABLEID stringTable, int stringNumber, const char *value ) = 0;
	// Set the value of a string already in a table
	virtual void			SetStringUserData( TABLEID stringTable, int stringNumber, int length, const void *userdata ) = 0;
	// Retrieve the value of a string
	virtual const char		*GetString( TABLEID stringTable, int stringNumber ) = 0;
	virtual const void		*GetStringUserData( TABLEID stringTable, int stringNumber, int *length = 0 ) = 0;

	virtual int				FindStringIndex( TABLEID stringTable, char const *string ) = 0;

	virtual const char		*GetTableName( TABLEID stringTable ) = 0;
};

#define INTERFACENAME_NETWORKSTRINGTABLESERVER "VEngineServerStringTable001"

#endif // INETWORKSTRINGTABLESERVER_H
