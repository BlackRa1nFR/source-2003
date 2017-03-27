#include "stdafx.h"
#include "ODStatic.h"
#include "status_colors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////
// CODStatic Implementation

/////////////////////////////////////////////////////////////////////////////
// CODStatic

CODStatic::CODStatic()
{
	strText.Empty();

	m_szOffsets = CSize(0,0);

	m_bCenterText = FALSE;
	m_clrBgnd = CLR_BG;
	m_bTransparent = TRUE;
	m_clrText         = RGB( 200 , 50, 50 );
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
		, "Arial" );	
}

CODStatic::~CODStatic()
{
}


BEGIN_MESSAGE_MAP(CODStatic, CStatic)
	//{{AFX_MSG_MAP(CODStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_DRAWITEM()
	ON_WM_NCPAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODStatic message handlers

void CODStatic::OnPaint() 
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

	dc.SetTextColor(m_clrText);
	CFont *pOldFont = dc.SelectObject(&m_hStaticFont);

	dc.SetBkMode(TRANSPARENT);

	DRAWTEXTPARAMS hDTP;
	hDTP.cbSize = sizeof(DRAWTEXTPARAMS);
	hDTP.iTabLength   = 4;
	hDTP.iLeftMargin  = 0;
	hDTP.iRightMargin = 0;

	DWORD dwMode;

	if (m_bCenterText)
		dwMode = DT_CENTER;
	else
		dwMode = DT_LEFT;

	rcClient.top += m_szOffsets.cy;
	rcClient.left += m_szOffsets.cx;

	::DrawTextEx(dc.GetSafeHdc(), (char *)(LPCSTR)strText, -1, rcClient, dwMode | DT_VCENTER | DT_WORDBREAK, &hDTP);

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

BOOL CODStatic::OnEraseBkgnd(CDC* pDC) 
{
	OnPaint();

	return TRUE;
}

void CODStatic::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
}

void CODStatic::SetWindowText(LPCTSTR lpszString )
{
	CStatic::SetWindowText( "" );
	strText = lpszString;

	InvalidateRect(NULL);
	UpdateWindow();
}

void CODStatic::OnNcPaint() 
{
	UpdateWindow();
}

void CODStatic::ForcePaint()
{
	InvalidateRect(NULL);
	UpdateWindow();
}

void CODStatic::SetTransparent(BOOL bTransparent)
{
	m_bTransparent = bTransparent;
}

void CODStatic::SetTextColor(COLORREF clr)
{
	m_clrText = clr;
}

void CODStatic::SetBkColor(COLORREF clr)
{
	m_clrBgnd = clr;
}

void CODStatic::SetFontSize(int nSize, int nWeight /* = FW_NORMAL */)
{
	m_hStaticFont.DeleteObject();
	m_hStaticFont.CreateFont(
		-nSize				             // H
		, 0   					         // W
		, 0								 // Escapement
		, 0							     // Orient
		, nWeight	 					 // Wt.  (BOLD)
		, 0								 // Ital.
		, 0             				 // Underl.
		, 0                 			 // SO
		, ANSI_CHARSET		    		 // Charset
		, OUT_TT_PRECIS					 // Out prec.
		, CLIP_DEFAULT_PRECIS			 // Clip prec.
		, PROOF_QUALITY   			     // Qual.
		, VARIABLE_PITCH | FF_DONTCARE   // Pitch and Fam.
		, "Arial" );	
}

void CODStatic::SetCentered(BOOL bCenter)
{
	m_bCenterText = TRUE;
}

void CODStatic::SetOffsets(int cx, int cy)
{
	m_szOffsets = CSize( cx, cy );
}

void CODStatic::SetWindowText( CString& strNewText )
{
	CStatic::SetWindowText( "" );
	strText = strNewText;

	InvalidateRect(NULL);
	UpdateWindow();	
}
