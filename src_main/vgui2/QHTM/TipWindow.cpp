/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	TipWindow.cpp
Owner:	leea@gipsysoft.com
Purpose:	Tool tips window
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "Defaults.h"
#include "TipWindow.h"
#include "utils.h"
#include "QHTM.h"

//	: 'this' : used in base member initializer list
//	Ignored because we want to pass the 'this' pointer...
#pragma warning( disable: 4355 )

extern void GetDisplayWorkArea( POINT pt, RECT &rc );

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

enum { g_knTipClearanceX = 4, g_knTipClearanceY = 22, g_knMaxTipWidth = 400, g_knMaxHeight = 400 };

DWORD	CTipWindow::m_nLastTipCreated = 0;

static bool HeightExceedsTipMax( StringClass &strHTML, int nMaxHeight )
{
	HDC hdcScreen = GetDC( NULL );
	int nHeaderHeight = QHTM_PrintGetHTMLHeight( hdcScreen, strHTML, g_knMaxTipWidth, QHTM_ZOOM_DEFAULT );
	ReleaseDC( NULL, hdcScreen );
	return nHeaderHeight > nMaxHeight;
}

CTipWindow::CTipWindow( LPCTSTR pcszTip, CPoint &pt )
	:	CWindowSection( TIP_WINDOW )
	, m_defaults( g_defaults )
	, m_htmlSection( this, &m_defaults )
{
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof( ncm );
	if( SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0 ) )
	{
		m_defaults.SetFont( ncm.lfStatusFont );
	}
	ASSERT( pcszTip );
	m_strTip = pcszTip;

	CRect rcDesktop;
	GetDisplayWorkArea( pt, rcDesktop );
	m_htmlSection.SetZoomLevel( QHTM_ZOOM_DEFAULT );

	//
	//	Stop tip exceeding 'g_knMaxHeight' pixels
	bool bDeleted = false;
	while( HeightExceedsTipMax( m_strTip, g_knMaxHeight ) )
	{	
		int iDeleteAt = m_strTip.GetLength() - m_strTip.GetLength() / 100 * 15; // Knock 15% off
		m_strTip.Delete( iDeleteAt, m_strTip.GetLength() - iDeleteAt );

		UINT iCheckHTML = m_strTip.GetLength() - 1;
		while( iCheckHTML + 5 > m_strTip.GetLength() )
		{
			if( m_strTip[iCheckHTML] == '<' )
			{
				m_strTip.Delete( iCheckHTML, m_strTip.GetLength() - iCheckHTML );
				break;
			}
			iCheckHTML--;
		}
		bDeleted = true;
	}

	if( bDeleted )
	{
		m_strTip += _T("...<br><b>more</b>");
	}

	m_defaults.m_crBackground = GetSysColor( COLOR_INFOBK );
	m_defaults.m_crDefaultForeColour = GetSysColor( COLOR_INFOTEXT );

	m_htmlSection.SetHTML( m_strTip, m_strTip.GetLength(), NULL );
	CRect rcMargins( 2, 2, 2, 5 );
	m_htmlSection.SetDefaultMargins( rcMargins );
	CRect rcTip( 0, 0, g_knMaxTipWidth, 30 );
	CDrawContext dc;
	m_htmlSection.OnLayout( rcTip, dc );
	const CSize size( m_htmlSection.GetSize() );

	//
	//	Adjust for mouse size
	pt.x += g_knTipClearanceX;
	pt.y += g_knTipClearanceY;

	rcTip.Set( pt.x, pt.y , pt.x + size.cx + 3, pt.y + size.cy );

	//
	//	Move rect around based on desktop rect
	if( rcTip.right > rcDesktop.right )
	{
		rcTip.Offset( -(rcTip.right - rcDesktop.right - g_knTipClearanceX), 0 );
	}

	if( rcTip.bottom > rcDesktop.bottom )
	{
		rcTip.Offset( 0, -rcTip.Height() - g_knTipClearanceY );
	}


	//
	//	Adjust tip so that it is within bounds
	VERIFY( CWindowSection::Create( rcTip ) );
	m_nLastTipCreated = GetTickCount();
	ShowWindow( SW_SHOW );
}


CTipWindow::~CTipWindow()
{
	m_pCurrentTip = NULL;
}



void CTipWindow::OnDraw( CDrawContext &dc )
{
	CWindowSection::OnDraw( dc );
}


void CTipWindow::OnDestroy()
{
	
}


void CTipWindow::OnWindowDestroyed()
{
	delete this;
}

void CTipWindow::OnMouseMove( const CPoint & )
{
	KillTip();
}


void CTipWindow::OnLayout( const CRect &rc )
{
	CWindowSection::OnLayout( rc );
	CDrawContext dc;
	m_htmlSection.OnLayout( rc, dc );
}
