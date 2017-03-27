//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the entity/prefab placement tool.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "GameData.h"
#include "History.h"
#include "MainFrm.h"
#include "MapDefs.h"
#include "MapSolid.h"
#include "MapDoc.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "Material.h"
#include "materialsystem/IMesh.h"
#include "Render2D.h"
#include "Render3D.h"
#include "StatusBarIDs.h"
#include "TextureSystem.h"
#include "ToolEntity.h"
#include "ToolManager.h"
#include "Worldcraft.h"


//#pragma warning(disable:4244)


static HCURSOR s_hcurEntity = NULL;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Marker3D::Marker3D(void)
{
	SetEmpty();

	m_vecPos = vec3_origin;
	m_vecTranslatePos = vec3_origin;

	m_bLButtonDown = false;

	if (s_hcurEntity == NULL)
	{
		s_hcurEntity = LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_ENTITY));
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Marker3D::~Marker3D(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Marker3D::IsEmpty(void)
{
	return m_bEmpty;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Marker3D::SetEmpty(void)
{
	m_bEmpty = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			BOOL - 
// Output : 
//-----------------------------------------------------------------------------
int Marker3D::HitTest(CPoint pt, BOOL)
{
	CRect r(m_vecPos[axHorz], m_vecPos[axVert], m_vecPos[axHorz], m_vecPos[axVert]);
	RectToScreen(r);
	r.InflateRect(8, 8);

	return r.PtInRect(pt) ? TRUE : -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
// Output : Returns TRUE.
//-----------------------------------------------------------------------------
BOOL Marker3D::StartTranslation(CPoint &pt)
{
	m_vecTranslatePos = m_vecPos;
	Tool3D::StartTranslation(pt);
	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bSave - 
//-----------------------------------------------------------------------------
void Marker3D::FinishTranslation(BOOL bSave)
{
	if (bSave)
	{
		m_vecPos = m_vecTranslatePos;
		m_bEmpty = false;
	}

	Tool3D::FinishTranslation(bSave);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			uFlags - 
//			size - 
// Output : Returns true if the translation delta was nonzero.
//-----------------------------------------------------------------------------
BOOL Marker3D::UpdateTranslation(CPoint pt, UINT uFlags, CSize &size)
{
	if (!(uFlags & constrainNosnap))
	{
		m_pDocument->Snap(pt);
	}

	Vector vecCompare = m_vecTranslatePos;

	m_vecTranslatePos[axHorz] = pt.x;
	m_vecTranslatePos[axVert] = pt.y;

	return !CompareVectors2D(m_vecTranslatePos, vecCompare);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt3 - 
//-----------------------------------------------------------------------------
void Marker3D::StartNew(const Vector &vecStart)
{
	m_vecTranslatePos[axHorz] = vecStart[axHorz];
	m_vecTranslatePos[axVert] = vecStart[axVert];

	m_bEmpty = false;

	CPoint pt(vecStart[axHorz], vecStart[axVert]);
	Tool3D::StartTranslation(pt);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void Marker3D::RenderTool2D(CRender2D *pRender)
{
	if (!IsActiveTool())
	{
		return;
	}

	Vector vecPos;
	CRect rect;
	CPoint pt;

	if (IsTranslating())
	{
		vecPos = m_vecTranslatePos;
	}
	else
	{
		if (IsEmpty())
		{
			return;
		}

		vecPos = m_vecPos;
	}

	pt.x = vecPos[axHorz];
	pt.y = vecPos[axVert];
	PointToScreen(pt);

	pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 35, 255, 75);

	//
	// Draw center rect.
	//
	rect.SetRect(pt, pt);
	rect.InflateRect(6, 6);
	pRender->Rectangle(rect, false);

	//
	// Draw crosshairs.
	//
	pRender->MoveTo(g_MIN_MAP_COORD, pt.y);
	pRender->LineTo(g_MAX_MAP_COORD, pt.y);
	pRender->MoveTo(pt.x, g_MIN_MAP_COORD);
	pRender->LineTo(pt.x, g_MAX_MAP_COORD);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			point - 
// Output : 
//-----------------------------------------------------------------------------
bool Marker3D::OnContextMenu2D(CMapView2D *pView, CPoint point)
{
	if (!IsEmpty())
	{
		CMapDoc *pDoc = pView->GetDocument();
		if (pDoc == NULL)
		{
			return true;
		}

		// FIXME: functionalize this or fix it!
		pView->ScreenToClient(&point);

		CRect rect;
		pView->GetClientRect(&rect);
		if (!rect.PtInRect(point))
		{
			return true;
		}

		CPoint ptScreen(point);
		CPoint ptMapScreen(point);
		pView->ClientToScreen(&ptScreen);
		pView->ClientToWorld(point);

		ptMapScreen.x += pView->GetScrollPos(SB_HORZ);
		ptMapScreen.y += pView->GetScrollPos(SB_VERT);

		if (HitTest(ptMapScreen, FALSE) != -1)
		{
			static CMenu menu, menuCreate;
			static bool bInit = false;

			if (!bInit)
			{
				bInit = true;
				menu.LoadMenu(IDR_POPUPS);
				menuCreate.Attach(::GetSubMenu(menu.m_hMenu, 1));
			}

			menuCreate.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN, ptScreen.x, ptScreen.y, pView);
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pView - 
//			nChar - 
//			nRepCnt - 
//			nFlags - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Marker3D::OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags)
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
// Purpose: 
// Input  : pView - 
//			nFlags - 
//			point - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Marker3D::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	CMapDoc *pDoc = pView->GetDocument();
	if (pDoc == NULL)
	{
		return true;
	}

	CMapWorld *pWorld = pDoc->GetMapWorld();

	m_bLButtonDown = true;

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
	pView->ClientToWorld(point);

	Vector ptOrg = vec3_origin;
	ptOrg[axHorz] = point.x;
	ptOrg[axVert] = point.y;

	//
	// Snap starting position to grid.
	//
	if (!(GetAsyncKeyState(VK_MENU) & 0x8000))
	{
		pDoc->Snap(ptOrg);
	}

	StartNew(ptOrg);
	pView->SetUpdateFlag(CMapView2D::updTool);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Pre CWnd::OnLButtonUp.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Marker3D::OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point)
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
// Purpose: 
// Input  : pView - 
//			nFlags - 
//			point - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Marker3D::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	CMapDoc *pDoc = pView->GetDocument();
	if (!pDoc)
	{
		return false;
	}

	bool bCursorSet = false;
	unsigned int uConstraints = 0;
	if ((GetAsyncKeyState(VK_MENU) & 0x8000))
	{
		uConstraints |= Tool3D::constrainNosnap;
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
	point.x = vecWorld[axHorz];
	point.y = vecWorld[axVert];

	//
	// Update status bar position display.
	//
	char szBuf[128];
	m_pDocument->Snap(vecWorld);
	sprintf(szBuf, " @%.0f, %.0f ", vecWorld[axHorz], vecWorld[axVert]);
	SetStatusText(SBI_COORDS, szBuf);

	//
	// If we are currently dragging the marker, update that operation based on
	// the current cursor position and keyboard state.
	//
	if (IsTranslating())
	{
		if (UpdateTranslation(point, uConstraints, CSize(0,0)))
		{
			pDoc->ToolUpdateViews(CMapView2D::updTool);
			pDoc->Update3DViews();
		}

		// Don't change the cursor while dragging - it should remain a cross.
		bCursorSet = true;
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
	}
	else if (!IsEmpty())
	{
		// Don't change the cursor while dragging - it should remain a cross.
		bCursorSet = true;
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
	}

	if (!bCursorSet)
	{
		SetCursor(s_hcurEntity);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			nChar - 
//			nRepCnt - 
//			nFlags - 
// Output : Returns true if the message was handled, false otherwise.
//-----------------------------------------------------------------------------
bool Marker3D::OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CMapDoc *pDoc = pView->GetDocument();
	if (pDoc == NULL)
	{
		return false;
	}

	switch (nChar)
	{
		case VK_RETURN:
		{
			//
			// Create the entity or prefab.
			//
			if (!IsEmpty())
			{
				//CreateMapObject(pView); // TODO: support in 3D
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
// Purpose: Handles the escape key in the 2D or 3D views.
//-----------------------------------------------------------------------------
void Marker3D::OnEscape(void)
{
	//
	// Cancel the object creation tool.
	//
	if (!IsEmpty())
	{
		SetEmpty();
		m_pDocument->ToolUpdateViews(CMapView2D::updEraseTool);
	}
	else
	{
		g_pToolManager->SetTool(TOOL_POINTER);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pView - 
//			nFlags - 
//			point - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Marker3D::OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	ULONG ulFace;
	CMapClass *pObject = pView->NearestObjectAt(point, ulFace);
	if (pObject != NULL)
	{
		CMapSolid *pSolid = dynamic_cast <CMapSolid *> (pObject);
		if (pSolid == NULL)
		{
			// Clicked on a point entity - do nothing.
			return true;
		}

		CMapDoc *pDoc = pView->GetDocument();

		//
		// Build a ray to trace against the face that they clicked on to
		// find the point of intersection.
		//			
		Vector Start;
		Vector End;
		pView->BuildRay(point, Start, End);

		Vector HitPos, HitNormal;
		CMapFace *pFace = pSolid->GetFace(ulFace);
		if (pFace->TraceLine(HitPos, HitNormal, Start, End))
		{
			if (GetMainWnd()->m_ObjectBar.IsEntityToolCreatingPrefab())
			{
				//
				// Prefab creation.
				//
				pDoc->Snap(HitPos);

				GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "New Prefab");

				// Get prefab object
				CMapClass *pPrefabObject = GetMainWnd()->m_ObjectBar.BuildPrefabObjectAtPoint(HitPos);

				//
				// Add prefab to the world.
				//
				CMapWorld *pWorld = pDoc->GetMapWorld();
				pDoc->ExpandObjectKeywords(pPrefabObject, pWorld);
				pDoc->AddObjectToWorld(pPrefabObject);
				GetHistory()->KeepNew(pPrefabObject);

				//
				// Select the new object.
				//
				pDoc->SelectObject(pPrefabObject, CMapDoc::scClear | CMapDoc::scSelect | CMapDoc::scUpdateDisplay);

				//
				// Update world bounds.
				//
				UpdateBox ub;
				CMapObjectList ObjectList;
				ObjectList.AddTail(pPrefabObject);
				ub.Objects = &ObjectList;

				Vector mins;
				Vector maxs;
				pPrefabObject->GetRender2DBox(mins, maxs);
				ub.Box.SetBounds(mins, maxs);

				pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);
				
				pDoc->SetModifiedFlag();
			}
			else if (GetMainWnd()->m_ObjectBar.IsEntityToolCreatingEntity())
			{
				//
				// Entity creation.
				//
				GetHistory()->MarkUndoPosition(pDoc->Selection_GetList(), "New Entity");
				
				CMapEntity *pEntity = new CMapEntity;
				pEntity->SetPlaceholder(TRUE);
				pEntity->SetOrigin(HitPos);
				pEntity->SetClass(CObjectBar::GetDefaultEntityClass());
				
				//Align the entity on the plane properly
				//				pEntity->AlignOnPlane(HitPos, &pFace->plane, (pFace->plane.normal[2] > 0.0f) ? CMapEntity::ALIGN_BOTTOM : CMapEntity::ALIGN_TOP);
				pEntity->AlignOnPlane(HitPos, &pFace->plane, (HitNormal[2] > 0.0f) ? CMapEntity::ALIGN_BOTTOM : CMapEntity::ALIGN_TOP);
									
				CMapWorld *pWorld = pDoc->GetMapWorld();
				pDoc->AddObjectToWorld(pEntity);
				
				GetHistory()->KeepNew(pEntity);

				//
				// Select the new object.
				//
				pDoc->SelectObject(pEntity, CMapDoc::scClear | CMapDoc::scSelect | CMapDoc::scUpdateDisplay);
				
 				UpdateBox ub;
				CMapObjectList ObjectList;
				ObjectList.AddTail(pEntity);
				ub.Objects = &ObjectList;

				Vector mins;
				Vector maxs;
				pEntity->GetRender2DBox(mins, maxs);
				ub.Box.SetBounds(mins, maxs);

				pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS, &ub);
			
				pDoc->SetModifiedFlag();
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Renders a selection gizmo at our bounds center.
// Input  : pRender - Rendering interface.
//-----------------------------------------------------------------------------
void Marker3D::RenderTool3D(CRender3D *pRender)
{
	if (!IsActiveTool())
	{
		return;
	}

	Vector *pPos;

	if (IsTranslating())
	{
		pPos = &m_vecTranslatePos;
	}
	else
	{
		if (IsEmpty())
		{
			return;
		}

		pPos = &m_vecPos;
	}
	
	//
	// Setup the renderer.
	//
	pRender->SetRenderMode(RENDER_MODE_WIREFRAME);

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();

	meshBuilder.Begin(pMesh, MATERIAL_LINES, 3);

	meshBuilder.Position3f(g_MIN_MAP_COORD, (*pPos)[1], (*pPos)[2]);
	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(g_MAX_MAP_COORD, (*pPos)[1], (*pPos)[2]);
	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f((*pPos)[0], g_MIN_MAP_COORD, (*pPos)[2]);
	meshBuilder.Color3ub(0, 255, 0);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f((*pPos)[0], g_MAX_MAP_COORD, (*pPos)[2]);
	meshBuilder.Color3ub(0, 255, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f((*pPos)[0], (*pPos)[1], g_MIN_MAP_COORD);
	meshBuilder.Color3ub(0, 0, 255);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f((*pPos)[0], (*pPos)[1], g_MAX_MAP_COORD);
	meshBuilder.Color3ub(0, 0, 255);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	pRender->SetRenderMode(RENDER_MODE_DEFAULT);
}


void Marker3D::CreateMapObject(CMapView2D *pView)
{
	CMapWorld *pWorld = m_pDocument->GetMapWorld();
	CMapClass *pobj = NULL;

	//
	// Handle prefab creation.
	//
	if (GetMainWnd()->m_ObjectBar.IsEntityToolCreatingPrefab())
	{			
		GetHistory()->MarkUndoPosition(m_pDocument->Selection_GetList(), "New Prefab");

		CMapClass *pPrefabObject = GetMainWnd()->m_ObjectBar.BuildPrefabObjectAtPoint(m_vecPos);

		if (pPrefabObject == NULL)
		{
			pView->MessageBox("Unable to load prefab", "Error", MB_OK);
			SetEmpty();
			return;
		}

		m_pDocument->ExpandObjectKeywords(pPrefabObject, pWorld);
		m_pDocument->AddObjectToWorld(pPrefabObject);

		GetHistory()->KeepNew(pPrefabObject);

		pobj = pPrefabObject;
	}
	//
	// Handle entity creation.
	//
	else if (GetMainWnd()->m_ObjectBar.IsEntityToolCreatingEntity())
	{
		GetHistory()->MarkUndoPosition(m_pDocument->Selection_GetList(), "New Entity");
		
		CMapEntity *pEntity = new CMapEntity;
		
		pEntity->SetPlaceholder(TRUE);
		pEntity->SetOrigin(m_vecPos);
		pEntity->SetClass(CObjectBar::GetDefaultEntityClass());

		m_pDocument->AddObjectToWorld(pEntity);
		
		pobj = pEntity;
		
		GetHistory()->KeepNew(pEntity);
	}

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


