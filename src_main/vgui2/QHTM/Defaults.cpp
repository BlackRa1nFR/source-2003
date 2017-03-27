/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	Defaults.cpp
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "QHTM_Types.h"
#include "Defaults.h"
#include "QHTM.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDefaults g_defaults;

CDefaults::CDefaults()
	: m_strFontName( _T("Times New Roman") )
	, m_strDefaultPreFontName( _T("Courier New") )
	, m_nFontSize( 2 )

	// richg - 19990621 - The indentation level used for Lists and BlockQuote
	// This is in 1000ths of an inch, not dependent on zoom level
	, m_nIndentSize( 250 )
	, m_nIndentSpaceSize( 125 )

	, m_crLinkColour( RGB( 141, 7, 102 ) )
	, m_crLinkHoverColour( RGB( 29, 49, 149 ) )

	, m_crBackground( RGB( 255, 255, 255 ) )
	, m_crDefaultForeColour( RGB( 0, 0, 0 ) )
	, m_nCellPadding( 1 )
	, m_nCellSpacing( 1 )
	, m_crBorderLight( RGB( 0xFF, 0xFF, 0xFF ) )
	, m_crBorderDark( RGB( 0x80, 0x80, 0x80 ) )
	, m_nAlignedTableMargin( 1 )

	, m_nParagraphLinesAbove( 1 )
	, m_nParagraphLinesBelow( 1 )

	, m_nImageMargin( 5 )
	, m_cCharSet( DEFAULT_CHARSET )

	, m_rcMargins( 5, 5, 5, 5 )

	, m_nZoomLevel( QHTM_ZOOM_DEFAULT )

{

}


CDefaults::~CDefaults()
{

}


void CDefaults::SetFont( LOGFONT &lf )
{
	m_strFontName = lf.lfFaceName;
	HDC hdc = GetDC( NULL );
	m_nFontSize = MulDiv( 72, lf.lfHeight, GetDeviceCaps( hdc, LOGPIXELSY) );
	ReleaseDC( NULL, hdc );
}


void CDefaults::SetFont( HFONT hFont )
{
	LOGFONT lf;
	GetObject( hFont, sizeof( lf ), &lf );
	SetFont( lf );
}