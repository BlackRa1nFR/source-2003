/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	EnableCoolTips.cpp
Owner:	russf@gipsysoft.com
Purpose:	Cool tips code.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "QHTM.h"
#include "QHTM_Includes.h"
#include <Commctrl.h>
#include "Defaults.h"
#include "HtmlSection.h"
#include <WindowText.h>
#include "WindowSection.h"

#ifdef QHTM_COOLTIPS

extern void GetDisplayWorkArea( HWND hwnd, RECT &rc );

LONG FAR PASCAL SubClassFunc(HWND hWnd,WORD Message,WORD wParam, LONG lParam);

WNDPROC g_lpfnOldWndProc = NULL;

static LPCTSTR g_pcszProperty = _T("CoolTipPrivate");
static const int g_knTipClearanceX = 4, g_knTipClearanceY = 22;

static HWND CreateTip()
{
  return CreateWindow(TOOLTIPS_CLASS
			, (LPTSTR) NULL
			, TTS_ALWAYSTIP
			, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT
			, NULL
			, (HMENU) NULL
			, g_hQHTMInstance
			, NULL); 
}

#endif	//QHTM_COOLTIPS

BOOL WINAPI QHTM_EnableCooltips()
{
	BOOL bRetVal = FALSE;
#ifdef QHTM_COOLTIPS
	HWND hwndTip = CreateTip();
	if( hwndTip )
	{
		g_lpfnOldWndProc = (WNDPROC)SetClassLong(hwndTip, GCL_WNDPROC, (DWORD) SubClassFunc);
		if( DestroyWindow( hwndTip ) )
		{
			bRetVal = TRUE;
		}
	}
#endif	//	QHTM_COOLTIPS
	return bRetVal;
}


void StopSublassing()
{
#ifdef QHTM_COOLTIPS
	if( g_lpfnOldWndProc )
	{
		HWND hwndTip = CreateTip();
		SetClassLong(hwndTip, GCL_WNDPROC, (DWORD) g_lpfnOldWndProc);
		DestroyWindow( hwndTip );
	}
#endif	//	QHTM_COOLTIPS
}

#ifdef QHTM_COOLTIPS

struct Property
{
	Property()
		: m_defaults( g_defaults )
		, m_sectHTML( NULL, &m_defaults )
		{
			m_defaults.m_crBackground = GetSysColor( COLOR_INFOBK );
			m_defaults.m_crDefaultForeColour = GetSysColor( COLOR_INFOTEXT );
		}
	//	m_defaults MUST be first;
	CDefaults m_defaults;
	CHTMLSection m_sectHTML;
private:
	Property( const Property& );
	Property & operator = ( const Property& );
};


LONG FAR PASCAL SubClassFunc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
	switch( msg )
	{
		case WM_CREATE:
			{
				Property *pProp = new Property;
				pProp->m_defaults.m_rcMargins.Set( 2, 2, 5, 5 );
				SetProp( hwnd, g_pcszProperty, reinterpret_cast<HANDLE>( pProp ) );
			}
			break;


		case WM_DESTROY:
			{
				Property *pProp = reinterpret_cast<Property *>( RemoveProp( hwnd, g_pcszProperty ) );
				delete pProp;
			}
			break;

		case WM_WINDOWPOSCHANGING:
			{
				WINDOWPOS &pos = *(WINDOWPOS *)lParam;
				Property *pProp = reinterpret_cast<Property *>( GetProp( hwnd, g_pcszProperty ) );
				if( !(pos.flags & SWP_NOSIZE) && !( pos.flags & SWP_NOMOVE ) && pProp )
				{
					CWindowText text( hwnd );
					if( text.GetLength() )
					{
						HFONT hFont = (HFONT)SendMessage( hwnd, WM_GETFONT, 0, 0 );
						pProp->m_defaults.SetFont( hFont );
						pProp->m_sectHTML.SetHTML( text, text.GetLength(), NULL );
					}
					return 0;
				}
			}
		break;

		case WM_WINDOWPOSCHANGED:
		{
			static bool bDealingWithIt = false;
			Property *pProp = reinterpret_cast<Property *>( GetProp( hwnd, g_pcszProperty ) );
			if( pProp && !bDealingWithIt )
			{
				bDealingWithIt = true;

				CRect rcTip( 0, 0, 300, 5 );
				CDrawContext dc;
				pProp->m_sectHTML.OnLayout( rcTip, dc );
				const CSize size( pProp->m_sectHTML.GetSize() );
				rcTip.SetSize( size );
				pProp->m_sectHTML.bottom = size.cy;
				pProp->m_sectHTML.right = size.cx;

				CPoint ptCursor;
				GetCursorPos( ptCursor );
				ptCursor.x += g_knTipClearanceX;
				ptCursor.y += g_knTipClearanceY;
				rcTip.Offset( ptCursor.x, ptCursor.y );

				CRect rcDesktop;
				GetDisplayWorkArea( hwnd, rcDesktop );
				if( rcTip.right >= rcDesktop.right )
				{
					rcTip.Offset( -(rcTip.right - rcDesktop.right) - g_knTipClearanceX, 0 );
				}

				if( rcTip.bottom >= rcDesktop.bottom )
				{
					rcTip.Offset( 0, -rcTip.Height() - g_knTipClearanceY );
				}
				SetWindowPos( hwnd, NULL, rcTip.left, rcTip.top, rcTip.Width(), rcTip.Height(), SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOZORDER );
				bDealingWithIt = false;
			}
			return 0;
		}

		case WM_PAINT:
		{
			Property *pProp = reinterpret_cast<Property *>( GetProp( hwnd, g_pcszProperty ) );
			if( pProp )
			{
				PAINTSTRUCT ps;
				BeginPaint( hwnd, &ps );

				CRect rcPaint( ps.rcPaint );
				if( rcPaint.IsEmpty() )
				{
					GetClientRect( hwnd, &rcPaint );
				}
				//
				//	Scoped to allow the draw context to go out of scope before EndPaint is called.
				{
					CDrawContext dc( &rcPaint, ps.hdc );
					SelectPalette( ps.hdc, GetCurrentWindowsPalette(), TRUE );
					RealizePalette( ps.hdc );
					pProp->m_sectHTML.OnDraw( dc );
				}
				EndPaint( hwnd, &ps );
				return 0;
			}
			break;
		}
	}

	return CallWindowProc( g_lpfnOldWndProc, hwnd, msg, wParam, lParam);
}

#endif	//	QHTM_COOLTIPS