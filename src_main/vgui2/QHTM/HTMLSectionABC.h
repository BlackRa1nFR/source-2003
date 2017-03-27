/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLSectionABC.h
Owner:	russf@gipsysoft.com
Purpose:	Base class used for the HTML display sections.
----------------------------------------------------------------------*/
#ifndef HTMLSECTIONABC_H
#define HTMLSECTIONABC_H

#ifndef SECTIONABC_H
	#include "SectionABC.h"
#endif	//	SECTIONABC_H

//
//	Uncommenting this will force all HTML components to draw a red
//	rectangle around themselves.
//	This helps a lot when debugging drawing problems.
//#define DRAW_DEBUG

class CHTMLSectionLink;

class CHTMLSectionABC : public CSectionABC
{
public:
	CHTMLSectionABC( CParentSection *psect );
	virtual ~CHTMLSectionABC();

	virtual StringClass GetTipText() const;
	virtual void OnMouseLeftUp( const CPoint &pt );
	virtual void OnMouseEnter();
	virtual void OnMouseLeave();

	//	Set this section as a link, either a link target or a name
	void SetAsLink(CHTMLSectionLink*);

	//	Get the target of this link, that is the URL where to go.
	LPCTSTR GetLinkTarget() const;

	//	Switch on/off the ability for the HTML to display tooltips.
	static void EnableTooltips( bool bEnable );
	static bool IsTooltipsEnabled();

	COLORREF LinkColour(); 
	COLORREF LinkHoverColour();
#ifdef DRAW_DEBUG
	virtual void OnDraw( CDrawContext &dc );
#endif	//	 DRAW_DEBUG
protected:
	bool inline IsLink() const { return m_pHtmlLink != NULL; }

private:
	CHTMLSectionLink*	m_pHtmlLink;

private:
	CHTMLSectionABC();
	CHTMLSectionABC( const CHTMLSectionABC & );
	CHTMLSectionABC &operator =( const CHTMLSectionABC & );

	friend class CHTMLSectionCreator;
};

#endif //HTMLSECTIONABC_H
