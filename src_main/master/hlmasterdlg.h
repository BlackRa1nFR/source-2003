// HLMasterDlg.h : header file
//

#define PEER_HEARTBEAT 1
#define PEER_SHUTDOWN  2
#define PEER_HEARTBEAT2 3

#define SERVER_HASH_SIZE 2019

#define MOD_HASH_SIZE 53

/////////////////////////////////////////////////////////////////////////////
// CHLMasterDlg dialog
class CHLMasterDlg : public CDialog
{
// Construction
public:
	void Packet_RequestsBatch( void );
	void Packet_WONMonitor( void );

	int VerToInt( const char *pszVersion );
	BOOL IsLessThan( const char *pszUser, const char *pszServer );
	BOOL IsEqualTo( const char *pszStringA, const char *pszStringB );
	void ParseVersion( void );
	CHLMasterDlg( int nMasterPort, 
					BOOL bGenerateLogs,
					CString strLocalIPAddress,
					CWnd* pParent = NULL);	// standard constructor
	~CHLMasterDlg();

// Initialization and shutdown
	void Master_Init();
	void NET_Init();
	void NET_GetLocalAddress (void);

	void CheckForNewLogfile();
	void OpenNewLogfile();

// Routines for handling serving out titan/auth/and master server lists
	sv_t *AddServer( sv_t **pList, netadr_t *adr );
	void ParseServers();
	sv_t *FindServerByAddress (netadr_t *adr);
	void FreeServers();

	void GenerateFakeServers( void );

// Mod info
	modsv_t *FindMod( const char *pszGameDir );
	void FreeMods( void );
	void ListMods( void );
	int SizeMod( modsv_t *p );
	int HashMod( const char *pszGameDir );
	void AdjustCounts( void );

// Query Responses
	void ServiceMessages();

	void GetPacketCommand (void);
	void PacketCommand();

	void Peer_Heartbeat( sv_t *sv );
	void Peer_Heartbeat2( sv_t *sv );
	void Peer_Shutdown( sv_t *sv );
	void Packet_GetPeerMessage( void );
	void Peer_GetHeartbeat( void );
	void Peer_GetHeartbeat2( void );
	void Peer_GetShutdown( void );

	void Packet_GetChallenge (void);
	void Packet_GetMasterServers ();
	void Packet_Heartbeat();
	void Packet_Heartbeat2();
	void Packet_Shutdown();
	void Packet_GetServers();
	void Packet_GetBatch();
	void Packet_GetBatch2();
	void Packet_GetModBatch();
	void Packet_GetModBatch2();
	void Packet_GetModBatch3();
	void Packet_Ping();
	void Packet_Motd();
	void Nack (char *comment, ...);

	void Packet_GetBatch_Responder( int truenextid, struct search_criteria_s *criteria );
	inline int ServerPassesCriteria( sv_t *server, struct search_criteria_s *criteria );

	BOOL PacketFilter();
	void RejectConnection(netadr_t *adr, char *pszMessage);
	void RequestRestart(netadr_t *adr);

// Utils
	unsigned char Nibble( char c );
	void HexConvert( char *pszInput, int nInputLength, unsigned char *pOutput );
	
	BOOL MSG_ReadData(int nLength, void *nBuffer);
	char *MSG_ReadString (void);
	unsigned char MSG_ReadByte( void );
	unsigned short MSG_ReadShort( void );
	unsigned int MSG_ReadLong( void );

	void Packet_Printf( int& curpos, char *fmt, ... );
	void Sys_SendPacket (netadr_t *to, byte *data, int len);

	void Sys_Error (char *string, ...);
	void UTIL_VPrintf (char *msg, ...);  // Print if we are showing traffic:  i.e. verbose
	void UTIL_Printf (char *msg, ...);   // Print message unconditionally

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

	void	TimeoutServers();

	int		CheckChallenge( int challenge, netadr_t *adr );

// Dialog Data
	//{{AFX_DATA(CHLMasterDlg)
	enum { IDD = IDD_HLMASTER_DIALOG };
	CEdit	m_hEditMOTD;
	CStatic	m_statLoadOut;
	CStatic	m_statInTransactions;
	CStatic	m_statTime;
	CStatic	m_statLoad;
	CStatic	m_statActiveInternet;
	CStatic	m_statActiveLan;
	CStatic	m_statActiveProxy;
	CStatic	m_statActiveTotal;
	BOOL	m_bShowTraffic;
	BOOL	m_bAllowOldProtocols;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHLMasterDlg)
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

	int			m_curtime;

	CString		m_strLocalIPAddress;	// Can be provided on cmd line if on dual nic machine
	
// Server lists
	sv_t		*authservers;		// WON auth servers in use.
	sv_t        *titanservers;		// WON directory servers in use
	sv_t        *masterservers;		// Other master servers in use.
	sv_t		*servers[ SERVER_HASH_SIZE ];			// Game servers being tracked by this master.
	sv_t		*bannedips;			// ip addresses that are banned

	modsv_t     *mods[ MOD_HASH_SIZE ];

	challenge_t	challenges[256][MAX_CHALLENGES];	// to prevent spoofed IPs from connecting

// Send/Receive buffers
	char		reply[1400];		// Outgoing buffer, limit to max UDP size
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
	//{{AFX_MSG(CHLMasterDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnList();
	afx_msg void OnReloadMotd();
	afx_msg void OnComputestats();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void FreeGameServers( void );
	int HashServer( netadr_t *address );

	char m_szHLVersion[32];
	char m_szCSVersion[32];
	char m_szTFCVersion[32];
	char m_szDMCVersion[32];
	char m_szOpForVersion[32];
	char m_szRicochetVersion[32];
	char m_szDODVersion[32];

	int  m_nUsers;
	int  m_nServerCount;
	int  m_nBotCount;

	int	 m_nLanServerCount;
	int	 m_nLanUsers;
	int  m_nLanBotCount;



};
