//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Contains string tools somehow missing from CRT
//=============================================================================

// These are redefined in the project settings to prevent anyone from using them.
// We in this module are of a higher caste and thus are privileged in their use.
#ifdef strncpy
	#undef strncpy
#endif

#ifdef strncat
	#undef strncat
#endif

#ifdef _vsnprintf
#ifdef _WIN32
	#undef _vsnprintf
#endif
#endif

#ifdef _snprintf
	#undef _snprintf
#endif


// NOTE: I have to include stdio + stdarg first so vsnprintf gets compiled in
#include <stdio.h>
#include <stdarg.h>
#ifdef _LINUX
#include <ctype.h>
#endif
#include "tier0/dbg.h"
#include "vstdlib/strtools.h"
#include <string.h>
#include <stdlib.h>
#include "basetypes.h"
#include "utldict.h"

void _Q_memset (const char* file, int line, void *dest, int fill, int count)
{
	Assert( count >= 0 );
	AssertValidWritePtr( dest, count );

	memset(dest,fill,count);
}

void _Q_memcpy (const char* file, int line, void *dest, const void *src, int count)
{
	Assert( count >= 0 );
	AssertValidReadPtr( src, count );
	AssertValidWritePtr( dest, count );

	memcpy( dest, src, count );
}

void _Q_memmove(const char* file, int line, void *dest, const void *src, int count)
{
	Assert( count >= 0 );
	AssertValidReadPtr( src, count );
	AssertValidWritePtr( dest, count );

	memmove( dest, src, count );
}

int _Q_memcmp (const char* file, int line, void *m1, void *m2, int count)
{
	Assert( count >= 0 );
	AssertValidReadPtr( m1, count );
	AssertValidReadPtr( m2, count );

	return memcmp( m1, m2, count );
}

int _Q_strlen (const char* file, int line, const char *str)
{
	AssertValidStringPtr(str);
	return strlen( str );
}

void _Q_strcpy (const char* file, int line, char *dest, const char *src)
{
	AssertValidWritePtr(dest);
	AssertValidStringPtr(src);

	strcpy( dest, src );
}

char *_Q_strrchr(const char* file, int line, const char *s, char c)
{
	AssertValidStringPtr( s );
    int len = Q_strlen(s);
    s += len;
    while (len--)
	if (*--s == c) return (char *)s;
    return 0;
}

int _Q_strcmp (const char* file, int line, const char *s1, const char *s2)
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( s2 );

	while (1)
	{
		if (*s1 != *s2)
			return -1;              // strings not equal    
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}
	
	return -1;
}

int	_Q_stricmp(const char* file, int line,  const char *s1, const char *s2 )
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( s2 );

	return stricmp( s1, s2 );
}


char *_Q_strstr(const char* file, int line,  const char *s1, const char *search )
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( search );

	return strstr( s1, search );
}

char *_Q_strupr (const char* file, int line, char *start)
{
	AssertValidStringPtr( start );
	return strupr( start );
}


char *_Q_strlower (const char* file, int line, char *start)
{
	AssertValidStringPtr( start );
	return strlwr(start);
}


void Q_strcat (char *dest, const char *src)
{
	AssertValidStringPtr(dest);
	AssertValidStringPtr(src);

	dest += Q_strlen(dest);
	Q_strcpy (dest, src);
} 

int Q_strncmp (const char *s1, const char *s2, int count)
{
	Assert( count >= 0 );
	AssertValidStringPtr( s1, count );
	AssertValidStringPtr( s2, count );

	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;              // strings not equal    
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}
	
	return -1;
}

char *Q_strnlwr(char *s, size_t count)
{
	Assert( count >= 0 );
	AssertValidStringPtr( s, count );

	char* pRet = s;
	if (!s)
		return s;

	while (--count >= 0)
	{
		if (!*s)
			break;

		*s = tolower(*s);
		++s;
	}

	return pRet;
}


int Q_strncasecmp (const char *s1, const char *s2, int n)
{
	Assert( n >= 0 );
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( s2 );

	int             c1, c2;
	
	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;               // strings are equal until end point
		
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;              // strings not equal
		}
		if (!c1)
			return 0;               // strings are equal
//              s1++;
//              s2++;
	}
	
	return -1;
}

int Q_strcasecmp (const char *s1, const char *s2)
{
	AssertValidStringPtr(s1);
	AssertValidStringPtr(s2);

	return Q_strncasecmp (s1, s2, 99999);
}

int Q_strnicmp (const char *s1, const char *s2, int n)
{
	Assert( n >= 0 );
	AssertValidStringPtr(s1);
	AssertValidStringPtr(s2);

	return Q_strncasecmp( s1, s2, n );
}

int Q_atoi (const char *str)
{
	AssertValidStringPtr( str );

	int             val;
	int             sign;
	int             c;
	
	Assert( str );
	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;
		
	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}
	
//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}
	
//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}
	
	return 0;
}


float Q_atof (const char *str)
{
	AssertValidStringPtr( str );
	double			val;
	int             sign;
	int             c;
	int             decimal, total;
	
	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;
		
	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}
	
//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}
	
//
// assume decimal
//
	decimal = -1;
	total = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
			break;
		val = val*10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}
	
	return val*sign;
}




//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test
//-----------------------------------------------------------------------------

char const* Q_stristr( char const* pStr, char const* pSearch )
{
	AssertValidStringPtr(pStr);
	AssertValidStringPtr(pSearch);

	if (!pStr || !pSearch) 
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower(*pLetter) == tolower(*pSearch))
		{
			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (tolower(*pMatch) != tolower(*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}

char* Q_stristr( char* pStr, char const* pSearch )
{
	AssertValidStringPtr( pStr );
	AssertValidStringPtr( pSearch );

	return (char*)Q_stristr( (char const*)pStr, pSearch );
}


void Q_strncpy( char *pDest, char const *pSrc, int maxLen )
{
	Assert( maxLen >= 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pSrc );

	strncpy( pDest, pSrc, maxLen );
	if( maxLen )
		pDest[maxLen-1] = 0;
}


int Q_snprintf( char *pDest, int maxLen, char const *pFormat, ... )
{
	Assert( maxLen >= 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pFormat );

	va_list marker;

	va_start( marker, pFormat );
	int len = _vsnprintf( pDest, maxLen, pFormat, marker );
	va_end( marker );

	// Len < 0 represents an overflow
	if( len < 0 )
	{
		len = maxLen;
		pDest[maxLen-1] = 0;
	}

	return len;
}

int Q_vsnprintf( char *pDest, int maxLen, char const *pFormat, va_list params )
{
	Assert( maxLen >= 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pFormat );

	int len = _vsnprintf( pDest, maxLen, pFormat, params );

	if( len < 0 )
	{
		len = maxLen;
		pDest[maxLen-1] = 0;
	}

	return len;
}



char *Q_strncat(char *pDest, const char *pSrc, size_t maxLen)
{
	Assert( maxLen >= 0 );
	AssertValidStringPtr( pDest);
	AssertValidStringPtr( pSrc );
	
	int len = strlen(pDest);
	maxLen = (maxLen - 1) - len;
	
	if ( maxLen <= 0 )
		return pDest;

	char *pOut = strncat( pDest, pSrc, maxLen );
	pOut[len + maxLen] = 0;
	return pOut;
}



//-----------------------------------------------------------------------------
// Purpose: Converts value into x.xx MB/ x.xx KB, x.xx bytes format, including commas
// Input  : value - 
//			2 - 
//			false - 
// Output : char
//-----------------------------------------------------------------------------
#define NUM_PRETIFYMEM_BUFFERS 8
char *Q_pretifymem( float value, int digitsafterdecimal /*= 2*/, bool usebinaryonek /*= false*/ )
{
	static char output[ NUM_PRETIFYMEM_BUFFERS ][ 32 ];
	static int  current;

	float		onekb = usebinaryonek ? 1024.0f : 1000.0f;
	float		onemb = onekb * onekb;

	char *out = output[ current ];
	current = ( current + 1 ) & ( NUM_PRETIFYMEM_BUFFERS -1 );

	char suffix[ 8 ];

	// First figure out which bin to use
	if ( value > onemb )
	{
		value /= onemb;
		sprintf( suffix, " Mb" );
	}
	else if ( value > onekb )
	{
		value /= onekb;
		sprintf( suffix, " Kb" );
	}
	else
	{
		sprintf( suffix, " bytes" );
	}

	char val[ 32 ];

	// Clamp to >= 0
	digitsafterdecimal = max( digitsafterdecimal, 0 );

	// If it's basically integral, don't do any decimals
	if ( FloatMakePositive( value - (int)value ) < 0.00001 )
	{
		sprintf( val, "%i%s", (int)value, suffix );
	}
	else
	{
		char fmt[ 32 ];

		// Otherwise, create a format string for the decimals
		sprintf( fmt, "%%.%if%s", digitsafterdecimal, suffix );
		sprintf( val, fmt, value );
	}

	// Copy from in to out
	char *i = val;
	char *o = out;

	// Search for decimal or if it was integral, find the space after the raw number
	char *dot = strstr( i, "." );
	if ( !dot )
	{
		dot = strstr( i, " " );
	}

	// Compute position of dot
	int pos = dot - i;
	// Don't put a comma if it's <= 3 long
	pos -= 3;

	while ( *i )
	{
		// If pos is still valid then insert a comma every third digit, except if we would be
		//  putting one in the first spot
		if ( pos >= 0 && !( pos % 3 ) )
		{
			// Never in first spot
			if ( o != out )
			{
				*o++ = ',';
			}
		}

		// Count down comma position
		pos--;

		// Copy rest of data as normal
		*o++ = *i++;
	}

	// Terminate
	*o = 0;

	return out;
}
