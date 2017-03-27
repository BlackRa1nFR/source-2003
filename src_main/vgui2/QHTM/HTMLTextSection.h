/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLTextSection.h
Owner:	russf@gipsysoft.com
Purpose:	Simple drawn text object.
----------------------------------------------------------------------*/
#ifndef HTMLTEXTSECTION_H
#define HTMLTEXTSECTION_H

#ifndef HTMLSECTIONABC_H
	#include "HTMLSectionABC.h"
#endif	//	HTMLSECTIONABC_H


class CHTMLTextSection : public CHTMLSectionABC
{
public:
	CHTMLTextSection( CParentSection *psect, LPCTSTR pcszText, int nLength, FontDef &fdef, COLORREF crFore );
	virtual ~CHTMLTextSection();

	virtual void OnDraw( CDrawContext &dc );
private:
	StringClass m_str;
	FontDef m_fdef;
	COLORREF m_crFore;

private:
	CHTMLTextSection();
	CHTMLTextSection( const CHTMLTextSection & );
	CHTMLTextSection& operator = ( const CHTMLTextSection & );
};

#endif //HTMLTEXTSECTION_H