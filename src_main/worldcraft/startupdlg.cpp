// StartupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Worldcraft.h"
#include "StartupDlg.h"
#include "Options.h"
#include <process.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStartupDlg dialog


CStartupDlg::CStartupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStartupDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CStartupDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CStartupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStartupDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStartupDlg, CDialog)
	//{{AFX_MSG_MAP(CStartupDlg)
	ON_BN_CLICKED(IDC_WAHH, OnWahh)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_SHOWORDERFORM, OnOrder)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStartupDlg message handlers

void CStartupDlg::OnWahh() 
{
#if 0
	char * pWahh1[] = 
	{
		"I don't like nag screens!",
		"Neither do I!",
		NULL, NULL,
		"Stop pushing me.",
		NULL, NULL, NULL,
		"Look - just click \"I Agree!\"!",
		NULL,
		"Bug the other guy, wouldya?!",
		NULL
	};
	char * pWahh2[] = 
	{
		"I hope you've registered.",
		"It's been more than 30 days!",
		NULL, NULL,
		"Don't I deserve it?",
		NULL, NULL, NULL,
		"I know you love me! :)",
		NULL,
		"Bug the other guy, wouldya?!",
		NULL
	};
	char * pWahh3[] = 
	{
		"Another day, another level.",
		NULL
	};

	int iWhichWahh;
	int nWahhs[] = { 12, 12, 2 };
	static int iWahh = 0;
	char *p;

	if(0)//Options.uDaysSinceInstalled > 39)
	{
		AfxMessageBox("Your evaluation period has expired.\n"
			"Please register Worldcraft.");
		char szBuf[MAX_PATH];
		GetWindowsDirectory(szBuf, MAX_PATH);
		strcat(szBuf, "\\notepad.exe");
		_spawnl(_P_WAIT, szBuf, szBuf, "order.txt", NULL);
	}

	if(Options.uDaysSinceInstalled < 30)
	{
		p = pWahh1[iWahh];
		iWhichWahh = 0;
	}
	else if(Options.uDaysSinceInstalled < 60)
	{
		p = pWahh2[iWahh];
		iWhichWahh = 1;
	}
	else
	{
		p = pWahh3[iWahh];
		iWhichWahh = 2;
	}
	if(iWahh == nWahhs[iWhichWahh])
		return;
	
	++iWahh;

	if(p)
		GetDlgItem(IDC_WAHH)->SetWindowText(p);
#endif
}

BOOL CStartupDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	OnWahh();

	return TRUE;
}

void CStartupDlg::OnOrder()
{
	char szBuf[MAX_PATH];
	GetWindowsDirectory(szBuf, MAX_PATH);
	strcat(szBuf, "\\notepad.exe");
	_spawnl(_P_NOWAIT, szBuf, szBuf, "order.txt", NULL);
}