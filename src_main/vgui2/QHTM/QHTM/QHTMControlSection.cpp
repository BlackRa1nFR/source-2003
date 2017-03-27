/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	QHTMControlSection.cpp
Owner:	russf@gipsysoft.com
Purpose:	Main control section.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "QHTM.h"
#include <stdio.h>
#include <SHELLAPI.H>	//	For ShellExecute
#include <stdlib.h>
#include "ResourceLoader.h"
#include "QHTMControlSection.h"
#include "Utils.h"

extern bool LoadTextFile( LPCTSTR pcszFilename, LPTSTR &pszBuffer, UINT &uLength );

//	: 'this' : used in base member initializer list
//	Ignored because we want to pass the 'this' pointer...
#pragma warning( disable: 4355 )

extern bool GotoURL( LPCTSTR url, int showcmd );

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQHTMControlSection::CQHTMControlSection( HWND hwnd )
	: CWindowSection( APP_WINDOW )
	, m_defaults( g_defaults )
	, m_htmlSection( this, &m_defaults )
	, m_bEnableTooltips( true )
	, m_bLayingOut( false )
	, m_bUseColorStatic( false )
	, m_bEnableScrollbars( true )
{
	m_hwnd = hwnd;
}


CQHTMControlSection::~CQHTMControlSection()
{
}


void CQHTMControlSection::OnLayout( const CRect &rc )
{
	CWindowSection::OnLayout( rc );

	LayoutHTML();
}


void CQHTMControlSection::SetText( LPCTSTR pcszText )
{
	if( pcszText == NULL )
	{
		pcszText = _T("");
	}
	m_defaults.m_crBackground = ::GetSysColor( COLOR_3DFACE );
	m_htmlSection.SetHTML( pcszText, lstrlen( pcszText ), NULL );

	ResetScrollPos();
	LayoutHTML();
	ForceRedraw( *this );
}


void CQHTMControlSection::OnExecuteHyperlink( LPCTSTR pcszLink )
{
	NMQHTM nm = { { m_hwnd, GetWindowLong( m_hwnd, GWL_ID ), QHTMN_HYPERLINK }, pcszLink, TRUE };
	::SendMessage( ::GetParent( m_hwnd ), WM_NOTIFY, (WPARAM)nm.hdr.idFrom, (LPARAM)&nm );
	if( nm.resReturnValue )
	{
		GotoURL( pcszLink, SW_SHOW );
	}
}


bool CQHTMControlSection::LoadFromResource( HINSTANCE hInst, LPCTSTR pcszName )
{
	m_defaults.m_crBackground = ::GetSysColor( COLOR_3DFACE );
	if( m_htmlSection.SetHTML( hInst, pcszName ) )
	{
		ResetScrollPos();
		LayoutHTML();
		ForceRedraw( *this );
		return true;
	}
	return false;
}


bool CQHTMControlSection::LoadFromFile( LPCTSTR pcszFilename )
{
	bool bRetVal = false;
	m_defaults.m_crBackground = ::GetSysColor( COLOR_3DFACE );
	if( m_htmlSection.SetHTMLFile( pcszFilename ) )
	{
		ResetScrollPos();
		LayoutHTML();
		ForceRedraw( *this );
		bRetVal = true;
	}

	return bRetVal;
}


void CQHTMControlSection::LayoutHTML()
{
	m_bLayingOut = true;
	/*
		Get the measurements of the window without scrollbars.
		Deterine if the document will fit without scrollbars.
		if it does then simply remove any scrollbars and move on.

		If the doc. does not fit without scrollbars:
			Does the doc fit vertically?
				No, so add a vertical scrollbar.
			Does the doc fit horizontally?
				No, so add a horiziontal scrollbar.
		
		With new sizes layout the doc once more.
	*/

	m_htmlSection.ResetMeasuringKludge();

	GetClientRect( m_hwnd, this );

	//
	//	Get the width and height of the scroll bars
	const int nScrollWidth = GetSystemMetrics( SM_CXVSCROLL );
	const int nScrollHeight = GetSystemMetrics( SM_CXHSCROLL );
	const int nHorizontalPos = ::GetScrollPos( m_hwnd, SB_HORZ );
	const int nVerticalPos = ::GetScrollPos( m_hwnd, SB_VERT );

	//
	//	If the window has scroll bars then remove them from our width and height
	if( GetWindowLong( m_hwnd, GWL_STYLE ) & WS_VSCROLL )
		right += nScrollWidth;

	if( GetWindowLong( m_hwnd, GWL_STYLE ) & WS_HSCROLL )
		bottom += nScrollHeight;

	CDrawContext dc;
	m_htmlSection.OnLayout( *this,  dc );

	CSize size( m_htmlSection.GetSize() );
	if( m_bEnableScrollbars )
	{
		BOOL bShowHorizontal = FALSE;
		BOOL bShowVertical = FALSE;
		bool bFirst = true;
		bool bDoItAgain = false;
		while( bFirst || bDoItAgain )
		{
			//
			//	Create locals of the current width and height so we can test them. If we uses Width() and Height() then they
			//	could be changed by the tests below.
			const int nCurrentWidth = Width();
			const int nCurrentHeight = Height();
			bDoItAgain = false;
			if( size.cy > nCurrentHeight && ! bShowVertical )
			{
				bShowVertical = true;
				right -= nScrollWidth;
				bDoItAgain = true;
			}

			if( size.cx > nCurrentWidth && !bShowHorizontal )
			{
				bShowHorizontal = true;
				bottom -= nScrollHeight;
				bDoItAgain = true;
			}

			if( bDoItAgain )
			{
				m_htmlSection.OnLayout( *this,  dc );
				size = m_htmlSection.GetSize();
			}

			if( !bFirst )
				break;
			bFirst = false;
		}

		if( size.cx > Width() )
		{
			SCROLLINFO siNew = {0};
			siNew.cbSize = sizeof( siNew );
			siNew.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			siNew.nMax = size.cx;
			siNew.nPos = min( siNew.nMax, nHorizontalPos );
			siNew.nPage = Width();
			SetScrollInfo( m_hwnd, SB_HORZ, &siNew, TRUE );
			bShowHorizontal = TRUE;
		}

		if( size.cy > Height() )
		{
			SCROLLINFO siNew = {0};
			siNew.cbSize = sizeof( siNew );
			siNew.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			siNew.nMax = size.cy;
			siNew.nPos = min( siNew.nMax, nVerticalPos );
			siNew.nPage = Height();
			SetScrollInfo( m_hwnd, SB_VERT, &siNew, TRUE );
			bShowVertical = TRUE;
		}

		ShowScrollBar( m_hwnd, SB_HORZ, bShowHorizontal );
		ShowScrollBar( m_hwnd, SB_VERT, bShowVertical );
	}
	m_bLayingOut = false;
}


void CQHTMControlSection::OnVScroll( int nScrollCode )
{
	CSize size( m_htmlSection.GetSize() );
	int	nNewPos = ::GetScrollPos( m_hwnd, SB_VERT );
	int nOldPos = nNewPos;

	switch( nScrollCode )
	{
	case SB_LINEDOWN:
		if( nNewPos < size.cy - Height() )
			nNewPos+=5;
		break;


	case SB_LINEUP:
		if( nNewPos > 0)
			nNewPos-=5;
		break;


	case SB_PAGEDOWN:
		if( nNewPos < size.cy - Height() )
		{
			nNewPos += Height();
			if( nNewPos > size.cy - Height() )
				nNewPos = size.cy - Height();
		}
		break;


	case SB_PAGEUP:
		if( nNewPos > 0 )
		{
			nNewPos -= (Height() - 1);
			if( nNewPos < 0 )
				nNewPos = 0;
		}
		break;

	case SB_THUMBTRACK:
		{
			SCROLLINFO siNew;
			siNew.cbSize = sizeof( siNew );
			siNew.fMask = SIF_TRACKPOS;
			GetScrollInfo( m_hwnd, SB_VERT, &siNew );
			nNewPos = siNew.nTrackPos;
		}
		break;

	default:
		break;
	}

	if( nNewPos != nOldPos )
	{
		::SetScrollPos( m_hwnd, SB_VERT, nNewPos, TRUE );
		m_htmlSection.SetPos( nNewPos );
		ForceRedraw( *this );
	}
}


void CQHTMControlSection::OnHScroll( int nScrollCode )
{
	CSize size( m_htmlSection.GetSize() );
	int	nNewPos = ::GetScrollPos( m_hwnd, SB_HORZ );
	int nOldPos = nNewPos;

	switch( nScrollCode )
	{
	case SB_LINEDOWN:
		if( nNewPos < size.cx - Width() )
			nNewPos+=5;
		break;


	case SB_LINEUP:
		if( nNewPos > 0)
			nNewPos-=5;
		break;


	case SB_PAGEDOWN:
		if( nNewPos < size.cx - Width() )
		{
			nNewPos += Width();
			if( nNewPos > size.cx - Width() )
				nNewPos = size.cx - Width();
		}
		break;


	case SB_PAGEUP:
		if( nNewPos > 0 )
		{
			nNewPos -= (Width() - 1);
			if( nNewPos < 0 )
				nNewPos = 0;
		}
		break;

	case SB_THUMBTRACK:
		{
			SCROLLINFO siNew;
			siNew.cbSize = sizeof( siNew );
			siNew.fMask = SIF_TRACKPOS;
			GetScrollInfo( m_hwnd, SB_HORZ, &siNew );
			nNewPos = siNew.nTrackPos;
		}
		break;

	default:
		break;
	}

	if( nNewPos != nOldPos )
	{
		::SetScrollPos( m_hwnd, SB_HORZ, nNewPos, TRUE );
		m_htmlSection.SetPosH( nNewPos );
		ForceRedraw( *this );
	}
}


int CQHTMControlSection::SetOption( UINT uOptionIndex, LPARAM lParam )
{
	switch( uOptionIndex )
	{
	case QHTM_OPT_TOOLTIPS:
		if( lParam )
			m_htmlSection.EnableTooltips( true );
		else
			m_htmlSection.EnableTooltips( false );
		return true;

	case QHTM_OPT_ZOOMLEVEL:
		if( lParam >= QHTM_ZOOM_MIN && lParam <= QHTM_ZOOM_MAX )
		{
			if( lParam != m_htmlSection.GetZoomLevel() )
			{
				m_htmlSection.SetZoomLevel( lParam );
				TRACE( _T("Zoom level set to %d\n"), lParam );

				LayoutHTML();
				ForceRedraw( *this );
			}
			return true;
		}

	case QHTM_OPT_MARGINS:
		{
			LPRECT lprect = reinterpret_cast<CRect *>( lParam );
			if( !::IsBadReadPtr( lprect, sizeof( RECT ) ) )
			{
				m_htmlSection.SetDefaultMargins( lprect );
				LayoutHTML();
				return true;
			}
		}

	case QHTM_OPT_USE_COLOR_STATIC:
		if( lParam )
			m_bUseColorStatic = true;
		else
			m_bUseColorStatic = false;
		return true;

	case QHTM_OPT_ENABLE_SCROLLBARS:
		m_bEnableScrollbars = lParam ? true : false;
		break;
	}
	return false;
}


LPARAM CQHTMControlSection::GetOption( UINT uOptionIndex, LPARAM lParam )
{
	switch( uOptionIndex )
	{
	case QHTM_OPT_TOOLTIPS:
		return m_htmlSection.IsTooltipsEnabled();

	case QHTM_OPT_ZOOMLEVEL:
		return m_htmlSection.GetZoomLevel();

	case QHTM_OPT_MARGINS:
		{
			LPRECT lprect = reinterpret_cast<CRect *>( lParam );
			if( !::IsBadWritePtr( lprect, sizeof( RECT ) ) )
			{
				m_htmlSection.GetDefaultMargins( lprect );
				LayoutHTML();
				return true;
			}
		}

	case QHTM_OPT_USE_COLOR_STATIC:
		return m_bUseColorStatic;
	}
	return 0;
}


void CQHTMControlSection::GotoLink( LPCTSTR pcszLinkName )
{
	m_htmlSection.GotoLink( pcszLinkName );
}


void CQHTMControlSection::ResetScrollPos()
{
	::SetScrollPos( m_hwnd, SB_HORZ, 0, TRUE );
	::SetScrollPos( m_hwnd, SB_VERT, 0, TRUE );

	m_htmlSection.SetPos( 0 );
	m_htmlSection.SetPosH( 0 );
}


bool CQHTMControlSection::OnNotify( const CSectionABC *pChild, const int nEvent )
{
	if( NotifyParent( nEvent, pChild ) )
		return true;

	if( pChild == &m_htmlSection && nEvent == 1 )
	{
		SCROLLINFO siOld = {0};
		siOld.cbSize = sizeof( siOld );
		siOld.fMask = SIF_POS | SIF_RANGE;
		GetScrollInfo( m_hwnd, SB_VERT, &siOld );
		siOld.nPos = min( m_htmlSection.GetScrollPos(), siOld.nMax );
		SetScrollInfo( m_hwnd, SB_VERT, &siOld, TRUE );

		siOld.cbSize = sizeof( siOld );
		siOld.fMask = SIF_POS | SIF_RANGE;
		GetScrollInfo( m_hwnd, SB_HORZ, &siOld );
		siOld.nPos = min( m_htmlSection.GetScrollPosH(), siOld.nMax );
		SetScrollInfo( m_hwnd, SB_HORZ, &siOld, TRUE );
	}
	return false;
}


UINT CQHTMControlSection::GetTitleLength() const
{
	return m_htmlSection.GetTitle().GetLength();
}


UINT CQHTMControlSection::GetTitle( UINT uBufferLength, LPTSTR pszBuffer ) const
{
	const StringClass & str = m_htmlSection.GetTitle();
	UINT uLength = min( str.GetLength(), uBufferLength - 1 );
	_tcsncpy( pszBuffer, m_htmlSection.GetTitle(), uLength );
	pszBuffer[ uLength ] = _T('\000');
	return uLength;
}


void CQHTMControlSection::SetFont( HFONT hFont )
{
	m_defaults.SetFont( hFont );
}


bool CQHTMControlSection::GetBackgroundColours( HDC hdc, HBRUSH &hbr ) const
{
	bool bReturnValue = false;
	if( m_bUseColorStatic )
	{
		HWND hwndParent = ::GetParent( m_hwnd );
		if( hwndParent )
		{
			BOOL bRetVal = SendMessage( hwndParent, WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)m_hwnd );
			if( bRetVal )
			{
				hbr = (HBRUSH)bRetVal;
				bReturnValue = true;
			}
		}
	}
	return bReturnValue;
}