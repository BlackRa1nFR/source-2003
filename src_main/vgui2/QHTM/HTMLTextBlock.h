/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLTextBlock.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef HTMLTEXTBLOCK_H
#define HTMLTEXTBLOCK_H

class CHTMLTextBlock : public CHTMLParagraphObject			//txt
//
//	Text block.
//	Has some text.
{
public:
	CHTMLTextBlock( const CTextABC &strText, UINT uFontDefIndex, COLORREF crFore, bool bPreformatted );

#ifdef _DEBUG
	virtual void Dump() const;
#endif	//	_DEBUG

	StringClass m_strText;
	UINT m_uFontDefIndex;
	COLORREF m_crFore;
	bool m_bPreformatted;

private:
	CHTMLTextBlock();
	CHTMLTextBlock( const CHTMLTextBlock &);
	CHTMLTextBlock& operator =( const CHTMLTextBlock &);
};

#endif //HTMLTEXTBLOCK_H