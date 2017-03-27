//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "MapDoc.h"
#include "MapView2D.h"
#include "resource.h"
#include "ToolMagnify.h"


static HCURSOR s_hcurMagnify = NULL;


//-----------------------------------------------------------------------------
// Purpose: Loads the cursor (only once).
//-----------------------------------------------------------------------------
CToolMagnify::CToolMagnify(void)
{
	if (s_hcurMagnify == NULL)
	{
		s_hcurMagnify = LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_MAGNIFY));
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pView - 
//			nFlags - 
//			point - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CToolMagnify::OnContextMenu2D(CMapView2D *pView, CPoint point)
{
	// Return true to suppress the default view context menu behavior.
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			nFlags - 
//			point - 
// Output : Returns true to indicate that the message was handled.
//-----------------------------------------------------------------------------
bool CToolMagnify::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	pView->SetZoom(pView->GetZoom() * 2);
	pView->Invalidate();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			nFlags - 
//			point - 
// Output : Returns true to indicate that the message was handled.
//-----------------------------------------------------------------------------
bool CToolMagnify::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	SetCursor(s_hcurMagnify);
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			nFlags - 
//			point - 
// Output : Returns true to indicate that the message was handled.
//-----------------------------------------------------------------------------
bool CToolMagnify::OnRMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	pView->SetZoom(pView->GetZoom() * 0.5f);
	pView->Invalidate();
	return true;
}


