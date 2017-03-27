//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "History.h"
#include "MainFrm.h"
#include "MapDefs.h"
#include "MapDoc.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "Options.h"
#include "StatusBarIDs.h"
#include "ToolBlock.h"
#include "ToolManager.h"


class CToolBlockMessageWnd : public CWnd
{
	public:

		bool Create(void);
		void PreMenu2D(CToolBlock *pTool, CMapView2D *pView);

	protected:

		//{{AFX_MSG_MAP(CToolBlockMessageWnd)
		afx_msg void OnCreateObject();
		//}}AFX_MSG
	
		DECLARE_MESSAGE_MAP()

	private:

		CToolBlock *m_pToolBlock;
		CMapView2D *m_pView2D;
};


static CToolBlockMessageWnd s_wndToolMessage;
static const char *g_pszClassName = "ValveEditor_BlockToolWnd";


BEGIN_MESSAGE_MAP(CToolBlockMessageWnd, CWnd)
	//{{AFX_MSG_MAP(CToolMessageWnd)
	ON_COMMAND(ID_CREATEOBJECT, OnCreateObject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Creates the hidden window that receives context menu commands for the
//			block tool.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CToolBlockMessageWnd::Create(void)
{
	WNDCLASS wndcls;
	memset(&wndcls, 0, sizeof(WNDCLASS));
    wndcls.lpfnWndProc   = AfxWndProc;
    wndcls.hInstance     = AfxGetInstanceHandle();
    wndcls.lpszClassName = g_pszClassName;

	if (!AfxRegisterClass(&wndcls))
	{
		return(false);
	}

	return(CWnd::CreateEx(0, g_pszClassName, g_pszClassName, 0, CRect(0, 0, 10, 10), NULL, 0) == TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Attaches the block tool to this window before activating the context
//			menu.
//-----------------------------------------------------------------------------
void CToolBlockMessageWnd::PreMenu2D(CToolBlock *pToolBlock, CMapView2D *pView)
{
	ASSERT(pToolBlock != NULL);
	m_pToolBlock = pToolBlock;
	m_pView2D = pView;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CToolBlockMessageWnd::OnCreateObject()
{
	m_pToolBlock->CreateMapObject(m_pView2D);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CToolBlock::CToolBlock(void)
{
	m_bLButtonDown = false;
	m_ptLDownClient.x = m_ptLDownClient.y = 0;

	// We always show our dimensions when we render.
	SetDrawFlags(GetDrawFlags() | Box3D::boundstext);
	SetDrawColors(Options.colors.clrToolHandle, Options.colors.clrToolBlock);
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CToolBlock::~CToolBlock(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Handles key down events in the 2D view.
// Input  : Per CWnd::OnKeyDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolBlock::OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
		case VK_RETURN:
		{
			if (!IsEmpty())
			{
				CreateMapObject(pView);
			}

			return true;
		}

		case VK_ESCAPE:
		{
			OnEscape();
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Handles context menu events in the 2D view.
// Input  : Per CWnd::OnContextMenu.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolBlock::OnContextMenu2D(CMapView2D *pView, CPoint point) 
{
	static CMenu menu, menuCreate;
	static bool bInit = false;

	if (!bInit)
	{
		bInit = true;

		// Create the menu.
		menu.LoadMenu(IDR_POPUPS);
		menuCreate.Attach(::GetSubMenu(menu.m_hMenu, 1));

		// Create the window that handles menu messages.
		s_wndToolMessage.Create();
	}

	pView->ScreenToClient(&point);

	CRect rect;
	pView->GetClientRect(&rect);
	if (!rect.PtInRect(point))
	{
		return false;
	}

	CPoint ptScreen(point);
	pView->ClientToScreen(&ptScreen);

	CPoint ptMapScreen(point);
	ptMapScreen.x += pView->GetScrollPos(SB_HORZ);
	ptMapScreen.y += pView->GetScrollPos(SB_VERT);

	if (!IsEmpty())
	{
		if (HitTest(ptMapScreen, FALSE) != inclNone)
		{
			s_wndToolMessage.PreMenu2D(this, pView);
			menuCreate.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN, ptScreen.x, ptScreen.y, &s_wndToolMessage);
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button down events in the 2D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolBlock::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point) 
{
	m_bLButtonDown = true;
	m_ptLDownClient = point;

	pView->SetCapture();

	//
	// Convert to some odd coordinate space that the base tools code uses.
	//
  	CPoint ptScreen = point;
	ptScreen.x += pView->GetScrollPos(SB_HORZ);
	ptScreen.y += pView->GetScrollPos(SB_VERT);
	
	//
	// Convert point to world coords.
	//
	Vector vecWorld;
	pView->ClientToWorld(vecWorld, point);

	//
	// If we have a box, see if they clicked on any part of it.
	//
	if (!IsEmpty())
	{
		StartTranslation(ptScreen);
	}
	else
	{
		//
		// Starting a new box. Build a starting point for the drag.
		//
		SetBlockCursor();

		Vector ptOrg(COORD_NOTINIT, COORD_NOTINIT, COORD_NOTINIT);
		ptOrg[axHorz] = vecWorld[axHorz];
		ptOrg[axVert] = vecWorld[axVert];
		m_pDocument->GetBestVisiblePoint(ptOrg);

		// Snap it to the grid.
		if (!(GetAsyncKeyState(VK_MENU) & 0x8000))
		{
			m_pDocument->Snap(ptOrg);
		}
		
		//
		// Start the new box with the extents of the last selected thing.
		//
		BoundBox box;		
		m_pDocument->Selection_GetLastValidBounds(box.bmins, box.bmaxs);
		StartNew(ptOrg, box.bmins, box.bmaxs);
		pView->SetUpdateFlag(CMapView2D::updTool);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button up events in the 2D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolBlock::OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	ReleaseCapture();

	if (IsTranslating())
	{
		m_bLButtonDown = false;
		FinishTranslation(TRUE);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
	}

	m_pDocument->UpdateStatusbar();
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles mouse move events in the 2D view.
// Input  : Per CWnd::OnMouseMove.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolBlock::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point) 
{
	bool bCursorSet = false;
	BOOL bDisableSnap = (GetAsyncKeyState(VK_MENU) & 0x8000) ? TRUE : FALSE;
					    
	//
	// Make sure the point is visible.
	//
	if (m_bLButtonDown)
	{
		pView->ToolScrollToPoint(point);
	}

	//
	// Convert to some odd coordinate space that the base tools code uses.
	//
  	CPoint ptScreen = point;
	ptScreen.x += pView->GetScrollPos(SB_HORZ);
	ptScreen.y += pView->GetScrollPos(SB_VERT);

	//
	// Convert to world coords.
	//
	Vector vecWorld;
	pView->ClientToWorld(vecWorld, point);
	point.x = vecWorld[axHorz];
	point.y = vecWorld[axVert];

	//
	// Update status bar position display.
	//
	char szBuf[128];
	m_pDocument->Snap(vecWorld);
	sprintf(szBuf, " @%.0f, %.0f ", vecWorld[axHorz], vecWorld[axVert]);
	SetStatusText(SBI_COORDS, szBuf);
	
	if (IsTranslating())
	{
		// cursor is cross here
		bCursorSet = true;

		UINT uConstraints = 0;

		if (bDisableSnap)
		{
			uConstraints |= Tool3D::constrainNosnap;
		}

		if (UpdateTranslation(point, uConstraints, CSize(0,0)))
		{
			m_pDocument->ToolUpdateViews(CMapView2D::updTool);
			m_pDocument->Update3DViews();
		}
	}
	else if (!IsEmpty())
	{
		//
		// If the cursor is on a handle, set it to a cross.
		//
		HCURSOR hCursor = NULL;
		if (HitTest(ptScreen, TRUE) != inclNone)
		{
			bCursorSet = true;
		}
	}

	if (!bCursorSet)
	{
		SetBlockCursor();
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles key down events in the 3D view.
// Input  : Per CWnd::OnKeyDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolBlock::OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
		case VK_RETURN:
		{
			if (!IsEmpty())
			{
				//CreateMapObject(pView); dvs: support object creation in 3D views
			}

			return true;
		}

		case VK_ESCAPE:
		{
			OnEscape();
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Per CWnd::OnMouseMove3D.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CToolBlock::OnMouseMove3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	SetBlockCursor();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the escape key in the 2D or 3D views.
//-----------------------------------------------------------------------------
void CToolBlock::OnEscape(void)
{
	//
	// If we have nothing blocked, go back to the pointer tool.
	//
	if (IsEmpty())
	{
		g_pToolManager->SetTool(TOOL_POINTER);
	}
	//
	// We're blocking out a region, so clear it.
	//
	else
	{
		SetEmpty();
		m_pDocument->ToolUpdateViews(CMapView2D::updEraseTool);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders the tool in the 2D view.
//-----------------------------------------------------------------------------
void CToolBlock::RenderTool2D(CRender2D *pRender)
{
	if (IsActiveTool())
	{
		Box3D::RenderTool2D(pRender);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders the tool in the 3D view.
//-----------------------------------------------------------------------------
void CToolBlock::RenderTool3D(CRender3D *pRender)
{
	if (IsActiveTool())
	{
		Box3D::RenderTool3D(pRender);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates a solid in the given 2D view.
//
// TODO: make a generic view-independent version of this function
//-----------------------------------------------------------------------------
void CToolBlock::CreateMapObject(CMapView2D *pView)
{
	CMapWorld *pWorld = m_pDocument->GetMapWorld();
	CMapClass *pobj = NULL;

	if (!(bmaxs[0] - bmins[0]) || !(bmaxs[1] - bmins[1]) || !(bmaxs[2] - bmins[2]))
	{
		AfxMessageBox("The box is empty.");
		SetEmpty();
		return;
	}

	BoundBox NewBox = *this;
	if (Options.view2d.bOrientPrimitives)
	{
		switch (pView->GetDrawType())
		{
			case VIEW2D_XY:
			{
				break;
			}

			case VIEW2D_YZ:
			{
				NewBox.Rotate90(AXIS_Y);
				break;
			}

			case VIEW2D_XZ:
			{
				NewBox.Rotate90(AXIS_X);
				break;
			}
		}
	}

	//
	// Create the object within the given bounding box.
	//
	CMapClass *pObject = GetMainWnd()->m_ObjectBar.CreateInBox(&NewBox, this);
	if (pObject == NULL)
	{
		SetEmpty();
		return;
	}

	m_pDocument->ExpandObjectKeywords(pObject, pWorld);

	GetHistory()->MarkUndoPosition(m_pDocument->Selection_GetList(), "New Object");

	m_pDocument->AddObjectToWorld(pObject);
	pobj = pObject;

	//
	// Do we need to rotate this object based on the view we created it in?
	//
	if (Options.view2d.bOrientPrimitives)
	{
		QAngle angles(0, 0, 0);
		switch (pView->GetDrawType())
		{
			case VIEW2D_XY:
			{
				break;
			}
			
			case VIEW2D_YZ:
			{
				angles[1] = 90.f;
				pObject->TransRotate(NULL, angles);
				break;
			}

			case VIEW2D_XZ:
			{
				angles[0] = 90.f;
				pObject->TransRotate(NULL, angles);
				break;
			}
		}
	}

	GetHistory()->KeepNew(pObject);

	m_pDocument->ToolUpdateViews(CMapView2D::updEraseTool);

	//
	// Select the new object.
	//
	m_pDocument->SelectObject(pobj, CMapDoc::scClear | CMapDoc::scSelect | CMapDoc::scUpdateDisplay);

	//
	// Update all the views.
	//	
	UpdateBox ub;
	CMapObjectList ObjectList;
	ObjectList.AddTail(pobj);
	ub.Objects = &ObjectList;

	Vector mins;
	Vector maxs;
	pobj->GetRender2DBox(mins, maxs);
	ub.Box.SetBounds(mins, maxs);

	m_pDocument->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);

	SetEmpty();

	m_pDocument->SetModifiedFlag();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the cursor to the block cursor.
//-----------------------------------------------------------------------------
void CToolBlock::SetBlockCursor(void)
{
	static HCURSOR hcurBlock;
	
	if (!hcurBlock)
	{
		hcurBlock = LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_NEWBOX));
	}
	
	SetCursor(hcurBlock);
}


