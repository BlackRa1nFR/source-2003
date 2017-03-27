/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLFontDef.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef HTMLFONTDEF_H
#define HTMLFONTDEF_H

#define MAX_FONT_NAMES	128

struct HTMLFontDef
{
	HTMLFontDef() {};
	HTMLFontDef( LPCTSTR pcszFont, int nSize, bool bBold, bool bItalic, bool bUnderline, bool bStrikeout, int nSup, int nSub )
		: m_strFont( pcszFont )
		, m_nSize( nSize )
		, m_nWeight( bBold ? FW_BOLD : FW_NORMAL )
		, m_bUnderline( bUnderline )
		, m_bItalic( bItalic )
		, m_bStrike( bStrikeout )
		, m_nSub( nSub )
		, m_nSup( nSup )
	{
	}

	StringClass m_strFont;
	int m_nSize;
	int m_nWeight;
	bool m_bUnderline;
	bool m_bItalic;
	bool m_bStrike;
	int m_nSub;
	int m_nSup;
};


#endif //HTMLFONTDEF_H