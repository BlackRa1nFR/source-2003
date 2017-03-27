//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "GameData.h"
#include "Gizmo.h"
#include "GlobalFunctions.h"		// FIXME: For NotifyDuplicates
#include "History.h"
#include "MainFrm.h"
#include "MapDoc.h"
#include "MapDefs.h"
#include "MapEntity.h"
#include "MapPointHandle.h"
#include "MapSolid.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "ObjectProperties.h"
#include "Options.h"
#include "Render2D.h"
#include "ToolSelection.h"
#include "StatusBarIDs.h"
#include "ToolManager.h"
#include "Worldcraft.h"


#pragma warning(disable:4244)


static CDC *pDrawDC = NULL;
static CBrush brDraw;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Selection3D::Selection3D(void)
{
	UpdateBounds();

	//
	// The block tool uses our bounds as the default size when starting a new
	// box. Set to reasonable defaults to begin with.
	//
	bmins = Vector(0, 0, 0);
	bmaxs = Vector(64, 64, 64);

	bBoxSelection = FALSE;

	m_bLButtonDown = false;
	m_bLeftDragged = false;
	
	m_eHandleMode = Options.GetShowHelpers() ? HandleMode_Both : HandleMode_SelectionOnly;

	m_bEyedropper = false;

	m_bSelected = false;

	m_eSelectMode = selectGroups;

	m_ptLDownClient.x = m_ptLDownClient.y = 0;

	SetDrawFlags(Box3D::expandbox);
	SetDrawColors(Options.colors.clrToolHandle, Options.colors.clrToolSelection);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Selection3D::~Selection3D(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Called when the tool is activated.
// Input  : eOldTool - The ID of the previously active tool.
//-----------------------------------------------------------------------------
void Selection3D::OnActivate(ToolID_t eOldTool)
{
	EnableHandles(true);

	if (GetCount() != 0)
	{
		UpdateBox ub;
		GetBounds(ub.Box.bmins, ub.Box.bmaxs);
		m_pDocument->UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY, &ub);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when the tool is deactivated.
// Input  : eNewTool - The ID of the tool that is being activated.
//-----------------------------------------------------------------------------
void Selection3D::OnDeactivate(ToolID_t eNewTool)
{
	EnableHandles(false);

	if (GetCount() != 0)
	{
		UpdateBox ub;
		GetBounds(ub.Box.bmins, ub.Box.bmaxs);
		m_pDocument->UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_DISPLAY, &ub);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Enables or disables the selection handles based on the current
//			state of the tool.
//-----------------------------------------------------------------------------
void Selection3D::UpdateHandleState(void)
{
	if (!IsActiveTool())
	{
		EnableHandles(false);
	}
	else
	{
		EnableHandles(true);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - The view that invoked the eyedropper.
//			VarList - 
// Output : 
//-----------------------------------------------------------------------------
GDinputvariable *Selection3D::ChooseEyedropperVar(CMapView *pView, CUtlVector<GDinputvariable *> &VarList)
{
	//
	// Build a popup menu containing all the variable names.
	//
	CMenu menu;
	menu.CreatePopupMenu();
	int nVarCount = VarList.Count();
	for (int nVar = 0; nVar < nVarCount; nVar++)
	{
		GDinputvariable *pVar = VarList.Element(nVar);
		menu.AppendMenu(MF_STRING, nVar + 1, pVar->GetLongName());
	}

	//
	// Invoke the popup menu.
	//
	CPoint point;
	GetCursorPos(&point);
	int nID = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, pView, NULL);
	if (nID == 0)
	{
		return NULL;
	}

	return VarList.Element(nID - 1);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Selection3D::SetHandleMode(SelectionHandleMode_t eHandleMode)
{
	m_eHandleMode = eHandleMode;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			bValidOnly - 
// Output : Returns the handle under the given point, -1 if there is none.
//-----------------------------------------------------------------------------
int Selection3D::HitTest(CPoint pt, BOOL bValidOnly)
{
	if (!IsEmpty())
	{
		return Box3D::HitTest(pt, bValidOnly);
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool Selection3D::IsAnEntitySelected(void)
{
	if (m_SelectionList.Count() > 0)
	{
		int nSelCount = m_SelectionList.Count();
		for (int i = 0; i < nSelCount; i++)
		{
			CMapClass *pObject = m_SelectionList.Element(i);
			CMapEntity *pEntity = dynamic_cast <CMapEntity *> (pObject);
			if (pEntity != NULL)
			{
				return true;
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Selection3D::SetEmpty(void)
{
	int nSelCount = m_SelectionList.Count();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pobj = m_SelectionList.Element(i);
		pobj->SetSelectionState(SELECT_NONE);
	}

	m_SelectionList.RemoveAll();

	stat.TranslateMode = modeScale;
	ZeroVector(m_Scale);

	UpdateBounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
BOOL Selection3D::IsEmpty(void)
{
	return (bBoxSelection || m_SelectionList.Count()) ? FALSE : TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Removes objects that are not visible from the selection set.
//-----------------------------------------------------------------------------
void Selection3D::RemoveInvisibles(void)
{
	//
	// Build a list of the visible objects and keep track of whether or
	// not we encounter any invisible objects.
	//
	bool bFoundOne = false;
	CMapObjectList NewSelection;

	int nSelCount = m_SelectionList.Count();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pobj = m_SelectionList.Element(i);
		if (pobj->IsVisible())
		{
			NewSelection.AddTail(pobj);
		}
		else
		{
			bFoundOne = true;
			pobj->SetSelectionState(SELECT_NONE);
		}
	}

	//
	// If any invisible objects were found, replace the selection list
	// with only the visible objects.
	//
	if (bFoundOne)
	{
		m_SelectionList.RemoveAll();

		POSITION pos = NewSelection.GetHeadPosition();
		while (pos != NULL)
		{
			CMapClass *pObject = NewSelection.GetNext(pos);
			m_SelectionList.AddToTail(pObject);
		}
	}

	UpdateBounds();
}


//-----------------------------------------------------------------------------
// Purpose: Adds the object to the selection set.
//-----------------------------------------------------------------------------
void Selection3D::Add(CMapClass *pobj)
{
	Vector mins;
	Vector maxs;
	pobj->GetRender2DBox(mins, maxs);
	BoundBox::UpdateBounds(mins, maxs);

	UpdateSavedBounds();

	//
	// Add object to the selection list and update its state.
	//
	m_SelectionList.AddToTail(pobj);
	pobj->SetSelectionState(SELECT_NORMAL);

	//
	// EXPERIMENT: look for helpers that implement the CBaseTool interface and
	// add them to a seperate list for hit testing.
	//
//	EnumChildrenPos_t pos;
//	CMapClass *pChild = pobj->GetFirstDescendent(pos);
//	while (pChild != NULL)
//	{
//		if ((dynamic_cast <CBaseTool *> (pChild)) != NULL)
//		{
//			m_ToolHelperList.AddToTail(pChild);
//		}
//		pChild = pobj->GetNextDescendent(pos);
//	}

	UpdateHandleState();
}


//-----------------------------------------------------------------------------
// Purpose: FIXME: Interim function to support old code that expects the
//				   selection list to be exposed as a CMapObjectList.
//
//			This doesn't work completely because if the selection changes after
//			this function is called, the returned list won't reflect the changes.
//-----------------------------------------------------------------------------
CMapObjectList *Selection3D::GetList(void)
{
	static CMapObjectList TempList;
	TempList.RemoveAll();

	int nSelCount = m_SelectionList.Count();
	for (int i = 0; i < nSelCount; i++)
	{
		TempList.AddTail(m_SelectionList.Element(i));
	}

	return &TempList;
}


//-----------------------------------------------------------------------------
// Purpose: Returns TRUE if the object is in the selection list, FALSE if not.
//-----------------------------------------------------------------------------
bool Selection3D::IsSelected(CMapClass *pobj)
{
	return (m_SelectionList.Find(pobj) != -1);
}


//-----------------------------------------------------------------------------
// Purpose: Removes the object from the selection set.
//-----------------------------------------------------------------------------
void Selection3D::Remove(CMapClass *pobj)
{
	int nIndex = m_SelectionList.Find(pobj);
	if (nIndex != -1)
	{
		m_SelectionList.Remove(nIndex);
		pobj->SetSelectionState(SELECT_NONE);
		UpdateBounds();
		UpdateHandleState();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void Selection3D::UpdateBounds( void )
{
	ResetBounds();

	int nSelCount = m_SelectionList.Count();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pobj = m_SelectionList.Element(i);
		Vector mins;
		Vector maxs;
		pobj->GetRender2DBox(mins, maxs);
		BoundBox::UpdateBounds(mins, maxs);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bSave - 
//-----------------------------------------------------------------------------
void Selection3D::FinishTranslation(BOOL bSave)
{
	Box3D::FinishTranslation(bSave);

	if (IsBoxSelecting())
	{
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt3 - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Selection3D::StartBoxSelection(Vector &vecStart)
{
	bBoxSelection = TRUE;
	SetDrawColors(Options.colors.clrToolHandle, RGB(50, 255, 255));

	CPoint pt(vecStart[axHorz], vecStart[axVert]);
	Tool3D::StartTranslation(pt);
	SetDrawFlags(GetDrawFlags() | Box3D::boundstext);
	
	bPreventOverlap = FALSE;
	stat.TranslateMode = modeScale;
	stat.bNewBox = TRUE;
	tbmins[axThird] = m_pDocument->Snap(vecStart[axThird]);
	tbmaxs[axThird] = tbmins[axThird];	// infinite box first
	tbmins[axHorz] = tbmaxs[axHorz] = pt.x;
	tbmins[axVert] = tbmaxs[axVert] = pt.y;
	iTransOrigin = inclBottomRight;
	ZeroVector(m_Scale);
	bmins = tbmins;
	bmaxs = tbmaxs;

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Selection3D::EndBoxSelection()
{
	SetDrawColors(Options.colors.clrToolHandle, Options.colors.clrToolSelection);
	bBoxSelection = FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : point - 
//			bMoveOnly - 
// Output : 
//-----------------------------------------------------------------------------
bool Selection3D::StartSelectionTranslation(CPoint &point, bool bMoveOnly)
{
	const Vector *pvecReference = NULL;
	
	//
	// If we are just moving one point entity, use its origin as the reference point.
	//
	if (m_SelectionList.Count() == 1)
	{
		CMapEntity *pObject = (CMapEntity *)m_SelectionList.Element(0);

		if (pObject->IsMapClass(MAPCLASS_TYPE(CMapEntity)) && pObject->IsPlaceholder())
		{
			//
			// It is a point entity.
			//
			Vector Origin;
			pObject->GetOrigin(Origin);
			pvecReference = &Origin;
		}
	}

	EnableHandles(!bMoveOnly);

	if (!StartTranslation(point, pvecReference))
	{
		EnableHandles(true);
		return false;
	}

	EnableHandles(true);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Selection3D::TransformSelection(void)
{
	//
	// Save any properties that may have been changed in the entity properties dialog.
	// This prevents the LoadData below from losing any changes that were made in the
	// object properties dialog.
	//
	GetMainWnd()->pObjectProperties->SaveData();

	//
	// Transform the selected objects.
	//
	int nSelCount = m_SelectionList.Count();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pobj = m_SelectionList.Element(i);

		Vector mins;
		Vector maxs;

		pobj->GetRender2DBox(mins, maxs);
		TotalArea.UpdateBounds(mins, maxs);

		pobj->Transform(this);

		pobj->GetRender2DBox(mins, maxs);
		TotalArea.UpdateBounds(mins, maxs);
	}

	//
	// The transformation may have changed some entity properties (such as the "angles" key),
	// so we must refresh the Object Properties dialog.
	//
	GetMainWnd()->pObjectProperties->RefreshData();

	UpdateBounds();
}


//-----------------------------------------------------------------------------
// Purpose: Draws objects when they are selected. Odd, how this code is stuck
//			in this obscure place, away from all the other 2D rendering code.
// Input  : pobj - Object to draw.
//			pSel - 
// Output : Returns TRUE to keep enumerating.
//-----------------------------------------------------------------------------
BOOL Selection3D::DrawObject(CMapClass *pobj, Selection3D *pSel)
{
	// TODO: I'd like to let objects render themselves with a selection state of SELECT_MODIFY,
	//       rather than having the selection tool do it, but that requires us to perform the
	//       operation on the object during MouseMove instead of on button up. The only way to
	//		 do that and prevent error creep would be to always start with the object's original
	//		state, which we can't do easily right now.
	CRender2D *pRender = pSel->m_pRender;

	POINT pt;
	static CRect re;

	bool bTranslating = pSel->IsTranslating() && !pSel->IsBoxSelecting();
	bool bDrawVertices = (Options.view2d.bDrawVertices == TRUE);

	if (pobj->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
	{
		CMapSolid* pSolid = (CMapSolid *)pobj;

		Vector ptTmp;
		pSolid->GetBoundsCenter(ptTmp);

		// draw "X"
		if (bTranslating)
		{
			pSel->TranslatePoint(ptTmp);
		}
		pSel->PointToScreen(ptTmp, pt);
		pRender->MoveTo(pt.x - 3, pt.y - 3);
		pRender->LineTo(pt.x + 4, pt.y + 4);
		pRender->MoveTo(pt.x + 3, pt.y - 3);
		pRender->LineTo(pt.x - 4, pt.y + 4);

		// draw faces
		int nFaces = pSolid->GetFaceCount();
		for (int face = 0; face < nFaces; face++)
		{
			CMapFace *pFace = pSolid->GetFace(face);
			Vector *pts = pFace->Points;

			if (!pts)
			{
				continue;
			}

			ptTmp = pts[0];
			if (bTranslating)
			{
				pSel->TranslatePoint(ptTmp);
			}

			pSel->PointToScreen(ptTmp, pt);
			pRender->MoveTo(pt);

			// draw lines in face
			++pts;
			int nPoints = pFace->nPoints;
			for (int point = 1; point <= nPoints; point++, pts++)
			{
				if (bDrawVertices)
				{
					// draw vertex
					re.left = pt.x - 1; re.right = pt.x + 2;
					re.top = pt.y - 1; re.bottom = pt.y + 2;
					pRender->SetFillColor(255, 0, 0);
					pRender->Rectangle(&re, true);
				}

				// last line
				if (point == nPoints)
				{
					pts = pFace->Points;
				}

				ptTmp = pts[0];
				if (bTranslating)
				{
					pSel->TranslatePoint(ptTmp);
				}
				pSel->PointToScreen(ptTmp, pt);
				pRender->LineTo(pt);
			}
		}
	}
	else if (pobj->IsMapClass(MAPCLASS_TYPE(CMapEntity)))
	{
		CMapEntity *pEntity = (CMapEntity *)pobj;

		if (pEntity->IsPlaceholder())
		{
			//
			// Draw 'X' at entity's origin.
			//
			CPoint pt;
			Vector Origin;

			pEntity->GetOrigin(Origin);
			pSel->PointToScreen(Origin, pt);

			pRender->MoveTo(pt.x - 7, pt.y - 7);
			pRender->LineTo(pt.x + 8, pt.y + 8);
			pRender->MoveTo(pt.x + 7, pt.y - 7);
			pRender->LineTo(pt.x - 8, pt.y + 8);
		}
	}
	else if (pobj->IsMapClass(MAPCLASS_TYPE(CMapPointHandle)))
	{
		if (bTranslating)
		{
			Vector vecOrigin;
			pobj->GetOrigin(vecOrigin);
			pSel->TranslatePoint(vecOrigin);

			pRender->SetLineType(CRender2D::LINE_DOT, CRender2D::LINE_THIN, GetRValue(Options.colors.clrSelection), GetGValue(Options.colors.clrSelection), GetBValue(Options.colors.clrSelection));

			CPoint ptClientOrigin;
			pRender->TransformPoint3D(ptClientOrigin, vecOrigin);

			pRender->DrawEllipse(ptClientOrigin, HANDLE_RADIUS, HANDLE_RADIUS, false);
			pRender->DrawEllipse(ptClientOrigin, 1, 1, false);
		}
	}
//	else
//	{
//		pobj->Render2D(pRender);
//	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRender - 
//-----------------------------------------------------------------------------
void Selection3D::RenderTool2D(CRender2D *pRender)
{
	Box3D::RenderTool2D(pRender);

	int nSelCount = m_SelectionList.Count();
	if (nSelCount != 0)
	{
		//
		// Even if this is not the active tool, selected objects should be rendered
		// with the selection color.
		//
		COLORREF clr = Options.colors.clrSelection;
		pRender->SetLineType(CRender2D::LINE_DOT, CRender2D::LINE_THIN, GetRValue(clr), GetGValue(clr), GetBValue(clr));
		pRender->SetFillColor(GetRValue(clr), GetGValue(clr), GetBValue(clr));

		m_pRender = pRender;

		for (int i = 0; i < nSelCount; i++)
		{
			CMapClass *pobj = m_SelectionList.Element(i);
			DrawObject(pobj, this);
			pobj->EnumChildren((ENUMMAPCHILDRENPROC)DrawObject, (DWORD)this);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes objects that have been deleted from the world but are still
//			in the selection list.
// FIXME: does this ever do anything? enable the assert and see if it gets hit
//-----------------------------------------------------------------------------
void Selection3D::RemoveDead(void)
{
	bool bFoundOne;

	do
	{
		bFoundOne = false;

		int nSelCount = m_SelectionList.Count();
		for (int i = 0; i < nSelCount; i++)
		{
			CMapClass *pObject = m_SelectionList.Element(i);
			if (!pObject->GetParent())
			{
				//ASSERT(false);
				m_SelectionList.Remove(i);
				bFoundOne = true;
				break;
			}
		}
	} while (bFoundOne);
}


//-----------------------------------------------------------------------------
// Purpose: Renders a selection gizmo at our bounds center.
// Input  : pRender - 
//-----------------------------------------------------------------------------
void Selection3D::RenderTool3D(CRender3D *pRender)
{
	if (m_SelectionList.Count() == 0)
	{
		Box3D::RenderTool3D(pRender);
	}
	/* dvs: need realtime preview of brush resizing.
	else
	{
		//
		// Setup the renderer.
		//
		RenderMode_t eDefaultRenderMode = pRender->GetDefaultRenderMode();
		pRender->SetDefaultRenderMode(RENDER_MODE_WIREFRAME);
		pRender->RenderEnable(RENDER_POLYGON_OFFSET_LINE, true);
    
		POSITION pos = Selection.GetHeadPosition();
		while (pos != NULL)
		{
			CMapClass *pObject = Selection.GetNext(pos);

			unsigned int dwColor = pObject->GetRenderColor();
			pObject->SetRenderColor(255, 255, 255);
			pObject->Render3D(pRender);
			pObject->SetRenderColor(dwColor);
		}

		pRender->RenderEnable(RENDER_POLYGON_OFFSET_LINE, false);
		pRender->SetDefaultRenderMode(eDefaultRenderMode);
	}

	if (!IsEmpty())
	{
		static CGizmo Gizmo;
		float fCenter[3];

		GetBoundsCenter(fCenter);
		Gizmo.SetAxisLength(50);
		Gizmo.SetPosition(fCenter[0], fCenter[1], fCenter[2]);
		Gizmo.Render(pRender);
	}
	*/
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			point - 
//-----------------------------------------------------------------------------
bool Selection3D::OnContextMenu2D(CMapView2D *pView, CPoint point) 
{
	static CMenu menu, menuSelection;
	static bool bInit = false;

	if (!bInit)
	{
		bInit = true;
		menu.LoadMenu(IDR_POPUPS);
		menuSelection.Attach(::GetSubMenu(menu.m_hMenu, 0));
	}

	pView->ScreenToClient(&point);

	CRect rect;
	pView->GetClientRect(&rect);
	if (!rect.PtInRect(point))
	{
		return false;
	}

	CPoint ptScreen(point);
	CPoint ptMapScreen(point);
	pView->ClientToScreen(&ptScreen);
	pView->ClientToWorld(point);

	ptMapScreen.x += pView->GetScrollPos(SB_HORZ);
	ptMapScreen.y += pView->GetScrollPos(SB_VERT);

	if (!IsEmpty() && !IsBoxSelecting())
	{
		if (HitTest(ptMapScreen, FALSE) != -1)
		{
			menuSelection.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN, ptScreen.x, ptScreen.y, pView);
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Selection3D::SelectInBox(CMapDoc *pDoc, bool bInsideOnly)
{
	BoundBox box(*this);
	EndBoxSelection();

	//
	// Make selection box "infinite" in 0-depth axes, of which there
	// should not be more than 1.
	//
	int countzero = 0;
	for(int i = 0; i < 3; i++)
	{
		if (box.bmaxs[i] == box.bmins[i])
		{
			box.bmins[i] = -COORD_NOTINIT;
			box.bmaxs[i] = COORD_NOTINIT;
			++countzero;
		}
	}

	if (countzero <= 1)
	{
		pDoc->SelectRegion(&box, bInsideOnly);
	}

	UpdateBounds();
	pDoc->ToolUpdateViews(CMapView2D::updTool);
}


//-----------------------------------------------------------------------------
// Purpose: Handles key down events in the 2D view.
// Input  : Per CWnd::OnKeyDown.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Selection3D::OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CMapDoc *pDoc = pView->GetDocument();
	if (pDoc == NULL)
	{
		return false;
	}

	bool bShift = ((GetKeyState(VK_SHIFT) & 0x8000) != 0);
	bool bCtrl = ((GetKeyState(VK_CONTROL) & 0x8000) != 0);

	if (Options.view2d.bNudge && (nChar == VK_UP || nChar == VK_DOWN || nChar == VK_LEFT || nChar == VK_RIGHT))
	{
		if (!IsEmpty())
		{
			pDoc->NudgeObjects(*pView, nChar, !bCtrl);
			return true;
		}
	}

	switch (nChar)
	{
		// TODO: do we want this here or in the view?
		case VK_DELETE:
		{
			pDoc->OnCmdMsg(ID_EDIT_DELETE, CN_COMMAND, NULL, NULL);
			break;
		}

		case VK_NEXT:
		{
			pDoc->OnCmdMsg(ID_EDIT_SELNEXT, CN_COMMAND, NULL, NULL);
			break;
		}

		case VK_PRIOR:
		{
			pDoc->OnCmdMsg(ID_EDIT_SELPREV, CN_COMMAND, NULL, NULL);
			break;
		}

		case VK_ESCAPE:
		{
			OnEscape(pDoc);
			break;
		}

		case VK_RETURN:
		{
			if (IsBoxSelecting())
			{
				SelectInBox(pDoc, bShift);
			}
			break;
		}

		default:
		{
			return false;
		}
	}
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left button down events in the 2D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Selection3D::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	bool bShift = ((GetKeyState(VK_SHIFT) & 0x8000) != 0);

	//
	// If tool helpers are enabled, first give any selected tool helpers a chance
	// to handle the message. Don't hit test against tool helpers when shift is held
	// down (beginning a Clone operation).
	//
	if (!bShift && (m_eHandleMode != HandleMode_SelectionOnly))
	{
		Vector2D vecPoint(point.x, point.y);
 		int nSelCount = m_SelectionList.Count();
		for (int i = 0; i < nSelCount; i++)
		{
			CMapClass *pObject = m_SelectionList.Element(i);

			//
			// Hit test against the object. nHitData will return with object-specific
			// information about what was clicked on.
			//
			int nHitData;
			CMapClass *pHit = pObject->HitTest2D(pView, vecPoint, nHitData);
			if (pHit != NULL)
			{
				//
				// They clicked on some part of the object. See if there is a
				// tool associated with what we clicked on.
				//
				CBaseTool *pToolHit = pHit->GetToolObject(nHitData);
				if (pToolHit)
				{
					//
					// There is a tool. Attach the object to the tool and forward
					// the message to the tool.
					//
					return pToolHit->OnLMouseDown2D(pView, nFlags, point);
				}
			}
		}
	}

	CMapDoc *pDoc = pView->GetDocument();
	if (pDoc == NULL)
	{
		return false;
	}

	CMapWorld *pWorld = pDoc->GetMapWorld();

	m_bLButtonDown = true;
	m_ptLDownClient = point;

	pView->SetCapture();

	//
	// Convert to some odd coordinate space that the base tools code uses.
	//
  	CPoint ptHitTest = point;
	ptHitTest.x += pView->GetScrollPos(SB_HORZ);
	ptHitTest.y += pView->GetScrollPos(SB_VERT);

	m_bLeftDragged = false;
	m_bSelected = false;

	if (IsBoxSelecting() && StartTranslation(ptHitTest))
	{
		return true;
	}
	else
	{
		if (IsBoxSelecting())
		{
			pDoc->ToolUpdateViews(CMapView2D::updEraseTool);
			EndBoxSelection();
		}

		if (nFlags & MK_CONTROL)
		{
			//
			// CONTROL is down, perform selection only.
			//
			m_bSelected = (pView->SelectAt(point, FALSE, false) == TRUE);
		}
		else if (!IsEmpty())
		{
			if (!StartSelectionTranslation(ptHitTest, false))
			{
				//
				// No translation started - but it might still be inside the
				// selection because only the scale/move mode pays attention
				// to inside. If it IS inside, do NOT perform selection.
				//
				if (HitTest(ptHitTest, FALSE) == -1)
				{
					//
					// They didn't click on a handle or inside the selected region, try to select
					// whatever they clicked on.
					//
					m_bSelected = (pView->SelectAt(point, !(nFlags & MK_CONTROL), false) == TRUE);
				}
				else
				{
					SetDrawFlags(GetDrawFlags() | Box3D::boundstext);
				}
			}
			else
			{
				SetDrawFlags(GetDrawFlags() | Box3D::boundstext);
			}
		}
		else
		{
			m_bSelected = (pView->SelectAt(point, TRUE, false) == TRUE);
		}
	}

	return (true);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the constraints flags for the translation.
// Input  : bDisableSnap - 
//			nKeyFlags - 
//-----------------------------------------------------------------------------
unsigned int Selection3D::GetConstraints(bool bDisableSnap, int nKeyFlags)
{
	unsigned int uConstraints = 0;

	if (bDisableSnap)
	{
		uConstraints |= Tool3D::constrainNosnap;
	}
	else if (m_SelectionList.Count() == 1)
	{
		CMapClass *pObject = m_SelectionList.Element(0);

		if (pObject->ShouldSnapToHalfGrid())
		{
			uConstraints |= Tool3D::constrainHalfSnap;
		}
	}

	if (Options.view2d.bRotateConstrain)
	{
		uConstraints |= Box3D::constrain15;
	}

	//
	// The shift key toggles the rotation constraint.
	//
	if (GetTranslateMode() == Box3D::modeRotate)
	{
		if (nKeyFlags & MK_SHIFT)
		{
			if (Options.view2d.bRotateConstrain)
			{
				// Disable 15 degree snap.
				uConstraints &= ~Box3D::constrain15;
			}
			else
			{
				// Enable 15 degree snap.
				uConstraints |= Box3D::constrain15;
			}
		}
	}

	if (nKeyFlags & MK_CONTROL)
	{
		uConstraints |= Box3D::constrainKeepScale;
	}

	return uConstraints;
}


//-----------------------------------------------------------------------------
// Purpose: Handles mouse move events in the 2D view.
// Input  : Per CWnd::OnMouseMove.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Selection3D::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point) 
{
	CPoint ptClient = point;

	CMapDoc *pDoc = pView->GetDocument();
	if (pDoc == NULL)
	{
		return false;
	}

	bool bCursorSet = false;
	bool bDisableSnap = (GetAsyncKeyState(VK_MENU) & 0x8000) ? true : false;
					    
	CSize sizeDragged = point - m_ptLDownClient;
	int const DRAG_THRESHHOLD = 1;
	if ((abs(sizeDragged.cx) > DRAG_THRESHHOLD) || (abs(sizeDragged.cy) > DRAG_THRESHHOLD))
	{
		// If here, means we've dragged the mouse
		m_bLeftDragged = true;
	}
	
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
  	CPoint ptHitTest = point;
	ptHitTest.x += pView->GetScrollPos(SB_HORZ);
	ptHitTest.y += pView->GetScrollPos(SB_VERT);

	//
	// Convert to world coords.
	//
	Vector vecWorld;
	pView->ClientToWorld(vecWorld, point);

	//
	// Update status bar position display.
	//
	char szBuf[128];
	sprintf(szBuf, " @%.0f, %.0f ", vecWorld[axHorz], vecWorld[axVert]);
	SetStatusText(SBI_COORDS, szBuf);
	
	//
	// If we are currently dragging the selection (moving, scaling, rotating, or shearing)
	// update that operation based on the current cursor position and keyboard state.
	//
	if (IsTranslating())
	{
		bCursorSet = true;

		unsigned int uConstraints = GetConstraints(bDisableSnap, nFlags);

		//
		// If they are dragging with a valid handle, update the views.
		//
		if (UpdateTranslation(vecWorld, uConstraints, sizeDragged))
		{
			pDoc->ToolUpdateViews(CMapView2D::updTool);
			pDoc->Update3DViews();
		}
	}
	//
	// Else if we have just started dragging the selection, begin that process here.
	//
	else if (m_bLButtonDown && ((nFlags & MK_LBUTTON) != 0) && (abs(sizeDragged.cx) > 4 || abs(sizeDragged.cy) > 4))
	{
		pView->SetCapture();

		point = m_ptLDownClient;
		pView->ClientToWorld(point);

		if (!m_bSelected)
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEALL));

			Vector ptOrg( COORD_NOTINIT, COORD_NOTINIT, COORD_NOTINIT );

			ptOrg[axHorz] = point.x;
			ptOrg[axVert] = point.y;

			pDoc->GetBestVisiblePoint(ptOrg);

			if (!bDisableSnap)
			{
				pDoc->Snap(ptOrg);
			}

			StartBoxSelection(ptOrg);
			EnableHandles(true);
			pView->SetUpdateFlag(CMapView2D::updTool);
		}
		else
		{
			// we selected something - now drag it too
			if (StartSelectionTranslation(ptHitTest, true))
			{
				// kill this because we want to mimic dragging a selected
				// rectangle, in which case this would be false
				m_bSelected = FALSE;
			}
		}
	}
	else if (!IsEmpty())
	{
		//
		// Just in case the selection set is not empty and "selection" hasn't received a left mouse click.
		// (NOTE: this is gross, but unfortunately necessary (cab))
		//
		UpdateHandleState();

		//
		// If the cursor is on a handle, the cursor will be set by the HitTest code.
		//
		if (m_eHandleMode != HandleMode_SelectionOnly)
		{
			// dvsFIXME: is this the best way to do cursor management?
			Vector2D vecPoint(ptClient.x, ptClient.y);
			int nSelCount = m_SelectionList.Count();
			for (int i = 0; i < nSelCount; i++)
			{
				CMapClass *pObject = m_SelectionList.Element(i);

				//
				// Hit test against the object. nHitData will return with object-specific
				// information about what was clicked on.
				//
				int nHitData;
				CMapClass *pHit = pObject->HitTest2D(pView, vecPoint, nHitData);
				if (pHit != NULL)
				{
					bCursorSet = true;
					break;
				}
			}
		}

		if (!bCursorSet && (HitTest(ptHitTest, TRUE) != inclNone))
		{
			bCursorSet = true;
		}
	}

	if (!bCursorSet)
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vecWorld - Point in world space.
//			uConstraints - 
//			dragSize - 
// Output : 
//-----------------------------------------------------------------------------
// FIXME: Copied from Box3D to change the function signature! This fixes problems
//		  with the world coordinates getting truncated to integer values, preventing snap
//		  to non-integer grid values. Fix everywhere!!
BOOL Selection3D::UpdateTranslation(const Vector &vecPoint, UINT uConstraints, CSize &dragSize)
{
	if (iTransOrigin == inclNone)
	{
		return FALSE;
	}
	else if (iTransOrigin == inclMain)
	{
		goto MoveObjects;
	}

	if (stat.TranslateMode == modeRotate)
	{
		// find rotation angle & return
		stat.fRotateAngle = fixang(lineangle(stat.rotate_org[axHorz], stat.rotate_org[axVert], vecPoint[axHorz], vecPoint[axVert]));
		stat.fRotateAngle -= stat.fStartRotateAngle;
		stat.fRotateAngle = fixang(stat.fRotateAngle);

		if (bInvertHorz + bInvertVert == 1)
		{
			stat.fRotateAngle = fixang(360.0 - stat.fRotateAngle);
		}

		stat.fRotateAngle -= fmod(double(stat.fRotateAngle), double(.5));

		if (uConstraints & constrain15)
		{
			stat.fRotateAngle -= fmod(double(stat.fRotateAngle), double(15.0));
		}

		return TRUE;
	}

	if (stat.TranslateMode == modeShear)
	{
		switch(iTransOrigin)
		{
		case inclTop:
		case inclBottom:
			stat.iShearX = m_pDocument->Snap(vecPoint[axHorz] - m_ptOrigin.x);
			stat.iShearY = bmaxs[axVert] - bmins[axVert];
			break;
		case inclLeft:
		case inclRight:
			stat.iShearX = bmaxs[axHorz] - bmins[axHorz];
			stat.iShearY = m_pDocument->Snap(vecPoint[axVert] - m_ptOrigin.y);
			break;
		}

		return TRUE;
	}

MoveObjects:

	// scaling / moving
	m_Scale[axThird] = 1.0;
	BOOL bNeedsUpdate = FALSE;

	bool bSnapToGrid = !(uConstraints & constrainNosnap);

	int nSnapFlags = 0;
	if (uConstraints & constrainHalfSnap)
	{
		nSnapFlags |= SNAP_HALF_GRID;
	}

	if (iTransOrigin == inclMain)
	{
		Vector vecCompare = ptMoveRef;

		ptMoveRef[axHorz] = vecPoint[axHorz] - sizeMoveOfs[axHorz];
		ptMoveRef[axVert] = vecPoint[axVert] - sizeMoveOfs[axVert];

		if (bSnapToGrid)
		{
			m_pDocument->Snap(ptMoveRef, nSnapFlags);
		}

		m_Scale[axHorz] = (tbmins[axHorz] + ptMoveRef[axHorz]) - bmins[axHorz];
		m_Scale[axVert] = (tbmins[axVert] + ptMoveRef[axVert]) - bmins[axVert];
		m_Scale[axThird] = 0.0f;

		bNeedsUpdate = !CompareVectors2D(vecCompare, ptMoveRef);

		if (!bNeedsUpdate)
		{
			if (dragSize.cx >= 4 || dragSize.cy >= 4)
				bNeedsUpdate = TRUE;
		}
	}
	else 
	{
		// update bounding box
		if (iTransOrigin & inclTop)
		{
			float fOld = tbmins[axVert];

			tbmins[axVert] = bmins[axVert] - (m_ptOrigin.y - vecPoint[axVert]);
			if (bSnapToGrid)
			{
				tbmins[axVert] = m_pDocument->Snap(tbmins[axVert]);
			}

			if(tbmins[axVert] > (tbmaxs[axVert] - 1) && bPreventOverlap)
				tbmins[axVert] = tbmaxs[axVert] - 1;
			if(fOld != tbmins[axVert])
				bNeedsUpdate = TRUE;
			m_Scale[axVert] = double(tbmaxs[axVert] - tbmins[axVert]) /
				double(bmaxs[axVert] - bmins[axVert]);
		}
		else if(iTransOrigin & inclBottom)
		{
			float fOld = tbmaxs[axVert];
			tbmaxs[axVert] = bmaxs[axVert] + (vecPoint[axVert] - m_ptOrigin.y);
			if (bSnapToGrid)
			{
				tbmaxs[axVert] = m_pDocument->Snap(tbmaxs[axVert]);
			}

			if(tbmaxs[axVert] < (tbmins[axVert] + 1) && bPreventOverlap)
				tbmaxs[axVert] = tbmins[axVert] + 1;
			if(fOld != tbmaxs[axVert])
				bNeedsUpdate = TRUE;
			m_Scale[axVert] = double(tbmaxs[axVert] - tbmins[axVert]) /
				double(bmaxs[axVert] - bmins[axVert]);
		}
		if(iTransOrigin & inclLeft)
		{
			float fOld = tbmins[axHorz];
			tbmins[axHorz] = bmins[axHorz] - (m_ptOrigin.x - vecPoint[axHorz]);
			if (bSnapToGrid)
			{
				tbmins[axHorz] = m_pDocument->Snap(tbmins[axHorz]);
			}

			if(tbmins[axHorz] > (tbmaxs[axHorz] - 1) && bPreventOverlap)
				tbmins[axHorz] = tbmaxs[axHorz] - 1;
			if(fOld != tbmins[axHorz])
				bNeedsUpdate = TRUE;
			m_Scale[axHorz] = double(tbmaxs[axHorz] - tbmins[axHorz]) /
				double(bmaxs[axHorz] - bmins[axHorz]);
		}

		else if(iTransOrigin & inclRight)
		{
			float fOld = tbmaxs[axHorz];
			
			tbmaxs[axHorz] = bmaxs[axHorz] + (vecPoint[axHorz] - m_ptOrigin.x);
			if (bSnapToGrid)
			{
				tbmaxs[axHorz] = m_pDocument->Snap(tbmaxs[axHorz]);
			}
			
			if(tbmaxs[axHorz] < (tbmins[axHorz] + 1) && bPreventOverlap)
				tbmaxs[axHorz] = tbmins[axHorz] + 1;
			if(fOld != tbmaxs[axHorz])
				bNeedsUpdate = TRUE;
			m_Scale[axHorz] = double(tbmaxs[axHorz] - tbmins[axHorz]) /
				double(bmaxs[axHorz] - bmins[axHorz]);
		}
	}

	//
	// Constrain to valid map boundaries.
	//
	for (int nDim = 0; nDim < 3; nDim++)
	{
		if (tbmaxs[nDim] < g_MIN_MAP_COORD)
		{
			tbmaxs[nDim] = g_MIN_MAP_COORD;
		}
		else if (tbmaxs[nDim] > g_MAX_MAP_COORD)
		{
			tbmaxs[nDim] = g_MAX_MAP_COORD;
		}

		if (tbmins[nDim] < g_MIN_MAP_COORD)
		{
			tbmins[nDim] = g_MIN_MAP_COORD;
		}
		else if (tbmins[nDim] > g_MAX_MAP_COORD)
		{
			tbmins[nDim] = g_MAX_MAP_COORD;
		}
	}

	UpdateSavedBounds();

	return bNeedsUpdate;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left button up events in the 2D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Selection3D::OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point) 
{
	CMapDoc *pDoc = pView->GetDocument();
	if (pDoc == NULL)
	{
		return false;
	}

	bool bShift = ((GetKeyState(VK_SHIFT) & 0x8000) != 0);

  	CPoint ptHitTest = point;
	ptHitTest.x += pView->GetScrollPos(SB_HORZ);
	ptHitTest.y += pView->GetScrollPos(SB_VERT);

	ReleaseCapture();

	SetDrawFlags(GetDrawFlags() & ~Box3D::boundstext);

	if (IsTranslating())
	{
		m_bLButtonDown = false;
		FinishTranslation(TRUE);

		// selecting stuff in box
		if (IsBoxSelecting())
		{
			if (Options.view2d.bAutoSelect)
			{
				SelectInBox(pDoc, bShift);
			}
		}
		// switch translation mode
		else if (!m_bLeftDragged && !m_bSelected)
		{
			goto _ToggleMode;
		}
		// perform transformation
		else if (!m_bSelected)
		{
			UpdateBox ub;
			ub.Objects = new CMapObjectList;

			BOOL bFullUpdate = TRUE;

			// keep copy of current objects?
			if ((nFlags & MK_SHIFT) && (GetTransOrigin() == Box3D::inclMain))
			{
				GetHistory()->MarkUndoPosition(GetList(), "Clone Objects");
				pView->CloneObjects(*GetList());
				GetHistory()->KeepNew(GetList());

				ub.Objects->AddTail(GetList());
				bFullUpdate = TRUE;
			}
			else
			{
				GetHistory()->MarkUndoPosition(GetList(), "Translation");
				GetHistory()->Keep(GetList());
				ub.Objects->AddTail(GetList());
			}

			// transform selection
			TransformSelection();
			ub.Box.UpdateBounds(&TotalArea);

			if (bFullUpdate)
            {
				pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);
            }
			else
			{
				pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS | MAPVIEW_UPDATE_3D, &ub);
				pDoc->ToolUpdateViews(CMapView2D::updTool);
			}

			pDoc->UpdateDispFaces(this);

			pDoc->SetModifiedFlag();
			NotifyDuplicates(GetList());

			// we allocated this above
			delete ub.Objects;
		}
	}
	else if (!m_bSelected)
	{
_ToggleMode:

		pView->ClientToWorld(point);
		if (HitTest(ptHitTest, FALSE) != -1)
		{
			ToggleTranslateMode();
			pDoc->ToolUpdateViews(CMapView2D::updTool);
		}
	}

	// we might have removed some stuff that was relevant:
	pDoc->UpdateStatusbar();
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles key down events in the 3D view.
// Input  : Per CWnd::OnKeyDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Selection3D::OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CMapDoc *pDoc = pView->GetDocument();
	if (!pDoc)
	{
		return false;
	}

	BOOL bCtrl = ((GetKeyState(VK_CONTROL) & 0x8000) != 0);

	switch (nChar)
	{
		case 'e':
		case 'E':
		{
			m_bEyedropper = !m_bEyedropper;
			if (m_bEyedropper)
			{
				SetEyedropperCursor();
			}
			else
			{
				SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
			}
			return true;
		}

		case VK_DELETE:
		{
			pDoc->OnCmdMsg(ID_EDIT_DELETE, CN_COMMAND, NULL, NULL);
			return true;
		}

		case VK_ESCAPE:
		{
			OnEscape(pDoc);
			return true;
		}
	}

	if (Options.view2d.bNudge && (nChar == VK_UP || nChar == VK_DOWN || nChar == VK_LEFT || nChar == VK_RIGHT))
	{
		if (!IsEmpty())
		{
			Axes2 BestAxes;
			pView->CalcBestAxes(BestAxes);
			pDoc->NudgeObjects(BestAxes, nChar, !bCtrl);
		}

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Handles double click events in the 3D view.
// Input  : Per CWnd::OnLButtonDblClk.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Selection3D::OnLMouseDblClk3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	if (m_SelectionList.Count() > 0)
	{
		GetMainWnd()->pObjectProperties->ShowWindow(SW_SHOW);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			point - 
//-----------------------------------------------------------------------------
void Selection3D::EyedropperPick2D(CMapView2D *pView, CPoint point)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pView - 
//			point - 
//-----------------------------------------------------------------------------
void Selection3D::EyedropperPick3D(CMapView3D *pView, CPoint point)
{
	//
	// We only want to do this if we have at least one entity selected.
	//
	if (!IsAnEntitySelected())
	{
		pView->MessageBox("No entities are selected, so the eyedropper has nothing to assign to.", "No selected entities");
		return;
	}

	//
	// If they clicked on an entity, get the name of the entity they clicked on.
	//
	ULONG ulFace;
	CMapClass *pClickObject = pView->NearestObjectAt(point, ulFace);
	if (pClickObject != NULL)
	{
		EyedropperPick(pView, pClickObject);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pObject - 
//-----------------------------------------------------------------------------
void Selection3D::EyedropperPick(CMapView *pView, CMapClass *pObject)
{
	//
	// The eyedropper currently only supports clicking on entities.
	// TODO: consider using this to fill out face lists if they click on a solid
	//
	CMapEntity *pEntity = FindEntityInTree(pObject);
	if (pEntity == NULL)
	{
		// They clicked on something that is not an entity.
		return;
	}

	//
	// Get the name of the clicked on entity.
	//
	const char *pszClickName = NULL;
	pszClickName = pEntity->GetKeyValue("targetname");
	if (pszClickName == NULL)
	{
		//
		// They clicked on an entity with no name.
		//
		pView->MessageBox("The chosen entity has no name.", "No name to pick");
		return;
	}

	//
	// Build a list of all the keyvalues in the selected entities that are
	// of type ivTargetDest.
	//
	CUtlVector<GDinputvariable *> VarList;

	int nEntityCount = 0;
	int nSelCount = m_SelectionList.Count();
	for (int i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = m_SelectionList.Element(i);
		CMapEntity *pEntity = dynamic_cast <CMapEntity *> (pObject);
		if (pEntity != NULL)
		{
			nEntityCount++;
			GDclass *pClass = pEntity->GetClass();

			int nVarCount = pClass->GetVariableCount();
			for (int nVar = 0; nVar < nVarCount; nVar++)
			{
				GDinputvariable *pVar = pClass->GetVariableAt(nVar);
				if (pVar->GetType() == ivTargetDest)
				{
					VarList.AddToTail(pVar);
				}
			}
		}
	}

	//
	// Prompt for what keyvalue in the selected entities we are filling out.
	//
	int nCount = VarList.Count();
	if (nCount <= 0)
	{
		//
		// No selected entities have keys of type ivTargetDest, so there's
		// nothing we can do.
		//
		pView->MessageBox("No selected entities have keyvalues that accept an entity name, so the eyedropper has nothing to assign to.", "No eligible keyvalues");
		return;
	}

	//
	// Determine the name of the key that we are filling out.
	//
	GDinputvariable *pVar = ChooseEyedropperVar(pView, VarList);
	if (!pVar)
	{
		return;
	}
	const char *pszVarName = pVar->GetName();
	if (!pszVarName)
	{
		return;
	}

	GetHistory()->MarkUndoPosition(GetList(), "Set Keyvalue");

	//
	// Apply the key to all selected entities with the chosen keyvalue.
	//
	nSelCount = m_SelectionList.Count();
	for (i = 0; i < nSelCount; i++)
	{
		CMapClass *pObject = m_SelectionList.Element(i);
	
		CMapEntity *pEntity = dynamic_cast <CMapEntity *> (pObject);
		if (pEntity != NULL)
		{
			GDclass *pClass = pEntity->GetClass();
			GDinputvariable *pVar = pClass->VarForName(pszVarName);
			if ((pVar != NULL) && (pVar->GetType() == ivTargetDest))
			{
				GetHistory()->Keep(pEntity);
				pEntity->SetKeyValue(pszVarName, pszClickName);
			}
		}
	}

	CMapDoc *pDoc = (CMapDoc *)pView->GetDocument();
	if (pDoc != NULL)
	{
		pDoc->SetModifiedFlag();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the nearest CMapEntity object up the hierarchy from the
//			given object.
// Input  : pObject - Object to start from.
//-----------------------------------------------------------------------------
CMapEntity *Selection3D::FindEntityInTree(CMapClass *pObject)
{
	do
	{
		CMapEntity *pEntity = dynamic_cast <CMapEntity *> (pObject);
		if (pEntity != NULL)
		{
			return pEntity;
		}

		pObject = pObject->GetParent();

	} while (pObject != NULL);

	// No entity in this branch of the object tree.
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left button down events in the 3D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Selection3D::OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point) 
{
	CMapDoc *pDoc = pView->GetDocument();

	//
	// If they are holding down the eyedropper hotkey, do an eyedropper pick. The eyedropper fills out
	// keyvalues in selected entities based on the object they clicked on.
	//
	if (m_bEyedropper)
	{
		EyedropperPick3D(pView, point);
		m_bEyedropper = false;
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
		return true;
	}

	pDoc->ClearHitList();
	CMapObjectList SelectList;

	GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "Selection");

	//
	// Find out how many (and what) map objects are under the point clicked on.
	//
	RenderHitInfo_t Objects[512];
	int nHits = pView->ObjectsAt(point.x, point.y, 1, 1, Objects, sizeof(Objects) / sizeof(Objects[0]));
	if (nHits != 0)
	{
		//
		// We now have an array of pointers to CMapAtoms. Any that can be upcast to CMapClass
		// we add to a list of hits.
		//
		for (int i = 0; i < nHits; i++)
		{
			CMapClass *pMapClass = dynamic_cast <CMapClass *>(Objects[i].pObject);
			if (pMapClass != NULL)
			{
				SelectList.AddTail(pMapClass);
			}
		}
	}

	if (!SelectList.GetCount())
	{
		// Clicked on nothing.
		if (!(nFlags & MK_CONTROL))
		{
			// Not holding down control - clear selection and exit.
			pDoc->SelectFace(NULL, 0, CMapDoc::scClear);
			pDoc->SelectObject(NULL, CMapDoc::scClear | CMapDoc::scUpdateDisplay);
		}
		return true;
	}

	//
	// Add each object to the document's hit list using the current selection mode.
	//
	SelectMode_t eSelectMode = pDoc->Selection_GetMode();
	POSITION p = SelectList.GetHeadPosition();
	while (p)
	{
		CMapClass *pObject = SelectList.GetNext(p);
		CMapClass *pHitObject = pObject->PrepareSelection( eSelectMode );
		if (pHitObject)
		{
			pDoc->AddHit( pHitObject );
		}
	}

	//
	// Clear the selection list unless they are holding down control.
	//
	if (!(nFlags & MK_CONTROL))
	{
		pDoc->SelectFace(NULL, 0, CMapDoc::scClear);
		pDoc->SelectObject(NULL, CMapDoc::scClear | CMapDoc::scUpdateDisplay);
	}

	//
	// Set the pick timer and force the first pick.
	//
	pView->BeginPick();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left button up events in the 3D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Selection3D::OnLMouseUp3D(CMapView3D *pView, UINT nFlags, CPoint point) 
{
	pView->EndPick();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles mouse move events in the 3D view.
// Input  : Per CWnd::OnMouseMove.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Selection3D::OnMouseMove3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	if (m_bEyedropper)
	{
		SetEyedropperCursor();
	}
	else
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the cursor to the eyedropper cursor.
//-----------------------------------------------------------------------------
void Selection3D::SetEyedropperCursor(void)
{
	static HCURSOR hcurEyedropper = NULL;
	
	if (!hcurEyedropper)
	{
		hcurEyedropper = LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_EYEDROPPER));
	}
	
	SetCursor(hcurEyedropper);
}


//-----------------------------------------------------------------------------
// Purpose: Handles the escape key in the 2D or 3D views.
//-----------------------------------------------------------------------------
void Selection3D::OnEscape(CMapDoc *pDoc)
{
	//
	// If we're in eyedropper mode, go back to selection mode.
	//
	if (m_bEyedropper)
	{
		m_bEyedropper = false;
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	}
	//
	// If we're box selecting, clear the box.
	//
	else if (IsBoxSelecting())
	{
		EndBoxSelection();
		UpdateBounds();
		
		pDoc->ToolUpdateViews(CMapView2D::updTool);
		pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY);
	}
	//
	// If we're moving a brush, put it back.
	//
	else if (IsTranslating())
	{
		FinishTranslation(false);

		pDoc->ToolUpdateViews(CMapView2D::updTool);
		pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY);
	}
	//
	// If we have a selection, deselect it.
	//
	else if (!IsEmpty())
	{
		SetEmpty();
		
		pDoc->ToolUpdateViews(CMapView2D::updTool);
		pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY);
	}
}

