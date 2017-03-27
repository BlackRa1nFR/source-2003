/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLGetAlignmentFromString.cpp
Owner:	russf@gipsysoft.com
Purpose:	Read a string and get the alignment value from it.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"

struct StructAlignment
{
	StructAlignment( LPCTSTR pcsz, CHTMLParse::Align alg )
		: m_str( pcsz ), m_alg( alg ) {}
	CStaticString m_str;
	CHTMLParse::Align m_alg;
};

const StructAlignment g_arrAligns[] = {
	StructAlignment( _T("left"), CHTMLParse::algLeft )
	, StructAlignment( _T("center"), CHTMLParse::algCentre )
	, StructAlignment( _T("right"), CHTMLParse::algRight )
	, StructAlignment( _T("top"),CHTMLParse::algTop )
	, StructAlignment( _T("middle"), CHTMLParse::algMiddle )
	, StructAlignment( _T("bottom"), CHTMLParse::algBottom )
};

CHTMLParse::Align GetAlignmentFromString( const CStaticString &str, CHTMLParse::Align algDefault )
{
	if( str.GetLength() )
	{
		for( int n = 0; n < countof( g_arrAligns ); n++ )
		{
			if( !_tcsnicmp( g_arrAligns[ n ].m_str.GetData(), str.GetData(), str.GetLength() ) )
			{
				return g_arrAligns[ n].m_alg;
			}
		}
	}
	return algDefault;
}


LPCTSTR GetStringFromAlignment( CHTMLParse::Align alg )
{
	for( int n = 0; n < countof( g_arrAligns ); n++ )
	{
		if( g_arrAligns[ n].m_alg == alg )
		{
			return g_arrAligns[ n ].m_str.GetData();
		}
	}

	return _T("Unknown");
}