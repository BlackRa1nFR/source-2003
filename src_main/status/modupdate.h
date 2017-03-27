#if !defined( MODUPDATEH )
#define MODUPDATEH
#pragma once

#include "mod.h"

class CModUpdate
{
public:
	static int mem( void )
	{
		return memused;
	}

	static int memused;

	CModUpdate( void )
	{
		next = NULL;
		realtime = 0;
		memset( stats, 0, MOD_STATCOUNT * sizeof( CStat ) );
		memset( &total, 0, sizeof( CStat ) );
		Mods.Reset();

		m_bLoadedFromFile = false;
		m_bIsPartialDataSet = false;

		memused += sizeof( *this );
	}

	~CModUpdate( void )
	{
		Mods.Reset();

		memused -= sizeof( *this );
	}

	void Add( CMod *mod )
	{
		total.Add( &mod->total );
		for ( int i = 0 ; i < MOD_STATCOUNT; i++ )
		{
			stats[ i ].Add( &mod->stats[ i ] );
		}
		ComputeTotals();
	}

	void ComputeTotals( void )
	{
		total.players = total.servers = total.bots = total.bots_servers = 0;
		for ( int i = 0 ; i < MOD_STATCOUNT; i++ )
		{
			total.players += stats[ i ].players;
			total.servers += stats[ i ].servers;
			total.bots    += stats[ i ].bots;
			total.bots_servers    += stats[ i ].bots_servers;
		}
	}

	void KillData( void );


	// Next in chain
	CModUpdate *next;

	// Time sample was taken
	time_t	realtime;

	CStat stats[ MOD_STATCOUNT ];
	CStat total;

	// Active Mods at this time
	CModList Mods;

	bool		m_bLoadedFromFile;

	// If load partial date, never save it back out to file
	bool		m_bIsPartialDataSet;
};

class CModStats
{
public:
	CModStats( void )
	{
		list = NULL;
		count = 0;
	}

	~CModStats( void )
	{
		Clear();
	}

	void Clear( void )
	{
		CModUpdate *p, *n;
		p = list;
		while ( p )
		{
			n = p->next;
			delete p;
			p = n;
		}
		list = NULL;
		count = 0;
	}

	CModUpdate		*list;
	int				count;
};

#endif