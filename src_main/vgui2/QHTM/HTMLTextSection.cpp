/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLTextSection.cpp
Owner:	russf@gipsysoft.com
Purpose:	Simple drawn text object.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLTextSection.h"
#include "Utils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHTMLTextSection::CHTMLTextSection( CParentSection *psect, LPCTSTR pcszText, int nLength, FontDef &fdef, COLORREF crFore )
	: CHTMLSectionABC( psect )
	, m_str( pcszText, nLength )
	, m_fdef( fdef )
	, m_crFore( crFore )
{
	Transparent( true );
}

CHTMLTextSection::~CHTMLTextSection()
{

}


void CHTMLTextSection::OnDraw( CDrawContext &dc )
{
#ifdef DRAW_DEBUG
	CHTMLSectionABC::OnDraw( dc );
#endif	//	DRAW_DEBUG

	dc.SelectFont( m_fdef );
	if( IsLink() )
	{
		if( IsMouseInSection() )
		{
			dc.DrawText( left, top, m_str, m_str.GetLength(), LinkHoverColour() );
		}
		else
		{
			dc.DrawText( left, top, m_str, m_str.GetLength(), LinkColour() );
		}
	}
	else
	{
		dc.DrawText( left, top, m_str, m_str.GetLength(), m_crFore );
	}
}
