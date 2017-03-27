// StatusDlg.h : header file
//

#define MAX_Q_SOCKETS 64

#define REFRESH_INTERVAL ( 300.0 )

class CMod;
class CModList;
class CModInfoSocket;
class CQInfoSocket;
class CGraph;
class CProjectionWnd;
class CServerListSocket;
class CStatusWindow;
class CWarnDialog;
class CSchedulerDialog;

#include "modupdate.h"
#include <UtlVector.h>
#include <time.h>

/////////////////////////////////////////////////////////////////////////////
// CStatusDlg dialog
#include "BaseDlg.h"

class CStatusDlg : public CBaseDlg
{
// Construction
public:
	CStatusDlg(CWnd* pParent = NULL);	// standard constructor
	~CStatusDlg( void );

	// Save/restore mod lists
	
	enum
	{
		NORMAL = 0,
		IGNOREMODS,
		USETHRESHOLD,
		SPECIFICMOD
	};

	void SaveModUpdates( CModStats *l, char *pszFile, bool savefileloaded, bool clobberfile );
	CModStats	*LoadModUpdates( char *pszFile, bool markasfileloaded, int flags = NORMAL, float threshold = 0.0f, char const *modname = NULL );

	// Computations
	void ComputeProjections( void );
	void ComputeHistories( int sortkey, CModList *pList );
	int GetHistoryBounds( int sortkey, CModUpdate **start, CModUpdate **end );
	void GenerateFakeData( bool fulldataset );
	void ResetPeaks( void );
	void SetRate( void );
	void MoveControls(  void );

	// Status messages
	void ClearStartMsg( void );
	void PrintStatusMsg( char *fmt, ... );
	void PrintStartMsg( void );

	void StartPing();

	void ChangeDays( time_t oldDate, time_t newDate );
	void CheckForNewDay (void);
	void RecomputeList( BOOL SkipPeaks = FALSE );

	int RMLPreIdle(void);
	void RMLIdle(void);
	void RMLPump(void);

	void StartRequest();
	void FinishRequest( long time_adjust = 0, BOOL SkipPeaks = FALSE, BOOL recomputeinfo = TRUE );
	void ContinueRequest();

	time_t GetTime( void );
	char const *GetTimeAsString( time_t *t = NULL );

	void SortUpdates( CModStats *list );
	void RemoveDuplicates( CModStats *stats );
	void MergeLists( CModStats *inlist1, CModStats *inlist2 );
	CModStats *RemoveItemsForDay( CModStats *original, time_t t, bool includinganythingbeforeday );
	CModStats *RemoveArchivedData( CModStats *original );

	char const *FileNameForTime( time_t t );
	bool IsInSameDay( time_t t1, time_t t2 );
	void LoadDateSpan( CModStats *list, time_t start, time_t end );
	bool IsFileDataForDateInList( CModStats *list, time_t t );

	time_t FindEarliestTime( CModStats *list );

	void	RedrawEverything( void );

	void	ResetHistory( void );

	char const *GetProjectionString( void );
	char const *GetPeakString( float threshold );

	bool	GenerateReport( void );
	void	EmailReport( char const *emailaddress );

// Dialog Data
	//{{AFX_DATA(CStatusDlg)
	enum { IDD = IDD_STATUS };
	CButton	m_btnEmail;
	CTreeCtrl	m_modList;
	CStatic	m_statPeak;
	CButton	m_btnWeek;
	CButton	m_btnMonth;
	CButton	m_btnHour;
	CButton	m_btnDay;
	CTreeCtrl	m_modPeak;
	CStatic	m_statRate;
	CButton	m_btnRefresh;
	CStatic	m_statRequest;
	CSpinButtonCtrl	m_spinRate;
	CStatic	m_statStatus;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatusDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CStatusDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDeltaposRate(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnRefresh();
	afx_msg void OnDay();
	afx_msg void OnHour();
	afx_msg void OnMonth();
	afx_msg void OnWeek();
	afx_msg void OnLoad();
	afx_msg void OnMemory();
	afx_msg void OnClosearchived();
	afx_msg void OnQuit();
	afx_msg void OnOptions();
	afx_msg void OnEmail();
	afx_msg void OnAddemail();
	afx_msg void OnWarnDialog();
	afx_msg void OnSchedulerDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	time_t		m_tmCurrentDay;
	int			m_nDaysInMemory;

	// Refresh interval
	double		m_fInterval;
	
	// Refresh status
	BOOL		m_bRequesting;

	// Raw refresh results
	CModList	m_List;

	// List of all mods seen for a game
	CModList	m_AllMods;

	// Global stats lists
	CModStats	m_Stats;

	// Has system init completed?
	BOOL		m_bInitialized;

	// Time of last update
	double		m_fLastUpdate;

	// Time for next update
	double		m_fNextUpdate;

	// Time request was started
	double		m_requestStarted;

	double		m_flLastDateCheck;
	
	int			m_nLastActive;

	// Current sort key for history windows
	int			m_nSortKey;

	// Session number for this server
	int			m_nSessionNumber;

	// Address of remote server when acting as a client
	CString		m_strAddress;

	// UI elements
	HICON		m_hIcon;
	CFont		m_hSmallFont;
	CFont		m_hNormalFont;

	CProjectionWnd *m_pProjection;

	CGraph		*m_pServers;
	CGraph		*m_pPlayers;

	CStatusWindow	*m_pStatus;

	// Control positions
	CRect		m_rcPlayers;
	CRect		m_rcServers;
	CRect		m_rcRate;
	CRect		m_rcRefresh;

	// Sockets for I/O ops

	CModInfoSocket		*m_pModInfo;

	CString		m_strMaster;

	time_t		m_startTime;

	int m_iMinPlayersLevel;
	char *m_sWarnEmail;

	struct times 
	{
		time_t nextTime; // the time to next run this command
		time_t dayOffset; // the time (in sec) to run this command from the start of the day (i.e 12am
	};

	CUtlVector<struct times> m_ReportTimes;
};
