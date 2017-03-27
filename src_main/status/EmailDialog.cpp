// EmailDialog.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "EmailDialog.h"
#include "AddEmailDialog.h"
#include "EmailManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEmailDialog dialog


CEmailDialog::CEmailDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CEmailDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEmailDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CEmailDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEmailDialog)
	DDX_Control(pDX, IDC_EMAILLIST, m_emailList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEmailDialog, CDialog)
	//{{AFX_MSG_MAP(CEmailDialog)
	ON_BN_CLICKED(IDC_ADDEMAIL, OnAddemail)
	ON_BN_CLICKED(IDC_REMOVEEMAIL, OnRemoveemail)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmailDialog message handlers

void CEmailDialog::OnAddemail() 
{
	// TODO: Add your control notification handler code here
	CAddEmailDialog dlg;
	dlg.DoModal();

	RepopulateList();
}

void CEmailDialog::OnRemoveemail() 
{
	// TODO: Add your control notification handler code here
	int idx = m_emailList.GetCurSel();
	if ( idx != -1 )
	{
		GetEmailManager()->RemoveItemByIndex( idx );
		RepopulateList();
	}
}

void CEmailDialog::RepopulateList()
{
	m_emailList.ResetContent();

	int c = GetEmailManager()->GetItemCount();
	for ( int i = 0; i < c; i++ )
	{
		char buf[ 256 ];
		GetEmailManager()->GetItemDescription( i, buf, sizeof( buf ) );
		m_emailList.AddString( buf );
	}
}

BOOL CEmailDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	RepopulateList();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
