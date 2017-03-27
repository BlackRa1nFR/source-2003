//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef IRECIPIENTFILTER_H
#define IRECIPIENTFILTER_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Generic interface for routing messages to users
//-----------------------------------------------------------------------------
class IRecipientFilter
{
public:
	virtual			~IRecipientFilter() {}

	virtual bool	IsReliable( void ) const = 0;

	virtual int		GetRecipientCount( void ) const = 0;
	virtual int		GetRecipientIndex( int slot ) const = 0;

	virtual bool	IsEntityMessage( void ) const = 0;
	virtual int		GetEntityIndex( void ) const = 0;
	virtual const char *GetEntityMessageName( void ) const = 0;

	virtual bool	IsBroadcastMessage( void ) = 0;

	virtual bool	IsInitMessage( void ) const = 0;
};

#endif // IRECIPIENTFILTER_H
