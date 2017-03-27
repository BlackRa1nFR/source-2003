/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	QHTM.cpp
Owner:	russf@gipsysoft.com
Purpose:	Quick HTM display control.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "QHTM.h"
#include "QHTMControlSection.h"
#include "utils.h"

#ifndef MSH_MOUSEWHEEL
	#include <ZMouse.h>
#endif	//	MSH_MOUSEWHEEL

//
//	Where the class information is stored in the class data area
#define WINDOW_DATA	0


extern bool FASTCALL RegisterWindow( HINSTANCE hInst );
extern CSectionABC *g_pSectMouseDowned;
extern CSectionABC *g_pSectHighlight;
extern void CancelHighlight();

HINSTANCE g_hQHTMInstance = NULL;
HWND g_hwnd = NULL;

extern void CancelMouseDowns();

BOOL WINAPI QHTM_Initialize( HINSTANCE hInst )
{
//
//	If we are a DLL then this gets set in the DLLMain, however, if we are statically linked
//	then we need to set it to the EXE.
#ifndef QHTM_DLL
	g_hQHTMInstance = hInst;
#endif	//	QHTM_DLL

	if( RegisterWindow( hInst ) )
	{
		return TRUE;
	}

	return FALSE;
}


LRESULT CALLBACK CQHTMControlSection::WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	LPARAM lparam = GetWindowLong( hwnd, WINDOW_DATA );
	CQHTMControlSection *pWnd = reinterpret_cast<CQHTMControlSection*>( lparam );

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

				if( pWnd->IsTransparent() )
				{
					pWnd->OnDraw( dc );
				}
				else
				{
					CBufferedDC dcBuffer( dc );
					pWnd->OnDraw( dcBuffer );
				}
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
				{
					return 0;
				}
			}
		}
		return DefWindowProc( hwnd, message, wParam, lParam );


	case WM_ERASEBKGND:
		if( pWnd->IsTransparent() )
			return DefWindowProc( hwnd, message, wParam, lParam );
		//	richg - 19990224 - Changed from break to return 1; Prevents
		//	redrawing of background, and eliminates flicker.
		return TRUE;
		// break;

	case WM_CREATE:
		{
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
			if ( lpcs->lpszName && *lpcs->lpszName )
				pWnd->SetText( lpcs->lpszName );

			if( lpcs->dwExStyle & WS_EX_TRANSPARENT )
			{
				pWnd->Transparent( true );
			}			
		}
	break;


	case WM_NCCREATE:
		if( pWnd == NULL )
		{
			pWnd = new CQHTMControlSection( hwnd );
			SetWindowLong( hwnd, WINDOW_DATA, reinterpret_cast<long>( pWnd ) );
			SetWindowLong( hwnd, GWL_STYLE, GetWindowLong( hwnd, GWL_STYLE ) | WS_VSCROLL | WS_HSCROLL );
		}
		return TRUE;

	case WM_SETFONT:
		pWnd->SetFont( (HFONT)wParam );
		break;

	case WM_SETTEXT:
		{
			CancelHighlight();
			CancelMouseDowns();
			CSectionABC::KillTip();
			pWnd->SetText( (LPCTSTR)lParam );
		}
		return TRUE;

	case WM_MOUSEMOVE:
		{
			CPoint pt( lParam );
			pWnd->OnMouseMove( pt );
		}
		break;

	case WM_SYSKEYDOWN:
		CancelMouseDowns();
		CSectionABC::KillTip();
		return DefWindowProc( hwnd, message, wParam, lParam );

	case WM_KILLFOCUS:
	case WM_CANCELMODE:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_CHAR:
		CancelMouseDowns();
		CSectionABC::KillTip();
		break;

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


	case WM_NCLBUTTONDOWN:
		CSectionABC::KillTip();
		return DefWindowProc( hwnd, message, wParam, lParam );

	case WM_LBUTTONDOWN:
		{
			CSectionABC::KillTip();
			CancelMouseDowns();

			CPoint pt( lParam );
			g_pSectMouseDowned = pWnd->FindSectionFromPoint( pt );
			pWnd->OnMouseLeftDown( pt );
		}
		break;



	case WM_LBUTTONUP:
		if( g_pSectMouseDowned )
		{
			CPoint pt( lParam );
			CSectionABC *psect = pWnd->FindSectionFromPoint( pt );
			if( psect && psect == g_pSectMouseDowned )
			{
				g_pSectMouseDowned->OnMouseLeftUp( pt );
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
			if( !pWnd->IsLayingOut() )
			{
				CRect rc( 0, 0, LOWORD( lParam ), HIWORD( lParam ) );
				if( rc.Width() && rc.Height() )
				{
					pWnd->OnLayout( rc );
					InvalidateRect( hwnd, NULL, FALSE );
				}
			}
		}
		break;


	case WM_STYLECHANGED:
		if( !pWnd->IsLayingOut() )
		{
			if( wParam == GWL_EXSTYLE )
			{
				LPSTYLESTRUCT pstyles = reinterpret_cast<LPSTYLESTRUCT>( lParam );
				if( pstyles->styleNew  & WS_EX_TRANSPARENT )
				{
					pWnd->Transparent( true );
				}
				else
				{
					pWnd->Transparent( false );
				}
			}

			SetWindowPos( hwnd, NULL, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER );
			pWnd->OnLayout( *pWnd );
			InvalidateRect( hwnd, NULL, FALSE );
		}
		return DefWindowProc( hwnd, message, wParam, lParam );

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
		pWnd->OnDestroy();

		SetWindowLong( hwnd, WINDOW_DATA, NULL );
		delete pWnd;
		break;

	case WM_VSCROLL:
		pWnd->OnVScroll( LOWORD( wParam ) );
		break;

	case WM_HSCROLL:
		pWnd->OnHScroll( LOWORD( wParam ) );
		break;
		
	case QHTM_LOAD_FROM_RESOURCE:
		CancelHighlight();
		CancelMouseDowns();
		CSectionABC::KillTip();
		return pWnd->LoadFromResource( (HINSTANCE)wParam, (LPCTSTR)lParam );

	case QHTM_LOAD_FROM_FILE:
		CancelHighlight();
		CancelMouseDowns();
		CSectionABC::KillTip();
		return pWnd->LoadFromFile( (LPCTSTR)lParam );

	case QHTM_GET_OPTION:
		return pWnd->GetOption( wParam, lParam );

	case QHTM_SET_OPTION:
		return pWnd->SetOption( wParam, lParam );

	case QHTM_GOTO_LINK:
		pWnd->GotoLink( (LPCTSTR)lParam );
		break;

	case QHTM_GET_HTML_TITLE_LENGTH:
		return pWnd->GetTitleLength();

	case QHTM_GET_HTML_TITLE:
		return pWnd->GetTitle( wParam, (LPTSTR)lParam );

	case QHTM_GET_DRAWN_SIZE:
		{
			LPSIZE lpSize = reinterpret_cast<LPSIZE>( lParam );
			if( !::IsBadReadPtr( lpSize, sizeof( SIZE ) ) )
			{
				CSize size( pWnd->GetSize() );
				lpSize->cx = size.cx;
				lpSize->cy = size.cy;
				return TRUE;
			}
			return FALSE;
		}
		break;

	case QHTM_GET_SCROLL_POS:
		return pWnd->GetScrollPos();

	case QHTM_SET_SCROLL_POS:
		pWnd->SetScrollPos( wParam );
		break;

	default:
		return DefWindowProc( hwnd, message, wParam, lParam );
	}
	return 0;
}
