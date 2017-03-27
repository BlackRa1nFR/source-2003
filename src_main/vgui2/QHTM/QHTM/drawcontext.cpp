/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
File:	drawcontext.cpp
Owner:	russf@gipsysoft.com
Purpose:	Drawing context.
					All drawing operations, brushes, colours, fonts etc. are managed
					here.

					REVIEW Notes
					Could we keep our resources for the duration of execution? Not sure,
					currently we destroy everything after we are done. It would probably
					speed up the display if we do not need to create brushes, fonts etc.
					HTML screws that scheme up a bit because it *could* use a large number
					of fonts, colurs, etc. and we coudl quickly become a resource hog.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "DrawContext.h"
#include "QHTM_Trace.h"
#include <MapIter.h>


static const int knParagraphSpacing = 3;


namespace Container {
	inline BOOL ElementsTheSame( FontDef n1, FontDef n2 )
	{
		//
		//	Don't optimise this unless you have empirical evidence to proove that you can
		//	increase it' speed.
		return n1.m_bItalic == n2.m_bItalic
			&& n1.m_nSizePixels == n2.m_nSizePixels
			&& n1.m_nWeight == n2.m_nWeight
			&& n1.m_bUnderline == n2.m_bUnderline
			&& n1.m_bStrike == n2.m_bStrike
			&& n1.m_cCharSet == n2.m_cCharSet
			&& !_tcscmp( n1.m_szFontName, n2.m_szFontName );
	}

	inline UINT HashIt( const FontDef &key )
	{
		return key.m_nHasValue;
	}
}


CDrawContext::CDrawContext( const CRect *prcClip, HDC hdc, bool bIsPrinting /*= false */ )
	: m_bWeCreatedDC( false )
	, m_hdc( hdc )
	, m_pFontInfo( NULL )
	, m_bBrushSelected( false )
	, m_bPenSelected( false )
	, m_cxDeviceScaleNumer( 1 )
	, m_cxDeviceScaleDenom( 1 )
	, m_cyDeviceScaleNumer( 1 )
	, m_cyDeviceScaleDenom( 1 )
	, m_crCurrentText( 0 )
	, m_bIsPrinting( bIsPrinting )
{
	if( prcClip )
		m_rcClip = *prcClip;
	else
		m_rcClip.Set(0,0,0,0);

	if( !m_hdc )
	{
		m_hdc = ::GetDC( g_hwnd );
		m_bWeCreatedDC = true;
	}

	// Set the device scale if needed
	if( IsPrinting() )
	{
		HDC screenDC = ::GetDC(g_hwnd);
		m_cxDeviceScaleNumer = ::GetDeviceCaps(m_hdc, LOGPIXELSX);
		m_cxDeviceScaleDenom = ::GetDeviceCaps(screenDC, LOGPIXELSX);
		m_cyDeviceScaleNumer = ::GetDeviceCaps(m_hdc, LOGPIXELSY);
		m_cyDeviceScaleDenom = ::GetDeviceCaps(screenDC, LOGPIXELSY);
		::ReleaseDC(g_hwnd, screenDC);
	}

	VAPI( ::SaveDC( m_hdc ) );

	::SetBkMode( m_hdc, TRANSPARENT );
}


CDrawContext::~CDrawContext()
{
	if( m_hdc )
		VAPI( ::RestoreDC( m_hdc, -1 ) );

	const UINT uSize = m_arrObjToDelete.GetSize();
	for( UINT n = 0; n < uSize; n++ )
	{
		VAPI( ::DeleteObject( m_arrObjToDelete[ n ] ) );
	}

	if( m_bWeCreatedDC )
	{
		::ReleaseDC( g_hwnd, m_hdc );
		m_hdc = NULL;
	}

	// TRACENL("DrawContext: There were %d fonts cached.\n", m_mapFonts.GetSize());

	for( Container::CMapIter<FontDef, FontInfo*> itr( m_mapFonts ); !itr.EOL(); itr.Next() )
	{
		delete itr.GetValue();
	}
}



HDC CDrawContext::GetSafeHdc()
{
	return m_hdc;
}


HPEN CDrawContext::GetPen( COLORREF cr )
{
	HPEN *pPen = m_mapPens.Lookup( cr );
	if( pPen )
	{
		return *pPen;
	}

	HPEN hPen = CreatePen( PS_SOLID, 1, 0x02000000 | cr );
	VAPI( hPen );
	m_arrObjToDelete.Add( hPen );
	m_mapPens.SetAt( cr, hPen );
	return hPen;
}


HBRUSH CDrawContext::GetBrush( COLORREF cr )
{
	HBRUSH *pBrush = m_mapBrushes.Lookup( cr );
	if( pBrush )
	{
		return *pBrush;
	}

	HBRUSH hbr = ::CreateSolidBrush( 0x02000000 | cr );
	VAPI( hbr );
	m_arrObjToDelete.Add( hbr );
	m_mapBrushes.SetAt( cr, hbr );
	return hbr;
}


void CDrawContext::SelectBrush( COLORREF cr )
{
	if( !m_bBrushSelected || cr != m_crCurrentBrush )
	{
		VAPI( ::SelectObject( m_hdc, GetBrush( cr ) ) );
		m_crCurrentBrush = cr;
		m_bBrushSelected = true;
	}
}


void CDrawContext::SelectPen( COLORREF cr )
{
	if( !m_bPenSelected || cr != m_crCurrentPen )
	{
		VAPI( ::SelectObject( m_hdc, GetPen( cr ) ) );
		m_crCurrentPen = cr;
		m_bPenSelected = true;
	}
}



void CDrawContext::FillRect( const CRect &rc, COLORREF cr )
{
	VAPI( ::FillRect( m_hdc, &rc, GetBrush( cr ) ) );
}


void CDrawContext::PolyFillOutlined( const POINT *pt, int nCount, COLORREF cr, COLORREF crOutline )
{
	ASSERT( nCount > 0 );
	ASSERT_VALID_READPTR( pt, sizeof( POINT) * nCount );

	SelectBrush( cr );
	SelectPen( crOutline );
	VAPI( ::Polygon( m_hdc, pt, nCount ) );

}


void CDrawContext::PolyLine( const POINT *pt, int nCount, COLORREF cr )
{
	SelectPen( cr );
	VAPI( ::Polyline( m_hdc, pt, nCount ) );
}


void CDrawContext::Rectangle( const CRect &rc, COLORREF cr )
{
	SelectPen( cr );
	//	Save and restore the brush because otherwise the caching system (GetBrush) still thinks
	//	a coloured brush is selected!
	HGDIOBJ hOld = ::SelectObject( m_hdc, GetStockObject( HOLLOW_BRUSH ) );
	VAPI( ::Rectangle( m_hdc, rc.left, rc.top, rc.right, rc.bottom ) );
	::SelectObject( m_hdc, hOld );
}



void CDrawContext::DrawText( int x, int y, LPCTSTR pcszText, int nLength, COLORREF crFore )
{
	if( crFore != m_crCurrentText )
	{
		::SetTextColor( m_hdc, 0x02000000 | crFore );
		m_crCurrentText = crFore;
	}
	VAPI( ::TextOut( m_hdc, x, y, pcszText, nLength ) );
}


int CDrawContext::GetTextExtent( LPCTSTR pcszText, int nTextLength )
{
	ASSERT_VALID_STR_LEN( pcszText, nTextLength );
	ASSERT( m_pFontInfo );

	int nLength = 0;
	while( nTextLength )
	{
		nLength += m_pFontInfo->m_nWidths[ (TBYTE)*pcszText ];
		pcszText++;
		nTextLength--;
	}
	if( nLength )
	{
		pcszText--;
		nLength += m_pFontInfo->m_nOverhang[ (TBYTE)*pcszText ];
	}
	return nLength;
}

/*
	//	This is a test version, much simpler and slightly faster - Still haven't finished testing it yet though!
	//	Note it doesn't need two loops and it doesn't need the startofline flag.

bool CDrawContext::GetTextFitWithWordWrap( int nMaxWidth, LPCTSTR &pcszText, CSize &size ) const
//	Get the text that fits within the length given, return the 'size' of the text object
//	needed.
//	Returns true if all of the text fits. If this is the case then pcszText points
//	to the next text needed to be rendered.
//  richg - 19990224 - This should break only at an acceptable boundary! If the text does not
//	fit into the space provided, continue anyway!
{
	size.cx = 0;
	size.cy = GetCurrentFontHeight();
	int nLength;
	int *parrWidths = m_pFontInfo->m_nWidths;
	int *parrOverhang = m_pFontInfo->m_nOverhang;
	LPCTSTR pcszPreviousSpace = pcszText;
	int nSpaceLength;
	nLength = nSpaceLength = 0;

	TBYTE ch;

	TBYTE cLast = 0;
	do
	{
		ch = *pcszText;
		if( ch == 0 )
			break;

		//
		//	Carriage returns force us out
		if( ch == '\r' )
		{
			size.cx = nLength;
			return false;
		}

		//
		//	If the next character makes us too wide then we should leave
		if( nSpaceLength && nLength + ( parrWidths[ ch ] + ( ( ch == 32 || ch == 255 ) ? 0 : parrOverhang[ ch ] ) ) > nMaxWidth )
		{
			//
			//	Set the text pointer to be the last position where we got up to that did fit and
			//	was also a wordbreak.
			pcszText = pcszPreviousSpace;
			size.cx = nSpaceLength;
			return false;
		}

		if( ch == 255 || ch == 32 )
		{
			pcszPreviousSpace = pcszText;
			nSpaceLength = nLength;
		}
		nLength += parrWidths[ ch ];
		cLast = ch;
		pcszText++;
	}
	while( 1 );

	//
	//	Spaces as the last character don't need overhang.
	if( cLast != 32 && cLast != 255 && cLast != 160 )
		nLength -= parrOverhang[ cLast ];

	size.cx = nLength;
	return true;
}



*/

bool CDrawContext::GetTextFitWithWordWrap( int nMaxWidth, LPCTSTR &pcszText, CSize &size, bool bStartOfLine ) const
//	Get the text that fits within the length given, return the 'size' of the text object
//	needed.
//	Returns true if all of the text fits. If this is the case then pcszText points
//	to the next text needed to be rendered.
//  richg - 19990224 - This should break only at an acceptable boundary! If the text does not
//	fit into the space provided, continue anyway!
{
	size.cx = 0;
	size.cy = GetCurrentFontHeight();
	int nLength;
	const int *parrWidths = m_pFontInfo->m_nWidths;
	const int *parrOverhang = m_pFontInfo->m_nOverhang;
	LPCTSTR pcszPreviousSpace = pcszText;
	int nSpaceLength;
	nLength = nSpaceLength = 0;

	TBYTE ch;
	//	richg - 19990224 - The way this was originally written, it can break a word
	//	in the middle, if it is the first word on the line! Words should not break,
	//	and are able to exceed the length of the space provided.
	//	A better way is to locate one word at a time, and see it it continues to fit.
	//	Stop when a word will not fit. If it is the first word, get its length.
	//	This should NOT break on &nbsp;!
	//	Unfortunately, there is no way yet to know here if we are at the beginning of a line!
	
	//	If we are at the start of a line, look past the first word to get the
	//	length. There shouldn't be any leading spaces.
	TBYTE cLast = 0;
	if (bStartOfLine)
	{
		while ((ch = *pcszText) != 0 && ch != '\r' && ch != 32 && ch != 255)
		{
			nLength += parrWidths[ ch ];
			cLast = ch;
			pcszText++;
		}
	}

	//
	//	We calculate the maximum width that will fit, we take into account the overhang for this character too.
	while( (ch = *pcszText) != 0 && ch != '\r' && nLength + ( parrWidths[ ch ] + ( ( ch == 32 || ch == 255 ) ? 0 : parrOverhang[ ch ] ) ) <= nMaxWidth )
	{
		if( ch == 255 || ch == 32 )
		{
			pcszPreviousSpace = pcszText;
			nSpaceLength = nLength;
		}
		nLength += parrWidths[ ch ];
		cLast = ch;
		pcszText++;
	}

	//
	//	Spaces as the last character don't need overhang.
	if( cLast != 32 && cLast != 255 && cLast != 160 )
		nLength -= parrOverhang[ cLast ];

	//
	//	If we did not get to the ned of the text then use our *word* wrap stuff
	if( *pcszText )
	{
		if( *pcszText == '\r' )
		{
			size.cx = nLength;
		}
		else
		{
			if( nSpaceLength && *pcszText != ' ' )
			{
				pcszText = pcszPreviousSpace;
				//
				//	If we had to break on a space then we need to add the overhang onto the length
				if( cLast )
					nSpaceLength -= parrOverhang[ cLast ];
				size.cx = nSpaceLength;
			}
			else
			{
				size.cx = nLength;
			}
		}
		return false;
	}
	else
	{
		size.cx = nLength;
	}

	return true;
}


const CRect &CDrawContext::GetClipRect() const
{
	return m_rcClip;
}


void CDrawContext::SetClipRect( const CRect &rc )
{
	HRGN hrgn = CreateRectRgnIndirect( &rc );
	::SelectClipRgn( m_hdc, hrgn );
	POINT pt;
	GetWindowOrgEx( m_hdc, &pt );
	OffsetClipRgn( m_hdc, -pt.x, -pt.y );
	VAPI( ::DeleteObject( hrgn ) );
}


void CDrawContext::RemoveClipRect()
{
	SelectClipRgn( m_hdc, NULL );
}


void CDrawContext::SelectFont( LPCTSTR pcszFontNames, int nSizePixels, int nWeight, bool bItalic, bool bUnderline, bool bStrike, BYTE cCharSet )
{
	//	we set the current font to be an invalid font because we don't know what this font may be,
	FontDef fdef( pcszFontNames, nSizePixels, nWeight, bUnderline, bItalic, bStrike, cCharSet );
	SelectFont( fdef );
}


void CDrawContext::SelectFont( const FontDef &fdef )
{
	FontInfo *pfinfo = GetFont( fdef );
	ASSERT( pfinfo );	//	Can't be NULL!
	if( pfinfo != m_pFontInfo )
	{
		m_pFontInfo = pfinfo;
		::SelectObject( m_hdc, m_pFontInfo->hFont );
	}
}


CDrawContext::FontInfo * CDrawContext::GetFont( const FontDef &fdef )
//
//	Create a font. If the font already exists in our map of fonts then we use that
//	if the font does not exist then we create it.
{
	FontInfo **ppInfo = m_mapFonts.Lookup( fdef );
	if( ppInfo )
	{
		return (*ppInfo);
	}

	LOGFONT lf = {0};
	(void)_tcscpy( lf.lfFaceName, fdef.m_szFontName );
	lf.lfWeight = fdef.m_nWeight;
	lf.lfHeight = fdef.m_nSizePixels;
	lf.lfItalic = fdef.m_bItalic;
	lf.lfUnderline = fdef.m_bUnderline;
	lf.lfStrikeOut = fdef.m_bStrike;
	lf.lfQuality = DRAFT_QUALITY;
	lf.lfCharSet = fdef.m_cCharSet;
	lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;

	FontInfo *pInfo = new FontInfo;
	pInfo->hFont = CreateFontIndirect( &lf );


	//
	//	Get and store the widths of the individual characters, these are used
	//	later when calculating the lengths of text in pixels.
	HGDIOBJ hFontOld = ::SelectObject( m_hdc, pInfo->hFont );

	//
	//	Store some extra bits and pieces.
	TEXTMETRIC tm;
	GetTextMetrics( m_hdc, &tm );
	pInfo->m_nBaseline = tm.tmAscent;
	pInfo->m_nLineSpace = tm.tmHeight + tm.tmExternalLeading;
	if( fdef.m_bUnderline )
		pInfo->m_nLineSpace++;

	ABC abcWids[ countof( pInfo->m_nWidths ) ];
	memset( abcWids, 0, sizeof( abcWids ) );
	if( GetCharABCWidths( m_hdc, 0, countof( pInfo->m_nWidths ) - 1, abcWids ) )
	{
		ABC *pABC = &abcWids[0];
		int *parrWidths = pInfo->m_nWidths;
		if( tm.tmOverhang )
		{
			int *parrOverhang = pInfo->m_nOverhang;

			for( UINT n = 0; n < countof( pInfo->m_nWidths ) ; n++, pABC++ )
			{
				*parrWidths++ = ( pABC->abcA + pABC->abcB + pABC->abcC );
				*parrOverhang++ = pABC->abcC;
			}
		}
		else
		{
			for( UINT n = 0; n < countof( pInfo->m_nWidths ) ; n++, pABC++ )
			{
				*parrWidths++ = ( pABC->abcA + pABC->abcB + pABC->abcC );
			}
			memset( pInfo->m_nOverhang, 0, sizeof( pInfo->m_nOverhang ) );
		}
	}
	else
	{
		int *parrWidths = pInfo->m_nWidths;
		if( !GetCharWidth32( m_hdc, 0, 255, parrWidths ) )
		{
			SIZE size;
			TCHAR ch = 0;
			for( UINT n = 0; n < 255; n++ )
			{
				ch = (TCHAR)n;
				GetTextExtentPoint32( m_hdc, &ch, 1, &size );
				*parrWidths++ = size.cx;
			}
		}
		memset( pInfo->m_nOverhang, 0, sizeof( pInfo->m_nOverhang ) );
	}

	::SelectObject( m_hdc, hFontOld );

	//
	//	And finally add the font to our cache.
	if( pInfo->hFont )
		m_arrObjToDelete.Add( pInfo->hFont );
	m_mapFonts.SetAt( fdef, pInfo );

	return pInfo;
}

int CDrawContext::ScaleX(int x) const
{
	return ::MulDiv(x, m_cxDeviceScaleNumer, m_cxDeviceScaleDenom);
}

int CDrawContext::ScaleY(int y) const
{
	return ::MulDiv(y, m_cyDeviceScaleNumer, m_cyDeviceScaleDenom);
}

CSize CDrawContext::Scale(const CSize& size) const
{
	return CSize(ScaleX(size.cx), ScaleY(size.cy));
}

void CDrawContext::SetScaling( int cxDeviceScaleNumer, int cxDeviceScaleDenom, int cyDeviceScaleNumer, int cyDeviceScaleDenom )
{
	m_cxDeviceScaleNumer = cxDeviceScaleNumer;
	m_cxDeviceScaleDenom = cxDeviceScaleDenom;
	m_cyDeviceScaleNumer = cyDeviceScaleNumer;
	m_cyDeviceScaleDenom = cyDeviceScaleDenom;
}