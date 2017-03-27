// ProjectionWnd.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "ProjectionWnd.h"
#include "status_colors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProjectionWnd

CProjectionWnd::CProjectionWnd()
{
	m_szOffsets = CSize( 2,2 );

	m_clrBgnd		= CLR_BG;
	m_clrText       = RGB( 50, 150, 200 );
	m_hStaticFont.CreateFont(
		-11					             // H
		, 0   					         // W
		, 0								 // Escapement
		, 0							     // Orient
		, FW_NORMAL	 					 // Wt.  (BOLD)
		, 0								 // Ital.
		, 0             				 // Underl.
		, 0                 			 // SO
		, ANSI_CHARSET		    		 // Charset
		, OUT_TT_PRECIS					 // Out prec.
		, CLIP_DEFAULT_PRECIS			 // Clip prec.
		, PROOF_QUALITY   			     // Qual.
		, VARIABLE_PITCH | FF_DONTCARE   // Pitch and Fam.
		, "Courier New" );
}

/*
==============================
~CProjectionWnd

==============================
*/
CProjectionWnd::~CProjectionWnd()
{
}


BEGIN_MESSAGE_MAP(CProjectionWnd, CWnd)
	//{{AFX_MSG_MAP(CProjectionWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CProjectionWnd message handlers

/*
==============================
OnPaint

==============================
*/
void CProjectionWnd::OnPaint() 
{
	CPaintDC m_hPaintDC(this);

	CRect rcClient;
	GetClientRect(&rcClient);

	BOOL bResult;
	CDC dc;
	bResult = dc.CreateCompatibleDC(&m_hPaintDC);
	if (!bResult)
		return;

	int w = rcClient.Width();
	int h = rcClient.Height();
	CBitmap bm, *oldBm;
	bm.CreateCompatibleBitmap(&m_hPaintDC, w, h);
	oldBm = dc.SelectObject(&bm);

	dc.FillRect(rcClient, &CBrush(m_clrBgnd));
	dc.FrameRect(&rcClient, &CBrush( RGB( 0, 0, 0 ) ) );


	dc.SetTextColor(m_clrText);
	CFont *pOldFont = dc.SelectObject(&m_hStaticFont);

	dc.SetBkMode(TRANSPARENT);

	DRAWTEXTPARAMS hDTP;
	hDTP.cbSize = sizeof(DRAWTEXTPARAMS);
	hDTP.iTabLength   = 4;
	hDTP.iLeftMargin  = 0;
	hDTP.iRightMargin = 0;

	DWORD dwMode;

	dwMode = DT_LEFT;

	rcClient.top += m_szOffsets.cy;
	rcClient.left += m_szOffsets.cx;

	// Draw forground stuff
	CString str;

	GetWindowText( str );

	dc.DrawText( str, &rcClient, dwMode );

	rcClient.top -= m_szOffsets.cy;
	rcClient.left -= m_szOffsets.cx;

	m_hPaintDC.BitBlt(
		rcClient.left,rcClient.top,
		rcClient.Width(),rcClient.Height(),
		&dc,
		0,0,
		SRCCOPY);

	dc.SelectObject( pOldFont );
	dc.SelectObject(oldBm);
	dc.DeleteDC();

	ValidateRect(rcClient);
}

/*
==============================
OnEraseBkgnd

==============================
*/
BOOL CProjectionWnd::OnEraseBkgnd(CDC* pDC) 
{
	OnPaint();
	
	return TRUE;
}

static char projwndclass[] = "PROJWNDCLS";

/*
==============================
Create

==============================
*/
BOOL CProjectionWnd::Create(DWORD dwStyle, CWnd *pParent, UINT id)
{
	CRect rcWndRect(0,0,100,100);

	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));

	wc.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW; 
    wc.lpfnWndProc   = AfxWndProc; 
    wc.hInstance     = AfxGetInstanceHandle();
    wc.hbrBackground = (HBRUSH)CreateSolidBrush( CLR_BG );
    wc.lpszMenuName  = NULL;
	wc.lpszClassName = projwndclass;
	wc.hCursor =       ::LoadCursor( NULL, IDC_ARROW );

	// Register it, exit if it fails    
	if (!AfxRegisterClass(&wc))
	{
		return FALSE;
	}

    if (!CWnd::CreateEx( 0, projwndclass, _T(""), dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_HSCROLL, 
                        rcWndRect.left, rcWndRect.top,
						rcWndRect.Width(), rcWndRect.Height(), // size updated soon
                        pParent->GetSafeHwnd(), (HMENU)id, NULL))
	{
        return FALSE;
	}
	
	return TRUE;
}