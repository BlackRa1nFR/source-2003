/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLTableCell.cpp
Owner:	russf@gipsysoft.com
Purpose:	A single table cell
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"

CHTMLTableCell::CHTMLTableCell( CDefaults *pDefaults, int nWidth, int nHeight, bool bNoWrap, bool bTransparent, COLORREF crBgColor, COLORREF crDark, COLORREF crLight, CHTMLParse::Align valg )
	: CHTMLDocument( pDefaults )
	,  m_nWidth( nWidth )
	, m_nHeight( nHeight )
	, m_bNoWrap( bNoWrap )
	, m_bTransparent( bTransparent )
	, m_crBgColor( crBgColor )
	, m_crBorderDark( crDark )
	, m_crBorderLight( crLight )
	, m_valg( valg )
{
}


#ifdef _DEBUG
void CHTMLTableCell::Dump() const
{
	TRACENL( _T("Table cell----------\n") );
	TRACENL(_T(" Width %d, Height %d\n"), m_nWidth, m_nHeight );
	TRACENL(_T(" NoWrap: %s\n"), (m_bNoWrap ? _T("true") : _T("false") ));
	TRACENL(_T(" Transparent: %s\n"), (m_bTransparent ? _T("true") : _T("false") ));
	TRACENL(_T(" Colors not shown.\n"));
	CHTMLDocument::Dump();
}
#endif	//	_DEBUG
