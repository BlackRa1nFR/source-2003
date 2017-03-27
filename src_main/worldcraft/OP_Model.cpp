//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "MapEntity.h"
#include "MapStudioModel.h"
#include "OP_Model.h"
#include "GameData.h"
#include "ObjectProperties.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(COP_Model, CObjectPage)


BEGIN_MESSAGE_MAP(COP_Model, CObjectPage)
	//{{AFX_MSG_MAP(COP_Model)
	ON_CBN_SELCHANGE(IDC_SEQUENCE, OnSelChangeSequence)
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


const int FRAME_SCROLLBAR_RANGE = 1000;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COP_Model::COP_Model() : CObjectPage(COP_Model::IDD)
{
	//{{AFX_DATA_INIT(COP_Model)
	//}}AFX_DATA_INIT

	m_pEditObjectRuntimeClass = RUNTIME_CLASS(editCEditGameClass);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COP_Model::~COP_Model()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDX - 
//-----------------------------------------------------------------------------
void COP_Model::DoDataExchange(CDataExchange* pDX)
{
	CObjectPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COP_Model)
	DDX_Control(pDX, IDC_SEQUENCE, m_ComboSequence);
	DDX_Control(pDX, IDC_FRAME_SCROLLBAR, m_ScrollBarFrame);
	//}}AFX_DATA_MAP
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Mode - 
//			pData - 
//-----------------------------------------------------------------------------
void COP_Model::UpdateData(int Mode, PVOID pData)
{
	if (!IsWindow(m_hWnd) || !pData)
	{
		return;
	}
	
	CEditGameClass *pObj = (CEditGameClass *)pData;
	if (Mode == LoadFirstData)
	{
		CMapStudioModel *pModel = GetModelHelper();
		if (pModel != NULL)
		{
			//
			// Fill the combo box with sequence names.
			//
			char szName[MAX_PATH];
			int nCount = pModel->GetSequenceCount();
			for (int i = 0; i < nCount; i++)
			{
				pModel->GetSequenceName(i, szName);
				m_ComboSequence.AddString(szName);
			}
			m_ComboSequence.SetCurSel(pModel->GetSequence());
			UpdateFrameText(pModel->GetFrame());
		}
	}
	else if (Mode == LoadData)
	{
		ASSERT(false);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool COP_Model::SaveData(void)
{
	if (!IsWindow(m_hWnd))
	{
		return false;
	}

	//
	// Apply the dialog data to all the objects being edited.
	//
	POSITION pos = m_pObjectList->GetHeadPosition();
	while (pos != NULL)
	{
		m_pObjectList->GetNext(pos);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszClass - 
//-----------------------------------------------------------------------------
void COP_Model::UpdateForClass(LPCTSTR pszClass)
{
	if (!IsWindow(m_hWnd))
	{
		return;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flFrame - 
//-----------------------------------------------------------------------------
void COP_Model::UpdateFrameText(float flFrame)
{
	char szFrame[40];
	sprintf(szFrame, "%0.2f", flFrame);
	GetDlgItem(IDC_FRAME_TEXT)->SetWindowText(szFrame);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL COP_Model::OnInitDialog() 
{
	CObjectPage::OnInitDialog();

	//
	// Set the frame number scrollbar range.
	//
	m_ScrollBarFrame.SetRange(0, FRAME_SCROLLBAR_RANGE);

	return TRUE;	             
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nSBCode - 
//			nPos - 
//			pScrollBar - 
//-----------------------------------------------------------------------------
void COP_Model::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar) 
{
	if (pScrollBar == (CScrollBar *)&m_ScrollBarFrame)
	{
		CMapStudioModel *pModel = GetModelHelper();
		if (pModel != NULL)
		{
			float flFrame = (float)nPos / (float)FRAME_SCROLLBAR_RANGE;
			pModel->SetFrame(flFrame);
			UpdateFrameText(flFrame);
		}
	}
	
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COP_Model::OnSelChangeSequence(void)
{
	CMapStudioModel *pModel = GetModelHelper();
	if (pModel != NULL)
	{
		int nIndex = m_ComboSequence.GetCurSel();
		pModel->SetSequence(nIndex);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapStudioModel *COP_Model::GetModelHelper(void)
{
	CMapClass *pObject = m_pObjectList->GetHead();
	if (pObject != NULL)
	{
		CMapEntity *pEntity = dynamic_cast <CMapEntity *>(pObject);
		if (pEntity != NULL)
		{
			CMapStudioModel *pModel = pEntity->GetChildOfType((CMapStudioModel *)NULL);
			return pModel;
		}
	}
	return NULL;
}
