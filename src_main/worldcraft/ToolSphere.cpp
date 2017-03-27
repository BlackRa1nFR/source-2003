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
#include "MapSphere.h"
#include "ObjectProperties.h"	// FIXME: For ObjectProperties::RefreshData
#include "StatusBarIDs.h"		// For updating status bar text
#include "ToolManager.h"
#include "ToolSphere.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CToolSphere::CToolSphere()
{
	m_pSphere = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pSphere - 
//-----------------------------------------------------------------------------
void CToolSphere::Attach(CMapSphere *pSphere)
{
	m_pSphere = pSphere;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			nFlags - 
//			point - 
// Output : Returns true if the message was handled, false otherwise.
//-----------------------------------------------------------------------------
bool CToolSphere::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	//
	// Activate this tool and start resizing the sphere.
	//
	g_pToolManager->PushTool(TOOL_SPHERE);
	pView->SetCapture();

	CMapDoc *pDoc = pView->GetDocument();
	GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Modify Radius");
	GetHistory()->Keep(m_pSphere);
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			nFlags - 
//			point - 
// Output : Returns true if the message was handled, false otherwise.
//-----------------------------------------------------------------------------
bool CToolSphere::OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	g_pToolManager->PopTool();

	ReleaseCapture();

	CMapDoc *pDoc = pView->GetDocument();
	pDoc->ToolUpdateViews(CMapView2D::updRenderAll);

	GetMainWnd()->pObjectProperties->RefreshData();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			nFlags - 
//			point - 
// Output : Returns true if the message was handled, false otherwise.
//-----------------------------------------------------------------------------
bool CToolSphere::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	CMapDoc *pDoc = pView->GetDocument();

	//
	// Make sure the point is visible.
	//
	pView->ToolScrollToPoint(point);

	pView->ClientToWorld(point);
	pDoc->Snap(point);

	//
	// Use whichever axis they dragged the most along as the drag axis.
	// Calculate the drag distance in client coordinates.
	//
	float flHorzRadius = fabs((float)point.x - m_pSphere->m_Origin[pView->axHorz]);
	float flVertRadius = fabs((float)point.y - m_pSphere->m_Origin[pView->axVert]);
	float flRadius = max(flHorzRadius, flVertRadius);
	
	m_pSphere->SetRadius(flRadius);

	//
	// Update the status bar text with the new value of the radius.
	//
	char szBuf[128];
	sprintf(szBuf, " %s = %g ", m_pSphere->m_szKeyName, (double)m_pSphere->m_flRadius);
	SetStatusText(SBI_COORDS, szBuf);

	pDoc->ToolUpdateViews(CMapView2D::updRenderAll);

	return true;
}

