/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLHorizontalRule.cpp
Owner:	russf@gipsysoft.com
Purpose:	Horizontal rule between paragraphs.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"

CHTMLHorizontalRule::CHTMLHorizontalRule( CHTMLParse::Align alg, int nSize, int nWidth, bool bNoShade, COLORREF crColor )
	: CHTMLParagraphObject( CHTMLParagraphObject::knHorizontalRule )
	, m_alg( alg )
	,	m_nSize( nSize )
	, m_nWidth( nWidth )
	, m_bNoShade( bNoShade )
	, m_crColor( crColor )
{
}

#ifdef _DEBUG
void CHTMLHorizontalRule::Dump() const
{
	TRACENL( _T("Horizontal Rule\n") );
	TRACENL( _T("\tAlignment (%s)\n"), GetStringFromAlignment( m_alg ) );
	TRACENL( _T("\tSize (%d)\n"), m_nSize );
	TRACENL( _T("\tWidth (%d)\n"), m_nWidth );
	TRACENL( _T("\tNo Shade (%d)\n"), m_bNoShade );
	TRACENL( _T("\tColor (%d)\n"), m_crColor );
}
#endif	//	_DEBUG
