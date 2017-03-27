// ConfigureExesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "worldcraft.h"
#include "OPTBuild.h"
#include "Options.h"
#include "shlobj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// dvs: this is duplicated in RunMapExpertDlg.cpp!!
enum
{
	id_InsertParmMapFileNoExt = 0x100,
	id_InsertParmMapFile,
	id_InsertParmMapPath,
	id_InsertParmBspDir,
	id_InsertParmExeDir,
	id_InsertParmGameDir,
	id_InsertParmModDir,

	id_InsertParmEnd
};


COPTBuild::COPTBuild()
	: CPropertyPage(COPTBuild::IDD)
{
	//{{AFX_DATA_INIT(COPTBuild)
	//}}AFX_DATA_INIT

	m_pConfig = NULL;
}


void COPTBuild::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COPTBuild)
	DDX_Control(pDX, IDC_BSPDIR, m_cBSPDir);
	DDX_Control(pDX, IDC_VIS, m_cVIS);
	DDX_Control(pDX, IDC_LIGHT, m_cLIGHT);
	DDX_Control(pDX, IDC_GAME, m_cGame);
	DDX_Control(pDX, IDC_CSG, m_cCSG);
	DDX_Control(pDX, IDC_BSP, m_cBSP);
	DDX_Control(pDX, IDC_CONFIGS, m_cConfigs);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COPTBuild, CPropertyPage)
	//{{AFX_MSG_MAP(COPTBuild)
	ON_BN_CLICKED(IDC_BROWSE_CSG, OnBrowseCSG)
	ON_BN_CLICKED(IDC_BROWSE_BSP, OnBrowseBsp)
	ON_BN_CLICKED(IDC_BROWSE_GAME, OnBrowseGame)
	ON_BN_CLICKED(IDC_BROWSE_LIGHT, OnBrowseLight)
	ON_BN_CLICKED(IDC_BROWSE_VIS, OnBrowseVis)
	ON_CBN_SELCHANGE(IDC_CONFIGS, OnSelchangeConfigs)
	ON_BN_CLICKED(IDC_PARMS_CSG, OnParmsCSG)
	ON_BN_CLICKED(IDC_PARMS_BSP, OnParmsBsp)
	ON_BN_CLICKED(IDC_PARMS_GAME, OnParmsGame)
	ON_BN_CLICKED(IDC_PARMS_LIGHT, OnParmsLight)
	ON_BN_CLICKED(IDC_PARMS_VIS, OnParmsVis)
	ON_BN_CLICKED(IDC_BROWSE_BSPDIR, OnBrowseBspdir)
	ON_COMMAND_EX_RANGE(id_InsertParmMapFileNoExt, id_InsertParmEnd, HandleInsertParm)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void COPTBuild::DoBrowse(CWnd *pWnd)
{
	char szProgram[MAX_PATH];
	
	pWnd->GetWindowText(szProgram, sizeof szProgram);

	CFileDialog dlg(TRUE, ".exe", szProgram, OFN_NOCHANGEDIR |
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, 
		"Programs (*.exe)|*.exe||", this);

	if(dlg.DoModal() == IDCANCEL)
		return;

	strcpy(szProgram, dlg.GetPathName());
	pWnd->SetWindowText(szProgram);
}

void COPTBuild::OnBrowseBsp() 
{
	DoBrowse(&m_cBSP);
}

void COPTBuild::OnBrowseCSG(void) 
{
	DoBrowse(&m_cCSG);
}

void COPTBuild::OnBrowseGame() 
{
	DoBrowse(&m_cGame);
}

void COPTBuild::OnBrowseLight() 
{
	DoBrowse(&m_cLIGHT);
}

void COPTBuild::OnBrowseVis() 
{
	DoBrowse(&m_cVIS);
}

void COPTBuild::OnSelchangeConfigs() 
{
	SaveInfo(m_pConfig);

	m_pConfig = NULL;

	int iCurSel = m_cConfigs.GetCurSel();
	
	BOOL bKillFields = (iCurSel == CB_ERR) ? FALSE : TRUE;
	m_cCSG.EnableWindow(bKillFields);
	m_cBSP.EnableWindow(bKillFields);
	m_cLIGHT.EnableWindow(bKillFields);
	m_cVIS.EnableWindow(bKillFields);
	m_cGame.EnableWindow(bKillFields);
	m_cBSPDir.EnableWindow(bKillFields);
	
	if(iCurSel == CB_ERR)
		return;

	// get pointer to the configuration
	m_pConfig = Options.configs.FindConfig(m_cConfigs.GetItemData(iCurSel));

	// update dialog data
	m_cCSG.SetWindowText(m_pConfig->m_szCSG);
	m_cBSP.SetWindowText(m_pConfig->szBSP);
	m_cLIGHT.SetWindowText(m_pConfig->szLIGHT);
	m_cVIS.SetWindowText(m_pConfig->szVIS);
	m_cGame.SetWindowText(m_pConfig->szExecutable);
	m_cBSPDir.SetWindowText(m_pConfig->szBSPDir);
}


void COPTBuild::SaveInfo(CGameConfig *pConfig)
{
	if (!pConfig)
	{
		return;
	}

	m_cCSG.GetWindowText(pConfig->m_szCSG, sizeof(pConfig->m_szCSG));
	m_cBSP.GetWindowText(pConfig->szBSP, sizeof(pConfig->szBSP));
	m_cLIGHT.GetWindowText(pConfig->szLIGHT, sizeof(pConfig->szLIGHT));
	m_cVIS.GetWindowText(pConfig->szVIS, sizeof(pConfig->szVIS));
	m_cGame.GetWindowText(pConfig->szExecutable, sizeof(pConfig->szExecutable));
	m_cBSPDir.GetWindowText(pConfig->szBSPDir, sizeof(pConfig->szBSPDir));
}


void COPTBuild::UpdateConfigList()
{
	m_pConfig = NULL;

	int iCurSel = m_cConfigs.GetCurSel();
	DWORD dwSelID = 0xffffffff;
	if(iCurSel != CB_ERR)
		dwSelID = m_cConfigs.GetItemData(iCurSel);

	m_cConfigs.ResetContent();

	iCurSel = -1;	// reset it for check below

	// add configs to the combo box
	for(int i = 0; i < Options.configs.nConfigs; i++)
	{
		CGameConfig *pConfig = Options.configs.Configs[i];

		int iIndex = m_cConfigs.AddString(pConfig->szName);
		m_cConfigs.SetItemData(iIndex, pConfig->dwID);

		// found same config as was already selected - make sure
		//  it stays selected
		if(dwSelID != 0xffffffff && dwSelID == pConfig->dwID)
			iCurSel = iIndex;
	}

	m_cConfigs.SetCurSel(iCurSel == -1 ? 0 : iCurSel);
	OnSelchangeConfigs();
	SetModified();
}

BOOL COPTBuild::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	UpdateConfigList();
	SetModified(TRUE);
	
	return TRUE;
}

BOOL COPTBuild::OnApply() 
{
	SaveInfo(m_pConfig);
	
	return CPropertyPage::OnApply();
}

BOOL COPTBuild::HandleInsertParm(UINT nID)
// insert a parm at the current cursor location into the parameters
//  edit control
{
	LPCTSTR pszInsert;

	switch (nID)
	{
		case id_InsertParmMapFileNoExt:
			pszInsert = "$file";
			break;
		case id_InsertParmMapFile:
			pszInsert = "$file.$ext";
			break;
		case id_InsertParmMapPath:
			pszInsert = "$path";
			break;
		case id_InsertParmExeDir:
			pszInsert = "$exedir";
			break;
		case id_InsertParmBspDir:
			pszInsert = "$bspdir";
			break;
		case id_InsertParmGameDir:
			pszInsert = "$gamedir";
			break;
		case id_InsertParmModDir:
			pszInsert = "$moddir";
			break;
	}

	ASSERT(pszInsert != NULL);
	if (!pszInsert)
	{
		return TRUE;
	}

	m_pAddParmWnd->ReplaceSel(pszInsert);

	return TRUE;
}


void COPTBuild::InsertParm(UINT nID, CEdit *pEdit)
{
	m_pAddParmWnd = pEdit;

	// two stages - name/description OR data itself
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, id_InsertParmMapFileNoExt, "Map Filename (no extension)");
	menu.AppendMenu(MF_STRING, id_InsertParmMapFile, "Map Filename (with extension)");
	menu.AppendMenu(MF_STRING, id_InsertParmMapPath, "Map Path (no filename)");
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, id_InsertParmExeDir, "Game Executable Directory");
	menu.AppendMenu(MF_STRING, id_InsertParmBspDir, "BSP Directory");
	menu.AppendMenu(MF_STRING, id_InsertParmGameDir, "Game Directory");
	menu.AppendMenu(MF_STRING, id_InsertParmModDir, "Mod Directory");

	// track menu
	CWnd *pButton = GetDlgItem(nID);
	CRect r;
	pButton->GetWindowRect(r);
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, r.left, r.bottom, this, NULL);
}


void COPTBuild::OnParmsCSG(void)
{
	InsertParm(IDC_PARMS_CSG, &m_cCSG);	
}

void COPTBuild::OnParmsBsp() 
{
	InsertParm(IDC_PARMS_BSP, &m_cBSP);	
}

void COPTBuild::OnParmsGame() 
{
	InsertParm(IDC_PARMS_GAME, &m_cGame);
}

void COPTBuild::OnParmsLight() 
{
	InsertParm(IDC_PARMS_LIGHT, &m_cLIGHT);
}

void COPTBuild::OnParmsVis() 
{
	InsertParm(IDC_PARMS_VIS, &m_cVIS);
}

void COPTBuild::OnBrowseBspdir() 
{
	char szTmp[MAX_PATH];

	BROWSEINFO bi;
	memset(&bi, 0, sizeof bi);
	bi.hwndOwner = m_hWnd;
	bi.pszDisplayName = szTmp;
	bi.lpszTitle = "Select BSP file directory";
	bi.ulFlags = BIF_RETURNONLYFSDIRS;

	LPITEMIDLIST idl = SHBrowseForFolder(&bi);

	if(idl == NULL)
		return;

	SHGetPathFromIDList(idl, szTmp);
	CoTaskMemFree(idl);

	m_cBSPDir.SetWindowText(szTmp);
}
