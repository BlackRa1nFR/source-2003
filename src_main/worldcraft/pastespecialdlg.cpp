// PasteSpecialDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Worldcraft.h"
#include "PasteSpecialDlg.h"

#pragma warning(disable:4244)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPasteSpecialDlg dialog

static LPCTSTR pszIni = "Paste Special";

CPasteSpecialDlg::CPasteSpecialDlg(CWnd* pParent /*=NULL*/, BoundBox* pBox)
	: CDialog(CPasteSpecialDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPasteSpecialDlg)
	m_iCopies = 1;
	m_bGroup = FALSE;
	m_iOffsetX = 0;
	m_iOffsetY = 0;
	m_iOffsetZ = 0;
	m_fRotateX = 0.0f;
	m_fRotateZ = 0.0f;
	m_fRotateY = 0.0f;
	m_bCenterOriginal = TRUE;
	//}}AFX_DATA_INIT

	CWinApp *App = AfxGetApp();
	CString str;
	LPCTSTR p;

	m_iCopies = App->GetProfileInt(pszIni, "Copies", 1);
	m_bGroup = App->GetProfileInt(pszIni, "Group", FALSE);

	str = App->GetProfileString(pszIni, "Offset", "0 0 0");
	p = str.GetBuffer(0);
	m_iOffsetX = atoi(p);
	m_iOffsetY = atoi(strchr(p, ' ')+1);
	m_iOffsetZ = atoi(strrchr(p, ' ')+1);

	str = App->GetProfileString(pszIni, "Rotate", "0 0 0");
	p = str.GetBuffer(0);
	m_fRotateX = atof(p);
	m_fRotateY = atof(strchr(p, ' ')+1);
	m_fRotateZ = atof(strrchr(p, ' ')+1);

	m_bCenterOriginal = App->GetProfileInt(pszIni, "Center", TRUE);

	ObjectsBox = *pBox;
}

void CPasteSpecialDlg::SaveToIni()
{
	CWinApp *App = AfxGetApp();
	CString str;

	App->WriteProfileInt(pszIni, "Copies", m_iCopies);
	App->WriteProfileInt(pszIni, "Group", m_bGroup);

	str.Format("%d %d %d", m_iOffsetX, m_iOffsetY, m_iOffsetZ);
	App->WriteProfileString(pszIni, "Offset", str);

	str.Format("%.1f %.1f %.1f", m_fRotateX, m_fRotateY, m_fRotateZ);
	App->WriteProfileString(pszIni, "Rotate", str);

	App->WriteProfileInt(pszIni, "Center", m_bCenterOriginal);
}

void CPasteSpecialDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasteSpecialDlg)
	DDX_Text(pDX, IDC_COPIES, m_iCopies);
	DDV_MinMaxInt(pDX, m_iCopies, 1, 256);
	DDX_Check(pDX, IDC_GROUP, m_bGroup);
	DDX_Text(pDX, IDC_OFFSETX, m_iOffsetX);
	DDX_Text(pDX, IDC_OFFSETY, m_iOffsetY);
	DDX_Text(pDX, IDC_OFFSETZ, m_iOffsetZ);
	DDX_Text(pDX, IDC_ROTATEX, m_fRotateX);
	DDV_MinMaxFloat(pDX, m_fRotateX, 0.f, 360.f);
	DDX_Text(pDX, IDC_ROTATEZ, m_fRotateZ);
	DDV_MinMaxFloat(pDX, m_fRotateZ, 0.f, 360.f);
	DDX_Text(pDX, IDC_ROTATEY, m_fRotateY);
	DDV_MinMaxFloat(pDX, m_fRotateY, 0.f, 360.f);
	DDX_Check(pDX, IDC_CENTERORIGINAL, m_bCenterOriginal);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasteSpecialDlg, CDialog)
	//{{AFX_MSG_MAP(CPasteSpecialDlg)
	ON_BN_CLICKED(IDC_GETOFFSETX, OnGetoffsetx)
	ON_BN_CLICKED(IDC_GETOFFSETY, OnGetoffsety)
	ON_BN_CLICKED(IDC_GETOFFSETZ, OnGetoffsetz)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasteSpecialDlg message handlers

void CPasteSpecialDlg::GetOffset(int iAxis, int iEditCtrl)
{
	CWnd *pWnd = GetDlgItem(iEditCtrl);

	ASSERT(pWnd);

	// get current value
	CString strValue;
	pWnd->GetWindowText(strValue);
	int iValue = atoi(strValue);

	int iAxisSize = ObjectsBox.bmaxs[iAxis] - ObjectsBox.bmins[iAxis];

	if(iValue == iAxisSize)	// if it's already positive, make it neg
		strValue.Format("%d", -iAxisSize);
	else	// it's negative or !=, set it positive
		strValue.Format("%d", iAxisSize);

	// set the window text
	pWnd->SetWindowText(strValue);
}

void CPasteSpecialDlg::OnGetoffsetx() 
{
	GetOffset(0, IDC_OFFSETX);
}

void CPasteSpecialDlg::OnGetoffsety() 
{
	GetOffset(1, IDC_OFFSETY);
}

void CPasteSpecialDlg::OnGetoffsetz() 
{
	GetOffset(2, IDC_OFFSETZ);
}
