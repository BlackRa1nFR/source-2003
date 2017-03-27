//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the spawnflags page of the Entity Properties dialog.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "OP_Flags.h"
#include "GameData.h"
#include "ObjectProperties.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COP_Flags property page

IMPLEMENT_DYNCREATE(COP_Flags, CObjectPage)

COP_Flags::COP_Flags() : CObjectPage(COP_Flags::IDD)
{
	//{{AFX_DATA_INIT(COP_Flags)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pEditObjectRuntimeClass = RUNTIME_CLASS(editCEditGameClass);
}

COP_Flags::~COP_Flags()
{
}

void COP_Flags::DoDataExchange(CDataExchange* pDX)
{
	CObjectPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COP_Flags)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COP_Flags, CObjectPage)
	//{{AFX_MSG_MAP(COP_Flags)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COP_Flags message handlers

void COP_Flags::UpdateData(int Mode, PVOID pData)
{
	int i, j, nBitPattern;


	if(!IsWindow(m_hWnd) || !pData)
	{
		return;
	}
	
	CEditGameClass *pObj = (CEditGameClass*) pData;

	if (Mode == LoadFirstData)
	{
		UpdateForClass(pObj->GetClassName());

		for ( i = 0; i < m_CheckList.GetCount(); i++ )
		{
			m_CheckList.SetCheckStyle(BS_AUTOCHECKBOX);
			m_CheckList.SetCheck(i, pObj->GetSpawnFlag(m_nItemBitNum[i]) ? TRUE : FALSE);
		}
	}
	else if (Mode == LoadData)
	{
		//
		// Merge spawnflags with existing checkbox settings.
		//
		for ( i = 0; i < m_CheckList.GetCount(); i++ )
		{
			m_CheckList.SetCheckStyle(BS_AUTO3STATE);
			BOOL bObjChecked = pObj->GetSpawnFlag(m_nItemBitNum[i]) ? TRUE : FALSE;
			if (bObjChecked != m_CheckList.GetCheck(i))
			{
				m_CheckList.SetCheck(i, 2);			// tri-state check if merge shows different settings
			}
		}
	}

	// strip all unused bits from spawnflags
	for ( i = 0; i < 24; i++ )
	{
		nBitPattern = 1 << i;
		// is spawnflag for this bit set?
		if ( pObj->GetSpawnFlag(nBitPattern) )
		{
			// then see if its allowed to be
			for ( j = 0; j < m_CheckList.GetCount(); j ++ )
			{
				if ( m_nItemBitNum[j] == nBitPattern )
					break;
			}
			// we fail to find it?
			if ( j == m_CheckList.GetCount() )
				pObj->SetSpawnFlag(nBitPattern, FALSE);		// clear flag if so
		}
	}

}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool COP_Flags::SaveData(void)
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
		CEditGameClass *pEdit = dynamic_cast <CEditGameClass *>(pObject);
		ASSERT(pEdit != NULL);

		if ( pEdit != NULL )
		{
			for ( int i = 0; i < m_CheckList.GetCount(); i++ )
			{
				// don't save tri-stated bit
				if ( m_CheckList.GetCheck(i) != 2 )
				{
					pEdit->SetSpawnFlag(m_nItemBitNum[i], m_CheckList.GetCheck(i) ? TRUE : FALSE);
				}
			}
		}
	}

	return(true);
}


void COP_Flags::UpdateForClass(LPCTSTR pszClass)
{
	extern GameData *pGD;

	GDclass * pClass = pGD->ClassForName(pszClass);

	if(!IsWindow(m_hWnd))
		return;

	m_CheckList.ResetContent();

	if(pClass)
	{
		GDinputvariable *pVar = pClass->VarForName("spawnflags");

		if(!pVar)
			return;

		int nItems = pVar->m_Items.GetSize();

		for ( int i = 0; i < nItems; i++ )
		{
			// save bit position for later
			m_nItemBitNum[i] = pVar->m_Items[i].iValue;
			m_CheckList.InsertString(i, pVar->m_Items[i].szCaption);
		}
	}
}

BOOL COP_Flags::OnInitDialog() 
{
	CObjectPage::OnInitDialog();

	// Subclass checklistbox
	m_CheckList.SubclassDlgItem(IDC_CHECKLIST, this);
	m_CheckList.SetCheckStyle(BS_AUTOCHECKBOX);
	m_CheckList.ResetContent();

	return TRUE;	             
}

