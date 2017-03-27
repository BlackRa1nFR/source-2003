// WarnDialog.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "WarnDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWarnDialog dialog


CWarnDialog::CWarnDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CWarnDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWarnDialog)
	m_sMinPlayers = _T("");
	m_sWarnEmail = _T("");
	//}}AFX_DATA_INIT
}


void CWarnDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWarnDialog)
	DDX_Text(pDX, IDC_EDIT1, m_sMinPlayers);
	DDX_Text(pDX, IDC_EDIT2, m_sWarnEmail);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWarnDialog, CDialog)
	//{{AFX_MSG_MAP(CWarnDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWarnDialog message handlers

void CWarnDialog::OnOK() 
{
	// TODO: Add extra validation here
	
	UpdateData( TRUE );

	if ( m_sWarnEmail.IsEmpty() || !m_sWarnEmail.FindOneOf( "@" ) )
	{
		AfxMessageBox( "Must enter a valid email address!" );
		return;
	}

	if ( m_sMinPlayers.IsEmpty() || atoi( m_sMinPlayers ) <= 0 )
	{
		AfxMessageBox( "Must enter a valid number!" );
		return;
	}

	CDialog::OnOK();
}

void CWarnDialog::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}
