/*----------------------------------------------------------------------
Copyright (c) 1998 Russell Freeman. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
Email: russf@gipsysoft.com
Web site: http://www.gipsysoft.com

This notice must remain intact.
This file belongs wholly to Russell Freeman, 
You may use this file compiled in your applications.
You may not sell this file in source form.
This source code may not be distributed as part of a library,

This file is provided 'as is' with no expressed or implied warranty.
The author accepts no liability if it causes any damage to your
computer.

Please use and enjoy. Please let me know of any bugs/mods/improvements 
that you have found/implemented and I will fix/incorporate them into this
file.

File:	vmalprintf.cpp
Owner:	russf@gipsysoft.com
Purpose:	printf style functions that return a heap allocated memory block
					Uses Heap API for memory allocations for the output buffer.
----------------------------------------------------------------------*/
#include "stdafx.h"

static HANDLE g_hHeap = GetProcessHeap();


inline LPVOID GetBuffer()
{
	return HeapAlloc( g_hHeap, HEAP_ZERO_MEMORY, 512 );
}


LPVOID ReallocBuffer( LPVOID pszBuffer, int nAllocated )
{
	LPVOID pVoid = HeapReAlloc( g_hHeap, HEAP_ZERO_MEMORY, pszBuffer, nAllocated );
	if( !pVoid )
	{
		HeapFree( g_hHeap, 0, pszBuffer );
		pszBuffer = NULL;
	}
	else
	{
		pszBuffer = pVoid;
	}
	return pszBuffer;
}


LPWSTR vmalprintfW( LPCWSTR pcszFormat, va_list list )
{
  bool  bSuccess = FALSE;

	LPWSTR pszBuffer = reinterpret_cast<LPWSTR>( GetBuffer() );

	int nAllocated = 512;
	do
  {
		if( _vsnwprintf( pszBuffer, nAllocated - 1, pcszFormat, list ) < 0 )
	  {
			nAllocated *= 2;
			pszBuffer = reinterpret_cast<LPWSTR>( ReallocBuffer( pszBuffer, nAllocated ) );
			if( !pszBuffer )
			{
				return NULL;
			}
	  }
    else
		{
      bSuccess = TRUE;
		}
  } while( !bSuccess );

	return pszBuffer;
}


LPTSTR vmalprintfA( LPCTSTR pcszFormat, va_list list )
{
  bool  bSuccess = FALSE;

	LPSTR pszBuffer = reinterpret_cast<LPTSTR>( GetBuffer() );

	int nAllocated = 512;
	do
  {
		if( _vsnprintf( pszBuffer, nAllocated - 1, pcszFormat, list ) < 0 )
	  {
			nAllocated *= 2;
			pszBuffer = reinterpret_cast<LPTSTR>( ReallocBuffer( pszBuffer, nAllocated ) );
			if( !pszBuffer )
			{
				return NULL;
			}
	  }
    else
		{
      bSuccess = TRUE;
		}
  } while( !bSuccess );

	return pszBuffer;
}
