/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLSectionABC.cpp
Owner:	russf@gipsysoft.com
Purpose:	Base class used for the HTML display sections.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLSectionABC.h"
#include "HTMLSectionLink.h"
#include "Utils.h"

bool g_bShowingTooltips = true;

CHTMLSectionABC::CHTMLSectionABC( CParentSection *psect )
	: CSectionABC( psect )
	, m_pHtmlLink( NULL )
{

}


CHTMLSectionABC::~CHTMLSectionABC()
{
}


#ifdef DRAW_DEBUG
void CHTMLSectionABC::OnDraw( CDrawContext &dc )
{
	CSectionABC::OnDraw( dc );
	dc.Rectangle( *this, RGB( 255, 0, 0 ) );
}
#endif	//	 DRAW_DEBUG


void CHTMLSectionABC::SetAsLink( CHTMLSectionLink* pLink )
{
	m_pHtmlLink = pLink;

	if( pLink )
	{
		m_cursor.Load( CCursor::knHand );
		if( pLink->m_strLinkTitle.GetLength() )
			SetTipText( pLink->m_strLinkTitle );
		else if( pLink->m_strLinkTarget.GetLength() )
			SetTipText( pLink->m_strLinkTarget );
		pLink->AddSection( this );
	}
}

LPCTSTR CHTMLSectionABC::GetLinkTarget() const
{ 
	return m_pHtmlLink ? (LPCTSTR)m_pHtmlLink->m_strLinkTarget : (LPCTSTR)0; 
}


void CHTMLSectionABC::OnMouseEnter()
{
	CSectionABC::OnMouseEnter();

	if (m_pHtmlLink)
		m_pHtmlLink->OnMouseEnter();
}


void CHTMLSectionABC::OnMouseLeave()
{
	CSectionABC::OnMouseLeave();

	if (m_pHtmlLink)
		m_pHtmlLink->OnMouseLeave();

}


void CHTMLSectionABC::OnMouseLeftUp( const CPoint & )
{
	if( m_pHtmlLink )
	{
		NotifyParent( CHTMLSectionLink::knEventGotoURL );
	}
}


StringClass CHTMLSectionABC::GetTipText() const
{
	if( g_bShowingTooltips )
		return CSectionABC::GetTipText();

	return NULL;
}


void CHTMLSectionABC::EnableTooltips( bool bEnable )
{
	g_bShowingTooltips = bEnable;
}

bool CHTMLSectionABC::IsTooltipsEnabled()
{
	return g_bShowingTooltips;
}

COLORREF CHTMLSectionABC::LinkColour()
{
	if (m_pHtmlLink)
		return m_pHtmlLink->m_crLink;
	else
		return RGB( 141, 7, 102 );
}


COLORREF CHTMLSectionABC::LinkHoverColour()
{
	if (m_pHtmlLink)
		return m_pHtmlLink->m_crHover;
	else
		return RGB( 29, 49, 149 );
}

