// PatchTimeout.cpp : implementation file
//

#include "stdafx.h"
#include "PatchTimeout.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPatchTimeout dialog


CPatchTimeout::CPatchTimeout(CWnd* pParent /*=NULL*/)
	: CDialog(CPatchTimeout::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPatchTimeout)
	m_Timeout = 0;
	//}}AFX_DATA_INIT
}


void CPatchTimeout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPatchTimeout)
	DDX_Text(pDX, IDC_TIMEOUT_SECONDS, m_Timeout);
	DDV_MinMaxInt(pDX, m_Timeout, 5, 120);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPatchTimeout, CDialog)
	//{{AFX_MSG_MAP(CPatchTimeout)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPatchTimeout message handlers
