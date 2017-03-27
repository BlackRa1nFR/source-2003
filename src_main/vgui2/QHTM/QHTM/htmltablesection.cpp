/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLTableSection.cpp
Owner:	russf@gipsysoft.com
Purpose:	HTML Table or Cell section.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLTableSection.h"

CHTMLTableSection::CHTMLTableSection( CParentSection *psect, int border, COLORREF crBorderDark, COLORREF crBorderLight, bool transparent, COLORREF crBack)
	: CHTMLSectionABC( psect )
	, m_crBack( crBack )
	, m_crBorderDark( crBorderDark )
	, m_crBorderLight ( crBorderLight )
	, m_nBorder( border )
{
	Transparent( transparent );
}

CHTMLTableSection::~CHTMLTableSection()
{
}


void CHTMLTableSection::OnDraw( CDrawContext &dc )
{
	CRect internalRect = *this;
	const int nScaledXBorder = dc.ScaleX(m_nBorder);
	// Leave room for the border
	internalRect.Inflate(-nScaledXBorder, dc.ScaleY(-m_nBorder));
	internalRect.bottom++;
	internalRect.right++;
	// Fill in all but the border
	if (!IsTransparent())
	{
		dc.FillRect(internalRect, m_crBack);
	}
	// Now, draw the border, if applicable.
	// Light border is top/left
	// Dark border is bottom/right
	if (nScaledXBorder > 1)
	{
		CPoint p[6];
		p[0] = CPoint(left, bottom);
		p[1] = TopLeft();
		p[2] = CPoint(right, top);
		p[3] = CPoint(internalRect.right, internalRect.top);
		p[4] = internalRect.TopLeft();
		p[5] = CPoint(internalRect.left, internalRect.bottom);

		dc.PolyFillOutlined(p, 6, m_crBorderLight, m_crBorderLight);

		p[0] = CPoint(right, top);
		p[1] = BottomRight();
		p[2] = CPoint(left, bottom);
		p[3] = CPoint(internalRect.left, internalRect.bottom);
		p[4] = internalRect.BottomRight();
		p[5] = CPoint(internalRect.right, internalRect.top);

		dc.PolyFillOutlined(p, 6, m_crBorderDark, m_crBorderDark);
	}
	else if( nScaledXBorder == 1 )
	{
		CPoint p[3];
		p[0] = CPoint(left, bottom);
		p[1] = TopLeft();
		p[2] = CPoint(right, top);
		dc.PolyLine(p, 3, m_crBorderLight);

		p[0] = CPoint(right, top);;
		p[1] = BottomRight();
		p[2] = CPoint(left, bottom);
		dc.PolyLine(p, 3, m_crBorderDark);
	}			

#ifdef _DEBUG
	//Draw last in case the table has a background colour
	CHTMLSectionABC::OnDraw( dc );
#endif	//	_DEBUG
}
