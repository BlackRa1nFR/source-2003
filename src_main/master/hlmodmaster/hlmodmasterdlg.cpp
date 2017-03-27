// HLModMasterDlg.cpp : implementation file
//

#include "stdafx.h"
#include <afxpriv.h>
#include "HLModMasterProtocol.h"
#include "HLModMaster.h"
#include "HLModMasterDlg.h"
#include "../../common/proto_oob.h"

#include "Token.h"

#include <time.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// UDP Port
static double		pfreq;
static double		curtime = 0.0;
static double		lastcurtime = 0.0;
static int			lowshift;

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
#define DO(src,dest)	\
	copy[0] = s[src];	\
	copy[1] = s[src + 1];	\
	sscanf (copy, "%x", &val);	\
	((struct sockaddr_ipx *)sadr)->dest = val

BOOL	NET_StringToSockaddr (const char *s, struct sockaddr *sadr)
{
	struct hostent	*h;
	char	*colon;
	char	copy[128];
	
	memset (sadr, 0, sizeof(*sadr));

	{
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
		
		if (copy[0] >= '0' && copy[0] <= '9')
		{
			*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(copy);
			if ( ((struct sockaddr_in *)sadr)->sin_addr.s_addr == INADDR_NONE )
				return FALSE;
		}
		else
		{
			if (! (h = gethostbyname(copy)) )
				return FALSE;
			*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
		}
	}
	
	return TRUE;
}

#undef DO
netadr_t	StringToAdr (char *s)
{
	netadr_t		a;
	int			b1, b2, b3, b4, p;
	
	b1 = b2 = b3 = b4 = p = 0;
	sscanf (s, "%i.%i.%i.%i:%i", &b1, &b2, &b3, &b4, &p);
	
	a.sin_addr.S_un.S_un_b.s_b1 = b1;
	a.sin_addr.S_un.S_un_b.s_b2 = b2;
	a.sin_addr.S_un.S_un_b.s_b3 = b3;
	a.sin_addr.S_un.S_un_b.s_b4 = b4;
	a.sin_port = ntohs(p);

	return a;
}

char *idprintf( unsigned char *buf, int nLen )
{
	static char szReturn[2048];
	unsigned char c;
	char szChunk[10];
	int i;

	memset(szReturn, 0, 2048);

	for (i = 0; i < nLen; i++)
	{
		c = (unsigned char)buf[i];
		sprintf(szChunk, "%02x", c);
		strcat(szReturn, szChunk);
	}

	return szReturn;
}

/*
================
Sys_FloatTime
================
*/
double Sys_FloatTime (void)
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

    return curtime;
}


int CHLModMasterDlg::DoModal()
{
	// can be constructed with a resource template or InitModalIndirect
	ASSERT(m_lpszTemplateName != NULL || m_hDialogTemplate != NULL ||
		m_lpDialogTemplate != NULL);

	// load resource as necessary
	LPCDLGTEMPLATE lpDialogTemplate = m_lpDialogTemplate;
	HGLOBAL hDialogTemplate = m_hDialogTemplate;
	HINSTANCE hInst = AfxGetResourceHandle();
	if (m_lpszTemplateName != NULL)
	{
		hInst = AfxFindResourceHandle(m_lpszTemplateName, RT_DIALOG);
		HRSRC hResource = ::FindResource(hInst, m_lpszTemplateName, RT_DIALOG);
		hDialogTemplate = LoadResource(hInst, hResource);
	}
	if (hDialogTemplate != NULL)
		lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialogTemplate);

	// return -1 in case of failure to load the dialog template resource
	if (lpDialogTemplate == NULL)
		return -1;

	// disable parent (before creating dialog)
	HWND hWndParent = PreModal();
	AfxUnhookWindowCreate();
	CWnd* pParentWnd = CWnd::FromHandle(hWndParent);
	BOOL bEnableParent = FALSE;
	if (hWndParent != NULL && ::IsWindowEnabled(hWndParent))
	{
		::EnableWindow(hWndParent, FALSE);
		bEnableParent = TRUE;
	}

	TRY
	{
		// create modeless dialog
		AfxHookWindowCreate(this);
		if (CreateDlgIndirect(lpDialogTemplate, CWnd::FromHandle(hWndParent), hInst))
		{
			if (m_nFlags & WF_CONTINUEMODAL)
			{
				// enter modal loop
				DWORD dwFlags = MLF_SHOWONIDLE;
				if (GetStyle() & DS_NOIDLEMSG)
					dwFlags |= MLF_NOIDLEMSG;
				VERIFY(RunModalLoop(dwFlags) == m_nModalResult);
			}

			// hide the window before enabling the parent, etc.
			if (m_hWnd != NULL)
				SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW|
					SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
		}
	}
	CATCH_ALL(e)
	{
		//DELETE_EXCEPTION(e);
		m_nModalResult = -1;
	}
	END_CATCH_ALL

	if (bEnableParent)
		::EnableWindow(hWndParent, TRUE);
	if (hWndParent != NULL && ::GetActiveWindow() == m_hWnd)
		::SetActiveWindow(hWndParent);

	// destroy modal window
	DestroyWindow();
	PostModal();

	// unlock/free resources as necessary
	if (m_lpszTemplateName != NULL || m_hDialogTemplate != NULL)
		UnlockResource(hDialogTemplate);
	if (m_lpszTemplateName != NULL)
		FreeResource(hDialogTemplate);

	return m_nModalResult;
}

int CHLModMasterDlg::RunModalLoop(DWORD dwFlags)
{
	ASSERT(::IsWindow(m_hWnd)); // window must be created
	ASSERT(!(m_nFlags & WF_MODALLOOP)); // window must not already be in modal state

	// for tracking the idle time state
	BOOL bIdle = TRUE;
	LONG lIdleCount = 0;
	BOOL bShowIdle = (dwFlags & MLF_SHOWONIDLE) && !(GetStyle() & WS_VISIBLE);
	HWND hWndParent = ::GetParent(m_hWnd);
	m_nFlags |= (WF_MODALLOOP|WF_CONTINUEMODAL);
	MSG* pMsg = &AfxGetThread()->m_msgCur;

	static clock_t tLast = 0;
	clock_t tCurrent;

	// acquire and dispatch messages until the modal state is done
	for (;;)
	{
		ASSERT(ContinueModal());

		// Check on network
		ServiceMessages();

		tCurrent = clock();

		if ( (float)(tCurrent - tLast)/CLOCKS_PER_SEC > 5.0f )
		{
			tLast = tCurrent;
			UpdateCount();
		}
		
		if (!ContinueModal())
			goto ExitModal;

		// phase1: check to see if we can do idle work
		while (bIdle &&
			!::PeekMessage(pMsg, NULL, NULL, NULL, PM_NOREMOVE))
		{
			ASSERT(ContinueModal());

			// show the dialog when the message queue goes idle
			if (bShowIdle)
			{
				ShowWindow(SW_SHOWNORMAL);
				UpdateWindow();
				bShowIdle = FALSE;
			}

			// call OnIdle while in bIdle state
			if (!(dwFlags & MLF_NOIDLEMSG) && hWndParent != NULL && lIdleCount == 0)
			{
				// send WM_ENTERIDLE to the parent
				::SendMessage(hWndParent, WM_ENTERIDLE, MSGF_DIALOGBOX, (LPARAM)m_hWnd);
			}
			if ((dwFlags & MLF_NOKICKIDLE) ||
				!SendMessage(WM_KICKIDLE, MSGF_DIALOGBOX, lIdleCount++))
			{
				// stop idle processing next time
				bIdle = FALSE;
			}
		}

		// phase2: pump messages while available
		do
		{
			ASSERT(ContinueModal());

			// pump message, but quit on WM_QUIT

			if (::PeekMessage(pMsg, NULL, NULL, NULL, PM_NOREMOVE))
			{
				if (!AfxGetThread()->PumpMessage())
				{
					AfxPostQuitMessage(0);
					return -1;
				}
			}

			// show the window when certain special messages rec'd
			if (bShowIdle &&
				(pMsg->message == 0x118 || pMsg->message == WM_SYSKEYDOWN))
			{
				ShowWindow(SW_SHOWNORMAL);
				UpdateWindow();
				bShowIdle = FALSE;
			}

			if (!ContinueModal())
				goto ExitModal;

			// reset "no idle" state after pumping "normal" message
			if (AfxGetThread()->IsIdleMessage(pMsg))
			{
				bIdle = TRUE;
				lIdleCount = 0;
			}

		} while (::PeekMessage(pMsg, NULL, NULL, NULL, PM_NOREMOVE));
	}

ExitModal:
	m_nFlags &= ~(WF_MODALLOOP|WF_CONTINUEMODAL);
	return m_nModalResult;
}

void CHLModMasterDlg::ServiceMessages()
{
	static double tLastUpdateTime = (double)0;
	float fElapsed, fTimeSinceScreenUpdate;
	float fInRate = 0.0f;
	float fOutRate = 0.0f;
	struct timeval tv;
	double tCurTime;

	FD_ZERO( &fdset);
	FD_SET( net_socket, &fdset );
	
	tv.tv_sec = 0;
	tv.tv_usec = 1000;         /* seconds */
	
	if ( select (net_socket+1, &fdset, NULL, NULL, &tv) == SOCKET_ERROR )
	{
		return;
	}

	if ( FD_ISSET( net_socket, &fdset) )
		GetPacketCommand ();

	tCurTime = Sys_FloatTime();
	fElapsed = (float)(tCurTime - GetStartTime());
	if (fElapsed > 0.0f)
	{
		fInRate  = GetBytesProcessed()/fElapsed;
		fOutRate = GetOutBytesProcessed()/fElapsed;
	}

	fTimeSinceScreenUpdate = (float)(tCurTime - tLastUpdateTime);
	if ( fTimeSinceScreenUpdate > 0.2f )
	{
		char szText[64];
		sprintf(szText, "%i", (int)(m_fCycleTime - fElapsed));
		m_statTime.SetWindowText(szText);
		
		sprintf(szText, "%i", GetInTransactions());
		m_statInTransactions.SetWindowText(szText);
		
		sprintf(szText, "%i / %.2f", (int)GetBytesProcessed(), fInRate);
		m_statLoad.SetWindowText(szText);
		
		sprintf(szText, "%i / %.2f", (int)GetOutBytesProcessed(), fOutRate);
		m_statLoadOut.SetWindowText(szText);

		tLastUpdateTime = Sys_FloatTime();
	}
	
	// Reset rate when needed
	if (fElapsed >= m_fCycleTime)
		ResetTimer();
}

char	*AdrToString (netadr_t a)
{
	static	char	s[64];
	
	sprintf (s, "%i.%i.%i.%i:%i",
		a.sin_addr.S_un.S_un_b.s_b1,
		a.sin_addr.S_un.S_un_b.s_b2,
		a.sin_addr.S_un.S_un_b.s_b3,
		a.sin_addr.S_un.S_un_b.s_b4,
		ntohs(a.sin_port));

	return s;
}


BOOL	CompareAdr (netadr_t a, netadr_t b)
{
	if (a.sin_addr.s_addr == b.sin_addr.s_addr 
	&& a.sin_port == b.sin_port)
		return TRUE;
	return FALSE;
}

BOOL	CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.sin_addr.s_addr == b.sin_addr.s_addr)
		return TRUE;
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CHLModMasterDlg dialog

CHLModMasterDlg::CHLModMasterDlg(	int nMasterPort,
									BOOL bGenerateLogs,
									CString strLocalIPAddress,
									CWnd* pParent /*=NULL*/)
	: CDialog(CHLModMasterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHLModMasterDlg)
	m_bShowTraffic = TRUE;
	//}}AFX_DATA_INIT

	logfile = NULL;

	m_bShowPackets = FALSE;

	net_hostport = nMasterPort;
	m_bGenerateLogs = bGenerateLogs;
	m_strLocalIPAddress = strLocalIPAddress;

	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_tStartTime = (double)0;
	m_fBytesProcessed = 0.0f;
	m_fBytesSent = 0.0f;
	
	m_fCycleTime = 60.0f;

	m_nInTransactions = 0;
	
	modlist = NULL;
	bannedips = NULL;
	m_nUniqueID = 1;

	// Timer
	unsigned int	lowpart, highpart;
	LARGE_INTEGER	PerformanceFreq;

	if (!QueryPerformanceFrequency (&PerformanceFreq))
		Sys_Error ("No hardware timer available");

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
}

CHLModMasterDlg::~CHLModMasterDlg()
{
	FreeMods();
	FreeServers();
	if (net_socket)
		closesocket (net_socket);
	if (logfile)
		{
		fclose(logfile);
		logfile = NULL;
		}
}

void CHLModMasterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHLModMasterDlg)
	DDX_Control(pDX, IDC_LIST1, m_modList);
	DDX_Control(pDX, IDC_STATIC_LOADOUT, m_statLoadOut);
	DDX_Control(pDX, IDC_STATIC_TRANSACTIONS, m_statInTransactions);
	DDX_Control(pDX, IDC_STATIC_TIME, m_statTime);
	DDX_Control(pDX, IDC_STATIC_LOAD, m_statLoad);
	DDX_Control(pDX, IDC_STATIC_ACTIVE, m_statActive);
	DDX_Check(pDX, IDC_SHOWTRAFFIC, m_bShowTraffic);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CHLModMasterDlg, CDialog)
	//{{AFX_MSG_MAP(CHLModMasterDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_LIST, OnList)
	ON_BN_CLICKED(IDC_RELOAD_MOTD, OnReload)
	ON_LBN_DBLCLK(IDC_LIST1, OnDblclkList1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHLModMasterDlg message handlers

BOOL CHLModMasterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	NET_Init();
	Master_Init();

	// Read MOTD from MOTD.txt
	OnReload();

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strAboutMenu;
	strAboutMenu.LoadString(IDS_ABOUTBOX);
	if (!strAboutMenu.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	char szText[256];
	sprintf(szText, "Half-Life Mod Master:  %s / ("__DATE__")", AdrToString(net_local_adr ) );
	SetWindowText(szText);

	m_tStartTime = Sys_FloatTime();

	MoveControls();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CHLModMasterDlg::OnPaint() 
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
HCURSOR CHLModMasterDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CHLModMasterDlg::NET_Init()
{
	netadr_t	address;
	WORD wVersionRequested;  
	WSADATA wsaData; 
	int err;
	//
	// find the port to run on
	//
	wVersionRequested = MAKEWORD(1, 1); 
	 
	err = WSAStartup(wVersionRequested, &wsaData); 
 	if (err != 0)
	{
		Sys_Error ("NET_Init: socket: %s %i", strerror(err), err);
		return;
	}
	
	//
	// open the socket
	//
	net_socket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (net_socket == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		Sys_Error ("NET_Init: socket: %s %i", strerror(err), err);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(net_hostport);
	if( bind (net_socket, (LPSOCKADDR)&address, sizeof(address)) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		Sys_Error ("NET_Init: bind: %s %i", strerror(errno), err);
	}

	//
	// determine local name & address
	//
	NET_GetLocalAddress ();
}

void CHLModMasterDlg::Master_Init()
{
	OpenNewLogfile ();
}

/*
===============
FindModByName
===============
*/
mod_t *CHLModMasterDlg::FindModByName ( char *modname )
{
	mod_t	*mod;
	const char *pdir;
	
	for (mod = modlist ; mod ; mod=mod->next)
	{
		pdir = GetValue( mod, "gamedir" );
		if ( pdir )
		{
			if ( !stricmp( pdir, modname ) )
			{
				return mod;
			}
		}
	}
	return NULL;
}

/*
===============
FindModByID
===============
*/
mod_t *CHLModMasterDlg::FindModByID ( int id )
{
	mod_t	*mod;
	const char *pId;
	
	for (mod = modlist ; mod ; mod=mod->next)
	{
		pId = GetValue( mod, "*uniqueid" );
		if ( pId )
		{
			if ( atoi( pId ) == id )
			{
				return mod;
			}
		}
	}
	return NULL;
}

int CHLModMasterDlg::ModSendSize( mod_t *pMod )
{
	int c = 0;
	keypair_t *kp;

	if ( !pMod )
	{
		return c;
	}

	kp = pMod->keys;
	while ( kp )
	{
		c += strlen( kp->key ) + 1;
		c += strlen( kp->value ) + 1;

		kp = kp->next;
	}

	return c;
}

/*
===============
Packet_GetModBatch
===============
*/
void CHLModMasterDlg::Packet_GetModBatch (void)
{
	mod_t	*mod;
	int		i = 0;
	int     nextid;
	int savepos;
	short numsent = 0;
	keypair_t *kp;
	const char *pId;

	msg_readcount = 1;

	// Read batch Start point
	nextid = (int)MSG_ReadLong();
	if ( nextid == 0 )  // First batch
	{
		UTIL_VPrintf ("Packet_GetBatch from %s\r\n", AdrToString (packet_from));
	}

	sprintf (reply, "%c%c%c%c%c\r\n", 255,255,255,255, M2A_MODLIST );
	i += 6;

	savepos = i;
	i += sizeof( int );
	i += sizeof( short ); // Save off # written, too

	mod = modlist;
	// If nextid was not zero, find the mod after the one requested:
	if ( nextid != 0 )
	{
		for ( ; mod ; mod=mod->next)
		{
			pId = GetValue( mod, "*uniqueid" );
			if ( pId )
			{
				if ( atoi( pId ) <= nextid )
				{
					break;
				}
			}
		}
	}
	
	for ( ; mod ; mod=mod->next)
	{
		// FIXME:  Need a function to estimate size of next mod.
		if ( ( i + ModSendSize( mod ) + 1 ) > sizeof( reply ) )
			break;

		kp = mod->keys;

		while ( kp )
		{
			strcpy( reply+i, kp->key );
			i += strlen( kp->key ) + 1;
			strcpy( reply+i, kp->value );
			i += strlen( kp->value ) + 1;

			kp = kp->next;
		}

		// Now write a terminal 0
		*(unsigned char *)&reply[i] = '\0';
		i += sizeof( unsigned char );

		numsent++;
	}

	// Now write the final one we got to, if any.
	if ( mod ) // Still more in list, had to abort early
	{
		pId = GetValue( mod, "*uniqueid" );
		*(int *)&reply[ savepos ] = atoi( pId );
	}
	else // Last packet, no more mods...
	{
		*(int *)&reply[ savepos ] = 0;  // FLAG that we are done sending.
	}
	
	*( short * )&reply[ savepos + sizeof(int ) ] = numsent;

	Sys_SendPacket (&packet_from, (unsigned char *)reply, i);	
}

/*
===============
Packet_RequestMod
===============
*/
void CHLModMasterDlg::Packet_RequestMod (void)
{
	mod_t	*mod;
	int     id;
	const char *pname, *pdir;
	const char *req;
	char szNumber[ 32 ];
	int requests;

	msg_readcount = 1;

	// Read batch Start point
	id = (int)MSG_ReadLong();

	mod = FindModByID( id );
	if ( !mod )
	{
		UTIL_VPrintf ("Packet_RequestMod bad id from %s\r\n", AdrToString (packet_from));
		return;
	}

	req = GetValue( mod, "requests" );
	if ( req )
	{
		requests = atoi( req );
		requests++;
		sprintf( szNumber, "%i", requests );

		SetKey( mod, "requests", szNumber );
	}

	pname = GetValue( mod, "game" );
	if ( !pname )
	{
		pname = "?";
	}

	pdir  = GetValue( mod, "gamedir" );
	if ( !pdir )
	{
		pdir = "?";
	}

	UTIL_VPrintf ("Packet_RequestMod %s:%s from %s\r\n", pdir, pname, AdrToString (packet_from) );

}

/*
===============
PacketFilter
Discard the packet if it is from the banned list
===============
*/
BOOL CHLModMasterDlg::PacketFilter (void)
{
	banned_sv_t* banned;

	banned = bannedips;
	while ( banned )
	{
		if ( CompareBaseAdr( banned->address, packet_from ) )
			{
			UTIL_VPrintf ("banned ip packet from: %s\r\n", AdrToString(packet_from));
			return FALSE;
			}
		banned = banned->next;
	}
	return TRUE;
}

/*
===============
PacketCommand
===============
*/
void CHLModMasterDlg::PacketCommand (void)
{
	curtime = (int)Sys_FloatTime(); // time(NULL);
	static char szChannel[1024];
	
	CheckForNewLogfile ();

	if (!PacketFilter())
		return;

	if (m_bShowPackets)
		UTIL_VPrintf ("%s\r\n", packet_data);

	msg_readcount = 2;		/*  skip command and \r\n */
	
	if (packet_length > 1400)
	{
		UTIL_VPrintf ("Bad packet size: %i\r\n", packet_length);
		return;
	}
	
	switch (packet_data[0])
	{
	case A2M_SELECTMOD:
		Packet_RequestMod();
		break;
	case A2M_GETMODLIST:
		Packet_GetModBatch ();
		break;	
	default:
		UTIL_VPrintf ("Bad PacketCommand from %s\r\n", AdrToString (packet_from));
		break;
	}	

	UpdateCount();
}

/*
===============
OpenNewLogfile
===============
*/
void CHLModMasterDlg::OpenNewLogfile (void)
{
	time_t	t;
	struct	tm	*tmv;
	char	name[32];
	int		iTry;

	if ( m_bGenerateLogs == FALSE )
		return;

	t = time(NULL);
	tmv = localtime(&t);
	
	m_nCurrentDay = tmv->tm_mday;
	for (iTry = 0 ; iTry < 999 ; iTry++)
	{
		sprintf (name, "%i_%i_%i_%i.log"
		, (int)tmv->tm_mon + 1, (int)tmv->tm_mday, (int)tmv->tm_year, iTry);
		logfile = fopen (name, "r");
		if (!logfile)
			break;
	}
	if (logfile)
		Sys_Error ("Logfile creation failed");

	logfile = fopen (name, "w");
	if (!logfile)
		Sys_Error ("Logfile creation failed");

	UTIL_VPrintf ("logfile %s created\r\n", name);
}

/*
===============
CheckForNewLogfile
===============
*/
void CHLModMasterDlg::CheckForNewLogfile (void)
{
	struct	tm	*tmv;
	time_t ct;

	if ( m_bGenerateLogs == FALSE )
		return;
	
	time (&ct);
	tmv = localtime (&ct);

	if (tmv->tm_mday == m_nCurrentDay)
		return;

	UTIL_VPrintf ("changing to new day logfile\r\n");
	if (logfile)
		{
		fclose(logfile);
		logfile = NULL;
		}
	OpenNewLogfile ();
}

/*
====================
Sys_Error
====================
*/
void CHLModMasterDlg::Sys_Error (char *string, ...)
{
	va_list		argptr;

	if (net_socket)
		closesocket (net_socket);
	if (logfile)
		{
		fclose(logfile);
		logfile = NULL;
		}

	UTIL_Printf ("\r\nSys_Error:\r\n");
	va_start (argptr, string);
	vprintf (string, argptr);
	
	AfxMessageBox(string);

	UTIL_Printf ("\r\n");
	va_end (argptr);

	exit (1);
}

void CHLModMasterDlg::NET_GetLocalAddress (void)
{
	struct hostent	*h;
	char	buff[MAXHOSTNAMELEN];
	struct sockaddr_in	address;
	int		namelen;

	if ( m_strLocalIPAddress != "" )
		{
		BOOL success;
		success = NET_StringToSockaddr((LPCTSTR)m_strLocalIPAddress,(sockaddr*)(&address));
		if ( success == FALSE )
			Sys_Error ("cannot parse ip address on command line");
		net_local_adr.sin_addr = address.sin_addr;
		}
	else
		{
		gethostname(buff, MAXHOSTNAMELEN);
		buff[MAXHOSTNAMELEN-1] = 0;

		if (! (h = gethostbyname(buff)) )
			Sys_Error ("gethostbyname failed");
		if ( h->h_addr_list[1] != NULL )
			Sys_Error ("multiple local ip addresses found. must specify one on the command line.");
		*(int *)&net_local_adr.sin_addr = *(int *)h->h_addr_list[0];
		}

	namelen = sizeof(address);
	if (getsockname (net_socket, (struct sockaddr *)&address, &namelen) == SOCKET_ERROR)
		Sys_Error ("NET_Init: getsockname:", strerror(errno));
	net_local_adr.sin_port = address.sin_port;

	UTIL_Printf("IP address %s\r\n", AdrToString (net_local_adr) );
}

void CHLModMasterDlg::UTIL_VPrintf (char *msg, ...)
{
	char szText[256];
	char szOrig[512];
	va_list	argptr;
	char szTime[32];
	char szDate[64];

	va_start (argptr, msg);
	vprintf (msg, argptr);
	
	vsprintf(szOrig, msg, argptr);

	_strtime( szTime );
	_strdate( szDate );
	sprintf( szText, "%s/%s:  %s", szDate, szTime, szOrig );

	// Add int the date and time
	if ( logfile )
		fprintf (logfile, szText);
	UTIL_Printf(szText);
	va_end (argptr);
}

void CHLModMasterDlg::UTIL_Printf (char *msg, ...)
{
	UpdateData(TRUE);
	if (!m_bShowTraffic)
		return;

	char szText[256];
	va_list	argptr;
	
	va_start (argptr, msg);
	vprintf (msg, argptr);
//	if ( logfile )
//		vfprintf (logfile, msg, argptr);
	vsprintf(szText, msg, argptr);
	va_end (argptr);
		
	CEdit *pEdit;

	pEdit = (CEdit *)GetDlgItem(IDC_EDIT1);
	if (pEdit)
	{
		CString curText;
		pEdit->GetWindowText(curText);
		int nLength;

		if ((curText.GetLength() + strlen(szText)) > 16384)
		{
			curText.Empty();
			pEdit->SetWindowText(curText);
		};

		nLength = curText.GetLength();
		
		pEdit->SetSel(nLength, nLength);
		pEdit->ReplaceSel(szText);
	};
}

/*
==============
Sys_SendPacket
==============
*/
void CHLModMasterDlg::Sys_SendPacket (netadr_t *to, byte *data, int len)
{
	int ret;

	ret = sendto (net_socket, (char *)data, len, 0, (struct sockaddr *)to, sizeof(*to));
	if (ret == -1)
	{
		errno = WSAGetLastError();
		UTIL_VPrintf ("ERROR: Sys_SendPacket: (%i) %s\r\n", errno, strerror(errno));
	}

	m_fBytesSent += len;
}

char *CHLModMasterDlg::MSG_ReadString (void)
{
	char	*start;
	BOOL    bMore;
	
	start = (char *)packet_data + msg_readcount;
	
	for ( ; msg_readcount < packet_length ; msg_readcount++)
		if (((packet_data[msg_readcount] == '\r') ||
		     (packet_data[msg_readcount] == '\n'))
		|| packet_data[msg_readcount] == 0)
			break;
	
	bMore = packet_data[msg_readcount] != '\0';

	packet_data[msg_readcount] = 0;
	msg_readcount++;
	
	// skip any \r\n
	if (bMore)
	{
		while (packet_data[msg_readcount] == '\r' ||
			   packet_data[msg_readcount] == '\n')
		{
		   msg_readcount++;
		};

	}
	return start;
}

unsigned char CHLModMasterDlg::MSG_ReadByte( void )
{
	unsigned char *c;

	if ( msg_readcount >= packet_length )
	{
		UTIL_Printf( "Overflow reading byte\n" );
		return (unsigned char)-1;
	}

	c = (unsigned char *)packet_data + msg_readcount;
	msg_readcount += sizeof( unsigned char );

	return *c;
}

unsigned short CHLModMasterDlg::MSG_ReadShort( void )
{
	unsigned char *c;
	unsigned short r;

	if ( msg_readcount >= packet_length )
	{
		UTIL_Printf( "Overflow reading short\n" );
		return (unsigned short)-1;
	}

	c = (unsigned char *)packet_data + msg_readcount;
	msg_readcount += sizeof( unsigned short );

	r = *(unsigned short *)c;

	return r;
}

unsigned int CHLModMasterDlg::MSG_ReadLong( void )
{
	unsigned char *c;
	unsigned int r;

	if ( msg_readcount >= packet_length )
	{
		UTIL_Printf( "Overflow reading int\n" );
		return (unsigned int)-1;
	}

	c = (unsigned char *)packet_data + msg_readcount;
	msg_readcount += sizeof( unsigned int );

	r = *(unsigned int *)c;

	return r;
}

/*
==============
GetPacketCommand
==============
*/
void CHLModMasterDlg::GetPacketCommand (void)
{
	int		fromlen;

	fromlen = sizeof(packet_from);
	packet_length = recvfrom (net_socket, (char *)packet_data, sizeof(packet_data), 0, (struct sockaddr *)&packet_from, &fromlen);
	if (packet_length == -1)
	{
		UTIL_VPrintf ("ERROR: GetPacketCommand: %s", strerror(errno));
		return;
	}
	packet_data[packet_length] = 0;		// so it can be printed as a string

	m_fBytesProcessed += (float)packet_length;
	m_nInTransactions++;

	if (packet_length)
		PacketCommand ();
}

void CHLModMasterDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	// Resize edit control, too.	
	MoveControls();
}

double CHLModMasterDlg::GetStartTime()
{
	return m_tStartTime;
}

void CHLModMasterDlg::ResetTimer()
{
	m_tStartTime       = Sys_FloatTime();
	m_fBytesProcessed  = 0.0f;
	m_nInTransactions  = 0;
	m_fBytesSent       = 0.0f;
}

float CHLModMasterDlg::GetBytesProcessed()
{
	return m_fBytesProcessed;
}

int CHLModMasterDlg::GetInTransactions()
{
	return m_nInTransactions;
}

float CHLModMasterDlg::GetOutBytesProcessed()
{
	return m_fBytesSent;
}

void CHLModMasterDlg::UTIL_PrintMod( int idx, mod_t *mod )
{
	keypair_t *kp;

	kp = mod->keys;
	UTIL_Printf( "%5i:\r\n", idx );

	while ( kp )
	{
		UTIL_Printf( "\"%s\"\t\"%s\"\r\n", kp->key, kp->value );
		kp = kp->next;
	}
}

void CHLModMasterDlg::OnList() 
{
	mod_t	*mod;
	// curtime = Sys_FloatTime();
	int i = 1;

	// TODO: Add your control notification handler code here
	UTIL_Printf("------- Mod List -----------\r\n");
	//UTIL_Printf("#     %21s %5s \r\n", "Address", "Active");

	for ( mod = modlist ; mod ; mod = mod->next )
	{
		UTIL_PrintMod( i++, mod );
	}	

	UTIL_Printf("-------------------------------\r\n");

}

BOOL CHLModMasterDlg::MSG_ReadData(int nLength, void *nBuffer)
{
	char	*start;
	start = (char *)packet_data + msg_readcount;
	
	// Read error
	if ((msg_readcount + nLength) > packet_length)
	{
		nBuffer = NULL;
		return FALSE;
	}

	memcpy(nBuffer, start, nLength);
	msg_readcount += nLength;
	return TRUE;
}

void CHLModMasterDlg::MoveControls()
{
	CEdit *pEdit;
	CListBox *pBox;

	CRect r;

	GetClientRect( &r );

	CRect rcItem;

	int nListBottom = r.Height() - 10;

	rcItem = CRect( 10 , 120, r.Width() - 10 - 100, nListBottom );

	pEdit = (CEdit *)GetDlgItem(IDC_EDIT1);
	if (pEdit && pEdit->GetSafeHwnd())
		pEdit->MoveWindow(&rcItem, TRUE);

	rcItem = CRect( r.Width() - 10 - 100, 120, r.Width() - 10, nListBottom );

	pBox = (CListBox *)GetDlgItem( IDC_LIST1 );
	if ( pBox && pBox->GetSafeHwnd() )
		pBox->MoveWindow( &rcItem, TRUE );
}

mod_t *CHLModMasterDlg::AddMod( mod_t **pList, mod_t *pMod )
{
	const char *pname, *pdir;
	char szID[20];

	pname = GetValue( pMod, "game" );
	pdir  = GetValue( pMod, "gamedir" ); // Guaranteed
	//  allocate a new mod 
	if ( pname )
	{
		UTIL_VPrintf ("Adding: '%s'\r\n",  pname );
	}
	else
	{
		SetKey( pMod, "game", pdir );
		pname = pdir;
		UTIL_VPrintf ("Adding: gd = '%s'\r\n",  pdir );
	}
		
	// Assign the unique id.
	sprintf( szID, "%i", m_nUniqueID++);
	SetKey( pMod, "*uniqueid", szID );

	if ( !GetValue( pMod, "requests" ) )
	{
		SetKey( pMod, "requests", "0" );
	}

	pMod->next = *pList;
	*pList = pMod;

	int idx;
	idx = m_modList.AddString( pname );
	m_modList.SetItemDataPtr( idx, (void *)pMod );

	return pMod;
}

void CHLModMasterDlg::ParseMods()
{
	// Count the # of .sav files in the directory.
	WIN32_FIND_DATA wfd;
	HANDLE hResult;
	memset(&wfd, 0, sizeof(WIN32_FIND_DATA));
	char szFileName[MAX_PATH];
	char szSearchPath[MAX_PATH];

	sprintf( szSearchPath, "mods/*.mod" );
	
	hResult = FindFirstFile(szSearchPath, &wfd);

	if (hResult != INVALID_HANDLE_VALUE)
	{
		BOOL bMoreFiles;
		while (1)
		{
			sprintf( szFileName, "mods/%s", wfd.cFileName );
			ParseModFile( szFileName );
			bMoreFiles = FindNextFile(hResult, &wfd);
			if (!bMoreFiles)
				break;
		}
		
		FindClose(hResult);
	}
}

banned_sv_t *CHLModMasterDlg::AddServer( banned_sv_t **pList, netadr_t *adr )
{
	banned_sv_t *sv;

	for ( sv = *pList ; sv ; sv=sv->next )
	{
		if ( CompareAdr( *adr, sv->address ) )
		{
			break;
		}
	}
	
	if (!sv)
	{	
		//  allocate a new server 
		sv = (banned_sv_t *)malloc(sizeof(*sv));
		memset (sv, 0, sizeof(*sv));
		
		sv->next = *pList;
		sv->address = *adr;

	//  update the current data 
		*pList = sv;
	}	

	return sv;
}

void CHLModMasterDlg::ParseServers()
{
	// Read from a file.
	char *pszBuffer = NULL;
	FILE *fp;
	int nSize;

	fp = fopen("servers.lst", "rb");
	if (!fp)
		return;

	// Get filesize
	fseek ( fp, 0, SEEK_END );
	nSize = ftell ( fp );
	fseek ( fp, 0, SEEK_SET );

	pszBuffer = new char[nSize + 1];

	fread ( pszBuffer, nSize, 1, fp );
	pszBuffer[nSize] = '\0';

	fclose( fp );
	
	// Now parse server lists
	netadr_t adr;

	CToken token(pszBuffer);
	token.SetCommentMode(TRUE);  // Skip comments when parsing.

	banned_sv_t **pList;

	while ( 1 )
	{
		token.ParseNextToken();

		if (strlen(token.token) <= 0)
			break;

		if (!stricmp( token.token, "Banned" ) )
		{
			pList = &bannedips;
		}
		else
		{
			// Bogus
			AfxMessageBox("Bogus list type");
			break;
		}

		// Now parse all addresses between { }
		token.ParseNextToken();
		if (strlen(token.token) <= 0)
		{
			AfxMessageBox("Expecting {");
			break;
		}

		if ( stricmp ( token.token, "{" ) )
		{
			AfxMessageBox("Expecting {");
			break;
		}

		// Parse addresses until we get to "}"
		while ( 1 )
		{
			BOOL bIsOld = FALSE;
			banned_sv_t *pServer;

			// Now parse all addresses between { }
			token.ParseNextToken();
			if (strlen(token.token) <= 0)
			{
				AfxMessageBox("Expecting }");
				break;
			}

			if ( !stricmp( token.token, "old" ) )
			{
				bIsOld = TRUE;
				token.ParseNextToken();
			}

			if ( !stricmp ( token.token, "}" ) )
				break;

			// It's an address
			memset( &adr, 0, sizeof( netadr_t ) );
			//adr = StringToAdr( token.token );
			NET_StringToSockaddr ( token.token, (struct sockaddr *)&adr );

			pServer = AddServer( pList, &adr );
		}
	}

	delete[] pszBuffer;

}/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char com_token[1024];
BOOL com_ignorecolons;
char *COM_Parse (char *data)
{
	int             c;
	int             len;
	
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
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || (!com_ignorecolons && c==':'))
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
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || (!com_ignorecolons && c==':'))
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

const char *CHLModMasterDlg::GetValue( mod_t *pMod, const char *pszKey )
{
	if ( !pMod )
		return NULL;

	keypair_t *kp;

	kp = pMod->keys;
	while ( kp )
	{
		if ( !stricmp( kp->key, pszKey ) )
			return kp->value;

		kp = kp->next;
	}

	return NULL;
}

void CHLModMasterDlg::SetKey( mod_t *pMod, const char *pszKey, const char *pszValue )
{
	if ( !pMod )
		return;

	keypair_t *kp = pMod->keys;
	while ( kp )
	{
		if ( !stricmp( kp->key, pszKey ) )
		{
			free( kp->value );
			kp->value = strdup( pszValue );
			return;
		}
		kp = kp->next;
	}

	kp = (keypair_t *)malloc( sizeof( keypair_t ) );
	memset( kp, 0, sizeof( keypair_t ) );

	kp->key   = strdup( pszKey );
	kp->value = strdup( pszValue );

	// Link it.
	kp->next   = pMod->keys;
	pMod->keys = kp;
}

void CHLModMasterDlg::ParseModFile( char *pszFileName )
{	
	// Read from a file.
	char *pszBuffer = NULL;
	FILE *fp;
	int nSize;
	char szKey[ 256 ];

	fp = fopen( pszFileName, "rb" );
	if (!fp)
		return;

	// Get filesize
	fseek ( fp, 0, SEEK_END );
	nSize = ftell ( fp );
	fseek ( fp, 0, SEEK_SET );

	pszBuffer = new char[nSize + 1];

	fread ( pszBuffer, nSize, 1, fp );
	pszBuffer[nSize] = '\0';

	fclose( fp );
	
	// Now parse mod lists
	mod_t **pList;
	mod_t *n = ( mod_t *)malloc( sizeof( mod_t ) );
	memset( n, 0, sizeof( mod_t ) );;

	pList = &modlist;

	char *token;

	token = pszBuffer;

	while ( 1 )
	{
		// First token
		token = COM_Parse( token );
		if ( strlen( com_token ) <= 0 )
			break;

		strcpy( szKey, com_token );

		token = COM_Parse( token );

		SetKey( n, szKey, com_token );
	}

	delete[] pszBuffer;

	const char *gamedir = GetValue( n, "gamedir" );
	if ( !gamedir )
	{
		keypair_t *kp, *nextkp;
		kp = n->keys;
		while ( kp )
		{
			nextkp = kp->next;
			free( kp->key );
			free( kp->value );
			free( kp );
			kp = nextkp;
		}
		free( n );
	}
	else
	{
		SetKey( n, "*file", pszFileName );
		AddMod( pList, n );
	}
}

void CHLModMasterDlg::UpdateCount()
{
	// Now count mods and display current count:
	char szModCount[64];
	mod_t	*mod;
	int nCount = 0;

	for (mod=modlist ; mod ; mod=mod->next)
	{
		nCount++;
	}
	
	sprintf(szModCount, "%i HL mods", nCount );
	if (m_statActive.GetSafeHwnd())
		m_statActive.SetWindowText(szModCount);

}

void CHLModMasterDlg::FreeMods()
{
	mod_t	*mod, *next;
	keypair_t *kp, *nextkp;
	FILE *fp;
	const char *filename;

	mod = modlist;
	while (mod)
	{
		next = mod->next;

		fp = NULL;
		filename = GetValue( mod, "*file" );
		if ( filename )
		{
			fp = fopen( filename, "wb" );
			fprintf( fp, "// Mod Info %s\r\n", filename );
		}

		kp = mod->keys;
		while ( kp )
		{
			nextkp = kp->next;

			if ( fp && kp->key[0] != '*' )
			{
				fprintf( fp, "\"%s\"\t\"%s\"\r\n", kp->key, kp->value );
			}
			free ( kp->key );
			free ( kp->value );
			free ( kp );
			kp = nextkp;
		}

		if ( fp )
		{
			fclose( fp );
			fp = NULL;
		}

		free (mod);
		mod = next;
	}

	modlist = NULL;
}

void CHLModMasterDlg::FreeServers()
{
	banned_sv_t	*sv, *next;

	sv = bannedips;
	while (sv)
	{
		next = sv->next;
		free (sv);
		sv = next;
	}
}

void CHLModMasterDlg::OnReload()
{
	FreeMods();
	FreeServers();

	m_modList.ResetContent();
	
	ParseMods();
	ParseServers();
}

void CHLModMasterDlg::OnDblclkList1() 
{
	mod_t *pmod;
	int idx;

	idx = m_modList.GetCurSel();
	if ( idx == LB_ERR )
		return;

	pmod = (mod_t *)m_modList.GetItemDataPtr( idx );
	if ( !pmod )
		return;

	UTIL_PrintMod( 1, pmod );
}
