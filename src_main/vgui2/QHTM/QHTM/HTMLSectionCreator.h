/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLSectionCreator.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef HTMLSECTIONCREATOR_H
#define HTMLSECTIONCREATOR_H


#ifndef STACK_H
	#include "Stack.h"
#endif	//	STACK_H

#ifndef HTMLPARSE_H
	#include "htmlparse.h"
#endif	//	HTMLPARSE_H

#ifndef FONTDEF_H
	#include "FontDef.h"
#endif	//	FONTDEF_H


// Constants used to indicate special conditions to CHTMLSectionCreator
// Valid only when m_bMeasuring is true
const int knFindMaximumWidth = 32767;

class CImage;
class CHTMLSection;
class CHTMLSectionLink;
class CDrawContext;

class CHTMLSectionCreator
{
public:
	CHTMLSectionCreator( CHTMLSection *psect, CDrawContext &dc, int nTop, int nLeft, int nRight, COLORREF crBack, bool bMeasuring, int nZoomLevel );
	virtual ~CHTMLSectionCreator();

	//	Get the height of the sections and therefore the height
	CSize GetSize() const { return CSize( m_nWidth, m_nYPos ); }

	//	Add a new paragraph
	void NewParagraph( int nSpaceAbove, int nSpaceBelow, CHTMLParse::Align alg );

	//	Add a text object
	void AddText( const StringClass &str, const HTMLFontDef &htmlfdef, COLORREF crFore, CHTMLSectionLink* pLink );
	void AddTextPreformat( const StringClass &str, const HTMLFontDef &htmlfdef, COLORREF crFore, CHTMLSectionLink* pLink );

	//	Add an image object
	void AddImage( int nWidth, int nHeight, int nBorder, CImage *pImage, CHTMLParse::Align alg, CHTMLSectionLink* pLink);

	//	Add a horizontal rule
	void AddHorizontalRule( CHTMLParse::Align alg, int nSize, int nWidth, bool bNoShade, COLORREF crColor, CHTMLSectionLink* pLink);

	//	Add a table.
	void AddTable( CHTMLTable *ptab );

	//	Add a list
	void AddList( CHTMLList *pList );

	//	Add a BlockQuote
	void AddBlockQuote( CHTMLBlockQuote *pList );

	//	Add a document
	void AddDocument( CHTMLDocument *pDocument );

private:
	void Finished();
	void CarriageReturn();
	void AdjustShapeBaselinesAndHorizontalAlignment();
	inline void AddBaseline( int nBaseline );
	inline int GetBaseline( int nShape ) const;
	inline int GetCurrentShapeID() const;

	//	Get the display width 
	inline int GetCurrentWidth() const { return m_nRightMargin - m_nLeftMargin; }

	CHTMLSection *m_psect;
	CDrawContext &m_dc;

	//	The left and right margins passed into this object, what we default to.
	const int m_nDefaultLeftMargin, m_nDefaultRightMargin;
	const int m_nTop;
	COLORREF m_crBack;

	//	Our current margins
	int m_nLeftMargin, m_nRightMargin;
	
	//	The current position when drawing our objects.
	int m_nYPos, m_nXPos;

	//	This flag is set in the ctor when we are determining the extents
	//	of a document. It prevents the re-alignment of objects based
	//	on artificial margins.
	bool m_bMeasuring;

	int m_nPreviousParaSpaceBelow;
	CHTMLParse::Align m_algCurrentPargraph;

	struct MarginStackItem	//	msi
	{
		int nMargin;
		int nYExpiry;
	};
	StackClass<MarginStackItem> m_stkLeftMargin;
	StackClass<MarginStackItem> m_stkRightMargin;

	int m_nFirstShapeOnLine;
	int m_nNextYPos;

	ArrayOfInt m_arrBaseline;
	int m_nBaseLineShapeStartID;		//	The amount by which the index into m_arrBaseline is offset
	int m_nLowestBaseline;

	int m_nWidth;
	UINT m_nZoomLevel;
	BYTE m_cCharSet;

	bool m_bFirst;

private:
	CHTMLSectionCreator();
	CHTMLSectionCreator( const CHTMLSectionCreator& );
	CHTMLSectionCreator& operator =( const CHTMLSectionCreator& );
};

#endif //HTMLSECTIONCREATOR_H