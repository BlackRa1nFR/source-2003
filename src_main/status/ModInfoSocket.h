#ifndef OEM_BUILD
// ModInfoSocket.h : header file

#define MASTER_BUFSIZE 8192

#define MOD_RETRIES 3
#define MOD_TIMEOUT ( 1.0f )
// Manages the connection to the HL Master Server.

class CModList;
/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket command target
class CModInfoSocket : public CAsyncSocket
{
// Attributes
public:
	enum ModStatus
	{
		MOD_PENDING = 0,
		MOD_SUCCESS,
		MOD_FAILED
	};

	int CheckForResend( void );  // Returns 1 if proc. should continue, 0 otherwise
	void StartModRequest( CModList *ppList, char *address, int port );
	void RequestModInfo( char *pszRequest );

	void BatchModInfo( bool oldStyle );     // Game mod info
	
	CMessageBuffer * pMessageBuffer;

// Operations
public:
	CModInfoSocket( void );
	virtual ~CModInfoSocket();

// Overrides
protected:
	BOOL m_bMasterListReceived;
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModInfoSocket)
	virtual void OnReceive(int nErrorCode);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CModInfoSocket)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
	CModList *m_pList;
private:
	int iBytesRead;
	ModStatus m_modStatus;
	int m_nRetries;
	float m_fTimeOut;
	float m_fSendTime;
};

/////////////////////////////////////////////////////////////////////////////
#endif