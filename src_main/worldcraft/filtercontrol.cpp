//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "FilterControl.h"
#include "ControlBarIDs.h"
#include "MapWorld.h"
#include "GlobalFunctions.h"
#include "EditGroups.h"
#include "CustomMessages.h"
#include "VisGroup.h"


typedef struct
{
	CVisGroup *pGroup;
	CMapDoc *pDoc;
	SelectMode_t eSelectMode;
} MARKMEMBERSINFO;


static const unsigned int g_uToggleStateMsg = ::RegisterWindowMessage(GROUPLIST_TOGGLESTATE_MSG);


BEGIN_MESSAGE_MAP(CFilterControl, CWorldcraftBar)
	//{{AFX_MSG_MAP(CFilterControl)
	ON_BN_CLICKED(IDC_EDITGROUPS, OnEditGroups)
	ON_BN_CLICKED(IDC_MARKMEMBERS, OnMarkMembers)
	ON_BN_CLICKED(IDC_SHOW_ALL, OnShowAllGroups)
	ON_UPDATE_COMMAND_UI(IDC_GROUPS, UpdateControlGroups)
	ON_UPDATE_COMMAND_UI(IDC_EDITGROUPS, UpdateControl)
	ON_UPDATE_COMMAND_UI(IDC_MARKMEMBERS, UpdateControl)
	ON_UPDATE_COMMAND_UI(IDC_SHOW_ALL, UpdateControl)
	ON_WM_ACTIVATE()
	ON_WM_SHOWWINDOW()
	ON_WM_WINDOWPOSCHANGED()
	ON_REGISTERED_MESSAGE(g_uToggleStateMsg, OnListToggleState)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParentWnd - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CFilterControl::Create(CWnd *pParentWnd)
{
	if(!CWorldcraftBar::Create(pParentWnd, IDD_FILTERCONTROL, CBRS_RIGHT, 
		IDCB_FILTERCONTROL))
		return FALSE;

	SetWindowText("Filter Control");

	// set up controls
	m_cGroupBox.SubclassDlgItem(IDC_GROUPS, this);
	m_cGroupImageList.Create(IDB_VISGROUPSTATUS, 10, 1, RGB(255, 255, 255));
	m_cGroupBox.SetImageList(&m_cGroupImageList, LVSIL_SMALL);

	CRect r;
	m_cGroupBox.GetClientRect(&r);

	// set up columns in groupbox
	m_cGroupBox.InsertColumn(0, "pic", LVCFMT_LEFT, r.Width() - 20);
//	m_cGroupBox.InsertColumn(1, "name", LVCFMT_LEFT, 60);

	// add VisGroups to list
	UpdateGroupList();

	bInitialized = TRUE;

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFilterControl::UpdateGroupList(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc == NULL)
	{
		return;
	}

	m_cGroupBox.SetRedraw(FALSE);
	m_cGroupBox.DeleteAllItems();

	POSITION pos = pDoc->VisGroups_GetHeadPosition();
	int iIndex = 0;
	while (pos != NULL)
	{
		CVisGroup *pGroup = pDoc->VisGroups_GetNext(pos);
		m_cGroupBox.InsertItem(iIndex, pGroup->GetName(), pGroup->IsVisible());
		m_cGroupBox.SetItemData(iIndex, pGroup->GetID());
		++iIndex;
	}

	m_cGroupBox.SetRedraw(TRUE);
	m_cGroupBox.Invalidate();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pCmdUI - 
//-----------------------------------------------------------------------------
void CFilterControl::UpdateControl(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetActiveWorld() ? TRUE : FALSE);
}


//-----------------------------------------------------------------------------
// Purpose: Disables the group list when there's no active world or when the
//			visgroups are currently overridden by the "Show All" button.
//-----------------------------------------------------------------------------
void CFilterControl::UpdateControlGroups(CCmdUI *pCmdUI)
{
	pCmdUI->Enable((GetActiveWorld() != NULL) && !CVisGroup::IsShowAllActive());
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTarget - 
//			bDisableIfNoHndler - 
//-----------------------------------------------------------------------------
void CFilterControl::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	UpdateDialogControls(pTarget, FALSE);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFilterControl::OnShowAllGroups(void)
{
	CButton *pButton = (CButton *)GetDlgItem(IDC_SHOW_ALL);
	if (pButton != NULL)
	{
		UINT uCheck = pButton->GetCheck();
		CVisGroup::ShowAllVisGroups(uCheck == 1);

		CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
		pDoc->UpdateVisibilityAll();

		UpdateGroupList();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFilterControl::OnEditGroups(void)
{
	CEditGroups dlg;
	dlg.DoModal();

	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc != NULL)
	{
		pDoc->SetModifiedFlag();
	}

	UpdateGroupList();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//			*pInfo - 
// Output : static BOOL
//-----------------------------------------------------------------------------
static BOOL MarkMembersOfGroup(CMapClass *pObject, MARKMEMBERSINFO *pInfo)
{
	if (pObject->GetVisGroup() == pInfo->pGroup)
	{
		if (!pObject->IsVisible())
		{
			return(TRUE);
		}

		CMapClass *pSelectObject = pObject->PrepareSelection(pInfo->eSelectMode);
		if (pSelectObject)
		{
			pInfo->pDoc->SelectObject(pSelectObject, CMapDoc::scSelect);
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CFilterControl::OnMarkMembers(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();

	int iSel = m_cGroupBox.GetNextItem(-1, LVNI_SELECTED);
	if (iSel == -1)
	{
		return;
	}

	// get current group
	DWORD id = m_cGroupBox.GetItemData(iSel);
	CVisGroup *pGroup = pDoc->VisGroups_GroupForID(id);
	if (pGroup == NULL)
	{
		return;
	}

	MARKMEMBERSINFO info;
	info.pGroup = pGroup;
	info.pDoc = pDoc;
	info.eSelectMode = pDoc->Selection_GetMode();

	pDoc->SelectObject(NULL, CMapDoc::scClear);

	CMapWorld *pWorld = pDoc->GetMapWorld();
	pWorld->EnumChildren((ENUMMAPCHILDRENPROC)MarkMembersOfGroup, (DWORD)&info);
	pDoc->SelectObject(NULL, CMapDoc::scUpdateDisplay);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPos - 
//-----------------------------------------------------------------------------
void CFilterControl::OnWindowPosChanged(WINDOWPOS *pPos)
{
	if (bInitialized && pPos->flags & SWP_SHOWWINDOW)
	{
		UpdateGroupList();
	}

	CWorldcraftBar::OnWindowPosChanged(pPos);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bShow - 
//			nStatus - 
//-----------------------------------------------------------------------------
void CFilterControl::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		UpdateGroupList();
	}

	CWorldcraftBar::OnShowWindow(bShow, nStatus);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nState - 
//			pWnd - 
//			bMinimized - 
//-----------------------------------------------------------------------------
void CFilterControl::OnActivate(UINT nState, CWnd* pWnd, BOOL bMinimized)
{
	if(nState == WA_ACTIVE)
		UpdateGroupList();

	CWorldcraftBar::OnActivate(nState, pWnd, bMinimized);
}


//-----------------------------------------------------------------------------
// Purpose: Called when the visibility of a group is toggled in the groups list.
// Input  : wParam - Index of item in the groups list that was toggled.
//			lParam - 0 to hide, 1 to show.
// Output : Returns zero.
//-----------------------------------------------------------------------------
LRESULT CFilterControl::OnListToggleState(WPARAM wParam, LPARAM lParam)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc != NULL)
	{
		//
		// Update the visibility of the group.
		//
		int id = m_cGroupBox.GetItemData(wParam);
		CVisGroup *pGroup = pDoc->VisGroups_GroupForID(id);
		ASSERT(pGroup != NULL);
		if (pGroup != NULL)
		{
			pGroup->SetVisible(lParam != 0);
			pDoc->UpdateVisibilityAll();
			pDoc->SetModifiedFlag();
		}
	}

	return(0);
}

