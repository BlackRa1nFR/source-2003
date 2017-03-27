// StatusWindow.cpp : implementation file
//

#include "stdafx.h"
#include <afxcmn.h>
#include "status.h"
#include "StatusWindow.h"
#include "status_colors.h"
#include "StatusDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

double Sys_DoubleTime( void );

CStatusWindow::CData::CData()
{
	clr = RGB( 0, 0, 0 );
	linetext[ 0 ] = 0;
	starttime = 0.0f;
}

CStatusWindow::CData::~CData()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CStatusWindow::CData::GetStartTime( void ) const
{
	return starttime;
}

void CStatusWindow::CData::SetText( COLORREF col, char const *text )
{
	linetext[ 0 ] = 0;
	if ( text )
	{
		strcpy( linetext, text );
	}

	clr = col;

	starttime = (float)Sys_DoubleTime();
}


char const *CStatusWindow::CData::GetText( void ) const
{
	return linetext;
}

COLORREF CStatusWindow::CData::GetColor( void ) const
{
	return clr;
}

/////////////////////////////////////////////////////////////////////////////
// CStatusWindow

CStatusWindow::CStatusWindow( CStatusDlg* pParent )
{
	m_pStatus = pParent;

	m_nFontHeight = 11;

	m_hSmallFont.CreateFont(
		 -m_nFontHeight	             // H
		, 0   					         // W
		, 0								 // Escapement
		, 0							     // Orient
		, FW_DONTCARE					 // Wt.  (BOLD)
		, 0								 // Ital.
		, 0             				 // Underl.
		, 0                 			 // SO
		, DEFAULT_CHARSET 	    		 // Charset
		, OUT_DEFAULT_PRECIS				 // Out prec.
		, CLIP_DEFAULT_PRECIS			 // Clip prec.
		, ANTIALIASED_QUALITY   			     // Qual.
		, DEFAULT_PITCH  | FF_DONTCARE   // Pitch and Fam.
		, "Courier New" );
}

CStatusWindow::~CStatusWindow()
{
	m_Text.RemoveAll();
}

static char statuswndclass[] = "STATUSWNDCLS";
BOOL CStatusWindow::Create(DWORD dwStyle )
{
	CRect rcWndRect(0,0,100,100);

	int id = 1000;

	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));

	wc.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW; 
    wc.lpfnWndProc   = AfxWndProc; 
    wc.hInstance     = AfxGetInstanceHandle();
    wc.hbrBackground = (HBRUSH)CreateSolidBrush( CLR_BG );
    wc.lpszMenuName  = NULL;
	wc.lpszClassName = statuswndclass;
	wc.hCursor =       ::LoadCursor( NULL, IDC_ARROW );

	// Register it, exit if it fails    
	if (!AfxRegisterClass(&wc))
	{
		return FALSE;
	}

    if (!CWnd::CreateEx( 0, statuswndclass, _T(""), dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL, 
                        rcWndRect.left, rcWndRect.top,
						rcWndRect.Width(), rcWndRect.Height(), // size updated soon
                        m_pStatus->GetSafeHwnd(), (HMENU)id, NULL))
	{
        return FALSE;
	}

	::SetScrollRange( GetSafeHwnd(), SB_VERT, 0, 100, TRUE );
	::SetScrollPos( GetSafeHwnd(), SB_VERT, 100, TRUE );

	return TRUE;
}

BEGIN_MESSAGE_MAP(CStatusWindow, CWnd)
	//{{AFX_MSG_MAP(CStatusWindow)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStatusWindow message handlers

void CStatusWindow::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CWnd::OnPaint() for painting messages
	RemoveOldStrings();

	RECT rc;
	GetClientRect( &rc );

	dc.SelectObject( m_hSmallFont );
	dc.SetBkMode( TRANSPARENT );

	int lineheight = m_nFontHeight + 2;

	int y = rc.bottom - lineheight;

	int item = m_Text.Count() - 1;

	int pos = ::GetScrollPos( GetSafeHwnd(), SB_VERT );

	if ( m_Text.Count() > 0 )
	{
		float frac = ( float ) pos / 100.0f;
		frac = 1.0f - frac;

		int backup = ( int )( m_Text.Count() * frac );
		int savelines = ( rc.bottom - rc.top ) / lineheight;
		backup -= savelines;
		backup = max( 0, backup );

		item -= backup;
	}

	while ( item >= 0 &&
		y + lineheight >= 0 )
	{
		
		CData *d = &m_Text[ item ];

		rc.top = y;
		rc.bottom = y + lineheight;

		dc.SetTextColor( d->GetColor() );
		dc.DrawText( d->GetText(), -1, &rc, DT_NOPREFIX | DT_LEFT | DT_NOCLIP | DT_SINGLELINE );

		item--;
		y -= lineheight;
	}
}

void CStatusWindow::Printf( const char *fmt, ... )
{
	COLORREF clr;

	clr = RGB( 0, 0, 0 );

	char string[ 4096 ];
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( string, fmt, argptr );
	va_end( argptr );

	ColorPrintf( clr, string );
}

void CStatusWindow::ColorPrintf(COLORREF rgb, const char *fmt, ...)
{
	char string2[ 4096 ];
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( string2, fmt, argptr );
	va_end( argptr );

	char string[ 4096 ];

	RemoveOldStrings();

	if ( !GetSafeHwnd() )
	{
		OutputDebugString( string2 );
		return;
	}

	if ( strlen( string2 ) <= 0 )
	{
		::SetScrollPos( GetSafeHwnd(), SB_VERT, 100, FALSE );
		Invalidate();
		return;
	}

	if ( string2[ 0 ] == '\n' )
	{
		ColorPrintf( rgb, string + 1 );
		return;
	}

	char *p = string2;
	while ( *p && *p != '\n' )
	{
		p++;
	}

	if ( !*p )
	{
		CData *d = &m_Text[ m_Text.AddToTail() ];
		// Prepend date and time
		sprintf( string, "%s:  %s", m_pStatus->GetTimeAsString(), string2 );
		d->SetText( rgb, string );
	}
	else
	{
		char t = *p;
		*p = 0;

		CData *d = &m_Text[ m_Text.AddToTail() ];
		// Prepend date and time
		sprintf( string, "%s:  %s", m_pStatus->GetTimeAsString(), string2 );
		d->SetText( rgb, string );

		*p = t;

		ColorPrintf( rgb, p + 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStatusWindow::RemoveOldStrings( void )
{
	// Only if count gets too big
	if ( m_Text.Count() < 128 )
		return;

	// one minute lifespan
	float killtime = (float)Sys_DoubleTime() - 60.0f;

	for ( int i = m_Text.Count() - 1; i >= 0; i-- )
	{
		CData *d = &m_Text[ i ];
		if ( d->GetStartTime() <= killtime )
		{
			m_Text.Remove( i );
		}
	}
}

void CStatusWindow::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	int nCurrent;

	nCurrent = ::GetScrollPos( GetSafeHwnd(), SB_VERT );

	switch ( nSBCode )
	{
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		::SetScrollPos( GetSafeHwnd(), SB_VERT, nPos, TRUE );
		InvalidateRect( NULL );
		UpdateWindow();
		return;
	case SB_PAGEUP:
		nCurrent -= 10;
		break;
	case SB_PAGEDOWN:
		nCurrent += 10;
		break;
	case SB_LINEUP:
		nCurrent -= 1;
		break;
	case SB_LINEDOWN:
		nCurrent += 1;
		break;
	default:
		return;
	}

	nCurrent = max( 0, nCurrent );
	nCurrent = min( 100, nCurrent );

	::SetScrollPos( GetSafeHwnd(), SB_VERT, nCurrent, TRUE );

	InvalidateRect( NULL );
	UpdateWindow();
}

BOOL CStatusWindow::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	// TODO: Add your message handler code here and/or call default
	int nCurrent;
	nCurrent = ::GetScrollPos( GetSafeHwnd(), SB_VERT );

	if ( zDelta > 0 )
		nCurrent+=10;
	else
		nCurrent-=10;

	nCurrent = max( 0, nCurrent );
	nCurrent = min( 100, nCurrent );

	::SetScrollPos( GetSafeHwnd(), SB_VERT, nCurrent, TRUE );

	InvalidateRect( NULL );
	UpdateWindow();
	return TRUE;
}

void CStatusWindow::OnLButtonDown(UINT nFlags, CPoint point) 
{
	SetFocus();
}
