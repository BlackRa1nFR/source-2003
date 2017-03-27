// MasterTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MasterTest.h"
#include "MessageBuffer.h"
#include "HLMasterAsyncSocket.h"
#include "MasterTestDlg.h"
#include "..\HLMasterProtocol.h"
#include "proto_version.h"
#include "proto_oob.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define GUID_LEN 13

/////////////////////////////////////////////////////////////////////////////
// CMasterTestDlg dialog

CMasterTestDlg::CMasterTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMasterTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMasterTestDlg)
	m_hIPAddress = _T("127.0.0.1");
	m_hPort = _T("27010");
	m_hMsgSize = _T("1024");
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bTimerActive = FALSE;
	m_bTestStopped = FALSE;
}

CMasterTestDlg::~CMasterTestDlg()
{
	ClearSockets();
}

void CMasterTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMasterTestDlg)
	DDX_Control(pDX, IDC_SPIN_RATE, m_spinRate);
	DDX_Control(pDX, IDC_SPIN_SOCKETS, m_spinSockets);
	DDX_Control(pDX, IDC_SPIN_TIME, m_spinTime);
	DDX_Control(pDX, IDC_TIMELEFT, m_statTimeLeft);
	DDX_Control(pDX, IDC_RECV, m_statRecv);
	DDX_Control(pDX, IDC_SENT, m_statSent);
	DDX_Text(pDX, IDC_EDIT1, m_hIPAddress);
	DDX_Text(pDX, IDC_EDIT2, m_hPort);
	DDX_Text(pDX, IDC_EDIT3, m_hMsgSize);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMasterTestDlg, CDialog)
	//{{AFX_MSG_MAP(CMasterTestDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_GO, OnGo)
	ON_BN_CLICKED(IDC_STOP, OnStop)
	ON_BN_CLICKED(IDC_STOPTEST, OnStoptest)
	ON_BN_CLICKED(IDC_RADIO_SERVERINFO, OnRadioServerinfo)
	ON_BN_CLICKED(IDC_RADIO_GUID, OnRadioGuid)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMasterTestDlg message handlers

BOOL CMasterTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here

	m_tStartTime = (clock_t)0;
	m_pMasterSockets = NULL;
	m_nMasterSockets = 0;
	fRate = 0.0f;

	m_spinRate.SetRange(1,1000);
	m_spinTime.SetRange(1,6000);
	m_spinSockets.SetRange(1,256);

	m_spinRate.SetPos(5);
	m_spinTime.SetPos(60);
	m_spinSockets.SetPos(5);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMasterTestDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CMasterTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

#define NUM_CRITERIA 6
char *RandomCrit[] = 
{
	{ "" },
	{ "" },
	{ "\\map\\map011\\gamedir\\mod002" },
	{ "\\map\\map004\\gamedir\\mod003\\dedicated\\1" },
	{ "\\map\\de_dust\\gamedir\\mod004" },
	{ "\\map\\map001\\gamedir\\valve" },
};

char *GetRandomCriteria( void )
{
	//
	return RandomCrit[ rand() % NUM_CRITERIA ];
}

BOOL CMasterTestDlg::GetChallenge(CHLMasterAsyncSocket *pHLMasterSocket)
{
	if (!pHLMasterSocket->bInitialized)
	{
		BOOL bNoError;
		bNoError = pHLMasterSocket->Create(0,SOCK_DGRAM);
		// Check for errors
		if (!bNoError)
			return FALSE;
	
		bNoError = pHLMasterSocket->AsyncSelect(FD_CONNECT);
		if (!bNoError)
			return FALSE;

		// Try instantaneous connection
		bNoError = pHLMasterSocket->Connect( (char *)(LPCSTR)m_hIPAddress, atoi( ( char * )(LPCSTR)m_hPort ) );
		if (!bNoError)
			return FALSE;
	}

	pHLMasterSocket->bInitialized = TRUE;
	pHLMasterSocket->pMessageBuffer->SZ_Clear();
	pHLMasterSocket->pSendBuffer->MSG_WriteByte( A2A_GETCHALLENGE );
	pHLMasterSocket->AsyncSelect(FD_READ | FD_WRITE);

	int iSent;
	iSent = pHLMasterSocket->Send(
		pHLMasterSocket->pSendBuffer->GetData(),
		pHLMasterSocket->pSendBuffer->GetCurSize());

	if (iSent != SOCKET_ERROR)
	{
		DataSent(iSent);
		// Clear message
		pHLMasterSocket->pSendBuffer->SZ_Clear();
	}
	else
	{
		DataSent(0);
	}

	return TRUE;
}

#include "../info.h"

char *va (char *comment, ...)
{
	static char	string[2048];
	va_list	argptr;
	
	va_start (argptr, comment);
	vsprintf (string, comment, argptr);
	va_end (argptr);

	return string;
}


BOOL CMasterTestDlg::SendHeartbeat( int challenge, CHLMasterAsyncSocket *pHLMasterSocket)
{
	if (!pHLMasterSocket->bInitialized)
	{
		BOOL bNoError;
		bNoError = pHLMasterSocket->Create(0,SOCK_DGRAM);
		// Check for errors
		if (!bNoError)
			return FALSE;
	
		bNoError = pHLMasterSocket->AsyncSelect(FD_CONNECT);
		if (!bNoError)
			return FALSE;

		// Try instantaneous connection
		bNoError = pHLMasterSocket->Connect( (char *)(LPCSTR)m_hIPAddress, atoi( ( char * )(LPCSTR)m_hPort ) );
		if (!bNoError)
			return FALSE;
	}

	pHLMasterSocket->bInitialized = TRUE;
	pHLMasterSocket->pMessageBuffer->SZ_Clear();
	pHLMasterSocket->pSendBuffer->MSG_WriteByte( S2M_HEARTBEAT2 );
	pHLMasterSocket->pSendBuffer->MSG_WriteByte( '\n' );
	
	char	string[4096];    // Buffer for sending heartbeat
	char	info[ MAX_SINFO ];
	int		iHasPW = 0;
	char	szOS[2];

	memset(string, 0, 2048);
	
#if _WIN32
	strcpy( szOS, "w" );
#else
	strcpy( szOS, "l" );
#endif

	info[0]='\0';

	Info_SetValueForKey( info, "protocol", va( "%i", 43 ), MAX_SINFO );
	Info_SetValueForKey( info, "challenge", va( "%i", challenge ), MAX_SINFO );

	int max = rand() % 25 + 5;

	Info_SetValueForKey( info, "max", va( "%i", max ), MAX_SINFO );
	Info_SetValueForKey( info, "players", va( "%i", rand() % max ), MAX_SINFO );
	Info_SetValueForKey( info, "gamedir", va( "mod%03i", rand() % 20 ), MAX_SINFO );
	Info_SetValueForKey( info, "map", va( "map%03i", rand() % 20 ), MAX_SINFO );
	Info_SetValueForKey( info, "dedicated", va( "%i", rand() % 2 ) , MAX_SINFO );
	Info_SetValueForKey( info, "password", va( "%i", iHasPW ) , MAX_SINFO );
	Info_SetValueForKey( info, "os", szOS , MAX_SINFO );

	pHLMasterSocket->pSendBuffer->MSG_WriteString( info );
	pHLMasterSocket->pSendBuffer->MSG_WriteByte( '\n' );


	pHLMasterSocket->AsyncSelect(FD_READ | FD_WRITE);

	int iSent;
	iSent = pHLMasterSocket->Send(
		pHLMasterSocket->pSendBuffer->GetData(),
		pHLMasterSocket->pSendBuffer->GetCurSize());

	if (iSent != SOCKET_ERROR)
	{
		DataSent(iSent);
		// Clear message
		pHLMasterSocket->pSendBuffer->SZ_Clear();
	}
	else
	{
		DataSent(0);
	}

	return TRUE;
}


BOOL CMasterTestDlg::RequestServerList(CHLMasterAsyncSocket *pHLMasterSocket)
{
	UpdateData( TRUE );

	if (!pHLMasterSocket->bInitialized)
	{
		BOOL bNoError;
		bNoError = pHLMasterSocket->Create(0,SOCK_DGRAM);
		// Check for errors
		if (!bNoError)
			return FALSE;
	
		bNoError = pHLMasterSocket->AsyncSelect(FD_CONNECT);
		if (!bNoError)
			return FALSE;

		// Try instantaneous connection
		bNoError = pHLMasterSocket->Connect( (char *)(LPCSTR)m_hIPAddress, atoi( ( char * )(LPCSTR)m_hPort ) );
		if (!bNoError)
			return FALSE;
	}

	pHLMasterSocket->bInitialized = TRUE;
	pHLMasterSocket->pMessageBuffer->SZ_Clear();
	pHLMasterSocket->pSendBuffer->MSG_WriteByte( A2M_GET_SERVERS_BATCH2 );
	// Next id
	pHLMasterSocket->pSendBuffer->MSG_WriteLong( rand() % 9999 );
	// Search criteria
	pHLMasterSocket->pSendBuffer->MSG_WriteString( GetRandomCriteria() );
	pHLMasterSocket->AsyncSelect(FD_READ | FD_WRITE);

	int iSent;
	iSent = pHLMasterSocket->Send(
		pHLMasterSocket->pSendBuffer->GetData(),
		pHLMasterSocket->pSendBuffer->GetCurSize());

	if (iSent != SOCKET_ERROR)
	{
		DataSent(iSent);
		// Clear message
		pHLMasterSocket->pSendBuffer->SZ_Clear();
	}
	else
	{
		DataSent(0);
	}

	return TRUE;
}

int g_nTestType = 0;

void CMasterTestDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	// Do something
	clock_t tCurTime;
	float fElapsed;
	static int nLastSend, nLastRecv;

	if (nIDEvent == TEST_TIMER)
	{
		tCurTime = clock();
		fElapsed = (float)(tCurTime - m_tStartTime)/CLOCKS_PER_SEC;
		if (fElapsed >= 1.0f/fRate && !m_bTestStopped)
		{
			UpdateData( TRUE );

			for (int i = 0; i < m_nMasterSockets; i++)
			{
				if (g_nTestType == 0)
				{
					if (!RequestServerList(m_pMasterSockets[i]))
					{
						delete m_pMasterSockets[i];
						m_pMasterSockets[i] = NULL;
					}
				}
				else if (g_nTestType == 1)
				{
					if (!GetChallenge(m_pMasterSockets[i]))
					{
						delete m_pMasterSockets[i];
						m_pMasterSockets[i] = NULL;
					}
				}
			}
			m_tStartTime = clock();
		}

		// Update Send/Receive
		char szStatus[64];
		if (m_nDataSent != nLastSend)
		{
			sprintf(szStatus, "%i / %i", m_nRequestsSent, m_nDataSent);
			nLastSend = m_nDataSent;
			if (m_statSent.GetSafeHwnd())
				m_statSent.SetWindowText(szStatus);
		}
		if (m_nDataReceived != nLastRecv)
		{
			sprintf(szStatus, "%i / %i", m_nResponsesReceived, m_nDataReceived);
			nLastRecv = m_nDataReceived;
			if (m_statRecv.GetSafeHwnd())
				m_statRecv.SetWindowText(szStatus);
		}

		fElapsed = (float)(tCurTime - m_tTestStartTime)/CLOCKS_PER_SEC;		
		sprintf(szStatus, "%.1f", m_fTestTime - fElapsed);
		if (fElapsed <= m_fTestTime)
		{
			if (m_statTimeLeft.GetSafeHwnd())
				m_statTimeLeft.SetWindowText(szStatus);
		}
		else
		{
			// Shut it down.
			float fDataTimeOut;
			fDataTimeOut = (float)(tCurTime - m_tLastDataReceived)/CLOCKS_PER_SEC;
			if (fDataTimeOut < 30.0f)
			{
				OnStoptest();
			}
			else
			{
				OnStop();
			}
		}
	}
	CDialog::OnTimer(nIDEvent);
}

void CMasterTestDlg::OnGo() 
{
	// TODO: Add your control notification handler code here
	// Delete any old data:
	ClearSockets();
	
	ClearData();

	UpdateData(TRUE);
	m_nMasterSockets = m_spinSockets.GetPos();
	m_nMasterSockets = max(m_nMasterSockets, 1);
	m_nMasterSockets = min(m_nMasterSockets, 256);

	m_pMasterSockets = new CHLMasterAsyncSocket *[m_nMasterSockets];
	memset(m_pMasterSockets, 0, m_nMasterSockets * sizeof(CHLMasterAsyncSocket *));
	for (int i = 0; i < m_nMasterSockets; i++)
	{
		m_pMasterSockets[i] = new CHLMasterAsyncSocket(this);
	}

	m_tStartTime = clock();
	m_tTestStartTime = m_tStartTime;

	m_fTestTime = (float)m_spinTime.GetPos();
	m_fTestTime = max(m_fTestTime, 1.0f);
	m_fTestTime = min(m_fTestTime, 6000.0f);

	fRate = (float)m_spinRate.GetPos();
	fRate = max(fRate, 1.0f);
	fRate = min(fRate, 1000.0f);

	// Set a timer to go off about 2 x per cycle
	float fTimer;
	fTimer = 1.0f/fRate * 1000.0f;
	fTimer /= 2.0f;
	// 10 ms min.
	fTimer = max(1.0f, fTimer);

	SetTimer(TEST_TIMER, (int)fTimer, NULL);
	m_bTimerActive = TRUE;
	m_bTestStopped = FALSE;
}

void CMasterTestDlg::OnStop() 
{
	// TODO: Add your control notification handler code here
	ClearSockets();

	if (m_bTimerActive)
	{
		m_bTimerActive = FALSE;
		KillTimer(TEST_TIMER);
	}
}

void CMasterTestDlg::ClearSockets()
{
	if (m_pMasterSockets)
	{
		for (int i = 0; i < m_nMasterSockets; i++)
		{
			if (m_pMasterSockets[i])
			{
				delete m_pMasterSockets[i];
				m_pMasterSockets[i] = NULL;
			}
		}
		delete[] m_pMasterSockets;
		m_pMasterSockets = NULL;
		m_nMasterSockets = 0;
	}
}

void CMasterTestDlg::ClearData()
{
	m_nDataReceived      = 0;
	m_nDataSent          = 0; 
	m_nResponsesReceived = 0;
	m_nRequestsSent      = 0;

	m_tLastDataReceived = (clock_t)0;
}

void CMasterTestDlg::DataSent(int nDataSize)
{
	m_nDataSent += nDataSize;
	m_nRequestsSent++;
}

void CMasterTestDlg::DataReceived(int nDataSize)
{
	m_nDataReceived += nDataSize;
	m_nResponsesReceived++;

	m_tLastDataReceived = clock();
}

void CMasterTestDlg::OnStoptest() 
{
	// TODO: Add your control notification handler code here
	m_bTestStopped = TRUE;
}

BOOL CMasterTestDlg::RequestGUIDValidation(CHLMasterAsyncSocket *pHLMasterSocket)
{
	if (!pHLMasterSocket->bInitialized)
	{
		BOOL bNoError;
		bNoError = pHLMasterSocket->Create(0,SOCK_DGRAM);
		// Check for errors
		if (!bNoError)
			return FALSE;
	
		bNoError = pHLMasterSocket->AsyncSelect(FD_CONNECT);
		if (!bNoError)
			return FALSE;

		// Try instantaneous connection
		bNoError = pHLMasterSocket->Connect("207.153.132.168", 27010);
		if (!bNoError)
			return FALSE;
	}

	pHLMasterSocket->bInitialized = TRUE;
	// Fixme, add an outgoing buffer?
	pHLMasterSocket->pMessageBuffer->SZ_Clear();
	//pHLMasterSocket->pSendMessageBuffer->SZ_Clear();
	//pHLMasterSocket->pSendBuffer->MSG_WriteByte(C2M_GUIDHEARTBEAT);

	char szGUID[GUID_LEN];
	CreateFakeGUID( szGUID, GUID_LEN );

	pHLMasterSocket->pSendBuffer->MSG_WriteBuf( GUID_LEN, szGUID );
	pHLMasterSocket->AsyncSelect(FD_READ | FD_WRITE);

	int iSent;
	iSent = pHLMasterSocket->Send(
		pHLMasterSocket->pSendBuffer->GetData(),
		pHLMasterSocket->pSendBuffer->GetCurSize());

	if (iSent != SOCKET_ERROR)
	{
		DataSent(iSent);
		// Clear message
		pHLMasterSocket->pSendBuffer->SZ_Clear();
	}
	else
	{
		DataSent( 0 );
	}


	return TRUE;
}

void CMasterTestDlg::OnRadioServerinfo() 
{
	// TODO: Add your control notification handler code here
	g_nTestType = 0;
}

void CMasterTestDlg::OnRadioGuid() 
{
	// TODO: Add your control notification handler code here
	g_nTestType = 1;
}

void CMasterTestDlg::CreateFakeGUID(char *pszGUID, int nLen)
{
	int i;

	if ((rand() % 20) == 5)
	{
		// Fake one is all '1's
		memset(pszGUID, '1', nLen);
		return;
	}

	for ( i = 0; i < nLen; i++ )
	{
		pszGUID[i] = '0' + (unsigned char)(rand() % 10);
	}
}
