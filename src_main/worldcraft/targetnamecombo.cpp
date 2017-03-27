//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: Implements an autoselection combo box that color codes the text
//			based on whether the current selection represents a single entity,
//			multiple entities, or an unresolved entity targetname.
//
//			The fonts are as follows:
//
//			Single entity		black, normal weight
//			Multiple entities	black, bold
//			Unresolved			red, normal weight
//
//=============================================================================

#include "stdafx.h"
#include "MapEntity.h"
#include "TargetNameCombo.h"


BEGIN_MESSAGE_MAP(CTargetNameComboBox, CAutoSelComboBox)
	//{{AFX_MSG_MAP(CTargetNameComboBox)
	ON_CONTROL_REFLECT_EX(CBN_SELCHANGE, OnSelChange)
	ON_CONTROL_REFLECT_EX(CBN_DROPDOWN, OnDropDown)
	ON_CONTROL_REFLECT_EX(CBN_CLOSEUP, OnCloseUp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTargetNameComboBox::CTargetNameComboBox(void)
{
	m_pEntityList = NULL;
	m_bBold = false;
}


//-----------------------------------------------------------------------------
// Purpose: Frees allocated memory.
//-----------------------------------------------------------------------------
CTargetNameComboBox::~CTargetNameComboBox(void)
{
	FreeSubLists();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetNameComboBox::FreeSubLists(void)
{
	POSITION pos = m_SubLists.GetHeadPosition();
	while (pos != NULL)
	{
		CMapEntityList *pList = m_SubLists.GetNext(pos);
		delete pList;
	}

	m_SubLists.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Attaches an entity list to the combo box. This list will be used
//			for matching targetnames to entities in the world.
// Input  : pEntityList - The beauty of Hungarian notation and meaningful naming
//				makes this comment utterly unnecessary.
//-----------------------------------------------------------------------------
void CTargetNameComboBox::SetEntityList(CMapEntityList *pEntityList)
{
	//
	// Create a normal and bold font.
	//
	if (!m_NormalFont.m_hObject)
	{
		CFont *pFont = GetFont();
		LOGFONT LogFont;
		pFont->GetLogFont(&LogFont);
		m_NormalFont.CreateFontIndirect(&LogFont);
		
		LogFont.lfWeight = FW_BOLD;
		m_BoldFont.CreateFontIndirect(&LogFont);
	}

	m_pEntityList = pEntityList;

	FreeSubLists();

	SetRedraw(FALSE);
	ResetContent();

	if (m_pEntityList != NULL)
	{
		POSITION pos = m_pEntityList->GetHeadPosition();
		while (pos != NULL)
		{
			CMapEntity *pEntity = m_pEntityList->GetNext(pos);
			const char *pszTargetName = pEntity->GetKeyValue("targetname");
			if (pszTargetName != NULL)
			{
				//
				// If the targetname is not in the combo box, add it to the combo as the
				// first entry in an entity list. The list is necessary because there
				// may be several entities in the map with the same targetname.
				//
				int nIndex = FindStringExact(-1, pszTargetName);
				if (nIndex == CB_ERR)
				{
					nIndex = AddString(pszTargetName);
					if (nIndex >= 0)
					{
						CMapEntityList *pList = new CMapEntityList;
						pList->AddTail(pEntity);
						SetItemDataPtr(nIndex, pList);

						//
						// Keep track of all the sub lists so we can delete them later.
						//
						m_SubLists.AddTail(pList);
					}
				}
				//
				// Else append the entity to the given targetname's list.
				//
				else
				{
					CMapEntityList *pList = (CMapEntityList *)GetItemDataPtr(nIndex);
					pList->AddTail(pEntity);
				}
			}
		}
	}

	SetRedraw(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetNameComboBox::OnUpdateText(void)
{
	BaseClass::OnUpdateText();
	UpdateForCurSel(GetCurSel());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetNameComboBox::UpdateForCurSel(int nIndex)
{
	DWORD dwEditSel = GetEditSel();

	int nCount = 0;

	if (nIndex != CB_ERR)
	{
		CMapEntityList *pList = (CMapEntityList *)GetItemDataPtr(nIndex);
		nCount = pList->GetCount();
	}

	// Font change replaces window text so save
	CString szSaveText;
	GetWindowText(szSaveText);

	if ((nCount == 0) || (nCount == 1))
	{
		SetFont(&m_NormalFont);
	}
	else
	{
		SetFont(&m_BoldFont);
	}

	if (nCount == 0)
	{
		SetTextColor(RGB(255, 0, 0));
	}
	else
	{
		// Known target. Render it in black.
		SetTextColor(RGB(0, 0, 0));
	}

	SetWindowText(szSaveText);
	SetEditSel(HIWORD(dwEditSel), LOWORD(dwEditSel));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE to prevent parent notification, FALSE to allow.
//-----------------------------------------------------------------------------
BOOL CTargetNameComboBox::OnSelChange(void)
{
	int nIndex = GetCurSel();
	UpdateForCurSel(nIndex);

	// Despite MSDN's lies, returning FALSE here allows the parent
	// window to hook the notification message as well, not TRUE.
	return m_bNotifyParent ? FALSE : TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetNameComboBox::SetTargetComboText(const char *szText)
{
	BaseClass::SetWindowText(szText);
	int nIndex = FindStringExact(-1, szText);
	UpdateForCurSel(nIndex);
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE to prevent parent notification, FALSE to allow.
//-----------------------------------------------------------------------------
BOOL CTargetNameComboBox::OnDropDown(void)
{
	// It's confusing if the list entries are in bold, so switch to normal weight.
	if (GetFont() == &m_BoldFont)
	{
		m_bBold = true;
		SetFont(&m_NormalFont);
	}

	// Despite MSDN's lies, returning FALSE here allows the parent
	// window to hook the notification message as well, not TRUE.
	return m_bNotifyParent ? FALSE : TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE to prevent parent notification, FALSE to allow.
//-----------------------------------------------------------------------------
BOOL CTargetNameComboBox::OnCloseUp(void)
{
	// Switch back to bold if we were bold when they dropped the combo down.
	if (m_bBold)
	{
		SetFont(&m_BoldFont);
	}

	// Despite MSDN's lies, returning FALSE here allows the parent
	// window to hook the notification message as well, not TRUE.
	return m_bNotifyParent ? FALSE : TRUE;
}
