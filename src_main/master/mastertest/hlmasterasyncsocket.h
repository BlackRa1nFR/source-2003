// HLMasterAsyncSocket.h : header file

#define MASTER_BUFSIZE 8192

class CMasterTestDlg;
/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket command target
class CHLMasterAsyncSocket : public CAsyncSocket
{
// Attributes
public:
	BOOL bInitialized;
	CMessageBuffer * pMessageBuffer;
	CMessageBuffer * pSendBuffer;

// Operations
public:
	CMasterTestDlg *pDialog;
	CHLMasterAsyncSocket(CMasterTestDlg *pDlg);
	virtual ~CHLMasterAsyncSocket();

// Overrides
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHLMasterAsyncSocket)
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CHLMasterAsyncSocket)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
};

/////////////////////////////////////////////////////////////////////////////
