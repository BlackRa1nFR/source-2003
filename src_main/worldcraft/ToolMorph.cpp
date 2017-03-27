//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "GlobalFunctions.h"
#include "History.h"
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMesh.h"
#include "MainFrm.h"
#include "MapDefs.h"
#include "MapDoc.h"
#include "MapSolid.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "Material.h"
#include "ToolManager.h"
#include "ToolMorph.h"
#include "Options.h"
#include "Render2D.h"
#include "StatusBarIDs.h"
#include "worldcraft.h"


//
// For rendering in 3D.
//
#define MORPH_VERTEX_DIST	3


//-----------------------------------------------------------------------------
// Purpose: Callback function to add objects to the morph selection.
// Input  : pSolid - Solid to add.
//			pMorph - Morph tool.
// Output : Returns TRUE to continue enumerating.
//-----------------------------------------------------------------------------
static BOOL AddToMorph(CMapSolid *pSolid, Morph3D *pMorph)
{
	pMorph->SelectObject(pSolid, Morph3D::scSelect);
	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Morph3D::Morph3D(void)
{
	m_nSelectedHandles = 0;
	m_SelectedType = shtNothing;
	m_HandleMode = hmBoth;
	m_bBoxSelecting = false;
	m_bScaling = false;
	m_pOrigPosList = NULL;
	m_LastThirdAxis = -1;

	m_bLButtonDown = false;
	m_ptLDownClient.x = m_ptLDownClient.y = 0;
	m_ptLastMouseMovement.x = m_ptLastMouseMovement.y = 0;

	m_bHit = false;
	m_bUpdateOrg = false;
	m_bLButtonDownControlState = false;

	SetDrawColors(Options.colors.clrToolHandle, Options.colors.clrToolMorph);

	memset(&m_DragHandle, 0, sizeof(m_DragHandle));
	m_bMorphing = false;
	m_bMovingSelected = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Morph3D::~Morph3D(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Morph3D::OnActivate(ToolID_t eOldTool)
{
	if ((eOldTool != TOOL_CAMERA) && (eOldTool != TOOL_NONE))
	{
		if (IsActiveTool())
		{
			//
			// Already active - change modes and redraw views.
			//
			ToggleMode();
			m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		}
		else
		{
			//
			// Put all selected objects into morph
			//
			int nSelCount = m_pDocument->Selection_GetCount();
			for (int i = 0; i < nSelCount; i++)
			{
				CMapClass *pobj = m_pDocument->Selection_GetObject(i);
				if (pobj->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
				{
					SelectObject((CMapSolid *)pobj, Morph3D::scSelect);
				}
				pobj->EnumChildren((ENUMMAPCHILDRENPROC)AddToMorph, (DWORD)this, MAPCLASS_TYPE(CMapSolid));
			}

			m_pDocument->SelectObject(NULL, scClear | CMapDoc::scUpdateDisplay);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Can we deactivate this tool.
//-----------------------------------------------------------------------------
bool Morph3D::CanDeactivate( void )
{
	return CanDeselectList();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the tool is deactivated.
// Input  : eNewTool - The ID of the tool that is being activated.
//-----------------------------------------------------------------------------
void Morph3D::OnDeactivate(ToolID_t eNewTool)
{
	if (IsScaling())
	{
		OnScaleCmd();
	}

	if (!IsEmpty() && (eNewTool != TOOL_CAMERA) && (eNewTool != TOOL_MAGNIFY))
	{
		CUtlVector <CMapClass *>List;
		GetMorphingObjects(List);

		// Empty morph tool (Save contents).
		SetEmpty();

		//
		// Select the solids that we were morphing.
		//
		int nObjectCount = List.Count();
		for (int i = 0; i < nObjectCount; i++)
		{
			CMapClass *pSolid = List.Element(i);
			CMapClass *pSelect = pSolid->PrepareSelection(m_pDocument->Selection_GetMode());
			if (pSelect == pSolid)
			{
				m_pDocument->SelectObject(pSolid, scSelect);
			}
		}

		m_pDocument->UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY, NULL);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the given solid is being morphed, false if not.
// Input  : pSolid - The solid.
//			pStrucSolidRvl - The corresponding structured solid.
//-----------------------------------------------------------------------------
BOOL Morph3D::IsMorphing(CMapSolid *pSolid, CSSolid **pStrucSolidRvl)
{
	POSITION p = m_StrucSolids.GetHeadPosition();
	while(p)
	{
		CSSolid *pSSolid = m_StrucSolids.GetNext(p);
		if(pSSolid->m_pMapSolid == pSolid)
		{
			if(pStrucSolidRvl)
				pStrucSolidRvl[0] = pSSolid;
			return TRUE;
		}
	}

	return FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the bounding box of the objects being morphed.
// Input  : bReset - 
//-----------------------------------------------------------------------------
void Morph3D::GetMorphBounds(Vector &mins, Vector &maxs, bool bReset)
{
	mins = m_MorphBounds.bmins;
	maxs = m_MorphBounds.bmaxs;

	if (bReset)
	{
		m_MorphBounds.ResetBounds();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Can we deslect the list?  All current SSolid with displacements 
//          are valid.
//-----------------------------------------------------------------------------
bool Morph3D::CanDeselectList( void )
{
	POSITION p = m_StrucSolids.GetHeadPosition();
	while ( p )
	{
		CSSolid *pSSolid = m_StrucSolids.GetNext( p );
		if ( pSSolid )
		{
			if ( !pSSolid->IsValidWithDisps() )
			{
				// Ask
				if( AfxMessageBox( "Invalid solid, destroy displacement(s)?", MB_YESNO ) == IDYES )
				{
					// Destroy the displacement data.
					pSSolid->DestroyDisps();
				}
				else
				{
					return false;
				}
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Selects a solid for vertex manipulation. An SSolid class is created
//			and the CMapSolid is attached to it. The map solid is removed from
//			the views, since it will be represented by the structured solid
//			until vertex manipulation is finished.
// Input  : pSolid - Map solid to select.
//			cmd - scClear, scToggle, scUnselect
//-----------------------------------------------------------------------------
void Morph3D::SelectObject(CMapSolid *pSolid, UINT cmd)
{
	// construct temporary list to pass to document functions:
	CMapObjectList List;
	List.AddTail( pSolid );

	if( cmd & scClear )
		SetEmpty();

	CSSolid *pStrucSolid;
	if ( IsMorphing( pSolid, &pStrucSolid ) )
	{
		if ( cmd & scToggle || cmd & scUnselect )
		{
			// stop morphing solid
			Vector mins;
			Vector maxs;
			pSolid->GetRender2DBox(mins, maxs);
			m_MorphBounds.UpdateBounds(mins, maxs);
			pStrucSolid->Convert(FALSE);
			pSolid->GetRender2DBox(mins, maxs);
			m_MorphBounds.UpdateBounds(mins, maxs);

			pStrucSolid->Detach();
			m_pDocument->SetModifiedFlag();
			
			// want to draw in 2d views again
			pSolid->SetVisible2D(true);

			// remove from linked list
			POSITION p = m_StrucSolids.Find(pStrucSolid);
			m_StrucSolids.RemoveAt(p);

			// make sure none of its handles are selected
			for(int i = 0; i < m_nSelectedHandles; i++)
			{
				if(m_SelectedHandles[i].pStrucSolid == pStrucSolid)
				{
					m_SelectedHandles.RemoveAt(i);
					--m_nSelectedHandles;
					--i;
				}
			}

			delete pStrucSolid;
		
			pSolid->SetSelectionState(SELECT_NONE);
		}

		return;
	}

	pStrucSolid = new CSSolid;
	
	// convert to structured solid
	pStrucSolid->Attach(pSolid);
	pStrucSolid->Convert();

	pStrucSolid->ShowHandles(m_HandleMode & hmVertex, m_HandleMode & hmEdge);

	// don't draw this solid in the 2D views anymore
	pSolid->SetVisible2D(false);
	pSolid->SetSelectionState(SELECT_MORPH);

	// add to list of structured solids
	m_StrucSolids.AddTail(pStrucSolid);
}


int Morph3D::HitTest(CPoint pt, BOOL)
{
	if (m_bBoxSelecting)
	{
		return Box3D::HitTest(pt, FALSE);
	}

	return inclNone;
}


void Morph3D::WorldToScreen2D(CPoint &Screen, const Vector &World)
{
	Vector ptHires;
	ptHires[0] = World[axHorz];
	ptHires[1] = World[axVert];

	if (bInvertHorz)
	{
		ptHires[0] = -ptHires[0];
	}

	if (bInvertVert)
	{
		ptHires[1] = -ptHires[1];
	}

	ptHires[0] *= fZoom;
	ptHires[1] *= fZoom;

	Screen.x = ptHires[0];
	Screen.y = ptHires[1];
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CompareMorphHandles(const MORPHHANDLE &mh1, const MORPHHANDLE &mh2)
{
	return ((mh1.pMapSolid == mh2.pMapSolid) && 
			(mh1.pStrucSolid == mh2.pStrucSolid) && 
			(mh1.ssh == mh2.ssh));
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not the given morph handle is selected.
//-----------------------------------------------------------------------------
bool Morph3D::IsSelected(MORPHHANDLE &mh)
{
	for (int i = 0; i < m_nSelectedHandles; i++)
	{
		if (CompareMorphHandles(m_SelectedHandles[i], mh))
		{
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Hit tests against all the handles. Sets the mouse cursor if we are
//			over a handle.
// Input  : pt - 
//			pInfo - 
// Output : Returns TRUE if the mouse cursor is over a handle, FALSE if not.
//-----------------------------------------------------------------------------
BOOL Morph3D::MorphHitTest2D(CPoint pt, MORPHHANDLE *pInfo)
{
	SSHANDLE hnd = 0;

	CRect r;
	CPoint ptv;

	// check scaling position first
	if (m_bScaling && pInfo)
	{
		WorldToScreen2D(ptv, m_ScaleOrg);

		r.SetRect(ptv.x - 8, ptv.y - 8, ptv.x + 8, ptv.y + 8);
		if (r.PtInRect(pt))
		{
			memset(pInfo, 0, sizeof(MORPHHANDLE));
			pInfo->ssh = SSH_SCALEORIGIN;
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
			return TRUE;
		}
	}

	POSITION p = m_StrucSolids.GetHeadPosition();
	while (p)
	{
		CSSolid *pStrucSolid = m_StrucSolids.GetNext(p);

		// do a hit test on all handles:
		if (m_HandleMode & hmVertex)
		{
			for(int i = 0; i < pStrucSolid->m_nVertices; i++)
			{
				CSSVertex &v = pStrucSolid->m_Vertices[i];

				WorldToScreen2D(ptv, v.pos);

				r.SetRect(ptv.x - 3, ptv.y - 3, ptv.x + 3, ptv.y + 3);

				ptv.x = v.pos[axHorz];
				ptv.y = v.pos[axVert];
				PointToScreen(ptv);
				
				if(PtInRect(&r, pt))
				{
					hnd = v.id;
					break;
				}
			}
		}

		if (!hnd && (m_HandleMode & hmEdge))
		{
			for (int i = 0; i < pStrucSolid->m_nEdges; i++)
			{
				CSSEdge &e = pStrucSolid->m_Edges[i];

				WorldToScreen2D(ptv, e.ptCenter);

				r.SetRect(ptv.x - 3, ptv.y - 3, ptv.x + 3, ptv.y + 3);

				if(PtInRect(&r, pt))
				{
					hnd = e.id;
					break;
				}
			}
		}

		if (hnd)
		{
			SSHANDLEINFO hi;
			pStrucSolid->GetHandleInfo(&hi, hnd);

			// see if there is a 2d match that is already selected - if
			//  there is, select that instead
			SSHANDLE hMatch = Get2DMatches(pStrucSolid, hi);

			if(hMatch)
				hnd = hMatch;

			if(pInfo)
			{
				pInfo->pMapSolid = pStrucSolid->m_pMapSolid;
				pInfo->pStrucSolid = pStrucSolid;
				pInfo->ssh = hnd;
			}
			break;
		}
	}

	if (hnd != 0)
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
	}

	return hnd ? TRUE : FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: Hit tests against all the handles. Sets the mouse cursor if we are
//			over a handle.
// Input  : pt - 
//			pInfo - 
// Output : Returns TRUE if the mouse cursor is over a handle, FALSE if not.
//-----------------------------------------------------------------------------
BOOL Morph3D::MorphHitTest3D(CMapView3D *pView, CPoint pt, MORPHHANDLE *pInfo)
{
	SSHANDLE hnd = 0;

	Vector2D vecScreen;
	CRect rectScreen;

	POSITION p = m_StrucSolids.GetHeadPosition();
	while (p)
	{
		CSSolid *pStrucSolid = m_StrucSolids.GetNext(p);

		//
		// Hit test against all vertex handles.
		//
		if (m_HandleMode & hmVertex)
		{
			for (int i = 0; i < pStrucSolid->m_nVertices; i++)
			{
				CSSVertex &v = pStrucSolid->m_Vertices[i];

				pView->WorldToClient(vecScreen, v.pos);
				rectScreen.SetRect(vecScreen.x - 3, vecScreen.y - 3, vecScreen.x + 3, vecScreen.y + 3);
				if (PtInRect(&rectScreen, pt))
				{
					hnd = v.id;
					break;
				}
			}
		}

		//
		// If no hit found, hit test against all edge handles.
		//
		if (!hnd && (m_HandleMode & hmEdge))
		{
			for (int i = 0; i < pStrucSolid->m_nEdges; i++)
			{
				CSSEdge &e = pStrucSolid->m_Edges[i];

				pView->WorldToClient(vecScreen, e.ptCenter);
				rectScreen.SetRect(vecScreen.x - 3, vecScreen.y - 3, vecScreen.x + 3, vecScreen.y + 3);
				if (PtInRect(&rectScreen, pt))
				{
					hnd = e.id;
					break;
				}
			}
		}

		if (hnd)
		{
			//
			// Found a hit, fill out pInfo.
			//
			if (pInfo)
			{
				pInfo->pMapSolid = pStrucSolid->m_pMapSolid;
				pInfo->pStrucSolid = pStrucSolid;
				pInfo->ssh = hnd;
			}
			break;
		}
	}

	if (hnd != 0)
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
	}
	else if (pInfo != NULL)
	{
		memset(pInfo, 0, sizeof(MORPHHANDLE));
	}

	return hnd ? TRUE : FALSE;
}


void Morph3D::GetHandlePos(MORPHHANDLE *pInfo, Vector& pt)
{
	SSHANDLEINFO hi;
	pInfo->pStrucSolid->GetHandleInfo(&hi, pInfo->ssh);
	pt = hi.pos;
}


//-----------------------------------------------------------------------------
// Purpose: Fills out a list of handles in the given solid that are at the same
//			position as the given handle in the current 2D view.
// Input  : pStrucSolid - 
//			hi - 
//			hAddSimilarList - 
//			pnAddSimilar - 
// Output : Returns a selected handle at the same position as the given handle,
//			if one exists, otherwise returns 0.
//-----------------------------------------------------------------------------
SSHANDLE Morph3D::Get2DMatches(CSSolid *pStrucSolid, SSHANDLEINFO &hi, SSHANDLE *hAddSimilarList, int *pnAddSimilar)
{
	bool bAddSimilar = true;
	SSHANDLE hNewMoveHandle = 0;

	if(hi.Type == shtVertex)
	{
		for(int i = 0; i < pStrucSolid->m_nVertices; i++)
		{
			CSSVertex & v = pStrucSolid->m_Vertices[i];
	
			// YWB Fixme, scale olerance to zoom amount?
			if( (fabs(hi.pos[axHorz] - v.pos[axHorz]) < 0.5) && 
				(fabs(hi.pos[axVert] - v.pos[axVert]) < 0.5) )
			{
				if(v.m_bSelected)
				{
					hNewMoveHandle = v.id;
				}

				// else add it to the array to select
				if(hAddSimilarList)
					hAddSimilarList[pnAddSimilar[0]++] = v.id;
			}
		}
	}
	else if(hi.Type == shtEdge)
	{
		for(int i = 0; i < pStrucSolid->m_nEdges; i++)
		{
			CSSEdge& e = pStrucSolid->m_Edges[i];
	
			if( (fabs(hi.pos[axHorz] - e.ptCenter[axHorz]) < 0.5) && 
				(fabs(hi.pos[axVert] - e.ptCenter[axVert]) < 0.5) )
			{
				if(e.m_bSelected)
				{
					hNewMoveHandle = e.id;
				}

				// else add it to the array to select
				if(hAddSimilarList)
					hAddSimilarList[pnAddSimilar[0]++] = e.id;
			}
		}
	}

	return hNewMoveHandle;
}


//-----------------------------------------------------------------------------
// Purpose: Selects all the handles that are of the same type and at the same
//			position in the current 2D projection as the given handle.
// Input  : pInfo - 
//			cmd - 
// Output : Returns the number of handles that were selected by this call.
//-----------------------------------------------------------------------------
int Morph3D::SelectHandle2D(MORPHHANDLE *pInfo, UINT cmd)
{
	SSHANDLEINFO hi;
	if (!pInfo->pStrucSolid->GetHandleInfo(&hi, pInfo->ssh))
	{
		// Can't find the handle info, bail.
		DeselectHandle(pInfo);
		return 0;
	}

	//
	// Check to see if there is a same type handle at the same
	// 2d coordinates.
	//
	SSHANDLE hAddSimilarList[64];
	int nAddSimilar = 0;
	SSHANDLE hNewMoveHandle = 0;

	Get2DMatches(pInfo->pStrucSolid, hi, hAddSimilarList, &nAddSimilar);

	for (int i = 0; i < nAddSimilar; i++)
	{
		MORPHHANDLE mh;
		mh.ssh = hAddSimilarList[i];
		mh.pStrucSolid = pInfo->pStrucSolid;
		mh.pMapSolid = pInfo->pStrucSolid->m_pMapSolid;

		SelectHandle(&mh, cmd);

		if (i == 0)
		{
			cmd &= ~scClear;
		}
	}

	return nAddSimilar;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInfo - 
//-----------------------------------------------------------------------------
void Morph3D::DeselectHandle(MORPHHANDLE *pInfo)
{
	for (int i = 0; i < m_nSelectedHandles; i++)
	{
		if (!memcmp(&m_SelectedHandles[i], pInfo, sizeof(*pInfo)))
		{
			m_SelectedHandles.RemoveAt(i);
			--m_nSelectedHandles;
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInfo - 
//			cmd - 
//-----------------------------------------------------------------------------
void Morph3D::SelectHandle(MORPHHANDLE *pInfo, UINT cmd)
{
	if(pInfo && pInfo->ssh == SSH_SCALEORIGIN)
		return;

	if(cmd & scSelectAll)
	{
		POSITION p = m_StrucSolids.GetHeadPosition();
		MORPHHANDLE mh;

		while(p)
		{	
			CSSolid *pStrucSolid = m_StrucSolids.GetNext(p);

			for(int i = 0; i < pStrucSolid->m_nVertices; i++)
			{
				CSSVertex& v = pStrucSolid->m_Vertices[i];
				mh.ssh = v.id;
				mh.pStrucSolid = pStrucSolid;
				mh.pMapSolid = pStrucSolid->m_pMapSolid;
				SelectHandle(&mh, scSelect);
			}
		}

		return;
	}

	if(cmd & scClear)
	{
		// clear handles first
		for(int i = 0; i < m_nSelectedHandles; i++)
		{
			SelectHandle(&m_SelectedHandles[i], scUnselect);
			--i;
		}
	}

	if(cmd == scClear)
	{
		if(m_bScaling)
			OnScaleCmd(TRUE);	// update scaling
		return;	// nothing else to do here
	}
	
	SSHANDLEINFO hi;
	if (!pInfo->pStrucSolid->GetHandleInfo(&hi, pInfo->ssh))
	{
		// Can't find the handle info, bail.
		DeselectHandle(pInfo);
		return;
	}

	if(hi.Type != m_SelectedType)
		SelectHandle(NULL, scClear);	// clear selection first

	m_SelectedType = hi.Type;

	bool bAlreadySelected = (hi.p2DHandle->m_bSelected == TRUE);
	bool bChanged = false;

	// toggle selection:
	if(cmd & scToggle)
	{
		cmd &= ~scToggle;
		cmd |= bAlreadySelected ? scUnselect : scSelect;
	}

	if(cmd & scSelect && !(hi.p2DHandle->m_bSelected))
	{
		hi.p2DHandle->m_bSelected = TRUE;
		bChanged = true;
	}
	else if(cmd & scUnselect && hi.p2DHandle->m_bSelected)
	{
		hi.p2DHandle->m_bSelected = FALSE;
		bChanged = true;
	}

	if(!bChanged)
		return;

	if(hi.p2DHandle->m_bSelected)
	{
		m_SelectedHandles.SetAtGrow(m_nSelectedHandles, *pInfo);
		++m_nSelectedHandles;
	}
	else
	{
		DeselectHandle(pInfo);
	}

	if(m_bScaling)
		OnScaleCmd(TRUE);
}


void Morph3D::MoveSelectedHandles(const Vector &Delta)
{
	POSITION p = m_StrucSolids.GetHeadPosition();
	while(p)
	{
		CSSolid *pStrucSolid = m_StrucSolids.GetNext(p);
		pStrucSolid->MoveSelectedHandles(Delta);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void Morph3D::RenderTool2D(CRender2D *pRender)
{
	CPoint pt;

	for (int nPass = 0; nPass < 2; nPass++)
	{
		POSITION p = m_StrucSolids.GetHeadPosition();
		while (p)
		{
			CSSolid *pStrucSolid = m_StrucSolids.GetNext(p);

			//
			// Draw the edges.
			//
			for (int i = 0; i < pStrucSolid->m_nEdges; i++)
			{
				CSSEdge *pEdge = & pStrucSolid->m_Edges[i];

				if (((pEdge->m_bSelected) && (nPass == 0)) ||
					((!pEdge->m_bSelected) && (nPass == 1)))
				{
					continue;
				}

				pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 255, 0, 0);

				SSHANDLEINFO hi;
				pStrucSolid->GetHandleInfo(&hi, pEdge->hvStart);
				WorldToScreen2D(pt, hi.pos);
				pRender->MoveTo(pt);

				pStrucSolid->GetHandleInfo(&hi, pEdge->hvEnd);
				WorldToScreen2D(pt, hi.pos);
				pRender->LineTo(pt);

				if (!(m_HandleMode & hmEdge))
				{
					// Don't draw edge handles.
					continue;
				}

				//
				// Draw the edge center handle.
				//
				PointToScreen(pEdge->ptCenter, pt);
				pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 0, 0, 0);
				if (pEdge->m_bSelected)
				{
					pRender->SetFillColor(GetRValue(Options.colors.clrSelection), GetGValue(Options.colors.clrSelection), GetBValue(Options.colors.clrSelection));
				}
				else
				{
					pRender->SetFillColor(255, 255, 0); // dvsFIXME: need a setting for unselected edge handle color
				}
				pRender->Rectangle(pt.x - 3, pt.y - 3, pt.x + 4, pt.y + 4, true);
			}

			if (!(m_HandleMode & hmVertex))
			{
				// Don't draw vertex handles.
				continue;
			}

			//
			// Draw vertex handles.
			//
			pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 0, 0, 0);

			for (i = 0; i < pStrucSolid->m_nVertices; i++)
			{
				CSSVertex &v = pStrucSolid->m_Vertices[i];

				if (((v.m_bSelected) && (nPass == 0)) ||
					((!v.m_bSelected) && (nPass == 1)))
				{
					continue;
				}

				WorldToScreen2D(pt, v.pos);

				if (v.m_bSelected)
				{
					pRender->SetFillColor(GetRValue(Options.colors.clrSelection), GetGValue(Options.colors.clrSelection), GetBValue(Options.colors.clrSelection));
				}
				else
				{
					pRender->SetFillColor(GetRValue(Options.colors.clrToolHandle), GetGValue(Options.colors.clrToolHandle), GetBValue(Options.colors.clrToolHandle));
				}

				pRender->Rectangle(pt.x - 3, pt.y - 3, pt.x + 4, pt.y + 4, true);
			}
		}
	}

	//
	// Draw scaling point.
	//
	if (m_bScaling && m_nSelectedHandles)
	{
		pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, GetRValue(Options.colors.clrToolHandle), GetGValue(Options.colors.clrToolHandle), GetBValue(Options.colors.clrToolHandle));

		WorldToScreen2D(pt, m_ScaleOrg);
		pRender->DrawEllipse(pt, 8, 8, false);
		pRender->DrawEllipse(pt, 7, 7, false);
	}

	if (m_bBoxSelecting)
	{
		Box3D::RenderTool2D(pRender);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Finishes the morph, committing changes made to the selected objects.
//-----------------------------------------------------------------------------
void Morph3D::SetEmpty()
{
	GetHistory()->MarkUndoPosition(NULL, "Morphing");

	while(1)
	{
		// keep getting the head position because SelectObject (below)
		//  removes the object from the list.
		POSITION p = m_StrucSolids.GetHeadPosition();
		if(!p)
			break;
		CSSolid *pStrucSolid = m_StrucSolids.GetAt(p);
		
		//
		// Save this solid. BUT, before doing so, set it as visible in the 2D views.
		// Otherwise, it will vanish if the user does an "Undo Morphing".
		//
		pStrucSolid->m_pMapSolid->SetVisible2D(true);
		GetHistory()->Keep(pStrucSolid->m_pMapSolid);
		pStrucSolid->m_pMapSolid->SetVisible2D(false);

		// calling SelectObject with scUnselect SAVES the contents
		//  of the morph.
		SelectObject(pStrucSolid->m_pMapSolid, scUnselect);
	}

	m_LastThirdAxis = -1;
}


// 3d translation --
void Morph3D::StartTranslation(MORPHHANDLE *pInfo)
{
	Tool3D::StartTranslation(CPoint());

	if(m_bScaling)
	{
		// back to 1
		m_bScaling = false;	// don't want it to update here
		m_ScaleDlg.m_cScale.SetWindowText("1.0");
		m_bScaling = true;
	}

	if(pInfo->ssh != SSH_SCALEORIGIN)
		GetHandlePos(pInfo, m_OrigHandlePos);
	m_DragHandle = *pInfo;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pNewPosition - 
//-----------------------------------------------------------------------------
void Morph3D::UpdateTranslation(Vector& pNewPosition)
{
	if (m_DragHandle.ssh == SSH_SCALEORIGIN)
	{
		m_bUpdateOrg = false;
		return;
	}

	// create delta
	Vector delta, curpos;
	GetHandlePos(&m_DragHandle, curpos);
	for(int i = 0; i < 3; i++)
	{
		delta[i] = pNewPosition[i] - curpos[i];
	}

	MoveSelectedHandles(delta);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			uFlags - 
//			& - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Morph3D::UpdateTranslation(CPoint pt, UINT uFlags, CSize &)
{
	if (m_bBoxSelecting)
	{
		return Box3D::UpdateTranslation(pt, uFlags);
	}

	bool bSnap = (CMapDoc::GetActiveMapDoc()->m_bSnapToGrid && !(GetAsyncKeyState(VK_MENU) & 0x8000));

	if (m_DragHandle.ssh == SSH_SCALEORIGIN)
	{
		if (bSnap)
		{
			pt.x = m_pDocument->Snap(pt.x);
			pt.y = m_pDocument->Snap(pt.y);
		}

		// set scaling position
		if (m_ScaleOrg[axHorz] == pt.x && m_ScaleOrg[axVert] == pt.y)
		{
			return FALSE;
		}

		m_ScaleOrg[axHorz] = pt.x;
		m_ScaleOrg[axVert] = pt.y;
		m_bUpdateOrg = false;
		return TRUE;
	}

	//
	// Get the current handle position.
	//
	Vector curpos;
	GetHandlePos(&m_DragHandle, curpos);

	CPoint SnapDelta(0, 0);
	if (bSnap)
	{
		//
		// We don't want to snap edge handles to the grid, because they don't
		// necessarily belong on the grid in the first place.
		//
		if (m_SelectedType == shtEdge)
		{
			SnapDelta.x = curpos[axHorz] - m_pDocument->Snap(curpos[axHorz]);
			SnapDelta.y = curpos[axVert] - m_pDocument->Snap(curpos[axVert]);
		}

		pt.x = m_pDocument->Snap(pt.x - SnapDelta.x);
		pt.y = m_pDocument->Snap(pt.y - SnapDelta.y);
	}

	//
	// Create delta and determine if it is large enough to warrant an update.
	//
	Vector delta;
	delta[axHorz] = (pt.x - curpos[axHorz]) + SnapDelta.x;
	delta[axVert] = (pt.y - curpos[axVert]) + SnapDelta.y;
	delta[axThird] = 0.0f;

	if ((fabs(delta[axHorz]) < 0.5f) && (fabs(delta[axVert]) < 0.5f))
	{
		return FALSE;	// no need to update.
	}

	MoveSelectedHandles(delta);

	return(TRUE);
}


BOOL Morph3D::StartTranslation(CPoint &pt)
{
	if(m_bBoxSelecting)
		Box3D::StartTranslation(pt);
	else
		Tool3D::StartTranslation(pt);

	return FALSE;
}

BOOL Morph3D::StartBoxSelection(Vector& pt3)
{
	m_bBoxSelecting = true;
	SetDrawColors(RGB(255, 255, 255), RGB(50, 255, 255));

	CPoint pt(pt3[axHorz], pt3[axVert]);
	m_pDocument->Snap(pt);
	Tool3D::StartTranslation(pt);
	
	bPreventOverlap = FALSE;
	stat.TranslateMode = modeScale;
	stat.bNewBox = TRUE;

	// disabled for now.
	if(0)//axThird == m_LastThirdAxis)
	{
		tbmins[axThird] = m_LastThirdAxisExtents[0];
		tbmaxs[axThird] = m_LastThirdAxisExtents[1];
	}
	else
	{
		tbmins[axThird] = m_pDocument->Snap(pt3[axThird]);
		tbmaxs[axThird] = tbmins[axThird];	// infinite box first
	}

	m_LastThirdAxis = axThird;

	tbmins[axHorz] = tbmaxs[axHorz] = pt.x;
	tbmins[axVert] = tbmaxs[axVert] = pt.y;
	iTransOrigin = inclBottomRight;
	ZeroVector(m_Scale);
	bmins = tbmins;
	bmaxs = tbmaxs;

	return TRUE;
}


void Morph3D::SelectInBox()
{
	if(!m_bBoxSelecting)
		return;

	// select all vertices within the box, and finish box
	//  selection.

	EndBoxSelection();	// may as well do it here

	m_LastThirdAxisExtents[0] = bmins[axThird];
	m_LastThirdAxisExtents[1] = bmaxs[axThird];

	// expand box along 0-depth axes
	int countzero = 0;
	for(int i = 0; i < 3; i++)
	{
		if(bmaxs[i] - bmins[i] == 0)
		{
			bmaxs[i] = COORD_NOTINIT;
			bmins[i] = -COORD_NOTINIT;
			countzero++;
		}
	}
	if(countzero > 1)
		return;

	POSITION p = m_StrucSolids.GetHeadPosition();

	while(p)
	{	
		CSSolid *pStrucSolid = m_StrucSolids.GetNext(p);

		for(i = 0; i < pStrucSolid->m_nVertices; i++)
		{
			CSSVertex& v = pStrucSolid->m_Vertices[i];
			for(int i2 = 0; i2 < 3; i2++)
			{
				if(v.pos[i2] < bmins[i2] || v.pos[i2] > bmaxs[i2])
					break;
			}
			
			if(i2 == 3)
			{
				// completed loop - intersects - select handle
				MORPHHANDLE mh;
				mh.ssh = v.id;
				mh.pStrucSolid = pStrucSolid;
				mh.pMapSolid = pStrucSolid->m_pMapSolid;
				SelectHandle(&mh, scSelect);
			}
		}
	}
}

void Morph3D::EndBoxSelection()
{
	m_bBoxSelecting = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bSave - 
//-----------------------------------------------------------------------------
void Morph3D::FinishTranslation(BOOL bSave)
{
	if (m_bBoxSelecting)
	{
		Box3D::FinishTranslation(bSave);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		return;
	}
	else if (bSave && m_DragHandle.ssh != SSH_SCALEORIGIN)
	{
		// figure out all the affected solids
		CTypedPtrList<CPtrList, CSSolid*> Affected;
		POSITION p = m_StrucSolids.GetHeadPosition();
		while(p)
		{
			CSSolid *pStrucSolid = m_StrucSolids.GetNext(p);
			if(Affected.Find(pStrucSolid) == NULL)
				Affected.AddTail(pStrucSolid);
		}

		p = Affected.GetHeadPosition();
		int iConfirm = -1;
		while(p)
		{
			CSSolid *pStrucSolid = Affected.GetNext(p);
			if(pStrucSolid->CanMergeVertices() && iConfirm != 0)
			{
				if(iConfirm == -1)
				{
					// ask
					if(AfxMessageBox("Merge vertices?", MB_YESNO) == IDYES)
						iConfirm = 1;
					else
						iConfirm = 0;
				}
				if(iConfirm == 1)
				{
					int nDeleted;
					SSHANDLE *pDeleted = pStrucSolid->MergeSameVertices(nDeleted);
					// ensure deleted handles are not marked
					for(int i = 0; i < nDeleted; i++)
					{
						MORPHHANDLE mh;
						mh.ssh = pDeleted[i];
						mh.pStrucSolid = pStrucSolid;
						mh.pMapSolid = pStrucSolid->m_pMapSolid;
						SelectHandle(&mh, scUnselect);
					}
				}
			}
//			pStrucSolid->CheckFaces();
		}
	}

	Tool3D::FinishTranslation(bSave);

	if(!bSave)
	{
		// move back to original positions
		UpdateTranslation(m_OrigHandlePos);
	}
	else if(m_bScaling)
	{
		OnScaleCmd(TRUE);
	}
}


BOOL Morph3D::SplitFace()
{
	if(!CanSplitFace())
		return FALSE;

	if(m_SelectedHandles[0].pStrucSolid->SplitFace(m_SelectedHandles[0].ssh,
		m_SelectedHandles[1].ssh))
	{
		// unselect those invalid edges
		if(m_SelectedType == shtVertex)
		{
			// proper deselection
			SelectHandle(NULL, scClear);
		}
		else	// selection is invalid; set count to 0
			m_nSelectedHandles = 0;
		return TRUE;
	}

	return FALSE;
}


BOOL Morph3D::CanSplitFace()
{
	// along two edges.
	if(m_nSelectedHandles != 2 || (m_SelectedType != shtEdge && 
		m_SelectedType != shtVertex))
		return FALSE;

	// make sure same solid.
	if(m_SelectedHandles[0].pStrucSolid != m_SelectedHandles[1].pStrucSolid)
		return FALSE;

	return TRUE;
}


void Morph3D::ToggleMode()
{
	if(m_HandleMode == hmBoth)
		m_HandleMode = hmVertex;
	else if(m_HandleMode == hmVertex)
		m_HandleMode = hmEdge;
	else
		m_HandleMode = hmBoth;

	// run through selected solids and tell them the new mode
	POSITION p = m_StrucSolids.GetHeadPosition();
	while(p)
	{
		CSSolid *pStrucSolid = m_StrucSolids.GetNext(p);
		pStrucSolid->ShowHandles(m_HandleMode & hmVertex, m_HandleMode & hmEdge);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the center of the morph selection.
// Input  : pt - Point at the center of the selection or selected handles.
//-----------------------------------------------------------------------------
void Morph3D::GetSelectedCenter(Vector& pt)
{
	BoundBox box;

	//
	// If we have selected handles, our bounds center is the center of those handles.
	//
	if (m_nSelectedHandles != 0)
	{
		SSHANDLEINFO hi;

		for (int i = 0; i < m_nSelectedHandles; i++)
		{
			MORPHHANDLE *mh = &m_SelectedHandles[i];
			mh->pStrucSolid->GetHandleInfo(&hi, mh->ssh);
			box.UpdateBounds(hi.pos);
		}
	}
	//
	// If no handles are selected, our bounds center is the center of all selected solids.
	//
	else
	{
		POSITION pos = m_StrucSolids.GetHeadPosition();
		
		while (pos != NULL)
		{
			CSSolid *pStrucSolid = m_StrucSolids.GetNext(pos);
			for (int nVertex = 0; nVertex < pStrucSolid->m_nVertices; nVertex++)
			{
				CSSVertex &v = pStrucSolid->m_Vertices[nVertex];
				box.UpdateBounds(v.pos);
			}
		}
	}

	box.GetBoundsCenter(pt);
}


//-----------------------------------------------------------------------------
// Purpose: Fills out a list of the objects selected for morphing.
//-----------------------------------------------------------------------------
void Morph3D::GetMorphingObjects(CUtlVector<CMapClass *> &List)
{
	POSITION p = m_StrucSolids.GetHeadPosition();
	while (p)
	{
		CSSolid *pStrucSolid = m_StrucSolids.GetNext(p);
		List.AddToTail(pStrucSolid->m_pMapSolid);
	}
}


void Morph3D::OnScaleCmd(BOOL bReInit)
{
	if(m_pOrigPosList)
	{
		delete[] m_pOrigPosList;
		m_pOrigPosList = NULL;
	}

	if(m_bScaling && !bReInit)
	{
		m_ScaleDlg.ShowWindow(SW_HIDE);
		m_ScaleDlg.DestroyWindow();
		m_bScaling = false;
		return;
	}

	// start scaling
	if(!bReInit)
	{
		m_ScaleDlg.Create(IDD_SCALEVERTICES);
		CPoint pt;
		GetCursorPos(&pt);
		m_bUpdateOrg = true;

		m_ScaleDlg.SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
	else
	{
		m_bScaling = false;	// don't want an update
		m_ScaleDlg.m_cScale.SetWindowText("1.0");
		m_bScaling = true;
	}

	if(!m_nSelectedHandles)
	{
		m_bScaling = true;
		return;
	}

	m_pOrigPosList = new Vector[m_nSelectedHandles];

	BoundBox box;

	// save original positions of vertices
	for(int i = 0; i < m_nSelectedHandles; i++)
	{
		MORPHHANDLE &hnd = m_SelectedHandles[i];
		SSHANDLEINFO hi;
		hnd.pStrucSolid->GetHandleInfo(&hi, hnd.ssh);

		if(hi.Type != shtVertex)
			continue;

		m_pOrigPosList[i] = hi.pos;
		box.UpdateBounds(hi.pos);
	}

	// center is default origin
	if(m_bUpdateOrg)
		box.GetBoundsCenter(m_ScaleOrg);

	m_bScaling = true;
}


void Morph3D::UpdateScale()
{
	// update scale with data in dialog box
	if(!m_bScaling)
		return;

	float fScale = m_ScaleDlg.m_fScale;

	// match up selected vertices to original position in m_pOrigPosList.
	int iMoved = 0;
	for(int i = 0; i < m_nSelectedHandles; i++)
	{
		MORPHHANDLE &hnd = m_SelectedHandles[i];
		SSHANDLEINFO hi;
		hnd.pStrucSolid->GetHandleInfo(&hi, hnd.ssh);

		if(hi.Type != shtVertex)
			continue;

		// ** scale **
		Vector& pOrigPos = m_pOrigPosList[iMoved++];
		Vector newpos;
		for(int d = 0; d < 3; d++)
		{
			float delta = pOrigPos[d] - m_ScaleOrg[d];
			// YWB rounding
			newpos[d] = /*rint*/(m_ScaleOrg[d] + (delta * fScale));
		}

		hnd.pStrucSolid->SetVertexPosition(hi.iIndex, newpos[0], 
			newpos[1], newpos[2]);

		// find edge that references this vertex
		int nEdges;
		CSSEdge **pEdges = hnd.pStrucSolid->FindAffectedEdges(&hnd.ssh, 1, 
			nEdges);
		for(int e = 0; e < nEdges; e++)
		{
			hnd.pStrucSolid->CalcEdgeCenter(pEdges[e]);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//			pMesh - 
//			ScreenPos - 
//			color - 
//-----------------------------------------------------------------------------
void Morph3D::RenderMorphHandle(CRender3D *pRender, IMesh *pMesh, const Vector2D &ScreenPos, unsigned char *color)
{
	//
	// Render the edge handle as a box.
	//
	pRender->BeginParallel();

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_POLYGON, 4 );

	meshBuilder.Position3f(ScreenPos[0] - MORPH_VERTEX_DIST, ScreenPos[1] - MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ubv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] - MORPH_VERTEX_DIST, ScreenPos[1] + MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ubv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] + MORPH_VERTEX_DIST, ScreenPos[1] + MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ubv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] + MORPH_VERTEX_DIST, ScreenPos[1] - MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ubv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	meshBuilder.Begin( pMesh, MATERIAL_LINE_LOOP, 4 );

	meshBuilder.Position3f(ScreenPos[0] - MORPH_VERTEX_DIST, ScreenPos[1] - MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ub(20, 20, 20);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] - MORPH_VERTEX_DIST, ScreenPos[1] + MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ub(20, 20, 20);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] + MORPH_VERTEX_DIST, ScreenPos[1] + MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ub(20, 20, 20);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(ScreenPos[0] + MORPH_VERTEX_DIST, ScreenPos[1] - MORPH_VERTEX_DIST, 0);
	meshBuilder.Color3ub(20, 20, 20);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	pRender->EndParallel();
}


//-----------------------------------------------------------------------------
// Purpose: Renders an object that is currently selected for morphing. The
//			object is rendered in three passes:
//
//			1. Flat shaded grey with transparency.
//			2. Wireframe in white.
//			3. Edges and/or vertices are rendered as boxes.
//
// Input  : pSolid - The structured solid to render.
//-----------------------------------------------------------------------------
void Morph3D::RenderSolid3D(CRender3D *pRender, CSSolid *pSolid)
{
	matrix4_t ViewMatrix;
	Vector ViewPos;

	for (int nPass = 1; nPass <= 3; nPass++)
	{
		if (nPass == 1)
		{
			pRender->SetRenderMode( RENDER_MODE_SELECTION_OVERLAY );
		}
		else if (nPass == 2)
		{
			pRender->SetRenderMode( RENDER_MODE_WIREFRAME );
		}
		else
		{
			pRender->SetRenderMode( RENDER_MODE_SELECTION_OVERLAY );
			MaterialSystemInterface()->GetMatrix(MATERIAL_VIEW, &ViewMatrix[0][0]);
			TransposeMatrix( ViewMatrix );
		}

		IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
		CMeshBuilder meshBuilder;

		int nFaceCount = pSolid->GetFaceCount();
		for (int nFace = 0; nFace < nFaceCount; nFace++)
		{
			CSSFace *pFace = pSolid->GetFace(nFace);

			int nEdgeCount = pFace->GetEdgeCount();

			unsigned char color[4];
			if (nPass == 1)
			{
				meshBuilder.Begin( pMesh, MATERIAL_POLYGON, nEdgeCount );
				color[0] = color[1] = color[2] = color[3] = 128; 
			}
			else if (nPass == 2)
			{
				meshBuilder.Begin( pMesh, MATERIAL_LINE_LOOP, nEdgeCount );
				color[0] = color[1] = color[2] = color[3] = 255; 
			}

			for (int nEdge = 0; nEdge < nEdgeCount; nEdge++)
			{
				//
				// Calc next edge so we can see which is the next clockwise point.
				//
				int nEdgeNext = nEdge + 1;
				if (nEdgeNext == nEdgeCount)
				{
					nEdgeNext = 0;
				}

				SSHANDLE hEdge = pFace->GetEdgeHandle(nEdge);
				CSSEdge *pEdgeCur = (CSSEdge *)pSolid->GetHandleData(hEdge);

				SSHANDLE hEdgeNext = pFace->GetEdgeHandle(nEdgeNext);
				CSSEdge *pEdgeNext = (CSSEdge *)pSolid->GetHandleData(hEdgeNext);

				if (!pEdgeCur || !pEdgeNext)
				{
					return;
				}

				if ((nPass == 1) || (nPass == 2))
				{
					SSHANDLE hVertex = pSolid->GetConnectionVertex(pEdgeCur, pEdgeNext);

					if (!hVertex)
					{
						return;
					}

					CSSVertex *pVertex = (CSSVertex *)pSolid->GetHandleData(hVertex);

					if (!pVertex)
					{
						return;
					}

					Vector Vertex;
					pVertex->GetPosition(Vertex);
					meshBuilder.Position3f(Vertex[0], Vertex[1], Vertex[2]);
					meshBuilder.Color4ubv( color );
					meshBuilder.AdvanceVertex();
				}
				else
				{
					if (pSolid->ShowEdges())
					{
						//
						// Project the edge midpoint into screen space.
						//
						Vector CenterPoint;
						pEdgeCur->GetCenterPoint(CenterPoint);

						matrix4_t inv;
						memcpy(&inv, &ViewMatrix, 16 * sizeof(float) );
						MatrixInvert( inv );
						MatrixMultiplyPoint(ViewPos, CenterPoint, ViewMatrix);

						if (ViewPos[2] < 0)
						{
							Vector2D ScreenPos;
							pRender->WorldToScreen(ScreenPos, CenterPoint);

							Vector2D ClientPos;
							pRender->ScreenToClient(ClientPos, ScreenPos);

							pEdgeCur->m_bVisible = TRUE;
							pEdgeCur->m_r.left = (long)(ClientPos[0] - MORPH_VERTEX_DIST - 1);
							pEdgeCur->m_r.top = (long)(ClientPos[1] - MORPH_VERTEX_DIST - 1);
							pEdgeCur->m_r.right = (long)(ClientPos[0] + MORPH_VERTEX_DIST + 2);
							pEdgeCur->m_r.bottom = (long)(ClientPos[1] + MORPH_VERTEX_DIST + 2);

							if (pEdgeCur->m_bSelected)
							{
								color[0] = 220; color[1] = color[2] = 0; color[3] = 255;
							}
							else
							{
								color[0] = color[1] = 255; color[2] = 0; color[3] = 255;

							}

							//
							// Render the edge handle as a box.
							//
							RenderMorphHandle( pRender, pMesh, ScreenPos, color );
						}
						else
						{
							pEdgeCur->m_bVisible = FALSE;
						}
					}

					if (pSolid->ShowVertices())
					{
						SSHANDLE hVertex = pSolid->GetConnectionVertex(pEdgeCur, pEdgeNext);
						CSSVertex *pVertex = (CSSVertex *)pSolid->GetHandleData(hVertex);

						//
						// Project the vertex into screen space.
						//
						Vector Point;

						pVertex->GetPosition(Point);
						MatrixMultiplyPoint(ViewPos, Point, ViewMatrix);

						if (ViewPos[2] < 0)
						{
							Vector2D ScreenPos;
							pRender->WorldToScreen(ScreenPos, Point);

							Vector2D ClientPos;
							pRender->ScreenToClient(ClientPos, ScreenPos);

							pVertex->m_bVisible = TRUE;
							pVertex->m_r.left = (long)(ClientPos[0] - MORPH_VERTEX_DIST - 1);
							pVertex->m_r.top = (long)(ClientPos[1] - MORPH_VERTEX_DIST - 1);
							pVertex->m_r.right = (long)(ClientPos[0] + MORPH_VERTEX_DIST + 2);
							pVertex->m_r.bottom = (long)(ClientPos[1] + MORPH_VERTEX_DIST + 2);

							if (pVertex->m_bSelected)
							{
								color[0] = 220; color[1] = color[2] = 0; color[3] = 255;
							}
							else
							{
								color[0] = color[1] = color[2] = 255; color[3] = 255;
							}

							//
							// Render the vertex as a box.
							//
							RenderMorphHandle( pRender, pMesh, ScreenPos, color );
						}
						else
						{
							pVertex->m_bVisible = FALSE;
						}
					}
				}
			}

			if ((nPass == 1) || (nPass == 2))
			{
				meshBuilder.End();
				pMesh->Draw();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders a our selection bounds while we are drag-selecting.
// Input  : pRender - Rendering interface.
//-----------------------------------------------------------------------------
void Morph3D::RenderTool3D(CRender3D *pRender)
{
	if (m_bBoxSelecting)
	{
		Box3D::RenderTool3D(pRender);
	}

	POSITION pos;
	CSSolid *pSolid = GetFirstObject(pos);
	while (pSolid != NULL)
	{
		RenderSolid3D(pRender, pSolid);
		pSolid = GetNextObject(pos);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles key down events in the 2D view.
// Input  : Per CWnd::OnKeyDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Morph3D::OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CMapDoc *pDoc = pView->GetDocument();

	bool bCtrl = ((GetKeyState(VK_CONTROL) & 0x8000) != 0);
	if (Options.view2d.bNudge && (nChar == VK_UP || nChar == VK_DOWN || nChar == VK_LEFT || nChar == VK_RIGHT))
	{
		if (GetSelectedHandleCount())
		{
			pDoc->NudgeObjects(*this, nChar, !bCtrl);
			return true;
		}
	}

	switch (nChar)
	{
		case VK_RETURN:
		{
			if (IsBoxSelecting())
			{
				SelectInBox();
				pDoc->ToolUpdateViews(CMapView2D::updTool);
			}
			break;
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
// Purpose: Handles character events in the 2D view.
// Input  : Per CWnd::OnChar.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Morph3D::OnChar2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button down events in the 2D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Morph3D::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	CMapDoc *pDoc = pView->GetDocument();
	CMapWorld *pWorld = pDoc->GetMapWorld();

	m_bLButtonDown = true;
	m_bLButtonDownControlState = (nFlags & MK_CONTROL) != 0;

	m_ptLDownClient = point;
	m_ptLastMouseMovement = point;

	pView->SetCapture();

	//
	// Convert to some odd coordinate space that the base tools code uses.
	//
  	CPoint ptScreen = point;
	ptScreen.x += pView->GetScrollPos(SB_HORZ);
	ptScreen.y += pView->GetScrollPos(SB_VERT);
	
	m_DragHandle.ssh = 0;

	MORPHHANDLE mh;
	if (IsBoxSelecting())
	{
		if (HitTest(ptScreen, FALSE) != -1)
		{
			StartTranslation(ptScreen);
		}
	}
	else if (MorphHitTest2D(ptScreen, &mh))
	{
		//
		// If they clicked on a valid handle, remember which one. We may need it in
		// left button up or mouse move messages.
		//
		m_DragHandle = mh;

		if (!m_bLButtonDownControlState)
		{
			//
			// If they are not holding down control and they clicked on an unselected
			// handle, select the handle they clicked on straightaway.
			//
			if (!IsSelected(m_DragHandle))
			{
				// Clear the selected handles and select this handle.
				UINT cmd = Morph3D::scClear | Morph3D::scSelect;
				SelectHandle2D(&m_DragHandle, cmd);
				m_pDocument->ToolUpdateViews(CMapView2D::updTool);
			}
		}
	}
	else
	{
		// Try to put another solid into morph mode.
		pView->SelectAt(point, !(nFlags & MK_CONTROL), false);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button up events in the 2D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Morph3D::OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	//
	// Convert to some odd coordinate space that the base tools code uses.
	//
  	CPoint ptScreen = point;
	ptScreen.x += pView->GetScrollPos(SB_HORZ);
	ptScreen.y += pView->GetScrollPos(SB_VERT);

	ReleaseCapture();

	m_bLButtonDown = false;

	if (!IsTranslating())
	{
		if (m_DragHandle.ssh != 0)
		{
			//
			// They clicked on a handle and released the left button without moving the mouse.
			// Change the selection state of the handle that was clicked on.
			//
			UINT cmd = Morph3D::scClear | Morph3D::scSelect;
			if (m_bLButtonDownControlState)
			{
				// Control-click: toggle.
				cmd = Morph3D::scToggle;
			}
			SelectHandle2D(&m_DragHandle, cmd);
			m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		}
	}
	else
	{
		//
		// Dragging out a selection box or dragging the selected vertices.
		//
		FinishTranslation(TRUE);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);

		if (IsBoxSelecting() && Options.view2d.bAutoSelect)
		{
			SelectInBox();
			m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		}
	}

	m_pDocument->UpdateStatusbar();
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles mouse move events in the 2D view.
// Input  : Per CWnd::OnMouseMove.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Morph3D::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	bool bCursorSet = false;
	bool bDisableSnap = (GetAsyncKeyState(VK_MENU) & 0x8000) ? true : false;
					    
	if (IsTranslating())
	{
		//
		// Make sure the point is visible.
		//
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

	if (m_bLButtonDown)
	{
		if (!IsTranslating() && (m_DragHandle.ssh != 0))
		{
			//
			// If they are not already dragging a handle and they clicked on a valid handle,
			// see if they have moved the mouse far enough to begin dragging the handle.
			//
			int const DRAG_THRESHHOLD = 1;
			CSize sizeDragged = point - m_ptLDownClient;
			if ((abs(sizeDragged.cx) > DRAG_THRESHHOLD) || (abs(sizeDragged.cy) > DRAG_THRESHHOLD))
			{
				if (m_bLButtonDownControlState && !IsSelected(m_DragHandle))
				{
					//
					// If they control-clicked on an unselected handle and then dragged the mouse,
					// select the handle that they clicked on now.
					//
					SelectHandle2D(&m_DragHandle, Morph3D::scSelect);
				}

				StartTranslation(&m_DragHandle);
			}
		}

		if (IsTranslating())
		{
			//
			// If they are dragging a selection box or one or more handles, update
			// the drag based on the cursor position.
			//
			bCursorSet = true;

			UINT uConstraints = bDisableSnap ? Tool3D::constrainNosnap : 0;
			if (UpdateTranslation(point, uConstraints, CSize(0,0)))
			{
				m_pDocument->ToolUpdateViews(CMapView2D::updTool);
				m_pDocument->Update3DViews();
			}
		}
		else if (!IsBoxSelecting())
		{
			//
			// Left dragging, didn't click on a handle, and we aren't yet dragging a
			// selection box. Start dragging the selection box.
			//
			if (!(nFlags & MK_CONTROL))
			{
				SelectHandle(NULL, Morph3D::scClear);
			}

			Vector vecStart( COORD_NOTINIT, COORD_NOTINIT, COORD_NOTINIT );
			CPoint ptTmp = m_ptLDownClient;
			pView->ClientToWorld(ptTmp);
			vecStart[axHorz] = ptTmp.x;
			vecStart[axVert] = ptTmp.y;

			m_pDocument->GetBestVisiblePoint(vecStart);

			if (!bDisableSnap)
			{
				m_pDocument->Snap(vecStart);
			}

			StartBoxSelection(vecStart);
			pView->SetUpdateFlag(CMapView2D::updTool);
		}
	}
	else if (!IsEmpty())
	{
		//
		// Left button is not down, just see what's under the cursor
		// position to update the cursor.
		//

		//
		// Check to see if the mouse is over a vertex handle.
		//
		if (!IsBoxSelecting() && MorphHitTest2D(ptScreen, NULL))
		{
			bCursorSet = true;
		}
		//
		// Check to see if the mouse is over a box handle.
		//
		else if (HitTest(ptScreen, TRUE) != -1)
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
// Purpose: Handles left mouse button down events in the 3D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Morph3D::OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	m_bHit = false;

	CMapDoc *pDoc = pView->GetDocument();

	//
	// Select morph handles?
	//
	MORPHHANDLE mh;
	if (MorphHitTest3D(pView, point, &mh))
	{
		m_bHit = true;
		m_DragHandle = mh;
		m_bMorphing = true;
		m_ptLastMouseMovement = point;
		m_bMovingSelected = false;	// not moving them yet - might just select this
		StartTranslation(&m_DragHandle);
	}
	else 
	{ 
		CMapClass *pMorphObject = NULL;
		bool bUpdate2D = false;
		pDoc->ClearHitList();
		CMapObjectList SelectList;

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

		//
		// Actual selection occurs here.
		//
		if (!SelectList.GetCount())
		{
			//
			// Clicked on nothing - clear selection.
			//
			pView->GetDocument()->SelectFace(NULL, 0, CMapDoc::scClear);
			pView->GetDocument()->SelectObject(NULL, CMapDoc::scClear | CMapDoc::scUpdateDisplay);
			return true;
		}

		POSITION p = SelectList.GetHeadPosition();
		bool bFirst = true;
		SelectMode_t eSelectMode = pDoc->Selection_GetMode();

		// Can we de-select objects?
		if ( !CanDeselectList() )
			return true;

		while (p)
		{
			CMapClass *pObject = SelectList.GetNext(p);

			// get hit object type and add it to the hit list
			CMapClass *pHitObject = pObject->PrepareSelection( eSelectMode );
			if (pHitObject)
			{
				pDoc->AddHit( pHitObject );
						
				if (bFirst)
				{
					if (pObject->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
					{
						CMapSolid *pSolid = (CMapSolid *)pObject;
						
						UINT cmd = Morph3D::scClear | Morph3D::scSelect;
						if (nFlags & MK_CONTROL)
						{
							cmd = Morph3D::scToggle;
						}
						SelectObject(pSolid, cmd);
						pMorphObject = pSolid;
						bUpdate2D = true;
						break;
					}
				}
			
				bFirst = false;
			}
		}

		// do we want to deselect all morphs?
		if (!pMorphObject && !IsEmpty())
		{
			SetEmpty();
			bUpdate2D = true;
		}

		if (bUpdate2D)
		{
			UpdateBox ub;
			ub.Objects = NULL;
			GetMorphBounds(ub.Box.bmins, ub.Box.bmaxs, true);

			if (pMorphObject)
			{
				Vector mins;
				Vector maxs;
				pMorphObject->GetRender2DBox(mins, maxs);
				ub.Box.UpdateBounds(mins, maxs);
			}
			pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_2D | MAPVIEW_UPDATE_OBJECTS, &ub);
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button up events in the 3D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Morph3D::OnLMouseUp3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	if (m_bHit)
	{
		m_bHit = false;
		UINT cmd = Morph3D::scClear | Morph3D::scSelect;
		if (nFlags & MK_CONTROL)
		{
			cmd = Morph3D::scToggle;
		}

		SelectHandle(&m_DragHandle, cmd);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
	}

	if (m_bMorphing)
	{
		FinishTranslation(TRUE);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		m_bMorphing = false;
	}

	ReleaseCapture();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the key down event in the 3D view.
// Input  : Per CWnd::OnKeyDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Morph3D::OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	bool bCtrl = ((GetKeyState(VK_CONTROL) & 0x8000) != 0);

	if (Options.view2d.bNudge && (nChar == VK_UP || nChar == VK_DOWN || nChar == VK_LEFT || nChar == VK_RIGHT))
	{
		if (GetSelectedHandleCount())
		{
			Axes2 BestAxes;
			pView->CalcBestAxes(BestAxes);
			m_pDocument->NudgeObjects(BestAxes, nChar, !bCtrl);
			return true;
		}
	}

	switch (nChar)
	{
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
void Morph3D::OnEscape(void)
{
	//
	// If we're box selecting with the morph tool, stop.
	//
	if (IsBoxSelecting())
	{
		EndBoxSelection();
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
	}
	//
	// If we have handle(s) selected, deselect them.
	//
	else if (!IsEmpty() && (GetSelectedHandleCount() != 0))
	{
		SelectHandle(NULL, Morph3D::scClear);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
	}
	//
	// Stop using the morph tool.
	//
	else
	{
		g_pToolManager->SetTool(TOOL_POINTER);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles the move move event in the 3D view.
// Input  : Per CWnd::OnMouseMove.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Morph3D::OnMouseMove3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	MORPHHANDLE mh;
	if (!MorphHitTest3D(pView, point, &mh))
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			point - 
// Output : 
//-----------------------------------------------------------------------------
bool Morph3D::OnIdle3D(CMapView3D *pView, CPoint &point)
{
	if (!m_bMorphing)
	{
		return false;
	}

	int nFlags = 0;

	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0)
	{
		nFlags |= MK_CONTROL;
	}

	if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
	{
		nFlags |= MK_SHIFT;
	}

	if (m_bHit)
	{
		m_bHit = false;
		SSHANDLEINFO hi;
		m_DragHandle.pStrucSolid->GetHandleInfo(&hi, m_DragHandle.ssh);
		unsigned uSelFlags = Morph3D::scSelect;

		if (!(nFlags & MK_CONTROL) && !hi.p2DHandle->m_bSelected)
		{
			uSelFlags |= Morph3D::scClear;
		}

		SelectHandle(&m_DragHandle, uSelFlags);
	}

	if (m_bMorphing)
	{
		//
		// Check distance moved since left button down and don't start
		// moving unless it's greater than the threshold.
		//
		if (!m_bMovingSelected)
		{
			CSize sizeMoved = point - m_ptLastMouseMovement;
			if ((abs(sizeMoved.cx) > 3) || (abs(sizeMoved.cy) > 3))
			{
				m_bMovingSelected = true;
			}
			else
			{
				return true;
			}
		}

		//
		// Determine the closest world axes to camera up & right.
		//
		Axes2 BestAxes;
		pView->CalcBestAxes(BestAxes);

		//
		// Get position of handle we're dragging
		//
		Vector handlepos;
		GetHandlePos(&m_DragHandle, handlepos);

		//
		// Calculate morph plane.
		//
		PLANE plane;
		memset(&plane, 0, sizeof plane);
		plane.normal[BestAxes.axThird] = 1.0f;
		plane.dist = handlepos[BestAxes.axThird];

		Vector newhandlepos;

		pView->GetHitPos(point, plane, newhandlepos);

		if (m_pDocument->m_bSnapToGrid && !(GetAsyncKeyState(VK_MENU) & 0x8000))
		{
			for (int i = 0; i < 3; i++)
			{
				newhandlepos[i] = (float)m_pDocument->Snap(newhandlepos[i]);
			}
		}

		if (nFlags & MK_CONTROL)
		{
			newhandlepos[BestAxes.axHorz] = handlepos[BestAxes.axHorz];
		}
		if (nFlags & MK_SHIFT)
		{
			newhandlepos[BestAxes.axVert] = handlepos[BestAxes.axVert];
		}

		// don't change this axis:
		newhandlepos[BestAxes.axThird] = handlepos[BestAxes.axThird];
	
		// move stuff.
		UpdateTranslation(newhandlepos);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
	}

	m_ptLastMouseMovement = point;

	return true;
}