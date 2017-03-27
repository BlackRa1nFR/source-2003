/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLTextBlock.cpp
Owner:	russf@gipsysoft.com
Purpose:	HTML Text block.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"

CHTMLTextBlock::CHTMLTextBlock( const CTextABC &strText, UINT uFontDefIndex, COLORREF crFore, bool bPreformatted )
	: CHTMLParagraphObject( CHTMLParagraphObject::knText )
	, m_strText( strText, strText.GetLength() )
	, m_uFontDefIndex( uFontDefIndex )
	, m_crFore( crFore )
	, m_bPreformatted( bPreformatted )
{
}

#ifdef _DEBUG
void CHTMLTextBlock::Dump() const
{
	TRACENL(_T("TextBlock\n") );
	TRACENL(_T("\tText( %s )\n"), (LPCTSTR)m_strText );
	TRACENL(_T("\t crFore(%d)\n"), m_crFore );
	TRACENL(_T("\t bPreformatted(%d)\n"), m_bPreformatted );	
}
#endif	//	_DEBUG
