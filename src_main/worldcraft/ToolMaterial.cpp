//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Handles the selection of faces and material application.
//
//			TODO: consider making face selection a mode of the selection tool
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "FaceEditSheet.h"
#include "History.h"
#include "MainFrm.h"
#include "MapDoc.h"
#include "MapSolid.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "StatusBarIDs.h"
#include "ToolMaterial.h"


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			nFlags - 
//			point - 
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolMaterial::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	CMapDoc *pDoc = pView->GetDocument();
	CMapWorld *pWorld = pDoc->GetMapWorld();

	if (nFlags & MK_CONTROL)
	{
		//
		// CONTROL is down, perform selection only.
		//
		pView->SelectAt(point, FALSE, true);
	}
	else
	{
		pView->SelectAt(point, TRUE, true);
	}

	return (true);
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button down events in the 3D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolMaterial::OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point) 
{
	CMapDoc *pDoc = pView->GetDocument();
	if (pDoc == NULL)
	{
		return false;
	}

	bool bShift = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);

	ULONG ulFace;
	CMapClass *pObject = pView->NearestObjectAt(point, ulFace);

	if ((pObject != NULL) && (pObject->IsMapClass(MAPCLASS_TYPE(CMapSolid))))
	{
		CMapSolid *pSolid = (CMapSolid *)pObject;

		int cmd = CMapDoc::scToggle | CMapDoc::scClear | CMapDoc::scUpdateDisplay;

		// No clear if CTRL pressed.
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		{
			cmd &= ~CMapDoc::scClear;	
		}

		// If they are holding down SHIFT, select the entire solid.
		if (bShift)
		{
			pDoc->SelectFace(pSolid, -1, cmd);
		}
		// Otherwise, select a single face.
		else
		{
			pDoc->SelectFace(pSolid, ulFace, cmd);
		}
	}

	// Update the controls given new information (ie. new faces).
	GetMainWnd()->m_pFaceEditSheet->UpdateControls();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles right mouse button down events in the 3D view.
// Input  : Per CWnd::OnRButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool CToolMaterial::OnRMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point) 
{
	BOOL bShift = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
	BOOL bEdgeAlign = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);
	
	ULONG ulFace;
	CMapClass *pObject = pView->NearestObjectAt(point, ulFace);

	if (pObject != NULL)
	{
		if (pObject->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
		{
			CMapSolid *pSolid = (CMapSolid *)pObject;
			GetHistory()->MarkUndoPosition(NULL, "Apply texture");
			GetHistory()->Keep(pSolid);
			
			// Setup the flags.
			int cmdFlags = 0;
			if(bEdgeAlign)
				cmdFlags |= CFaceEditSheet::cfEdgeAlign;
			
			// If they are holding down the shift key, apply to the entire solid.
			if (bShift)
			{
				int nFaces = pSolid->GetFaceCount();
				for(int i = 0; i < nFaces; i++)
				{
					GetMainWnd()->m_pFaceEditSheet->ClickFace( pSolid, i, cmdFlags, CFaceEditSheet::ModeApplyAll );
				}
			}
			// If not, apply to a single face.
			else
			{
				GetMainWnd()->m_pFaceEditSheet->ClickFace( pSolid, ulFace, cmdFlags, CFaceEditSheet::ModeApplyAll );
			}
		}				
	}
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the mouse move message in the 3D view.
// Input  : Per CWnd::OnMouseMove.
//-----------------------------------------------------------------------------
bool CToolMaterial::OnMouseMove3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	//
	// Manage the cursor.
	//
	static HCURSOR hcurFacePaint = 0;
	if (!hcurFacePaint)
	{
		hcurFacePaint = LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_FACEPAINT));
	}

	SetCursor(hcurFacePaint);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CToolMaterial::UpdateStatusBar()
{
	CString str;
	str.Format("%d faces selected", GetMainWnd()->m_pFaceEditSheet->GetFaceListCount() );
	SetStatusText(SBI_SELECTION, str);
	SetStatusText(SBI_SIZE, "");
}

