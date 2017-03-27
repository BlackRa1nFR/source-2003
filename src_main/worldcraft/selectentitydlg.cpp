//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile: SelectEntityDlg.cpp $
// $Date: 8/03/99 6:57p $
//
//-----------------------------------------------------------------------------
// $Log: /Src/Worldcraft/SelectEntityDlg.cpp $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "worldcraft.h"
#include "SelectEntityDlg.h"
#include "GlobalFunctions.h"
#include "MapDoc.h"
#include "MapEntity.h"
#include "MapSolid.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CSelectEntityDlg::CSelectEntityDlg(CMapObjectList *pList,
								   CWnd* pParent /*=NULL*/)
	: CDialog(CSelectEntityDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSelectEntityDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pEntityList = pList;
}


void CSelectEntityDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectEntityDlg)
	DDX_Control(pDX, IDC_ENTITIES, m_cEntities);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectEntityDlg, CDialog)
	//{{AFX_MSG_MAP(CSelectEntityDlg)
	ON_LBN_SELCHANGE(IDC_ENTITIES, OnSelchangeEntities)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSelectEntityDlg::OnSelchangeEntities() 
{
	int iSel = m_cEntities.GetCurSel();

	CMapEntity *pEntity = (CMapEntity*) m_cEntities.GetItemData(iSel);

	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	pDoc->SelectObject(pEntity, CMapDoc::scClear | CMapDoc::scSelect | CMapDoc::scUpdateDisplay);
}


BOOL CSelectEntityDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// add entities from our list of entities to the listbox
	POSITION p = m_pEntityList->GetHeadPosition();
	while(p)
	{
		CMapClass *pObject = m_pEntityList->GetNext(p);
		if(!pObject->IsMapClass(MAPCLASS_TYPE(CMapEntity)))
			continue;
		CMapEntity *pEntity = (CMapEntity*) pObject;
		if(pEntity->IsPlaceholder())
			continue;
		int iIndex = m_cEntities.AddString(pEntity->GetClassName());
		m_cEntities.SetItemData(iIndex, DWORD(pEntity));
	}

	m_cEntities.SetCurSel(0);
	OnSelchangeEntities();

	return TRUE;
}

void CSelectEntityDlg::OnOK() 
{
	int iSel = m_cEntities.GetCurSel();
	m_pFinalEntity = (CMapEntity*) m_cEntities.GetItemData(iSel);
	
	CDialog::OnOK();
}
