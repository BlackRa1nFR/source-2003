//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================

#include "vstdlib/ICommandLine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vstdlib/strtools.h"
#include "tier0/dbg.h"


#ifdef _LINUX
#include <limits.h>
#define _MAX_PATH PATH_MAX
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static const int MAX_PARAMETER_LEN = 128;

//-----------------------------------------------------------------------------
// Purpose: Implements ICommandLine
//-----------------------------------------------------------------------------
class CCommandLine : public ICommandLine
{
public:
	// Construction
						CCommandLine( void );
	virtual 			~CCommandLine( void );

	// Implements ICommandLine
	virtual void		CreateCmdLine( const char *commandline  );
	virtual void		CreateCmdLine( int argc, char **argv );
	virtual const char	*GetCmdLine( void ) const;
	virtual	const char	*CheckParm( const char *psz, const char **ppszValue = 0 ) const;

	virtual void		RemoveParm( const char *parm );
	virtual void		AppendParm( const char *pszParm, const char *pszValues );

	virtual int			ParmCount() const;
	virtual int			FindParm( const char *psz ) const;
	virtual const char* GetParm( int nIndex ) const;

	virtual const char	*ParmValue( const char *psz, const char *pDefaultVal = NULL ) const;
	virtual int			ParmValue( const char *psz, int nDefaultVal ) const;
	virtual float		ParmValue( const char *psz, float flDefaultVal ) const;

private:
	enum
	{
		MAX_PARAMETER_LEN = 128,
		MAX_PARAMETERS = 256,
	};

	// When the commandline contains @name, it reads the parameters from that file
	void LoadParametersFromFile( char *&pSrc, char *&pDst );

	// Parse command line...
	void ParseCommandLine();

	// Frees the command line arguments
	void CleanUpParms();

	// Adds an argument..
	void AddArgument( const char *pFirst, const char *pLast );

	// Copy of actual command line
	char *m_pszCmdLine;

	// Pointers to each argument...
	int m_nParmCount;
	char *m_ppParms[MAX_PARAMETERS];
};


//-----------------------------------------------------------------------------
// Instance singleton and expose interface to rest of code
//-----------------------------------------------------------------------------
static CCommandLine g_CmdLine;
ICommandLine *CommandLine()
{
	return &g_CmdLine;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommandLine::CCommandLine( void )
{
	m_pszCmdLine = NULL;
	m_nParmCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommandLine::~CCommandLine( void )
{
	CleanUpParms();
	delete[] m_pszCmdLine;
}


//-----------------------------------------------------------------------------
// Read commandline from file instead...
//-----------------------------------------------------------------------------
void CCommandLine::LoadParametersFromFile( char *&pSrc, char *&pDst )
{
	// Suck out the file name
	char szFileName[ _MAX_PATH ];
	char *pOut;

	// Skip the @ sign
	pSrc++;

	pOut = szFileName;

	while ( *pSrc && *pSrc != ' ' )
	{
		*pOut++ = *pSrc++;
	}

	*pOut = '\0';

	// Skip the space after the file name
	if ( *pSrc )
		pSrc++;

	// Now read in parameters from file
	FILE *fp = fopen( szFileName, "r" );
	if ( fp )
	{
		char c;
		c = (char)fgetc( fp );
		while ( c != EOF )
		{
			// Turn return characters into spaces
			if ( c == '\n' )
				c = ' ';

			*pDst++ = c;

			// Get the next character, if there are more
			c = (char)fgetc( fp );
		}
	
		// Add a terminating space character
		*pDst++ = ' ';

		fclose( fp );
	}
	else
	{
		printf( "Parameter file '%s' not found, skipping...", szFileName );
	}
}


//-----------------------------------------------------------------------------
// Creates a command line from the arguments passed in
//-----------------------------------------------------------------------------
void CCommandLine::CreateCmdLine( int argc, char **argv )
{
	char cmdline[2048];
	cmdline[0] = 0;
	for ( int i = 0; i < argc; ++i )
	{
		strcat( cmdline, argv[i] );
		strcat( cmdline, " " );
	}

	CreateCmdLine( cmdline );
}


//-----------------------------------------------------------------------------
// Purpose: Create a command line from the passed in string
//  Note that if you pass in a @filename, then the routine will read settings
//  from a file instead of the command line
//-----------------------------------------------------------------------------
void CCommandLine::CreateCmdLine( const char *commandline )
{
	if (m_pszCmdLine)
		delete[] m_pszCmdLine;

	char szFull[ 4096 ];

	int len = strlen( commandline ) + 1;
	char *pOrig = (char*)stackalloc( len );
	memcpy( pOrig, commandline, len );

	char *pSrc, *pDst;

	pDst = szFull;
	pSrc = pOrig;

	while ( *pSrc )
	{
		if ( *pSrc == '@' )
		{
			LoadParametersFromFile( pSrc, pDst );
			continue;
		}

		*pDst++ = *pSrc++;
	}

	*pDst = '\0';

	len = strlen( szFull ) + 1;
	m_pszCmdLine = new char[len];
	memcpy( m_pszCmdLine, szFull, len );

	ParseCommandLine();
}


//-----------------------------------------------------------------------------
// Purpose: Remove specified string ( and any args attached to it ) from command line
// Input  : *pszParm - 
//-----------------------------------------------------------------------------
void CCommandLine::RemoveParm( const char *pszParm )
{
	if ( !m_pszCmdLine )
		return;

	// Search for first occurrence of pszParm
	char *p, *found;
	char *pnextparam;
	int n;
	int curlen;
	int curpos;

	while ( 1 )
	{
		p = m_pszCmdLine;
		curlen = strlen( p );

		found = strstr( p, pszParm );
		if ( !found )
			break;

		curpos = found - p;

		pnextparam = found + 1;
		while ( pnextparam && *pnextparam && (*pnextparam != '-') && (*pnextparam != '+') )
			pnextparam++;

		if ( pnextparam && *pnextparam )
		{
			// We are either at the end of the string, or at the next param.  Just chop out the current param.
			n = curlen - ( pnextparam - p ); // # of characters after this param.
		
			memcpy( found, pnextparam, n );

			found[n] = '\0';
		}
		else
		{
			// Clear out rest of string.
			n = pnextparam - found;
			memset( found, 0, n );
		}
	}

	// Strip and trailing ' ' characters left over.
	while ( m_pszCmdLine[ strlen( m_pszCmdLine ) - 1 ] == ' ' )
	{
		m_pszCmdLine[ strlen( m_pszCmdLine ) - 1 ] = '\0';
	}

	ParseCommandLine();
}


//-----------------------------------------------------------------------------
// Purpose: Append parameter and argument values to command line
// Input  : *pszParm - 
//			*pszValues - 
//-----------------------------------------------------------------------------
void CCommandLine::AppendParm( const char *pszParm, const char *pszValues )
{
	int nNewLength = 0;
	char *pCmdString;

	nNewLength = strlen( pszParm );            // Parameter.
	if ( pszValues )
		nNewLength += strlen( pszValues ) + 1;  // Values + leading space character.
	nNewLength++; // Terminal 0;

	if ( !m_pszCmdLine )
	{
		m_pszCmdLine = new char[ nNewLength ];
		strcpy( m_pszCmdLine, pszParm );
		if ( pszValues )
		{
			strcat( m_pszCmdLine, " " );
			strcat( m_pszCmdLine, pszValues );
		}

		ParseCommandLine();
		return;
	}

	// Remove any remnants from the current Cmd Line.
	RemoveParm( pszParm );

	nNewLength += strlen( m_pszCmdLine ) + 1 + 1;

	pCmdString = new char[ nNewLength ];
	memset( pCmdString, 0, nNewLength );

	strcpy ( pCmdString, m_pszCmdLine ); // Copy old command line.
	strcat ( pCmdString, " " ); // Put in a space
	strcat ( pCmdString, pszParm );
	if ( pszValues )
	{
		strcat( pCmdString, " " );
		strcat( pCmdString, pszValues );
	}

	// Kill off the old one
	delete[] m_pszCmdLine;

	// Point at the new command line.
	m_pszCmdLine = pCmdString;

	ParseCommandLine();
}


//-----------------------------------------------------------------------------
// Purpose: Return current command line
// Output : const char
//-----------------------------------------------------------------------------
const char *CCommandLine::GetCmdLine( void ) const
{
	return m_pszCmdLine;
}


//-----------------------------------------------------------------------------
// Purpose: Search for the parameter in the current commandline
// Input  : *psz - 
//			**ppszValue - 
// Output : char
//-----------------------------------------------------------------------------
const char *CCommandLine::CheckParm( const char *psz, const char **ppszValue ) const
{
	static char sz[ MAX_PARAMETER_LEN ];
	char *pret;

	if ( !m_pszCmdLine )
	{
		return NULL;
	}

	// Point to first occurrence
	pret = strstr( m_pszCmdLine, psz );

	// should we return a pointer to the value?
	if ( pret && ppszValue )
	{
		char *p1 = pret;
		*ppszValue = NULL;

		while ( *p1 && ( *p1 != 32 ) )
		{
			p1++;
		}

		if ( ( *p1 != 0 ) && ( *p1 != '+' ) && ( *p1 != '-' ) )
		{
			char *p2 = ++p1;

			int i;
			for ( i = 0; i < MAX_PARAMETER_LEN ; i++ )
			{
				if ( !*p2 || ( *p2 == 32 ) )
					break;
				sz[i] = *p2++;
			}

			sz[i] = 0;
			*ppszValue = &sz[0];		
		}	
	}

	return pret;
}


//-----------------------------------------------------------------------------
// Adds an argument..
//-----------------------------------------------------------------------------
void CCommandLine::AddArgument( const char *pFirst, const char *pLast )
{
	if ( pLast == pFirst )
		return;

	Assert( m_nParmCount < MAX_PARAMETERS );

	int nLen = (int)pLast - (int)pFirst + 1;
	m_ppParms[m_nParmCount] = new char[nLen];
	memcpy( m_ppParms[m_nParmCount], pFirst, nLen - 1 );
	m_ppParms[m_nParmCount][nLen - 1] = 0;

	++m_nParmCount;
}


//-----------------------------------------------------------------------------
// Parse command line...
//-----------------------------------------------------------------------------
void CCommandLine::ParseCommandLine()
{
	CleanUpParms();
	if (!m_pszCmdLine)
		return;

	const char *pChar = m_pszCmdLine;
	while ( *pChar && isspace(*pChar) )
	{
		++pChar;
	}

	bool bInQuotes = false;
	const char *pFirstLetter = NULL;
	for ( ; *pChar; ++pChar )
	{
		if ( bInQuotes )
		{
			if ( *pChar != '\"' )
				continue;

			AddArgument( pFirstLetter, pChar );
			pFirstLetter = NULL;
			bInQuotes = false;
			continue;
		}

		// Haven't started a word yet...
		if ( !pFirstLetter )
		{
			if ( *pChar == '\"' )
			{
				bInQuotes = true;
				pFirstLetter = pChar + 1;
				continue;
			}

			if ( isspace( *pChar ) )
				continue;

			pFirstLetter = pChar;
			continue;
		}

		// Here, we're in the middle of a word. Look for the end of it.
		if ( isspace( *pChar ) )
		{
			AddArgument( pFirstLetter, pChar );
			pFirstLetter = NULL;
		}
	}

	if ( pFirstLetter )
	{
		AddArgument( pFirstLetter, pChar );
	}
}


//-----------------------------------------------------------------------------
// Individual command line arguments
//-----------------------------------------------------------------------------
void CCommandLine::CleanUpParms()
{
	for ( int i = 0; i < m_nParmCount; ++i )
	{
		delete [] m_ppParms[i];
		m_ppParms[i] = NULL;
	}
	m_nParmCount = 0;
}


//-----------------------------------------------------------------------------
// Returns individual command line arguments
//-----------------------------------------------------------------------------
int CCommandLine::ParmCount() const
{
	return m_nParmCount;
}

int CCommandLine::FindParm( const char *psz ) const
{
	// Start at 1 so as to not search the exe name
	for ( int i = 1; i < m_nParmCount; ++i )
	{
		if ( !Q_stricmp( psz, m_ppParms[i] ) )
			return i;
	}
	return 0;
}

const char* CCommandLine::GetParm( int nIndex ) const
{
	Assert( (nIndex >= 0) && (nIndex < m_nParmCount) );
	if ( (nIndex < 0) || (nIndex >= m_nParmCount) )
		return "";
	return m_ppParms[nIndex];
}


//-----------------------------------------------------------------------------
// Returns the argument after the one specified, or the default if not found
//-----------------------------------------------------------------------------
const char *CCommandLine::ParmValue( const char *psz, const char *pDefaultVal ) const
{
	int nIndex = FindParm( psz );
	if (( nIndex == 0 ) || (nIndex == m_nParmCount - 1))
		return pDefaultVal;

	// Probably another cmdline parameter instead of a valid arg if it starts with '+' or '-'
	if ( m_ppParms[nIndex + 1][0] == '-' || m_ppParms[nIndex + 1][0] == '+' )
		return pDefaultVal;

	return m_ppParms[nIndex + 1];
}

int	CCommandLine::ParmValue( const char *psz, int nDefaultVal ) const
{
	int nIndex = FindParm( psz );
	if (( nIndex == 0 ) || (nIndex == m_nParmCount - 1))
		return nDefaultVal;

	// Probably another cmdline parameter instead of a valid arg if it starts with '+' or '-'
	if ( m_ppParms[nIndex + 1][0] == '-' || m_ppParms[nIndex + 1][0] == '+' )
		return nDefaultVal;

	return atoi( m_ppParms[nIndex + 1] );
}

float CCommandLine::ParmValue( const char *psz, float flDefaultVal ) const
{
	int nIndex = FindParm( psz );
	if (( nIndex == 0 ) || (nIndex == m_nParmCount - 1))
		return flDefaultVal;

	// Probably another cmdline parameter instead of a valid arg if it starts with '+' or '-'
	if ( m_ppParms[nIndex + 1][0] == '-' || m_ppParms[nIndex + 1][0] == '+' )
		return flDefaultVal;

	return atof( m_ppParms[nIndex + 1] );
}
