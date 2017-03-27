//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the cordon tool. The cordon tool defines a rectangular
//			volume that acts as a visibility filter. Only objects that intersect
//			the cordon are rendered in the views. When saving the MAP file while
//			the cordon tool is active, only brushes that intersect the cordon
//			bounds are saved. The cordon box is replaced by brushes in order to
//			seal the map.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "ChunkFile.h"
#include "ToolCordon.h"
#include "StockSolids.h"
#include "History.h"
#include "GlobalFunctions.h"
#include "MainFrm.h"
#include "MapDoc.h"
#include "MapDefs.h"
#include "MapSolid.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "MapWorld.h"
#include "StatusBarIDs.h"
#include "ToolManager.h"
#include "Options.h"
#include "WorldSize.h"


//
// Cordon brush thickness. Must be thick enough to enclose any entities that are
// on the edges of the cordon bounds to prevent leaks.
//
#define CORDON_THICKNESS	600


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
Cordon3D::Cordon3D(void)
{
	m_bCordonActive = false;
	m_bEnableEdit = false;
	m_bLButtonDown = false;

	SetDrawColors(RGB(0, 255, 255), RGB(255, 0, 0));
}


//-----------------------------------------------------------------------------
// Purpose: Called when the tool is activated.
// Input  : eOldTool - The ID of the previously active tool.
//-----------------------------------------------------------------------------
void Cordon3D::OnActivate(ToolID_t eOldTool)
{
	if (!IsActiveTool())
	{
		EnableEdit(true);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when the tool is deactivated.
// Input  : eNewTool - The ID of the tool that is being activated.
//-----------------------------------------------------------------------------
void Cordon3D::OnDeactivate(ToolID_t eNewTool)
{
	EnableEdit(false);
}


//-----------------------------------------------------------------------------
// Purpose: CreateTempWorld creates a world with the brushes that make up the
//			cordoned area. it does this by creating a solid of the size of the
//			box, and another solid 1200 units bigger. subtract A from B, and
//			keep those brushes.
// Output : Returns a pointer to the newly-created world.
//-----------------------------------------------------------------------------
CMapWorld *Cordon3D::CreateTempWorld(void)
{
	CMapWorld *pWorld = new CMapWorld;

	GetHistory()->Pause();

	// create solids
	CMapSolid *pBigSolid = new CMapSolid;
	CMapSolid *pSmallSolid = new CMapSolid;

	StockBlock box;
	box.SetFromBox(this);
	box.CreateMapSolid(pSmallSolid);

	// make bigger box
	BoundBox bigbounds;
	bigbounds = *this;
	for (int i = 0; i < 3; i++)
	{
		bigbounds.bmins[i] -= CORDON_THICKNESS;
		if (bigbounds.bmins[i] < g_MIN_MAP_COORD)
		{
			bigbounds.bmins[i] = g_MIN_MAP_COORD;
		}

		bigbounds.bmaxs[i] += CORDON_THICKNESS;
		if (bigbounds.bmaxs[i] > g_MAX_MAP_COORD)
		{
			bigbounds.bmaxs[i] = g_MAX_MAP_COORD;
		}
	}
	box.SetFromBox(&bigbounds);
	box.CreateMapSolid(pBigSolid);

	//
	// Subtract the small box from the large box and add the outside pieces to the
	// cordon world. This gives a hollow box.
	//
	CMapObjectList Outside;
	pBigSolid->Subtract(NULL, &Outside, pSmallSolid);

	POSITION p = Outside.GetHeadPosition();
	while (p != NULL)
	{
		CMapClass *pObject = Outside.GetNext(p);
		pWorld->AddObjectToWorld(pObject);
	}

	delete pBigSolid;
	delete pSmallSolid;

	//
	// Set all the brush textures to the texture specified in the options.
	//
	const char *pszTexture = g_pGameConfig->GetCordonTexture();
	EnumChildrenPos_t pos;
	CMapClass *pChild = pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CMapSolid *pSolid = dynamic_cast<CMapSolid *>(pChild);
		if (pSolid != NULL)
		{
			pSolid->SetCordonBrush(true);
			pSolid->SetTexture(pszTexture);
		}

		pChild = pWorld->GetNextDescendent(pos);
	}

	GetHistory()->Resume();

	return(pWorld);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			pszValue - 
//			pCordon - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t Cordon3D::LoadCordonKeyCallback(const char *szKey, const char *szValue, Cordon3D *pCordon)
{
	if (!stricmp(szKey, "mins"))
	{
		CChunkFile::ReadKeyValuePoint(szValue, pCordon->bmins);
	}
	else if (!stricmp(szKey, "maxs"))
	{
		CChunkFile::ReadKeyValuePoint(szValue, pCordon->bmaxs);
	}
	else if (!stricmp(szKey, "active"))
	{
		bool bActive = false;
		CChunkFile::ReadKeyValueBool(szValue, bActive);
		pCordon->SetCordonActive(bActive);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: Loads the cordon state from the given VMF.
// Input  : pFile - 
//-----------------------------------------------------------------------------
ChunkFileResult_t Cordon3D::LoadVMF(CChunkFile *pFile)
{
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadCordonKeyCallback, this);
	if (IsValidBox())
	{
		bEmpty = FALSE;
	}
	return eResult;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t Cordon3D::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	ChunkFileResult_t eResult = pFile->BeginChunk("cordon");

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValuePoint("mins", bmins);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValuePoint("maxs", bmaxs);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueBool("active", IsCordonActive());
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Sets render variables based on the current state of the tool.
//-----------------------------------------------------------------------------
void Cordon3D::UpdateRenderState(void)
{
	if (m_bCordonActive)
	{
		SetDrawFlags(GetDrawFlags() | Box3D::thicklines);
	}
	else
	{
		SetDrawFlags(GetDrawFlags() & ~Box3D::thicklines);
	}

	EnableHandles(m_bEnableEdit);
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button down events in the 2D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Cordon3D::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point)
{
	CMapWorld *pWorld = m_pDocument->GetMapWorld();

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
	pView->ClientToWorld(point);

	Vector ptOrg( COORD_NOTINIT, COORD_NOTINIT, COORD_NOTINIT );
	ptOrg[axHorz] = point.x;
	ptOrg[axVert] = point.y;

	// getvisiblepoint fills in any coord that's still set to COORD_NOTINIT:
	m_pDocument->GetBestVisiblePoint(ptOrg);

	// snap starting position to grid
	if (!(GetAsyncKeyState(VK_MENU) & 0x8000))
	{
		m_pDocument->Snap(ptOrg);
	}
	
	bool bStarting = false;

	if (!IsEmpty())
	{
		if (!StartTranslation(ptScreen))
		{
			if (nFlags & MK_SHIFT)
			{
				SetEmpty();
				bStarting = true;
			}
		}
	}
	else
	{
		bStarting = true;
	}

	if (bStarting)
	{
		StartNew(ptOrg);
		pView->SetUpdateFlag(CMapView2D::updTool);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button up events in the 2D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Cordon3D::OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point) 
{
	ReleaseCapture();

	if (IsTranslating())
	{
		m_bLButtonDown = false;
		FinishTranslation(TRUE);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		m_pDocument->UpdateVisibilityAll();
	}

	m_pDocument->UpdateStatusbar();
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles mouse move events in the 2D view.
// Input  : Per CWnd::OnMouseMove.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Cordon3D::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point) 
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

		if (bDisableSnap || !m_pDocument->IsSnapEnabled())
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
		if (HitTest(ptScreen, TRUE) != inclNone)
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
// Purpose: Handles right mouse button up events in the 2D view.
// Input  : Per CWnd::OnRButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Cordon3D::OnRMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point) 
{
	ReleaseCapture();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the escape key in the 2D or 3D views.
//-----------------------------------------------------------------------------
void Cordon3D::OnEscape(void)
{
	if (IsTranslating())
	{
		FinishTranslation(FALSE);
		m_pDocument->ToolUpdateViews(CMapView2D::updEraseTool);
	}
	else
	{
		g_pToolManager->SetTool(TOOL_POINTER);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output :
//-----------------------------------------------------------------------------
bool Cordon3D::OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_ESCAPE)
	{
		OnEscape();
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output :
//-----------------------------------------------------------------------------
bool Cordon3D::OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_ESCAPE)
	{
		OnEscape();
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Renders the cordon tool in the 2D view.
//-----------------------------------------------------------------------------
void Cordon3D::RenderTool2D(CRender2D *pRender)
{
	if (IsActiveTool() || m_bCordonActive)
	{
		BaseClass::RenderTool2D(pRender);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders the cordon tool in the 3D view.
//-----------------------------------------------------------------------------
void Cordon3D::RenderTool3D(CRender3D *pRender)
{
	if (IsActiveTool() || m_bCordonActive)
	{
		BaseClass::RenderTool3D(pRender);
	}
}

