/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLDocument.h
Owner:	russf@gipsysoft.com
Purpose:	The main document object.
----------------------------------------------------------------------*/
#ifndef HTMLDOCUMENT_H
#define HTMLDOCUMENT_H

class CHTMLDocumentObject;
class CHTMLParagraph;

class CHTMLDocument		//	doc
//
//	Main HTML document
{
public:
	CHTMLDocument( CDefaults *pDefaults );
	~CHTMLDocument();

	//	Hack alert - to workaround problem with data stored in tables which should not be (ideally).
	void ResetMeasuringKludge();

	void AddItem( CHTMLDocumentObject *pdocobj );
	void AddParagraph( CHTMLParagraph *pPara );
	CHTMLParagraph *CurrentParagraph() const;
	
	UINT GetFontDefIndex( const HTMLFontDef &def );
	const HTMLFontDef * GetFontDef( UINT uIndex );

	//
	//	This is the buffer used by all text for this document
	//	Items don't get deleted from it, instead new items are added to the
	//	end and old items remain unused.
	ArrayOfChar m_arrTextBuffer;

#ifdef _DEBUG
	virtual void Dump() const;
#endif	//	_DEBUG

	COLORREF m_crBack;
	COLORREF m_crLink;
	COLORREF m_crLinkHover;
	int m_nLeftMargin;
	int m_nTopMargin;
	int m_nRightMargin;
	int m_nBottomMargin;
	
	ArrayClass<CHTMLDocumentObject*> m_arrItems;

	// A Map which stores the images loaded for this document
	MapClass<StringClass, CImage*> m_mapImages;

	StringClass m_strTitle;

	ArrayClass<HTMLFontDef*> m_arrFontDefs;
	MapClass<HTMLFontDef, UINT> m_mapFonts;

	BYTE m_cCharSet;
	CDefaults *m_pDefaults;

private:
	CHTMLParagraph *m_pCurrentPara;

private:
	CHTMLDocument( const CHTMLDocument &);
	CHTMLDocument& operator =( const CHTMLDocument &);
};

//
//	Document object includes....

#ifndef HTMLDOCUMENTOBJECT_H
	#include "HTMLDocumentObject.h"
#endif	//	HTMLDOCUMENTOBJECT_H

#ifndef HTMLPARAGRAPH_H
	#include "HTMLParagraph.h"
#endif	//	HTMLPARAGRAPH_H

#ifndef HTMLPARAGRAPHOBJECT_H
	#include "HTMLParagraphObject.h"
#endif	//	HTMLPARAGRAPHOBJECT_H

#ifndef HTMLBLOCKQUOTE_H
	#include "HTMLBlockQuote.h"
#endif	//	HTMLBLOCKQUOTE_H

#ifndef HTMLLIST_H
	#include "HTMLList.h"
#endif	//	HTMLLIST_H

#ifndef HTMLLISTITEM_H
	#include "HTMLListItem.h"
#endif	//	HTMLLISTITEM_H

#ifndef HTMLTABLE_H
	#include "HTMLTable.h"
#endif	//HTMLTABLE_H

#ifndef HTMLTABLECELL_H
	#include "HTMLTableCell.h"
#endif	//	HTMLTABLECELL_H

#ifndef HTMLTEXTBLOCK_H
	#include "HTMLTextBlock.h"
#endif	//	HTMLTEXTBLOCK_H

#ifndef HTMLANCHOR_H
	#include "HTMLAnchor.h"
#endif	//	HTMLANCHOR_H

#ifndef HTMLHORIZONTALRULE_H
	#include "HTMLHorizontalRule.h"
#endif	//	HTMLHORIZONTALRULE_H

#ifndef HTMLIMAGE_H
	#include "HTMLImage.h"
#endif	//	HTMLIMAGE_H


#endif //HTMLDOCUMENT_H