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

#ifndef VSTDLIB_STRTOOLS_H
#define VSTDLIB_STRTOOLS_H

#ifdef _WIN32
#pragma once
#elif _LINUX
#include <ctype.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "vstdlib/vstdlib.h"

//-----------------------------------------------------------------------------
// Portable versions of standard string functions
//-----------------------------------------------------------------------------
VSTDLIB_INTERFACE void	_Q_memset (const char* file, int line, void *dest, int fill, int count);
VSTDLIB_INTERFACE void	_Q_memcpy (const char* file, int line, void *dest, const void *src, int count);
VSTDLIB_INTERFACE void  _Q_memmove(const char* file, int line, void *dest, const void *src, int count);
VSTDLIB_INTERFACE int	_Q_memcmp (const char* file, int line, void *m1, void *m2, int count);
VSTDLIB_INTERFACE int	_Q_strlen (const char* file, int line, const char *str);
VSTDLIB_INTERFACE void	_Q_strcpy (const char* file, int line, char *dest, const char *src);
VSTDLIB_INTERFACE char*	_Q_strrchr (const char* file, int line, const char *s, char c);
VSTDLIB_INTERFACE int	_Q_strcmp (const char* file, int line, const char *s1, const char *s2);
VSTDLIB_INTERFACE int	_Q_stricmp( const char* file, int line, const char *s1, const char *s2 );
VSTDLIB_INTERFACE char*	_Q_strstr( const char* file, int line, const char *s1, const char *search );
VSTDLIB_INTERFACE char*	_Q_strupr (const char* file, int line, char *start);
VSTDLIB_INTERFACE char*	_Q_strlower (const char* file, int line, char *start);

#ifdef _DEBUG

	#define Q_memset(dest, fill, count)		_Q_memset   (__FILE__, __LINE__, (dest), (fill), (count))	
	#define Q_memcpy(dest, src, count)		_Q_memcpy	(__FILE__, __LINE__, (dest), (src), (count))	
	#define Q_memmove(dest, src, count)		_Q_memmove	(__FILE__, __LINE__, (dest), (src), (count))	
	#define Q_memcmp(m1, m2, count)			_Q_memcmp	(__FILE__, __LINE__, (m1), (m2), (count))		
	#define Q_strlen(str)					_Q_strlen	(__FILE__, __LINE__, (str))				
	#define Q_strcpy(dest, src)				_Q_strcpy	(__FILE__, __LINE__, (dest), (src))			
	#define Q_strrchr(s, c)					_Q_strrchr	(__FILE__, __LINE__, (s), (c))				
	#define Q_strcmp(s1, s2)				_Q_strcmp	(__FILE__, __LINE__, (s1), (s2))			
	#define Q_stricmp(s1, s2 )				_Q_stricmp	(__FILE__, __LINE__, (s1), (s2) )			
	#define Q_strstr(s1, search )			_Q_strstr	(__FILE__, __LINE__, (s1), (search) )		
	#define Q_strupr(start)					_Q_strupr	(__FILE__, __LINE__, (start))				
	#define Q_strlower(start)				_Q_strlower (__FILE__, __LINE__, (start))				

#else

#ifdef _LINUX
inline char *strupr( char *start )
{
      char *str = start;
      while( str && *str )
      {
              *str = (char)toupper(*str);
              str++;
      }
      return start;
}

inline char *strlwr( char *start )
{
      char *str = start;
      while( str && *str )
      {
              *str = (char)tolower(*str);
              str++;
      }
      return start;
}

#endif

inline void		Q_memset (void *dest, int fill, int count)			{ memset( dest, fill, count ); }
inline void		Q_memcpy (void *dest, const void *src, int count)	{ memcpy( dest, src, count ); }
inline void		Q_memmove (void *dest, const void *src, int count)	{ memmove( dest, src, count ); }
inline int		Q_memcmp (void *m1, void *m2, int count)			{ return memcmp( m1, m2, count ); } 
inline int		Q_strlen (const char *str)							{ return strlen ( str ); }
inline void		Q_strcpy (char *dest, const char *src)				{ strcpy( dest, src ); }
inline char*	Q_strrchr (const char *s, char c)					{ return strrchr( s, c ); }
inline int		Q_strcmp (const char *s1, const char *s2)			{ return strcmp( s1, s2 ); }
inline int		Q_stricmp( const char *s1, const char *s2 )			{ return stricmp( s1, s2 ); }
inline char*	Q_strstr( const char *s1, const char *search )		{ return strstr( s1, search ); }
inline char*	Q_strupr (char *start)								{ return strupr( start ); }
inline char*	Q_strlower (char *start)							{ return strlwr( start ); }

#endif

VSTDLIB_INTERFACE int	Q_strncmp (const char *s1, const char *s2, int count);
VSTDLIB_INTERFACE void	Q_strcat (char *dest, const char *src);
VSTDLIB_INTERFACE int	Q_strcasecmp (const char *s1, const char *s2);
VSTDLIB_INTERFACE int	Q_strncasecmp (const char *s1, const char *s2, int n);
VSTDLIB_INTERFACE int	Q_strnicmp (const char *s1, const char *s2, int n);
VSTDLIB_INTERFACE int	Q_atoi (const char *str);
VSTDLIB_INTERFACE float	Q_atof (const char *str);
VSTDLIB_INTERFACE char*	Q_stristr( char* pStr, char const* pSearch );
VSTDLIB_OVERLOAD  char const*	Q_stristr( char const* pStr, char const* pSearch );


// These are versions of functions that guarantee null termination.
//
// maxLen is the maximum number of bytes in the destination string.
// pDest[maxLen-1] is always null terminated if pSrc's length is >= maxLen.
//
// This means the last parameter can usually be a sizeof() of a string.
VSTDLIB_INTERFACE void Q_strncpy( char *pDest, char const *pSrc, int maxLen );
VSTDLIB_INTERFACE int Q_snprintf( char *pDest, int destLen, char const *pFormat, ... );
VSTDLIB_INTERFACE char *Q_strncat(char *, const char *, size_t);
VSTDLIB_INTERFACE char *Q_strnlwr(char *, size_t);


// UNDONE: Find a non-compiler-specific way to do this
#ifdef _VA_LIST_DEFINED
VSTDLIB_INTERFACE int Q_vsnprintf( char *pDest, int maxLen, char const *pFormat, va_list params );
#endif

// Prints out a pretified memory counter string value ( e.g., 7,233.27 Mb, 1,298.003 Kb, 127 bytes )

VSTDLIB_INTERFACE char *Q_pretifymem( float value, int digitsafterdecimal = 2, bool usebinaryonek = false );

#endif	// VSTDLIB_STRTOOLS_H
