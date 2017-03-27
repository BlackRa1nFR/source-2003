// MasterTest.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MasterTest.h"
#include "MasterTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMasterTestApp

BEGIN_MESSAGE_MAP(CMasterTestApp, CWinApp)
	//{{AFX_MSG_MAP(CMasterTestApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMasterTestApp construction

CMasterTestApp::CMasterTestApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMasterTestApp object

CMasterTestApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMasterTestApp initialization

BOOL CMasterTestApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CMasterTestDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
