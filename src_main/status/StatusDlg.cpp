// StatusDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Status.h"
#include <assert.h>
#include <afxcmn.h>
#include <direct.h>
#include <math.h>
#include "StatusDlg.h"
#include "MessageBuffer.h"
#include "HLAsyncSocket.h"
#include "Mod.h"
#include "ModInfoSocket.h"
#include "Graph.h"
#include "ProjectionWnd.h"
#include "status_protocol.h"
#include "util.h"
#include "status_colors.h"
#include "LoadDialog.h"
#include "StatusWindow.h"
#include "OptionsDlg.h"
#include "EmailDialog.h"
#include "EmailManager.h"
#include "WarnDialog.h"
#include "SchedulerDialog.h"
#include "Scheduler.h"
#include <sys/types.h>
#include <sys/stat.h>

extern CSVTestApp theApp;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SECONDS_PER_DAY 86400
#define DATADIR "data"
#define TEMPDATA "data\\unsaved.txt"

#define TIMER_NUMBER 1

static char g_szTempFile[] = TEMPDATA;

static bool continueIdle = true;

char *gpszCmdLine = NULL;

static char const *attachment = "c:\\status_email.txt";

CStatusDlg *g_pDlg = NULL;

enum
{
	SORT_HOUR = 0,
	SORT_DAY,
	SORT_WEEK,
	SORT_MONTH
};

/*
bool CheckList( CModStats *list )
{
	time_t lasttime = (time_t)0x7fffffff;

	CModUpdate *p = list->list;
	while ( p )
	{
		if ( p->realtime > lasttime )
		{
			assert( 0 );
		}

		lasttime = p->realtime;

		p = p->next;
	}

	return true;
}
*/

/*
==============================
PrintStatusMsg

==============================
*/
void PrintStatusMsg( char *fmt, ... )
{
	char string[ 1024 ];
	va_list argptr;
	
	if ( !g_pDlg )
		return;

	va_start( argptr, fmt );
	vsprintf( string, fmt, argptr );
	va_end( argptr );

	g_pDlg->PrintStatusMsg( string );
}

time_t GlobalGetTime( void )
{
	if ( !g_pDlg )
		return time( NULL );

	return g_pDlg->GetTime();
}

/*
============
COM_CreatePath

Only used for CopyFile
============
*/
void    COM_CreatePath (char *path)
{
	char    *ofs;
	char old;
	
	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{       // create the directory
			old = *ofs;
			*ofs = 0;
			_mkdir (path);
			*ofs = old;
		}
	}
}

/*
==============================
CheckParm

==============================
*/
char *CheckParm(const char *psz, char **ppszValue = NULL )
{
	static char sz[128];
	char *pret;

	if (!gpszCmdLine)
		return NULL;

	pret = strstr(gpszCmdLine, psz);

	// should we return a pointer to the value?
	if (pret && ppszValue)
	{
		char *p1 = pret;
		*ppszValue = NULL;

		while ( *p1 && (*p1 != 32))
			p1++;

		if (p1 != 0)
		{
			char *p2 = ++p1;

			for (int i = 0; i < 128; i++)
			{
				if ( !*p2 || (*p2 == 32))
					break;
				sz[i] = *p2++;
			}

			sz[i] = 0;
			*ppszValue = &sz[0];		
		}	
	}

	return pret;
}

static double		pfreq;
static double		curtime = 0.0;
static double		lastcurtime = 0.0;
static int			lowshift;

static double		scaletime = 1.0f;

time_t CStatusDlg::GetTime( void )
{
	time_t temp = time( NULL );

	double dt = temp - m_startTime;
	dt *= scaletime;

	return (time_t)( m_startTime + dt );
}

/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	static int			sametimecount;
	static unsigned int	oldtime;
	static int			first = 1;
	LARGE_INTEGER		PerformanceCount;
	unsigned int		temp, t2;
	double				time;

	QueryPerformanceCounter (&PerformanceCount);

	temp = ((unsigned int)PerformanceCount.LowPart >> lowshift) |
		   ((unsigned int)PerformanceCount.HighPart << (32 - lowshift));

	if (first)
	{
		oldtime = temp;
		first = 0;
	}
	else
	{
	// check for turnover or backward time
		if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000))
		{
			oldtime = temp;	// so we can't get stuck
		}
		else
		{
			t2 = temp - oldtime;

			time = (double)t2 * pfreq;
			oldtime = temp;

			curtime += time;

			if (curtime == lastcurtime)
			{
				sametimecount++;

				if (sametimecount > 100000)
				{
					curtime += 1.0;
					sametimecount = 0;
				}
			}
			else
			{
				sametimecount = 0;
			}

			lastcurtime = curtime;
		}
	}

    return curtime * scaletime;
}


/*
==============================
Init_Timer

==============================
*/
void Init_Timer()
{
// Set up the clock
	// Timer
	unsigned int	lowpart, highpart;
	LARGE_INTEGER	PerformanceFreq;

	if (!QueryPerformanceFrequency (&PerformanceFreq))
	{
		exit(-1);
	}

// get 32 out of the 64 time bits such that we have around
// 1 microsecond resolution
	lowpart = (unsigned int)PerformanceFreq.LowPart;
	highpart = (unsigned int)PerformanceFreq.HighPart;
	lowshift = 0;

	while (highpart || (lowpart > 2000000.0))
	{
		lowshift++;
		lowpart >>= 1;
		lowpart |= (highpart & 1) << 31;
		highpart >>= 1;
	}

	pfreq = 1.0 / (double)lowpart;

	Sys_DoubleTime();
	Sys_DoubleTime();
}

/////////////////////////////////////////////////////////////////////////////
// CStatusDlg dialog

CStatusDlg::CStatusDlg(CWnd* pParent /*=NULL*/)
	: CBaseDlg(CStatusDlg::IDD, pParent)
{
	g_pDlg = this;

	m_strMaster = "";

	Init_Timer();

	m_startTime = time( NULL );

	m_flLastDateCheck = 0.0f;

	srand( clock() );
	srand( clock() );

	m_strAddress = "127.0.0.1";

	m_nSessionNumber = rand();

	//{{AFX_DATA_INIT(CStatusDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bInitialized = FALSE;
	m_fLastUpdate = 0.0;

	m_bRequesting = FALSE;

	m_requestStarted = 0.0;
	m_fNextUpdate = 0.0;

	m_pServers = NULL;
	m_pPlayers = NULL;
	m_pStatus = NULL;

	m_pProjection = FALSE;

	m_nSortKey = SORT_MONTH;
}

/*
==============================
~CStatusDlg

==============================
*/
CStatusDlg::~CStatusDlg( void )
{

	continueIdle = false;

	GetEmailManager()->Shutdown();

	if ( !CheckParm( "-fake" ) )
	{
		SaveModUpdates( &m_Stats, g_szTempFile, false, true );
	}

	OnMemory();

	ResetHistory();

	m_Stats.Clear();

	m_List.Reset();
	m_AllMods.Reset();

	OnMemory();

	delete m_pProjection;
	delete m_pServers;
	delete m_pPlayers;
	delete m_pStatus;
	delete m_sWarnEmail;
}

#define DAYCHANGEDCHECKINTERVAL 15.0f
void CStatusDlg::CheckForNewDay (void)
{
	float curtime = (float) ( Sys_DoubleTime() / scaletime );
	if ( curtime < m_flLastDateCheck + DAYCHANGEDCHECKINTERVAL )
		return;

	m_flLastDateCheck = curtime;

	time_t t = GetTime();

	if ( IsInSameDay( t, m_tmCurrentDay ) )
		return;

	ChangeDays( m_tmCurrentDay, t );

	m_tmCurrentDay = t;
}



/*
==============================
DoDataExchange

==============================
*/
void CStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatusDlg)
	DDX_Control(pDX, IDC_EMAIL, m_btnEmail);
	DDX_Control(pDX, IDC_MODTREE, m_modList);
	DDX_Control(pDX, IDC_STATIC_PEAK, m_statPeak);
	DDX_Control(pDX, IDC_WEEK, m_btnWeek);
	DDX_Control(pDX, IDC_MONTH, m_btnMonth);
	DDX_Control(pDX, IDC_HOUR, m_btnHour);
	DDX_Control(pDX, IDC_DAY, m_btnDay);
	DDX_Control(pDX, IDC_MODPEAK, m_modPeak);
	DDX_Control(pDX, IDC_STATIC_RATE, m_statRate);
	DDX_Control(pDX, IDC_REFRESH, m_btnRefresh);
	DDX_Control(pDX, IDC_STAT_REQUEST, m_statRequest);
	DDX_Control(pDX, IDC_RATE, m_spinRate);
	//}}AFX_DATA_MAP
}

/*
==============================
RMLPreIdle

==============================
*/

int CStatusDlg::RMLPreIdle(void)
{
	if ( !continueIdle )
		return 0;

	double curtime;

	curtime = Sys_DoubleTime();
	if ( curtime >= m_fNextUpdate )
	{
		m_fNextUpdate = curtime + m_fInterval;
		StartPing();
	}

	ContinueRequest();

	CheckForNewDay();

	GetEmailManager()->Update( curtime );


	return 0;
}

/*
==============================
RMLIdle

==============================
*/
void CStatusDlg::RMLIdle(void)
{
	RMLPreIdle();
}

/*
==============================
RMLPump

==============================
*/
void CStatusDlg::RMLPump(void)
{
	RMLPreIdle();
}

BEGIN_MESSAGE_MAP(CStatusDlg, CBaseDlg)
	//{{AFX_MSG_MAP(CStatusDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_NOTIFY(UDN_DELTAPOS, IDC_RATE, OnDeltaposRate)
	ON_WM_MOVE()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_BN_CLICKED(IDC_DAY, OnDay)
	ON_BN_CLICKED(IDC_HOUR, OnHour)
	ON_BN_CLICKED(IDC_MONTH, OnMonth)
	ON_BN_CLICKED(IDC_WEEK, OnWeek)
	ON_COMMAND(IDC_FILELOAD, OnLoad)
	ON_COMMAND(IDM_MEMORY, OnMemory )
	ON_COMMAND(IDC_CLOSEARCHIVED, OnClosearchived)
	ON_COMMAND(IDC_QUIT, OnQuit)
	ON_COMMAND(IDM_OPTIONS, OnOptions)
	ON_BN_CLICKED(IDC_EMAIL, OnEmail)
	ON_COMMAND(IDM_ADDEMAIL, OnAddemail)
	ON_COMMAND(IDM_WARNDIALOG, OnWarnDialog)
	ON_COMMAND(IDD_SCHEDULER, OnSchedulerDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatusDlg message handlers
/*
==============================
OnInitDialog

==============================
*/
BOOL CStatusDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pStatus = new CStatusWindow( this );
	m_pStatus->Create( WS_VISIBLE | WS_CHILD );

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	gpszCmdLine = GetCommandLine();

	m_fInterval = REFRESH_INTERVAL;

	char *p;

	if ( CheckParm( "-generate", &p ) && p )
	{
		scaletime = max( atof( p ), 0.1f );
	}

	if ( CheckParm( "-interval", &p ) && p )
	{
		m_fInterval = atof( p );
		if ( m_fInterval < 1.0 )
			m_fInterval = 1.0;
	}

	time_t t = GetTime();
	m_tmCurrentDay = t;
	if ( !CheckParm( "-fake" ) )
	{
		CModStats *loaded = LoadModUpdates( g_szTempFile, false );
		if ( loaded )
		{
			// Don't use it if it's after current date ( with 15 minute fudge factor ) for some reason
			if ( loaded->list && loaded->list->realtime < ( m_tmCurrentDay + 900.0f ) )
			{
				MergeLists( &m_Stats, loaded );
			}
			delete loaded;
		}
	}

	// If there was some left over data, see if it's from a previous day and go ahead and save it out if needed
	if ( m_Stats.list )
	{
		time_t earliest = FindEarliestTime( &m_Stats );
		ChangeDays( earliest, m_tmCurrentDay );
	}

	m_fLastUpdate = Sys_DoubleTime();
	m_fNextUpdate = m_fLastUpdate + m_fInterval;

	m_bInitialized = TRUE;

	m_hSmallFont.CreateFont(
		 -11	             // H
		, 0   					         // W
		, 0								 // Escapement
		, 0							     // Orient
		, FW_DONTCARE					 // Wt.  (BOLD)
		, 0								 // Ital.
		, 0             				 // Underl.
		, 0                 			 // SO
		, DEFAULT_CHARSET 	    		 // Charset
		, OUT_DEFAULT_PRECIS				 // Out prec.
		, CLIP_DEFAULT_PRECIS			 // Clip prec.
		, ANTIALIASED_QUALITY   			     // Qual.
		, DEFAULT_PITCH  | FF_DONTCARE   // Pitch and Fam.
		, "Courier New" );

	m_hNormalFont.CreateFont(
		 14			             // H
		, 0   					         // W
		, 0								 // Escapement
		, 0							     // Orient
		, FW_DONTCARE					 // Wt.  (BOLD)
		, 0								 // Ital.
		, 0             				 // Underl.
		, 0                 			 // SO
		, DEFAULT_CHARSET 	    		 // Charset
		, OUT_DEFAULT_PRECIS				 // Out prec.
		, CLIP_DEFAULT_PRECIS			 // Clip prec.
		, ANTIALIASED_QUALITY   			     // Qual.
		, DEFAULT_PITCH  | FF_DONTCARE   // Pitch and Fam.
		, "Courier New" );

	m_modList.SetFont( &m_hNormalFont, TRUE );
	m_modPeak.SetFont( &m_hSmallFont, TRUE );

	m_modPeak.SetBkColor( CLR_BG );
	m_modPeak.SetTextColor( RGB( 50, 150, 200 ) );

	m_modList.SetBkColor( CLR_BG );
	m_modList.SetTextColor( RGB( 50, 150, 200 ) );

	unsigned int nServers, nPlayers;

	CString strS, strP, strMaster, strDaysToLoad,sMinPlayers,sWarnEmail,sScheduleTimes;
	strS = theApp.GetProfileString( "Status", "Servers", "30000" );
	strP = theApp.GetProfileString( "Status", "Players", "150000" );
	strMaster = theApp.GetProfileString( "Status", "Master", "half-life.west.won.net:27010" );
	strDaysToLoad = theApp.GetProfileString( "Status", "DaysKeptInMemory", "30" );

	sMinPlayers = theApp.GetProfileString( "Status", "MinPlayers", "500" );
	sWarnEmail = theApp.GetProfileString( "Status", "WarnEmail", "" );

	m_strAddress = theApp.GetProfileString( "Status", "Server", "127.0.0.1" );

	char *padr;
	if ( CheckParm( "-address", &padr ) )
	{
		m_strAddress = padr;

		theApp.WriteProfileString( "Status", "Server", m_strAddress );
	}

	char *overrideS, *overrideP, *overrideMaster, *overrideDays;

	if ( CheckParm( "-sv", &overrideS ) && overrideS )
	{
		strS = overrideS;
	}

	if ( CheckParm( "-pl", &overrideP ) && overrideP )
	{
		strP = overrideP;
	}

	if ( CheckParm( "-master", &overrideMaster ) && overrideMaster )
	{
		strMaster = overrideMaster;
	}

	if ( CheckParm( "-days", &overrideDays ) && overrideDays )
	{
		strDaysToLoad = overrideDays;
	}

	if ( atoi( strDaysToLoad ) < 1 )
	{
		strDaysToLoad = "1";
	}

	if ( CheckParm( "-minpl", &overrideS ) && overrideS )
	{
		sMinPlayers = overrideS;
	}

	if ( CheckParm( "-warn", &overrideS ) && overrideS )
	{
		sWarnEmail = overrideS;
	}

	theApp.WriteProfileString( "Status", "Servers", strS );
	theApp.WriteProfileString( "Status", "Players", strP );
	theApp.WriteProfileString( "Status", "Master", strMaster );
	theApp.WriteProfileString( "Status", "DaysKeptInMemory", strDaysToLoad );
	theApp.WriteProfileString( "Status", "MinPlayers", sMinPlayers );
	theApp.WriteProfileString( "Status", "WarnEmail", sWarnEmail );

	m_nDaysInMemory = atoi( strDaysToLoad );
	m_strMaster = strMaster;

	LoadDateSpan( &m_Stats, m_tmCurrentDay - SECONDS_PER_DAY * m_nDaysInMemory, m_tmCurrentDay );

	nServers = (unsigned int)atoi( (char *)(LPCSTR)strS );
	nPlayers = (unsigned int)atoi( (char *)(LPCSTR)strP );

	m_iMinPlayersLevel = (unsigned int)atoi( (char *)(LPCSTR)sMinPlayers );
	m_sWarnEmail = new char[ strlen(sWarnEmail) + 1];
	assert( m_sWarnEmail );
	strcpy( m_sWarnEmail, sWarnEmail );

	m_pServers = new CGraph( "Servers" );
	m_pServers->Create( WS_VISIBLE | WS_CHILD, this, 109 );
	m_pServers->SetData( &m_Stats, nServers, CGraph::GRAPHTYPE_SERVERS );

	m_pPlayers = new CGraph( "Players" );
	m_pPlayers->Create( WS_VISIBLE | WS_CHILD, this, 110 );
	m_pPlayers->SetData( &m_Stats, nPlayers, CGraph::GRAPHTYPE_PLAYERS );
	
	m_pProjection = new CProjectionWnd();
	m_pProjection->Create( WS_VISIBLE | WS_CHILD, this, 111 );

	SetRate();

	MoveControls();

	ShowWindow( SW_MAXIMIZE );

	m_btnHour.SetCheck( BST_CHECKED );
	m_btnHour.InvalidateRect( NULL );
	m_btnHour.UpdateWindow();

	// Start requests now
	StartPing();

	ResetHistory();

	GetEmailManager()->Init( this );

	GetScheduler()->RemoveAllTimes();
	sScheduleTimes = theApp.GetProfileString( "Status", "ScheduleTimes", "" );
	char *ascSched = new char[sScheduleTimes.GetLength()+1];
	char *start = ascSched;
	strcpy( ascSched, sScheduleTimes );

	if ( strlen(ascSched) > 0 )
	{
		while( strchr(ascSched, ';') )
		{
			char *sNextTime = strchr(ascSched, ';');
			if ( sNextTime )
			{
				*sNextTime = '\0';
			}
			GetScheduler()->AddTime(ascSched);
		
			struct times newTime;
			newTime.nextTime = 0;

			int hour=0,min=0,sec=0;
			sscanf( ascSched, "%d:%d:%d", &hour, &min, &sec );
			newTime.dayOffset = hour*60*60 + min*60 + sec;

			m_ReportTimes.AddToTail( newTime );

			ascSched = sNextTime+1;
		}
	}
	delete start;

	OnTimer( 0 ); // the value 0 makes it initialize the times


	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CStatusDlg::OnPaint() 
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

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CStatusDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

#define DRAW_MARGIN 5
#define GRAPH_Y 150
#define GRAPH_PLTOP ( 10000.0 )
#define GRAPH_SVTOP ( 2000.0 )

int CALLBACK ModCompareFunc(LPARAM lParam1, LPARAM lParam2, 
   LPARAM lParamSort)
{
	CMod *p1, *p2;

	p1 = ( CMod * )lParam1;
	p2 = ( CMod * )lParam2;

	if ( !p1 || !p2 )
	{
		if ( !p1 && p2 )
			return -1;
		else if ( p1 && !p2 )
			return 1;
		return 0;
	}

	return ( stricmp( p1->GetGameDir(), p2->GetGameDir() ) );
}

char *game_name =
{
	"Half-Life",
};

/*
==============================
CompareTime

==============================
*/
int CompareTime( struct tm *t1, struct tm *t2, int sortkey )
{
	int week1, week2;

	if ( t1->tm_year != t2->tm_year )
		return 0;

	if ( t1->tm_mon != t2->tm_mon )
		return 0;

	if ( sortkey == SORT_MONTH )
		return 1;

	week1 = t1->tm_mday / 7;
	week2 = t2->tm_mday / 7;

	if ( week1 != week2 )
		return 0;

	if ( sortkey == SORT_WEEK )
		return 1;

	if ( t1->tm_yday != t2->tm_yday )
		return 0;

	if ( sortkey == SORT_DAY )
		return 1;

	if ( t1->tm_hour != t2->tm_hour )
		return 0;

	return 1;
}

/*
==============================
GetHistoryBounds

==============================
*/
#define SECSPERHOUR (60.0*60.0)
#define SECSPERDAY  (24.0*SECSPERHOUR)
#define SECSPERWEEK (7.0*SECSPERDAY)
#define SECSPERMONTH (30.0*SECSPERDAY)

int CStatusDlg::GetHistoryBounds( int sortkey, CModUpdate **start, CModUpdate **end )
{
	// Get current time
	time_t current;
	struct tm tv;
//	struct tm lt;
	int i = 0;

	current = GetTime();
	tv = *localtime( &current );

	CModUpdate *p;

	p = m_Stats.list;

	*start	= p;
	*end = p;

	double maxdelta = SECSPERHOUR;
	switch ( sortkey )
	{
	default:
	case SORT_HOUR:
		maxdelta = SECSPERHOUR;
		break;
	case SORT_DAY:
		maxdelta = SECSPERDAY;
		break;
	case SORT_WEEK:
		maxdelta = SECSPERWEEK;
		break;
	case SORT_MONTH:
		maxdelta = SECSPERMONTH;
		break;
	}

	while ( p )
	{
		*end = p;
		
		double dt = difftime( current, p->realtime );

		if ( dt >= maxdelta )
			break;

		//lt = *localtime( &p->realtime );
		//if ( !CompareTime( &lt, &tv, sortkey ) )
		//	break;
		p = p->next;
		i++;
	}

	return i;
}

char const *CStatusDlg::GetTimeAsString( time_t *t /* = NULL */ )
{
	static char sz[ 4 ][ 512 ];
	static int current;
	static char *dow[] =
	{
		{ "Su" },
		{ "M" },
		{ "Tu" },
		{ "W" },
		{ "Th" },
		{ "F" },
		{ "S" },
	};

	time_t curtime;

	if ( !t )
	{
		curtime = GetTime();
		t = &curtime;
	}

	struct tm *tv = localtime( t );

	current = ( current + 1 ) % 4;

	char *out = sz[ current ];

	out[ 0 ] = 0;

	sprintf( out, "%s %i/%02i %i:%02i",
		dow[ tv->tm_wday ], tv->tm_mon+1, tv->tm_mday, tv->tm_hour, tv->tm_min );

	return out;
}
/*
==============================
ComputeHistories

==============================
*/
void CStatusDlg::ComputeHistories( int sortkey, CModList *pList )
{
	CModUpdate *st, *ed, *iter;
	CModList *l;
	CMod *p;
	char sz[ 1024 ];
	char szTitle[ 256 ];
	TVITEM curItem;
	HTREEITEM hPeakParentItem;
	HTREEITEM hPeakGameItem;
	TVINSERTSTRUCT tvins;

	CModUpdate *peakIterPlayers = NULL, *peakIterServers = NULL, *peakIterBots = NULL,*peakIterBotsServers = NULL;

	int count;

	ResetPeaks();

	m_modPeak.DeleteAllItems();
	hPeakParentItem = m_modPeak.GetRootItem();

	memset( &curItem, 0, sizeof( curItem ) );
	curItem.mask= TVIF_TEXT;

	count =	GetHistoryBounds( sortkey, &st, &ed );

	l = pList;

	tvins.hParent = hPeakParentItem;
	tvins.hInsertAfter = TVI_LAST;
	hPeakGameItem = m_modPeak.InsertItem( &tvins );

	int nPeakServers = 0;
	int nPeakPlayers = 0;
	int nPeakBots = 0;
	int nPeakBotsServers = 0;

	int i;
	for ( i = 0; i < l->getCount(); i++ )
	{
		p = l->getMod( i );

		CModUpdate *peakServers = NULL, *peakPlayers = NULL, *peakBots = NULL, *peakBotsServers = NULL ;

		iter = st;
		while ( iter )
		{
			CMod *mod = iter->Mods.FindMod( p->GetGameDir() );
			if ( mod )
			{
				if ( mod->total.players > p->pl[ sortkey ] )
				{
					p->pl[ sortkey ] = mod->total.players;
					peakPlayers = iter;
				}
				if ( mod->total.servers > p->sv[ sortkey ] )
				{
					p->sv[ sortkey ] = mod->total.servers;
					peakServers = iter;
				}			
				if ( mod->total.bots > p->bots[ sortkey ] )
				{
					p->bots[ sortkey ] = mod->total.bots;
					peakBots = iter;
				}	
				if ( mod->total.bots_servers > p->bots_servers[ sortkey ] )
				{
					p->bots_servers[ sortkey ] = mod->total.bots_servers;
					peakBotsServers = iter;
				}
			}

			if ( iter->total.players > nPeakPlayers )
			{
				nPeakPlayers = iter->total.players;
				peakIterPlayers = iter;
			}

			if ( iter->total.servers > nPeakServers )
			{
				nPeakServers = iter->total.servers;
				peakIterServers = iter;
			}		
			
			if ( iter->total.bots > nPeakBots )
			{
				nPeakBots = iter->total.bots;
				peakIterBots = iter;
			}
			if ( iter->total.bots_servers > nPeakBotsServers )
			{
				nPeakBotsServers = iter->total.bots_servers;
				peakIterBotsServers = iter;
			}

			if ( iter == ed )
				break;
			
			iter = iter->next;
			if ( !iter )
				break;
		}

		sprintf( sz, "%-10s: %6i sv %6i pl %6i bt %6i bs", p->GetGameDir(), p->sv[ sortkey ], p->pl[ sortkey ], p->bots[ sortkey ],  p->bots_servers[ sortkey ]);

		if ( peakServers && peakPlayers )
		{
			char outstr[ 512 ];
			sprintf( outstr, " -- %s & %s", GetTimeAsString( &peakServers->realtime ), GetTimeAsString( &peakPlayers->realtime ) );
			strcat( sz, outstr );
		}

		if ( p->sv[ sortkey ] || p->pl[ sortkey ] || p->bots[ sortkey ] || p->bots_servers[ sortkey ]  )
		{
			curItem.pszText = sz;
			tvins.hParent	= hPeakGameItem;
			tvins.item		= curItem;
			m_modPeak.InsertItem( &tvins );
		}

//			nPeakServers += p->sv[ sortkey ];
//			nPeakPlayers += p->pl[ sortkey ];
	}

	sprintf( szTitle, "%-10s: %6i sv, %6i pl %6i bt %6i bs", game_name, nPeakServers, nPeakPlayers, nPeakBots, nPeakBotsServers );

	if ( peakIterServers && peakIterPlayers )
	{
		char outstr[ 512 ];
		sprintf( outstr, " -- %s & %s", GetTimeAsString( &peakIterServers->realtime ), GetTimeAsString( &peakIterPlayers->realtime ));
		strcat( szTitle, outstr );
	}

	curItem.mask= TVIF_TEXT | TVIF_HANDLE;
	curItem.hItem = hPeakGameItem;
	curItem.pszText = szTitle;

	m_modPeak.SetItem( &curItem );
}

/*
==============================
RecomputeList

==============================
*/
void CStatusDlg::RecomputeList( BOOL SkipPeaks )
{
	TVITEM curItem;
	HTREEITEM hParentItem;
	HTREEITEM hGameItem;
	TVINSERTSTRUCT tvins;
	int nCount;

	// Clear out the tree
	m_modList.DeleteAllItems();

	// Find parent to this item
	hParentItem = m_modList.GetRootItem();

	int nPlayers = 0;
	int nServers = 0;
	int nBots	 = 0;
	int nBotsServers	 = 0;

	int index = 0;
	
	char szTitle[ 256 ];
	char sz[ 1024 ];

	nCount = m_AllMods.getCount();;

	sprintf( szTitle, "%s:  %i servers, %i players", game_name, 0, 0 );

	memset( &curItem, 0, sizeof( curItem ) );
	curItem.mask= TVIF_TEXT;
	curItem.pszText = szTitle;

	memset( &tvins, 0, sizeof( tvins ) );
	tvins.hParent = hParentItem;
	tvins.hInsertAfter = TVI_LAST;
	tvins.item = curItem;

	hGameItem = m_modList.InsertItem( &tvins );

	int j;

	for ( j = 0; j < m_AllMods.getCount(); j++ )
	{
		CMod *p;
		p = m_AllMods.getMod( j );

		sprintf( sz, "%-16s:  %5i sv %5i pl %5i bt %5i bs",
					p->GetGameDir(),
					p->total.servers,
					p->total.players,
					p->total.bots,
					p->total.bots_servers);

		curItem.pszText		= sz;
		tvins.hParent		= hGameItem;
		tvins.item			= curItem;
		m_modList.InsertItem( &tvins );
	
		nServers += p->total.servers;
		nPlayers += p->total.players;
		nBots    += p->total.bots;
		nBotsServers    += p->total.bots_servers;
	}

	sprintf( szTitle, "%s:  %i servers, %i players %i bots %i bs", game_name, nServers, nPlayers, nBots, nBotsServers );

	curItem.mask= TVIF_TEXT | TVIF_HANDLE;
	curItem.hItem = hGameItem;
	curItem.pszText = szTitle;

	m_modList.SetItem( &curItem );

	if ( !SkipPeaks )
	{
		ComputeHistories( m_nSortKey, &m_AllMods );
		ComputeProjections();
	}

}

/*
==============================
StartPing

==============================
*/
void CStatusDlg::StartPing()
{
	static int firsttime = 1;

	if ( firsttime && CheckParm( "-fake" ) )
	{
		GenerateFakeData( true );
		return;
	}

	if ( CheckParm( "-generate" ) )
	{
		GenerateFakeData( false );
		return;
	}

	firsttime = 0;

	StartRequest();
}

/*
==============================
OnOK

==============================
*/
void CStatusDlg::OnOK() 
{
	CDialog::OnOK();
}

/*
==============================
StartRequest

==============================
*/
void CStatusDlg::StartRequest()
{
	if ( ( Sys_DoubleTime() - m_requestStarted ) > 5.0 )
	{
		if ( m_bRequesting )
		{
			FinishRequest();

			RedrawEverything();
		}
	}

	if ( m_bRequesting )
		return;

	m_pStatus->ColorPrintf( RGB( 0, 63, 150 ), "Starting request...\n" );

	m_requestStarted = Sys_DoubleTime();

	m_List.Reset();

	m_bRequesting  = TRUE;
	m_pModInfo = new CModInfoSocket();
	int port;
	char adr[ 512 ];
	NET_SplitAddress( (char *)(LPCSTR)m_strMaster, adr, &port, 27010 );
	m_pModInfo->StartModRequest( &m_List, adr, port );

	PrintStartMsg();
}

/*
==============================
ContinueRequest

==============================
*/
void CStatusDlg::ContinueRequest()
{
	if ( !m_bRequesting )
		return;

	if ( !m_pModInfo )
	{
		m_bRequesting = FALSE;
	}
	
	if ( m_bRequesting && m_pModInfo && !m_pModInfo->CheckForResend() )
	{
		FinishRequest();

		RedrawEverything();
		//ClearStartMsg();
		if ( m_pModInfo )
		{
			delete m_pModInfo;
			m_pModInfo = NULL;
		}
	}

	if ( ( Sys_DoubleTime() - m_requestStarted ) > 1.0 )
	{
		//ClearStartMsg();
	}
}

/*
==============================
FinishRequest

==============================
*/
void CStatusDlg::FinishRequest( long time_adjust, BOOL SkipPeaks, BOOL recomputeinfo )
{
	m_bRequesting = FALSE;

	CModUpdate *pupdate;
	pupdate = new CModUpdate;

	CModStats *temp = new CModStats;
	temp->count = 1;
	temp->list = pupdate;

	pupdate->next = NULL;
	pupdate->realtime			= GetTime() + time_adjust;

	pupdate->m_bLoadedFromFile = false;

	CMod *p;
	int j;
	
	int c = m_List.getCount();
	for ( j = 0; j < c; j++ )
	{
		p = m_List.getMod( j );

		pupdate->Add( p );
	}

	
	int nmods = m_List.getCount();
	for ( j = nmods - 1; j >= 0; j-- )
	{
		p = m_List.getMod( j );
		m_List.RemoveMod( p );
		pupdate->Mods.AddMod( p );
	}
	m_List.Reset();

	for ( j = 0 ; j < m_AllMods.getCount(); j++ )
	{
		// Transfer mod data to global list
		p = m_AllMods.getMod( j );
		p->Reset();
	}
	CMod *all;

	for ( j = 0 ; j < pupdate->Mods.getCount(); j++ )
	{
		p = pupdate->Mods.getMod( j );

		all = m_AllMods.FindMod( p->GetGameDir() );
		if ( !all )
		{
			all = p->Copy();

			m_AllMods.AddMod( all );
		}
		else
		{
			all->Add( p );
		}
	}

	MergeLists( &m_Stats, temp );
	delete temp;

	// count up the player numbers
	int nServers=0, nPlayers=0, nBots=0, nBotsServers=0;
	for ( j = 0; j < m_AllMods.getCount(); j++ )
	{
		CMod *p;
		p = m_AllMods.getMod( j );

		nServers += p->total.servers;
		nPlayers += p->total.players;
		nBots    += p->total.bots;
		nBotsServers    += p->total.bots_servers;
	}

	if ( nPlayers < m_iMinPlayersLevel )
	{
		char history[512];
		_snprintf( history, 512, "Warning, player levels dropped too low.\nPlayers:%i\nServers:%i\n", nPlayers, nServers);

		// Construct email to yahn
		FILE *fp = fopen( attachment, "wt" );
		if ( fp )
		{
			fwrite( history, strlen( history ) + 1, 1, fp );
			fclose( fp );
			EmailReport( m_sWarnEmail );
		}

	}

	if ( recomputeinfo )
	{
		RecomputeList( SkipPeaks );
	}

	if ( !CheckParm( "-fake" ) )
	{
		SaveModUpdates( &m_Stats, g_szTempFile, false, true );
	}

	ClearStartMsg();
}

void CStatusDlg::RedrawEverything( void )
{
	if ( m_pServers )
	{
		m_pServers->InvalidateRect( NULL );
		m_pServers->UpdateWindow();
	}
	
	if ( m_pPlayers )
	{
		m_pPlayers->InvalidateRect( NULL );
		m_pPlayers->UpdateWindow();
	}

	if ( m_pProjection )
	{
		m_pProjection->InvalidateRect( NULL );
		m_pProjection->UpdateWindow();
	}

	if ( m_pStatus )
	{
		m_pStatus->InvalidateRect( NULL );
		m_pStatus->UpdateWindow();
	}

	InvalidateRect( NULL );
	UpdateWindow();
}

/*
==============================
OnSize

==============================
*/
void CStatusDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	MoveControls();

	RedrawEverything();
}

/*
==============================
OnDeltaposRate

==============================
*/
void CStatusDlg::OnDeltaposRate(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;

	if ( pNMUpDown->iDelta > 0 )
	{
		m_fInterval += 1.0;
		m_fNextUpdate += 1.0;
	}
	else
	{
		m_fInterval -= 1.0;
		if ( m_fInterval < 1.0 )
			m_fInterval = 1.0;
		else
			m_fNextUpdate -= 1.0;
	}

	SetRate();

	RedrawEverything();
}

/*
==============================
PrintStartMsg

==============================
*/
void CStatusDlg::PrintStartMsg()
{
	m_statRequest.SetWindowText( "Requesting data..." );
}

/*
==============================
ClearStartMsg

==============================
*/
void CStatusDlg::ClearStartMsg()
{
	m_statRequest.SetWindowText( "" );
}

/*
==============================
PrintStatusMsg

==============================
*/
void CStatusDlg::PrintStatusMsg( char *fmt, ... )
{
	char string[ 1024 ];
	va_list argptr;
	
	va_start( argptr, fmt );
	vsprintf( string, fmt, argptr );
	va_end( argptr );

	m_statRequest.SetWindowText( string );
	m_statRequest.UpdateWindow();

	m_pStatus->Printf( string );
}

/*
==============================
OnMove

==============================
*/
void CStatusDlg::OnMove(int x, int y) 
{
	CDialog::OnMove(x, y);
	
	MoveControls();	

RedrawEverything();
}

void CStatusDlg::OnTimer( UINT nIDEvent )
{
	time_t now = time( NULL );
	struct tm *tmstruct = localtime( &now );	
	int timeAt12AM = now - ( tmstruct->tm_sec + tmstruct->tm_min*60 + tmstruct->tm_hour*60*60 );

	KillTimer(TIMER_NUMBER);

	time_t nextTime = 0xffffffff;
	int timeIndex = 0;
	for ( int i = 0; i < m_ReportTimes.Count(); i++ )
	{
		if ( m_ReportTimes[i].nextTime == 0 || m_ReportTimes[i].nextTime <= now )
		{
			if ( m_ReportTimes[i].nextTime == 0 )
			{
				m_ReportTimes[i].nextTime = timeAt12AM + m_ReportTimes[i].dayOffset;
			}
			if ( m_ReportTimes[i].nextTime <= now )
			{
				m_ReportTimes[i].nextTime = timeAt12AM + m_ReportTimes[i].dayOffset + SECONDS_PER_DAY;
			}
		}
		if ( m_ReportTimes[i].nextTime < nextTime ||  nextTime == 0xffffffff )
		{
			nextTime = m_ReportTimes[i].nextTime;
			timeIndex = i;
		}
	}
	
	if (  m_ReportTimes.Count() )
	{
		SetTimer( TIMER_NUMBER, (nextTime - now) * 1000, (TIMERPROC) NULL);
		if ( nIDEvent == TIMER_NUMBER )
		{
			GetEmailManager()->SendEmailsNow();
		}
	}
}

/*
==============================
MoveControls

==============================
*/
void CStatusDlg::MoveControls()
{
	if ( !m_pProjection || !m_pProjection->GetSafeHwnd() )
		return;

	CRect rcLog, rcSpin, rcMod;
	GetClientRect( &rcLog );

	rcLog.bottom -= 80;
	
	rcMod = rcLog;
	rcMod.bottom -= 20;

	m_rcServers = rcMod;

	rcMod.right = rcMod.left + 360;

	m_rcServers.left = rcMod.right;
	m_rcPlayers = m_rcServers;

	m_rcServers.bottom = m_rcServers.top + m_rcServers.Height() / 2;
	m_rcServers.InflateRect( -5, -5 );

	if ( m_pServers && m_pServers->GetSafeHwnd() )
	{
		m_pServers->MoveWindow( &m_rcServers, TRUE );
	}
	
	m_rcPlayers.top = m_rcPlayers.bottom - m_rcPlayers.Height() / 2;
	m_rcPlayers.InflateRect( -5, -5 );

	if ( m_pPlayers && m_pPlayers->GetSafeHwnd() )
	{
		m_pPlayers->MoveWindow( &m_rcPlayers, TRUE );
	}

	CRect rcProj = rcMod;
	rcProj.top = rcProj.bottom - 200;
	rcProj.InflateRect( -5, -5 );

	m_pProjection->MoveWindow( &rcProj, TRUE );

	rcMod.bottom -= 200;

	CRect rcPeak = rcMod;
	rcMod.bottom = rcMod.top + rcMod.Height() / 2;
	rcMod.InflateRect( -5, -5 );
	rcPeak.top = rcPeak.bottom - rcPeak.Height() / 2;
	rcPeak.InflateRect( -5, -5 );

	CRect rcRadio = rcPeak;
	rcPeak.top += 15;

	rcRadio.bottom = rcRadio.top + 15;
	rcRadio.right = rcRadio.left + 75;

	m_statPeak.MoveWindow( &rcRadio, TRUE );
	rcRadio.OffsetRect( rcRadio.Width(), 0 );
	rcRadio.right -= 25;

	m_btnHour.MoveWindow( &rcRadio, TRUE );
	rcRadio.OffsetRect( rcRadio.Width(), 0 );
	m_btnDay.MoveWindow( &rcRadio, TRUE );
	rcRadio.OffsetRect( rcRadio.Width(), 0 );
	m_btnWeek.MoveWindow( &rcRadio, TRUE );
	rcRadio.OffsetRect( rcRadio.Width(), 0 );
	m_btnMonth.MoveWindow( &rcRadio, TRUE );
	rcRadio.OffsetRect( rcRadio.Width(), 0 );

	m_modList.MoveWindow( &rcMod, TRUE );
	m_modPeak.MoveWindow( &rcPeak, TRUE );

	rcLog.top = rcLog.bottom - 20;

	rcLog.DeflateRect( 2, 2 );

	rcSpin = rcLog;

	rcLog.right = rcLog.left + 100;

	m_rcRefresh = rcLog;
	m_btnRefresh.MoveWindow( &m_rcRefresh, TRUE );

	rcLog.OffsetRect( rcLog.Width() + 10, 0 );
	m_btnEmail.MoveWindow( &rcLog, TRUE );

	rcLog.OffsetRect( rcLog.Width() + 10, 0 );
	rcLog.right += 150;
	m_statRequest.MoveWindow( &rcLog, TRUE );

	rcSpin.left = rcSpin.right - 15;

	m_rcRate = rcSpin;
	m_rcRate.right = m_rcRate.left;
	m_rcRate.left = m_rcRate.right - 100;

	rcSpin.InflateRect( 0, 2 );
	m_spinRate.MoveWindow( &rcSpin, TRUE );
	
	m_statRate.MoveWindow( &m_rcRate, TRUE );

	if ( m_pStatus && m_pStatus->GetSafeHwnd() )
	{
		RECT rcStat;
		GetClientRect( &rcStat );
		rcStat.top = rcStat.bottom - 80;
		m_pStatus->MoveWindow( &rcStat, TRUE );
	}
}

/*
==============================
OnRefresh

==============================
*/
void CStatusDlg::OnRefresh() 
{
	// TODO: Add your control notification handler code here
	double curtime;
	curtime = Sys_DoubleTime();
	m_fNextUpdate = curtime + m_fInterval;
	StartPing();
}

/*
==============================
SetRate

==============================
*/
void CStatusDlg::SetRate()
{
	char sz[ 256 ];

	sprintf( sz, "update every %.1f min", m_fInterval / 60.0 );

	m_statRate.SetWindowText( sz );
}

/*
==============================
OnDay

==============================
*/
void CStatusDlg::OnDay() 
{
	// TODO: Add your control notification handler code here
	UpdateData( TRUE );
	m_btnHour.SetCheck( 0 );
	m_btnWeek.SetCheck( 0 );
	m_btnMonth.SetCheck( 0 );

	m_nSortKey = SORT_DAY;
	RecomputeList();
}

/*
==============================
OnHour

==============================
*/
void CStatusDlg::OnHour() 
{
	// TODO: Add your control notification handler code here
	UpdateData( TRUE );
	m_btnDay.SetCheck( 0 );
	m_btnWeek.SetCheck( 0 );
	m_btnMonth.SetCheck( 0 );

	m_nSortKey = SORT_HOUR;
	RecomputeList();
}

/*
==============================
OnMonth

==============================
*/
void CStatusDlg::OnMonth() 
{
	// TODO: Add your control notification handler code here
	UpdateData( TRUE );
	m_btnHour.SetCheck( 0 );
	m_btnWeek.SetCheck( 0 );
	m_btnDay.SetCheck( 0 );

	m_nSortKey = SORT_MONTH;
	RecomputeList();
}

/*
==============================
OnWeek

==============================
*/
void CStatusDlg::OnWeek() 
{
	// TODO: Add your control notification handler code here
	UpdateData( TRUE );
	m_btnHour.SetCheck( 0 );
	m_btnDay.SetCheck( 0 );
	m_btnMonth.SetCheck( 0 );

	m_nSortKey = SORT_WEEK;
	RecomputeList();
}

/*
==============================
ResetPeaks

==============================
*/
void CStatusDlg::ResetPeaks( void )
{
	CMod *p;
	int j;
	for ( j = 0; j < m_AllMods.getCount(); j++ )
	{
		p = m_AllMods.getMod( j );

		memset( p->pl, 0, NUM_PEAKS * sizeof( int ) );
		memset( p->sv, 0, NUM_PEAKS * sizeof( int ) );
		memset( p->bots, 0, NUM_PEAKS * sizeof( int ) );
		memset( p->bots_servers, 0, NUM_PEAKS * sizeof( int ) );

	}
}

#define SAMPLE_INTERVAL 300.0
#define SAMPLES_PER_DAY ( (float)SECONDS_PER_DAY / SAMPLE_INTERVAL )
#define NUM_DAYS	30

/*
==============================
GenerateFakeData

==============================
*/
void CStatusDlg::GenerateFakeData( bool fulldataset )
{
	int i = 0;

	int j;
	int ns, np, nb, nbs;
	time_t starttime;
	CMod *pNewMod;

	int num_fakes = (int)( NUM_DAYS * SAMPLES_PER_DAY );

	starttime = GetTime();

	double player_base = { 75000.0 };
	double server_base = { 20000.0 };
	double bot_base    = { 20000.0 };
	double bot_server_base    = { 10000.0 };

#define MODCOUNT 20
	char modnames[ MODCOUNT ][32];
	for ( int n = 0; n < MODCOUNT; n++ )
	{
		int len = rand() % 10 + 2;
		for ( int c = 0 ; c < len; c++ )
		{
			modnames[ n ][ c ] = 'a' + rand() % 26;
		}
		modnames[ n ][ c ] = 0;
	}

	float statfrac[ MOD_STATCOUNT ] = { 0.45f, 0.25f, 0.3f };

	num_fakes = fulldataset ? num_fakes : 1;

	for ( j = 0; j < num_fakes; j++ )
	{
		int nummods = rand() % 20 + 15;
		nummods %= MODCOUNT;

		m_List.Reset();

		np = (int)( player_base + player_base * 0.4 * sin ( 6.28 * (float)j / (float)SAMPLES_PER_DAY ) );
		ns = (int)( server_base + server_base * 0.3 * sin ( 6.28 * (float)j / (float)SAMPLES_PER_DAY ) );
		nb = (int)( bot_base + bot_base * 0.3 * sin ( 6.28 * (float)j / (float)SAMPLES_PER_DAY ) );
		nbs = (int)( bot_server_base + bot_server_base * 0.3 * sin ( 6.28 * (float)j / (float)SAMPLES_PER_DAY ) );

		for ( int modcount = 0; modcount < nummods; modcount++ )
		{
			int modnum = rand() % MODCOUNT;

			float frand = (float)rand()/(float)RAND_MAX;
			frand = 0.6f + 0.4f * frand;
			float frand2 = (float)rand()/(float)RAND_MAX;
			frand2 = 0.6f + 0.4f * frand2;

			int modp = (int) ( ( np / nummods ) * frand );
			int mods = (int) ( ( ns / nummods ) * frand2 );
			int modb = (int) ( ( nb / nummods ) * frand2 );
			int modbs = (int) ( ( nbs / nummods ) * frand2 );
			
		//	np += (int)( 2000.0 * (float)rand()/(float)RAND_MAX);
		//	ns += (int)( 500.0 * (float)rand()/(float)RAND_MAX);

			pNewMod = new CMod();			

			for ( int stat = 0; stat < MOD_STATCOUNT; stat++ )
			{
				pNewMod->stats[ stat ].players = (int)( modp * statfrac[ stat ] );
				pNewMod->stats[ stat ].servers = (int)( mods * statfrac[ stat ] );
				pNewMod->stats[ stat ].bots    = (int)( modb * statfrac[ stat ] );
				pNewMod->stats[ stat ].bots_servers    = (int)( modbs * statfrac[ stat ] );
			}
			pNewMod->ComputeTotals();
			pNewMod->SetGameDir( modnames[ modnum ] );

			m_List.AddMod( pNewMod );
		}

		FinishRequest( - (long)SAMPLE_INTERVAL * ( num_fakes - j ), TRUE, FALSE );
	}

	RecomputeList();

	// If there was some left over data, see if it's from a previous day and go ahead and save it out if needed
	if ( m_Stats.list )
	{
		time_t earliest = FindEarliestTime( &m_Stats );
		ChangeDays( earliest, m_tmCurrentDay );
	}

	RedrawEverything();
}

#define MINS_PER_MONTH ( 24.0 * 60.0 * 30.0 )

/*
==============================
ComputeStats

==============================
*/
void ComputeStats( CModUpdate *st, CModUpdate *ed, double *pm, double *sm )
{
	CModUpdate *iter, *prev;
	double dt;
	double avg_units[2];
	double times[ 2 ];

	*pm = 0.0;
	*sm = 0.0;
	double total_time = 0.0;

	if ( st && ed && st != ed )
	{
		iter = st;
		prev = iter;
		iter = iter->next;
		while ( iter )
		{
			dt = difftime( prev->realtime, iter->realtime );

			avg_units[0] = (double)( iter->total.players + prev->total.players ) / 2.0;
			avg_units[1] = (double)( iter->total.servers + prev->total.servers ) / 2.0;

			for ( int i = 0; i < 2; i++ )
			{
				times[ i ] += dt * avg_units[ i ];
			}

			total_time += dt;

			if ( iter == ed )
				break;

			prev = iter;
			iter = iter->next;
			if ( !iter )
				break;
		}
	}

	// Convert to minutes
	for ( int i = 0; i < 2; i++ )
	{
		times[ i ] /= 60.0;
	}

	// Convert to monthly extrapolation
	if ( total_time != 0.0 )
	{
		// Convert time to minutes
		total_time /= 60.0;

		*pm = times[ 0 ] * ( MINS_PER_MONTH / total_time );
		*sm = times[ 1 ] * ( MINS_PER_MONTH / total_time );
	}
}

/*
==============================
ConvertDoubleToString

==============================
*/
char *ConvertDoubleToString( double inp )
{
	static char sz[ 64 ];
	char suffix[ 4 ];

	if ( inp > 1000000 )
	{
		sprintf( suffix, "M" );
		inp /= 1000000.0;
	}
	else if ( inp > 1000 )
	{
		sprintf( suffix, "K" );
		inp /= 1000.0;
	}
	else
	{
		sprintf( suffix, "" );
	}

	sprintf( sz, "%.2f%s", inp, suffix );
	return sz;
}

char const *CStatusDlg::GetPeakString( float threshold )
{
	static char final[ 32768 ];
	char peakstring[ 32768 ];

	final[ 0 ] = 0;

	// Across all mods
	CModList *pList = &m_AllMods;

	for ( int sortkey = SORT_HOUR; sortkey <= SORT_MONTH; sortkey++ )
	{
		peakstring[ 0 ] = 0;

		CModUpdate *st, *ed, *iter;
		CModList *l;
		CMod *p;
		char sz[ 1024 ];
		char szTitle[ 256 ];

		CModUpdate *peakIterPlayers = NULL, *peakIterServers = NULL, *peakIterBots = NULL, *peakIterBotsServers = NULL;

		int count;

		ResetPeaks();

		count =	GetHistoryBounds( sortkey, &st, &ed );

		l = pList;

		int nPeakServers = 0;
		int nPeakPlayers = 0;
		int nPeakBots = 0;
		int nPeakBotsServers = 0;

		int i;
		for ( i = 0; i < l->getCount(); i++ )
		{
			p = l->getMod( i );

			CModUpdate *peakServers = NULL, *peakPlayers = NULL;

			iter = st;
			while ( iter )
			{
				if ( iter->total.players > nPeakPlayers )
				{
					nPeakPlayers = iter->total.players;
					peakIterPlayers = iter;
				}

				if ( iter->total.servers > nPeakServers )
				{
					nPeakServers = iter->total.servers;
					peakIterServers = iter;
				}
				
				if ( iter->total.bots > nPeakBots )
				{
					nPeakBots = iter->total.bots;
					peakIterBots = iter;
				}	
				
				if ( iter->total.bots_servers > nPeakBotsServers )
				{
					nPeakBotsServers = iter->total.bots_servers;
					peakIterBotsServers = iter;
				}

				if ( iter == ed )
					break;
				
				iter = iter->next;
				if ( !iter )
					break;
			}
		}

		int svcutoff = (int)( (float)nPeakServers * threshold );
		int plcutoff = (int)( (float)nPeakPlayers * threshold );
		int botscutoff = (int)( (float)nPeakBots * threshold );
		int botsserverscutoff = (int)( (float)nPeakBotsServers * threshold );

		int ptot = 0, stot = 0, bottot = 0, botservertot = 0;

		for ( i = 0; i < l->getCount(); i++ )
		{
			p = l->getMod( i );

			CModUpdate *peakServers = NULL, *peakPlayers = NULL, *peakBots = NULL, *peakBotsServers = NULL;

			iter = st;
			while ( iter )
			{
				CMod *mod = iter->Mods.FindMod( p->GetGameDir() );
				if ( mod )
				{
					if ( mod->total.players > p->pl[ sortkey ] )
					{
						p->pl[ sortkey ] = mod->total.players;
						peakPlayers = iter;
					}
					if ( mod->total.servers > p->sv[ sortkey ] )
					{
						p->sv[ sortkey ] = mod->total.servers;
						peakServers = iter;
					}	
					if ( mod->total.bots > p->bots[ sortkey ] )
					{
						p->bots[ sortkey ] = mod->total.bots;
						peakBots = iter;
					}		
					if ( mod->total.bots_servers > p->bots_servers[ sortkey ] )
					{
						p->bots_servers[ sortkey ] = mod->total.bots_servers;
						peakBotsServers = iter;
					}
					
				}

				if ( iter == ed )
					break;
				
				iter = iter->next;
				if ( !iter )
					break;
			}

			sprintf( sz, "%-10s: %6i sv %6i pl %6i bt %6i bs", p->GetGameDir(), p->sv[ sortkey ], p->pl[ sortkey ], p->bots[ sortkey ], p->bots_servers[ sortkey ] );

			if ( peakServers && peakPlayers)
			{
				char outstr[ 512 ];
				sprintf( outstr, " -- %s & %s", GetTimeAsString( &peakServers->realtime ), GetTimeAsString( &peakPlayers->realtime ));
				strcat( sz, outstr );
			}

			bool counted = false;
			if ( p->sv[ sortkey ] || p->pl[ sortkey ] || p->bots[ sortkey ] || p->bots_servers[ sortkey ] )
			{
				if ( p->sv[ sortkey ] >= svcutoff ||
					p->pl[ sortkey ] >= plcutoff ||
					p->bots[ sortkey ] >= botscutoff ||
					p->bots_servers[ sortkey ] >= botsserverscutoff)
				{
					strcat( peakstring, sz );
					strcat( peakstring, "\n" );
					counted = true;
				}
			}

			if ( !counted )
			{
				ptot += p->pl[ sortkey ];
				stot += p->sv[ sortkey ];
				bottot += p->bots[ sortkey ];
				botservertot += p->bots_servers[ sortkey ];
			}
		}

		sprintf( szTitle, "%-10s: %6i sv, %6i pl %6i bt %6i bs", game_name, nPeakServers, nPeakPlayers, nPeakBots, nPeakBotsServers );

		if ( peakIterServers && peakIterPlayers )
		{
			char outstr[ 512 ];
			sprintf( outstr, " -- %s & %s", GetTimeAsString( &peakIterServers->realtime ), GetTimeAsString( &peakIterPlayers->realtime ));
			strcat( szTitle, outstr );
		}

		switch( sortkey )
		{
		default:
		case SORT_HOUR:
			strcat( final, "Hourly:\n" );
			break;
		case SORT_DAY:
			strcat( final, "Daily:\n" );
			break;
		case SORT_WEEK:
			strcat( final, "Weekly:\n" );
			break;
		case SORT_MONTH:
			strcat( final, "Monthly:\n" );
			break;
		}
		strcat( final, szTitle );
		strcat( final, "\n----------------------------------------------------------\n" );
		strcat( final, peakstring );
		sprintf( sz, "%-10s: %6i sv %6i pl %6i bt %6i bs", "other", stot, ptot, bottot, botservertot );
		strcat( final, sz );
		strcat( final, "\n\n" );

	}

	return final;
}

char const *CStatusDlg::GetProjectionString( void )
{
	if ( !m_pProjection )
		return "";

	if ( !m_pProjection->GetSafeHwnd() )
		return "";

	static char szFinal[ 4096 ];
	char sz[ 1024 ];

	double times[ 2 ];

	CModUpdate *st, *ed;
	int count;
	int sortkey;

	sprintf( szFinal, "Projected Rates\n" );
	char type[ 32 ];

	times[ 0 ]	= times[ 1 ]		= 0.0;

	sprintf( sz, "  %s\n", game_name );
	strcat( szFinal, sz );

	for ( int sk = SORT_HOUR; sk <= SORT_MONTH; sk++ )
	{
		sortkey = sk;
		count =	GetHistoryBounds( sortkey, &st, &ed );
		ComputeStats( st, ed, &times[ 0 ], &times[ 1 ] );
		switch( sk )
		{
		case SORT_HOUR:
		default:
			sprintf( type, "hour" );
			break;
		case SORT_DAY:
			sprintf( type, "day" );
			break;
		case SORT_WEEK:
			sprintf( type, "week" );
			break;
		case SORT_MONTH:
			sprintf( type, "month" );
			break;
		}

		sprintf( sz, "    %-10s pm", ConvertDoubleToString( times[ 0 ] ) );
		strcat( szFinal, sz );
		sprintf( sz, " %10s sm over last %s\n", ConvertDoubleToString( times[ 1 ] ), type );
		strcat( szFinal, sz );
	}

	return szFinal;
}

/*
==============================
ComputeProjections

==============================
*/
void CStatusDlg::ComputeProjections( void )
{
	char const *str = GetProjectionString();

	m_pProjection->SetWindowText( str );
	m_pProjection->InvalidateRect( NULL );
	m_pProjection->UpdateWindow();
}

/*
==============================
COM_StripExtension

==============================
*/
void COM_StripExtension (char *in, char *out)
{
	char * in_current  = in + strlen(in);
	char * out_current = out + strlen(in);
	int    found_extension = 0;
	
	while (in_current >= in)
	{
		if ((found_extension == 0) && (*in_current == '.'))
		{
			*out_current = 0;
			found_extension = 1;
		}
		else
		{
			if ((*in_current == '/') || (*in_current == '\\'))
			{
				found_extension = 1;
			}
			
			*out_current = *in_current;
		}
		
		in_current--;
		out_current--;
	}
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}

/*
==============================
SaveModUpdates

==============================
*/
void CStatusDlg::SaveModUpdates( CModStats *l, char *pszFile, bool savefileloaded, bool clobberfile )
{
	CModUpdate		*p;
	CMod			*m;
	FILE			*fp;

	// If file is already there, merge them together and save out as a new list
	if ( !clobberfile )
	{
		fp = fopen( pszFile, "rb" );
		if ( fp )
		{
			fclose( fp );

			if ( !savefileloaded )
				return;

			// Load old data 
			CModStats *old = LoadModUpdates( pszFile, false );

			// Mark all stuff loaded from file as if it wasn't loaded from file...
			p = old ? old->list : NULL;
			while ( p )
			{
				p->m_bLoadedFromFile = false;
				p = p->next;
			}

			// Remove the old file
			_unlink( pszFile );

			// Was there actually old data, if so, recurse after merging, otherwise, just
			// continue on saving the new data...
			if ( old )
			{
				MergeLists( l, old );

				// old data is now in new list...
				delete old;

				// Recurse and save merged data
				SaveModUpdates( l, pszFile, true, false );

				return;
			}
		}
	}

	COM_CreatePath( pszFile );

	fp = fopen( pszFile, "wt" );
	if ( !fp )
	{
		m_pStatus->ColorPrintf( RGB( 255, 0, 0 ), "Error opening file %s\n",
			pszFile );
		return;
	}

	// Write the data!
	p = l->list;

	fprintf( fp, "{\n" );

	while ( p )
	{
		if ( !p->m_bLoadedFromFile || savefileloaded )
		{
			int j;

			struct tm *lt;

			lt = localtime( &p->realtime );

			fprintf( fp, "\t//--------------------------------------------------\n" );
			fprintf( fp, "\ttime %u // %s", *(unsigned int *)&p->realtime, asctime( lt ) );
			fprintf( fp, "\t%i\n", MOD_STATCOUNT );
			for ( j = 0; j < MOD_STATCOUNT; j++ )
			{
				fprintf( fp, "\t%i %i %i %i\n", p->stats[ j ].players, p->stats[ j ].servers, p->stats[ j ].bots, p->stats[ j ].bots_servers );
			}

			// Serialize raw data.
			fprintf( fp, "\t{\n" );

			for ( j = 0; j < p->Mods.getCount(); j++ )
			{
				m = p->Mods.getMod( j );
				m->WriteToFile( fp );
			}

			fprintf( fp, "\t}\n\n" );
		}

		p = p->next;
	}

	fprintf( fp, "}\n" );


	fclose( fp );
}

bool com_ignorecolons = false;  // YWB:  Ignore colons as token separators in COM_Parse
static bool s_com_token_unget = false;
static char	com_token[1024];

/*
==================
COM_UngetToken

Requeue previous token
==================
*/
void COM_UngetToken( void )
{
	s_com_token_unget = true;
}

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
static char *COM_Parse (char *data)
{
	int             c;
	int             len;
	
	if ( s_com_token_unget )
	{
		s_com_token_unget = false;
		return data;
	}

	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || (!com_ignorecolons && c==':') || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || (!com_ignorecolons && c==':') || c == ',' )
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

void ReverseList(CModUpdate **ppList)
{
	CModUpdate *p, *n, *newlist = NULL;
	p = *ppList;
	while ( p )
	{
		n = p->next;
		p->next = newlist;
		newlist = p;
		p = n;
	}
	*ppList = newlist;
}

/*
==============================
LoadModUpdates

==============================
*/
CModStats *CStatusDlg::LoadModUpdates( char *pszFile, bool markasfileloaded, int flags /* = NORMAL */, float threshold /*= 0.0f*/, char const *modname /*= NULL*/ )
{
	CModStats		*l;
	CModUpdate		*u;
	CMod			*m;
	FILE			*fp;
	char			sz[ 256 ];
	char			*buffer;
	char			*p;
	bool			success = false;

	fp = fopen( pszFile, "rb" );
	if ( !fp )
	{
		sprintf( sz, "Error opening file %s", pszFile );
		return NULL;
	}

	fseek( fp, 0, SEEK_END );

	int size = ftell( fp );

	fseek( fp, 0, SEEK_SET );

	if ( !size )
		return NULL;

	buffer = ( char * )malloc( size + 1 );
	if ( !buffer )
		return NULL;

	fread( buffer, size, 1, fp );
	buffer[ size ] = '\0';

	fclose( fp );

	p = buffer;

	l = new CModStats();
	l->count = 0;
	l->list = NULL;

	p = COM_Parse( p );
	if ( stricmp( com_token, "{" ) )
	{
		goto finish_parse;
	}

	while ( 1 )
	{
		u = new CModUpdate;
		if ( !u )
		{
			goto finish_parse;
		}

		p = COM_Parse( p );
		if ( stricmp( com_token, "time" ) )
		{
			goto finish_parse;
		}
		p = COM_Parse( p );
		u->realtime = (time_t)(unsigned int)( atoi( com_token ) );
		
		p = COM_Parse( p );
		int count = atoi( com_token );
		for ( int k = 0; k < count; k++ )
		{
			int pl = 0, sv = 0, bt = 0, bs = 0;

			p = COM_Parse( p );
			pl = atoi( com_token );
			p = COM_Parse( p );
			sv = atoi( com_token );
			if ( *p == '\r' ) // old file format
			{
				bt = bs = 0;
			}
			else
			{
				p = COM_Parse( p );
				bt = atoi( com_token );
				p = COM_Parse( p );
				bs = atoi( com_token );
			}

			if ( k < MOD_STATCOUNT )
			{
				u->stats[ k ].players = pl;
				u->stats[ k ].servers = sv;
				u->stats[ k ].bots = bt;
				u->stats[ k ].bots_servers = bs;
			}
		}

		u->ComputeTotals();

		// Serialize raw data.
		p = COM_Parse( p );
		if ( !stricmp( com_token, "}" ) )
			break;

		if ( stricmp( com_token, "{" ) )
		{
			goto finish_parse;
		}

		bool fulldataset = true;

		while ( 1 )
		{
			m = new CMod;
			bool bOk = m->ReadFromFile( &p );
			if ( !bOk )
			{
				delete m;
				break;
			}

			bool keepmod = true;

			switch( flags )
			{
			case NORMAL:
			default:
				break;
			case IGNOREMODS:
				keepmod = false;
				break;
			case SPECIFICMOD:
				if ( modname && modname[ 0 ] &&	stricmp( modname, m->GetGameDir() )  )
				{
					keepmod = false;
				}
				break;
			case USETHRESHOLD:
				if ( m->total.players < u->total.players * threshold &&
					 m->total.servers < u->total.servers * threshold )
				{
					keepmod = false;
				}
			}

			// Add it to global names list at least
			CMod *all = m_AllMods.FindMod( m->GetGameDir() );
			if ( !all )
			{
				all = m->Copy();
				all->Reset();

				// Just add it as a blank
				m_AllMods.AddMod( all );
			}

			if ( keepmod )
			{
				u->Mods.AddMod( m );
			}
			else
			{
				fulldataset = false;
				delete m;
			}
		}

		u->m_bLoadedFromFile = markasfileloaded;

		u->m_bIsPartialDataSet = !fulldataset;

		u->next = l->list;
		l->list = u;
		l->count++;

		p = COM_Parse( p );
		if ( !stricmp( com_token, "}" ) )
		{
			break;
		}

		COM_UngetToken();

	}

	SortUpdates( l );
	RemoveDuplicates( l );

	ReverseList( &l->list );

	success = true;

finish_parse:
	if ( buffer )
	{
		free( buffer );
	}

	if ( !success )
	{
		delete l;
		l = NULL;
	}

	return l;
}

void CStatusDlg::MergeLists( CModStats *inlist1, CModStats *inlist2 )
{
	int count = 0;

	CModUpdate *l1, *l2;

	CModUpdate *newlist = NULL;

	l1 = inlist1->list;
	l2 = inlist2->list;

	while ( l1 || l2 )
	{
		CModUpdate *l1next = NULL, *l2next = NULL;

		if ( l1 )
		{
			l1next = l1->next;
		}

		if ( l2 )
		{
			l2next = l2->next;
		}

		bool keepl1 = false, keepl2 = false;

		// Yuck, this is kind of spaghetti
		if ( l1 && l2 )
		{
			if ( l1->realtime > l2->realtime )
			{
				keepl1 = true;
			}
			else if ( l1->realtime < l2->realtime )
			{
				keepl2 = true;
			}
			else
			{
				if ( l1->m_bIsPartialDataSet && !l2->m_bIsPartialDataSet )
				{
					keepl2 = true;

					delete l1;
					l1 = l1next;
				}
				else if ( !l1->m_bIsPartialDataSet && l2->m_bIsPartialDataSet)
				{
					keepl1 = true;

					delete l2;
					l2 = l2next;
				}
				else
				{
					if ( l1->m_bLoadedFromFile && !l2->m_bLoadedFromFile )
					{
						keepl1 = true;

						delete l2;
						l2 = l2next;
					}
					else if ( l2->m_bLoadedFromFile && !l1->m_bLoadedFromFile )
					{
						keepl2 = true;

						delete l1;
						l1 = l1next;
					}
					else
					{
						keepl1 = true;

						delete l2;
						l2 = l2next;
					}
				}
			}
		}
		else
		{
			if ( l1 )
			{
				keepl1 = true;
			}
			if ( l2 )
			{
				keepl2 = true;
			}
		}

		if ( keepl1 )
		{
			l1->next = newlist;
			newlist = l1;

			l1 = l1next;

			count++;
		}
		
		if ( keepl2 )
		{
			l2->next = newlist;
			newlist = l2;

			l2 = l2next;

			count++;
		}
	}

	ReverseList( &newlist );

	inlist2->list = NULL;
	inlist1->list = newlist;
	inlist1->count = count;
}

int _cdecl FnUpdateCompare( const void *elem1, const void *elem2 )
{
	CModUpdate *p1 = *( CModUpdate ** )elem1;
	CModUpdate *p2 = *( CModUpdate ** )elem2;

	unsigned t1, t2;
	t1 = (unsigned int)p1->realtime;
	t2 = (unsigned int)p2->realtime;

	if ( t1 < t2 )
		return -1;
	else if ( t1 > t2 )
		return 1;

	return 0;
}

void CStatusDlg::SortUpdates( CModStats *list )
{
	if ( list->count <= 1 )
		return;

	CModUpdate **temp = new CModUpdate *[ list->count ];
	CModUpdate *p = list->list;

	for ( int i = 0; i < list->count; i++, p = p->next )
	{
		temp[ i ] = p;
	}

	assert( p == NULL );

	qsort( temp, list->count, sizeof( CModUpdate * ), FnUpdateCompare );

	for ( i = 0; i < list->count-1; i++ )
	{
		temp[ i ]->next = temp[ i + 1 ];
	}
	temp[ i ]->next = NULL;

	list->list = temp[ 0 ];
	delete[] temp;
}

void CStatusDlg::RemoveDuplicates( CModStats *stats )
{
	if ( !stats || !stats->list )
		return;

	CModUpdate *list	= stats->list;
	CModUpdate *p		= list;
	CModUpdate *prev	= NULL;
	while ( p )
	{
		CModUpdate *next = p->next;

		if ( prev && prev->realtime == p->realtime )
		{
			// Remove this entry from list

			// FIXME:  Throw away new sample in favor of disk sample!
			prev->next = next;
			stats->count--;
			delete p;
			// leave prev the same
		}
		else
		{
			prev = p;
		}

		p = next;
	}

	ResetHistory();
}

/*
==============================
NET_StringToSockaddr

==============================
*/
bool NET_StringToSockaddr (char *s, struct sockaddr *sadr)
{
	struct hostent	*h;
	char	*colon;
	char	copy[128];
	
	memset (sadr, 0, sizeof(*sadr));

	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	((struct sockaddr_in *)sadr)->sin_port = 0;

	strcpy (copy, s);
	// strip off a trailing :port if present
	for (colon = copy ; *colon ; colon++)
		if (*colon == ':')
		{
			*colon = 0;
			((struct sockaddr_in *)sadr)->sin_port = htons((short)atoi(colon+1));	
		}

	if (copy[0] >= '0' && copy[0] <= '9' && strstr( copy, "." ) )
	{
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(copy);
	}
	else
	{
		if (! (h = gethostbyname(copy)) )
			return 0;
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}
	
	return true;
}

/*
==============================
NET_SockadrToString

==============================
*/
const char *NET_SockadrToString( struct sockaddr *s )
{
	static char sz[ 256 ];

	sz[ 0 ] = '\0';
	if (s->sa_family == AF_INET)
	{
		sprintf( sz, "%s", inet_ntoa( ((struct sockaddr_in *)s)->sin_addr ) );
	}

	return sz;
}

static char const *MemToString( int mem )
{
	static char ret[ 32 ];

	int onekb = 1024;
	int onemb = onekb * onekb;

	if ( mem > onemb )
	{
		sprintf( ret, "%.2f Mb", (float)mem / (float)onemb );
	}
	else if ( mem > onekb )
	{
		sprintf( ret, "%.2f Kb", (float)mem / (float)onekb );
	}
	else
	{
		sprintf( ret, "%i bytes", mem );
	}

	return ret;
}


void CStatusDlg::OnMemory() 
{
	// TODO: Add your command handler code here
	char sz[ 512 ];

#define NUM_MEM_COUNTERS 3
	int mem[ NUM_MEM_COUNTERS ];

	mem[ 0 ] = CMod::mem();
	mem[ 1 ] = CModUpdate::mem();
	mem[ 2 ] = CModList::mem();

	sprintf( sz, "CMod uses:  %s\n", MemToString( mem[ 0 ] ) );
	m_pStatus->ColorPrintf( RGB( 0, 63, 150 ), sz );
	sprintf( sz, "CModUpdate uses:  %s\n", MemToString( mem[ 1 ] ) );
	m_pStatus->ColorPrintf( RGB( 0, 63, 150 ), sz );
	sprintf( sz, "CModList uses:  %s\n", MemToString( mem[ 2 ] ) );
	m_pStatus->ColorPrintf( RGB( 0, 63, 150 ), sz );

	int total = 0;
	for ( int i = 0; i < NUM_MEM_COUNTERS; i++ )
	{
		total += mem[ i ];
	}

	sprintf( sz, "Total:  %s\n", MemToString( total ) );
	m_pStatus->ColorPrintf( RGB( 0, 63, 150 ), sz );
}


void CStatusDlg::OnLoad()
{
	CLoadDialog dlg( this );
	if ( dlg.DoModal() != IDOK )
		return;

	bool dirty = false;

	int flags = dlg.GetFlags();

	float t = (float)atof( ( char * )( LPCSTR )dlg.m_strThreshold );
	char const *modname = ( LPCSTR )dlg.m_strModName;

	for ( int i = 0; i < dlg.GetNumFiles(); i++ )
	{
		char *filename = ( char * )dlg.GetFile( i );
		if ( !filename || !filename[ 0 ] )
			continue;

		CModStats *stats = NULL;

		switch ( flags )
		{
		default:
		case NORMAL:
			stats = LoadModUpdates( filename, true, flags );
			break;
		case IGNOREMODS:
			stats = LoadModUpdates( filename, true, flags );
			break;
		case USETHRESHOLD:
			{
				if ( t > 0.0001f )
				{
					stats = LoadModUpdates( filename, true, flags, t );
				}
			}
			break;
		case SPECIFICMOD:
			{
				if ( modname && modname[ 0 ] )
				{
					stats = LoadModUpdates( filename, true, flags, 0.0f, modname );
				}
			}
			break;
		}

		if ( stats )
		{
			dirty = true;
			MergeLists( &m_Stats, stats );
			delete stats;
		}
	}

	if ( dirty )
	{
		RecomputeList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CStatusDlg::FileNameForTime( time_t t )
{
	static char filename[ 512 ];

	filename[ 0 ] = 0;

	struct tm *lt;
	lt = localtime( &t );

	sprintf( filename, DATADIR"\\%04i\\%02i_%02i.txt",
		lt->tm_year + 1900,
		lt->tm_mon + 1,
		lt->tm_mday );

	return filename;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t1 - 
//			t2 - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CStatusDlg::IsInSameDay( time_t t1, time_t t2 )
{
	struct tm lt1, lt2;
	lt1 = *localtime( &t1 );
	lt2 = *localtime( &t2 );

	if ( lt1.tm_yday != lt2.tm_yday )
		return false;

	if ( lt1.tm_year != lt2.tm_year )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : oldDate - 
//			newDate - 
//-----------------------------------------------------------------------------
void CStatusDlg::ChangeDays( time_t oldDate, time_t newDate )
{
	if ( IsInSameDay( oldDate, newDate ) )
		return;
	
	if ( oldDate > newDate )
	{
		time_t temp = newDate;
		newDate = oldDate;
		oldDate = temp;
	}

	CModStats *save = NULL;

	for ( time_t t = oldDate; !IsInSameDay( t, newDate ) && t < ( newDate + SECONDS_PER_DAY ); t += SECONDS_PER_DAY )
	{
		char sz[ 512 ];

		sprintf( sz, "Saving data file %s\n", (char *)FileNameForTime( t ) );
		m_pStatus->ColorPrintf( RGB( 0, 63, 150 ), sz );

		// Save current day
		save = RemoveItemsForDay( &m_Stats, t, false );
		if ( save )
		{
			if ( save->count )
			{
				SaveModUpdates( save, (char *)FileNameForTime( t ), false, false );
				
				if ( t >= m_tmCurrentDay - m_nDaysInMemory * SECONDS_PER_DAY )
				{
					CModUpdate *p = save->list;
					while ( p )
					{
						p->m_bLoadedFromFile = true;
						p = p->next;
					}
					
					MergeLists( &m_Stats, save );
				}
			}
			
			delete save;
			save = NULL;
		}
	}


	int secondstokeep = m_nDaysInMemory * SECONDS_PER_DAY;
	time_t archiveDate = oldDate - secondstokeep;

	// Archive after two weeks
	// FIXME:  Make this user specifiable
	save = RemoveItemsForDay( &m_Stats, archiveDate, true );
	delete save;

	ResetHistory();
	RedrawEverything();
}

//-----------------------------------------------------------------------------
// Purpose: Splits all items matching date of t into a new list which is returned
// Input  : *original - 
//			t - 
// Output : CModStats
//-----------------------------------------------------------------------------
CModStats *CStatusDlg::RemoveItemsForDay( CModStats *original, time_t t, bool includinganythingbeforeday )
{
	assert( original );

	CModUpdate *newlist = NULL;
	CModUpdate *cutlist = NULL;
	int count = 0;

	CModUpdate *p, *n;

	p = original->list;
	while ( p )
	{
		n = p->next;
		bool earlier = ( p->realtime < t ) && includinganythingbeforeday;
		if ( IsInSameDay( t, p->realtime ) || earlier )
		{
			p->next = cutlist;
			cutlist = p;

			count++;
			original->count--;
		}
		else
		{
			p->next = newlist;
			newlist = p;
		}

		p = n;
	}

	ReverseList( &newlist );
	ReverseList( &cutlist );

	original->list = newlist;

	CModStats *r = new CModStats;
	assert( r );
	r->list = cutlist;
	r->count = count;

	return r;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *list - 
//			start - 
//			end - 
//-----------------------------------------------------------------------------
void CStatusDlg::LoadDateSpan( CModStats *list, time_t start, time_t end )
{
	if ( end < start )
	{
		time_t temp = end;
		end = start;
		start = temp;
	}

	for ( time_t t = start; t <= end + SECONDS_PER_DAY; t += SECONDS_PER_DAY )
	{
		if ( t > end )
		{
			if ( !IsInSameDay( t, end ) )
				break;
		}

		if ( IsFileDataForDateInList( list, t ) )
			continue;

		// Load file data
		CModStats *merge = LoadModUpdates( (char *)FileNameForTime( t ), true );
		if ( merge  )
		{
			if ( merge->count > 0 )
			{
				MergeLists( list, merge );
			}
			delete merge;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *list - 
//			t - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CStatusDlg::IsFileDataForDateInList( CModStats *list, time_t t )
{
	if ( !list || !list->list )
		return false;

	CModUpdate *p = list->list;
	while ( p )
	{
		if ( IsInSameDay( p->realtime, t ) && p->m_bLoadedFromFile )
		{
			return true;
		}

		p = p->next;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *list - 
// Output : time_t
//-----------------------------------------------------------------------------
time_t CStatusDlg::FindEarliestTime( CModStats *list )
{
	CModUpdate *p = list->list;
	if ( !p )
		return (time_t)0;

	while ( p->next )
	{
		p = p->next;
	}

	return p->realtime;
}

void CStatusDlg::OnClosearchived() 
{
	// TODO: Add your command handler code here
	CModStats *del = RemoveArchivedData( &m_Stats );
	delete del;

	ResetHistory();
	RedrawEverything();
}


//-----------------------------------------------------------------------------
// Purpose: Remove all data marked as "loaded from file"
// Output : CModStats
//-----------------------------------------------------------------------------
CModStats *CStatusDlg::RemoveArchivedData( CModStats *original )
{
	assert( original );

	CModUpdate *newlist = NULL;
	CModUpdate *cutlist = NULL;
	int count = 0;

	CModUpdate *p, *n;

	p = original->list;
	while ( p )
	{
		n = p->next;
		if ( p->m_bLoadedFromFile )
		{
			p->next = cutlist;
			cutlist = p;

			count++;
			original->count--;
		}
		else
		{
			p->next = newlist;
			newlist = p;
		}

		p = n;
	}

	ReverseList( &newlist );
	ReverseList( &cutlist );

	original->list = newlist;

	CModStats *r = new CModStats;
	assert( r );
	r->list = cutlist;
	r->count = count;

	return r;
}

void CStatusDlg::ResetHistory( void )
{
	if ( m_pPlayers && m_pServers )
	{
		m_pPlayers->ResetHistory();
		m_pServers->ResetHistory();
	}
}

void CStatusDlg::OnQuit() 
{
	// TODO: Add your command handler code here
	if ( MessageBox( "Are you sure?", "Quit Status", MB_OKCANCEL ) == IDOK )
	{
		OnOK();
	}
}

void CStatusDlg::OnSchedulerDialog()
{
	CSchedulerDialog dlg( this );

	if ( dlg.DoModal() != IDOK )
		return;	

	m_ReportTimes.RemoveAll();
	
	char scheduleString[512];
	memset( scheduleString, 0x0, sizeof( scheduleString ) );
	for ( int i = 0; i < GetScheduler()->GetItemCount(); i++ )
	{
		struct times newTime;
		newTime.nextTime = 0;

		int hour=0,min=0,sec=0;
		sscanf( GetScheduler()->GetTime(i), "%d:%d:%d", &hour, &min, &sec );
		strncat( scheduleString, GetScheduler()->GetTime(i), sizeof(scheduleString) );
		strncat(scheduleString, ";", sizeof(scheduleString));

		newTime.dayOffset = hour*60*60 + min*60 + sec;
		m_ReportTimes.AddToTail( newTime );
	}
	theApp.WriteProfileString( "Status", "ScheduleTimes", scheduleString );

	OnTimer( 0 );
}

void CStatusDlg::OnOptions() 
{
	if ( !m_pServers || !m_pPlayers )
		return;

	// TODO: Add your command handler code here
	COptionsDlg dlg( this );

	char sz[ 32 ];
	sprintf( sz, "%i", m_pServers->GetYScale() );
	dlg.m_strServers = sz;
	sprintf( sz, "%i", m_pPlayers->GetYScale() );
	dlg.m_strPlayers = sz;
	sprintf( sz, "%i", m_nDaysInMemory );
	dlg.m_strDays = sz;
	dlg.m_strMaster = m_strMaster;

	if ( dlg.DoModal() != IDOK )
		return;

	int nServers = atoi( ( LPCSTR )dlg.m_strServers );
	int nPlayers = atoi( ( LPCSTR )dlg.m_strPlayers );
	int nDays = atoi( (LPCSTR )dlg.m_strDays );

	if ( nServers > 0 && nServers != m_pServers->GetYScale() )
	{
		theApp.WriteProfileString( "Status", "Servers", dlg.m_strServers );
		m_pServers->SetYScale( nServers );
		m_pStatus->ColorPrintf( RGB( 0, 0, 0 ), "Servers set to %i\n", nServers );
	}

	if ( nPlayers > 0 && nPlayers != m_pPlayers->GetYScale() )
	{
		theApp.WriteProfileString( "Status", "Players", dlg.m_strPlayers );
		m_pPlayers->SetYScale( nPlayers );
		m_pStatus->ColorPrintf( RGB( 0, 0, 0 ), "Players set to %i\n", nPlayers );
	}

	if ( nDays > 0 && nDays != m_nDaysInMemory)
	{
		theApp.WriteProfileString( "Status", "DaysKeptInMemory", dlg.m_strDays );
		m_nDaysInMemory = nDays;

		m_pStatus->ColorPrintf( RGB( 0, 0, 0 ), "Keeping %i days in memory\n", nDays );

		time_t earliest = FindEarliestTime( &m_Stats );
		time_t startkeeping = m_tmCurrentDay - SECONDS_PER_DAY * m_nDaysInMemory;

		// Remove any stuff that's earlier than new count
		if ( earliest <= startkeeping )
		{
			ChangeDays( earliest, startkeeping );
		}

		// But make sure we're up to date
		LoadDateSpan( &m_Stats, startkeeping, m_tmCurrentDay );
	}

	if ( !dlg.m_strMaster.IsEmpty() && dlg.m_strMaster != m_strMaster )
	{
		m_strMaster = dlg.m_strMaster;
		theApp.WriteProfileString( "Status", "Master", m_strMaster );

		m_pStatus->ColorPrintf( RGB( 0, 0, 0 ), "Master server set to %s\n", (LPCSTR)m_strMaster );

	}
}

void CStatusDlg::OnWarnDialog() 
{
	// TODO: Add your command handler code here
	CWarnDialog dlg( this );

	char sz[ 32 ];
	sprintf( sz, "%i", m_iMinPlayersLevel );
	dlg.m_sMinPlayers = sz;
	dlg.m_sWarnEmail = m_sWarnEmail;

	if ( dlg.DoModal() != IDOK )
		return;

	int minPlayers = atoi( ( LPCSTR )dlg.m_sMinPlayers );
	const char  *warnEmail = dlg.m_sWarnEmail;

	if ( minPlayers != m_iMinPlayersLevel )
	{
		theApp.WriteProfileString( "Status", "MinPlayers", dlg.m_sMinPlayers );
		m_iMinPlayersLevel = minPlayers;
	}

	if ( warnEmail && stricmp( warnEmail, m_sWarnEmail ) ) 
	{
		theApp.WriteProfileString( "Status", "WarnEmail", dlg.m_sWarnEmail );
		delete m_sWarnEmail;
		m_sWarnEmail = new char[ strlen(warnEmail) + 1 ];
		assert( m_sWarnEmail );
		strcpy ( m_sWarnEmail, warnEmail );
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
bool CStatusDlg::GenerateReport( void )
{
	char *history = (char *)_alloca( 256 * 1024 );

	char servers[ 4096 ];
	char players[ 4096 ];

	CModUpdate *mostRecent = m_Stats.list;

	strcpy( servers, m_pServers->GetHistoryString( mostRecent ) );
	strcpy( players, m_pPlayers->GetHistoryString( mostRecent ) );

	sprintf( history, "Status Report\nReport time %s\nServers\n%s\n\nPlayers\n%s\nProjections\n%s\nPeaks\n%s\n",
		GetTimeAsString(), servers, players, GetProjectionString(), GetPeakString( 0.001f ) );

	// Construct email to yahn
	FILE *fp = fopen( attachment, "wt" );
	if ( fp )
	{
		fwrite( history, strlen( history ) + 1, 1, fp );
		fclose( fp );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *emailaddress - 
//-----------------------------------------------------------------------------
void CStatusDlg::EmailReport( char const *emailaddress )
{
	char cmdargs[ 256 ];
	sprintf( cmdargs, "-to %s -from %s -subject %s %s", 
		emailaddress, 
		"status.exe@valvesoftware.com",
		"\"Status Update\"",
		attachment );

	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	struct _stat statBuf;
	if ( !_stat("d:\\tf2\\bin\\smtpmail.exe", &statBuf ) )
	{
		BOOL failed = CreateProcess( "d:\\tf2\\bin\\smtpmail.exe", cmdargs,
			NULL, NULL,
			FALSE,
			CREATE_NO_WINDOW,
			NULL,
			NULL,
			&si,
			&pi );
	}
	else
	{	// assume its in the path somewhere
		BOOL failed = CreateProcess( "smtpmail.exe", cmdargs,
		NULL, NULL,
		FALSE,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&si,
		&pi );
	}


}

void CStatusDlg::OnEmail() 
{
	// TODO: Add your control notification handler code here
	GetEmailManager()->SendEmailsNow();
}

void CStatusDlg::OnAddemail() 
{
	// TODO: Add your command handler code here
	CEmailDialog dlg;
	dlg.DoModal();
}
