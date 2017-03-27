/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLSectionCreator.cpp
Owner:	russf@gipsysoft.com
Purpose:	Section creator for the HTML display.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <math.h>
#include <ImgLib.h>
#include "HTMLParse.h"
#include "HTMLSectionCreator.h"
#include "HTMLTextSection.h"
#include "HTMLImageSection.h"
#include "HTMLHorizontalRuleSection.h"
#include "HTMLSection.h"
#include "HTMLTableSection.h"
#include "tableLayout.h"
#include "defaults.h"

CHTMLSectionCreator::CHTMLSectionCreator( CHTMLSection *psect, CDrawContext &dc, int nTop, int nLeft, int nRight, COLORREF crBack, bool bMeasuring, int nZoomLevel )
	: m_psect( psect )
	, m_dc( dc )

	, m_nDefaultLeftMargin( nLeft )
	, m_nDefaultRightMargin( nRight )
	, m_crBack( crBack )
	, m_nLeftMargin( nLeft )
	, m_nRightMargin( nRight )

	, m_nYPos( nTop )
	, m_nTop( nTop )
	, m_nPreviousParaSpaceBelow( 0 )

	, m_nFirstShapeOnLine( 0 )
	, m_nNextYPos( nTop )
	, m_nWidth( 0 )
	, m_bMeasuring( bMeasuring )
	, m_nBaseLineShapeStartID( 0 )
	, m_nLowestBaseline( 0 )
	, m_algCurrentPargraph( CHTMLParse::algLeft )
	, m_nZoomLevel( nZoomLevel )
	, m_nXPos( 0 )
{
	m_dc.SelectFont( g_defaults.m_strFontName, GetFontSizeAsPixels( m_dc.GetSafeHdc(), g_defaults.m_nFontSize, m_nZoomLevel ), FW_NORMAL, false, false, false, g_defaults.m_cCharSet );
}

CHTMLSectionCreator::~CHTMLSectionCreator()
{

}


void CHTMLSectionCreator::CarriageReturn()
//
//	Recalcualtes the left and right margins for the current Y position and
//	moves the X position to the new left margin.
{
	if( GetCurrentShapeID() )
	{
		AdjustShapeBaselinesAndHorizontalAlignment();
	}
	m_nBaseLineShapeStartID = GetCurrentShapeID();

	m_nLeftMargin = m_nDefaultLeftMargin;
	while( m_stkLeftMargin.GetSize() )
	{
		MarginStackItem msi = m_stkLeftMargin.Top();
		if( msi.nYExpiry >= m_nYPos )
		{
			m_nLeftMargin = msi.nMargin;
			break;
		}
		else
		{
			m_stkLeftMargin.Pop();
		}
	}


	m_nRightMargin = m_nDefaultRightMargin;
	while( m_stkRightMargin.GetSize() )
	{
		MarginStackItem msi = m_stkRightMargin.Top();
		if( msi.nYExpiry >= m_nYPos )
		{
			m_nRightMargin = msi.nMargin;
			break;
		}
		else
		{
			m_stkRightMargin.Pop();
		}
	}
	m_nXPos = m_nLeftMargin;

	m_arrBaseline.RemoveAll();
	m_nLowestBaseline = 0;
	m_nFirstShapeOnLine = GetCurrentShapeID();

	//	If there is nothing in the margin stack, this is a good place for
	//	a page break.
	if( m_dc.IsPrinting() && m_nLeftMargin == m_nDefaultLeftMargin && m_nRightMargin == m_nDefaultRightMargin)
	{
		m_psect->AddBreakSection(GetCurrentShapeID());
	}

}


void CHTMLSectionCreator::Finished()
//
//	Signals the end fo the document.
{
	CarriageReturn();
	m_nYPos = m_nNextYPos;

	//
	//	Adjust the height of our document to take into account any images	there may be on the left or right
	//	of the document and that may be taller than we have left the document
	while( m_stkRightMargin.GetSize() )
	{
		MarginStackItem msi = m_stkRightMargin.Top();
		if( msi.nYExpiry > m_nYPos )
			m_nYPos = msi.nYExpiry;
		m_stkRightMargin.Pop();
	}

	while( m_stkLeftMargin.GetSize() )
	{
		MarginStackItem msi = m_stkLeftMargin.Top();
		if( msi.nYExpiry > m_nYPos )
			m_nYPos = msi.nYExpiry;
		m_stkLeftMargin.Pop();
	}
	if( m_dc.IsPrinting() )
		m_psect->AddBreakSection(GetCurrentShapeID());
}


void CHTMLSectionCreator::NewParagraph( int nSpaceAbove, int nSpaceBelow, CHTMLParse::Align alg )
//
//	New paragraph.
//	Add on the space below from the previous paragraph and the space above for this paragraph.
{
	CarriageReturn();
	m_nYPos = m_nNextYPos;
	if( m_nYPos != m_nTop )
	{
		m_nYPos += ( m_dc.GetCurrentFontHeight() * m_nPreviousParaSpaceBelow ) / 2;
		m_nYPos += ( m_dc.GetCurrentFontHeight() * nSpaceAbove ) / 2;
	}
	m_algCurrentPargraph = alg;
	m_nPreviousParaSpaceBelow = nSpaceBelow;
}


void CHTMLSectionCreator::AddTextPreformat( const StringClass &str, const HTMLFontDef &htmlfdef, COLORREF crFore, CHTMLSectionLink* pLink )
{
	FontDef fdef( htmlfdef.m_strFont, htmlfdef.m_nSize, htmlfdef.m_nWeight, htmlfdef.m_bItalic, htmlfdef.m_bUnderline, htmlfdef.m_bStrike, m_cCharSet );
	fdef.m_nSizePixels = GetFontSizeAsPixels( m_dc.GetSafeHdc(), htmlfdef.m_nSize, m_nZoomLevel );

	if( pLink )
		fdef.m_bUnderline = true;

	m_dc.SelectFont( fdef );

	LPCTSTR pcszTextLocal = str;
	LPCTSTR pcszPrevious = str;
	while( 1 )
	{
		if( *pcszTextLocal == '\r' || *pcszTextLocal == '\n' || *pcszTextLocal == '\000' )
		{
			const UINT uLength = pcszTextLocal - pcszPrevious;
			const CSize size( m_dc.GetTextExtent( pcszPrevious, uLength ), m_dc.GetCurrentFontHeight() );

			CHTMLTextSection *pText = new CHTMLTextSection( m_psect, pcszPrevious, uLength, fdef, crFore );
			pText->Set( m_nXPos, m_nYPos, m_nXPos + size.cx, m_nYPos + size.cy );

			AddBaseline( m_dc.GetCurrentFontBaseline() );

			CarriageReturn();
			m_nYPos = m_nNextYPos;

			if( *pcszTextLocal == '\000' )
			{
				break;
			}

			if( *pcszTextLocal == '\r' && *( pcszTextLocal + 1 ) == '\n' )
			{
				*pcszTextLocal++;
			}
			pcszPrevious = pcszTextLocal + 1;
		}
		*pcszTextLocal++;
	}
}


void CHTMLSectionCreator::AddText( const StringClass &str, const HTMLFontDef &htmlfdef, COLORREF crFore, CHTMLSectionLink* pLink )
{
	FontDef fdef( htmlfdef.m_strFont, htmlfdef.m_nSize, htmlfdef.m_nWeight, htmlfdef.m_bItalic, htmlfdef.m_bUnderline, htmlfdef.m_bStrike, m_cCharSet );
	fdef.m_nSizePixels = GetFontSizeAsPixels( m_dc.GetSafeHdc(), htmlfdef.m_nSize, m_nZoomLevel );

	if( pLink )
		fdef.m_bUnderline = true;

	m_dc.SelectFont( fdef );

	const int nCurrentFontHeight = m_dc.GetCurrentFontBaseline();

	/*
		try the text to see if it fits.
			if it does then simply create a new object and bump along the XPos
			if it does not then create an object with the least amount of data to fit
				create a new line (CarriageReturn) and try again.
	*/
	LPCTSTR pcszTextLocal = str;
	LPCTSTR pcszPrevious = str;
	CSize size;
	bool bDone = false;
	while( !bDone )
	{
		CHTMLTextSection *pText = NULL;

		//	richg - 19990224 - Altered to indicate the the CDrawContext whether or not
		//	we are at the beginning of a line.
		if( m_dc.GetTextFitWithWordWrap( m_nRightMargin - m_nXPos, pcszTextLocal, size, (bool)(m_nXPos == m_nLeftMargin) ) )
		{
			pText = new CHTMLTextSection( m_psect, pcszPrevious, pcszTextLocal - pcszPrevious, fdef, crFore );
			pText->Set( m_nXPos, m_nYPos, m_nXPos + size.cx, m_nYPos + size.cy );
			m_nXPos+=size.cx;
			if( htmlfdef.m_nSup )
			{
				AddBaseline( nCurrentFontHeight + htmlfdef.m_nSup * nCurrentFontHeight / 3 );
			}
			else if( htmlfdef.m_nSub )
			{
				AddBaseline( nCurrentFontHeight - htmlfdef.m_nSub * nCurrentFontHeight / 3 );
			}
			else
			{
				AddBaseline( nCurrentFontHeight );
			}
			bDone = true;
		}
		else
		{
			if( pcszTextLocal - pcszPrevious > 0 && ( m_nLeftMargin == m_nXPos || _istspace( *pcszTextLocal ) ) )
			{
				pText = new CHTMLTextSection( m_psect, pcszPrevious, pcszTextLocal - pcszPrevious, fdef, crFore );
				pText->Set( m_nXPos, m_nYPos, m_nXPos + size.cx, m_nYPos + size.cy );
				if( htmlfdef.m_nSup )
				{
					AddBaseline( nCurrentFontHeight + htmlfdef.m_nSup * nCurrentFontHeight / 3 );
				}
				else if( htmlfdef.m_nSub )
				{
					AddBaseline( nCurrentFontHeight - htmlfdef.m_nSub * nCurrentFontHeight / 3 );
				}
				else
				{
					AddBaseline( nCurrentFontHeight );
				}
			}
			else
			{
				pcszTextLocal = pcszPrevious;
			}
			CarriageReturn();
			m_nYPos = m_nNextYPos;

			//	Skip over trailing spaces
			while( *pcszTextLocal && _istspace( *pcszTextLocal ) ) pcszTextLocal++;
			pcszPrevious = pcszTextLocal;
		}

		if( pText && pLink )
		{
			pText->SetAsLink( pLink );
		}
	}
}


int CHTMLSectionCreator::GetCurrentShapeID() const
{
	return m_psect->GetSectionCount();
}


void CHTMLSectionCreator::AddBaseline( int nBaseline )
{
	UINT nIndex = GetCurrentShapeID() - m_nBaseLineShapeStartID;
	while( nIndex > m_arrBaseline.GetSize() )
		m_arrBaseline.Add( -1 );

	m_arrBaseline[ nIndex - 1 ] = nBaseline;
	if( nBaseline > m_nLowestBaseline )
	{
		m_nLowestBaseline = nBaseline;
	}
}


int CHTMLSectionCreator::GetBaseline( int nShape ) const
{
	// richg - Made this a little more tolerant of error.
	if (nShape < m_nBaseLineShapeStartID)
		return -1;
	const UINT index = nShape - m_nBaseLineShapeStartID;
	if( index < m_arrBaseline.GetSize())
	{
		return m_arrBaseline[ index ];
	}
	return -1;
}


void CHTMLSectionCreator::AdjustShapeBaselinesAndHorizontalAlignment()
//
//	Adjust the shapes baselines to allow for different heights and styles of text
{
	int nHorizontalDelta = 0;

	//
	//	Horizontally align the shapes.
	//
	//	Get the horizontal extent of allof the shapes.
	//	if right aligned then move allthe shapes on the line by the difference
	//		between the width between the margins and the line extent.
	//	if centre aligned then move the shapes half the difference between the
	//		margins and the line extent.
	if( !m_bMeasuring && m_algCurrentPargraph != CHTMLParse::algLeft && m_nFirstShapeOnLine != GetCurrentShapeID() )
	{
		const int nCurrentShape = GetCurrentShapeID() - 1;
		const CSectionABC *psectFirst = m_psect->GetSectionAt( m_nFirstShapeOnLine );
		const CSectionABC *psectSecond = m_psect->GetSectionAt( nCurrentShape );
		const int nExtent =  psectSecond->right -  psectFirst->left;
		const int nDocWidth = GetCurrentWidth();
		const int nSpaceWidth = nDocWidth - nExtent;
		if( m_algCurrentPargraph == CHTMLParse::algCentre )
		{
			// richg - 19990225 - Never shift left!
			nHorizontalDelta = max( 0, nSpaceWidth / 2 );
		}
		else if( m_algCurrentPargraph == CHTMLParse::algRight )
		{
			// richg - 19990225 - Never shift left!
			nHorizontalDelta = max( 0, nSpaceWidth );
		}
	}

	//
	//	Minus one is a signal that the shapes are all the same size.
	m_nNextYPos = m_nYPos;
	const int nCurrentShapeID = GetCurrentShapeID();
	if( m_nFirstShapeOnLine != nCurrentShapeID )
	{
		for( int n = m_nFirstShapeOnLine; n < nCurrentShapeID; n++ )
		{
			CSectionABC *pSect = m_psect->GetSectionAt( n );
			const int nBaseline = GetBaseline( n );
			if( nBaseline >= 0 )
			{
				const int nBaselineDelta = m_nLowestBaseline - nBaseline;
				if( nBaselineDelta || nHorizontalDelta )
				{
					pSect->Offset( nHorizontalDelta, nBaselineDelta );
				}

				//
				//	We only use shapes that have a baseline
				if( pSect->bottom > m_nNextYPos )
				{
					m_nNextYPos = pSect->bottom;
				}
			}

			if( pSect->right > m_nWidth )
				m_nWidth = pSect->right;
		}
	}
}


void CHTMLSectionCreator::AddImage( int nWidth, int nHeight, int nBorder, CImage *pImage, CHTMLParse::Align alg, CHTMLSectionLink* pLink )
{
	ASSERT( pImage );

	CSize size( pImage->GetSize() );
	if( m_dc.IsPrinting() )
		size = m_dc.Scale( size );

	if( nWidth == 0 )
		nWidth = size.cx;
	else
		nWidth = m_dc.ScaleX( nWidth );

	if( nHeight == 0 )
		nHeight = size.cy;
	else
		nHeight = m_dc.ScaleY( nHeight );

	nWidth += m_dc.ScaleX(nBorder * 2);
	nHeight += m_dc.ScaleY(nBorder * 2);

	int nTop = m_nYPos;
	int nBaseline = nHeight;

	if( nWidth > m_nRightMargin - m_nXPos )
	{
		CarriageReturn();
		m_nYPos = m_nNextYPos;
		nTop = m_nYPos;
	}
	int nLeft = m_nXPos;

	switch( alg )
	{
	case CHTMLParse::algBottom:	break;

	case CHTMLParse::algCentre:
	case CHTMLParse::algMiddle:
		nBaseline = nHeight / 2;
		break;

	case CHTMLParse::algTop:
		nBaseline = m_dc.GetCurrentFontBaseline();
		break;

	case CHTMLParse::algLeft:
		{
			nLeft = m_nLeftMargin;
			MarginStackItem msi = { nLeft + nWidth + g_defaults.m_nImageMargin, m_nYPos + nHeight };
			m_stkLeftMargin.Push( msi );
			if( m_nXPos == m_nLeftMargin )
				m_nXPos = msi.nMargin;
			m_nLeftMargin = msi.nMargin;
			
		}
		break;

	case CHTMLParse::algRight:
		{
			nLeft = m_nRightMargin - nWidth - g_defaults.m_nImageMargin;
			MarginStackItem msi = { nLeft, m_nYPos + nHeight };
			m_stkRightMargin.Push( msi );
			m_nRightMargin = msi.nMargin;
		}
		break;
	}
	CHTMLImageSection *pImageSection = new CHTMLImageSection( m_psect, pImage, nBorder );
	pImageSection->Set( nLeft, nTop, nLeft + nWidth, nTop + nHeight );
	if( pLink )
		pImageSection->SetAsLink( pLink );

	if( alg != CHTMLParse::algLeft && alg != CHTMLParse::algRight )
	{
		m_nXPos += nWidth;
		AddBaseline( nBaseline );
	}
}


void CHTMLSectionCreator::AddHorizontalRule( CHTMLParse::Align alg, int nSize, int nWidth, bool bNoShade, COLORREF crColor, CHTMLSectionLink* pLink )
//
//	Horizontal ruler creates a space as tall as it is, also does a carriage return.
{
	const int nDisplayWidth = GetCurrentWidth();
	if( nWidth < 0 )
	{
		// If we are measuring, and looking atthe largest possible size,
		// return some small number, as it is irrelevant!
		if (m_bMeasuring && nDisplayWidth == knFindMaximumWidth)
			nWidth = 1;
		else
			nWidth = int( float( nDisplayWidth ) / 100 * abs(nWidth) );
	}

	int nLeft = m_nLeftMargin;
	if( !m_bMeasuring && nWidth != nDisplayWidth )
	{
		switch( alg )
		{
		case CHTMLParse::algRight:
			nLeft = m_nRightMargin - nWidth;
			break;

		case CHTMLParse::algCentre:
			nLeft = m_nLeftMargin + ( nDisplayWidth - nWidth ) / 2;
			break;
		}
	}

	// Size may need to be scaled, if we are not on a screen
	nSize = m_dc.ScaleY( nSize );

	CHTMLHorizontalRuleSection *psect = new CHTMLHorizontalRuleSection( m_psect, bNoShade, crColor );
	psect->Set( nLeft, m_nYPos, nLeft + nWidth, m_nYPos + nSize );
	if( pLink )
		psect->SetAsLink( pLink );
	
	CarriageReturn();
	m_nYPos = m_nNextYPos;
	m_nYPos += nSize;
}



void CHTMLSectionCreator::AddDocument( CHTMLDocument *pDocument )
{
	//pDocument->Dump();
	m_cCharSet = pDocument->m_cCharSet;

	//	Pointers used to process the anchor information
	CHTMLSectionLink* pLastLink = NULL;
	CHTMLAnchor* pLastAnchor = NULL;
	const UINT uDocumentSize = pDocument->m_arrItems.GetSize();
	for( UINT n = 0; n < uDocumentSize; n++ )
	{
		CHTMLDocumentObject *pItem = pDocument->m_arrItems[ n ];
		switch( pItem->GetType() )
		{
		case CHTMLDocumentObject::knParagraph:
			{
				CHTMLParagraph *pPara = static_cast<CHTMLParagraph *>( pItem );
				if( pPara->IsEmpty() )
					continue;

				NewParagraph( pPara->m_nSpaceAbove, pPara->m_nSpaceBelow, pPara->m_alg );

				const UINT uParagraphSize = pPara->m_arrItems.GetSize();
				for( UINT nItem = 0; nItem < uParagraphSize; nItem++ )
				{
					CHTMLParagraphObject *pParaObj = pPara->m_arrItems[ nItem ];

					// Update the Anchor status
					if (pParaObj->m_pAnchor == NULL || pParaObj->m_pAnchor != pLastAnchor)
						pLastLink = NULL;
		
					switch( pParaObj->GetType() )
					{
					case CHTMLParagraphObject::knText:
						{
							CHTMLTextBlock *pText = static_cast<CHTMLTextBlock *>( pParaObj );
							const HTMLFontDef *pdef = pDocument->GetFontDef( pText->m_uFontDefIndex );
							if( pText->m_bPreformatted )
							{
								AddTextPreformat( pText->m_strText, *pdef, pText->m_crFore , pLastLink );
							}
							else
							{
								AddText( pText->m_strText, *pdef, pText->m_crFore , pLastLink );
							}
						}
						break;

					case CHTMLParagraphObject::knTable:
						AddTable( static_cast<CHTMLTable *>( pParaObj ) );
						break;

					case CHTMLParagraphObject::knList:
						AddList( static_cast<CHTMLList *>( pParaObj ) );
						break;

					case CHTMLParagraphObject::knBlockQuote:
						AddBlockQuote( static_cast<CHTMLBlockQuote *>( pParaObj ) );
						break;


					case CHTMLParagraphObject::knImage:
						{
							CHTMLImage *pImage = static_cast<CHTMLImage *>( pParaObj );
							AddImage( pImage->m_nWidth
									, pImage->m_nHeight
									,	pImage->m_nBorder
									, pImage->m_pImage
									, pImage->m_alg
									, pLastLink
									);
						}
						break;


					case CHTMLParagraphObject::knHorizontalRule:
						{
							CHTMLHorizontalRule *pHR = static_cast<CHTMLHorizontalRule *>( pParaObj );
							AddHorizontalRule( pHR->m_alg
											, pHR->m_nSize
											, pHR->m_nWidth
											, pHR->m_bNoShade
											, pHR->m_crColor
											, pLastLink
											);
						}
						break;

					
					case CHTMLParagraphObject::knAnchor:
						{
							CHTMLAnchor *pAnchor = static_cast<CHTMLAnchor *>( pParaObj );
							// Add an entry to the parent section
							if (pAnchor->m_strLinkTarget.GetLength())
							{
								pLastLink = m_psect->AddLink(pAnchor->m_strLinkTarget, pAnchor->m_strLinkTitle, pAnchor->m_crLink, pAnchor->m_crHover );
								pLastAnchor = pAnchor;
							}
							else
							{
								pLastLink = NULL;
								pLastAnchor = NULL;
							}


							// Handle named section...
							if (pAnchor->m_strLinkName.GetLength())
							{
								m_psect->AddNamedSection(pAnchor->m_strLinkName, CPoint(m_nXPos, m_nYPos));
							}
						}
						break;
					}
				}
			}
			break;
		}
	}
	Finished();
}


void CHTMLSectionCreator::AddTable(CHTMLTable *ptab )
{
	// In order to ensure that the borders and backgrounds will be drawn
	// properly, we will need either sections to display each of those properties
	// and ensure that they are displayed before the cell's contents. This way
	// we do not need another layer of indirection (such as 
	// CHTMLTableSection->CHTMLCellSection->CHTMLTextSection)
	//

	const CSize sizeTable( ptab->GetRowsCols() );
	if( sizeTable.cx && sizeTable.cy )
	{
		if( ptab->m_alg == CHTMLParse::algCentre )
		{
			NewParagraph( 1, 1, m_algCurrentPargraph );
		}

		int nMaxWidth = GetCurrentWidth();

		CHTMLTableLayout layout(ptab, m_dc, m_nDefaultRightMargin - m_nDefaultLeftMargin, m_nZoomLevel );
		int nTableWidth = layout.GetTableWidth();
		
		//
		//	If the table is too wide creep the image/table margins until the table fits or there are no more margins.
		if( nTableWidth  + g_defaults.m_nAlignedTableMargin < nMaxWidth )
		{
			while( m_nDefaultRightMargin != m_nRightMargin && m_nDefaultLeftMargin != m_nRightMargin && nTableWidth + g_defaults.m_nAlignedTableMargin >= GetCurrentWidth() )
			{
				NewParagraph( 1, 1, m_algCurrentPargraph );
			}

			nMaxWidth = GetCurrentWidth();
		}
		
		int m_nKeepYPos = m_nYPos;
		
		//
		//  Note that the default alignment is algTop, which is 
		//  meaningless for tables, but provides the appropriate non-wrapping
		//  behavior
		int nLeftMargin = m_nXPos;
		if( nTableWidth < nMaxWidth )
		{
			switch( ptab->m_alg )
			{
			case CHTMLParse::algRight:
				nLeftMargin = m_nRightMargin - nTableWidth;
				break;

			case CHTMLParse::algCentre:
				nLeftMargin += ( nMaxWidth - nTableWidth ) / 2;
				break;
			}
		}


		// If there is a border or background color, we need a CHTMLTableSection to display them.
		// We could create it without checking, but its wasteful.
		CHTMLTableSection* pTableSection = 0;
		if (ptab->m_nBorder || !ptab->m_bTransparent)
		{
			pTableSection = new CHTMLTableSection( m_psect, ptab->m_nBorder, ptab->m_crBorderDark, ptab->m_crBorderLight, ptab->m_bTransparent, ptab->m_crBgColor);
			// This will need to have it's dimensions set, specifically the botttom!
			pTableSection->Set( nLeftMargin, m_nYPos, nLeftMargin + nTableWidth, m_nYPos );
			// nShapeId = GetCurrentShapeID() - 1;
		}

		ArrayClass<CHTMLTableSection*> arrCellSection( sizeTable.cy );

		//		iterate over the columns again, this time lay them out by calling this function and remember the
		//			tallest cell.
		//		Move onto the next row by bumping up the creator Y position.
		m_nYPos += ptab->m_nBorder;
		const UINT uTableRows = ptab->m_arrRows.GetSize();
		for(UINT nRow = 0; nRow < uTableRows; nRow++ )
		{
			// Skip the border of the table, if there is one.
			m_nXPos = nLeftMargin + m_dc.ScaleX(ptab->m_nBorder);
			m_nYPos += m_dc.ScaleY(ptab->m_nCellSpacing);
			CHTMLTable::CHTMLTableRow *pRow = ptab->m_arrRows[ nRow ];
			int nTallestY = m_nYPos;


			ArrayOfInt arrStartShape;
			ArrayOfInt arrLowest;
			ArrayOfInt arrEndShape;

			const UINT uRowColumns = pRow->m_arrCells.GetSize();
			for( UINT nCol = 0; nCol < uRowColumns; nCol++ )
			{
				// Here we include code to account for borders, spacing, and padding
				const int nOuterLeft = m_nXPos + m_dc.ScaleX(ptab->m_nCellSpacing);
				const int nInnerLeft = nOuterLeft + m_dc.ScaleX(ptab->m_nCellPadding + (ptab->m_nBorder ? 1 : 0));
				const int nInnerRight = nInnerLeft + layout.GetColumnWidth(nCol); 
				const int nOuterRight = nInnerRight + m_dc.ScaleX(ptab->m_nCellPadding + (ptab->m_nBorder ? 1 : 0));
				const int nOuterTop = m_nYPos;
				const int nInnerTop = nOuterTop + m_dc.ScaleY(ptab->m_nCellPadding + (ptab->m_nBorder ? 1 : 0));

				m_nXPos = nOuterRight;
				CHTMLTableCell* pcell = pRow->m_arrCells[nCol];
				// Create a TableSection if there is a border or background color
				if (ptab->m_nBorder || !pcell->m_bTransparent)
				{
					arrCellSection[nCol] = new CHTMLTableSection( m_psect, ptab->m_nBorder ? 1 : 0,  pcell->m_crBorderLight, pcell->m_crBorderDark, pcell->m_bTransparent, pcell->m_crBgColor);
					arrCellSection[nCol]->Set( nOuterLeft, nOuterTop, nOuterRight, nOuterTop);
				}
				else
					arrCellSection[nCol] = NULL;

				arrStartShape.Add( GetCurrentShapeID() );

				CHTMLSectionCreator htCreate( m_psect, m_dc, nInnerTop, nInnerLeft, nInnerRight, m_crBack, false, m_nZoomLevel );
				htCreate.AddDocument( pcell );

				const CSize size( htCreate.GetSize() );
				if( size.cy > nTallestY )
				{	
					nTallestY = size.cy;
				}
				if( pcell->m_nHeight + m_nYPos > nTallestY )
				{	
					nTallestY = pcell->m_nHeight + m_nYPos;
				}
				arrLowest.Add( nTallestY );
				arrEndShape.Add( GetCurrentShapeID() );
			}

			m_nYPos = nTallestY + ptab->m_nCellPadding + (ptab->m_nBorder ? 1 : 0);

			//
			//	Adjust all of the cells to have the same bottom.
			//	Also, moves all of the contained shapes if the vertical alignment dictates
			for (UINT ii = 0; ii < uRowColumns; ++ii)
			{
				if (arrCellSection[ii])
					arrCellSection[ii]->bottom = m_nYPos;

				const CHTMLTableCell* pcell = pRow->m_arrCells[ ii ];
				int nOffset = 0;
				switch( pcell->m_valg )
				{
				case CHTMLParse::algTop:
					//	Leave them as they are
					break;

				case CHTMLParse::algMiddle:
					nOffset = ( nTallestY - arrLowest[ ii ] ) / 2;
					break;

				case CHTMLParse::algBottom:
					nOffset = nTallestY - arrLowest[ ii ];
					break;
				}

				if( nOffset )
				{
					for( int n = arrStartShape[ ii ]; n < arrEndShape[ ii ]; n++ )
					{
						m_psect->GetSectionAt( n )->Offset( 0, nOffset );
					}
				}
			}
			arrStartShape.RemoveAll();
			arrLowest.RemoveAll();
			arrEndShape.RemoveAll();

			m_nYPos++;

			// Good place for a page break, between rows.
			if( m_dc.IsPrinting() && m_nLeftMargin == m_nDefaultLeftMargin && m_nRightMargin == m_nDefaultRightMargin)
			{
				m_psect->AddBreakSection(GetCurrentShapeID());
			}
		}
		// Complete the table section!
		m_nYPos += ptab->m_nCellSpacing;
		if( pTableSection )
		{
			pTableSection->bottom = m_nYPos;
		}

		//	Insert a paragraph if were not floating
		if (ptab->m_alg != CHTMLParse::algLeft && ptab->m_alg != CHTMLParse::algRight)
		{
			if( m_dc.IsPrinting() && m_nLeftMargin == m_nDefaultLeftMargin && m_nRightMargin == m_nDefaultRightMargin )
			{
				m_psect->AddBreakSection(GetCurrentShapeID());
			}
		}
		m_nYPos++;

		if( nTableWidth < nMaxWidth )
		{
			//
			//	For tables that are right or left aligned we need to alter the margin stack.
			//	For 'normal' tables we do nothing.
			switch( ptab->m_alg )
			{
			case CHTMLParse::algRight:
				{
					MarginStackItem msi = { nLeftMargin - g_defaults.m_nAlignedTableMargin, m_nYPos - 1 };
					m_stkRightMargin.Push( msi );
					if( m_nXPos == nLeftMargin )
						m_nXPos = msi.nMargin;
					m_nRightMargin = msi.nMargin;
					m_nYPos = m_nKeepYPos;
				}
				break;

			case CHTMLParse::algLeft:
				{
					MarginStackItem msi = { nLeftMargin + nTableWidth + g_defaults.m_nAlignedTableMargin, m_nYPos - 1 };
					m_stkLeftMargin.Push( msi );
					m_nLeftMargin = msi.nMargin;
					m_nYPos = m_nKeepYPos;
				}
				break;
			}

		}
	}
}

void CHTMLSectionCreator::AddList(CHTMLList *pList )
{
	/*
		A List is similar to a table, in that each list item is a
		document in itself. Lists are laid out by creating the
		bullet, which is right justified along the left margin, 
		and has width knIndentSize. The contents depend on whether
		the list is ordered, and the index of the item.
		The list item contents use the right edge if the bullet space
		as the left margin. That's it! Note that current font properties
		apply to the bullet.
		Only color applies to bullets in unordered lists.
	*/
	static LPCTSTR knBulletText = _T("\xB7");
	static LPCTSTR knBulletFormat = _T("%u.");

	const int nBulletFontSize = GetFontSizeAsPixels( m_dc.GetSafeHdc(), 2, m_nZoomLevel );
	const int nBulletWidth = MulDiv(g_defaults.m_nIndentSize, GetDeviceCaps( m_dc.GetSafeHdc(), LOGPIXELSX), 1000); 
	const int nBulletSpace = MulDiv(g_defaults.m_nIndentSpaceSize, GetDeviceCaps( m_dc.GetSafeHdc(), LOGPIXELSX), 1000); 
	int nIndex = 1;	// Index, used for ordered lists

	// Iterate the list items, creating the bullet and the subdocument.
	const UINT nItems = pList->m_arrItems.GetSize();

	m_nNextYPos += m_dc.GetCurrentFontHeight();

	for (UINT i = 0; i < nItems; ++i)
	{
		int nBulletHeight = 0;
		CHTMLListItem* pItem = pList->m_arrItems[i];

		NewParagraph(0,0,CHTMLParse::algLeft);
		// Do we need to create a bullet?
		if (pItem->m_bBullet)
		{
			// Create the bullet...
			LPCTSTR pcszFont;
			int nFontSize;
			bool bBold, bItalic, bUnderline, bStrikeOut;
			LPCTSTR pcszText = 0;			
			TCHAR szBuffer[12];
			UINT uLength = 1;

			if (pList->m_bOrdered)
			{
				uLength = wsprintf(szBuffer, knBulletFormat, nIndex);
				// Set text parameters
				pcszText = szBuffer;
				pcszFont = pItem->m_strFont;
				nFontSize = GetFontSizeAsPixels( m_dc.GetSafeHdc(), pItem->m_nSize, m_nZoomLevel );
				bBold = pItem->m_bBold;
				bItalic = pItem->m_bItalic;
				bUnderline = pItem->m_bUnderline;
				bStrikeOut = pItem->m_bStrikeout;
			}
			else
			{
				// Set text parameters
				pcszText = knBulletText;
				pcszFont = _T("Symbol");
				nFontSize = nBulletFontSize;
				bBold = false;
				bItalic = false;
				bUnderline = false;
				bStrikeOut = false;
			}

			FontDef fdef( pcszFont, nFontSize, bBold, bItalic, bUnderline, bStrikeOut, m_cCharSet );

			CHTMLTextSection* pText = new CHTMLTextSection( m_psect, pcszText, uLength, fdef, pItem->m_crFore );

			//
			// Position the object...
			m_dc.SelectFont( fdef );

			const int nTextWidth = m_dc.GetTextExtent(pcszText, uLength );
			nBulletHeight = m_nYPos + m_dc.GetCurrentFontHeight();
			const int right = m_nXPos + nBulletWidth;
			const int left = right - nTextWidth;
			pText->Set( left, m_nYPos, right, nBulletHeight );
			// Increment the index
			nIndex++;
		}
		// Now, create the sub-document
		CHTMLSectionCreator htCreate( m_psect, m_dc, m_nYPos, m_nXPos + nBulletWidth + nBulletSpace, m_nRightMargin, m_crBack, false, m_nZoomLevel );
		htCreate.AddDocument( pItem );
		const CSize size( htCreate.GetSize() );
		// Adjust y-pos
		m_nYPos = max(size.cy, nBulletHeight) + 1;
	}
	m_nYPos += m_dc.GetCurrentFontHeight();
}

void CHTMLSectionCreator::AddBlockQuote(CHTMLBlockQuote *pBQ )
{
	/*
		A Blockquote contains a single element, a document. Just lay it out into
		smaller margins.
	*/
	const int nIndentWidth = MulDiv(g_defaults.m_nIndentSize + g_defaults.m_nIndentSpaceSize, GetDeviceCaps( m_dc.GetSafeHdc(), LOGPIXELSX), 1000); 

	NewParagraph(1,1,CHTMLParse::algLeft);
	/* 
		Since there is the possibility that the margins exceed the page width,
		force the quote to be at least as wide as the margins!
	*/
	int nLeft = m_nXPos + nIndentWidth;
	int nRight = m_nRightMargin - nIndentWidth;
	if (nRight < nLeft)
		nRight = nLeft + nIndentWidth;

	CHTMLSectionCreator htCreate( m_psect, m_dc, m_nYPos, nLeft, nRight, m_crBack, false, m_nZoomLevel );
	htCreate.AddDocument( pBQ->m_pDoc );
	const CSize size( htCreate.GetSize() );
	m_nYPos = size.cy + (( m_dc.GetCurrentFontHeight() ) / 2);
	NewParagraph(0, 0, m_algCurrentPargraph);
}
