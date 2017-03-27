// AddEmailDialog.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "AddEmailDialog.h"
#include "EmailManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddEmailDialog dialog


CAddEmailDialog::CAddEmailDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CAddEmailDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddEmailDialog)
	m_strAddress = _T("");
	m_strMagnitude = _T("12");
	m_strUnits = _T("hours");
	//}}AFX_DATA_INIT
}


void CAddEmailDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddEmailDialog)
	DDX_Text(pDX, IDC_EMAILADDRESS, m_strAddress);
	DDX_Text(pDX, IDC_MAGNITUDE, m_strMagnitude);
	DDX_CBString(pDX, IDC_UNIT, m_strUnits);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddEmailDialog, CDialog)
	//{{AFX_MSG_MAP(CAddEmailDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddEmailDialog message handlers


void CAddEmailDialog::OnOK() 
{
	// TODO: Add extra validation here
	
	UpdateData( TRUE );

	if ( m_strAddress.IsEmpty() || !m_strAddress.FindOneOf( "@" ) )
	{
		AfxMessageBox( "Must enter a valid email address!" );
		return;
	}

	if ( m_strMagnitude.IsEmpty() || atof( m_strMagnitude ) <= 0.0f )
	{
		AfxMessageBox( "Must enter a valid number!" );
		return;
	}

	if ( m_strUnits.IsEmpty() )
	{
		AfxMessageBox( "Must choose a valid unit" );
		return;
	}

	char interval[ 256 ];
	sprintf( interval, "%s %s", m_strMagnitude, m_strUnits );

	GetEmailManager()->AddEmailAddress( m_strAddress, interval );

	CDialog::OnOK();
}

BOOL CAddEmailDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_strUnits = "hours";

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
