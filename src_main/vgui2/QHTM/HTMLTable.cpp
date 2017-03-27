/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLTable.cpp
Owner:	russf@gipsysoft.com
Purpose:	A table
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"

CHTMLTable::CHTMLTable(	int nWidth, int nBorder, CHTMLParse::Align alg, CHTMLParse::Align	valg, int nCellSpacing, int nCellPadding,  bool bTransparent, COLORREF crBgColor, COLORREF crDark, COLORREF crLight)
	: CHTMLParagraphObject( CHTMLParagraphObject::knTable )
	, m_nWidth( nWidth )
	, m_nBorder( nBorder )
	, m_alg( alg )
	, m_valg( valg )
	, m_pCurrentRow( NULL )
	, m_nCellSpacing( nCellSpacing )
	, m_nCellPadding( nCellPadding )
	, m_bTransparent( bTransparent )
	, m_crBgColor( crBgColor )
	, m_crBorderLight( crLight )
	, m_crBorderDark( crDark )
	, m_bCellsMeasured( false )
{

}


CHTMLTable::~CHTMLTable()
{
	for( UINT nRow = 0; nRow < m_arrRows.GetSize(); nRow++ )
	{
		delete m_arrRows[ nRow ];
	}
	m_arrRows.RemoveAll();
}

void CHTMLTable::AddCell( CHTMLTableCell *pCell )
{
	ASSERT( m_pCurrentRow );
	m_pCurrentRow->m_arrCells.Add( pCell );
}


void CHTMLTable::NewRow( CHTMLParse::Align valg )
{
	m_pCurrentRow = new CHTMLTableRow( valg );
	m_arrRows.Add( m_pCurrentRow );
}


CSize CHTMLTable::GetRowsCols() const
//
//	Get the dimensions of the table in rows and columns
{
	CSize size( m_arrRows.GetSize(), 0 );
	for( UINT nRow = 0; nRow < m_arrRows.GetSize(); nRow++ )
	{
		if( m_arrRows[ nRow ]->m_arrCells.GetSize() > (UINT)size.cy )
		{
			size.cy = m_arrRows[ nRow ]->m_arrCells.GetSize();
		}
	}
	//	Must have a 
	ASSERT( size.cx && size.cy );
	return size;
}

#ifdef _DEBUG
void CHTMLTable::Dump() const
{
	CSize size( GetRowsCols() );
	TRACENL( _T("Table----------------\n"));
	TRACENL( _T(" Size %d rows %d cols\n"), size.cx, size.cy );
	TRACENL( _T(" Width( %d )\n"), m_nWidth );
	TRACENL( _T(" Border( %d )\n"), m_nBorder );	
	TRACENL( _T(" Alignment (%s)\n"), GetStringFromAlignment( m_alg ) );
	TRACENL( _T(" CellSpacing (%d)\n"), m_nCellSpacing );
	TRACENL( _T(" CellPadding (%d)\n"), m_nCellPadding );
	TRACENL( _T(" Transparent (%s)\n"), (m_bTransparent ? _T("true") : _T("false") ));
	TRACENL( _T(" Colors Not Shown.\n") );

	for( UINT nRow = 0; nRow < m_arrRows.GetSize(); nRow++ )
	{
		TRACENL( _T(" Row %d\n"), nRow );
		m_arrRows[ nRow ]->Dump();
	}

}
#endif	//	_DEBUG


CHTMLTable::CHTMLTableRow::CHTMLTableRow( CHTMLParse::Align	valg )
	: m_valg( valg )
{
}


CHTMLTable::CHTMLTableRow::~CHTMLTableRow()
{
	for( UINT nCol = 0; nCol < m_arrCells.GetSize(); nCol++ )
	{
		delete m_arrCells[ nCol ];
	}
	m_arrCells.RemoveAll();
}

#ifdef _DEBUG
void CHTMLTable::CHTMLTableRow::Dump() const
{
	for( UINT nCol = 0; nCol < m_arrCells.GetSize(); nCol++ )
	{
		TRACENL( _T(" Col %d\n"), nCol );
		m_arrCells[ nCol ]->Dump();
	}
}
#endif	//	_DEBUG


void CHTMLTable::ResetMeasuringKludge()
{
	m_bCellsMeasured = false;
	for( UINT nRow = 0; nRow < m_arrRows.GetSize(); nRow++ )
	{
		for( UINT nCol = 0; nCol < m_arrRows[ nRow ]->m_arrCells.GetSize(); nCol++ )
		{
			m_arrRows[ nRow ]->m_arrCells[ nCol ]->ResetMeasuringKludge();
		}
	}
}