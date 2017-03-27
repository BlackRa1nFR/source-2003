// HLModMasterDlg.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CHLModMasterDlg dialog
class CHLModMasterDlg : public CDialog
{
// Construction
public:
//	CHLModMasterDlg( int nMasterPort, CWnd* pParent = NULL);	// standard constructor
	CHLModMasterDlg( int nMasterPort,
					 BOOL bGenerateLogs,
					 CString strLocalIPAddress,
					 CWnd* pParent = NULL);	// standard constructor
	~CHLModMasterDlg();

// Initialization and shutdown
	void Master_Init();
	void NET_Init();
	void NET_GetLocalAddress (void);

	void CheckForNewLogfile();
	void OpenNewLogfile();

// Routines for handling serving out titan/auth/and master server lists
	void ParseMods();
	void ParseServers();
	void ParseModFile( char *pszFileName );

	const char *GetValue( mod_t *pMod, const char *pszKey );
	void SetKey( mod_t *pMod, const char *pszKey, const char *pszValue );

	mod_t *FindModByName ( char *pszName );
	mod_t *FindModByID ( int id );
	mod_t *AddMod( mod_t **pList, mod_t *pMod );
	banned_sv_t *AddServer( banned_sv_t **pList, netadr_t *adr );
	void FreeMods();
	void FreeServers();
	int ModSendSize( mod_t *pMod );

// Query Responses
	void ServiceMessages();

	void GetPacketCommand (void);
	void PacketCommand();

	void Packet_RequestMod();
	void Packet_GetModBatch();
	BOOL PacketFilter();

// Utils
	BOOL MSG_ReadData(int nLength, void *nBuffer);
	char *MSG_ReadString (void);
	unsigned char MSG_ReadByte( void );
	unsigned short MSG_ReadShort( void );
	unsigned int MSG_ReadLong( void );

	void Sys_SendPacket (netadr_t *to, byte *data, int len);

	void Sys_Error (char *string, ...);
	void UTIL_VPrintf (char *msg, ...);  // Print if we are showing traffic:  i.e. verbose
	void UTIL_Printf (char *msg, ...);   // Print message unconditionally

	void UTIL_PrintMod( int idx, mod_t *mod );

// UI
	void MoveControls();
	void UpdateCount();
	
	int DoModal();
	int RunModalLoop(DWORD dwFlags);

	float	GetOutBytesProcessed();
	int		GetInTransactions();
	float	GetBytesProcessed();
	
	void	ResetTimer();
	double	GetStartTime();

// Dialog Data
	//{{AFX_DATA(CHLModMasterDlg)
	enum { IDD = IDD_HLMASTER_DIALOG };
	CListBox	m_modList;
	CStatic	m_statLoadOut;
	CStatic	m_statInTransactions;
	CStatic	m_statTime;
	CStatic	m_statLoad;
	CStatic	m_statActive;
	BOOL	m_bShowTraffic;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHLModMasterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Data
protected:
	fd_set		fdset;				// Master server file descriptor set.
	int			net_socket;			// Master server socket
	int			net_hostport;		// Port # this master is listening on
	netadr_t	net_local_adr;		// IP address of this master
	netadr_t	packet_from;		// Address of message we just received
	int			packet_length;		// Amount of data we received

	int			curtime;
	
	CString		m_strLocalIPAddress;	// Can be provided on cmd line if on dual nic machine

// Server lists
	mod_t		*modlist;			// Game servers being tracked by this master.
	banned_sv_t	*bannedips;			// ip addresses that are banned

// Send/Receive buffers
	char		reply[512];		// Outgoing buffer, limit to max UDP size
	byte		packet_data[65536]; // Incoming data
	int			msg_readcount;      // # of bytes of message we have parsed


	int			m_nUniqueID;        // Incrementing server ID counter for batch query
	
// Profiling vars ( per cycle )
	float		m_fCycleTime;       // Time of each cycle ( e.g., 1 min ).

	int			m_nInTransactions;  // # of requests we've parsed
	float		m_fBytesSent;       // # of outgoing bytes
	float		m_fBytesProcessed;  // # of incoming bytes
	double		m_tStartTime;       // Cycle Start time

// Log File
	FILE		*logfile;
	int			m_nCurrentDay;
	BOOL		m_bGenerateLogs;

// UI
	HICON		m_hIcon;
	BOOL		m_bShowPackets;
	
	// Generated message map functions
	//{{AFX_MSG(CHLModMasterDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnList();
	afx_msg void OnReload();
	afx_msg void OnDblclkList1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
