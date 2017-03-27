//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a dialog for editing visgroups.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "EditGroups.h"
#include "MainFrm.h"
#include "MapWorld.h"
#include "CustomMessages.h"
#include "GlobalFunctions.h"
#include "VisGroup.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CEditGroups, CDialog)
	//{{AFX_MSG_MAP(CEditGroups)
	ON_BN_CLICKED(IDC_COLOR, OnColor)
	ON_EN_CHANGE(IDC_NAME, OnChangeName)
	ON_BN_CLICKED(IDC_NEW, OnNew)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_LBN_SELCHANGE(IDC_GROUPLIST, OnSelchangeGrouplist)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Constructor.
// Input  : pParent - Parent window.
//-----------------------------------------------------------------------------
CEditGroups::CEditGroups(CWnd* pParent /*=NULL*/)
	: CDialog(CEditGroups::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditGroups)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


//-----------------------------------------------------------------------------
// Purpose: Exchanges data between controls and data members.
// Input  : pDX - 
//-----------------------------------------------------------------------------
void CEditGroups::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditGroups)
	DDX_Control(pDX, IDC_NAME, m_cName);
	DDX_Control(pDX, IDC_GROUPLIST, m_cGroupList);
	//}}AFX_DATA_MAP
}


//-----------------------------------------------------------------------------
// Purpose: Returns the visgroup associated with an item in the visgroup list.
// Input  : iSel - Index of item in list box.
//-----------------------------------------------------------------------------
CVisGroup *CEditGroups::GetSelectedGroup(int iSel)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc == NULL)
	{
		return(NULL);
	}

	if (iSel == -1)
	{
		iSel = m_cGroupList.GetCurSel();
	}

	if (iSel == LB_ERR)
	{
		return(NULL);
	}

	WORD id = m_cGroupList.GetItemData(iSel);
	return(pDoc->VisGroups_GroupForID(id));
}


//-----------------------------------------------------------------------------
// Purpose: Sets the object's color as the visgroup color if the object belongs
//			to the given visgroup.
// Input  : pObject - Object to evaluate.
//			pGroup - Visgroup to check against.
// Output : Returns TRUE to continue enumerating.
//-----------------------------------------------------------------------------
static BOOL UpdateObjectColor(CMapClass *pObject, CVisGroup *pGroup)
{
	if ((pGroup != NULL) && (pObject->GetVisGroup() == pGroup))
	{
		pObject->SetColorFromVisGroups();
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Invokes the color picker dialog to modify the selected visgroup.
//-----------------------------------------------------------------------------
void CEditGroups::OnColor(void) 
{
	CVisGroup *pGroup = GetSelectedGroup();

	if (pGroup != NULL)
	{
		color32 rgbColor = pGroup->GetColor();
		CColorDialog dlg(RGB(rgbColor.r, rgbColor.g, rgbColor.b), CC_FULLOPEN);

		if (dlg.DoModal() == IDOK)
		{
			// change group color
			pGroup->SetColor(GetRValue(dlg.m_cc.rgbResult), GetGValue(dlg.m_cc.rgbResult), GetBValue(dlg.m_cc.rgbResult));
			m_cColorBox.SetColor(dlg.m_cc.rgbResult, TRUE);

			// change all object colors
			GetActiveWorld()->EnumChildren(ENUMMAPCHILDRENPROC(UpdateObjectColor), DWORD(pGroup));
			CMapDoc::GetActiveMapDoc()->UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_COLOR_ONLY);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when the contents of the name edit control changes. Renames
//			the currently selected group with the new edit control contents.
//-----------------------------------------------------------------------------
void CEditGroups::OnChangeName(void)
{
	// save name
	CVisGroup * pGroup = GetSelectedGroup();

	int iSel = m_cGroupList.GetCurSel();
	ASSERT(iSel != LB_ERR);
	CString szName;
	m_cName.GetWindowText(szName);
	m_cGroupList.SetRedraw(FALSE);
	m_cGroupList.DeleteString(iSel);
	int iIndex = m_cGroupList.InsertString(iSel, szName);
	m_cGroupList.SetItemData(iSel, pGroup->GetID());
	m_cGroupList.SetCurSel(iSel);
	m_cGroupList.SetRedraw(TRUE);

	CRect r;
	m_cGroupList.GetItemRect(iSel, r);
	m_cGroupList.InvalidateRect(r);

	pGroup->SetName(szName);
}


//-----------------------------------------------------------------------------
// Purpose: Creates a new visgroup and adds it to the list.
//-----------------------------------------------------------------------------
void CEditGroups::OnNew(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	CVisGroup *pGroup = pDoc->VisGroups_AddGroup("new group");

	pGroup->SetVisible(true);

	int iIndex = m_cGroupList.AddString(pGroup->GetName());
	m_cGroupList.SetItemData(iIndex, pGroup->GetID());
	m_cGroupList.SetCurSel(iIndex);
	m_cName.EnableWindow(TRUE);
	OnSelchangeGrouplist();
	m_cName.SetActiveWindow();
}


//-----------------------------------------------------------------------------
// Purpose: Removes the object from the given group if it is a member of the
//			group.
// Input  : pObject - Object to remove from the group.
//			pGroup - Group from which to remove the object.
// Output : Returns TRUE to continue enumerating.
//-----------------------------------------------------------------------------
static BOOL RemoveFromGroup(CMapClass *pObject, CVisGroup *pGroup)
{
	if ((pGroup != NULL) && (pObject->GetVisGroup() == pGroup))
	{
		pObject->SetVisGroup(NULL);
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Called when the remove button is pressed from the visgroup editor.
//			Deletes the selected visgroup and removes all references to it.
//-----------------------------------------------------------------------------
void CEditGroups::OnRemove(void) 
{
	// kill the group. go thru objects loaded too and make sure they
	// don't have the id (set the id to 0)
	CVisGroup *pGroup = GetSelectedGroup();
	int iCurSel = m_cGroupList.GetCurSel();
	if(!pGroup)
		return;	// nothing
	m_cGroupList.SetRedraw(FALSE);
	m_cGroupList.DeleteString(iCurSel);
	if(iCurSel >= m_cGroupList.GetCount())
		m_cGroupList.SetCurSel(m_cGroupList.GetCount()-1);
	else
		m_cGroupList.SetCurSel(iCurSel);
	m_cGroupList.SetRedraw(TRUE);
	m_cGroupList.Invalidate();

	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc != NULL)
	{
		CMapWorld *pWorld = pDoc->GetMapWorld();
		if (pWorld != NULL)
		{
			pWorld->EnumChildren(ENUMMAPCHILDRENPROC(RemoveFromGroup), DWORD(pGroup));
		}

		if (!pGroup->IsVisible())
		{
			pDoc->UpdateVisibilityAll();
			pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_OBJECTS);
		}
		else
		{
			pDoc->UpdateAllViews(NULL, MAPVIEW_UPDATE_DISPLAY | MAPVIEW_COLOR_ONLY);
		}

		pDoc->VisGroups_RemoveID(pGroup->GetID());
	}

	delete pGroup;
}


//-----------------------------------------------------------------------------
// Purpose: Handles selection change in the visgroup list. Updates the static
//			text controls with the name and colot of the selected visgroup.
//-----------------------------------------------------------------------------
void CEditGroups::OnSelchangeGrouplist(void)
{
	CVisGroup *pGroup;
	
	pGroup = GetSelectedGroup();

	//
	// Update the name and color controls.
	//
	m_cName.SetWindowText(pGroup->GetName());
	color32 rgbColor = pGroup->GetColor();
	m_cColorBox.SetColor(RGB(rgbColor.r, rgbColor.g, rgbColor.b), TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Sets up initial state of dialog.
//-----------------------------------------------------------------------------
BOOL CEditGroups::OnInitDialog(void)
{
	CDialog::OnInitDialog();

	m_cColorBox.SubclassDlgItem(IDC_COLORBOX, this);

	//
	// Fill the listbox with the visgroup names.
	//
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	ASSERT(pDoc != NULL);
	if (pDoc != NULL)
	{
		POSITION p = pDoc->VisGroups_GetHeadPosition();
		while (p != NULL)
		{
			CVisGroup *pGroup = pDoc->VisGroups_GetNext(p);
			int iIndex = m_cGroupList.AddString(pGroup->GetName());
			m_cGroupList.SetItemData(iIndex, pGroup->GetID());
		}
		
		//
		// Disable the edit name window if there are no visgroups in the list.
		//
		if (m_cGroupList.GetCount())
		{
			m_cGroupList.SetCurSel(0);
			OnSelchangeGrouplist();
		}
		else
		{
			m_cName.EnableWindow(FALSE);
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Called when the dialog is closing. Sends a message to update the
//			visgroup dialog bar in case any visgroup changes were made.
//-----------------------------------------------------------------------------
void CEditGroups::OnClose(void)
{
	GetMainWnd()->GlobalNotify(WM_MAPDOC_CHANGED);
	CDialog::OnClose();
}


//-----------------------------------------------------------------------------
// Purpose: Called when the dialog is closing. Sends a message to update the
//			visgroup dialog bar in case any visgroup changes were made.
//-----------------------------------------------------------------------------
BOOL CEditGroups::DestroyWindow(void)
{
	GetMainWnd()->GlobalNotify(WM_MAPDOC_CHANGED);
	return(CDialog::DestroyWindow());
}


BEGIN_MESSAGE_MAP(CColorBox, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Sets the color of the color box.
// Input  : c - RGB color to set.
//			bRedraw - TRUE repaints, FALSE does now.
//-----------------------------------------------------------------------------
void CColorBox::SetColor(COLORREF c, BOOL bRedraw)
{
	m_c = c;
	if (bRedraw)
	{
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Fills the colorbox window with the current color.
//-----------------------------------------------------------------------------
void CColorBox::OnPaint(void)
{
	CPaintDC dc(this);
	CRect r;
	GetClientRect(r);
	CBrush brush(m_c);
	dc.FillRect(r, &brush);
}

