/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLSection.h
Owner:	russf@gipsysoft.com
Purpose:	Main container sectionfor the basic HTML control.
----------------------------------------------------------------------*/
#ifndef HTMLSECTION_H
#define HTMLSECTION_H

#ifndef SCROLLCONTAINER_H
	#include "ScrollContainer.h"
#endif	//	SCROLLCONTAINER_H

#ifndef HTMLSECTIONLINK_H
	#include "HTMLSectionLink.h"
#endif // HTMLSECTIONLINK_H

#ifndef SIMPLESTRING_H
	#include "SimpleString.h"
#endif	//	SIMPLESTRING_H

class CDefaults;

class CHTMLSection : public CScrollContainer
{
public:
	explicit CHTMLSection( CParentSection *psectParent, CDefaults *pDefaults );
	virtual ~CHTMLSection();

	virtual void OnLayout( const CRect &rc, CDrawContext &dc );
	virtual void OnDraw( CDrawContext &dc );
	virtual bool OnNotify( const CSectionABC *pChild, const int nEvent );

	//	Set the HTML content for this section
	void SetHTML( LPCTSTR pcszHTMLText, UINT nLength, LPCTSTR pcszPathToFile );
	bool SetHTML( HINSTANCE hInst, LPCTSTR pcszName );
	bool SetHTMLFile( LPCTSTR pcszFilename );

	//	Set the default margins for the control.
	void SetDefaultMargins( LPCRECT lprect );
	void GetDefaultMargins( LPRECT lprect ) const;

	//	Load the HTML from a resource.
	void LoadFromResource( UINT uID );

	//	Accessor for the title
	const StringClass & GetTitle() const;

	//	Goto a named link.
	void GotoLink( LPCTSTR pcszLink );

	inline const CSize &GetSize() const { return sizeHTML; }
	inline int GetHeight() const { return sizeHTML.cy; }
	inline int GetWidth() { return sizeHTML.cx; }

	//	Enable/disable tooltips
	void EnableTooltips( bool bEnable );
	bool IsTooltipsEnabled() const;

	//	Pagination Methods
	int Paginate(const CRect& rcPage);	// Returns number of pages
	CRect GetPageRect(UINT nPage) const;

	//
	//	Kludge for preasured tables
	void ResetMeasuringKludge();

	//	Set the zoom level used.
	void SetZoomLevel( UINT uZoomLevel );
	//	Get the zoom level used.
	inline int GetZoomLevel() const { return m_uZoomLevel; }

private:
	//
	//	Get the background for us
	virtual bool GetBackgroundColours( HDC , HBRUSH & ) const { return false;}

	virtual void OnExecuteHyperlink( LPCTSTR pcszLink );

	class CHTMLDocument *m_pDocument;
	
	//	Destroy the document and all child sections
	void DestroyDocument();
	//	Just remove the child sections
	void RemoveAllChildSections();

	//	Layout a document to the display given a width, returns the document height
	CSize LayoutDocument( CDrawContext &dc, CHTMLDocument *m_pDocument, int nYPos, int nLeft, int nRight );

	CSize sizeHTML;
	COLORREF m_crDefaultBackground, m_crDefaultForeground;

	//	Store a list of links in this Section. Each link will have a list
	//	of sections it contains.

	ArrayClass<CHTMLSectionLink*> m_arrLinks;

	CHTMLSectionLink* AddLink(LPCTSTR pcszLinkTarget, LPCTSTR pcszLinkTitle, COLORREF crLink, COLORREF crHover);


	//	Create a map of names in this document. The position saved is 
	//	The current position.
	MapClass<StringClass, CPoint> m_mapNames;
	void AddNamedSection(LPCTSTR name, const CPoint& point);

	//	Create a list of sections (by index) before which a page break may occur
	ArrayClass< int > m_arrBreakSections;
	void AddBreakSection( int i);

	//	Pagination data
	ArrayClass< CRect > m_arrPageRects;

	UINT m_uZoomLevel;

	StringClass m_strTitle;

	CDefaults *m_pDefaults;

private:
	CHTMLSection();
	CHTMLSection( const CHTMLSection &);
	CHTMLSection& operator =( const CHTMLSection &);

	friend class CHTMLSectionCreator;
};


#endif //HTMLSECTION_H