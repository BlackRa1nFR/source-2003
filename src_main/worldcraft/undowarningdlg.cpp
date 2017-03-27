// UndoWarningDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Worldcraft.h"
#include "UndoWarningDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUndoWarningDlg dialog


CUndoWarningDlg::CUndoWarningDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUndoWarningDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUndoWarningDlg)
	m_bNoShow = FALSE;
	//}}AFX_DATA_INIT
}


void CUndoWarningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUndoWarningDlg)
	DDX_Check(pDX, IDC_NOSHOW, m_bNoShow);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUndoWarningDlg, CDialog)
	//{{AFX_MSG_MAP(CUndoWarningDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUndoWarningDlg message handlers
