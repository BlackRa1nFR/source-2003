// OptionsDlg1.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "OptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg dialog


COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsDlg)
	m_strPlayers = _T("");
	m_strServers = _T("");
	m_strDays = _T("");
	m_strMaster = _T("");
	//}}AFX_DATA_INIT
}


void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDlg)
	DDX_Text(pDX, IDC_MAXPLAYERS, m_strPlayers);
	DDX_Text(pDX, IDC_MAXSERVERS, m_strServers);
	DDX_Text(pDX, IDC_DAYSINMEMORY, m_strDays);
	DDX_Text(pDX, IDC_MASTERSERVER, m_strMaster);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	//{{AFX_MSG_MAP(COptionsDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg message handlers
