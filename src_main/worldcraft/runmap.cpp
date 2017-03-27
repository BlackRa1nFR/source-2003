// RunMap.cpp : implementation file
//

#include "stdafx.h"
#include "Worldcraft.h"
#include "RunMap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRunMap dialog

static LPCTSTR pszSection = "Run Map";

CRunMap::CRunMap(CWnd* pParent /*=NULL*/)
	: CDialog(CRunMap::IDD, pParent)
{
	m_bSwitchMode = FALSE;

	//{{AFX_DATA_INIT(CRunMap)
	m_iVis = -1;
	m_bNoQuake = FALSE;
	m_strQuakeParms = _T("");
	m_bSaveVisiblesOnly = FALSE;
	m_iLight = -1;
	m_iCSG = -1;
	m_iQBSP = -1;
	//}}AFX_DATA_INIT

	// read from ini
	CWinApp *App = AfxGetApp();
	m_iCSG = App->GetProfileInt(pszSection, "CSG", 0);
	m_iQBSP = App->GetProfileInt(pszSection, "QBSP", 0);

	// The onlyents option was moved to the CSG setting, so don't allow it for BSP.
	if (m_iQBSP > 1)
	{
		m_iQBSP = 1;
	}

	m_iVis = App->GetProfileInt(pszSection, "Vis", 0);
	m_iLight = App->GetProfileInt(pszSection, "Light", 0);
	m_bNoQuake = App->GetProfileInt(pszSection, "No Game", 0);
	m_strQuakeParms = App->GetProfileString(pszSection, "Game Parms", "");
}


void CRunMap::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRunMap)
	DDX_Check(pDX, IDC_NOQUAKE, m_bNoQuake);
	DDX_Text(pDX, IDC_QUAKEPARMS, m_strQuakeParms);
	DDX_Check(pDX, IDC_SAVEVISIBLESONLY, m_bSaveVisiblesOnly);
	DDX_Radio(pDX, IDC_CSG0, m_iCSG);
	DDX_Radio(pDX, IDC_BSP0, m_iQBSP);
	DDX_Radio(pDX, IDC_VIS0, m_iVis);
	DDX_Radio(pDX, IDC_RAD0, m_iLight);
	//}}AFX_DATA_MAP
}


void CRunMap::SaveToIni(void)
{
	CWinApp *App = AfxGetApp();
	App->WriteProfileInt(pszSection, "CSG", m_iCSG);
	App->WriteProfileInt(pszSection, "QBSP", m_iQBSP);
	App->WriteProfileInt(pszSection, "Vis", m_iVis);
	App->WriteProfileInt(pszSection, "Light", m_iLight);
	App->WriteProfileInt(pszSection, "No Game", m_bNoQuake);
	App->WriteProfileString(pszSection, "Game Parms", m_strQuakeParms);
}


BEGIN_MESSAGE_MAP(CRunMap, CDialog)
	//{{AFX_MSG_MAP(CRunMap)
	ON_BN_CLICKED(IDC_EXPERT, OnExpert)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRunMap message handlers

void CRunMap::OnExpert() 
{
	m_bSwitchMode = TRUE;
	EndDialog(IDOK);
}

BOOL CRunMap::OnInitDialog() 
{
	CDialog::OnInitDialog();

	return TRUE;
}
