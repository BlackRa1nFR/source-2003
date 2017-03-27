/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	WindowSection.cpp
Owner:	leea@gipsysoft.com
Purpose:	Manages funnneling windows messages into CSectionABC
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "WindowSection.h"
#include "utils.h"
#include "TipWindow.h"
#ifndef MSH_MOUSEWHEEL
	#include <ZMouse.h>
#endif	//	MSH_MOUSEWHEEL

#define WINDOW_DATA	0

#include "Palette.h"
extern void GetDisplayWorkArea( HWND hwnd, RECT &rc );

LPCTSTR CWindowSection::m_pcszClassName = _T("SectionWindow001");

CSectionABC *g_pSectMouseDowned = NULL;

bool IsMouseDown()
{
	return g_pSectMouseDowned != NULL;
}

void CancelMouseDowns()
{
	if( g_pSectMouseDowned )
	{
		g_pSectMouseDowned->OnStopMouseDown();
		g_pSectMouseDowned = NULL;
	}

}



CSectionABC *g_pSectHighlight = NULL;

extern void CancelHighlight()
{
	if( g_pSectHighlight )
	{
		g_pSectHighlight->OnMouseLeave();
		g_pSectHighlight = NULL;
	}
}


enum {knDeadWindow = 1};


LRESULT CALLBACK CWindowSection::WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	LPARAM lparam = GetWindowLong( hwnd, WINDOW_DATA );

	if( lparam == knDeadWindow )
	{
		return DefWindowProc( hwnd, message, wParam, lParam );
	}

	CWindowSection *pWnd = reinterpret_cast<CWindowSection*>( lparam );

	switch( message )
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint( hwnd, &ps );
			{
				CRect rcPaint( ps.rcPaint );
				if( rcPaint.IsEmpty() )
				{
					GetClientRect( hwnd, &rcPaint );
				}
				CDrawContext dc( &rcPaint, ps.hdc );
				SelectPalette( ps.hdc, GetCurrentWindowsPalette(), TRUE );
				RealizePalette( ps.hdc );
				pWnd->OnDraw( dc );
			}
			EndPaint( hwnd, &ps );
		}
		break;


	case WM_SETCURSOR:
		{
			if( LOWORD( lParam ) == HTCLIENT )
			{
				CPoint pt;
				GetCursorPos( &pt );
				ScreenToClient( hwnd, &pt );
				if( pWnd->OnSetMouseCursor( pt ) )
					return 0;
			}
		}
		return DefWindowProc( hwnd, message, wParam, lParam );


	case WM_ERASEBKGND:
		return 1;

	//
	//	Crude message reflection
	case WM_COMMAND:
		SendMessage( (HWND) lParam, message, wParam, lParam );
		if( HIWORD( wParam ) == EN_KILLFOCUS )
		{
			if( !IsChild( pWnd->GetHwnd(), ::GetFocus() ) )
				pWnd->DrawNow();
		}
		break;

	case WM_CREATE:
	break;

	case WM_NCCREATE:
		if( pWnd == NULL )
		{
			LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>( lParam );
			SetWindowLong( hwnd, WINDOW_DATA, reinterpret_cast<long>( lpcs->lpCreateParams ) );
		}
		return TRUE;


	case WM_TIMER:
		if( (int)wParam == pWnd->m_nMouseMoveTimerID )
		{
			CPoint pt;
			GetCursorPos( pt );
			HWND hwndCursor = WindowFromPoint( pt );
			if( hwndCursor != pWnd->GetHwnd() )
			{
				CancelHighlight();
				pWnd->UnregisterTimerEvent( pWnd->m_nMouseMoveTimerID );
				pWnd->m_nMouseMoveTimerID = knNullTimerId;
			}
		}
		else
		{
			pWnd->OnTimer( wParam );
		}
		break;


	case WM_SYSKEYDOWN:
		CancelMouseDowns();
		CSectionABC::KillTip();
		return DefWindowProc( hwnd, message, wParam, lParam );


	case WM_CANCELMODE:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_CHAR:
		CancelMouseDowns();
		CSectionABC::KillTip();
		break;

	case WM_MOUSEMOVE:
		{
			CPoint pt( lParam );
			pWnd->OnMouseMove( pt );
		}
		return DefWindowProc( hwnd, message, wParam, lParam );

	case WM_LBUTTONDOWN:
		{
			CSectionABC::KillTip();
			CancelMouseDowns();

			CPoint pt( lParam );
			if( pWnd->m_pCaptureSection )
			{
				pWnd->m_pCaptureSection->OnMouseLeftDown( pt );
			}
			else
			{
				CSectionABC *pSect = pWnd->FindSectionFromPoint( pt );
				if( pSect )
				{
					g_pSectMouseDowned = pSect;
					pWnd->OnMouseLeftDown( pt );
				}
			}
		}
		break;

	case WM_NCHITTEST:
		if( pWnd->m_bCaptionRectSet )
		{
			CPoint pt( LOWORD( lParam ), HIWORD( lParam ) );
			ScreenToClient( hwnd, pt );
			if( pWnd->m_rcCaptionRect.PtInRect( pt ) )
			{
				pWnd->OnMouseMove( pt );
				return HTCAPTION;
			}
		}
	return DefWindowProc( hwnd, message, wParam, lParam );

	case WM_NCLBUTTONDOWN:
		CSectionABC::KillTip();
	return DefWindowProc( hwnd, message, wParam, lParam );

	case WM_LBUTTONUP:
		if( g_pSectMouseDowned )
		{
			CPoint pt( lParam );
			CSectionABC *psect = pWnd->FindSectionFromPoint( pt );

			if( pWnd->m_pCaptureSection )
			{
				pWnd->m_pCaptureSection->OnMouseLeftUp( pt );
			}
			else
			{
				if( psect && psect == g_pSectMouseDowned )
				{
					g_pSectMouseDowned->OnMouseLeftUp( pt );
				}
			}
			CancelMouseDowns();
		}
		break;


	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = reinterpret_cast<LPMINMAXINFO>( lParam );
			lpmmi->ptMinTrackSize.x = 640;
			lpmmi->ptMinTrackSize.y = 480;
		}
		break;

	case WM_SIZE:
		if( wParam != SIZE_MINIMIZED )
		{
			CRect rc( 0, 0, LOWORD( lParam ), HIWORD( lParam ) );
			pWnd->OnLayout( rc );
			InvalidateRect( hwnd, NULL, NULL );
		}
		break;

	case WM_PALETTECHANGED:
		if ((HWND)wParam == hwnd)       // Responding to own message.
			break;									// Nothing to do.

	case WM_QUERYNEWPALETTE:
		{
			HDC hdc = GetDC( hwnd );
			HPALETTE hOldPal = SelectPalette( hdc, GetCurrentWindowsPalette(), FALSE );
			int i = RealizePalette( hdc );							// Realize drawing palette.
			if (i)																	// Did the realization change?
				InvalidateRect(hwnd, NULL, TRUE);			// Yes, so force a repaint.
			SelectPalette( hdc, hOldPal, TRUE);
			RealizePalette( hdc );
			ReleaseDC( hwnd, hdc );
			return(i);
		}

	case WM_NCDESTROY:
		pWnd->OnWindowDestroyed();
		SetWindowLong( hwnd, WINDOW_DATA, knDeadWindow );
	break;

	case WM_DESTROY:
		pWnd->OnDestroy();
		break;

	case WM_MOUSEWHEEL:
		{
			CancelHighlight();
			CancelMouseDowns();
			CSectionABC::KillTip();
			CPoint pt( LOWORD(lParam), HIWORD(lParam) );
			//UINT nFlags = LOWORD( wParam );
			short nDelta = HIWORD( wParam );
			ScreenToClient( hwnd, &pt );
			CSectionABC *psect = pWnd->FindSectionFromPoint( pt );
			if( psect )
				psect->OnMouseWheel( nDelta );
		}
		break;

	default:
		return DefWindowProc( hwnd, message, wParam, lParam );
	}
	return 0;
}


CWindowSection::CWindowSection( WindowType type )
	:	CParentSection( NULL )
	,	m_hwnd( NULL )
	,	m_nWindowType( type )
	, m_nMouseMoveTimerID( knNullTimerId )
	,	m_pCaptureSection( NULL )
	, m_bCaptionRectSet( false )
	, m_nNextTimerID( 0 )
{

}

CWindowSection::~CWindowSection()
{
	//	All timers must have been stopped
	ASSERT( m_mapTimerEvents.GetSize() == 0 );
}


void CWindowSection::GetDesktopRect( CRect &rc )
{
	GetDisplayWorkArea( m_hwnd, rc );
}


bool CWindowSection::RegisterWindow()
{
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof( WNDCLASSEX );
	if( !GetClassInfoEx( g_hQHTMInstance, m_pcszClassName, &wcex ) )
	{
		wcex.style				= CS_BYTEALIGNCLIENT;
		wcex.lpfnWndProc	= (WNDPROC)WndProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= sizeof( CWindowSection *);
		wcex.hInstance		= g_hQHTMInstance;
		wcex.hIcon			= NULL;
		wcex.hIconSm		= NULL;
		wcex.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground	= 0;
		wcex.lpszMenuName	= NULL;
		wcex.lpszClassName	= m_pcszClassName;

		return RegisterClassEx( &wcex ) != 0;
	}
	return true;
}

void CWindowSection::SizeWindow( const CSize &size )
{
	::SetWindowPos( m_hwnd, NULL, 0, 0, size.cx, size.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
}

bool CWindowSection::Create( const CRect &rcPos )
{
	if( !RegisterWindow() )
		return false;

	DWORD dwStyle, dwStyleEx;
	switch( m_nWindowType )
	{
		case APP_WINDOW:
			dwStyle = WS_CLIPCHILDREN | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
			dwStyleEx = WS_EX_APPWINDOW;
		break;

		case TIP_WINDOW:
			dwStyle = WS_POPUP | WS_BORDER;
			dwStyleEx = WS_EX_TOPMOST| WS_EX_TOOLWINDOW;
			//VERIFY( MapWindowPoints( GetDesktopWindow(), g_hwnd, reinterpret_cast<POINT *>( &rcPos ), 2 ) != 0 );
		break;

		default:
			return false;
	}

	m_hwnd = CreateWindowEx( dwStyleEx, m_pcszClassName, _T(""), dwStyle, rcPos.left, rcPos.top, rcPos.Width(), rcPos.Height(), g_hwnd, NULL, g_hQHTMInstance, this );

	if( m_hwnd )
	{
		dwStyle |= WS_MAXIMIZE; 
	}

	return m_hwnd != NULL; 
}


void CWindowSection::ShowWindow( int nCmdShow )
{
	ASSERT_VALID_HWND( m_hwnd );

	if( m_nWindowType == TIP_WINDOW && nCmdShow == SW_SHOW )
	{
		SetWindowPos( m_hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW );
		return;
	}

	(void)::ShowWindow( m_hwnd, nCmdShow );
}


void CWindowSection::OnMouseMove( const CPoint &pt )
{
	if( m_pCaptureSection )
	{
		m_pCaptureSection->OnMouseMove( pt );
		return;
	}

	if( m_nMouseMoveTimerID == knNullTimerId )
		m_nMouseMoveTimerID = RegisterTimerEvent( this, 20 );

	if( m_nWindowType == TIP_WINDOW )
	{
		return;
	}
			

	//
	//	Find the section beneath the point, if there is no section then
	//	we simply use this section. Doing this simplifies the handling and we
	//	may want to do highlighting in this section in the future.
	CSectionABC *pSect = FindSectionFromPoint( pt );

	if( CTipWindow::LastTipCreated() < (DWORD)GetMessageTime() )
	{
		if( CSectionABC::m_pTippedWindow && pSect != CSectionABC::m_pTippedWindow )
		{
			CSectionABC::KillTip();
		}
	}


	if( !pSect )
	{
		pSect = this;
	}

	if( g_pSectHighlight != pSect )
	{
		if( g_pSectHighlight )
			g_pSectHighlight->OnMouseLeave();
		g_pSectHighlight = pSect;
		g_pSectHighlight->OnMouseEnter();
	}

	if( pSect != this )
	{
		pSect->OnMouseMove( pt );
	}
}



void CWindowSection::DestroyWindow()
{
	ASSERT_VALID_HWND( m_hwnd );
	VERIFY( ::DestroyWindow( m_hwnd ) );
}


void CWindowSection::ForceRedraw( const CRect &rc )
{
	if( m_hwnd )
	{
		ASSERT_VALID_HWND( m_hwnd );
		::InvalidateRect( m_hwnd, &rc, TRUE );
	}
}


void CWindowSection::DrawNow()
{
	ForceRedraw( *this );
	UpdateWindow( m_hwnd );
}


void CWindowSection::OnTimer( int nTimerID )
{
	if( nTimerID == m_nMouseMoveTimerID )
	{
		CPoint pt;
		GetCursorPos( pt );
		HWND hwndCursor = WindowFromPoint( pt );
		if( hwndCursor != m_hwnd )
		{
			CancelHighlight();
			UnregisterTimerEvent( m_nMouseMoveTimerID );
			m_nMouseMoveTimerID = knNullTimerId;
		}
	}
	else
	{
		CSectionABC **pSect = m_mapTimerEvents.Lookup( nTimerID );
		if( pSect )
		{
			ASSERT_VALID_WRITEOBJPTR( pSect );
			(*pSect)->OnTimer( nTimerID );
		}
		else
		{
			::KillTimer( GetHwnd(), nTimerID );
		}
	}
}


void CWindowSection::OnWindowDestroyed()
{
	if( m_nWindowType == APP_WINDOW )
	{
		PostQuitMessage( 0 );
	}
};


void CWindowSection::OnDestroy()
{
	if( m_nMouseMoveTimerID != knNullTimerId )
	{
		UnregisterTimerEvent( m_nMouseMoveTimerID );
	}
	m_nMouseMoveTimerID = knNullTimerId;
	m_hwnd = NULL;
}


int CWindowSection::RegisterTimerEvent( CSectionABC *pSect, int nInterval )
{
	ASSERT_VALID_HWND( GetHwnd() );
	ASSERT_VALID_WRITEOBJPTR( pSect );
	ASSERT( nInterval > 0 );

	//TRACE( "Added timer id %d\n", m_nNextTimerID );
	VAPI( ::SetTimer( GetHwnd(), m_nNextTimerID, nInterval, 0 ) );
	m_mapTimerEvents.SetAt( m_nNextTimerID, pSect );
	return m_nNextTimerID++;
}


void CWindowSection::UnregisterTimerEvent( const int nTimerEventID )
{
	//TRACE( "Removed timer id %d\n", nTimerEventID );
	ASSERT( nTimerEventID != knNullTimerId );
	if( m_mapTimerEvents.Lookup( nTimerEventID ) )
	{
		(void)::KillTimer( GetHwnd(), nTimerEventID );
		m_mapTimerEvents.RemoveAt( nTimerEventID );
	}
	else
	{
		//	Trying to unregister an event twice
		ASSERT( FALSE );
	}
}
