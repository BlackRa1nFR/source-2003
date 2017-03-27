//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "ReplaceTexDlg.h"
#include "MainFrm.h"
#include "GlobalFunctions.h"
#include "TextureBrowser.h"
#include "TextureSystem.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CReplaceTexDlg::CReplaceTexDlg(int nSelected, CWnd* pParent /*=NULL*/)
	: CDialog(CReplaceTexDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CReplaceTexDlg)
	m_iSearchAll = nSelected ? FALSE : TRUE;
	m_strFind = _T("");
	m_strReplace = _T("");
	m_iAction = 0;
	m_bMarkOnly = FALSE;
	m_bHidden = FALSE;
	//}}AFX_DATA_INIT

	m_nSelected = nSelected;
}


void CReplaceTexDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReplaceTexDlg)
	DDX_Control(pDX, IDC_FIND, m_cFind);
	DDX_Control(pDX, IDC_REPLACE, m_cReplace);
	DDX_Control(pDX, IDC_REPLACEPIC, m_cReplacePic);
	DDX_Control(pDX, IDC_FINDPIC, m_cFindPic);
	DDX_Radio(pDX, IDC_INMARKED, m_iSearchAll);
	DDX_Text(pDX, IDC_FIND, m_strFind);
	DDX_Text(pDX, IDC_REPLACE, m_strReplace);
	DDX_Radio(pDX, IDC_ACTION, m_iAction);
	DDX_Check(pDX, IDC_MARKONLY, m_bMarkOnly);
	DDX_Check(pDX, IDC_HIDDEN, m_bHidden);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReplaceTexDlg, CDialog)
	//{{AFX_MSG_MAP(CReplaceTexDlg)
	ON_BN_CLICKED(IDC_BROWSEREPLACE, OnBrowsereplace)
	ON_BN_CLICKED(IDC_BROWSEFIND, OnBrowsefind)
	ON_EN_UPDATE(IDC_FIND, OnUpdateFind)
	ON_EN_UPDATE(IDC_REPLACE, OnUpdateReplace)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReplaceTexDlg message handlers


void CReplaceTexDlg::BrowseTex(int iEdit)
{
	CString strTex;
	CWnd *pWnd = GetDlgItem(iEdit);

	pWnd->GetWindowText(strTex);

	CTextureBrowser *pBrowser = new CTextureBrowser(GetMainWnd());
	pBrowser->SetUsed(iEdit == IDC_FIND);
	pBrowser->SetInitialTexture(strTex);

	if (pBrowser->DoModal() == IDOK)
	{
		IEditorTexture *pTex = g_Textures.FindActiveTexture(pBrowser->m_cTextureWindow.szCurTexture);
		char szName[MAX_PATH];
		if (pTex != NULL)
		{
			pTex->GetShortName(szName);
		}
		else
		{
			szName[0] = '\0';
		}
		pWnd->SetWindowText(szName);
	}

	delete pBrowser;
}

void CReplaceTexDlg::OnBrowsereplace() 
{
	BrowseTex(IDC_REPLACE);
}

void CReplaceTexDlg::OnBrowsefind() 
{
	BrowseTex(IDC_FIND);
}

//
// find/replace text string updates:
//

void CReplaceTexDlg::OnUpdateFind() 
{
	// get texture window and set texture in there
	CString strTex;
	m_cFind.GetWindowText(strTex);
	IEditorTexture *pTex = g_Textures.FindActiveTexture(strTex);
	m_cFindPic.SetTexture(pTex);
}

void CReplaceTexDlg::OnUpdateReplace() 
{
	// get texture window and set texture in there
	CString strTex;
	m_cReplace.GetWindowText(strTex);
	IEditorTexture *pTex = g_Textures.FindActiveTexture(strTex);
	m_cReplacePic.SetTexture(pTex);
}

BOOL CReplaceTexDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if(!m_nSelected)
	{
		CWnd *pWnd = GetDlgItem(IDC_INMARKED);
		pWnd->EnableWindow(FALSE);
	}

	OnUpdateFind();

	return TRUE;
}
