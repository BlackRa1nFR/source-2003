//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPVIEW2D_H
#define MAPVIEW2D_H
#ifdef _WIN32
#pragma once
#endif


#include "Axes2.h"
#include "MapView.h"
#include "MapClass.h"		// For CMapObjectList
#include "UtlVector.h"


class CMapView3D;		// dvs: get rid of this
class Selection3D;
class CTitleWnd;
class CMapDoc;
class Tool3D;


struct RenderList_t;


class CMapView2D : public CMapView, public Axes2
{
	protected:
		int m_nSelectedVisibles;
		CMapClass ** m_hSelectedVisibles;
		CMapView2D();           // protected constructor used by dynamic creation
		DECLARE_DYNCREATE(CMapView2D)

	// Operations
	public:
		enum
		{
			updDrawTool = 0x01,
			updRenderTool = 0x02,
			updEraseTool = 0x04,
			updRender = 0x08,
			updTool = updDrawTool | updRenderTool | updEraseTool,
			updRenderAll = updRender | updTool
		};

		inline CMapDoc* GetDocument()
		{ return (CMapDoc*) m_pDocument; }
		void SetDrawType2D(DrawType_t eDrawType, BOOL bForceUpdate = FALSE);
		void SetDrawType(DrawType_t eDrawType);
		DrawType_t GetDrawType()
		{ return m_DrawType; }
		void DrawPointFile(CDC *pDrawDC);

		void SetZoom(float flNewZoom);
		inline float GetZoom(void);

		void CenterView(Vector *pt3 = NULL);
		void Activate(BOOL bActivate);
		void DrawActive(CDC *pDC = NULL);
		void CalcViewExtents();
		void SetupDC(CDC *pDC);
		void ToolScrollToPoint(CPoint &point);
		void UpdateStatusBar();
		bool SelectAt(CPoint point, bool bMakeFirst, bool bFace);
		void GetCenterPoint(Vector& pt);
		void ScrollWindow(int xAmount, int yAmount);
		void MapToClient(Vector& pt3, CPoint& pt);
		void EnsureVisible(Vector& pt3);
		void UpdateSizeVariables();
		void UpdateTitleWindowPos();
		void Update(int cmd, CDC *pDC = NULL, CRgn *pRgn = NULL);
		void Render(CRect rectUpdate);
		void SetUpdateFlag(int flag = updRenderAll);
		void ZoomIn(BOOL bAllViews = FALSE);
		void ZoomOut(BOOL bAllViews = FALSE);

		//
		// Only called by the tools:
		//
		void CloneObjects(CMapObjectList &Objects);

		//
		// Coordinate transformation functions.
		//
		//void WorldToClient(CPoint &ptClient, const Vector &vecWorld);
		void WorldToClient(Vector2D &vecClient, const Vector &vecWorld);
		void WorldToClient(CPoint &point);
		void ClientToWorld(Vector &vecWorld, const CPoint &ptClient);
		void ClientToWorld(CPoint &point);

		bool CheckDistance(const Vector2D &vecCheck, const Vector2D &vecRef, int nDist);

		inline void Translate3D(Vector& pt3, CPoint& pt)
		{
			pt.x = pt3[axHorz] * m_fZoom;
			pt.y = pt3[axVert] * m_fZoom;

			// should use Axes2::PointToScreen, but we want this to be fast
			if(bInvertHorz)
				pt.x = -pt.x;
			if(bInvertVert)
				pt.y = -pt.y;
		}
		
	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMapView2D)
		protected:
		virtual void OnDraw(CDC* pDC);      // overridden to draw this view
		virtual void OnInitialUpdate();     // first time after construct
		virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
		virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
		virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
		virtual BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint point);
		//}}AFX_VIRTUAL

	// Implementation
	protected:
		virtual ~CMapView2D();
	#ifdef _DEBUG
		virtual void AssertValid() const;
		virtual void Dump(CDumpContext& dc) const;
	#endif

	private:

		static HCURSOR m_hcurHand;
		static HCURSOR m_hcurHandClosed;

		// timer IDs:
		enum 
		{ 
			TIMER_SCROLLVIEW = 1, 
			TIMER_MOUSEDRAG,
		};

		void ScreenToWorld(CPoint &pt);

		static BOOL AddToRenderList(CMapClass *pObject, RenderList_t *pRenderList);
		void RenderObjects(CDC *pDC, CUtlVector<CMapClass *> &Objects, int nObjects, CRect rectUpdate);
		void DrawGrid(CDC *pDC);

		void SelectAtCheckHit(CMapClass *pObject, const Vector2D &vecPoint, bool bMakeFirst);

		// general variables:	
		DrawType_t m_DrawType;			// xy/xz/yz
		float m_fZoom;					// zoom factor (* map units)
		CPoint m_ptLDownClient;			// client pos at which lbutton was pressed, for dragging the view
		BOOL m_bLastActiveView;			// is this the last active view?
		CBitmap m_bmpDraw, m_bmpTool;	// draw to this, then blt to screen
		CRect m_rectClient;				// client rect
		CRect m_rectScreen;				// screen rect
		int m_xScroll, m_yScroll;		// amt to scroll on timer
		CRgn m_rgnUpdate;				// region that needs updating
		BOOL m_bToolShown;				// is tool currently visible?
		CTitleWnd *m_pwndTitle;			// title window

		//
		// Color scheme functions.
		//
		COLORREF AdjustColorIntensity(COLORREF ulColor, int nIntensity, bool bReverse);
		void SetColorMode(BOOL bBlackOnWhite);

		//
		// Color scheme variables.
		//
		CPen m_penGrid;
		CPen m_penGrid1024;
		CPen m_penGrid10;
		CPen m_penGrid0;
		COLORREF m_dwDotColor;

		// mouse drag (space + leftbutton):
		CSize m_sizeScrolled;	// amount scrolled (mouse drag) since last update
		enum
		{
			MD_NOTHING,
			MD_DRAG,
		} m_MouseDrag;	// status indicator

		// Generated message map functions
		//{{AFX_MSG(CMapView2D)
		afx_msg void OnView2dxy();
		afx_msg void OnView2dyz();
		afx_msg void OnView2dxz();
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnTimer(UINT nIDEvent);
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnEditProperties();
		afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnUpdateEditFunction(CCmdUI *pCmdUI);
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnPaint();
		afx_msg BOOL OnToolsAlign(UINT nID);
		afx_msg BOOL OnFlip(UINT nID);
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()
};


//-----------------------------------------------------------------------------
// Purpose: Returns the current zoom level, from ZOOM_MIN to ZOOM_MAX.
//-----------------------------------------------------------------------------
float CMapView2D::GetZoom(void)
{
	return m_fZoom;
}


#endif // MAPVIEW2D_H
