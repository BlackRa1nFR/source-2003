/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	htmlparse.cpp
Owner:	russf@gipsysoft.com
Purpose:	Main HTML parser
					The CHTMLParse::Parse() function generates a document, the document
					contains all of the elements of the HTML page but broken into
					their parts.
					The document will then be used to create the display page.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <WinHelper.h>
#include <ImgLib.h>
#include "QHTM_Includes.h"
#include "AquireImage.h"
#include "DrawContext.h"
#include "defaults.h"
#include "smallstringhash.h"
#include "HTMLParse.h"

extern LPTSTR stristr( LPTSTR pszSource, LPCTSTR pcszSearch );
extern BYTE DecodeCharset( const CStaticString &strCharSet );

static CHTMLParse::Align knDefaultImageAlignment = CHTMLParse::algBottom;
static CHTMLParse::Align knDefaultHRAlignment = CHTMLParse::algLeft;
static CHTMLParse::Align knDefaultParagraphAlignment = CHTMLParse::algLeft;
// richg - 19990224 - Default table alignment changed form algLeft to algTop
static CHTMLParse::Align knDefaultTableAlignment = CHTMLParse::algMiddle;
// richg - 19990224 - Table Cells have their own default
static CHTMLParse::Align knDefaultTableCellAlignment = CHTMLParse::algLeft;

TCHAR g_cCarriageReturn = _T('\r');
static const CStaticString g_strTabSpaces( _T("        ") );

static MapClass< StringClass, bool> g_mapFontName;

CHTMLParse::CHTMLParse( LPCTSTR pcszStream, UINT uLength, HINSTANCE hInstLoadedFrom, LPCTSTR pcszFilePath, CDefaults *pDefaults )
	: CHTMLParseBase( pcszStream, uLength )
	, m_pProp( NULL )
	, m_pDocument( NULL )
	, m_hInstLoadedFrom( hInstLoadedFrom )
	, m_pcszFilePath( pcszFilePath )
	, m_pLastAnchor( NULL )
	, m_pDefaults( pDefaults )
{
	ASSERT( m_pDefaults );
}


CHTMLParse::~CHTMLParse()
{
	CleanupParse();
}


CHTMLDocument * CHTMLParse::Parse()
//
//	returns either a fully created document ready to create the display from or
//	 NULL in the event of failure.
{
	CleanupParse();

	//
	//	Create the first properties
	m_pProp = new Properties;
	m_pProp->m_crFore = m_pDefaults->m_crDefaultForeColour;
	_tcscpy( m_pProp->szFaceName, m_pDefaults->m_strFontName );
	m_pProp->nSize = m_pDefaults->m_nFontSize;
	m_stkProperties.Push( m_pProp );

	//
	//	Create the main document and a paragraph to add to it.
	m_pMasterDocument = m_pDocument = new CHTMLDocument( m_pDefaults );
	m_pDocument->m_crBack = m_pDefaults->m_crBackground;
	CreateNewParagraph( m_pDefaults->m_nParagraphLinesAbove, m_pDefaults->m_nParagraphLinesBelow, knDefaultParagraphAlignment );

	if( !ParseBase() )
	{
		if( m_pMasterDocument )
		{
			delete m_pMasterDocument;
			m_pDocument = NULL;
			m_pMasterDocument = NULL;
		}
	}

	return m_pMasterDocument;
}


void CHTMLParse::CleanupParse()
//
//	Cleanup anything left over from our previous parsing
{
	while( m_stkProperties.GetSize() )
		delete m_stkProperties.Pop();

	while( m_stkDocument.GetSize() )
		m_stkDocument.Pop();


	//	richg - 19990227 - Clean up the table stack as well
	while( m_stkTable.GetSize() )
		m_stkTable.Pop();

	while( m_stkInTableCell.GetSize() )
		m_stkInTableCell.Pop();

	// richg - 19990621 - Clean up list stack
	while( m_stkList.GetSize() )
		m_stkList.Pop();
	
	m_pLastAnchor = NULL;
}


void CHTMLParse::OnGotText( TCHAR ch )
//
//	Callback when some text has been interrupted with a tag or end tag.
{
	if( ch == _T('\t') )
	{
		m_strToken.Add( g_strTabSpaces, g_strTabSpaces.GetLength() );
	}
	else
	{
		m_strToken.Add( ch );
	}
}


void CHTMLParse::OnEndDoc()
{
	CreateNewTextObject();
}


void CHTMLParse::OnGotTag( const Token token, const CParameters &pList )
{
	switch( token )
	{
	case tokIgnore:
		break;

	case tokSub:
		CreateNewTextObject();
		CreateNewProperties();
		if( m_pProp->nSize > 1 )
			m_pProp->nSize--;
		m_pProp->m_nSub++;
		break;

	case tokSup:
		CreateNewTextObject();
		CreateNewProperties();

		if( m_pProp->nSize > 1 )
			m_pProp->nSize--;
		m_pProp->m_nSup++;
		break;

	case tokPre:
		CreateNewProperties();
		_tcscpy( m_pProp->szFaceName, m_pDefaults->m_strDefaultPreFontName );
		break;

	case tokBody:
		OnGotBody( pList );
		break;

	case tokFont:
		OnGotFont( pList );
		break;

	case tokBold:
		CreateNewTextObject();
		CreateNewProperties();
		m_pProp->bBold = true;
		break;

	case tokUnderline:
		CreateNewTextObject();
		CreateNewProperties();
		m_pProp->bUnderline = true;
		break;

	case tokItalic:
		CreateNewTextObject();
		CreateNewProperties();
		m_pProp->bItalic = true;
		break;

	case tokStrikeout:
		CreateNewTextObject();
		CreateNewProperties();
		m_pProp->bStrikeThrough = true;
		break;

	case tokImage:
		OnGotImage( pList );
		break;

	case tokTableHeading:
	case tokTableDef:
		OnGotTableCell( pList );
		break;

	case tokTableRow:
		OnGotTableRow( pList );
		break;

	case tokTable:
		CreateNewProperties();
		OnGotTable( pList );
		break;

	case tokHorizontalRule:
		OnGotHR( pList );
		break;

	case tokCenter:
		CreateNewProperties();
		m_pProp->nAlignment = algCentre;
		OnGotParagraph( pList );
		break;

	case tokDiv:
		OnGotParagraph( pList );
		break;

	case tokParagraph:
		OnGotParagraph( pList );
		break;

	case tokBreak:
		m_strToken.Add( g_cCarriageReturn );
		break;

	case tokAnchor:
		OnGotAnchor( pList );
		break;

	case tokH1:
	case tokH2:
	case tokH3:
	case tokH4:
	case tokH5:
	case tokH6:
		OnGotHeading( token, pList );
		break;

	case tokOrderedList:
		OnGotOrderedList( pList );
		break;

	case tokUnorderedList:
		OnGotUnorderedList( pList );
		break;

	case tokListItem:
		OnGotListItem( pList );
		break;

	case tokAddress:
		OnGotAddress( pList );
		break;

	case tokBlockQuote:
		OnGotBlockQuote( pList );
		break;

	case tokCode:
		CreateNewTextObject();
		CreateNewProperties();
		_tcscpy( m_pProp->szFaceName, m_pDefaults->m_strDefaultPreFontName );
		break;

	case tokMeta:
		OnGotMeta( pList );
		break;
	}
}


void CHTMLParse::OnGotEndTag( const Token token )
{
	switch( token )
	{
	case tokCode:
		CreateNewTextObject();
		PopPreviousProperties();
		break;

	case tokPre:
		CreateNewTextObject();
		PopPreviousProperties();
		CreateNewParagraph( 0, 0, m_pProp->nAlignment );
		break;

	case tokCenter:
		CreateNewTextObject();
		PopPreviousProperties();
		CreateNewParagraph( 0, 0, m_pProp->nAlignment );
		break;
		
	case tokParagraph:
		CreateNewTextObject();
		CreateNewParagraph( 0, 0, m_pProp->nAlignment );
		break;

	case tokImage:				//	Ignore end images
		//	There is no end image tag!
		ASSERT( FALSE );
		break;

	case tokDiv:
	case tokIgnore:
		CreateNewTextObject();
		CreateNewParagraph( 0, 0, m_pProp->nAlignment );
		break;

	case tokFont:
	case tokBold:
	case tokUnderline:
	case tokItalic:
	case tokStrikeout:
	case tokSub:
	case tokSup:
		CreateNewTextObject();
		PopPreviousProperties();
		break;

	case tokH1:
	case tokH2:
	case tokH3:
	case tokH4:
	case tokH5:
	case tokH6:
		CreateNewTextObject();
		PopPreviousProperties();
		CreateNewParagraph( 0, 0, m_pProp->nAlignment );
		break;

	case tokAnchor:
		OnGotEndAnchor();
		break;

	case tokTableHeading:
	case tokTableDef:
		OnGotEndTableCell();
		break;

	case tokTableRow:
		OnGotEndTableRow();
		break;

	//
	//	IE seems to ne okay about using an end BR (</br>) so we should too.
	case tokBreak:
		m_strToken.Add( g_cCarriageReturn );
		break;

	case tokTable:
		if( m_stkTable.GetSize() )
		{
			m_stkTable.Pop();
		}
		else
		{
			TRACE( _T("Got an table but no tables left in the stack\n") );
		}

		// Pop the InACell flag
		if (m_stkInTableCell.GetSize())
		{
			// If the cell is still open... close it!
			OnGotEndTableRow();		// Does the same thing.
			m_stkInTableCell.Pop();
		}
		else
		{
			TRACE( _T("Got an end table but in-cell stack empty.") );
		}
		PopPreviousProperties();
		CreateNewParagraph( 0, 0, m_pProp->nAlignment );
		break;

	case tokHorizontalRule:
		break;

	case tokOrderedList:
	case tokUnorderedList:
		OnGotEndList();
		break;

	case tokListItem:
		OnGotEndListItem();
		break;

	case tokAddress:
		OnGotEndAddress();
		break;

	case tokBlockQuote:
		OnGotEndBlockQuote();
		break;

	case tokDocumentTitle:
		m_strToken.Add( 0 );
		m_pMasterDocument->m_strTitle = m_strToken.GetData();
		m_strToken.SetSize( 0 );
		break;
	}
}


void CHTMLParse::OnGotBody( const CParameters &pList )
{
	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;
		switch( pList[n].m_param )
		{
		case pBColor:
			m_pDocument->m_crBack = GetColourFromString( strParam, m_pDocument->m_crBack );
			break;
		case pLink:
			m_pDocument->m_crLink = GetColourFromString( strParam, RGB( 141, 7, 102 ) );
			break;
		case pALink:
			m_pDocument->m_crLinkHover = GetColourFromString( strParam, RGB( 29, 49, 149 ) );
			break;
		case pMarginTop:
			m_pDocument->m_nTopMargin = GetNumberParameter( strParam, m_pDocument->m_nTopMargin );
			break;
		case pMarginBottom:
			m_pDocument->m_nBottomMargin = GetNumberParameter( strParam, m_pDocument->m_nBottomMargin );
			break;
		case pMarginLeft:
			m_pDocument->m_nLeftMargin = GetNumberParameter( strParam, m_pDocument->m_nLeftMargin );
			break;
		case pMarginRight:
			m_pDocument->m_nRightMargin = GetNumberParameter( strParam, m_pDocument->m_nRightMargin );
			break;
		}
	}
}


void CHTMLParse::OnGotImage( const CParameters &pList )
{
	CreateNewTextObject();
	int nHeight = 0;
	int nWidth = 0;
	int nBorder = 0;
	CStaticString strFilename;
	Align alg = knDefaultImageAlignment;

	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;
		switch( pList[n].m_param )
		{
		case pWidth:
			nWidth = GetNumberParameter( strParam, nWidth );
			break;

		case pHeight:
			nHeight = GetNumberParameter( strParam, nHeight );
			break;

		case pAlign:
			alg = GetAlignmentFromString( strParam, alg );
			break;

		case pSrc:
			strFilename = strParam;
			break;

		case pBorder:
			nBorder = GetNumberParameter( strParam, nBorder );
			break;
		}
	}

	CHTMLParagraph *pPara = m_pDocument->CurrentParagraph();

	// Before loading the image, see if it already exists in the cache.
	StringClass fname( strFilename.GetData(), strFilename.GetLength() );

	CImage** ppLoadedImage = m_pMasterDocument->m_mapImages.Lookup(fname);
	CImage* pImage;
	if (!ppLoadedImage)
	{
		TRACENL( _T("Image %s not in cache. Attempting Load\n"), (LPCTSTR)fname);
		pImage = OnLoadImage( fname );
		if( !pImage )
		{
			TRACENL( _T("Image %s could not be loaded.\n"), (LPCTSTR)fname);
			// Consider making this image a global instance.
			pImage = new CImage;
			VERIFY( pImage->Load( g_hResourceInstance, g_uNoImageBitmapID ) );
		}
		m_pMasterDocument->m_mapImages.SetAt(fname, pImage);
	}
	else
	{
		TRACENL( _T("Image %s loaded from cache.\n"), (LPCTSTR)fname);
		pImage = *ppLoadedImage;
	}

	CHTMLImage *pHTMLImage = new CHTMLImage( nWidth, nHeight, nBorder, fname, alg, pImage );

	UpdateItemLinkStatus( pHTMLImage );
	pPara->AddItem( pHTMLImage );
}


CImage *CHTMLParse::OnLoadImage( LPCTSTR pcszFilename )
//
//	Called when we need an image to be loaded.
{
	return AquireImage( m_hInstLoadedFrom, m_pcszFilePath, pcszFilename );
}


void CHTMLParse::UpdateItemLinkStatus( CHTMLParagraphObject *pItem )
{
	// Assign the current link pointer to the object
	pItem->m_pAnchor = m_pLastAnchor;	

}


void CHTMLParse::CreateNewTextObject()
//
//	Create a new text block and add it to the current docuemnt-paragraph
{
	if( m_strToken.GetSize() )
	{
		CHTMLParagraph *pPara = m_pDocument->CurrentParagraph();
		HTMLFontDef def( m_pProp->szFaceName, m_pProp->nSize, m_pProp->bBold, m_pProp->bItalic, m_pProp->bUnderline, m_pProp->bStrikeThrough, m_pProp->m_nSup, m_pProp->m_nSub );

		CStaticString str( m_strToken.GetData(), m_strToken.GetSize() );
		CHTMLTextBlock *pText = new CHTMLTextBlock( str, m_pDocument->GetFontDefIndex( def ), m_pProp->m_crFore, IsPreformatted() );
		if( m_pLastAnchor )
			UpdateItemLinkStatus( pText );

		pPara->AddItem( pText );

		m_strToken.SetSize( 0 );
	}
}


void CHTMLParse::CreateNewProperties()
//
//	Create a new properties set and add it to the stack.
{
	m_pProp = new Properties( *m_pProp );
	m_stkProperties.Push( m_pProp );
}


void CHTMLParse::PopPreviousProperties()
{
	if( m_stkProperties.GetSize() > 1 )
	{
		delete m_stkProperties.Pop();
		m_pProp = m_stkProperties.Top();
	}
}


void CHTMLParse::CreateNewParagraph( int nLinesAbove, int nLinesBelow, Align alg )
{
	if( m_pDocument->CurrentParagraph() && m_pDocument->CurrentParagraph()->IsEmpty() )
	{
		m_pDocument->CurrentParagraph()->Reset( nLinesAbove, nLinesBelow, alg );
	}
	else
	{
		m_pDocument->AddParagraph( new CHTMLParagraph( nLinesAbove, nLinesBelow, alg ) );
	}
}


void CHTMLParse::OnGotHR( const CParameters &pList )
{
	CreateNewTextObject();
	if( !m_pDocument->CurrentParagraph() || !m_pDocument->CurrentParagraph()->IsEmpty() )
	{
		CreateNewParagraph( m_pDefaults->m_nParagraphLinesAbove, m_pDefaults->m_nParagraphLinesBelow, m_pProp->nAlignment );
	}

	CHTMLParse::Align alg = knDefaultHRAlignment;
	int nSize = 2;
	int nWidth = -100;
	bool bNoShade = false;
	COLORREF crColor = 0;

	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;

		switch( pList[n].m_param )
		{
		case pWidth:
			nWidth = GetNumberParameterPercent( strParam, nWidth );
			if( nWidth == 0 )
				nWidth = -100;
			break;

		case pSize:
			//	REVIEW - russf - could allow dan to use point/font sizes here too.
			nSize = GetNumberParameter(strParam, nSize);
			if( nSize <= 0 )
				nSize = 3;
			break;

		case pNoShade:
			bNoShade = true;
			break;

		case pAlign:
			alg = GetAlignmentFromString( strParam, alg );
			break;

		case pColor:
			crColor = GetColourFromString( strParam, crColor );
			break;
		}
	}

	CHTMLParagraph *pPara = m_pDocument->CurrentParagraph();
	CHTMLHorizontalRule *pHR = new CHTMLHorizontalRule( alg, nSize, nWidth, bNoShade, crColor );
	UpdateItemLinkStatus( pHR );
	pPara->AddItem( pHR );
	CreateNewParagraph( m_pDefaults->m_nParagraphLinesAbove, m_pDefaults->m_nParagraphLinesBelow, m_pProp->nAlignment );
}


void CHTMLParse::OnGotParagraph( const CParameters &pList )
{
	CreateNewTextObject();
	CHTMLParse::Align alg = m_pProp->nAlignment;

	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;

		switch( pList[n].m_param )
		{
		case pAlign:
			alg = GetAlignmentFromString( strParam, alg );
			break;
		}
	}

	m_pProp->nAlignment;
	CreateNewParagraph( m_pDefaults->m_nParagraphLinesAbove, m_pDefaults->m_nParagraphLinesBelow, alg );
}


void CHTMLParse::OnGotFont( const CParameters &pList )
{
	CreateNewTextObject();
	CreateNewProperties();

	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;
		switch( pList[n].m_param )
		{
		case pColor:
			m_pProp->m_crFore = GetColourFromString( strParam, m_pProp->m_crFore );
			break;

		case pFace:
			GetFontName( m_pProp->szFaceName, countof( m_pProp->szFaceName ), strParam );
			break;

		case pSize:
			{
				//
				//	REVIEW - russf - Could add the ability to have +n and -n font and point sizes here.
				m_pProp->nSize = GetFontSize( strParam, m_pProp->nSize );
			}
			break;
		}
	}
}


void CHTMLParse::OnGotHeading( const Token token, const CParameters &pList )
{
	int nLinesAbove = 1, nLinesBelow = 1;
	Align alg	= m_pProp->nAlignment;

	CreateNewTextObject();
	CreateNewProperties();

	switch( token )
	{
	case tokH1:
		m_pProp->nSize = 6;
		m_pProp->bBold = true;
		break;

	case tokH2:
		m_pProp->nSize = 5;
		m_pProp->bBold = true;
		break;

	case tokH3:
		m_pProp->nSize = 4;
		m_pProp->bBold = true;
		break;

	case tokH4:
		m_pProp->nSize = 3;
		m_pProp->bBold = true;
		break;

	case tokH5:
		m_pProp->nSize = 3;
		m_pProp->bBold = true;
		break;

	case tokH6:
		nLinesAbove = 2;
		m_pProp->nSize = 1;
		m_pProp->bBold = true;
		break;
	}

	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;
		switch( pList[n].m_param )
		{
		case pAlign:
			alg = GetAlignmentFromString( strParam, alg );
			break;
		}
	}

	CreateNewParagraph( nLinesAbove, nLinesBelow, alg );

}

void CHTMLParse::OnGotAnchor( const CParameters &pList )
{
	//	We will no longer store link attributes in the properties.
	//	They will instead be stored in an inline ParagraphObject
	//	The sectionCreator will be responsible for keeping the list
	//	of links and map of named sections
	
	//	Since the coloring will change, we will still create a new
	//	text object. This also ensures that we have a paragraph into which
	//	we store the HTMLAnchor object.
	CreateNewTextObject();

	//	Anchor information is no longer stored in the properties stack,
	//	So we don't need new properties.
	//	CreateNewProperties();


	//	Create a new Anchor object, then store some data in it.
	//	This one has a default ctor, which provides empty members.
	//	We need to specify colors, though.

	CHTMLAnchor* pAnchor = new CHTMLAnchor();

	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;
		switch( pList[n].m_param )
		{
		case pName:
			pAnchor->m_strLinkName.Set( strParam.GetData(), strParam.GetLength() );
			break;

		case pHref:
			pAnchor->m_strLinkTarget.Set( strParam.GetData(), strParam.GetLength() );
			break;

		case pTitle:
			pAnchor->m_strLinkTitle.Set( strParam.GetData(), strParam.GetLength() );
			break;
		}
	}
	//	Get the colors from the master document, which is the only place they should be set
	pAnchor->m_crLink = m_pMasterDocument->m_crLink;
	pAnchor->m_crHover = m_pMasterDocument->m_crLinkHover;

	
	//	Signifies to the parser that we are in a anchor tag, and this is it!
	//	This remains complicated by named tags. i.e. what should happen
	//	in this case:
	//	<a href="someTarget">Some text<a name="here">More Text</a> Other Text</a>
	//	is the Other Text in a link?
	//	In IE4, the second tag cancels out the first. We will use that behavior.
	if (pAnchor->m_strLinkTarget.GetLength())
		m_pLastAnchor = pAnchor;
	else
		m_pLastAnchor = NULL;

	CHTMLParagraph *pPara = m_pDocument->CurrentParagraph();
	pPara->AddItem( pAnchor );
}

void CHTMLParse::OnGotEndAnchor()
{
	//	Signify to the parser that we are no longer in an anchor tag.
	//	If we are truly anding a link, make a new text object.
	//	Otherwise, it's ignored.
	if (m_pLastAnchor)
		CreateNewTextObject();
	m_pLastAnchor = NULL;
}

void CHTMLParse::OnGotTable( const CParameters &pList )
{
	CreateNewTextObject();
	int nWidth = 0;	// Default width - unspec, -100;	//	For 100 percent
	int nBorder = 0;
	int nCellPadding = m_pDefaults->m_nCellPadding;
	int nCellSpacing = m_pDefaults->m_nCellSpacing;
	bool bTransparent = true;
	COLORREF crBorder = RGB(0, 0, 0);
	COLORREF crBgColor = RGB( 255, 255, 255 );	// Not really used.
	COLORREF crBorderLight = m_pDefaults->m_crBorderLight;
	COLORREF crBorderDark = m_pDefaults->m_crBorderDark;
	bool bBorderDarkSet = false;
	bool bBorderLightSet = false;

	CHTMLParse::Align alg = knDefaultTableAlignment;
	CHTMLParse::Align valg = knDefaultTableAlignment;

	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;
		switch( pList[n].m_param )
		{
		case pWidth:
			nWidth = GetNumberParameterPercent( strParam, nWidth );
			break;

		case pAlign:
			alg = GetAlignmentFromString( strParam, alg );
			break;

		case pVAlign:
			valg = GetAlignmentFromString( strParam, valg );
			break;

		case pBorder:
			nBorder = GetNumberParameter( strParam, nBorder );
			break;

		case pBColor:
			crBgColor = GetColourFromString( strParam, crBgColor );
			bTransparent = false;
			break;

		case pCellSpacing:
			nCellSpacing = GetNumberParameter( strParam, nCellSpacing );
			break;

		case pCellPadding:
			nCellPadding = GetNumberParameter( strParam, nCellPadding );
			break;

		case pBorderColor:
			crBorder = GetColourFromString( strParam, crBorder );
			if ( !bBorderDarkSet )
				crBorderDark = crBorder;
			if ( !bBorderLightSet )
				crBorderLight = crBorder;
			break;

		case pBorderColorLight:
			crBorderLight = GetColourFromString( strParam, crBorderLight );
			bBorderLightSet = true;
			break;

		case pBorderColorDark:
			crBorderDark = GetColourFromString( strParam, crBorderDark );
			bBorderDarkSet = true;
			break;

		}
	}

	switch( alg )
	{
	case algLeft: case algRight:
		break;
	default:
		CreateNewParagraph( 0, 0, m_pProp->nAlignment );
	}

	CHTMLTable *ptab = new CHTMLTable( nWidth, nBorder, alg, valg, nCellSpacing, nCellPadding, bTransparent, crBgColor, crBorderDark, crBorderLight);
	m_stkTable.Push( ptab  );
	m_pDocument->CurrentParagraph()->AddItem( m_stkTable.Top() );
	// Push onto the inTableCell stack
	m_stkInTableCell.Push(false);	// Not in a cell yet!
}


void CHTMLParse::OnGotTableRow( const CParameters &pList )
{
	// Close an open cell first.
	if (m_stkInTableCell.GetSize() && m_stkInTableCell.Top())
		OnGotEndTableCell();

	CHTMLParse::Align valg = knDefaultTableAlignment;
	if( m_stkTable.GetSize() )
	{
		valg = m_stkTable.Top()->m_valg;
	}

	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;
		switch( pList[n].m_param )
		{
		case pVAlign:
			valg = GetAlignmentFromString( strParam, valg );
			break;
		}
	}

	if( m_stkTable.GetSize() )
	{
		m_stkTable.Top()->NewRow( valg );
	}
}


void CHTMLParse::OnGotTableCell( const CParameters &pList )
{
	// Ensure that we are processing documents properly.
	// close the previous cell if it was left open...
	if (m_stkInTableCell.GetSize())
	{
		if (m_stkInTableCell.Top())
			OnGotEndTableCell();
		m_stkInTableCell.Top() = true;
	}

	int nWidth = 0;
	int nHeight = 0;
	bool bNoWrap = false;
	bool bTransparent = true;
	COLORREF crBorder = RGB(0,0,0);
	COLORREF crBgColor = RGB( 255, 255, 255 );	// Not really used.
	COLORREF crBorderLight = m_pDefaults->m_crBorderLight;
	COLORREF crBorderDark = m_pDefaults->m_crBorderDark;
	bool bBorderDarkSet = false;
	bool bBorderLightSet = false;

	CHTMLParse::Align alg = knDefaultTableCellAlignment;
	CHTMLParse::Align valg = m_stkTable.Top()->GetCurrentRow()->m_valg;

	// Get defaults from the current table
	// Could really ignore everything if we are not in a table!
	if( m_stkTable.GetSize() )
	{
		CHTMLTable* pTab = m_stkTable.Top();
		crBorderLight = pTab->m_crBorderLight;
		crBorderDark = pTab->m_crBorderDark;
	}

	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;
		switch( pList[n].m_param )
		{
		case pWidth:
			nWidth = GetNumberParameterPercent( strParam, nWidth );
			break;

		case pHeight:
			nHeight = GetNumberParameter( strParam, nHeight );
			break;

		case pAlign:
			alg = GetAlignmentFromString( strParam, alg );
			break;

		case pVAlign:
			valg = GetAlignmentFromString( strParam, alg );
			break;

		//  richg - 19990224 - Add support for NOWRAP
		case pNoWrap:
			bNoWrap = true;
			break;

		case pBColor:
			crBgColor = GetColourFromString( strParam, crBgColor );
			bTransparent = false;
			break;

		case pBorderColor:
			crBorder = GetColourFromString( strParam, crBorder );
			if ( !bBorderDarkSet )
				crBorderDark = crBorder;
			if ( !bBorderLightSet )
				crBorderLight = crBorder;
			break;

		case pBorderColorLight:
			crBorderLight = GetColourFromString( strParam, crBorderLight );
			bBorderLightSet = true;
			break;

		case pBorderColorDark:
			crBorderDark = GetColourFromString( strParam, crBorderDark );
			bBorderDarkSet = true;
			break;

		}
	}

	//
	//	If there is a table to add our cell to...
	if( m_stkTable.GetSize() )
	{
		CHTMLTableCell *pCell = new CHTMLTableCell( m_pDefaults, nWidth, nHeight, bNoWrap, bTransparent, crBgColor, crBorderDark, crBorderLight, valg );
		// Copy the current documents link colors into the new document
		pCell->m_crLink = m_pDocument->m_crLink;
		pCell->m_crLinkHover = m_pDocument->m_crLinkHover;
		// Push it onto the document stack
		m_stkDocument.Push( m_pDocument );

		// TRACENL("Document Stack Pushed. %d\n", m_stkDocument.GetSize());

		m_pDocument = pCell;
		m_pProp->nAlignment = alg;
		CreateNewParagraph( 0, 0, m_pProp->nAlignment );
		m_stkTable.Top()->AddCell( pCell );
	}
}


void CHTMLParse::OnGotEndTableCell()
{
	CreateNewTextObject();
	if( m_stkDocument.GetSize() )
	{
		m_pDocument = m_stkDocument.Pop();
		// TRACENL("Document Stack Popped. %d\n", m_stkDocument.GetSize());
	}
	else
	{	
		TRACE( _T("Got an end table cell but no document stack left\n") );
	}
	if (m_stkInTableCell.GetSize())
		m_stkInTableCell.Top() = false;
}


void CHTMLParse::OnGotEndTableRow()
{
	if( m_stkInTableCell.GetSize() && m_stkInTableCell.Top() )
		OnGotEndTableCell();
}

void CHTMLParse::CreateList( bool bOrdered )
{
	CreateNewTextObject();
	CreateNewParagraph( 0, 0, CHTMLParse::algLeft );

	CHTMLList *pList = new CHTMLList( bOrdered );
	m_stkList.Push( pList  );
	m_pDocument->CurrentParagraph()->AddItem( m_stkList.Top() );

	// Create the default first item
	CHTMLListItem *pItem = new CHTMLListItem( m_pDefaults, false, m_pProp->szFaceName, m_pProp->nSize, m_pProp->bBold, m_pProp->bItalic, m_pProp->bUnderline, m_pProp->bStrikeThrough, m_pProp->m_crFore );	// Not bulleted
	// Copy the current documents link colors into the new document
	pItem->m_crLink = m_pDocument->m_crLink;
	pItem->m_crLinkHover = m_pDocument->m_crLinkHover;
	m_stkDocument.Push( m_pDocument );
	m_pDocument = pItem;
	CreateNewParagraph( 0, 0, CHTMLParse::algLeft );
	m_stkList.Top()->AddItem( pItem ); 
}

void CHTMLParse::OnGotUnorderedList( const CParameters & /* pList */ )
{
	CreateList( false );
}

void CHTMLParse::OnGotOrderedList( const CParameters & /* pList */ )
{
	CreateList( true );
}

void CHTMLParse::OnGotListItem( const CParameters & /* pList */ )
{
	// </li> is completely ignored by IE4, and by NetScape
	// Additionally, items occuring after the <ol> or <ul> and
	// before the first <li> are positioned as if they were preceeded
	// by <li>, but the bullet is not shown. Empty <LI> tags DO show their
	// bullets.
	// In order to handle these cases, do the following:
	// For a new list, create the initial list item, but flag it as
	// unbulleted. When a <LI> is reached, determine if there is
	// exactly one item in the list. If so, and it is empty, Mark the
	// existing item as bulleted, and use it... otherwise, proceed by adding
	// another list item.
	// See that we are in a list, otherwise, ignore it...
	if ( m_stkList.GetSize() )
	{
		CreateNewTextObject();
		// See if we can use the existing item
		if ( m_stkList.Top()->m_arrItems.GetSize() == 1 && 
			!m_stkList.Top()->m_arrItems[0]->m_bBullet &&  
			m_stkList.Top()->m_arrItems[0]->IsEmpty() )
		{
			// There is en emptry entry that is not bulleted
			m_stkList.Top()->m_arrItems[0]->m_bBullet = true;
			// Nothing more to do!
			return;
		}
		// We are in a list and creating a new item, close the previous item
		CreateNewTextObject();
		if( m_stkDocument.GetSize() )
		{
			m_pDocument = m_stkDocument.Pop();
		}
		// Need to create a new item...
		CHTMLListItem *pItem = new CHTMLListItem( m_pDefaults, true, m_pProp->szFaceName, m_pProp->nSize, m_pProp->bBold, m_pProp->bItalic, m_pProp->bUnderline, m_pProp->bStrikeThrough, m_pProp->m_crFore );	// bulleted
		// Copy the current documents link colors into the new document
		pItem->m_crLink = m_pDocument->m_crLink;
		pItem->m_crLinkHover = m_pDocument->m_crLinkHover;
		m_stkDocument.Push( m_pDocument );
		m_pDocument = pItem;
		CreateNewParagraph( 0, 0, CHTMLParse::algLeft );
		m_stkList.Top()->AddItem( pItem ); 
	}
}


void CHTMLParse::OnGotEndListItem()
{
	// Do nothing.
}

void CHTMLParse::OnGotEndList()
{
	// See that we are in a list, otherwise, ignore it...
	if ( m_stkList.GetSize() )
	{
		// Close the last item...
		CreateNewTextObject();
		if( m_stkDocument.GetSize() )
		{
			m_pDocument = m_stkDocument.Pop();
		}
		CreateNewParagraph( 0, 0, CHTMLParse::algLeft );
		// Close this list.
		m_stkList.Pop();
	}
}


void CHTMLParse::OnGotBlockQuote( const CParameters & /* pList */ )
{
	/*
		A Blockquote is treated as a sub document with different
		margins. This is fairly straightforward.
	*/
	CreateNewTextObject();
	CreateNewParagraph( 0, 0, m_pProp->nAlignment );

	CHTMLBlockQuote *pBQ = new CHTMLBlockQuote( m_pDefaults );
	m_pDocument->CurrentParagraph()->AddItem( pBQ );
	// Make the sub document the main document
	m_stkDocument.Push( m_pDocument );
	m_pDocument = pBQ->m_pDoc;
	CreateNewParagraph( 0, 0, m_pProp->nAlignment);
}


void CHTMLParse::OnGotEndBlockQuote()
{
	/*
		Cause a document stack pop if possible...
	*/
	CreateNewTextObject();
	if( m_stkDocument.GetSize() )
	{
		m_pDocument = m_stkDocument.Pop();
	}
	else
	{
		TRACE( _T("At </BLOCKQUOTE> but document stack is empty.\n") );
	}
	CreateNewParagraph(0, 0, m_pProp->nAlignment);
}



void CHTMLParse::OnGotAddress( const CParameters & /* pList */ )
//
//	Fundtionally the same as BLOCKQUOTE but with italics
{
	CreateNewTextObject();
	CreateNewParagraph( 0, 0, m_pProp->nAlignment );

	CreateNewProperties();
	m_pProp->bItalic = true;

	CHTMLBlockQuote *pBQ = new CHTMLBlockQuote( m_pDefaults );
	m_pDocument->CurrentParagraph()->AddItem( pBQ );
	// Make the sub document the main document
	m_stkDocument.Push( m_pDocument );
	m_pDocument = pBQ->m_pDoc;
	CreateNewParagraph( 0, 0, m_pProp->nAlignment);
}


void CHTMLParse::OnGotEndAddress()
{
	CreateNewTextObject();
	if( m_stkDocument.GetSize() )
	{
		m_pDocument = m_stkDocument.Pop();
	}
	else
	{
		TRACE( _T("At </BLOCKQUOTE> but document stack is empty.\n") );
	}
	PopPreviousProperties();
	CreateNewParagraph(0, 0, m_pProp->nAlignment);
}


void CHTMLParse::GetFontName( LPTSTR pszBuffer, int nBufferSize, const CStaticString &strFontNameSpec )
//
//	Get the font name from a font name spec.
//	This consists of comma delimited font names "tahoma,arial,helvetica"
//	We need to find
{
	static const TCHAR cComma = _T(',');
	if( strFontNameSpec.GetData() )
	{
		LPCTSTR pcszFontNameSpec = strFontNameSpec.GetData();
		//	Set our string to empty
		*pszBuffer = _T('\000');

		if( strFontNameSpec.Find( cComma ) )
		{
			//
			//	Iterate over the passed font names, copying each font name as we go.
			LPCTSTR pcszFontNameSpecEnd = strFontNameSpec.GetEndPointer();
			LPCTSTR pszFontNameStart, pszFontNameEnd;
			pszFontNameStart = pszFontNameEnd = pcszFontNameSpec;

			while( pszFontNameStart < pcszFontNameSpecEnd )
			{
				//
				//	Find the end of the first font.
				while( pszFontNameEnd < pcszFontNameSpecEnd && *pszFontNameEnd != cComma )
					pszFontNameEnd++;

				if( pszFontNameEnd - pszFontNameStart && pszFontNameEnd - pszFontNameStart < nBufferSize)
				{
					StringClass strFontName( pszFontNameStart, pszFontNameEnd - pszFontNameStart );

					//
					//	We have our font name, now determine whether the font exists or not.
					if( DoesFontExist( strFontName ) )
					{
						//	It does, then we get out of here. If the font doesn't exist then we simply
						//	continue looping around.
						_tcscpy( pszBuffer, strFontName );
						return;
					}
				}


				//
				//	Skip past the white space and commas
				while( pszFontNameEnd < pcszFontNameSpecEnd && ( isspace( *pszFontNameEnd ) || *pszFontNameEnd == cComma ) )
					pszFontNameEnd++;

				pszFontNameStart = pszFontNameEnd;
			}
		}
		else
		{
			StringClass strFontName( strFontNameSpec.GetData(), strFontNameSpec.GetLength() );
			if( DoesFontExist( strFontName ) )
			{
				//	It does, then we get out of here. If the font doesn't exist then we simply
				//	continue looping around.
				_tcscpy( pszBuffer, strFontName );
			}
			else
			{
				_tcscpy( pszBuffer, m_pDefaults->m_strFontName );
			}
		}

	}
	else
	{
		_tcscpy( pszBuffer, m_pDefaults->m_strFontName );
	}

	//
	//	If we drop out here it means that none of the user requested fonts exist on the system.
}

static int CALLBACK EnumFontFamExProc( ENUMLOGFONTEX * /*lpelfe*/, NEWTEXTMETRICEX * /*lpntme*/, int /*FontType*/, LPARAM /*lParam*/ )
{
	return -1;
}


bool CHTMLParse::DoesFontExist( const StringClass &strFontName )
//
//	Determine if the font exists in our map and return that result.
//	If it doesn't exist in our map then ask the system if the font exists and add
//	that known state to our map.
{
	bool *pb = g_mapFontName.Lookup( strFontName );
	if( pb )
	{
		return *pb;
	}

	LOGFONT lf = { 0 };
	_tcscpy( lf.lfFaceName, strFontName );
	lf.lfCharSet = m_pMasterDocument->m_cCharSet;

	CDrawContext dc;
	bool bExists = false;

	//
	//	NOTE: If the font doesn't exist EnumFontFamiliesEx returns 1 - originally I had my callbcak return
	//	true or false. This didn't work if the font didn't exist. Now the callback returns -1 if the font
	//	exists.
	if( EnumFontFamiliesEx( dc.GetSafeHdc(), &lf, (FONTENUMPROC)EnumFontFamExProc, 0, 0 ) == -1 )
		bExists = true;
	g_mapFontName.SetAt( strFontName, bExists );
#ifdef _DEBUG
	if( !bExists )
	{
		TRACE( "Font doesn't exist '%s'\n", strFontName );
	}
#endif	//	_DEBUG
	return bExists;
}


void CHTMLParse::OnGotMeta( const CParameters &pList )
{
	const UINT uParamSize = pList.GetSize();
	for( UINT n = 0; n < uParamSize; n++ )
	{
		const CStaticString &strParam = pList[n].m_strValue;
		switch( pList[n].m_param )
		{
		case pContent:
			{
				LPCTSTR pcszEnd = strParam.GetData() + strParam.GetLength();
				static const TCHAR szCharSet[] = _T("charset");
				LPCTSTR p = stristr( (LPTSTR)strParam.GetData(), szCharSet );
				if( p )
				{
					//
					//	We have a charset, so next we need to decode it
					p += countof( szCharSet ) - 1;
					while( p < pcszEnd && ( isspace( *p ) || *p == _T('=') ) )
						p++;
					if( p < pcszEnd )
					{
						const CStaticString str( p, pcszEnd - p );
						m_pDocument->m_cCharSet = DecodeCharset( str );
					}
				}
			}
			break;

		case pHTTPEquiv:
			//TRACE( "pHTTPEquiv %s\n", strParam );
			break;
		}
	}
}