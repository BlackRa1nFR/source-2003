//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ENGINESINGLEUSERFILTER_H
#define ENGINESINGLEUSERFILTER_H
#ifdef _WIN32
#pragma once
#endif

#include "irecipientfilter.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEngineRecipientFilter : public IRecipientFilter
{
public:
					CEngineRecipientFilter();

	virtual bool	IsReliable( void ) const;

	virtual int		GetRecipientCount( void ) const;
	virtual int		GetRecipientIndex( int slot ) const;

	virtual bool	IsEntityMessage( void ) const
	{
		return false;
	}
	virtual const char *GetEntityMessageName( void ) const
	{
		return "";
	}

	virtual int		GetEntityIndex( void ) const
	{
		return -1;
	}

	virtual bool	IsBroadcastMessage( void );

	virtual bool	IsInitMessage( void ) const
	{
		return false;
	}

public:

	void			Reset( void );

	void			MakeReliable( void );
	
	void			AddAllPlayers( void );
	void			AddRecipientsByPVS( const Vector& origin );
	void			AddRecipientsByPAS( const Vector& origin );
	void			AddRecipient( int playerindex );
	void			RemoveRecipient( int playerindex );

private:
	void			EnsureTruePlayerCount( void );
	void			AddPlayersFromBitMask( unsigned int playerbits );

	bool				m_bReliable;
	int					m_nTruePlayerCount;
	CUtlVector< int >	m_Recipients;
};

//-----------------------------------------------------------------------------
// Purpose: Simple filter for doing MSG_ONE type stuff directly in engine
//-----------------------------------------------------------------------------
class CEngineSingleUserFilter : public IRecipientFilter
{
public:
	CEngineSingleUserFilter( int clientindex )
	{
		m_nClientIndex = clientindex;
	}

	virtual bool	IsReliable( void ) const
	{
		return false;
	}

	virtual int		GetRecipientCount( void ) const
	{
		return 1;
	}

	virtual int		GetRecipientIndex( int slot ) const
	{
		return m_nClientIndex;
	}

	virtual bool	IsEntityMessage( void ) const
	{
		return false;
	}

	virtual int		GetEntityIndex( void ) const
	{
		return -1;
	}

	virtual const char *GetEntityMessageName( void ) const
	{
		return "";
	}

	virtual bool	IsBroadcastMessage( void )
	{
		return false;
	}

	virtual bool	IsInitMessage( void ) const
	{
		return false;
	}

private:
	int				m_nClientIndex;
};

#endif // ENGINESINGLEUSERFILTER_H
