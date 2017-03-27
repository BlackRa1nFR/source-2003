/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLSectionLink.h
Owner:	russf@gipsysoft.com
Author: rich@woodbridgeinternalmed.com
Purpose:	Hyperlink 'link' object, links all of teh hyperlink sections
					together. Ensure they all highlight at the same time if the
					hyperlink is made up of multiple sections etc.
----------------------------------------------------------------------*/
#ifndef HTMLSECTIONLINK_H
#define HTMLSECTIONLINK_H

#ifndef SECTIONABC_H
	#include "sectionABC.h"
#endif	// SECTIONABC_H


//	This struct is used to store link information for a section
class CHTMLSectionLink
{
public:
	CHTMLSectionLink(LPCTSTR pcszLinkTarget, LPCTSTR pcszLinkTitle, COLORREF crLink, COLORREF crHover)
		: m_strLinkTarget( pcszLinkTarget )
		, m_strLinkTitle( pcszLinkTitle )
		, m_crLink( crLink )
		, m_crHover( crHover )
	{}

	enum{ knEventGotoURL = 65000 };

	StringClass m_strLinkTarget;
	StringClass m_strLinkTitle;
	COLORREF	m_crLink;
	COLORREF	m_crHover;
	ArrayClass<CSectionABC*> m_arrSections;

	void OnMouseEnter();
	void OnMouseLeave();
	void AddSection(CSectionABC* psect) { m_arrSections.Add( psect ); }
};


#endif
