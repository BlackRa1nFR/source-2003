//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "History.h"
#include "MainFrm.h"			// FIXME: For ObjectProperties
#include "MapDoc.h"
#include "MapView2D.h"
#include "MapPointHandle.h"
#include "ObjectProperties.h"	// FIXME: For ObjectProperties::RefreshData
#include "Render2D.h"
#include "StatusBarIDs.h"		// For SetStatusText
#include "ToolManager.h"
#include "ToolPointHandle.h"


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CToolPointHandle::CToolPointHandle(void)
{
	m_pPoint = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Attaches the point to the tool for manipulation.
//-----------------------------------------------------------------------------
void CToolPointHandle::Attach(CMapPointHandle *pPoint)
{
	m_pPoint = pPoint;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left button down events in the 2D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolPointHandle::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	//
	// Activate this tool and start dragging the axis endpoint.
	//
	g_pToolManager->PushTool(TOOL_POINT_HANDLE);
	pView->SetCapture();

	CMapDoc *pDoc = pView->GetDocument();
	GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Modify Origin");
	GetHistory()->Keep(m_pPoint);
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left button up events in the 2D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolPointHandle::OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	m_pPoint->UpdateOrigin(m_pPoint->m_Origin);

	g_pToolManager->PopTool();
	ReleaseCapture();

	CMapDoc *pDoc = pView->GetDocument();
	pDoc->ToolUpdateViews(CMapView2D::updRenderAll);
	GetMainWnd()->pObjectProperties->RefreshData();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles mouse move events in the 2D view.
// Input  : Per CWnd::OnMouseMove.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolPointHandle::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	//
	// Make sure the point is visible.
	//
	pView->ToolScrollToPoint(point);

	//
	// Snap the point to half the grid size. Do this so that we can always center
	// the origin even on odd-width objects.
	//
	Vector vecWorld;
	pView->ClientToWorld(vecWorld, point);

	CMapDoc *pDoc = pView->GetDocument();
	pDoc->Snap(vecWorld, SNAP_HALF_GRID);

	//
	// Move to the snapped position.
	//
	m_pPoint->m_Origin[pView->axHorz] = vecWorld[pView->axHorz];
	m_pPoint->m_Origin[pView->axVert] = vecWorld[pView->axVert];
	
	//
	// Update the status bar and the views.
	//
	char szBuf[128];
	sprintf(szBuf, " @%.0f, %.0f ", m_pPoint->m_Origin[pView->axHorz], m_pPoint->m_Origin[pView->axVert]);
	SetStatusText(SBI_COORDS, szBuf);

	pDoc->ToolUpdateViews(CMapView2D::updTool);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Renders the tool in the 2D view.
// Input  : pRender - The interface to use for rendering.
//-----------------------------------------------------------------------------
void CToolPointHandle::RenderTool2D(CRender2D *pRender)
{
	if (!IsActiveTool())
	{
		return;
	}

	SelectionState_t eState = m_pPoint->SetSelectionState(SELECT_MODIFY);
	m_pPoint->Render2D(pRender);
	m_pPoint->SetSelectionState(eState);
}


