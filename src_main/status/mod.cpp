#include <assert.h>
#include "stdafx.h"
#include "Status.h"
#include "mod.h"
#include "modupdate.h"

char *g_StatNames[] =
{
	"net",
	"lan",
	"proxy"
};

CMod::CMod()
{
	gamedir = UTL_INVAL_SYMBOL;

	memset( sv, 0, NUM_PEAKS * sizeof( int ) );
	memset( pl, 0, NUM_PEAKS * sizeof( int ) );

	Reset();

	memused += sizeof( *this );
}

CMod::~CMod()
{
	memused -= sizeof( *this );
}


int CMod::mem( void )
{
	return memused;
}

int CMod::memused = 0;

CMod *CMod::Copy( void )
{
	CMod *c = new CMod;
	c->total = total;
	memcpy( c->stats, stats, MOD_STATCOUNT * sizeof( CStat ) );
	c->gamedir = gamedir;

	return c;
}

void CMod::Reset( void )
{
	memset( stats, 0, MOD_STATCOUNT * sizeof( CStat ) );
	memset( &total, 0, sizeof( CStat ) );
}

void CMod::Add( CMod *other )
{
	total.Add( &other->total );
	for ( int i = 0 ; i < MOD_STATCOUNT; i++ )
	{
		stats[ i ].Add( &other->stats[ i ] );
	}
	ComputeTotals();
}

void CMod::ComputeTotals( void )
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

static CUtlSymbolTable g_GameDirTable(0);

void CMod::SetGameDir( const char *dir )
{
	CUtlSymbol lookup;

	lookup = g_GameDirTable.Find( dir );
	if ( lookup != UTL_INVAL_SYMBOL )
	{
		gamedir = lookup;
	}
	else
	{
		gamedir = g_GameDirTable.AddString( dir );
	}
}

char const *CMod::GetGameDir( void )
{
	assert( gamedir != UTL_INVAL_SYMBOL );
	return g_GameDirTable.String( gamedir );
}

int CMod::WriteToFile(FILE *fp)
{
	fprintf( fp, "\t\t{\n" );

	fprintf( fp, "\t\t\tgamedir \"%s\"\n", GetGameDir() );
	fprintf( fp, "\t\t\t%i\n", MOD_STATCOUNT );
	for ( int i = 0; i < MOD_STATCOUNT; i++ )
	{
		fprintf( fp, "\t\t\t%i %i %i %i\n", stats[ i ].servers, stats[ i ].players, stats[ i ].bots, stats[ i ].bots_servers );
	}

	fprintf( fp, "\t\t}\n" );

	return 1;
}

extern char	com_token[1024];
extern char *COM_Parse (char *data);

bool CMod::ReadFromFile( char **data )
{
	int i;
	int count = 0;
	char *p = *data;
	bool bret = false;

	p = COM_Parse( p );
	if ( stricmp( com_token, "{" ) )
	{
		goto finish_read;
	}

	p = COM_Parse( p );
	if ( !stricmp( com_token, "}" ) )
	{
		goto finish_read;
	}

	if ( stricmp( com_token, "gamedir" ) )
	{
		goto finish_read;
	}

	p = COM_Parse( p );

	SetGameDir( com_token );

	p = COM_Parse( p );

	count = atoi( com_token );
	assert( count <= MOD_STATCOUNT );

	for ( i = 0; i < count; i++ )
	{
		p = COM_Parse( p );

		stats[ i ].servers = atoi( com_token );

		p = COM_Parse( p );

		stats[ i ].players = atoi( com_token );
	
		if ( *p == '\r' ) // old file format
		{
			stats[ i ].bots  = stats[ i ].bots_servers = 0;
		}
		else
		{

			p = COM_Parse( p );

			stats[ i ].bots = atoi( com_token );
		
			p = COM_Parse( p );

			stats[ i ].bots_servers = atoi( com_token );
		}
	}

	p = COM_Parse( p );
	if ( stricmp( com_token, "}" ) )
	{
		goto finish_read;
	}

	ComputeTotals();

	bret = true;
finish_read:

	*data = p;
	return bret;
}

void CModList::Reset( void )
{
	if ( m_ppMods )
	{
		for ( int i = 0; i < m_nCount; i++ )
		{
			delete m_ppMods[ i ];
		}
		delete[] m_ppMods;
		m_ppMods = NULL;
	}
	m_nCount = 0;
	m_nMaxcount = 0;
}

CModList::CModList( void )
{ 
	m_nCount = 0; 
	m_nMaxcount = 0; 
	m_ppMods = NULL; 
	
	memused += sizeof( *this );
}

CModList::~CModList( void )
{
	Reset();

	memused -= sizeof( *this );
}

int CModList::mem( void )
{
	return memused;
}

int CModList::memused = 0;

void CModList::CheckGrow( int newcount )
{
	if ( newcount <= m_nMaxcount )
		return;
	
	// Reallocate arrays
	int oldmaxcount = m_nMaxcount;
	m_nMaxcount = newcount;
	CMod **pNew = new CMod *[ m_nMaxcount ];
	if ( oldmaxcount != 0 )
	{
		for ( int i = 0; i < oldmaxcount; i++ )
		{
			pNew[ i ] = m_ppMods[ i ];
		}

		for ( ; i < m_nMaxcount; i++ )
		{
			pNew[ i ] = NULL;
		}
	}
	delete[] m_ppMods;
	m_ppMods = pNew;
}

// Sort it by game directory:
int __cdecl FnGameDirectoryCompare(const void *elem1, const void *elem2 )
{
	CMod *p1, *p2;
	p1 = *(CMod **)elem1;
	p2 = *(CMod **)elem2;

	if ( !p1 || !p2 )  // No meaningful comparison
	{
		return 0;  
	}

	return stricmp( p1->GetGameDir(), p2->GetGameDir() );
}

// Sort it by game directory:
int __cdecl FnGameDirectorySearch(const void *elem1, const void *elem2 )
{
	char *str;
	CMod *p2;
	str = (char *)elem1;
	p2 = *(CMod **)elem2;

	if ( !str || !p2 )  // No meaningful comparison
	{
		return 0;  
	}

	return stricmp( str, p2->GetGameDir() );
}

void CModList::AddMod( CMod *pAdd )
{
	if ( !pAdd )
		return;

	CheckGrow( m_nCount+1 );

	// Add to end and re-sort
	//
	m_ppMods[ m_nCount ] = pAdd;
	m_nCount++;

	// Quicksort it
	if ( m_nCount < 2 )
		return;
	//
	// Now do that actual sorting.
	size_t nCount = m_nCount;
	size_t nSize  = sizeof(CMod *);

	qsort(
		m_ppMods,
		(size_t)nCount,
		(size_t)nSize,
		FnGameDirectoryCompare
	);
}

CMod * CModList::FindMod( const char *gamedir )
{
	CMod **p;
	// Bsearc it
	//
	// Now do that actual sorting.
	size_t nCount = m_nCount;
	size_t nSize  = sizeof(CMod *);

	if ( nCount == 0 )
		return NULL;
	if ( nCount == 1 )
	{
		if ( !stricmp( gamedir, m_ppMods[ 0 ]->GetGameDir() ) )
			return m_ppMods[ 0 ];
		else
			return NULL;
	}

	p = ( CMod **)bsearch( (const void *)gamedir, (const void *)m_ppMods,
		(size_t)nCount,
		(size_t)nSize,
		FnGameDirectorySearch
	);

	if ( !p )
		return NULL;
	return *p;
}

void CModList::RemoveMod( CMod *pkill )
{
	int i;

	for ( i = 0 ; i < m_nCount;  i++ )
	{
		if ( m_ppMods[ i ] == pkill )
			break;
	}

	if ( i >= m_nCount )
		return;

	for ( ; i < m_nCount - 1; i++ )
	{
		m_ppMods[ i ] = m_ppMods[ i + 1 ];
	}
	m_nCount--;
	m_ppMods[ m_nCount ] = NULL;
}

void CModList::copyData( CModList *from )
{
	// Copy data and counts, etc
	Reset(); // Remove any current data

	int i;
	CMod *p;
	for ( i = from->getCount()-1; i >= 0 ; i-- )
	{
		p = from->getMod( i );
		from->RemoveMod( p );
		AddMod( p );
	}
}

int CModUpdate::memused = 0;
