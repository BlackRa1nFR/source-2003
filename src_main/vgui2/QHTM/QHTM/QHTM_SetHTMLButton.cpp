/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	QHTM_SetHTMLButton.cpp
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "qhtm.h"
#include "QHTM.h"
#include "QHTM_Includes.h"
#include "Defaults.h"
#include "HtmlSection.h"

#define QHTM_SUBCLASSED_BUTTON_WNDPROC_PROP	_T("QHTM_OldWindowProc")
#define QHTM_SUBCLASSED_BUTTON_QHTM_PROP	_T("QHTM_QHTM_Window")
#define QHTM_SUBCLASSED_QHTM_WNDPROC_PROP	_T("QHTM_WindowSubClass")


static void SizeHTML( HWND hwndButton, HWND hwndQHTM )
{
	bool bButtonPushed = SendMessage( hwndButton, BM_GETSTATE, 0, 0 ) & BST_PUSHED ? true : false;
	CRect rc, rcButton;
	GetWindowRect( hwndButton, &rcButton );
	rc = rcButton;

	const int nVerticalBorder = GetSystemMetrics( SM_CXEDGE );
	const int nHorizontalBorder = GetSystemMetrics( SM_CYEDGE );
	//	REVIEW - russf - need to find a better way of figuring out where to place the control
	rc.bottom -= ( nVerticalBorder + 3 );
	rc.top += (nVerticalBorder + 3);
	rc.left += (nHorizontalBorder + 4 );
	rc.right -= ( nHorizontalBorder + 5 );
	if( bButtonPushed )
	{
		rc.top++;
		rc.left++;
		rc.right++;
	}
	MapWindowPoints( HWND_DESKTOP, hwndButton, reinterpret_cast<LPPOINT>(&rc.left), 2 );
	SetWindowPos( hwndQHTM, NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOZORDER );
}


LONG FAR PASCAL QHTMSubClassFunc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
	WNDPROC old = (WNDPROC)GetProp( hwnd, QHTM_SUBCLASSED_QHTM_WNDPROC_PROP );
	if( msg == WM_NCHITTEST )
		return HTTRANSPARENT;

	return CallWindowProc( old, hwnd, msg, wParam, lParam );	
}


LONG FAR PASCAL QHTM_BUTTON_SubClassFunc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
	LRESULT lr = FALSE;
	WNDPROC old = (WNDPROC)GetProp( hwnd, QHTM_SUBCLASSED_BUTTON_WNDPROC_PROP );
	switch( msg )
	{
	case WM_CTLCOLORSTATIC:
		return SendMessage( GetParent( hwnd ), msg, wParam, lParam );

	case WM_SETTEXT:
		{
			HWND hwndQHTM = (HWND)GetProp( hwnd, QHTM_SUBCLASSED_BUTTON_QHTM_PROP );
			SendMessage( hwndQHTM, WM_SETTEXT, wParam, lParam );
		}
		break;

	case WM_LBUTTONDOWN:
		lr = CallWindowProc( old, hwnd, msg, wParam, lParam );
		{
			HWND hwndQHTM = (HWND)GetProp( hwnd, QHTM_SUBCLASSED_BUTTON_QHTM_PROP );
			SizeHTML( hwnd, hwndQHTM );
		}
		break;		

	case WM_MOUSEMOVE:
		if( GetCapture() == hwnd )
		{
			HWND hwndQHTM = (HWND)GetProp( hwnd, QHTM_SUBCLASSED_BUTTON_QHTM_PROP );
			SizeHTML( hwnd, hwndQHTM );
		}
		lr = CallWindowProc( old, hwnd, msg, wParam, lParam );
		break;

	case WM_SIZE:
		lr = CallWindowProc( old, hwnd, msg, wParam, lParam );
		{
			HWND hwndQHTM = (HWND)GetProp( hwnd, QHTM_SUBCLASSED_BUTTON_QHTM_PROP );
			SizeHTML( hwnd, hwndQHTM );
		}
		break;

	default:
		lr = CallWindowProc( old, hwnd, msg, wParam, lParam );
	};

	return lr;
}


extern "C" BOOL WINAPI QHTM_SetHTMLButton( HWND hwndButton )
{
#ifdef QHTM_ALLOW_HTML_BUTTON
	UINT uTextSize = ::GetWindowTextLength( hwndButton );
	LPTSTR pszText = new TCHAR [ uTextSize + 1 ];
	::GetWindowText( hwndButton, pszText, uTextSize + 1 );
	SetWindowText( hwndButton, _T(" ") );

	LONG l = ::GetWindowLong( hwndButton, GWL_WNDPROC );
	::SetProp( hwndButton, QHTM_SUBCLASSED_BUTTON_WNDPROC_PROP, (HANDLE)l );
	::SetWindowLong( hwndButton, GWL_WNDPROC, (LONG)QHTM_BUTTON_SubClassFunc );
	::SetWindowLong( hwndButton, GWL_STYLE, WS_CLIPCHILDREN | ::GetWindowLong( hwndButton, GWL_STYLE ) );
	

	HFONT hFont = (HFONT)::SendMessage( hwndButton, WM_GETFONT, 0, 0 );

	HWND hwndQHTM = CreateWindowEx( 0, QHTM_CLASSNAME, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwndButton, NULL, (HINSTANCE)GetWindowLong( hwndButton, GWL_HINSTANCE ), NULL );
	SetProp( hwndButton, QHTM_SUBCLASSED_BUTTON_QHTM_PROP, (HANDLE)hwndQHTM );

	QHTM_EnableScrollbars( hwndQHTM, FALSE );
	CRect rcMargins( 0, 0, 0, 0 );
	QHTM_SetMargins( hwndQHTM, rcMargins );
	SizeHTML( hwndButton, hwndQHTM );

	SendMessage( hwndQHTM, WM_SETFONT, (WPARAM)hFont, 0 );
	SetWindowText( hwndQHTM, pszText );
	
	l = ::GetWindowLong( hwndQHTM, GWL_WNDPROC );
	::SetProp( hwndQHTM, QHTM_SUBCLASSED_QHTM_WNDPROC_PROP, (HANDLE)l );
	::SetWindowLong( hwndQHTM, GWL_WNDPROC, (LONG)QHTMSubClassFunc );

	delete pszText;
	pszText = NULL;
	return TRUE;

#else		//	QHTM_ALLOW_HTML_BUTTON
	UNREFERENCED_PARAMETER( hwndButton );
	return FALSE;
#endif	//	QHTM_ALLOW_HTML_BUTTON
}