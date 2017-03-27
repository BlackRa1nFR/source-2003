//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the groups page of the Object Properties dialog.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "MapWorld.h"
#include "OP_Groups.h"
#include "EditGroups.h"
#include "GlobalFunctions.h"
#include "CustomMessages.h"
#include "ObjectProperties.h"
#include "VisGroup.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(COP_Groups, CObjectPage)


COP_Groups::COP_Groups() : CObjectPage(COP_Groups::IDD)
{
	//{{AFX_DATA_INIT(COP_Groups)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pEditObjectRuntimeClass = RUNTIME_CLASS(editCMapClass);
}

COP_Groups::~COP_Groups()
{
}

void COP_Groups::DoDataExchange(CDataExchange* pDX)
{
	CObjectPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COP_Groups)
	DDX_Control(pDX, IDC_GROUPS, m_cGroups);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COP_Groups, CObjectPage)
	//{{AFX_MSG_MAP(COP_Groups)
	ON_BN_CLICKED(IDC_EDITGROUPS, OnEditgroups)
	//}}AFX_MSG_MAP
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

const char * NO_GROUP_STRING = "(no group)";
const DWORD NO_GROUP_ID = 0xffff;
const DWORD VALUE_DIFFERENT_ID = 0xfffe;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : b - 
//-----------------------------------------------------------------------------
void COP_Groups::SetMultiEdit(bool b)
{
	CObjectPage::SetMultiEdit(b);

	UpdateGrouplist();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool COP_Groups::SaveData(void)
{
	if (!IsWindow(m_hWnd))
	{
		return(false);
	}

	//
	// Apply the dialog data to all the objects being edited.
	//
	POSITION pos = m_pObjectList->GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = m_pObjectList->GetNext(pos);

		// get current selection
		int iSel = m_cGroups.GetCurSel();

		// find ID from selection (in listbox data)
		DWORD id = m_cGroups.GetItemData(iSel);
		if (id == NO_GROUP_ID && pObject->GetVisGroup())
		{
			pObject->SetVisGroup(NULL);
		}
		else if (id != VALUE_DIFFERENT_ID)
		{
			CVisGroup *pGroup = CMapDoc::GetActiveMapDoc()->VisGroups_GroupForID(id);
			pObject->SetVisGroup(pGroup);
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Mode - 
//			pData - 
//-----------------------------------------------------------------------------
void COP_Groups::UpdateData(int Mode, PVOID pData)
{
	static BOOL bDiffSelected = FALSE;

	if(!IsWindow(m_hWnd) || !pData)
	{
		return;
	}

	CMapClass *pObject = (CMapClass*) pData;

	CMapWorld * pWorld = GetActiveWorld();

	if(Mode == LoadData || Mode == LoadFirstData)
	{
		if(Mode == LoadFirstData)
		{
			UpdateGrouplist();

			// loading the first object -> just select the group
			CVisGroup *pGroup = pObject->GetVisGroup();
			if (pGroup != NULL)
			{
				int iSize = m_cGroups.GetCount();
				DWORD id = pGroup->GetID();
				for(int i = 0; i < iSize; i++)
				{
					DWORD itemid = 	m_cGroups.GetItemData(i);
					if(itemid == id)
					{
						m_cGroups.SetCurSel(i);
						break;
					}
				}
			}
			else
			{
				m_cGroups.SelectString(-1, NO_GROUP_STRING);
			}

			bDiffSelected = FALSE;
		}
		else if(!bDiffSelected)
		{
			// loading the next object -> if the group is not the same
			//  as the currently selected group, set it to "(different)"
			int iSel = m_cGroups.GetCurSel();
			DWORD id;
			ASSERT(iSel != -1);
			id = m_cGroups.GetItemData(iSel);

			// if NOT any of the following -> 
			//  1. no object group, and id is at no_group_id,
			//  2. object has group, and id is set at that group
			// then set value_different_string!

			if (!((!pObject->GetVisGroup() && id == NO_GROUP_ID) ||
				(pObject->GetVisGroup() && id == pObject->GetVisGroup()->GetID())))
			{
				m_cGroups.SelectString(-1, VALUE_DIFFERENT_STRING);
				bDiffSelected = TRUE;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Groups::UpdateGrouplist(void)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc == NULL)
	{
		return;
	}

	if(!::IsWindow(m_cGroups.m_hWnd))
		return;

	int iSel = m_cGroups.GetCurSel();
	DWORD oldselid = 0xffffffff;
	if(iSel != LB_ERR)
		oldselid = m_cGroups.GetItemData(iSel);

	m_cGroups.ResetContent();

	if (IsMultiEdit())
	{
		m_cGroups.InsertString(0, VALUE_DIFFERENT_STRING);
		m_cGroups.SetItemData(0, VALUE_DIFFERENT_ID);
	}

	m_cGroups.InsertString(0, NO_GROUP_STRING);
	m_cGroups.SetItemData(0, NO_GROUP_ID);

	POSITION pos = pDoc->VisGroups_GetHeadPosition();
	while (pos != NULL)
	{
		CVisGroup *pGroup = pDoc->VisGroups_GetNext(pos);

		int iIndex = m_cGroups.AddString(pGroup->GetName());
		m_cGroups.SetItemData(iIndex, pGroup->GetID());
	}

	// restore old selection based on stored id
	iSel = 0;
	int iSize = m_cGroups.GetCount();
	for(int i = 0; i < iSize; i++)
	{
		if(m_cGroups.GetItemData(i) == oldselid)
		{
			iSel = i;
			break;
		}
	}

	m_cGroups.SetCurSel(iSel);
	m_cGroups.Invalidate();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Groups::OnEditgroups() 
{
	CEditGroups dlg;
	dlg.DoModal();

	UpdateGrouplist();
}


BOOL COP_Groups::OnInitDialog() 
{
	CObjectPage::OnInitDialog();
	UpdateGrouplist();

	return TRUE;
}

void COP_Groups::OnSetFocus(CWnd *pOld)
{
	UpdateGrouplist();
	CPropertyPage::OnSetFocus(pOld);
}
