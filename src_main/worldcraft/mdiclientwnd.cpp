//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "MDIClientWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CMDIClientWnd, CWnd)
	//{{AFX_MSG_MAP(CMDIClientWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CMDIClientWnd::CMDIClientWnd()
{
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CMDIClientWnd::~CMDIClientWnd()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lpCreateStruct - 
//-----------------------------------------------------------------------------
int CMDIClientWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
//	m_CenterBitmap.LoadBitmap(IDB_SPLASH);
	return CWnd::OnCreate(lpCreateStruct);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMDIClientWnd::OnPaint()
{
	CPaintDC dc(this);

//	CDC dcImage;
//	if (!dcImage.CreateCompatibleDC(&dc))
//	{
//		return;
//	}
//
//	BITMAP bm;
//	m_CenterBitmap.GetBitmap(&bm);
//
//	CRect rect;
//	GetClientRect(rect);
//
//	// Paint the image.
//	CBitmap *pOldBitmap = dcImage.SelectObject(&m_bitmap);
//	dc.BitBlt(rect.left, rect.top, bm.bmWidth, bm.bmHeight, &dcImage, 0, 0, SRCCOPY);
//	dcImage.SelectObject(pOldBitmap);
}


//-----------------------------------------------------------------------------
// Purpose: Makes our background color mesh with the splash screen for maximum effect.
//-----------------------------------------------------------------------------
BOOL CMDIClientWnd::OnEraseBkgnd(CDC *pDC)
{
	// Set brush to desired background color
	CBrush backBrush(RGB(141, 136, 130)); // This color blends with the splash image!

	// Save old brush
	CBrush *pOldBrush = pDC->SelectObject(&backBrush);

	CRect rect;
	pDC->GetClipBox(&rect);     // Erase the area needed

	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);

	pDC->SelectObject(pOldBrush);
	return TRUE;
} 

