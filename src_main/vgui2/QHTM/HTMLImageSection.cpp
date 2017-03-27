/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLImageSection.cpp
Owner:	russf@gipsysoft.com
Purpose:	HTML Image section.
					NOTE: Currently ignores the width and height
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <ImgLib.h>
#include "HTMLImageSection.h"
#include "Utils.h"

CHTMLImageSection::CHTMLImageSection( CParentSection *psect, CImage *pImage, int nBorder )
	: CHTMLSectionABC( psect )
	, m_pImage( pImage )
	,	m_nBorder( nBorder )
	, m_nTimerID( knNullTimerId )
	, m_nFrame( 0 )
{
	ASSERT( m_pImage );
	if( m_pImage->GetFrameCount() && m_pImage->GetFrameTime( 0 ) )
	{
		m_nTimerID = RegisterTimerEvent( this, m_pImage->GetFrameTime( 0 ) );
	}
}

CHTMLImageSection::~CHTMLImageSection()
{
	if( m_nTimerID !=  knNullTimerId )
	{
		UnregisterTimerEvent( m_nTimerID );
		m_nTimerID = knNullTimerId;
	}
}


void CHTMLImageSection::OnDraw( CDrawContext &dc )
{
	if( m_pImage )
	{
		if( m_nBorder )
		{
			COLORREF clr = RGB( 0, 0, 0 );
			
			//
			//	Top border
			CRect rc( left, top, right, top + m_nBorder );
			dc.FillRect( rc, clr );

			//
			//	Bottom border
			rc.bottom = bottom;
			rc.top = bottom - m_nBorder;
			dc.FillRect( rc, clr );

			//
			//	Left border
			rc.top = top;
			rc.right = rc.left + m_nBorder;
			dc.FillRect( rc, clr );

			//
			//	Right border
			rc.right = right;
			rc.left = right - m_nBorder;
			dc.FillRect( rc, clr );

			VERIFY( m_pImage->DrawFrame( m_nFrame, dc.GetSafeHdc(), left + m_nBorder, top + m_nBorder, right - m_nBorder, bottom - m_nBorder ) );
		}
		else
		{
			VERIFY( m_pImage->DrawFrame( m_nFrame, dc.GetSafeHdc(), left, top, right, bottom ) );
		}
	}

	if( IsMouseInSection() && IsLink() )
	{
		dc.Rectangle( *this, CHTMLSectionABC::LinkHoverColour() );
	}
#ifdef DRAW_DEBUG
	//	Do this after the image so it draws on top!
	CHTMLSectionABC::OnDraw( dc );
#endif	//	DRAW_DEBUG
}


void CHTMLImageSection::OnTimer( int nTimerID )
{
	if( nTimerID == m_nTimerID )
	{
		m_nFrame++;
		if( m_nFrame == m_pImage->GetFrameCount() )
			m_nFrame = 0;

		UnregisterTimerEvent( m_nTimerID );
		m_nTimerID = RegisterTimerEvent( this, m_pImage->GetFrameTime( m_nFrame ) );
		ForceRedraw();
	}
	else
	{
		CHTMLSectionABC::OnTimer( nTimerID );
	}
}

