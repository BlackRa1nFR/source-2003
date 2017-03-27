/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLBlockQuote.cpp
Owner:	rich@woodbridgeinternalmed.com
Purpose:	BlockQuote entity
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"

CHTMLBlockQuote::CHTMLBlockQuote( CDefaults *pDefaults )
	: CHTMLParagraphObject( CHTMLParagraphObject::knBlockQuote )
	, m_pDoc( new CHTMLDocument( pDefaults ) )
{
}


CHTMLBlockQuote::~CHTMLBlockQuote()
{
	delete m_pDoc;
}

#ifdef _DEBUG
void CHTMLBlockQuote::Dump() const
{
	TRACENL( _T("BlockQuote----------------\n") );
	if (m_pDoc)
		m_pDoc->Dump();
}
#endif	//	_DEBUG

