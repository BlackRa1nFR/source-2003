// MasterTestDlg.h : header file
//
#define TEST_TIMER 1

class CHLMasterAsyncSocket;
/////////////////////////////////////////////////////////////////////////////
// CMasterTestDlg dialog

class CMasterTestDlg : public CDialog
{
// Construction
public:
	void CreateFakeGUID(char *pszGUID, int nLen);
	BOOL RequestGUIDValidation(CHLMasterAsyncSocket *pHLMasterSocket);
	void DataReceived(int nDataSize);
	void DataSent(int nDataSize);
	void ClearData();
	void ClearSockets();
	BOOL RequestServerList(CHLMasterAsyncSocket *pHLMasterSocket);
	BOOL SendHeartbeat( int challenge, CHLMasterAsyncSocket *pHLMasterSocket);
	BOOL GetChallenge(CHLMasterAsyncSocket *pHLMasterSocket);
	CMasterTestDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CMasterTestDlg();

// Dialog Data
	//{{AFX_DATA(CMasterTestDlg)
	enum { IDD = IDD_MASTERTEST_DIALOG };
	CSpinButtonCtrl	m_spinRate;
	CSpinButtonCtrl	m_spinSockets;
	CSpinButtonCtrl	m_spinTime;
	CStatic	m_statTimeLeft;
	CStatic	m_statRecv;
	CStatic	m_statSent;
	CString	m_hIPAddress;
	CString	m_hPort;
	CString	m_hMsgSize;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMasterTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	clock_t m_tLastDataReceived;
	float m_fTestTime;
	clock_t m_tTestStartTime;
	BOOL m_bTestStopped;
	int m_nDataReceived;
	int m_nDataSent;
	int m_nResponsesReceived;
	int m_nRequestsSent;
	BOOL m_bTimerActive;
	clock_t m_tStartTime;
	CHLMasterAsyncSocket **m_pMasterSockets;
	int m_nMasterSockets;
	float fRate; 
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMasterTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnGo();
	afx_msg void OnStop();
	afx_msg void OnStoptest();
	afx_msg void OnRadioServerinfo();
	afx_msg void OnRadioGuid();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
