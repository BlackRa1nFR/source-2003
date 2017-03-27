/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	GetColourFromString.cpp
Owner:	russf@gipsysoft.com
Purpose:	Parse a string for it's colour value, used by the HTML code
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "WinHelper.h"
#include "QHTM_Types.h"
#include "StaticString.h"
#include <stdlib.h>

struct StructColour
{
	StructColour( LPCTSTR pcszName, COLORREF cr )
		: m_strName( pcszName ), m_cr( cr ) {}
	CStaticString m_strName;
	COLORREF m_cr;
};

const StructColour g_colours[] =
{
	StructColour( _T("Black"),		RGB( 0, 0, 0 ) )
	, StructColour( _T("Green"),		RGB( 0, 128, 0 ) )
	, StructColour( _T("Silver"),		RGB( 192, 192, 192 ) )
	, StructColour( _T("Lime"),			RGB( 0, 255, 0 ) )
	, StructColour( _T("Gray"),			RGB( 128, 128, 128 ) )
	, StructColour( _T("Olive"),		RGB( 128, 128, 0) )
	, StructColour( _T("White"),		RGB( 255, 255, 255 ) )
	, StructColour( _T("Yellow"),		RGB( 255, 255, 0 ) )
	, StructColour( _T("Maroon"),		RGB( 128, 0, 0 ) )
	, StructColour( _T("Navy"),			RGB( 0, 0, 128 ) )
	, StructColour( _T("Red"),			RGB( 255, 0, 0 ) )
	, StructColour( _T("Blue"),			RGB( 0, 0, 255 ) )
	, StructColour( _T("Purple"),		RGB( 128, 0, 128 ) )
	, StructColour( _T("Teal"),			RGB( 0, 128, 128 ) )
	, StructColour( _T("Fuchsia"),	RGB( 255, 0, 255 ) )
	, StructColour( _T("Aqua"),			RGB( 0, 255, 255 ) )
};

static MapClass< CStaticString , COLORREF> g_mapColour;

int InitialiseMap()
{
	if( g_mapColour.GetSize() == 0 )
	{
		for( UINT n = 0; n < countof( g_colours ); n++ )
		{
			g_mapColour.SetAt( g_colours[n].m_strName, g_colours[n].m_cr );
		}
	}
	return 1;
}

//	REVIEW - russf - Is this a performance hit on app start?
//	To ensure the colour map is initialised correctly before anything else.
//	This can make the program startup slower but I will cross that bridge later.
static int nDummy = InitialiseMap();

COLORREF GetColourFromString( const CStaticString &strColour, COLORREF crDefault)
//
//	Return the colour from the string passed.
//
//	Does a quick lookup to see if the string is oe of the standard colours
//	if not it simple assumes the colour is in hex.
{
	if( !strColour.GetLength() )
		return crDefault;	// black

	LPCTSTR pcszColour = strColour.GetData();

	if( *pcszColour == '#' )
	{
		pcszColour++;

		//
		//	This bit of code adapted from the mozilla source for the same purpose.
		const int nLength = strColour.GetLength() - 1;
		int nBytesPerColour = min( nLength / 3, 2 );
		int rgb[3] = { 0, 0, 0 };
		for( int nColour = 0; nColour < 3; nColour++ )
		{
			int val = 0;
			for( int nByte = 0; nByte < nBytesPerColour; nByte++ )
			{
				int c = 0;
				if( *pcszColour )
				{
					c = tolower( (TCHAR) *pcszColour );
					if( (c >= '0') && (c <= '9'))
					{
						c = c - '0';
					}
					else if( (c >= 'a') && ( c <= 'f') )
					{
						c = c - 'a' + 10;
					}
					else
					{
						c = 0;
					}
					val = (val << 4) + c;
					pcszColour++;
				}
			}
			rgb[ nColour ] = val;
		}

		return RGB( rgb[0], rgb[1], rgb[2] );
	}

	COLORREF *pcr = g_mapColour.Lookup( strColour );
	if( pcr )
	{
		return *pcr;
	}

	if( *pcszColour  )
	{
		//
		//	It's safe to use _tcstoul because the assumption that the data originally passed to us is zero terminated.
		//	That way even if the colour is *the* last item in the file _tcstoul will properly terminate!
		TCHAR *endptr;
		return _tcstoul( pcszColour, &endptr, 16 );
	}
	return crDefault;
}