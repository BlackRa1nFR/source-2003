/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLParseBase.h
Owner:	russf@gipsysoft.com
Purpose:	HTML parser base class
					At it's most basic it will fire OnGotText, OnGotTag and OnGotEndTag
					events, you must derived from this class to get these events. All
					plain text goes to OnGotText and all tags go to OnGotTag and
					OnGotEndTag. Withinthe tag is also any attributes the tag may have.
----------------------------------------------------------------------*/
#ifndef HTMLPARSEBASE_H
#define HTMLPARSEBASE_H

#ifndef STATICSTRING_H
	#include "StaticString.h"
#endif	//	STATICSTRING_H

//	The maximum string length of a tag
#define MAX_TAG_NAME 20

//	The maximum string length of a parameter name
#define MAX_PARAM_NAME 20

//	The maximum string length of a parameter
#define MAX_PARAM 256

#ifndef ArrayClass
	#include <array.h>
	#define ArrayClass			Container::CArray
#endif	//	ArrayClass


class CHTMLParseBase
{
public:

	CHTMLParseBase( LPCTSTR pcszStream, UINT uLength );
	virtual ~CHTMLParseBase();

	enum Token
	{
		tokIgnore
		,tokBody
		,tokFont
		,tokBold
		,tokUnderline
		,tokItalic
		,tokStrikeout
		,tokImage
		,tokTableDef
		,tokTableRow
		,tokTableHeading
		,tokTable
		,tokHorizontalRule
		,tokParagraph
		,tokBreak
		,tokAnchor
		,tokH1
		,tokH2
		,tokH3
		,tokH4
		,tokH5
		,tokH6
		,tokListItem
		,tokOrderedList
		,tokUnorderedList
		,tokBlockQuote
		,tokAddress
		,tokDocumentTitle
		,tokCenter
		,tokDiv
		,tokPre
		,tokCode
		,tokMeta
		,tokHead
		,tokHTML
		,tokSub
		,tokSup
	};

	enum Param
	{
		pUnknown,
		pBColor,
		pAlign,
		pVAlign,
		pNoShade,
		pSize,
		pWidth,
		pHeight,
		pName,
		pHref,
		pTitle,
		pSrc,
		pColor,
		pFace,
		pAlt,
		pBorder,
		pNoWrap,
		pCellSpacing,
		pCellPadding,
		pBorderColor,
		pBorderColorLight,
		pBorderColorDark,
		pLink,
		pALink,
		pValue,
		pType,
		pMarginTop,
		pMarginBottom,
		pMarginLeft,
		pMarginRight,
		pContent,
		pHTTPEquiv
	};

	static int Initialise();

protected:
	class CParameterPair
	{
	public:
		Param m_param;
		CStaticString m_strValue;
	};
	typedef ArrayClass< CParameterPair > CParameters;

	//	Call this to actually do the parsing
	bool ParseBase();

	//	Called when a tag has been parsed, complete with it's parameter list
	virtual void OnGotTag( const Token token, const CParameters &arrParameter );
	//	Called when an end tag has been parsed
	virtual void OnGotEndTag( const Token token );

	//	Send when a comment is found
	virtual void OnGotComment( LPCTSTR pszText, UINT uLength );

	//	called when some text has been parsed
	virtual void OnGotText( TCHAR ch );

	//	Called when the end of document is reached
	virtual void OnEndDoc();

	//	Are we currently in preformatted mode?
	bool IsPreformatted() { return m_bPreFormatted; }

private:
	inline void EatStreamWhitespace();

	void OnGotTagText( const CStaticString &strTag );

	bool GetTag( CStaticString &strTag );
	bool GetSpecialChar( TCHAR &ch );
	bool AddTokenChar( const TCHAR ch );
	bool GetChar( TCHAR &ch );
	bool GetTagChar( TCHAR &ch );

	LPCTSTR  m_pcszStream;
	LPCTSTR  m_pcszStreamMax;

	bool m_bPreviousCharWasSpace;
	bool m_bPreviousWasSpecial;
	bool m_bPreFormatted;

private:
	CHTMLParseBase();
	CHTMLParseBase( const CHTMLParseBase & );
	CHTMLParseBase& operator =( const CHTMLParseBase & );
};

COLORREF GetColourFromString( const CStaticString &strColour, COLORREF crDefault);

#endif //HTMLPARSEBASE_H
