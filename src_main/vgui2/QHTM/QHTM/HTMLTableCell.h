/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLTableCell.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef HTMLTABLECELL_H
#define HTMLTABLECELL_H

class CHTMLTableCell : public CHTMLDocument			//	cell
//
//	Table cell.
//	Is a document.
{
public:
	CHTMLTableCell( CDefaults *pDefaults, int nWidth, int nHeight, bool bNoWrap, bool bTransparent, COLORREF crBgColor, COLORREF crDark, COLORREF crLight, CHTMLParse::Align valg );

	int m_nWidth;
	int m_nHeight;
	bool m_bNoWrap;
	COLORREF m_crBorderLight;
	COLORREF m_crBorderDark;
	COLORREF m_crBgColor;
	bool m_bTransparent;
	CHTMLParse::Align	m_valg;
	
#ifdef _DEBUG
	virtual void Dump() const;
#endif	//	_DEBUG
	
private:
	CHTMLTableCell( const CHTMLTableCell &);
	CHTMLTableCell& operator =( const CHTMLTableCell &);
};

#endif //HTMLTABLECELL_H