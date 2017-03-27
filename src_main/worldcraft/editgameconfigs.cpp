// EditGameConfigs.cpp : implementation file
//

#include "stdafx.h"
#include "worldcraft.h"
#include "EditGameConfigs.h"
#include "StrDlg.h"
#include "MapDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditGameConfigs dialog


CEditGameConfigs::CEditGameConfigs(BOOL bSelectOnly, 
								   CWnd* pParent /*=NULL*/)
	: CDialog(CEditGameConfigs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditGameConfigs)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bSelectOnly = bSelectOnly;
}


void CEditGameConfigs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditGameConfigs)
	DDX_Control(pDX, IDC_CONFIGS, m_cConfigs);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditGameConfigs, CDialog)
	//{{AFX_MSG_MAP(CEditGameConfigs)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_COPY, OnCopy)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_LBN_SELCHANGE(IDC_CONFIGS, OnSelchangeConfigs)
	ON_LBN_DBLCLK(IDC_CONFIGS, OnDblclkConfigs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditGameConfigs message handlers

void CEditGameConfigs::OnAdd() 
{
	char szName[128];
	szName[0] = 0;
	CStrDlg dlg(0, szName, "Enter the game's name:", "Add a game");
	if(dlg.DoModal() != IDOK)
		return;

	// add a new game config
	CGameConfig *pConfig = Options.configs.AddConfig();
	strcpy(pConfig->szName, dlg.m_string);

	FillConfigList(pConfig->dwID);
}

void CEditGameConfigs::OnCopy() 
{
	int iCurSel = m_cConfigs.GetCurSel();
	if(iCurSel == CB_ERR)
		return;

	CGameConfig *pConfig = Options.configs.FindConfig(
		m_cConfigs.GetItemData(iCurSel));

	CGameConfig *pNewConfig = Options.configs.AddConfig();
	pNewConfig->CopyFrom(pConfig);

	FillConfigList(pNewConfig->dwID);
}

void CEditGameConfigs::OnRemove() 
{
	int iCurSel = m_cConfigs.GetCurSel();
	if(iCurSel == CB_ERR)
		return;

	int iArrayIndex;
	CGameConfig *pConfig = Options.configs.FindConfig(
		m_cConfigs.GetItemData(iCurSel), &iArrayIndex);

	// check to see if any docs use this game - if so, can't
	//  delete it.
	POSITION p = CMapDoc::GetFirstDocPosition();
	while(p)
	{
		CMapDoc *pDoc = CMapDoc::GetNextDoc(p);
		if(pDoc->GetGame() == pConfig)
		{
			AfxMessageBox("You can't delete this game configuration now\n"
				"because some loaded documents are using it.\n"
				"If you want to delete it, you must close those\n"
				"documents first.");
			return;
		}
	}

	// remove sel
	m_cConfigs.DeleteString(iCurSel);

	Options.configs.Configs.RemoveAt(iArrayIndex);
	Options.configs.nConfigs--;
}

void CEditGameConfigs::FillConfigList(DWORD dwSelectID)
{
	// get current selection so we can keep it
	DWORD dwCurID = dwSelectID;
	int iNewIndex = -1;
	
	if(m_cConfigs.GetCurSel() != LB_ERR && dwCurID == 0xFFFFFFFF)
	{
		dwCurID = m_cConfigs.GetItemData(m_cConfigs.GetCurSel());
	}

	m_cConfigs.ResetContent();

	for(int i = 0; i < Options.configs.nConfigs; i++)
	{
		CGameConfig *pConfig = Options.configs.Configs[i];
		int iIndex = m_cConfigs.AddString(pConfig->szName);
		m_cConfigs.SetItemData(iIndex, pConfig->dwID);

		if (dwCurID == pConfig->dwID)
		{
			iNewIndex = iIndex;
		}
	}

	if (iNewIndex == -1)
	{
		iNewIndex = 0;
	}
	m_cConfigs.SetCurSel(iNewIndex);

	OnSelchangeConfigs();

	if (m_bSelectOnly && Options.configs.nConfigs == 1)
		OnOK();
}

void CEditGameConfigs::OnSelchangeConfigs() 
{
	int iCurSel = m_cConfigs.GetCurSel();
	if(iCurSel == LB_ERR)
		return;

	m_pSelectedGame = Options.configs.FindConfig(
		m_cConfigs.GetItemData(iCurSel));
}

BOOL CEditGameConfigs::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if(m_bSelectOnly)
	{
		SetWindowText("Select a game");

		GetDlgItem(IDOK)->SetWindowText("OK");
		GetDlgItem(IDC_REMOVE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_ADD)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_COPY)->ShowWindow(SW_HIDE);
	}

	FillConfigList();

	return TRUE;
}

void CEditGameConfigs::OnDblclkConfigs() 
{
	OnOK();
	
}
