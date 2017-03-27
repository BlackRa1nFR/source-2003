//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a list view for visgroups. Supports drag and drop, and
//			posts a registered windows message to the list view's parent window
//			when visgroups are hidden or shown.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "worldcraft.h"
#include "GroupList.h"
#include "MapDoc.h"
#include "MapSolid.h"
#include "MapWorld.h"
#include "GlobalFunctions.h"
#include "VisGroup.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static const unsigned int g_uToggleStateMsg = ::RegisterWindowMessage(GROUPLIST_TOGGLESTATE_MSG);


BEGIN_MESSAGE_MAP(CGroupList, CListCtrl)
	//{{AFX_MSG_MAP(CGroupList)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBegindrag)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndlabeledit)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGroupList::CGroupList(void)
{
	m_pDragImageList = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGroupList::~CGroupList(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFlags - 
//			point - 
//-----------------------------------------------------------------------------
void CGroupList::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int nItem = HitTest(point);
	if (nItem != -1)
	{
		if (point.x < 12)
		{
			//
			// Change item's image state.
			//
			LV_ITEM item;
			memset(&item, 0, sizeof(item));
			item.mask = LVIF_IMAGE;
			item.iItem = nItem;
			GetItem(&item);
			item.iImage = !item.iImage;
			SetItem(&item);

			//
			// Notify our parent window that this item's state has changed.
			//
			CWnd *pwndParent = GetParent();
			if (pwndParent != NULL)
			{
				pwndParent->PostMessage(g_uToggleStateMsg, nItem, item.iImage);
			}
		}
	}

	CListCtrl::OnLButtonDown(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pNMHDR - 
//			pResult - 
//-----------------------------------------------------------------------------
void CGroupList::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO *pInfo = (LV_DISPINFO*) pNMHDR;

	if(!pInfo->item.pszText)
		return;

	int id = GetItemData(pInfo->item.iItem);
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc != NULL)
	{
		CVisGroup *pGroup = pDoc->VisGroups_GroupForID(id);
		pGroup->SetName(pInfo->item.pszText);
	}

	pResult[0] = TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Begins dragging an item in the visgroup list. The drag image is
//			created and anchored relative to the mouse cursor.
// Input  : pNMHDR - 
//			pResult - 
//-----------------------------------------------------------------------------
void CGroupList::OnBegindrag(NMHDR *pNMHDR, LRESULT *pResult) 
{
	NM_LISTVIEW *pLV = (NM_LISTVIEW*) pNMHDR;
	CPoint ptCursor(pLV->ptAction);
	CPoint pt;

	m_iDragItem = pLV->iItem;

	m_pDragImageList = CreateDragImage(m_iDragItem, &pt);

	CPoint ptHotSpot;
	ptHotSpot.x = ptCursor.x - pt.x;
	ptHotSpot.y = ptCursor.y - pt.y;

	m_pDragImageList->BeginDrag(0, ptHotSpot);
	m_pDragImageList->DragEnter(this, ptCursor);
	m_iLastDropItem = -1;
	
	SetCapture();

	*pResult = 0;
}


typedef struct
{
	CVisGroup *pGroup1;
	CVisGroup *pGroup2;
} EXCHANGEGROUPINFO;


static BOOL ExchangeGroup(CMapSolid *pSolid, EXCHANGEGROUPINFO *pInfo)
{
	if (pSolid->GetVisGroup() == pInfo->pGroup1)
	{
		pSolid->SetVisGroup(pInfo->pGroup2);
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFlags - 
//			point - 
//-----------------------------------------------------------------------------
void CGroupList::OnLButtonUp(UINT nFlags, CPoint point) 
{
	ReleaseCapture();

	CListCtrl::OnLButtonUp(nFlags, point);

	// are we dragging? drop!
	if(!m_pDragImageList)
		return;

	m_pDragImageList->DragLeave(this);
	m_pDragImageList->EndDrag();
	delete m_pDragImageList;
	m_pDragImageList = NULL;

	LV_ITEM item;
	memset(&item, 0, sizeof item);
	item.mask = LVIF_STATE;
	item.stateMask = LVIS_DROPHILITED;
	item.iItem = m_iLastDropItem;
	item.state = 0;
	SetItem(&item);

	if(m_iLastDropItem == m_iDragItem)
		return;	// no combine same group

	if(m_iLastDropItem != -1)
	{
		if(AfxMessageBox("Combine groups?", MB_YESNO | MB_ICONQUESTION) == 
			IDNO)
			return;	// no!!!!
	}
	else
	{
		// delete group?
		if(AfxMessageBox("Delete group?", MB_YESNO | MB_ICONQUESTION) == 
			IDNO)
			return;	// no!!!!
	}

	// combine group 1 with group 2 (or delete group 1 only)
	EXCHANGEGROUPINFO info;
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	info.pGroup1 = pDoc->VisGroups_GroupForID(GetItemData(m_iDragItem));
	if (m_iLastDropItem == -1)
	{
		info.pGroup2 = NULL;
	}
	else
	{
		info.pGroup2 = pDoc->VisGroups_GroupForID(GetItemData(m_iLastDropItem));
	}

	CMapWorld *pWorld = pDoc->GetMapWorld();	
	pWorld->EnumChildren((ENUMMAPCHILDRENPROC)ExchangeGroup, (DWORD)&info);

	bool bNeedUpdate = false;
	if (info.pGroup1 != NULL)
	{
		// if one is visible and the other is not, need to update
		if (info.pGroup2 != NULL)
		{
			bNeedUpdate = info.pGroup1->IsVisible() != info.pGroup2->IsVisible();
		}
		else
		{
			bNeedUpdate = !info.pGroup1->IsVisible();
		}

		pDoc->VisGroups_RemoveID(info.pGroup1->GetID());
	}

	DeleteItem(m_iDragItem);
	delete info.pGroup1;

	// update display
	if (bNeedUpdate)
	{
		pDoc->UpdateVisibilityAll();
		pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFlags - 
//			point - 
//-----------------------------------------------------------------------------
void CGroupList::OnMouseMove(UINT nFlags, CPoint point) 
{
	CListCtrl::OnMouseMove(nFlags, point);

	if(!m_pDragImageList)
		return;

	m_pDragImageList->DragMove(point);

	// highlight object we hit
	int iItem = HitTest(point);

	if(iItem == m_iLastDropItem)
		return;

	// hide image first
	m_pDragImageList->DragLeave(this);
	m_pDragImageList->DragShowNolock(FALSE);

	LV_ITEM item;
	memset(&item, 0, sizeof item);
	item.mask = LVIF_STATE;
	item.stateMask = LVIS_DROPHILITED;

	// first unhighlight old item
	if(m_iLastDropItem != -1)
	{
		item.iItem = m_iLastDropItem;
		item.state = 0;
		SetItem(&item);
	}

	if(iItem != -1)
	{
		// now highlight new item
		item.iItem = iItem;
		item.state = LVIS_DROPHILITED;
		SetItem(&item);
	}

	RedrawWindow();

	m_iLastDropItem = iItem;

	// now show
	m_pDragImageList->DragEnter(this, point);
}
