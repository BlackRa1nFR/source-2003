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

File:	Trace.cpp
Owner:	russf@gipsysoft.com
Purpose:	TRACE handling function.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <DebugHlp.h>


void _cdecl DebugTraceLineAndFileA( LPCSTR pcszFilename, int nLine )
{
	OutputDebugStringA( pcszFilename );

	char szLineNumber[30] = "(";
	ltoa( nLine, szLineNumber + 1, 10 );
	strcat( szLineNumber , ") : " );
	OutputDebugStringA( szLineNumber );
}


void _cdecl DebugTraceA( LPCSTR pcszFormat, ... )
{
 	va_list marker;
	va_start( marker, pcszFormat );

	LPTSTR pszBuffer = vmalprintfA( pcszFormat, marker );
	OutputDebugStringA( pszBuffer );
	VERIFY( HeapFree( GetProcessHeap(), 0, pszBuffer ) );

	va_end( marker );
}



void _cdecl DebugTraceLineAndFileW( LPCWSTR pcszFilename, int nLine )
{
	OutputDebugStringW( pcszFilename );

	WCHAR szLineNumber[30] = L"(";
	_ltow( nLine, szLineNumber + 1, 10 );
	wcscat( szLineNumber , L") : " );
	OutputDebugStringW( szLineNumber );
}


void _cdecl DebugTraceW( LPCWSTR pcszFormat, ... )
{
 	va_list marker;
	va_start( marker, pcszFormat );

	LPWSTR pszBuffer = vmalprintfW( pcszFormat, marker );
	OutputDebugStringW( pszBuffer );
	VERIFY( HeapFree( GetProcessHeap(), 0,pszBuffer ) );

	va_end( marker );
}
