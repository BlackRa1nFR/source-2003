#ifndef INC_MODH
#define INC_MODH

#include "UtlSymbol.h"

#define NUM_PEAKS 4

#define MOD_INTERNET	0
#define MOD_LAN			1
#define MOD_PROXY		2

#define TYPE_NORMAL		0
#define TYPE_BOTS		1

#define MOD_STATCOUNT ( MOD_PROXY + 1 )

extern char *g_StatNames[];

class CStat
{
public:
	CStat( void )
	{
		players = servers = bots = bots_servers = 0;
	}

	void Add( CStat *other )
	{
		players += other->players;
		servers += other->servers;
		bots    += other->bots;
		bots_servers  += other->bots_servers;
	}

	int			servers;
	int			players;
	int			bots;
	int			bots_servers;
};

class CMod
{
public:
	static int mem( void );
	static int memused;

	CMod();
	~CMod();

	CMod *Copy( void );

	void Reset( void );
	
	void ComputeTotals( void );
	// CMod *next;

	void Add( CMod *other );

	void SetGameDir( const char *dir );
	char const *GetGameDir( void );

	bool ReadFromFile( char **data );
	int WriteToFile( FILE *fp );

// char gamedir[ 64 ];
	
	CStat stats[ MOD_STATCOUNT ];
	CStat total;

	int sv[ NUM_PEAKS ];
	int pl[ NUM_PEAKS ];
	int bots[ NUM_PEAKS ];
	int bots_servers[ NUM_PEAKS ];

private:
	CUtlSymbol		gamedir;
};

class CModList
{
public:
	static int mem( void );
	static int memused;

	CModList();
	~CModList();

	void CheckGrow( int newcount );
	void Reset( void );

	int getCount( void ) { return m_nCount; }
	CMod *getMod( int num )
	{
		if ( num >= m_nCount || num < 0 )
		{
			return NULL;
		}
		else
		{
			return m_ppMods[ num ];
		}
	}

	int		m_nCount;
	int		m_nMaxcount;
	CMod	**m_ppMods;

	void AddMod( CMod *pAdd );
	CMod * FindMod( const char *gamedir );
	void RemoveMod( CMod *pKill );

	void copyData( CModList *from );
};

#endif // !INC_MODH