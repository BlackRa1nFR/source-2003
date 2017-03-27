//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a tool for laying down path corner entities. The tool
//			automatically sets up the target/targetname keys.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "EditPathNodeDlg.h"
#include "GlobalFunctions.h"
#include "MainFrm.h"
#include "MapDefs.h"
#include "MapDoc.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "Options.h"
#include "ToolPath.h"
#include "Render2D.h"
#include "StatusBarIDs.h"
#include "Worldcraft.h"


const int iHandleRadius = 3;


static const char *g_pszClassName = "Worldcraft_PathToolWnd";


class CPathToolWnd : public CWnd
{
	public:

		bool Create(void);
		void PreMenu(Path3D *pToolPath, CPoint &ptMapScreen);

	protected:

		Path3D *m_pToolPath;
		CMapDoc *m_pDocument;
		CPoint m_ptMapScreen;

		//{{AFX_MSG_MAP(CShellMessageWnd)
		afx_msg BOOL OnPathMenuCommand(UINT nCmd);
		//}}AFX_MSG
	
		DECLARE_MESSAGE_MAP()
};

static CPathToolWnd s_wndPathMessage;


BEGIN_MESSAGE_MAP(CPathToolWnd, CWnd)
	//{{AFX_MSG_MAP(CPathToolWnd)
	ON_COMMAND_EX(ID_PATHEDIT_DELETE, OnPathMenuCommand)
	ON_COMMAND_EX(ID_PATHEDIT_DELETEPATH, OnPathMenuCommand)
	ON_COMMAND_EX(ID_PATHEDIT_ADD, OnPathMenuCommand)
	ON_COMMAND_EX(ID_PATHEDIT_NODEPROPERTIES, OnPathMenuCommand)
	ON_COMMAND_EX(ID_PATHEDIT_PATHPROPERTIES, OnPathMenuCommand)
	ON_COMMAND_EX(ID_PATHEDIT_SELECTPATH, OnPathMenuCommand)
	ON_COMMAND_EX(ID_PATHEDIT_HIDEPATH, OnPathMenuCommand)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Creates the hidden shell message window.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPathToolWnd::Create(void)
{
	WNDCLASS wndcls;
	memset(&wndcls, 0, sizeof(WNDCLASS));
    wndcls.style         = 0;
    wndcls.lpfnWndProc   = AfxWndProc;
    wndcls.hInstance     = AfxGetInstanceHandle();
    wndcls.hIcon         = NULL;
    wndcls.hCursor       = NULL;
    wndcls.hbrBackground = NULL;
    wndcls.lpszMenuName  = NULL;
	wndcls.cbWndExtra    = 0;
    wndcls.lpszClassName = g_pszClassName;

	if (!AfxRegisterClass(&wndcls))
	{
		return(false);
	}

	return(CWnd::CreateEx(0, g_pszClassName, g_pszClassName, 0, CRect(0, 0, 10, 10), NULL, 0) == TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the path tool for subsequent menu messages to use. Called from
//			Path3D::OnContextMenu2D before bringing up the context menu.
//-----------------------------------------------------------------------------
void CPathToolWnd::PreMenu(Path3D *pToolPath, CPoint &ptMapScreen)
{
	ASSERT(pToolPath != NULL);
	m_pToolPath = pToolPath;
	m_ptMapScreen = ptMapScreen;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nCmd - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CPathToolWnd::OnPathMenuCommand(UINT nCmd)
{
	if (nCmd == ID_PATHEDIT_DELETE)
	{
		m_pDocument->OnCmdMsg(ID_EDIT_DELETE, CN_COMMAND, NULL, NULL);
	}
	else if (nCmd == ID_PATHEDIT_ADD)
	{
		m_pToolPath->AddNodeAt(m_ptMapScreen);
	}
	else if(nCmd == ID_PATHEDIT_NODEPROPERTIES)
	{
		m_pToolPath->EditNodeProperties(m_ptMapScreen);
	}
	else if (nCmd == ID_PATHEDIT_PATHPROPERTIES)
	{
		m_pToolPath->EditPathProperties(m_ptMapScreen);
	}
	else if (nCmd == ID_PATHEDIT_DELETEPATH)
	{
		if (m_pToolPath->DeletePath(m_ptMapScreen))
		{
			m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		}
	}
	else if (nCmd == ID_PATHEDIT_SELECTPATH)
	{
		if (m_pToolPath->SelectPath(m_ptMapScreen))
		{
			m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		}
	}
	else if (nCmd == ID_PATHEDIT_HIDEPATH)
	{
		if (m_pToolPath->HidePath(m_ptMapScreen))
		{
			m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Path3D::Path3D(void)
{
	// Create the message window once only.
	static bool bFirst = true;
	if (bFirst)
	{
		bFirst = false;
		s_wndPathMessage.Create();
	}

	m_nSelectedHandles = 0;
	m_bBoxSelecting = FALSE;
	m_bNew = FALSE;
	m_pPaths = NULL;
	m_pNewPath = NULL;
	m_bLButtonDown = false;
	m_ptLDownClient.x = m_ptLDownClient.y = 0;

	SetDrawColors(Options.colors.clrToolHandle, Options.colors.clrToolPath);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Path3D::~Path3D(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Called when the tool is deactivated.
// Input  : eNewTool - The ID of the tool that is being activated.
//-----------------------------------------------------------------------------
void Path3D::OnDeactivate(ToolID_t eNewTool)
{
	if (!IsEmpty())
	{
		m_pDocument->ToolUpdateViews(CMapView2D::updRenderAll);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pList - 
//-----------------------------------------------------------------------------
void Path3D::SetPathList(CMapPathList *pList)
{
	m_pPaths = pList;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			BOOL - 
// Output : int
//-----------------------------------------------------------------------------
int Path3D::HitTest(CPoint pt, BOOL)
{
	if(m_bBoxSelecting)
		return Box3D::HitTest(pt, FALSE);

	MAPPATHHANDLE tmp;
	return PathHitTest2D(pt, &tmp);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			*pHandle - 
// Output : int
//-----------------------------------------------------------------------------
int Path3D::PathHitTest2D(CPoint pt, MAPPATHHANDLE *pHandle)
{
	POSITION p = m_pPaths->GetHeadPosition();
	while(p)
	{
		CMapPath *pPath = m_pPaths->GetNext(p);
		// test this path's nodes
		for(int i = 0; i < pPath->m_nNodes; i++)
		{
			CPoint ptNode;
			PointToScreen(pPath->m_Nodes[i].pos, ptNode);
			CRect r(ptNode.x - iHandleRadius, ptNode.y - iHandleRadius,
				ptNode.x + iHandleRadius, ptNode.y + iHandleRadius);
			if(r.PtInRect(pt))
			{
				pHandle->pPath = pPath;
				pHandle->bSelected = pPath->m_Nodes[i].bSelected;
				pHandle->dwNodeID = pPath->m_Nodes[i].dwID;

				return TRUE;
			}
		}
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt3 - 
//-----------------------------------------------------------------------------
void Path3D::StartNew(const Vector &vecStart)
{
	// create new path and start it off here; then start
	//  translation.
	CMapPath *pPath = new CMapPath;
	m_pPaths->AddTail(pPath);
	m_pNewPath = pPath;
	
	// add two nodes, then start translating the second one
	pPath->AddNode(CMapPath::ADD_START, vecStart);

	MAPPATHHANDLE handle;
	handle.pPath = pPath;
	handle.dwNodeID = pPath->AddNode(CMapPath::ADD_END, vecStart);

	SelectHandle(&handle, scSelect | scClear);

	// do start translation:
	m_hndTranslate = handle;
	CPoint pt(vecStart[axHorz], vecStart[axVert]);
	Tool3D::StartTranslation(pt);

	// this is a new path (we want to edit it at the end):
	m_bNew = TRUE;
	m_bCreatedNewNode = FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Path3D::StartTranslation(CPoint &pt)
{
	if(IsBoxSelecting())
	{
		return Box3D::StartTranslation(pt);
	}

	// nope!
	MAPPATHHANDLE hnd;
	if (PathHitTest2D(pt, &hnd) == -1)
		return FALSE;
	m_hndTranslate = hnd;

	SelectHandle(&m_hndTranslate, scSelect);

	// save original handle position
	CMapPathNode *pNode = hnd.pPath->NodeForID(hnd.dwNodeID);
	m_ptOrigHandlePos.x = pNode->pos[axHorz];
	m_ptOrigHandlePos.y = pNode->pos[axVert];

	Tool3D::StartTranslation(pt);
	m_bCreatedNewNode = FALSE;

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			nFlags - 
//			CSize& - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Path3D::UpdateTranslation(CPoint pt, UINT nFlags, CSize&)
{
	if(m_bBoxSelecting)
	{
		return Box3D::UpdateTranslation(pt, nFlags);
	}

	BOOL bSnap = m_pDocument->m_bSnapToGrid && 
		!(GetAsyncKeyState(VK_MENU) & 0x8000);

	if(bSnap)
	{
		pt.x = m_pDocument->Snap(pt.x);
		pt.y = m_pDocument->Snap(pt.y);
	}

	if((nFlags & flagCreateNewNode) && !m_bCreatedNewNode && 
		m_nSelectedHandles == 1 && !m_bNew)
	{
		// create a new node, mark it, and move it.
		int iIndex;
		MAPPATHHANDLE hnd = m_SelectedHandles[0];
		CMapPathNode *pOldNode = hnd.pPath->NodeForID(hnd.dwNodeID, &iIndex);

		DWORD dwAddPos;
		if(iIndex == hnd.pPath->m_nNodes-1)
			dwAddPos = CMapPath::ADD_END;
		else
			dwAddPos = hnd.pPath->m_Nodes[iIndex+1].dwID;

		DWORD dwNewNode = hnd.pPath->AddNode(dwAddPos, pOldNode->pos);
		CMapPathNode *pNewNode = hnd.pPath->NodeForID(dwNewNode);

		SelectHandle(&hnd, scUnselect);

		// create new handle
		hnd.dwNodeID = pNewNode->dwID;

		m_hndTranslate = hnd;
		SelectHandle(&hnd, scSelect);

		m_bCreatedNewNode = TRUE;
	}

	// move handles to new location - get a delta from the point
	//  to the current location of the translate handle, then
	//  add that delta to all selected handles.

	CMapPathNode *pNode = m_hndTranslate.pPath->NodeForID(
		m_hndTranslate.dwNodeID);

	if(pNode->pos[axHorz] == pt.x && pNode->pos[axVert] == pt.y)
		return FALSE;	// no need to update

	Vector delta;
	delta[axHorz] = pt.x - pNode->pos[axHorz];
	delta[axVert] = pt.y - pNode->pos[axVert];
	delta[axThird] = 0.f;

	// move all selected handles
	for(int i = 0; i < m_nSelectedHandles; i++)
	{
		MAPPATHHANDLE& hnd = m_SelectedHandles[i];
		CMapPathNode *pNode = hnd.pPath->NodeForID(hnd.dwNodeID);

		Vector newpos;
		for(int d = 0; d < 3; d++)
		{
			newpos[d] = pNode->pos[d] + delta[d];
		}

		hnd.pPath->SetNodePosition(hnd.dwNodeID, newpos);
	}

	// done!

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bSave - 
//-----------------------------------------------------------------------------
void Path3D::FinishTranslation(BOOL bSave)
{
	if (m_bBoxSelecting)
	{
		Box3D::FinishTranslation(bSave);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		return;
	}

	Tool3D::FinishTranslation(bSave);

	if(!bSave)
	{
		// move back to original positions
		UpdateTranslation(m_ptOrigHandlePos, 0);
	}
	else if(m_bNew)
	{
		// new path!!! (edit properties)
		m_pNewPath->EditInfo();		
	}

	m_bNew = FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pHandle - 
//			cmd - 
//-----------------------------------------------------------------------------
void Path3D::SelectHandle(MAPPATHHANDLE *pHandle, int cmd)
{
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
		return;	// nothing else to do here
	
	// find that info!!!!!!
	CMapPathNode *pNode;
	pNode = pHandle->pPath->NodeForID(pHandle->dwNodeID);

	BOOL bChanged = FALSE;

	// toggle selection:
	if(cmd & scToggle)
	{
		pNode->bSelected = !pNode->bSelected;
		bChanged = TRUE;
	}
	else if(cmd & scSelect && !pNode->bSelected)
	{
		pNode->bSelected = TRUE;
		bChanged = TRUE;
	}
	else if(cmd & scUnselect && pNode->bSelected)
	{
		pNode->bSelected = FALSE;
		bChanged = TRUE;
	}

	if(!bChanged)
		return;

	if(pNode->bSelected)
	{
		m_SelectedHandles.SetAtGrow(m_nSelectedHandles, *pHandle);
		++m_nSelectedHandles;
	}
	else
	{
		for(int i = 0; i < m_nSelectedHandles; i++)
		{
			if(!memcmp(&m_SelectedHandles[i], pHandle, sizeof *pHandle))
			{
				// delete this handle from list
				m_SelectedHandles.RemoveAt(i);
				--m_nSelectedHandles;
				break;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt3 - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Path3D::StartBoxSelection(Vector& pt3)
{
	m_bBoxSelecting = TRUE;
	SetDrawColors(0xffffffff, RGB(50, 255, 255));

	CPoint pt(pt3[axHorz], pt3[axVert]);
	m_pDocument->Snap(pt);
	Tool3D::StartTranslation(pt);
	
	bPreventOverlap = FALSE;
	stat.TranslateMode = modeScale;
	stat.bNewBox = TRUE;
	tbmins[axThird] = m_pDocument->Snap(pt3[axThird]);
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
void Path3D::SelectInBox(void)
{
	EndBoxSelection();	// may as well do it here

	// expand box along 0-depth axes
	for(int i = 0; i < 3; i++)
	{
		if(bmaxs[i] - bmins[i] == 0)
		{
			bmaxs[i] = COORD_NOTINIT;
			bmins[i] = -COORD_NOTINIT;
		}
	}

	POSITION p = m_pPaths->GetHeadPosition();
	while(p)
	{	
		CMapPath *pPath = m_pPaths->GetNext(p);

		for(i = 0; i < pPath->m_nNodes; i++)
		{
			CMapPathNode& node = pPath->m_Nodes[i];

			for(int i2 = 0; i2 < 3; i2++)
			{
				if(node.pos[i2] < bmins[i2] || node.pos[i2] > bmaxs[i2])
					break;
			}
			
			if(i2 == 3)
			{
				// completed loop - intersects - select handle
				MAPPATHHANDLE hnd;
				hnd.dwNodeID = node.dwID;
				hnd.pPath = pPath;
				SelectHandle(&hnd, scSelect);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Path3D::EndBoxSelection(void)
{
	m_bBoxSelecting = FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pDC - 
//			bounds - 
//-----------------------------------------------------------------------------
void Path3D::RenderTool2D(CRender2D *pRender)
{
	BOOL bDrawNodes = FALSE;

	if (m_pPaths != NULL)
	{
		DrawAgain:

		POSITION p = m_pPaths->GetHeadPosition();
		while (p)
		{
			CMapPath *pPath = m_pPaths->GetNext(p);
			for (int iNode = 0; iNode < pPath->m_nNodes; iNode++)
			{
				CMapPathNode &node = pPath->m_Nodes[iNode];

				//
				// Draw node.
				//
				CPoint pt;
				PointToScreen(node.pos, pt);

				if (bDrawNodes)
				{
					if (node.bSelected)
					{
						pRender->SetFillColor(255, 0, 0);
					}
					else
					{
						pRender->SetFillColor(200, 250, 250);
					}

					pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 255, 255, 255);
					pRender->Rectangle(pt.x - iHandleRadius, pt.y - iHandleRadius, pt.x + iHandleRadius, pt.y + iHandleRadius, true);
				}

				//
				// Draw line to next node.
				//
				int iNextNode = iNode + 1;
				if (iNextNode == pPath->m_nNodes)
				{
					if (pPath->m_iDirection == CMapPath::dirCircular)
					{
						iNextNode = 0;
					}
					else
					{
						iNextNode = -1;
					}
				}

				if (iNextNode != -1 && !bDrawNodes)
				{
					pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 200, 250, 250);
					CMapPathNode &node2 = pPath->m_Nodes[iNextNode];
					
					CPoint pt2;
					PointToScreen(node2.pos, pt2);
					pRender->MoveTo(pt);
					pRender->LineTo(pt2);
				}

				//
				// Draw a circle round the first node.
				//
				if (!iNode && bDrawNodes)
				{
					pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 0, 255, 0);
					pRender->DrawEllipse(pt, iHandleRadius + 2, iHandleRadius + 2, false);
				}
			}
		}

		if (!bDrawNodes)
		{
			// Draw nodes after the lines.
			bDrawNodes = TRUE;
			goto DrawAgain;
		}
	}

	if (m_bBoxSelecting)
	{
		Box3D::RenderTool2D(pRender);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Path3D::IsEmpty()
{
	return !m_pPaths->GetCount();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ptMapScreen - 
//-----------------------------------------------------------------------------
void Path3D::AddNodeAt(CPoint ptMapScreen)
{
	// add node at this position
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ptMapScreen - 
//-----------------------------------------------------------------------------
void Path3D::EditPathProperties(CPoint ptMapScreen)
{
	MAPPATHHANDLE hnd;
	if (PathHitTest2D(ptMapScreen, &hnd) == -1)
	{
		return;
	}

	hnd.pPath->EditInfo();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ptMapScreen - 
//-----------------------------------------------------------------------------
void Path3D::EditNodeProperties(CPoint ptMapScreen)
{
	// edit node properties! select a node (if not already selected)
	//  and do the objectproperties thing. if no hits, return.
	
	MAPPATHHANDLE hnd;
	if (PathHitTest2D(ptMapScreen, &hnd) == -1)
		return;
	CMapPathNode *pNode = hnd.pPath->NodeForID(hnd.dwNodeID);

	SelectHandle(&hnd, scClear | scSelect);

	CEditPathNodeDlg dlg;

	LPCTSTR pszStr = pNode->kv.GetValue("spawnflags");
	if(pszStr)	dlg.m_bRetrigger = atoi(pszStr);
	pszStr = pNode->kv.GetValue("speed");
	if(pszStr)	dlg.m_iSpeed = atoi(pszStr);
	pszStr = pNode->kv.GetValue("yawspeed");
	if(pszStr)	dlg.m_iYawSpeed = atoi(pszStr);
	pszStr = pNode->kv.GetValue("wait");
	if(pszStr)	dlg.m_iWait = atoi(pszStr);

	dlg.m_strName = pNode->szName;

	if(dlg.DoModal() != IDOK)
		return;

	pNode->kv.SetValue("wait", dlg.m_iWait);
	pNode->kv.SetValue("spawnflags", dlg.m_bRetrigger);
	pNode->kv.SetValue("yawspeed", dlg.m_iYawSpeed);
	pNode->kv.SetValue("speed", dlg.m_iSpeed);
	strcpy(pNode->szName, dlg.m_strName);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Path3D::DeleteSelected()
{
	// delete selected handles
	for(int i = 0; i < m_nSelectedHandles; i++)
	{
		MAPPATHHANDLE& hnd = m_SelectedHandles[i];
		// kill this node
		hnd.pPath->DeleteNode(hnd.dwNodeID);
	}

	m_nSelectedHandles = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ptMapScreen - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Path3D::DeletePath(CPoint ptMapScreen)
{
	// delete entire path
	MAPPATHHANDLE hnd;
	if (PathHitTest2D(ptMapScreen, &hnd) == -1)
		return FALSE;

	SelectHandle(NULL, scClear);

	// confirm
	CString str;
	str.Format("Delete path '%s'?", hnd.pPath->GetName());
	if(AfxMessageBox(str, MB_YESNO) == IDNO)
		return FALSE;	// no

	POSITION p = m_pPaths->Find(hnd.pPath);
	ASSERT(p);

	m_pPaths->RemoveAt(p);
	delete hnd.pPath;

	return TRUE;	// killedit
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//-----------------------------------------------------------------------------
void Path3D::GetSelectedCenter(Vector& pt)
{
	BoundBox box;

	for(int i = 0; i < m_nSelectedHandles; i++)
	{
		MAPPATHHANDLE& hnd = m_SelectedHandles[i];
		CMapPathNode *pNode = hnd.pPath->NodeForID(hnd.dwNodeID);
		box.UpdateBounds(pNode->pos);
	}

	box.GetBoundsCenter(pt);
}	


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ptMapScreen - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Path3D::SelectPath(CPoint ptMapScreen)
{	
	MAPPATHHANDLE hndPath;
	if (PathHitTest2D(ptMapScreen, &hndPath) == -1)
		return FALSE;

	for(int i = 0; i < hndPath.pPath->m_nNodes; i++)
	{
		MAPPATHHANDLE hnd;
		hnd.dwNodeID = hndPath.pPath->m_Nodes[i].dwID;
		hnd.pPath = hndPath.pPath;
		SelectHandle(&hnd, scSelect);
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ptMapScreen - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Path3D::HidePath(CPoint ptMapScreen)
{
	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
/*
ChunkFileResult_t Path3D::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	ChunkFileResult_t eResult = pFile->BeginChunk("paths");
	if (eResult == ChunkFile_Ok)
	{
		POSITION pos = m_Paths.GetHeadPosition();
		while ((pos != NULL) && (eResult == ChunkFile_Ok))
		{
			CMapPath *pPath = m_Paths.GetNext(pos);
			eResult = pPath->SaveVMF(pFile, pSaveInfo);
		}
	}

	return(eResult);
}
*/


//-----------------------------------------------------------------------------
// Purpose: Handles key down events in the 2D view.
// Input  : Per CWnd::OnKeyDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Path3D::OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
		case VK_RETURN:
		{
			if (IsBoxSelecting())
			{
				SelectInBox();
				m_pDocument->ToolUpdateViews(CMapView2D::updTool);
			}
			break;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Handles context menu events in the 2D view.
// Input  : Per CWnd::OnContextMenu.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Path3D::OnContextMenu2D(CMapView2D *pView, CPoint point)
{
	static CMenu menu, menuPath;
	static bool bInit = false;

	if (!bInit)
	{
		bInit = true;
		menu.LoadMenu(IDR_POPUPS);
		menuPath.Attach(::GetSubMenu(menu.m_hMenu, 3));
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

	if (!IsEmpty() && GetSelectedCount())
	{
		if (HitTest(ptMapScreen, FALSE) != -1)
		{
			s_wndPathMessage.PreMenu(this, ptMapScreen);
			menuPath.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN, ptScreen.x, ptScreen.y, &s_wndPathMessage);
		}
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button down events in the 2D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Path3D::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point) 
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

	MAPPATHHANDLE hnd;
	if (IsBoxSelecting() && !(nFlags & MK_SHIFT))
	{
		if (HitTest(ptScreen, FALSE) != -1)
		{
			StartTranslation(ptScreen);
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
		}
	}
	else if (PathHitTest2D(ptScreen, &hnd) != -1)
	{
		if (!(nFlags & MK_CONTROL) && !hnd.bSelected)
		{
			SelectHandle(&hnd, Path3D::scClear | Path3D::scSelect);
		}
		StartTranslation(ptScreen);
	}
	else if (nFlags & MK_SHIFT)
	{
		EndBoxSelection();

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
		
		StartNew(ptOrg);
		pView->SetUpdateFlag(CMapView2D::updTool);
	}
	else if (!(nFlags & MK_CONTROL))
	{
		SelectHandle(NULL, Path3D::scClear);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles left mouse button up events in the 2D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Path3D::OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point) 
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
bool Path3D::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point) 
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
	
	//
	// Start selection box.
	//
	if (!m_bLButtonDown && (nFlags & MK_LBUTTON))
	{
		Vector vecStart(COORD_NOTINIT, COORD_NOTINIT, COORD_NOTINIT);

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

	if (m_bLButtonDown)
	{
		// cursor is cross here
		bCursorSet = true;

		UINT uConstraints = 0;

		if (bDisableSnap)
		{
			uConstraints |= Tool3D::constrainNosnap;
		}

		if (nFlags & MK_CONTROL)
		{
			uConstraints |= Path3D::flagCreateNewNode;
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
		if (HitTest(ptScreen, TRUE) != -1)
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
			bCursorSet = true;
		}
	}

	if (!bCursorSet)
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	}

	return true;
}
