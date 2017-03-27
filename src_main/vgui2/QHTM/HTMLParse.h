/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	htmlparse.h
Owner:	russf@gipsysoft.com
Purpose:	HTML parser, it will interpret the HTML and create HTML display engine
					The engine will then be used to layout and display to a window
					display context.
----------------------------------------------------------------------*/
#ifndef HTMLPARSE_H
#define HTMLPARSE_H

#ifndef QHTM_INCLUDES_H
	#include "QHTM_Includes.h"
#endif	//	QHTM_INCLUDES_H

#ifndef QHTM_TYPES_H
	#include "QHTM_Types.h"
#endif	//	QHTM_TYPES_H

#ifndef HTMLPARSEBASE_H
	#include "HTMLParseBase.h"
#endif	//	HTMLPARSEBASE_H

#ifndef STACK_H
	#include "Stack.h"
#endif	//	STACK_H

#ifndef SIMPLESTRING_H
	#include "SimpleString.h"
#endif	//	SIMPLESTRING_H

#ifndef HTMLFONTDEF_H
	#include "HTMLFontDef.h"
#endif	//	HTMLFONTDEF_H

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
class CHTMLDocument;
class CHTMLParagraphObject;
class CHTMLTable;
class CHTMLAnchor;
class CHTMLList;
class CDefaults;

class CHTMLParse : public CHTMLParseBase
//
//	HTML Parser
{
public:
	explicit CHTMLParse( LPCTSTR pcszStream, UINT uLength, HINSTANCE hInstLoadedFrom, LPCTSTR pcszFilePath, CDefaults *pDefaults );
	virtual ~CHTMLParse();

	CHTMLDocument * Parse();

	enum Align { algLeft, algCentre, algRight, algTop, algMiddle, algBottom };

protected:
	virtual void OnGotTag( const Token token, const CParameters &arrParameter );
	virtual void OnGotEndTag( const Token token );
	virtual void OnGotText( TCHAR ch );
	virtual void OnEndDoc();

	class CImage *OnLoadImage( LPCTSTR pcszFilename );

private:
	void OnGotBody( const CParameters &pList );
	void OnGotImage( const CParameters &pList );
	void OnGotHR( const CParameters &pList );
	void OnGotParagraph( const CParameters &pList );
	void OnGotFont( const CParameters &pList );
	void OnGotAnchor( const CParameters &pList );

	void OnGotTable( const CParameters &pList );
	void OnGotTableRow( const CParameters &pList );
	void OnGotTableCell( const CParameters &pList );

	void OnGotEndTableCell();
	void OnGotEndTableRow();
	void OnGotEndAnchor();

	void OnGotUnorderedList( const CParameters &pList );
	void OnGotOrderedList( const CParameters &pList );
	void OnGotListItem( const CParameters &pList );
	void CreateList( bool bOrdered );

	void OnGotEndListItem();
	void OnGotEndList();	// Both ordered & unordered

	void OnGotBlockQuote( const CParameters &pList );
	void OnGotEndBlockQuote();

	void OnGotAddress( const CParameters &pList );
	void OnGotEndAddress();
	
	void CleanupParse();

	void OnGotHeading( const Token token, const CParameters &pList );
	void OnGotMeta( const CParameters &pList );

	void CreateNewParagraph( int nLinesAbove, int nLinesBelow, Align alg );
	void CreateNewTextObject();
	void CreateNewProperties();
	void PopPreviousProperties();
	void UpdateItemLinkStatus( CHTMLParagraphObject *pItem );
	void GetFontName( LPTSTR pszBuffer, int nBufferSize, const CStaticString &srFontNameSpec );
	bool DoesFontExist( const StringClass &strFontName );

	ArrayOfChar m_strToken;

	//	REVIEW - os specific stuff
	#ifdef _WINDOWS
		#define MAX_FACE_NAME	LF_FACESIZE
	#endif	//	_WINDOWS

	struct Properties
	{
		Properties() { memset( this, 0, sizeof( Properties ) ); }
		Properties( const Properties &rhs ) { memcpy( this, &rhs, sizeof( Properties ) ); }
		COLORREF m_crFore;

		TCHAR szFaceName[ MAX_FACE_NAME ];
		int nSize;			//	Logical size not pixels
		bool bBold;
		bool bItalic;
		bool bStrikeThrough;
		bool bUnderline;
		int m_nSub;
		int m_nSup;

		Align nAlignment;
	};
	Properties *m_pProp;
	StackClass<Properties*> m_stkProperties;

	CHTMLDocument *m_pDocument;
	StackClass<CHTMLDocument *> m_stkDocument;
	CHTMLDocument *m_pMasterDocument;		// Top Most Doc

	CHTMLAnchor	*m_pLastAnchor;
	
	StackClass<CHTMLTable *> m_stkTable;
	StackClass<bool> m_stkInTableCell;

	StackClass<CHTMLList *> m_stkList;
	
	HINSTANCE m_hInstLoadedFrom;
	LPCTSTR m_pcszFilePath;

	CDefaults *m_pDefaults;

private:
	CHTMLParse();
	CHTMLParse( const CHTMLParse & );
	CHTMLParse & operator =( const CHTMLParse & );
};

#include "HTMLDocument.h"

////////////////////////////////////////////////////////////////////////
// Utility Methods
////////////////////////////////////////////////////////////////////////
//
//	Free fucntion to get the descriptive text from an alignment value
LPCTSTR GetStringFromAlignment( CHTMLParse::Align alg );

//
//	Free fucntion to get the alignment value from descriptive text
CHTMLParse::Align GetAlignmentFromString( const CStaticString &str, CHTMLParse::Align algDefault );

//
//	Get a parameter as a number, can be positive to reflect pixels
//	or negative to represent a percentage.
int GetNumberParameterPercent( const CStaticString &strParam, int nDefault );
int GetNumberParameter( const CStaticString &strParam, int nDefault );
int GetFontSize( const CStaticString &strParam, int nDefault );

//
//	Get the font size as pixels. Basically converts the HTML logical font sizes to pixels.
//	If the nSize passed is negative then it will assume the number is a point size request
int GetFontSizeAsPixels( HDC hdc, int nSize, UINT nZoomLevel );

#endif //HTMLPARSE_H
