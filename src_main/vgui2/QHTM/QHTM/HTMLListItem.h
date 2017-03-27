/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLListItem.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef HTMLLISTITEM_H
#define HTMLLISTITEM_H

class CHTMLListItem : public CHTMLDocument
//
//	List Item
//	Is a document.
{
public:
	CHTMLListItem( CDefaults *pDefaults, bool bBullet, LPCTSTR pcszFont, int nSize, bool bBold, bool bItalic, bool bUnderline, bool bStrikeout, COLORREF crFore );

	bool IsEmpty() const;
	
	StringClass m_strFont;
	int m_nSize;
	bool m_bBold;
	bool m_bItalic;
	bool m_bUnderline;
	bool m_bStrikeout;
	COLORREF m_crFore;

	bool m_bBullet;
	
#ifdef _DEBUG
	virtual void Dump() const;
#endif	//	_DEBUG
	
private:
	CHTMLListItem( const CHTMLListItem &);
	CHTMLListItem& operator =( const CHTMLListItem &);
};

#endif //HTMLLISTITEM_H