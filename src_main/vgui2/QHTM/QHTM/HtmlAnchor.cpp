/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLAnchor.cpp
Owner:	russf@gipsysoft.com
Author: rich@woodbridgeinternalmed.com
Purpose:	Hidden element that represents anchor tag data
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"

CHTMLAnchor::CHTMLAnchor(LPCTSTR pcszLinkName, LPCTSTR pcszLinkTarget, LPCTSTR  pcszLinkTitle, COLORREF crLink, COLORREF crHover)
	: CHTMLParagraphObject( CHTMLParagraphObject::knAnchor )
	, m_strLinkName( pcszLinkName )
	, m_strLinkTarget( pcszLinkTarget )
	, m_strLinkTitle( pcszLinkTitle )
	, m_crLink( crLink )
	, m_crHover( crHover )
{
}

CHTMLAnchor::CHTMLAnchor()
	: CHTMLParagraphObject( CHTMLParagraphObject::knAnchor )
	, m_crLink( RGB(0,0,0) )
	, m_crHover( RGB(0,0,0) )
{
}

#ifdef _DEBUG
void CHTMLAnchor::Dump() const
{
	TRACENL( _T("Anchor\n") );
	if (m_strLinkName.GetLength())
	{
		TRACENL( _T("\tName: \t%s\n"), m_strLinkName);
	}
	if (m_strLinkTarget.GetLength())
	{
		TRACENL( _T("\tTarget: \t%s\n"), m_strLinkTarget);
	}
	if (m_strLinkTitle.GetLength())
	{
		TRACENL( _T("\tTitle: \t%s\n"), m_strLinkTitle);
	}
	TRACENL( _T("\tColors not shown.\n" ) );
}
#endif	//	_DEBUG
