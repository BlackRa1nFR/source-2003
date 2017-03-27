// HLMaster.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"

#include "proto_oob.h"

#include "HLMasterProtocol.h"
#include "HLMaster.h"
#include "HLMasterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CHLCmdInfo : public CCommandLineInfo
{
public:
	CHLCmdInfo( void );
	void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );
 
	int m_nPort;
	BOOL m_bGenerateLogs;
	CString m_strLocalIPAddress;
	BOOL m_bAskedForHelp;
};

CHLCmdInfo::CHLCmdInfo( void )
{
	m_nPort = PORT_MASTER;
	m_bGenerateLogs = FALSE;
	m_bAskedForHelp = FALSE;
	CCommandLineInfo();
}

void CHLCmdInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
	CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
	CString strParam(lpszParam);

	strParam.MakeLower();
	if ( strParam.Find("udpport=") != -1 )
		{
		strParam.Delete(0,strParam.Find('=')+1);
		m_nPort = atoi(strParam);
		}
	else if ( strParam.Find("logs=") != -1 )
		{
		strParam.Delete(0,strParam.Find('=')+1);
		if ( strParam.CompareNoCase("true") == 0 )
			m_bGenerateLogs = TRUE;
		}
	else if ( strParam.Find("localip=") != -1 )
		{
		strParam.Delete(0,strParam.Find('=')+1);
		m_strLocalIPAddress = strParam;
		}
	else if ( strParam == "?" || strParam == "h" )
		m_bAskedForHelp = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterApp

BEGIN_MESSAGE_MAP(CHLMasterApp, CWinApp)
	//{{AFX_MSG_MAP(CHLMasterApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHLMasterApp construction

CHLMasterApp::CHLMasterApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

CHLMasterApp::~CHLMasterApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance

}
/////////////////////////////////////////////////////////////////////////////
// The one and only CHLMasterApp object

CHLMasterApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CHLMasterApp initialization

BOOL CHLMasterApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Parse command line for standard shell commands, DDE, file open
	CHLCmdInfo cmdInfo;
	ParseCommandLine( cmdInfo );

	if ( cmdInfo.m_bAskedForHelp )
		{
		MessageBox(NULL,"Usage: hlmaster [/udpport=#] [/logs=(true|false)] [/localip=a.b.c.d]","hl master command line",MB_OK);
		return FALSE;
		}
	
	CHLMasterDlg dlg( cmdInfo.m_nPort, cmdInfo.m_bGenerateLogs, cmdInfo.m_strLocalIPAddress );
	m_pMainWnd = &dlg;
	dlg.DoModal();
	
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
