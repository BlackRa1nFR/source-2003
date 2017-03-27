// SchedulerDialog.cpp : implementation file
//

#include "stdafx.h"
#include "status.h"
#include "SchedulerDialog.h"
#include "Scheduler.h"

static CScheduler g_Scheduler;

CScheduler *GetScheduler()
{
	return &g_Scheduler; 
}

/*#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
*/

/////////////////////////////////////////////////////////////////////////////
// CSchedulerDialog dialog


CSchedulerDialog::CSchedulerDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSchedulerDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSchedulerDialog)
	m_Time = 0;
	//}}AFX_DATA_INIT
}


void CSchedulerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSchedulerDialog)
	DDX_Control(pDX, IDC_DATETIMEPICKER1, m_TimeControl);
	DDX_Control(pDX, IDC_TIMELIST, m_TimeList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSchedulerDialog, CDialog)
	//{{AFX_MSG_MAP(CSchedulerDialog)
	ON_BN_CLICKED(IDC_ADDTIME, OnAddtime)
	ON_BN_CLICKED(IDC_DELTIME, OnDeltime)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSchedulerDialog message handlers.

BOOL CSchedulerDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	for( int i = 0; i < GetScheduler()->GetItemCount(); i++ )
	{
		m_TimeList.AddString( GetScheduler()->GetTime(i) );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CSchedulerDialog::OnOK() 
{
	// TODO: Add extra validation here
	GetScheduler()->RemoveAllTimes();
	for ( int i = 0; i < m_TimeList.GetCount(); i++ )
	{
		CString str;
		int n = m_TimeList.GetTextLen( i );
		m_TimeList.GetText( i, str.GetBuffer(n) );
		GetScheduler()->AddTime(str);
	}

	CDialog::OnOK();
}

void CSchedulerDialog::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void CSchedulerDialog::OnAddtime() 
{
   SYSTEMTIME time;

   m_TimeControl.GetTime(&time);

   CString s;
   s.Format(_T("%.2d:%.2d:%.2d"), time.wHour, time.wMinute, time.wSecond);
   m_TimeList.AddString( s );
}

void CSchedulerDialog::OnDeltime() 
{
	// TODO: Add your control notification handler code here
	
	int idx = m_TimeList.GetCurSel();
	if ( idx != -1 )
	{
		m_TimeList.DeleteString( idx );
	}
}

