//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "MapEntity.h"
#include "MapFace.h"
#include "MapSolid.h"
#include "MapWorld.h"
#include "MapDoc.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "tooldefs.h"
#include "StockSolids.h"
#include "statusbarids.h"
#include "GameData.h"
#include "ObjectProperties.h"
#include "Options.h"
#include "History.h"
#include "GlobalFunctions.h"
#include "MapDefs.h"		// dvs: For COORD_NOTINIT
#include "Render2D.h"
#include "TitleWnd.h"
#include "ToolManager.h"
#include "ToolMorph.h"		// FIXME: remove
#include "ToolInterface.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#pragma warning( disable: 4244 )


#define _Snap(n) ((((n) / pDoc->GetGridSpacing()) * pDoc->GetGridSpacing()))


#define ZOOM_MIN		0.03125
#define ZOOM_MAX		256.0


static DrawType_t __eNextViewType = VIEW2D_XY;


HCURSOR CMapView2D::m_hcurHand = NULL;
HCURSOR CMapView2D::m_hcurHandClosed = NULL;


struct RenderList_t
{
	CUtlVector<CMapClass *> NormalObjects;
	CUtlVector<CMapClass *> SelectedObjects;
};


static RenderList_t g_RenderList;


IMPLEMENT_DYNCREATE(CMapView2D, CMapView)


BEGIN_MESSAGE_MAP(CMapView2D, CMapView)
	//{{AFX_MSG_MAP(CMapView2D)
	ON_COMMAND(ID_VIEW_2DXY, OnView2dxy)
	ON_COMMAND(ID_VIEW_2DYZ, OnView2dyz)
	ON_COMMAND(ID_VIEW_2DXZ, OnView2dxz)
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONUP()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_RBUTTONDOWN()
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	ON_COMMAND(ID_EDIT_PROPERTIES, OnEditProperties)
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_RBUTTONUP()
	ON_UPDATE_COMMAND_UI(ID_CREATEOBJECT, OnUpdateEditFunction)
	ON_WM_ERASEBKGND()
	ON_UPDATE_COMMAND_UI(ID_EDIT_PROPERTIES, OnUpdateEditFunction)
	ON_WM_PAINT()
	ON_COMMAND_EX(ID_TOOLS_ALIGNTOP, OnToolsAlign)
	ON_COMMAND_EX(ID_TOOLS_ALIGNBOTTOM, OnToolsAlign)
	ON_COMMAND_EX(ID_TOOLS_ALIGNLEFT, OnToolsAlign)
	ON_COMMAND_EX(ID_TOOLS_ALIGNRIGHT, OnToolsAlign)
	ON_COMMAND_EX(ID_FLIP_HORIZONTAL, OnFlip)
	ON_COMMAND_EX(ID_FLIP_VERTICAL, OnFlip)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_ALIGNTOP, OnUpdateEditFunction)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_ALIGNBOTTOM, OnUpdateEditFunction)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_ALIGNLEFT, OnUpdateEditFunction)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_ALIGNRIGHT, OnUpdateEditFunction)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Allows for iteration of draw types in order.
// Input  : eDrawType - Current draw type.
// Output : Returns the next draw type in the list: XY, YZ, XZ. List wraps.
//-----------------------------------------------------------------------------
static DrawType_t NextDrawType(DrawType_t eDrawType)
{
	if (eDrawType == VIEW2D_XY)
	{
		return(VIEW2D_YZ);
	}

	if (eDrawType == VIEW2D_YZ)
	{
		return(VIEW2D_XZ);
	}

	return(VIEW2D_XY);
}


//-----------------------------------------------------------------------------
// Purpose: Allows for iteration of draw types in reverse order.
// Input  : eDrawType - Current draw type.
// Output : Returns the previous draw type in the list: XY, YZ, XZ. List wraps.
//-----------------------------------------------------------------------------
static DrawType_t PrevDrawType(DrawType_t eDrawType)
{
	if (eDrawType == VIEW2D_XY)
	{
		return(VIEW2D_XZ);
	}

	if (eDrawType == VIEW2D_YZ)
	{
		return(VIEW2D_XY);
	}

	return(VIEW2D_YZ);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes data members.
//	---------------------------------------------------------------------------
CMapView2D::CMapView2D(void)
{
	//
	// Must initialize the title window pointer before calling SetDrawType!
	//
	m_pwndTitle = NULL;

	//
	// Create next 2d view type.
	//
	__eNextViewType = NextDrawType(__eNextViewType);
	SetDrawType(__eNextViewType);

	m_fZoom = -1;	// make sure setzoom performs

	SetZoom(0.25);

	m_xScroll = m_yScroll = 0;
	m_bActive = FALSE;
	m_rectScreen.SetRect(0, 0, 0, 0);
	m_MouseDrag = MD_NOTHING;
	m_bLastActiveView = FALSE;
	m_bToolShown = FALSE;

	m_rgnUpdate.CreateRectRgnIndirect(CRect(0, 0, 10, 10));	// dummy init

	// static members - 
	if(!m_hcurHand)
	{
		m_hcurHand = LoadCursor(AfxGetInstanceHandle(),
			MAKEINTRESOURCE(IDC_HANDCURSOR));
		m_hcurHandClosed = LoadCursor(AfxGetInstanceHandle(),
			MAKEINTRESOURCE(IDC_HANDCLOSEDCURSOR));
	}

	m_hSelectedVisibles = new CMapClass*[MAX_VISIBLE_OBJECTS];
	if (!m_hSelectedVisibles)
	{
		AfxMessageBox("Could not allocate visible object buffer!");
		exit(-1);
	}
	memset(m_hSelectedVisibles, 0, MAX_VISIBLE_OBJECTS * sizeof(CMapClass *));
	m_nSelectedVisibles = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees dynamically allocated resources.
//-----------------------------------------------------------------------------
CMapView2D::~CMapView2D(void)
{
	if (GetDocument())
	{
		GetDocument()->RemoveMRU(this);
	}
	
	BOOL bSuccess = m_bmpDraw.DeleteObject();
	bSuccess      = m_bmpTool.DeleteObject();

	if (m_hSelectedVisibles)
	{
		delete[] m_hSelectedVisibles;
	}

	if (m_pwndTitle != NULL)
	{
		delete m_pwndTitle;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bActivate - 
//-----------------------------------------------------------------------------
void CMapView2D::Activate(BOOL bActivate)
{
	BOOL bActive = IsActive();

	CMapView::Activate(bActivate);
	if (bActivate != bActive)
	{
		Invalidate();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Convert client point to world coordinates.
// Input  : point - 
//-----------------------------------------------------------------------------
void CMapView2D::ClientToWorld(CPoint &point)
{
	point.x += GetScrollPos(SB_HORZ);
	point.y += GetScrollPos(SB_VERT);

	point.x /= m_fZoom;
	point.y /= m_fZoom;

	if (bInvertHorz)
	{
		point.x = -point.x;
	}

	if (bInvertVert)
	{
		point.y = -point.y;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Converts a 2D client coordinate into 3D world coordinates.
// Input  : vecWorld - 
//			ptClient - 
//-----------------------------------------------------------------------------
void CMapView2D::ClientToWorld(Vector &vecWorld, const CPoint &ptClient)
{
	vecWorld[axHorz] = ptClient.x;
	vecWorld[axVert] = ptClient.y;
	vecWorld[axThird] = 0;

	vecWorld[axHorz] += GetScrollPos(SB_HORZ);
	vecWorld[axVert] += GetScrollPos(SB_VERT);

	vecWorld[axHorz] /= m_fZoom;
	vecWorld[axVert] /= m_fZoom;

	if (bInvertHorz)
	{
		vecWorld[axHorz] = -vecWorld[axHorz];
	}

	if (bInvertVert)
	{
		vecWorld[axVert] = -vecWorld[axVert];
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView2D::UpdateTitleWindowPos(void)
{
	if (m_pwndTitle != NULL)
	{
		if (!::IsWindow(m_pwndTitle->m_hWnd))
		{
			return;
		}

		CRect r;
		GetClientRect(&r);
		m_pwndTitle->SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		m_pwndTitle->ShowWindow(SW_SHOW);
	}
}


//-----------------------------------------------------------------------------
// Purpose: First-time initialization of this view.
//-----------------------------------------------------------------------------
void CMapView2D::OnInitialUpdate(void)
{
	//
	// Create a title window.
	//
	m_pwndTitle = CTitleWnd::CreateTitleWnd(this, ID_2DTITLEWND);
	ASSERT(m_pwndTitle != NULL);
	UpdateTitleWindowPos();

	//
	// Other initialization.
	//
	UpdateSizeVariables();
	SetDrawType2D(m_DrawType, TRUE);
	CenterView();
	SetColorMode(Options.view2d.bWhiteOnBlack);

	ShowScrollBar(SB_HORZ, Options.view2d.bScrollbars);
	ShowScrollBar(SB_VERT, Options.view2d.bScrollbars);

	//
	// Add to doc's MRU list
	//
	CMapDoc *pDoc = GetDocument();
	pDoc->SetMRU(this);

	CView::OnInitialUpdate();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flag - 
//-----------------------------------------------------------------------------
void CMapView2D::SetUpdateFlag(int flag)
{
	Update(flag);
}


//-----------------------------------------------------------------------------
// Purpose: Called by the tools to scroll the 2D view so that a point is visible.
//			Sets a timer to do the scroll so that we don't much with the view state
//			while the tool is handling a mouse message.
// Input  : point - Point in client coordinates to make visible.
//-----------------------------------------------------------------------------
void CMapView2D::ToolScrollToPoint(CPoint &point)
{
	if ((GetCapture() == this) && !m_rectClient.PtInRect(point))
	{
		// reset these
		m_xScroll = m_yScroll = 0;
		if (point.x < 0)
		{
			// scroll left
			m_xScroll = 20;
		}
		else if (point.x >= m_rectClient.Width())
		{
			// scroll right
			m_xScroll = -20;
		}
		if (point.y < 0)
		{
			// scroll up
			m_yScroll = 20;
		}
		else if (point.y >= m_rectClient.Height())
		{
			// scroll down
			m_yScroll = -20;
		}
	
		SetTimer(TIMER_SCROLLVIEW, 10, NULL);
	}
	else
	{
		m_xScroll = m_yScroll = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Adjusts a color's intensity - will not overbrighten.
// Input  : ulColor - Color to adjust.
//			nIntensity - Percentage of original color intensity to keep (0 - 100).
//			bReverse - True ramps toward black, false ramps toward the given color.
// Output : Returns the adjusted color.
//-----------------------------------------------------------------------------
COLORREF CMapView2D::AdjustColorIntensity(COLORREF ulColor, int nIntensity, bool bReverse)
{
	if (bReverse)
	{
		nIntensity = 100 - nIntensity;
	}

	nIntensity = clamp(nIntensity, 0, 100);

	//
	// Extract the RGB components.
	//
	unsigned int uColorTemp[3];
	uColorTemp[0] = GetRValue(ulColor);
	uColorTemp[1] = GetGValue(ulColor);
	uColorTemp[2] = GetBValue(ulColor);

	//
	// Adjust each component's intensity.
	//
	for (int i = 0; i < 3; i++)
	{
		uColorTemp[i] = (uColorTemp[i] * nIntensity) / 100;
		if (uColorTemp[i] > 255)
		{
			uColorTemp[i] = 255;
		}
	}

	//
	// Build the adjusted RGB color.
	//
	ulColor = RGB(uColorTemp[0], uColorTemp[1], uColorTemp[2]);
	return(ulColor);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bWhiteOnBlack - 
//-----------------------------------------------------------------------------
void CMapView2D::SetColorMode(BOOL bWhiteOnBlack)
{
	if(m_penGrid.m_hObject != NULL)
	{
		// destroy pens and stuff
		m_penGrid.DeleteObject();

		#ifndef SDK_BUILD
		m_penGrid1024.DeleteObject();
		#endif // SDK_BUILD

		m_penGrid10.DeleteObject();
		m_penGrid0.DeleteObject();
	}

	//
	// Grid color.
	//
	COLORREF clr = Options.colors.clrGrid;
	if (Options.colors.bScaleGridColor)
	{
		clr = AdjustColorIntensity(clr, Options.view2d.iGridIntensity, !Options.view2d.bWhiteOnBlack);
	}
	m_penGrid.CreatePen(PS_SOLID, 0, clr);

	//
	// Grid highlight color.
	//
	clr = Options.colors.clrGrid10;
	if (Options.colors.bScaleGrid10Color)
	{
		clr = AdjustColorIntensity(clr, 1.5 * Options.view2d.iGridIntensity, !Options.view2d.bWhiteOnBlack);
	}
	m_penGrid10.CreatePen(PS_SOLID, 0, clr);

	//
	// Grid 1024 highlight color.
	//
	#ifndef SDK_BUILD
	clr = Options.colors.clrGrid1024;
	if (Options.colors.bScaleGrid1024Color)
	{
		clr = AdjustColorIntensity(clr, Options.view2d.iGridIntensity, !Options.view2d.bWhiteOnBlack);
	}
	m_penGrid1024.CreatePen(PS_SOLID, 0, clr);
	#endif // SDK_BUILD

	//
	// Dotted grid color. No need to create a pen since all we do is SetPixel with it.
	//
	clr = Options.colors.clrGridDot;
	if (Options.colors.bScaleGridDotColor)
	{
		m_dwDotColor = AdjustColorIntensity(clr, Options.view2d.iGridIntensity + 20, !Options.view2d.bWhiteOnBlack);
	}

	//
	// Axis color.
	//
	clr = Options.colors.clrAxis;
	if (Options.colors.bScaleAxisColor)
	{
		clr = AdjustColorIntensity(clr, Options.view2d.iGridIntensity, !Options.view2d.bWhiteOnBlack);
	}
	m_penGrid0.CreatePen(PS_SOLID, 0, clr);
}


//-----------------------------------------------------------------------------
// Purpose: Draws the grid, using dots or lines depending on the user setting.
// Input  : pDC - Device context to draw in.
//-----------------------------------------------------------------------------
void CMapView2D::DrawGrid(CDC *pDC)
{
	CMapDoc *pDoc = GetDocument();
	if (pDoc == NULL)
	{
		return;
	}

	//
	// Check for too small grid.
	//
	int nGridSpacing = pDoc->GetGridSpacing();
	BOOL bNoSmallGrid = FALSE;
	if ((((float)nGridSpacing * m_fZoom) < 4.0f) && Options.view2d.bHideSmallGrid)
	{
		bNoSmallGrid = TRUE;
	}

	//
	// No dots if too close together.
	//
	BOOL bGridDots = Options.view2d.bGridDots;

	int iWindowWidth  = m_rectClient.Width();
	int iWindowHeight = m_rectClient.Height();
	
	pDC->SelectObject(&m_penGrid);

	//
	// Get the bottom left window coordinate (in world coordinates).
	//
	CPoint ptOrg = pDC->GetWindowOrg();

	//
	// Find the window extents snapped to the nearest grid that is just outside the view,
	// and clamped to the legal world coordinates.
	//
	int x1 = _Snap(ptOrg.x - nGridSpacing);
	int x2 = x1 + iWindowWidth / m_fZoom + nGridSpacing * 3;

	int y1 = _Snap(ptOrg.y - nGridSpacing);
	int y2 = y1 + iWindowHeight / m_fZoom + nGridSpacing * 3;

	x1  = clamp(x1, g_MIN_MAP_COORD, g_MAX_MAP_COORD);
	x2  = clamp(x2, g_MIN_MAP_COORD, g_MAX_MAP_COORD);

	y1  = clamp(y1, g_MIN_MAP_COORD, g_MAX_MAP_COORD);
	y2  = clamp(y2, g_MIN_MAP_COORD, g_MAX_MAP_COORD);

	pDC->SaveDC();
	
	pDC->SetMapMode(MM_TEXT);
	pDC->SetWindowOrg(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));

	int iCustomGridSpacing = nGridSpacing * Options.view2d.iGridHighSpec;

	//
	// Draw the vertical grid lines.
	//
	BOOL bSel = FALSE;

	for (int y = y1; y <= y2; y += nGridSpacing)
	{
		//
		// Don't draw outside the map bounds.
		//
		if ((y > g_MAX_MAP_COORD) || (y < g_MIN_MAP_COORD))
		{
			continue;
		}

		//
		// Highlight the axes.
		//
		if (y == 0)
		{
			pDC->SelectObject(m_penGrid0);
			bSel = TRUE;
		}
		//
		// Optionally highlight every 1024.
		//
		#ifndef SDK_BUILD
		else if (Options.view2d.bGridHigh1024 && (!(y % 1024)))
		{
			pDC->SelectObject(m_penGrid1024);
			bSel = TRUE;
		}
		#endif // SDK_BUILD
		//
		// Optionally highlight every 64.
		//
		else if (Options.view2d.bGridHigh64 && (!(y % 64)))
		{
			if (!bGridDots)
			{
				pDC->SelectObject(m_penGrid10);
				bSel = TRUE;
			}
		}
		//
		// Optionally highlight every nth grid line.
		//
		else if (Options.view2d.bGridHigh10 && (!(y % iCustomGridSpacing)))
		{
			if (!bGridDots)
			{
				pDC->SelectObject(m_penGrid10);
				bSel = TRUE;
			}
		}

		//
		// Don't draw the base grid if it is too small.
		//
		if (!bSel && bNoSmallGrid)
		{
			continue;
		}

		//
		// Always draw lines for the axes and map boundaries.
		//
		if ((!bGridDots) || (bSel) || (y == g_MAX_MAP_COORD) || (y == g_MIN_MAP_COORD))
		{
			pDC->MoveTo(x1 * m_fZoom, y * m_fZoom);
			pDC->LineTo(x2 * m_fZoom, y * m_fZoom);
		}

		if (bGridDots && !bNoSmallGrid)
		{
			for (int xx = x1; xx <= x2; xx += nGridSpacing)
			{
				pDC->SetPixel(xx * m_fZoom, y * m_fZoom, m_dwDotColor);
			}
		}

		if (bSel)
		{
			pDC->SelectObject(m_penGrid);
			bSel = FALSE;
		}
	}

	//
	// Draw the horizontal grid lines.
	//
	bSel = FALSE;

	for (int x = x1; x <= x2; x += nGridSpacing)
	{
		//
		// Don't draw outside the map bounds.
		//
		if ((x > g_MAX_MAP_COORD) || (x < g_MIN_MAP_COORD))
		{
			continue;
		}

		//
		// Highlight the axes.
		//
		if (x == 0)
		{
			pDC->SelectObject(m_penGrid0);
			bSel = TRUE;
		}
		//
		// Optionally highlight every 1024.
		//
		#ifndef SDK_BUILD
		else if (Options.view2d.bGridHigh1024 && (!(x % 1024)))
		{
			pDC->SelectObject(m_penGrid1024);
			bSel = TRUE;
		}
		#endif // SDK_BUILD
		//
		// Optionally highlight every 64.
		//
		else if (Options.view2d.bGridHigh64 && (!(x % 64)))
		{
			if (!bGridDots)
			{
				pDC->SelectObject(m_penGrid10);
				bSel = TRUE;
			}
		}
		//
		// Optionally highlight every nth grid line.
		//
		else if (Options.view2d.bGridHigh10 && (!(x % iCustomGridSpacing)))
		{
			if (!bGridDots)
			{
				pDC->SelectObject(m_penGrid10);
				bSel = TRUE;
			}
		}

		//
		// Don't draw the base grid if it is too small.
		//
		if (!bSel && bNoSmallGrid)
		{
			continue;
		}

		//
		// Always draw lines for the axes and map boundaries.
		//
		if ((!bGridDots) || (bSel) || (x == g_MAX_MAP_COORD) || (x == g_MIN_MAP_COORD))
		{
			pDC->MoveTo(x * m_fZoom, y1 * m_fZoom);
			pDC->LineTo(x * m_fZoom, y2 * m_fZoom);
		}

		if (bGridDots && !bNoSmallGrid)
		{
			for (int yy = y1; yy <= y2; yy += nGridSpacing)
			{
				pDC->SetPixel(x * m_fZoom, yy * m_fZoom, m_dwDotColor);
			}
		}

		if (bSel)
		{
			pDC->SelectObject(m_penGrid);
			bSel = FALSE;
		}
	}

	//
	// Redraw the horizontal axis unless drawing with a dotted grid. This is necessary
	// because it was drawn over by the vertical grid lines.
	//
	if (!bGridDots)
	{
		pDC->SelectObject(m_penGrid0);
		pDC->MoveTo(x1 * m_fZoom, 0);
		pDC->LineTo(x2 * m_fZoom, 0);
	}

	pDC->RestoreDC(-1);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static CFont *_GetEntityFont()
{
	static BOOL bCreated = FALSE;	
	static CFont font;

	if(bCreated)
	{
		return &font;
	}

	// create font
	bCreated = TRUE;
	font.CreatePointFont(80, "Courier New");
	return &font;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mins - 
//			maxs - 
//			iAxis - 
//			iStart - 
//			iDepth - 
// Output : 
//-----------------------------------------------------------------------------
static BOOL CheckBoundsWithRange(Vector& mins, Vector& maxs, int iAxis, 
								 int iStart, int iDepth)
{
	if(mins[iAxis] > iStart && maxs[iAxis] < (iStart + iDepth))
		return TRUE;
	return FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pointCheck - 
//			pointRef - 
//			nDist - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMapView2D::CheckDistance(const Vector2D &vecCheck, const Vector2D &vecRef, int nDist)
{
	if ((fabs(vecRef.x - vecCheck.x) <= nDist) &&
		(fabs(vecRef.y - vecCheck.y) <= nDist))
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Gets the center point of the view in world coordinates.
// Input  : pt - Receives the center point. Only dimensions initialized with
//				COORD_NOTINIT will be filled out.
//-----------------------------------------------------------------------------
void CMapView2D::GetCenterPoint(Vector &pt)
{
	CPoint pt2 = m_rectClient.CenterPoint();
	ClientToWorld(pt2);

	if (pt[axHorz] == COORD_NOTINIT)
	{
		pt[axHorz] = pt2.x;
	}

	if (pt[axVert] == COORD_NOTINIT)
	{
		pt[axVert] = pt2.y;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDC - 
//-----------------------------------------------------------------------------
void CMapView2D::SetupDC(CDC *pDC)
{
	pDC->SetMapMode(MM_ISOTROPIC);

	pDC->SetWindowExt(1000, 1000);
	pDC->SetViewportExt(1000 * m_fZoom, 1000 * m_fZoom);

	pDC->SetWindowOrg(GetScrollPos(SB_HORZ) / m_fZoom, GetScrollPos(SB_VERT) / m_fZoom);
}


//-----------------------------------------------------------------------------
// Purpose: Recalculates member variables that depend on window size.
//-----------------------------------------------------------------------------
void CMapView2D::UpdateSizeVariables()
{
	//
	// Get new size so we know if we need to do any work.
	//
	CSize size = m_rectScreen.Size();

	// updates m_rectClient and m_lpDrawSurface
	GetClientRect(m_rectClient);
	GetWindowRect(m_rectScreen);

	// no change required
	if (m_rectScreen.Size() == size && m_bmpDraw.m_hObject)
	{
		return;
	}

	CalcViewExtents();

	m_bmpDraw.DeleteObject();
	m_bmpTool.DeleteObject();

	if (!m_rectScreen.Height() || !m_rectScreen.Width())
	{
		return;
	}
		
	// create bitmaps
	CDC *pDC = GetDC(), bmpDC;
	m_bmpDraw.CreateCompatibleBitmap(pDC, m_rectScreen.Width(), m_rectScreen.Height());

	m_bmpTool.CreateCompatibleBitmap(pDC, m_rectScreen.Width(), m_rectScreen.Height());
	ReleaseDC(pDC);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
static void WriteLog(LPCTSTR buf)
{
	FILE *fp = fopen("2d.log", "at");
	fprintf(fp, buf);
	fclose(fp);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDC - 
//-----------------------------------------------------------------------------
void CMapView2D::OnDraw(CDC* pDC)
{
	Update(updRenderAll, pDC, &m_rgnUpdate);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cmd - 
//			pDC - 
//			pRgn - 
//-----------------------------------------------------------------------------
void CMapView2D::Update(int cmd, CDC *pDC, CRgn *pRgn)
{
	static CRgn rgnWork;	// temporary region to work with

	CMapDoc *pDoc = GetDocument();

	//
	// Make sure the document is up to date.
	//
	pDoc->Update();

	if(HRGN(rgnWork) == NULL)
	{
		// need to create the region before we can use it
		rgnWork.CreateRectRgn(0, 0, 10, 10);	// temp.
	}

	BOOL bReleaseDC = FALSE;
	if(pDC == NULL)
	{
		pDC = GetDC();
		bReleaseDC = TRUE;
	}

	// find the bitmap we want to use from the current tool
	CDC ToolDC, DrawDC;
	ToolDC.CreateCompatibleDC(pDC);
	DrawDC.CreateCompatibleDC(pDC);
	CBitmap *pOldToolBmp = ToolDC.SelectObject(&m_bmpTool);
	CBitmap *pOldDrawBmp = DrawDC.SelectObject(&m_bmpDraw);

	static int nCounter = 0;
	nCounter++;

	//
	// Erase the tool from the image by blitting the world without the tool from the last
	// render. Only do this if:
	//
	// 1. There is an active tool.
	// 2. Erase tool command has been specified.
	// 3. The tool is visible in draw bitmap.
	// 4. We're not going to erase it below with a full render anyway.
	//
	if ((cmd & updEraseTool) && (m_bToolShown) && !(cmd & updRender))
	{
		ToolDC.BitBlt(0, 0, m_rectScreen.Width(), m_rectScreen.Height(), &DrawDC, 0, 0, SRCCOPY);
		m_bToolShown = FALSE;
	}

	//
	// Render the world if the flag is specified.
	//
	if (cmd & updRender)
	{
		CRect rectFill = m_rectClient;
		if (pRgn)
		{
			pRgn->GetRgnBox(&rectFill);
		}

		DrawDC.FillSolidRect(rectFill.left, rectFill.top, rectFill.Width(), rectFill.Height(), Options.colors.clrBackground);
		DrawDC.SelectObject(pOldDrawBmp);

		Render(rectFill);

		// select it back into this DC
		pOldDrawBmp = DrawDC.SelectObject(&m_bmpDraw);

		// Copy it to the tool DC
		ToolDC.BitBlt(0, 0, m_rectScreen.Width(), m_rectScreen.Height(), &DrawDC, 0, 0, SRCCOPY);

		// The tools are now obscured (we've drawn over them).
		m_bToolShown = FALSE;
	}

	//
	// Render the active tool if the flag is specified.
	//
	if (cmd & updRenderTool)
	{
		CBaseTool *pCurTool = g_pToolManager->GetTool();
		int nToolCount = g_pToolManager->GetToolCount();
		for (int i = 0; i < nToolCount; i++)
		{
			CBaseTool *pTool = g_pToolManager->GetTool(i);
			if ((pTool != NULL) && (pTool != pCurTool))
			{
				//
				// Configure the DC identically to our render DC.
				//
				ToolDC.SaveDC();
				ToolDC.SetWindowOrg(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));

				CRender2D Render;
				Render.SetCDC(&ToolDC);
				Render.Set2DViewInfo(axHorz, axVert, m_fZoom, bInvertHorz == TRUE, bInvertVert == TRUE);
				pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
				pTool->RenderTool2D(&Render);

				ToolDC.RestoreDC(-1);

				m_bToolShown = TRUE;
			}
		}

		if (pCurTool != NULL)
		{
			// FIXME: functionalize or otherwise clean up
			// Configure the DC identically to our render DC.
			//
			ToolDC.SaveDC();
			ToolDC.SetWindowOrg(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));

			CRender2D Render;
			Render.SetCDC(&ToolDC);
			Render.Set2DViewInfo(axHorz, axVert, m_fZoom, bInvertHorz == TRUE, bInvertVert == TRUE);
			pCurTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
			pCurTool->RenderTool2D(&Render);

			ToolDC.RestoreDC(-1);
			
			m_bToolShown = TRUE;
		}
	}

	//
	// Blit the image to the screen.
	//
	pDC->BitBlt(0, 0, m_rectScreen.Width(), m_rectScreen.Height(), &ToolDC, 0, 0, SRCCOPY);

	if (bReleaseDC)
	{
		ReleaseDC(pDC);
	}
	
	DrawDC.SelectObject(pOldDrawBmp);

	if (m_pwndTitle != NULL)
	{
		m_pwndTitle->Invalidate(FALSE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pDrawDC - 
//-----------------------------------------------------------------------------
void CMapView2D::DrawPointFile(CDC *pDrawDC)
{
	static CPen penPoints(PS_SOLID, 2, RGB(255, 0, 0));

	int &nPFPoints = GetDocument()->m_nPFPoints;
	Vector* &pPFPoints = GetDocument()->m_pPFPoints;
	CPoint pt;

	pDrawDC->SelectObject(&penPoints);
	Translate3D(pPFPoints[0], pt);
	pDrawDC->MoveTo(pt);

	for(int i = 1; i < nPFPoints; i++)
	{
		Translate3D(pPFPoints[i], pt);
		pDrawDC->LineTo(pt);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ppObjects - 
//			nObjects - 
//-----------------------------------------------------------------------------
void CMapView2D::RenderObjects(CDC *pDC, CUtlVector<CMapClass *> &Objects, int nObjects, CRect rectUpdate)
{
	CPoint pt;
	CPoint pt2;

	CRender2D Render;
    Render.SetCDC(pDC);
	Render.Set2DViewInfo(axHorz, axVert, m_fZoom, bInvertHorz == TRUE, bInvertVert == TRUE);

	for (int i = 0; i < nObjects; i++)
	{
		CMapClass *pobj = Objects[i];

		if (!pobj->IsVisible())
		{
			continue;
		}

		if (!pobj->IsVisible2D())
		{
			continue;
		}

		//
		// Make sure the object is in the update region.
		//
		Vector vecMins;
		Vector vecMaxs;
		pobj->GetRender2DBox(vecMins, vecMaxs);
		Translate3D(vecMins, pt);
		Translate3D(vecMaxs, pt2);

		RECT rCheck;
		rCheck.left = min(pt.x, pt2.x);
		rCheck.top = min(pt.y, pt2.y);
		rCheck.right = max(pt.x, pt2.x);
		rCheck.bottom = max(pt.y, pt2.y);

		if ((rCheck.left <= rectUpdate.right) && (rCheck.right >= rectUpdate.left) &&
			(rCheck.top <= rectUpdate.bottom) && (rCheck.bottom >= rectUpdate.top))
		{
			pobj->Render2D(&Render);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sorts the object to be rendered into one of two groups: normal objects
//			and selected objects, so that selected objects can be rendered last.
// Input  : pObject - 
//			pRenderList - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapView2D::AddToRenderList(CMapClass *pObject, RenderList_t *pRenderList)
{
	if (pObject->IsVisible())
	{
		if (pObject->IsSelected())
		{
			pRenderList->SelectedObjects.AddToTail(pObject);
		}
		else
		{
			pRenderList->NormalObjects.AddToTail(pObject);
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rectUpdate - 
//-----------------------------------------------------------------------------
void CMapView2D::Render(CRect rectUpdate)
{
	CMapDoc *pDoc = GetDocument();
	CMapWorld *pWorld = pDoc->GetMapWorld();

	CDC DrawDC;
	CDC *pDrawDC = &DrawDC;
	DrawDC.CreateCompatibleDC(NULL);
	CBitmap *pOldBitmap = DrawDC.SelectObject(&m_bmpDraw);
	pDrawDC->IntersectClipRect(rectUpdate);

	SetupDC(pDrawDC);

	rectUpdate.OffsetRect(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));

	//
	// Draw grid if enabled.
	//
	if (pDoc->m_bShowGrid)
	{
		DrawGrid(pDrawDC);
	}

	//
	// Draw the world if we have one.
	//
	if (pWorld != NULL)
	{
		//
		// Set up text output.
		//
		pDrawDC->SelectObject(_GetEntityFont());
		pDrawDC->SetTextColor(RGB(100, 100, 255));
		pDrawDC->SetBkMode(OPAQUE);
		pDrawDC->SetBkColor(Options.colors.clrBackground);
		pDrawDC->SetMapMode(MM_TEXT);
		pDrawDC->SetWindowOrg(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));

		//
		// Traverse the entire world, sorting visible elements into two arrays:
		// Normal objects and selected objects, so that we can render the selected
		// objects last.
		//
		g_RenderList.NormalObjects.RemoveAll();
		g_RenderList.SelectedObjects.RemoveAll();
		pWorld->EnumChildren((ENUMMAPCHILDRENPROC)AddToRenderList, (DWORD)&g_RenderList);

		//
		// Render normal (nonselected) objects if any.
		//
		if (g_RenderList.NormalObjects.Count() > 0)
		{
			RenderObjects(pDrawDC, g_RenderList.NormalObjects, g_RenderList.NormalObjects.Count(), rectUpdate);
		}

		//
		// Render selected objects if any.
		//
		if (g_RenderList.SelectedObjects.Count() > 0)
		{
			RenderObjects(pDrawDC, g_RenderList.SelectedObjects, g_RenderList.SelectedObjects.Count(), rectUpdate);
		}

		//
		// Draw pointfile if enabled.
		//
		if (pDoc->m_nPFPoints)
		{
			DrawPointFile(pDrawDC);
		}
	}

	pDrawDC->SelectObject(pOldBitmap);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : m_DrawType - 
//			bForceUpdate - 
//-----------------------------------------------------------------------------
void CMapView2D::SetDrawType(DrawType_t m_DrawType)
{
	SetDrawType2D(m_DrawType);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : m_DrawType - 
//			bForceUpdate - 
//-----------------------------------------------------------------------------
void CMapView2D::SetDrawType2D(DrawType_t m_DrawType, BOOL bForceUpdate)
{
	if (m_DrawType != this->m_DrawType || bForceUpdate)
	{
		switch (m_DrawType)
		{
			case VIEW2D_XY:
			{
				SetAxes(AXIS_X, FALSE, AXIS_Y, TRUE);
				if (m_pwndTitle != NULL)
				{
					m_pwndTitle->SetTitle("top (x/y)");
				}
				break;
			}
			
			case VIEW2D_YZ:
			{
				SetAxes(AXIS_Y, FALSE, AXIS_Z, TRUE);
				if (m_pwndTitle != NULL)
				{
					m_pwndTitle->SetTitle("front (y/z)");
				}
				break;
			}

			case VIEW2D_XZ:
			{
				SetAxes(AXIS_X, FALSE, AXIS_Z, TRUE);
				if (m_pwndTitle != NULL)
				{
					m_pwndTitle->SetTitle("side (x/z)");
				}
				break;
			}
		}

		this->m_DrawType = m_DrawType;
		CalcViewExtents();

		if (m_bLastActiveView && GetDocument())
		{
			GetDocument()->UpdateTitle(this);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Calculates scroll ranges based on view size, zoom level, and map size.
//-----------------------------------------------------------------------------
void CMapView2D::CalcViewExtents(void)
{
	if (!::IsWindow(m_hWnd))
	{
		return;
	}

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE;

	si.nMin = g_MIN_MAP_COORD * m_fZoom - (m_rectClient.Width() / 2);
	si.nMax = g_MAX_MAP_COORD * m_fZoom + (m_rectClient.Width() / 2);
	si.nPage = m_rectClient.Width();
	SetScrollInfo(SB_HORZ, &si, FALSE);

	si.nMin = g_MIN_MAP_COORD * m_fZoom - (m_rectClient.Height() / 2);
	si.nMax = g_MAX_MAP_COORD * m_fZoom + (m_rectClient.Height() / 2);
	si.nPage = m_rectClient.Height();
	SetScrollInfo(SB_VERT, &si, FALSE);

	//
	// SetScrollInfo causes hidden scroll bars to show themselves! Re-hide them.
	//
	if (!Options.view2d.bScrollbars)
	{
		ShowScrollBar(SB_HORZ, FALSE);
		ShowScrollBar(SB_VERT, FALSE);
	}

	Invalidate();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView2D::UpdateStatusBar()
{
	if(!IsWindow(m_hWnd))
		return;

	char szBuf[128];
	sprintf(szBuf, " Zoom: %.2f ", m_fZoom);
	SetStatusText(SBI_GRIDZOOM, szBuf);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fNewZoom - 
//-----------------------------------------------------------------------------
void CMapView2D::SetZoom(float fNewZoom)
{
	if (fNewZoom < ZOOM_MIN)
	{
		fNewZoom = ZOOM_MIN;
	}

	else if (fNewZoom > ZOOM_MAX)
	{
		fNewZoom = ZOOM_MAX;
	}

	if (m_fZoom == fNewZoom)
	{
		return;
	}

	if (IsWindow(m_hWnd))
	{
		// zoom in on cursor position
		CPoint pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		if (!m_rectClient.PtInRect(pt))
		{
			// cursor is not in window; zoom on center instead
			pt.x = m_rectClient.Width() / 2;
			pt.y = m_rectClient.Height() / 2;
		}

		CPoint ptScrl(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));

		int posX = (ptScrl.x + pt.x) / m_fZoom;
		int posY = (ptScrl.y + pt.y) / m_fZoom;
		m_fZoom = fNewZoom;
		CalcViewExtents();
		ptScrl.x = posX - (pt.x / m_fZoom);
		ptScrl.y = posY - (pt.y / m_fZoom);
		SetScrollPos(SB_HORZ, ptScrl.x * m_fZoom);
		SetScrollPos(SB_VERT, ptScrl.y * m_fZoom);
	}
	else
	{
		m_fZoom = fNewZoom;
		CalcViewExtents();
	}

	UpdateStatusBar();
}


#ifdef _DEBUG
void CMapView2D::AssertValid() const
{
	CView::AssertValid();
}

void CMapView2D::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cs - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapView2D::PreCreateWindow(CREATESTRUCT& cs) 
{
	static CString className;
	
	if(className.IsEmpty())
	{
		className = AfxRegisterWndClass(CS_BYTEALIGNCLIENT, 
			AfxGetApp()->LoadStandardCursor(IDC_ARROW), HBRUSH(NULL));
	}

	cs.lpszClass = className;
	
	return CView::PreCreateWindow(cs);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView2D::OnView2dxy(void)
{
	SetDrawType(VIEW2D_XY);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView2D::OnView2dyz(void)
{
	SetDrawType(VIEW2D_YZ);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView2D::OnView2dxz(void)
{
	SetDrawType(VIEW2D_XZ);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nChar - 
//			nRepCnt - 
//			nFlags - 
//-----------------------------------------------------------------------------
void CMapView2D::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CMapDoc *pDoc = GetDocument();
	if (pDoc == NULL)
	{
		return;
	}

	if (nChar == VK_SPACE)
	{
		// Switch the cursor to the hand. We'll start panning the view
		// on the left button down event.
		SetCursor(m_MouseDrag ? m_hcurHandClosed : m_hcurHand);
		return;
	}

    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
		if (pTool->OnKeyDown2D(this, nChar, nRepCnt, nFlags))
		{
			return;
		}
	}

	//
	// The tool didn't handle the key. Perform default handling for this view.
	//
	int bShift = GetKeyState(VK_SHIFT) & 0x8000;
	int bCtrl = GetKeyState(VK_CONTROL) & 0x8000;

	switch (nChar)
	{
		//
		// Cycle the view to the next or previous 2D view type.
		//
		case VK_TAB:
		{
			if (!bShift)
			{
				SetDrawType(NextDrawType(m_DrawType));
			}
			else
			{
				SetDrawType(PrevDrawType(m_DrawType));
			}
			return;
		}

		//
		// Zoom in.
		//
		case '+':
		case VK_ADD:
		case 'D':
		{
			ZoomIn(bCtrl);
			break;
		}

		//
		// Zoom out.
		//
		case '-':
		case VK_SUBTRACT:
		case 'C':
		{
			ZoomOut(bCtrl);
			break;
		}

		case VK_UP:
		{
			// scroll up
			OnVScroll(SB_LINEUP, 0, NULL);
			break;
		}

		case VK_DOWN:
		{
			// scroll down
			OnVScroll(SB_LINEDOWN, 0, NULL);
			break;
		}

		case VK_LEFT:
		{
			// scroll up
			OnHScroll(SB_LINELEFT, 0, NULL);
			break;
		}

		case VK_RIGHT:
		{
			// scroll up
			OnHScroll(SB_LINERIGHT, 0, NULL);
			break;
		}

		//
		// 1-9 +0 shortcuts to various zoom levels.
		//
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '0':
		{
			int iZoom = nChar - '1';
			if (nChar == '0')
			{
				iZoom = 9;
			}
			SetZoom(ZOOM_MIN * (1 << iZoom));
			break;
		}

		case VK_HOME:
		{
				// Go to the previous point in the point file.
			pDoc->GotoPFPoint(CMapDoc::PFPPrev);
			break;
		}

		case VK_END:
		{
			// Go to the next point in the point file.
			pDoc->GotoPFPoint(CMapDoc::PFPNext);
			break;
		}
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Per CWnd::OnKeyUp.
//-----------------------------------------------------------------------------
void CMapView2D::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_SPACE)
	{
		//
		// Releasing the space bar stops panning the view.
		//
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	}
	else
	{
		//
		// Pass the message to the active tool.
		//
		CBaseTool *pTool = g_pToolManager->GetTool();
		if (pTool)
		{
			pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
			if (pTool->OnKeyUp2D(this, nChar, nRepCnt, nFlags))
			{
				return;
			}
		}
	}

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Per CWnd::OnChar.
//-----------------------------------------------------------------------------
void CMapView2D::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
		if (pTool->OnChar2D(this, nChar, nRepCnt, nFlags))
		{
			return;
		}
	}

	CView::OnChar(nChar, nRepCnt, nFlags);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt1 - 
//			pt2 - 
//			x1 - 
//			y1 - 
//			x2 - 
//			y2 - 
//-----------------------------------------------------------------------------
BOOL IsLineInside(const Vector2D &pt1, const Vector2D &pt2, int x1, int y1, int x2, int y2)
{
    int lx1 = pt1.x;
    int ly1 = pt1.y;
    int lx2 = pt2.x;
    int ly2 = pt2.y;
    int i;

    /* is the line totally on one side of the box? */
    if( (lx2 > x2 && lx1 > x2) ||
        (lx2 < x1 && lx1 < x1) ||
        (ly2 > y2 && ly1 > y2) ||
        (ly2 < y1 && ly1 < y1) )
        return 0;

    if( lx1 >= x1 && lx1 <= x2 && ly1 >= y1 && ly1 <= y2 )
        return 1; /* the first point entirely inside the square */
    if( lx2 >= x1 && lx2 <= x2 && ly2 >= y1 && ly2 <= y2 )
        return 1; /* the second point is entirely inside the square */
    if( (ly1 > y1) != (ly2 > y1) )
    {
        i = lx1 + (int) ( (long) (y1 - ly1) * (long) (lx2 - lx1) / (long) (ly2 - ly1));
        if( i >= x1 && i <= x2 )
            return 1; /* the line crosses the y1 side (left) */
    }
    if( (ly1 > y2) != (ly2 > y2))
    {
        i = lx1 + (int) ( (long) (y2 - ly1) * (long) (lx2 - lx1) / (long) (ly2 - ly1));
        if( i >= x1 && i <= x2 )
            return 1; /* the line crosses the y2 side (right) */
    }
    if( (lx1 > x1) != (lx2 > x1))
    {
        i = ly1 + (int) ( (long) (x1 - lx1) * (long) (ly2 - ly1) / (long) (lx2 - lx1));
        if( i >= y1 && i <= y2 )
            return 1; /* the line crosses the x1 side (down) */
    }
    if( (lx1 > x2) != (lx2 > x2))
    {
        i = ly1 + (int) ( (long) (x2 - lx1) * (long) (ly2 - ly1) / (long) (lx2 - lx1));
        if( i >= y1 && i <= y2 )
            return 1; /* the line crosses the x2 side (up) */
    }

    /* The line is not inside the square. */
    return 0;
}


#define CENTER_HANDLE_RADIUS 3


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//			vecPoint - Point in client coordinates.
//			bMakeFirst - 
//-----------------------------------------------------------------------------
void CMapView2D::SelectAtCheckHit(CMapClass *pObject, const Vector2D &vecPoint, bool bMakeFirst)
{
	if (!pObject->IsVisible())
	{
		return;
	}

	CMapDoc *pDoc = GetDocument();

	if (pObject->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
	{
		CMapSolid *pSolid = (CMapSolid*)pObject;

		//
		// First check center X.
		//
		Vector vecCenter;
		pSolid->GetBoundsCenter(vecCenter);

		Vector2D vecClientCenter;
		WorldToClient(vecClientCenter, vecCenter);

		if (CheckDistance(vecPoint, vecClientCenter, CENTER_HANDLE_RADIUS))
		{
			ToolID_t eTool = g_pToolManager->GetToolID();
			if (eTool == TOOL_MORPH)
			{
				UINT cmd = Morph3D::scClear | Morph3D::scSelect;
				if (!bMakeFirst)
				{
					cmd = Morph3D::scToggle;
				}
				pDoc->Morph_SelectObject(pSolid, cmd);

				UpdateBox ub;
				ub.Objects = NULL;
				pDoc->Morph_GetBounds(ub.Box.bmins, ub.Box.bmaxs, true);

				Vector mins;
				Vector maxs;
				pSolid->GetRender2DBox(mins, maxs);
				ub.Box.UpdateBounds(mins, maxs);

				pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_OBJECTS, &ub);
			}
			else
			{
				CMapClass *pSelObject = pObject->PrepareSelection(pDoc->Selection_GetMode());
				if (pSelObject)
				{
					pDoc->AddHit(pSelObject);
				}
			}
		}
		else if (!Options.view2d.bSelectbyhandles && (g_pToolManager->GetToolID() != TOOL_MORPH))
		{
			//
			// See if any edges are within certain distance from the the point.
			//
			int iSelUnits = (1 / m_fZoom) + 2;
			int x1 = vecPoint.x - iSelUnits;
			int x2 = vecPoint.x + iSelUnits;
			int y1 = vecPoint.y - iSelUnits;
			int y2 = vecPoint.y + iSelUnits;

			int nFaces = pSolid->GetFaceCount();
			for (int i = 0; i < nFaces; i++)
			{
				CMapFace *pFace = pSolid->GetFace(i);
				int nPoints = pFace->nPoints;
				if (nPoints > 0)
				{
					Vector *pPoints = pFace->Points;

					Vector2D vec1;
					WorldToClient(vec1, pPoints[0]);

					for (int j = 1; j < nPoints; j++)
					{
						Vector2D vec2;
						WorldToClient(vec2, pPoints[j]);

						if (IsLineInside(vec1, vec2, x1, y1, x2, y2))
						{
							CMapClass *pSelObject = pObject->PrepareSelection(pDoc->Selection_GetMode());
							if (pSelObject)
							{
								pDoc->AddHit(pSelObject);
							}
						}
						else
						{
							vec1 = vec2;
						}
					}
				}
			}
		}
	}
	else if (pObject->IsMapClass(MAPCLASS_TYPE(CMapEntity)))
	{
		CMapEntity *pEntity = (CMapEntity *)pObject;

		//
		// Only check point entities; brush entities are selected via their brushes.
		//
		if (pEntity->IsPlaceholder())
		{
			//
			// First check center X.
			//
			Vector vecCenter;
			pEntity->GetOrigin(vecCenter);

			Vector2D vecClientCenter;
			WorldToClient(vecClientCenter, vecCenter);

			if (CheckDistance(vecPoint, vecClientCenter, CENTER_HANDLE_RADIUS))
			{
				CMapClass *pSelObject = pObject->PrepareSelection(pDoc->Selection_GetMode());
				if (pSelObject)
				{
					pDoc->AddHit(pSelObject);
				}
			}
			else if (!Options.view2d.bSelectbyhandles)
			{
				//
				// Check the entire bounding box.
				// FIXME: shouldn't we check the entity's children instead?
				//
				Vector vecMins;
				Vector vecMaxs;
				pEntity->GetRender2DBox(vecMins, vecMaxs);
				CRect r(vecMins[axHorz], vecMins[axVert], vecMaxs[axHorz], vecMaxs[axVert]);

				if (r.PtInRect(CPoint(vecPoint.x, vecPoint.y)))
				{
					CMapClass *pSelObject = pObject->PrepareSelection(pDoc->Selection_GetMode());
					if (pSelObject)
					{
						pDoc->AddHit(pSelObject);
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : point - Point in client coordinates.
//			bMakeFirst - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMapView2D::SelectAt(CPoint point, bool bMakeFirst, bool bFace)
{
	CMapDoc *pDoc = GetDocument();
	CMapWorld *pWorld = pDoc->GetMapWorld();

	Vector2D vecPoint(point.x, point.y);

	pDoc->ClearHitList();

	GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Selection");

	//
	// Check all the objects in the world for a hit at this point.
	//
	EnumChildrenPos_t pos;
	CMapClass *pChild = pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		SelectAtCheckHit(pChild, vecPoint, bMakeFirst);
		pChild = pWorld->GetNextDescendent(pos);
	}

	//
	// If there were no hits at the given point, clear selection.
	//
	if (!pDoc->GetFirstHitPosition())
	{
		if (bMakeFirst)
		{
			if (bFace)
			{
				// clear face selection
				pDoc->SelectFace(NULL, 0, CMapDoc::scClear | CMapDoc::scUpdateDisplay);
			}
			else
			{
				// clear selection
				pDoc->SelectObject(NULL, CMapDoc::scClear | CMapDoc::scUpdateDisplay);
			}
		}

		return false;
	}

	//
	// If we're selecting faces, mark all faces on the first solid we hit.
	//
	if (bFace)
	{
		POSITION p = pDoc->GetFirstHitPosition();
		while (p != NULL)
		{
			CMapClass *pObject = pDoc->GetNextHit(p);
			if (pObject->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
			{
				pDoc->SelectFace((CMapSolid*)pObject, -1, CMapDoc::scSelect | CMapDoc::scUpdateDisplay | (bMakeFirst ? CMapDoc::scClear : 0));
				break;
			}
		}

		return true;
	}

	//
	// Select a single object.
	//
	if (bMakeFirst)
	{
		pDoc->SelectObject(NULL, CMapDoc::scClear);
	}

	pDoc->SetCurHit(0);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFlags - 
//			point - 
//-----------------------------------------------------------------------------
void CMapView2D::OnLButtonDown(UINT nFlags, CPoint point) 
{
	//
	// Check for view-specific keyboard overrides.
	//
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		//
		// Space bar + mouse move scrolls the view.
		//
		m_sizeScrolled.cx = m_sizeScrolled.cy = 0;
		m_MouseDrag = MD_DRAG;

		m_ptLDownClient = point;

		SetCapture();
		SetTimer(TIMER_MOUSEDRAG, 10, NULL);
		SetCursor(m_hcurHandClosed);
		return;
	}

    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
		if (pTool->OnLMouseDown2D(this, nFlags, point))
		{
			return;
		}
	}

	m_ptLDownClient = point;

	CView::OnLButtonDown(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFlags - 
//			point - 
//-----------------------------------------------------------------------------
void CMapView2D::OnMouseMove(UINT nFlags, CPoint point) 
{
	//
	// Make sure we are the active view.
	//
	if (!IsActive())
	{
		CMapDoc *pDoc = GetDocument();
		if (pDoc != NULL)
		{
			pDoc->SetActiveView(this);
		}
	}

	//
	// If we are the active application, make sure this view has the input focus.
	//	
	if (APP()->IsActiveApp())
	{
		if (GetFocus() != this)
		{
			SetFocus();
		}
	}

	//
	// Panning the view with the mouse, just update current position and exit.
	//
	if (m_MouseDrag)
	{
		m_sizeScrolled = point - m_ptLDownClient;
		return;
	}

    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
		if (pTool->OnMouseMove2D(this, nFlags, point))
		{
			return;
		}
	}

	//
	// The tool didn't handle the message. Make sure the cursor is set.
	//
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		SetCursor(m_MouseDrag ? m_hcurHandClosed : m_hcurHand);
	}
	else
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	}

	CView::OnMouseMove(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: Handles mouse wheel events. The mouse wheel is used to zoom the 2D
//			view in and out.
// Input  : Per CWnd::OnMouseWheel.
//-----------------------------------------------------------------------------
BOOL CMapView2D::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
		if (pTool->OnMouseWheel2D(this, nFlags, zDelta, point))
		{
			return(TRUE);
		}
	}

	if (zDelta < 0)
	{
		ZoomOut(nFlags & MK_CONTROL);
	}
	else
	{
		ZoomIn(nFlags & MK_CONTROL);
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Scrolls the view to make sure that the position in world space is visible.
// Input  : vecPos - 
//-----------------------------------------------------------------------------
void CMapView2D::EnsureVisible(Vector &vecPos)
{
	CPoint pt;
	Translate3D(vecPos, pt);

	// make point relative to client area
	pt.x -= GetScrollPos(SB_HORZ);
	pt.y -= GetScrollPos(SB_VERT);

	// check to see if it's in the client
	if (!m_rectClient.PtInRect(pt))
	{
		// otherwise, move to point + margin
		ScrollWindow(pt.x + (pt.x < 0) ? -25 : 25, pt.y + (pt.y < 0) ? -25 : 25);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Scrolls the view horizontally and vertically.
// Input  : xAmount - Horizontal distance to scroll.
//			yAmount - Vertical distance to scroll.
//-----------------------------------------------------------------------------
void CMapView2D::ScrollWindow(int xAmount, int yAmount)
{
	SCROLLINFO siHorz;
	siHorz.cbSize = sizeof(siHorz);
	siHorz.fMask = SIF_POS | SIF_RANGE;
	GetScrollInfo(SB_HORZ, &siHorz);

	SCROLLINFO siVert;
	siVert.cbSize = sizeof(siVert);
	siVert.fMask = SIF_POS | SIF_RANGE;
	GetScrollInfo(SB_VERT, &siVert);

	//
	// Make sure we don't scroll past our scroll range.
	//
	if (xAmount != 0)
	{
		if (siHorz.nPos - xAmount < siHorz.nMin)
		{
			xAmount = siHorz.nPos - siHorz.nMin;
		}
		else if ((int)((siHorz.nPos + siHorz.nPage) - xAmount) > siHorz.nMax)
		{
			xAmount = siHorz.nPos - (siHorz.nMax + siHorz.nPage);
		}
	}

	if (yAmount != 0)
	{
		if (siVert.nPos - yAmount < siVert.nMin)
		{
			yAmount = siVert.nPos - siVert.nMin;
		}
		else if ((int)((siVert.nPos  + siVert.nPage) - yAmount) > siVert.nMax)
		{
			yAmount = siVert.nPos - (siVert.nMax + siVert.nPage);
		}
	}

	//
	// Do the scroll (if any).
	//
	if ((xAmount != 0) || (yAmount != 0))
	{
		if (xAmount != 0)
		{
			siHorz.fMask = SIF_POS;
			siHorz.nPos -= xAmount;
			SetScrollInfo(SB_HORZ, &siHorz);
		}

		if (yAmount != 0)
		{
			siVert.fMask = SIF_POS;
			siVert.nPos -= yAmount;
			SetScrollInfo(SB_VERT, &siVert);
		}

		//
		// Hide the title window before scrolling the view, or it will
		// show briefly in the scrolled location.
		//
		m_pwndTitle->ShowWindow(SW_HIDE);

		CView::ScrollWindow(xAmount, yAmount);

		//
		// Put the title window back where it belongs and show it.
		//
		UpdateTitleWindowPos();
		m_pwndTitle->ShowWindow(SW_SHOW);

		Invalidate();
		UpdateWindow();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Replicates the given list of objects, then selects the duplicates.
// Input  : Objects - 
//-----------------------------------------------------------------------------
void CMapView2D::CloneObjects(CMapObjectList &Objects)
{
	CMapObjectList NewObjects;
	
	CMapDoc *pDoc = GetDocument();
	CMapWorld *pWorld = GetActiveWorld();

	//
	// Run through list of objects and copy each to build a list of cloned
	// objects.
	//
	POSITION p = Objects.GetHeadPosition();
	while (p != NULL)
	{
		CMapClass *pobj = Objects.GetNext(p);
		CMapClass *pNewobj = pobj->Copy(false);
		pNewobj->CopyChildrenFrom(pobj, false);

		if (!Options.view2d.bKeepclonegroup)
		{
			pNewobj->SetVisGroup(NULL);
		}
		else
		{
			pNewobj->SetVisGroup(pobj->GetVisGroup());
		}

		NewObjects.AddTail(pNewobj);
	}

	//
	// Notification happens in two-passes. The first pass lets objects generate new unique
	// IDs, the second pass lets objects fixup references to other cloned objects.
	//
	POSITION p1 = Objects.GetHeadPosition();
	POSITION p2 = NewObjects.GetHeadPosition();
	while (p1 != NULL)
	{
		CMapClass *pobj = Objects.GetNext(p1);
		CMapClass *pNewobj = NewObjects.GetNext(p2);

		pobj->OnPreClone(pNewobj, pWorld, Objects, NewObjects);
	}

	//
	// Do the second pass of notification and add the objects to the world. The second pass
	// of notification lets objects fixup references to other cloned objects.
	//
	p1 = Objects.GetHeadPosition();
	p2 = NewObjects.GetHeadPosition();
	while (p1 != NULL)
	{
		CMapClass *pobj = Objects.GetNext(p1);
		CMapClass *pNewobj = NewObjects.GetNext(p2);

		pobj->OnClone(pNewobj, pWorld, Objects, NewObjects);

		pDoc->AddObjectToWorld(pNewobj);
	}

	//
	// Select these new objects - don't transform old ones.
	// We have to do this a yucky way to get by selection's
	// default "set empty" procedure.
	//
	while (pDoc->Selection_GetCount())
	{
		pDoc->SelectObject(pDoc->Selection_GetObject(0), CMapDoc::scUnselect);
	}

	p = NewObjects.GetHeadPosition();
	while (p)
	{
		pDoc->SelectObject(NewObjects.GetNext(p), CMapDoc::scSelect);
	}

	// need to do this to put things in object properties:
	pDoc->SelectObject(NULL, CMapDoc::scUpdateDisplay);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFlags - 
//			point - 
//-----------------------------------------------------------------------------
void CMapView2D::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CMapDoc *pDoc = GetDocument();

	//
	// Convert to some odd coordinate space that the base tools code uses.
	//
  	CPoint ptScreen = point;
	ptScreen.x += GetScrollPos(SB_HORZ);
	ptScreen.y += GetScrollPos(SB_VERT);

	ReleaseCapture();

	if (m_MouseDrag)
	{
		m_MouseDrag = MD_NOTHING;
		KillTimer(TIMER_MOUSEDRAG);
		OnMouseMove(nFlags, point);
		return;
	}

    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
		if (pTool->OnLMouseUp2D(this, nFlags, point))
		{
			return;
		}
	}
	
	// we might have removed some stuff that was relevant:
	pDoc->UpdateStatusbar();
	
	CView::OnLButtonUp(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDC - 
//-----------------------------------------------------------------------------
void CMapView2D::DrawActive(CDC *pDC)
{
	return;

	BOOL bMadeDC = pDC ? FALSE : TRUE;
	CDC dc;

	if(bMadeDC)
	{
		dc.Attach(::GetDC(m_hWnd));
		pDC = &dc;
	}

	CBrush brush;

	pDC->SetROP2(R2_COPYPEN);

	if(m_bActive)
		brush.CreateSolidBrush(RGB(0, 0, 255));
	else
		brush.FromHandle(HBRUSH(GetStockObject(WHITE_BRUSH)));

	pDC->FrameRect(m_rectClient, &brush);

	if(bMadeDC)
	{
		::ReleaseDC(m_hWnd, dc.Detach());
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bActivate - 
//			pActivateView - 
//			pDeactiveView - 
//-----------------------------------------------------------------------------
void CMapView2D::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if (bActivate)
	{
		CMapDoc *pDoc = GetDocument();

		pDoc->SetMRU(this);
		CMapDoc::SetActiveMapDoc(pDoc);

		pDoc->SetActiveView(this);

		UpdateStatusBar();
		m_bActive = TRUE;

		// tell doc to update title
		m_bLastActiveView = TRUE;
		pDoc->UpdateTitle(this);
	}
	else
	{
		m_bActive = FALSE;
		m_xScroll = m_yScroll = 0;
		m_bLastActiveView = FALSE;
	}
	DrawActive();
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt3 - 
//			pt - 
//-----------------------------------------------------------------------------
void CMapView2D::MapToClient(Vector& pt3, CPoint& pt)
{
	pt.x = pt3[axHorz] * m_fZoom;
	pt.y = pt3[axVert] * m_fZoom;

	PointToScreen(pt);

	// add scrollbar stuff
	pt.x -= GetScrollPos(SB_HORZ);
	pt.y -= GetScrollPos(SB_VERT);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pSender - 
//			lHint - 
//			pHint - 
//-----------------------------------------------------------------------------
void CMapView2D::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if(((lHint & MAPVIEW_COLOR_ONLY) && !Options.view2d.bUsegroupcolors) ||
		lHint & MAPVIEW_UPDATE_3D)
		return;	// nope.

	if(lHint & MAPVIEW_OPTIONS_CHANGED)
	{
		ShowScrollBar(SB_HORZ, Options.view2d.bScrollbars);
		ShowScrollBar(SB_VERT, Options.view2d.bScrollbars);
		SetColorMode(Options.view2d.bWhiteOnBlack);
		
		Invalidate();
	}
	else if(lHint & (MAPVIEW_UPDATE_OBJECTS | MAPVIEW_UPDATE_DISPLAY))
	{
		if(!pHint)
			Invalidate();
		else
		{
			UpdateBox *pUB = (UpdateBox*) pHint;
			CPoint pt;
			CRect r;

			// invalidate area in update box
			MapToClient(pUB->Box.bmins, pt);
			r.left = pt.x; r.top = pt.y;
			MapToClient(pUB->Box.bmaxs, pt);
			r.right = pt.x; r.bottom = pt.y;

			r.NormalizeRect();
			r.InflateRect(11, 11);
			InvalidateRect(r);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt3 - 
//-----------------------------------------------------------------------------
void CMapView2D::CenterView(Vector* pt3)
{
	CMapWorld *pWorld = GetDocument()->GetMapWorld();

	float fPointX, fPointY;

	if(pt3)
	{
		// use provided point
		fPointX = (*pt3)[axHorz];
		fPointY = (*pt3)[axVert];
	}
	else
	{
		//
		// Use center of map.
		//
		Vector vecMins;
		Vector vecMaxs;
		pWorld->GetRender2DBox(vecMins, vecMaxs);

		fPointX = (vecMaxs[axHorz] + vecMins[axHorz]) / 2;
		fPointY = (vecMaxs[axVert] + vecMins[axVert]) / 2;
	}

	if(bInvertHorz)
		fPointX = -fPointX;
	fPointX -= (m_rectClient.Width() / m_fZoom) / 2;
	SetScrollPos(SB_HORZ, fPointX * m_fZoom);

	if(bInvertVert)
		fPointY = -fPointY;
	fPointY -= (m_rectClient.Height() / m_fZoom) / 2;
	SetScrollPos(SB_VERT, fPointY * m_fZoom);

	Invalidate();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nSBCode - 
//			nPos - 
//			pScrollBar - 
//-----------------------------------------------------------------------------
void CMapView2D::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar) 
{
	int iPos = int(nPos);

	SCROLLINFO si;
	GetScrollInfo(SB_HORZ, &si);
	int iCurPos = si.nPos;

	switch (nSBCode)
	{
		case SB_LINELEFT:
		{
			iPos = -int(si.nPage / 4);
			break;
		}
		case SB_LINERIGHT:
		{
			iPos = int(si.nPage / 4);
			break;
		}
		case SB_PAGELEFT:
		{
			iPos = -int(si.nPage / 2);
			break;
		}
		case SB_PAGERIGHT:
		{
			iPos = int(si.nPage / 2);
			break;
		}
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		{
			iPos -= iCurPos;
			break;
		}
	}

	ScrollWindow(-iPos, 0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nSBCode - 
//			nPos - 
//			pScrollBar - 
//-----------------------------------------------------------------------------
void CMapView2D::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar) 
{
	int iPos = int(nPos);

	SCROLLINFO si;
	GetScrollInfo(SB_VERT, &si);
	int iCurPos = si.nPos;

	switch (nSBCode)
	{
		case SB_LINEUP:
		{
			iPos = -int(si.nPage / 4);
			break;
		}
		case SB_LINEDOWN:
		{
			iPos = int(si.nPage / 4);
			break;
		}
		case SB_PAGEUP:
		{
			iPos = -int(si.nPage / 2);
			break;
		}
		case SB_PAGEDOWN:
		{
			iPos = int(si.nPage / 2);
			break;
		}
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		{
			iPos -= iCurPos;
			break;
		}
	}

	ScrollWindow(0, -iPos);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFlags - 
//			point - 
//-----------------------------------------------------------------------------
void CMapView2D::OnRButtonDown(UINT nFlags, CPoint point) 
{
    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
		if (pTool->OnRMouseDown2D(this, nFlags, point))
		{
			return;
		}
	}

	CView::OnRButtonDown(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIDEvent - 
//-----------------------------------------------------------------------------
void CMapView2D::OnTimer(UINT nIDEvent) 
{
	switch(nIDEvent)
	{
	case TIMER_SCROLLVIEW:
	{
		if (m_xScroll || m_yScroll)
		{
			ScrollWindow(m_xScroll, m_yScroll);
			// force mousemove event
			CPoint pt;
			GetCursorPos(&pt);
			ScreenToClient(&pt);
			OnMouseMove(0, pt);
		}
		KillTimer(TIMER_SCROLLVIEW);
		break;
	}
	case TIMER_MOUSEDRAG:
		if(m_MouseDrag == MD_DRAG)
		{
			GetCursorPos(&m_ptLDownClient);
			ScreenToClient(&m_ptLDownClient);
			ScrollWindow(m_sizeScrolled.cx, m_sizeScrolled.cy);
			UpdateWindow();
			m_sizeScrolled.cx = m_sizeScrolled.cy = 0;
		}
		break;
	}

	CView::OnTimer(nIDEvent);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nID - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapView2D::OnToolsAlign(UINT nID) 
{
	CMapDoc *pDoc = GetDocument();

	GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Align");
	GetHistory()->Keep(pDoc->Selection_GetList());

	// convert nID into the appropriate ID_TOOLS_ALIGNxxx define
	// taking into consideration the orientation of the axes
	if(nID == ID_TOOLS_ALIGNTOP && bInvertVert)
		nID = ID_TOOLS_ALIGNBOTTOM;
	else if(nID == ID_TOOLS_ALIGNBOTTOM && bInvertVert)
		nID = ID_TOOLS_ALIGNTOP;
	else if(nID == ID_TOOLS_ALIGNLEFT && bInvertHorz)
		nID = ID_TOOLS_ALIGNRIGHT;
	else if(nID == ID_TOOLS_ALIGNRIGHT && bInvertHorz)
		nID = ID_TOOLS_ALIGNLEFT;

	// use boundbox of selection - move all objects to match extreme 
	// side of all the objects
	BoundBox box;
	pDoc->Selection_GetBounds(box.bmins, box.bmaxs);

	Vector ptMove( 0, 0, 0 );

	int nSelCount = pDoc->Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = pDoc->Selection_GetObject(i);

		Vector vecMins;
		Vector vecMaxs;
		pObject->GetRender2DBox(vecMins, vecMaxs);

		// align top
		if (nID == ID_TOOLS_ALIGNTOP)
		{
			ptMove[axVert] = box.bmins[axVert] - vecMins[axVert];
		}
		else if (nID == ID_TOOLS_ALIGNBOTTOM)
		{
			ptMove[axVert] = box.bmaxs[axVert] - vecMaxs[axVert];
		}
		else if (nID == ID_TOOLS_ALIGNLEFT)
		{
			ptMove[axHorz] = box.bmins[axHorz] - vecMins[axHorz];
		}
		else if (nID == ID_TOOLS_ALIGNRIGHT)
		{
			ptMove[axHorz] = box.bmaxs[axHorz] - vecMaxs[axHorz];
		}
		pObject->TransMove(ptMove);
	}

//	pDoc->ToolUpdateViews(updEraseTool);	// remove sel rect
	pDoc->Selection_UpdateBounds();
	UpdateBox ub;
	ub.Box = box;
	ub.Objects = pDoc->Selection_GetList();
	pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);
	pDoc->SetModifiedFlag();

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pWnd - 
//			point - 
//-----------------------------------------------------------------------------
void CMapView2D::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMapDoc *pDoc = GetDocument();
	if (m_MouseDrag != MD_NOTHING)
	{
		return;
	}

    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
		if (pTool->OnContextMenu2D(this, point))
		{
			return;
		}
	}

	static CMenu menu, menuDefault;
	static bool bInit = false;

	if(!bInit)
	{
		bInit = true;
		menu.LoadMenu(IDR_POPUPS);
		menuDefault.Attach(::GetSubMenu(menu.m_hMenu, 2));
	}

	ScreenToClient(&point);

	if(!m_rectClient.PtInRect(point))
	{
		return;
	}

	CPoint ptScreen(point);
	ClientToScreen(&ptScreen);
	ClientToWorld(point);

	menuDefault.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN, ptScreen.x, ptScreen.y, this);
}


//-----------------------------------------------------------------------------
// Purpose: Called whenever the view is resized.
// Input  : nType - 
//			cx - 
//			cy - 
//-----------------------------------------------------------------------------
void CMapView2D::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	UpdateSizeVariables();
	Invalidate();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView2D::OnEditProperties() 
{
	// kludge for trackpopupmenu()
	GetMainWnd()->pObjectProperties->ShowWindow(SW_SHOW);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFlags - 
//			point - 
//-----------------------------------------------------------------------------
void CMapView2D::OnRButtonUp(UINT nFlags, CPoint point) 
{
    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		pTool->SetAxesZoom(axHorz, bInvertHorz, axVert, bInvertVert, m_fZoom);
		if (pTool->OnRMouseUp2D(this, nFlags, point))
		{
			return;
		}
	}

	CView::OnRButtonUp(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCmdUI - 
//-----------------------------------------------------------------------------
void CMapView2D::OnUpdateEditFunction(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( g_pToolManager->GetToolID() != TOOL_FACEEDIT_MATERIAL );
}


//-----------------------------------------------------------------------------
// Purpose: Flips the selection horizontally or vertically (with respect to the
//			view orientation.
// Input  : nID - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapView2D::OnFlip(UINT nID) 
{
	CMapDoc *pDoc = GetDocument();

	if (!pDoc->Selection_GetCount())
	{
		return TRUE;	// no selection
	}

	// flip objects from center of selection
	Vector ptRef;
	pDoc->Selection_GetBoundsCenter(ptRef);

	// never about this axis:
	ptRef[axThird] = COORD_NOTINIT;
	if (nID == ID_FLIP_HORIZONTAL)
	{
		ptRef[axVert] = COORD_NOTINIT;
	}
	else if (nID == ID_FLIP_VERTICAL)
	{
		ptRef[axHorz] = COORD_NOTINIT;
	}

	GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Flip Objects");
	GetHistory()->Keep(pDoc->Selection_GetList());
		
	// do flip
	int nSelCount = pDoc->Selection_GetCount();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = pDoc->Selection_GetObject(i);
		pObject->TransFlip(ptRef);
	}

	UpdateBox ub;
	pDoc->Selection_GetBounds(ub.Box.bmins, ub.Box.bmaxs);
	ub.Objects = pDoc->Selection_GetList();
	pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);
	pDoc->SetModifiedFlag();

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDC - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapView2D::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView2D::OnPaint()
{
	GetUpdateRgn(&m_rgnUpdate);
	CView::OnPaint();
}


//-----------------------------------------------------------------------------
// Purpose: Converts a 3D world coordinate to 2D client coordinates.
// Input  : ptClient - 
//			vecWorld - 
//-----------------------------------------------------------------------------
void CMapView2D::WorldToClient(Vector2D &vecClient, const Vector &vecWorld)
{
	vecClient.x = vecWorld[axHorz];
	vecClient.y = vecWorld[axVert];

	if (bInvertHorz)
	{
		vecClient.x = -vecClient.x;
	}

	if (bInvertVert)
	{
		vecClient.y = -vecClient.y;
	}

	vecClient.x *= m_fZoom;
	vecClient.y *= m_fZoom;

	vecClient.x -= GetScrollPos(SB_HORZ);
	vecClient.y -= GetScrollPos(SB_VERT);
}


//-----------------------------------------------------------------------------
// Purpose: Converts a 2D point in world coordinates to client coordinates.
// Input  : point - 
//-----------------------------------------------------------------------------
void CMapView2D::WorldToClient(CPoint &point)
	{
	if (bInvertHorz)
	{
		point.x = -point.x;
	}

	if (bInvertVert)
	{
		point.y = -point.y;
	}

	point.x *= m_fZoom;
	point.y *= m_fZoom;

	point.x -= GetScrollPos(SB_HORZ);
	point.y -= GetScrollPos(SB_VERT);
}


//-----------------------------------------------------------------------------
// Purpose: Zooms the 2D view in.
// Input  : bAllViews - Whether to set all 2D views to this zoom level.
//-----------------------------------------------------------------------------
void CMapView2D::ZoomIn(BOOL bAllViews)
{
	SetZoom(m_fZoom * 1.2);

	//
	// Set all doc 2d view zooms to this zoom level.
	//
	if (bAllViews)
	{
		VIEW2DINFO vi;
		vi.wFlags = VI_ZOOM;
		vi.fZoom = m_fZoom;

		CMapDoc *pDoc = GetDocument();
		if (pDoc != NULL)
		{
			pDoc->SetView2dInfo(vi);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Zooms the 2D view out.
// Input  : bAllViews - Whether to set all 2D views to this zoom level.
//-----------------------------------------------------------------------------
void CMapView2D::ZoomOut(BOOL bAllViews)
{
	SetZoom(m_fZoom / 1.2);

	//
	// Set all doc 2d view zooms to this zoom level.
	//
	if (bAllViews)
	{
		VIEW2DINFO vi;
		vi.wFlags = VI_ZOOM;
		vi.fZoom = m_fZoom;

		CMapDoc *pDoc = GetDocument();
		if (pDoc != NULL)
		{
			pDoc->SetView2dInfo(vi);
		}
	}
}
