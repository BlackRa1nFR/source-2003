/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLDocument.cpp
Owner:	russf@gipsysoft.com
Purpose:	HTML document.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"
#include "Defaults.h"
#include <MapIter.h>
#include <ImgLib.h>


namespace Container {
	inline BOOL ElementsTheSame( HTMLFontDef n1, HTMLFontDef n2 )
	{
		//
		//	Don't optimise this unless you have empirical evidence to proove that you can
		//	increase it' speed.
		return n1.m_bItalic == n2.m_bItalic
			&& n1.m_nSize == n2.m_nSize
			&& n1.m_nWeight == n2.m_nWeight
			&& n1.m_bUnderline == n2.m_bUnderline
			&& n1.m_bStrike == n2.m_bStrike
			&& n1.m_nSub == n2.m_nSub
			&& n1.m_nSup == n2.m_nSup
			&& !_tcscmp( n1.m_strFont, n2.m_strFont );
	}

	inline UINT HashIt( const HTMLFontDef &key )
	{
		LPCTSTR p = key.m_strFont;
		UINT uHash = 0;
		while( *p )
		{
			uHash = uHash << 1 ^ _totupper( *p++ );
		}
		uHash+=(key.m_bItalic<<1);
		uHash+=(key.m_bUnderline<<2);
		uHash+=(key.m_nSize<<3);
		uHash+=(key.m_nWeight<<4);
		uHash+=(key.m_bStrike<<5);
		uHash+=(key.m_nSup<<5);
		uHash+=(key.m_nSub<<5);		
		return uHash;
	}
}


CHTMLDocument::CHTMLDocument( CDefaults *pDefaults )
	: m_pCurrentPara( NULL )
	, m_crLink( pDefaults->m_crLinkColour )
	, m_crLinkHover( pDefaults->m_crLinkHoverColour )
	, m_crBack( pDefaults->m_crBackground )
	, m_nLeftMargin( pDefaults->m_rcMargins.left )
	, m_nTopMargin( pDefaults->m_rcMargins.top )
	, m_nRightMargin( pDefaults->m_rcMargins.right )
	, m_nBottomMargin( pDefaults->m_rcMargins.bottom )
	, m_cCharSet( pDefaults->m_cCharSet )
	, m_pDefaults( pDefaults )
{

}


CHTMLDocument::~CHTMLDocument()
{
	UINT uChildCount = m_arrItems.GetSize();
	for( UINT n = 0; n < uChildCount; n++ )
	{
		delete m_arrItems[ n ];
	}
	// Delete the images
	for( MapIterClass<StringClass, CImage*> itr( m_mapImages ); !itr.EOL(); itr.Next() )
	{
		delete itr.GetValue();
	}

	m_mapFonts.RemoveAll();

	UINT uFontCount = m_arrFontDefs.GetSize();
	for( n = 0; n < uFontCount; n++ )
	{
		delete m_arrFontDefs[ n ];
	}
}

#ifdef _DEBUG
void CHTMLDocument::Dump() const
{
	TRACENL( _T("Document %d Items, %d Images, Margins( %d, %d, %d, %d )\n"), m_arrItems.GetSize(), m_mapImages.GetSize(), m_nLeftMargin, m_nTopMargin, m_nRightMargin, m_nBottomMargin );
	for( UINT n = 0; n < m_arrItems.GetSize(); n++ )
	{
		m_arrItems[ n ]->Dump();
	}
}
#endif	//	_DEBUG


void CHTMLDocument::AddItem( CHTMLDocumentObject *pdocobj )
{
	m_arrItems.Add( pdocobj );
}


void CHTMLDocument::AddParagraph( CHTMLParagraph *pPara )
{
	m_arrItems.Add( pPara );
	m_pCurrentPara = pPara;
}


CHTMLParagraph *CHTMLDocument::CurrentParagraph() const
{
	return m_pCurrentPara;
}


const HTMLFontDef * CHTMLDocument::GetFontDef( UINT uIndex )
{
	return m_arrFontDefs[ uIndex ];
}


UINT CHTMLDocument::GetFontDefIndex( const HTMLFontDef &def )
{
	UINT *pu = m_mapFonts.Lookup( def );
	if( !pu )
	{
		HTMLFontDef *pdef = new HTMLFontDef;
		*pdef = def;

		UINT u = m_arrFontDefs.GetSize();
		m_arrFontDefs.Add( pdef );
		m_mapFonts.SetAt( def, u );

		return u;
	}
	return *pu;
}


void CHTMLDocument::ResetMeasuringKludge()
{
	const UINT uDocumentSize = m_arrItems.GetSize();
	for( UINT n = 0; n < uDocumentSize; n++ )
	{
		CHTMLDocumentObject *pItem = m_arrItems[ n ];
		switch( pItem->GetType() )
		{
			case CHTMLDocumentObject::knParagraph:
			// A Table breaks the line like a <hr>
			// NewParagraph( 1, 0, CHTMLParse::algLeft );
			static_cast<CHTMLParagraph *>( pItem )->ResetMeasuringKludge();
			break;
		}
	}
}