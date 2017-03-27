/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLImage.cpp
Owner:	russf@gipsysoft.com
Purpose:	HTML Image object
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"
#include <ImgLib.h>

CHTMLImage::CHTMLImage( int nWidth, int nHeight, int nBorder, LPCTSTR pcszFilename, CHTMLParse::Align alg, CImage *pImage )
	: CHTMLParagraphObject( CHTMLParagraphObject::knImage )
	, m_nWidth( nWidth )
	, m_nHeight( nHeight )
	,	m_nBorder( nBorder )
	, m_strFilename( pcszFilename )
	, m_alg( alg )
	, m_pImage( pImage )
{
}


CHTMLImage::~CHTMLImage()
{
	// Since images are cached in the document, they are not owned here.
	// This object just points to the image in the document.
	// delete m_pImage;
}

#ifdef _DEBUG
void CHTMLImage::Dump() const
{
	TRACENL( _T("Image\n") );
	TRACENL( _T("\tName(%s)\n"), (LPCTSTR)m_strFilename );
	TRACENL( _T("\t Width(%d)\n"), m_nWidth );
	TRACENL( _T("\t Height(%d)\n"), m_nHeight );
	TRACENL( _T("\tAlignment (%s)\n"), GetStringFromAlignment( m_alg ) );
}
#endif	//	_DEBUG
