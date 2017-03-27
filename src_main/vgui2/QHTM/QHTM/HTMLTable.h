/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLTable.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef HTMLTABLE_H
#define HTMLTABLE_H

class CHTMLTableCell;

class CHTMLTable : public CHTMLParagraphObject			//	tab
//
//	Table.
//	Can have many rows, each row can have many columns
{
public:
	CHTMLTable(	int nWidth, int nBorder, CHTMLParse::Align alg, CHTMLParse::Align valg, int nCellSpacing, int nCellPadding,  bool bTransparent, COLORREF crBgColor, COLORREF crDark, COLORREF crLight);

	~CHTMLTable();

	//	Add a cell to the current row.
	void AddCell( CHTMLTableCell *pCell );

	//	Create a new row.
	void NewRow( CHTMLParse::Align valg );

	//	Get the dimensions of the table in rows and columns
	CSize GetRowsCols() const;

	//	Hack alert - to workaround problem with data stored in tables which should not be (ideally).
	void ResetMeasuringKludge();

	class CHTMLTableRow				//	row
	{
	public:
		CHTMLTableRow( CHTMLParse::Align valg );
		~CHTMLTableRow();
#ifdef _DEBUG
		void Dump() const;
#endif	//	_DEBUG
		ArrayClass< CHTMLTableCell* > m_arrCells;
		CHTMLParse::Align	m_valg;
	};
	ArrayClass< CHTMLTableRow * > m_arrRows;

	const CHTMLTableRow	*GetCurrentRow() { return m_pCurrentRow; }

	int m_nWidth;
	int m_nBorder;
	CHTMLParse::Align	m_alg;
	CHTMLParse::Align	m_valg;
	COLORREF m_crBorderLight;
	COLORREF m_crBorderDark;
	COLORREF m_crBgColor;
	int	m_nCellSpacing;
	int m_nCellPadding;
	bool m_bTransparent;

	//  Data used to layout the table and it's contents. It is referenced
	//  primarily by CHTMLSectionCreator and CHTMLTableLayout.
	bool m_bCellsMeasured;	// Have the cells been measured?
	ArrayOfInt m_arrDesiredWidth;	// Desired width for column
	ArrayOfInt m_arrMinimumWidth;	// Minimum width for column
	ArrayOfInt m_arrMaximumWidth;	// Maximum width for column
	ArrayOfBool m_arrNoWrap;			// NoWrap status for column
	ArrayOfInt m_arrLayoutWidth;	// Current layout width for column

#ifdef _DEBUG
	virtual void Dump() const;
#endif	//	_DEBUG

private:
	CHTMLTableRow	*m_pCurrentRow;

private:
	CHTMLTable( const CHTMLTable &);
	CHTMLTable& operator =( const CHTMLTable &);
};


#endif //HTMLTABLE_H