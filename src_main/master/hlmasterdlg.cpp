// HLMasterDlg.cpp : implementation file
//
#pragma warning( disable : 4706 )

#include "stdafx.h"
#include <afxpriv.h>
#include "proto_oob.h"
#include "HLMasterProtocol.h"
#include "HLMaster.h"
#include "HLMasterDlg.h"
#include "info.h"
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

int CURRENT_PROTOCOL = PROTOCOL_VERSION;

#define LOAD_MIN		2000
#define SV_PER_PKT ( 1500 / 6 )
#define MAX_PACKETS_ALLOWED 8

#include "archtypes.h"     // DAL
#include "crc.h"

//-----------------------------------------------------------------------------
// Purpose: Compute a crc for the value
// Input  : *value - 
// Output : inline int
//-----------------------------------------------------------------------------
inline int CriteriaHash( const char *value, int length )
{
	CRC32_t val;
	CRC32_Init( &val );
	CRC32_ProcessBuffer(&val, (void *)value, length );
	return (int)CRC32_Final( val );
}

void SetCriteria( string_criteria_t *c, const char *value )
{
	strncpy( c->value, value, VALUE_LENGTH -1 );
	c->value[ VALUE_LENGTH-1 ] =0;

	strlwr( c->value );

	// compute length
	c->length = strlen( c->value );

	// compute checksum ( use use the mod hash stuff )
	c->checksum = CriteriaHash( c->value, c->length );
}

char *va(char *fmt, ...)
{
	static char	string[2048];
	va_list	argptr;
	
	va_start (argptr, fmt);
	vsprintf (string, fmt, argptr);
	va_end (argptr);

	return string;
}

char *CheckParm(const char *psz, char **ppszValue)
{
	static char sz[128];
	char *pret;

	char cmdline[ 1024 ];
	strcpy( cmdline, GetCommandLine() );

	pret = strstr( cmdline, psz );

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
	
	a.sin_addr.S_un.S_un_b.s_b1 = (unsigned char)b1;
	a.sin_addr.S_un.S_un_b.s_b2 = (unsigned char)b2;
	a.sin_addr.S_un.S_un_b.s_b3 = (unsigned char)b3;
	a.sin_addr.S_un.S_un_b.s_b4 = (unsigned char)b4;
	a.sin_port = ntohs((unsigned short)p);
	a.sin_family = AF_INET;

	return a;
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


void CHLMasterDlg::Peer_GetHeartbeat( void )
{
	// Read in an ip address as 6 bytes
	unsigned char ip[4];
	unsigned short port;
	int i;
	int nusers;
	char *pgamedir = NULL;
	char *pinfo = NULL;
	char gamedir [ 1024 ];
	char szAddress[ 256 ];
	sv_t *sv;
	netadr_t svadr;
	BOOL bNewServer = FALSE;

	for ( i = 0; i < 4; i++ )
		ip[i] = MSG_ReadByte();

	port = ntohs((unsigned short)MSG_ReadShort());

	sprintf( szAddress, "%i.%i.%i.%i:%i", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3], (int)port );
	
	svadr = StringToAdr( szAddress );

	nusers = (int)MSG_ReadShort();

	sprintf( gamedir, "valve" );
	
	pgamedir = MSG_ReadString();
	if ( pgamedir && pgamedir[0] )
		strcpy( gamedir, pgamedir );

	strlwr( gamedir );

	pinfo = MSG_ReadString();

	// Find it in the list
	sv = FindServerByAddress( &svadr );

	if (!sv)
	{	
		int h;
		sv = (sv_t *)malloc(sizeof(*sv));
		memset (sv, 0, sizeof(*sv));
		sv->address = svadr;

		// Assign the unique id.
		sv->uniqueid = m_nUniqueID++;

		h = HashServer( &svadr );

		sv->next = servers[ h ];
		servers[ h ] = sv;
		bNewServer = TRUE; // Try and send it the master's public key
	}

	sv->time = (int)Sys_FloatTime();

	SetCriteria( &sv->gamedir, gamedir );
	
	sv->players = nusers;
	
	strcpy( sv->info, pinfo );
	sv->info_length = strlen( sv->info ) + 1;
}

void CHLMasterDlg::Peer_GetHeartbeat2( void )
{
	// Read in an ip address as 6 bytes
	unsigned char ip[4];
	unsigned short port;
	netadr_t svadr;
	char szAddress[ 256 ];
	static char szSName[2048];
	
	sv_t	*sv;
	int num;
	int maxplayers;
	int bots;

	int i;
	int challenge;
	char gamedir[ 64 ];
	char map[ 64 ];
	char os[ 2 ];
	int  dedicated;
	int	 password;
	int	 secure;
	int	 islan = 0;
	char	proxyaddress[ 128 ];
	int isproxytarget;
	int isproxy = 0;

	char info[ MAX_SINFO ];

	int protocol;

	//UpdateData( TRUE );

	for ( i = 0; i < 4; i++ )
	{
		ip[i] = MSG_ReadByte();
	}

	port = ntohs((unsigned short)MSG_ReadShort());

	sprintf( szAddress, "%i.%i.%i.%i:%i", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3], (int)port );
	
	svadr = StringToAdr( szAddress );

	strncpy( info, MSG_ReadString(), 2047 );
	info[ 2047 ] = '\0';

	if ( !info || !info[0] )
	{
		return;
	}

	protocol   = atoi( Info_ValueForKey( info, "protocol" ) );
	challenge  = atoi( Info_ValueForKey( info, "challenge" ) );
	num		   = atoi( Info_ValueForKey( info, "players" ) );
	maxplayers = atoi( Info_ValueForKey( info, "max" ) );
	bots	   = atoi( Info_ValueForKey( info, "bots" ) );
	dedicated  = atoi( Info_ValueForKey( info, "dedicated" ) );
	password   = atoi( Info_ValueForKey( info, "password" ) );
	secure	   = atoi( Info_ValueForKey( info, "secure" ) );
	strncpy( gamedir, Info_ValueForKey( info, "gamedir" ), 63 );
	strncpy( map, Info_ValueForKey( info, "map" ), 63 );
	strncpy( os, Info_ValueForKey( info, "os" ), 1 );
	islan = atoi( Info_ValueForKey( info, "lan" ) );
	isproxy = atoi( Info_ValueForKey( info, "proxy" ) ) ? 1 : 0;
	isproxytarget = atoi( Info_ValueForKey( info, "proxytarget" ) ) ? 1 : 0;
	strncpy( proxyaddress, Info_ValueForKey( info, "proxyaddress" ), 127 );

	proxyaddress[ 127 ] = 0;
	gamedir[ 63 ]	= 0;
	map[ 63 ]		= 0;
	os[ 1 ]			= 0;

	strlwr( gamedir );
	strlwr( map );
	strlwr( os );

	// protocol != 1 for Sony stand-alone game support...1.1.1.0 engine license (EricS)
	if ( !islan && !m_bAllowOldProtocols && ( protocol != CURRENT_PROTOCOL ) && ( protocol != 1 ) ) 
	{
		return;
	}

	/*  find the server */
	sv = FindServerByAddress( &svadr );
	if (!sv)
	{
		int h;
		
		sv = (sv_t *)malloc(sizeof(*sv));
		memset (sv, 0, sizeof(*sv));
		
		sv->address = svadr;

		// Assign the unique id.
		sv->uniqueid = m_nUniqueID++;

		h = HashServer( &svadr );

		sv->next = servers[ h ];
		servers[ h ] = sv;
	}

	/*  update the current data */
	sv->time = m_curtime;

	SetCriteria( &sv->gamedir, gamedir );
	SetCriteria( &sv->map, map );
	strcpy( sv->os, os );
	
	sv->players		= num;
	sv->max			= maxplayers;
	sv->bots		= bots;
	sv->password	= password;
	sv->dedicated	= dedicated;
	sv->secure		= secure;
	strcpy( sv->info, info );
	sv->info_length = strlen( sv->info ) + 1;
	sv->islan		= islan;
	sv->isproxy		= isproxy;
	sv->isProxyTarget = isproxytarget;
	if ( isproxytarget )
	{
		SetCriteria( &sv->proxyTarget, proxyaddress );
	}
}

void CHLMasterDlg::Peer_GetShutdown( void )
{
	unsigned char ip[4];
	unsigned short port;
	int i;
	char szAddress[ 256 ];
	netadr_t svadr;

	for ( i = 0; i < 4; i++ )
		ip[i] = MSG_ReadByte();

	port = ntohs((unsigned short)MSG_ReadShort());

	sprintf( szAddress, "%i.%i.%i.%i:%i", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3], (int)port );
	
	svadr = StringToAdr( szAddress );

	sv_t	*sv, **svp;

	int h;

	h = HashServer( &svadr );

	svp = &servers[h];

	while (1)
	{
		sv = *svp;
		if (!sv)
		{
			break;
		}
		if ( CompareAdr(svadr, sv->address ) )
		{
			*svp = sv->next;
			free (sv);
			return;
		}
		else
		{
			svp = &sv->next;
		}
	}
}

void CHLMasterDlg::Peer_Heartbeat( sv_t *sv )
{
	// Send it to other masters
	unsigned char msgbuf[ 2048 ], *pch;
	int i = 0;
	sv_t *masters;

	// Construct the appropriate message:
	*( unsigned char *)&msgbuf[i] = M2M_MSG;
	i += sizeof( unsigned char );

	*( unsigned char *)&msgbuf[i] = PEER_HEARTBEAT;
	i += sizeof( unsigned char );

	// Now send the ip address
	*( unsigned int *)&msgbuf[i] = sv->address.sin_addr.s_addr;
	i += sizeof ( unsigned int );

	*( unsigned short *)&msgbuf[i] = sv->address.sin_port;
	i += sizeof( unsigned short );

	// Now write # of users
	*( unsigned short *)&msgbuf[i] = (unsigned short)sv->players;
	i += sizeof( unsigned short );

	// Now write the basedir
	pch = (unsigned char *)sv->gamedir.value;

	while ( *pch )
	{
		*( unsigned char *)&msgbuf[i++] = *pch++;
	}

	// Terminal zero
	*( unsigned char *)&msgbuf[i++] = '\0';

	// Now write the info string
	pch = (unsigned char *)sv->info;
	while ( *pch )
	{
		*( unsigned char *)&msgbuf[i++] = *pch++;
	}

	// Terminal zero
	*( unsigned char *)&msgbuf[i++] = '\0';

	// Send to all masters
	masters = masterservers;
	while( masters )
	{
		if ( !CompareAdr( masters->address, net_local_adr ) )
		{
			Sys_SendPacket ( &masters->address, (unsigned char *)msgbuf, i );
		}
		masters = masters->next;
	}
}

void CHLMasterDlg::Peer_Heartbeat2( sv_t *sv )
{
	// Send it to other masters
	unsigned char msgbuf[ 2048 ], *pch;
	int i = 0;
	sv_t *masters;

	// Construct the appropriate message:
	*( unsigned char *)&msgbuf[i] = M2M_MSG;
	i += sizeof( unsigned char );

	*( unsigned char *)&msgbuf[i] = PEER_HEARTBEAT2;
	i += sizeof( unsigned char );

	// Now send the ip address
	*( unsigned int *)&msgbuf[i] = sv->address.sin_addr.s_addr;
	i += sizeof ( unsigned int );

	*( unsigned short *)&msgbuf[i] = sv->address.sin_port;
	i += sizeof( unsigned short );

	// Now write # of users
	// Now write raw input
	pch = (unsigned char *)sv->info;
	while ( *pch )
	{
		*( unsigned char *)&msgbuf[i++] = *pch++;
	}

	// Terminal zero
	*( unsigned char *)&msgbuf[i++] = '\0';

	// Send to all masters
	masters = masterservers;
	while( masters )
	{
		if ( !CompareAdr( masters->address, net_local_adr ) )
		{
			Sys_SendPacket ( &masters->address, (unsigned char *)msgbuf, i );
		}
		masters = masters->next;
	}
}

void CHLMasterDlg::Peer_Shutdown( sv_t *sv )
{
	// Send it to other masters
	unsigned char msgbuf[ 256 ];
	int i = 0;
	sv_t *masters;

	// Construct the appropriate message:
	*( unsigned char *)&msgbuf[i] = M2M_MSG;
	i += sizeof( unsigned char );

	*( unsigned char *)&msgbuf[i] = PEER_SHUTDOWN;
	i += sizeof( unsigned char );

	// Now send the ip address
	*( unsigned int *)&msgbuf[i] = sv->address.sin_addr.s_addr;
	i += sizeof ( unsigned int );

	*( unsigned short *)&msgbuf[i] = sv->address.sin_port;
	i += sizeof( unsigned short );

	// Send to all masters
	masters = masterservers;
	while( masters )
	{
		if ( !CompareAdr( masters->address, net_local_adr ) )
			Sys_SendPacket ( &masters->address, (unsigned char *)msgbuf, i );
		masters = masters->next;
	}
}

void CHLMasterDlg::Packet_GetPeerMessage( void )
{
	unsigned char b;
	sv_t *masters;
	BOOL bValid = FALSE;

	// The only messages we are about are
	// PEER_HEARTBEAT && PEER_SHUTDOWN
	msg_readcount = 1;

	masters = masterservers;
	while ( masters )
	{
		if ( CompareBaseAdr( masters->address, packet_from ) )
		{
			bValid = TRUE;
			break;
		}
		masters = masters->next;
	}

#if !defined( _DEBUG )
	if ( !bValid )
	{
		UTIL_VPrintf( "M2M_MSG from unrecognized address %s\r\n", AdrToString( packet_from ) );
		return;
	}
#endif

	// Validate the incoming address
	// Now read the next byte
	b = MSG_ReadByte();
	switch ( b )
	{
	case PEER_HEARTBEAT:
		Peer_GetHeartbeat();
		break;
	case PEER_HEARTBEAT2:
		Peer_GetHeartbeat2();
		break;
	case PEER_SHUTDOWN:
		Peer_GetShutdown();
		break;
	default:
		UTIL_VPrintf( "unknown peer message %i from %s\r\n", (int)b, AdrToString( packet_from ) );
		break;
	}
}

int CHLMasterDlg::DoModal()
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

int CHLMasterDlg::RunModalLoop(DWORD dwFlags)
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

		if ( (float)(tCurrent - tLast)/CLOCKS_PER_SEC > 60.0f )
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

void CHLMasterDlg::ServiceMessages()
{
	static double tLastUpdateTime = (double)0;
	float fElapsed, fTimeSinceScreenUpdate;
	float fInRate = 0.0f;
	float fOutRate = 0.0f;
	struct timeval tv;

	m_curtime = (int)Sys_FloatTime();

	TimeoutServers();

	FD_ZERO( &fdset);
	FD_SET( net_socket, &fdset );
	
	tv.tv_sec = 0;
	tv.tv_usec = 10000;         /* seconds */
	
	if ( select (net_socket+1, &fdset, NULL, NULL, &tv) == SOCKET_ERROR )
	{
		return;
	}

	if ( FD_ISSET( net_socket, &fdset) )
		GetPacketCommand ();

	fElapsed = (float)(m_curtime - GetStartTime() );
	if (fElapsed > 0.0f)
	{
		fInRate  = GetBytesProcessed()/fElapsed;
		fOutRate = GetOutBytesProcessed()/fElapsed;
	}

	fTimeSinceScreenUpdate = (float)( m_curtime - tLastUpdateTime);
	if ( fTimeSinceScreenUpdate > 3.0f )
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

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHLMasterDlg dialog

CHLMasterDlg::CHLMasterDlg( int nMasterPort, 
					BOOL bGenerateLogs,
					CString strLocalIPAddress,
					CWnd* pParent /*=NULL*/)
	: CDialog(CHLMasterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHLMasterDlg)
	m_bShowTraffic = TRUE;
	m_bAllowOldProtocols = FALSE;
	//}}AFX_DATA_INIT

	logfile = NULL;

	strcpy( m_szHLVersion, "1.0.1.0" );
	strcpy( m_szCSVersion, "1.0.0.0" );
	strcpy( m_szTFCVersion, "1.0.0.0" );
	strcpy( m_szDMCVersion, "1.0.0.0" );
	strcpy( m_szOpForVersion, "1.0.0.0" );
	strcpy( m_szRicochetVersion, "1.0.0.0" );
	strcpy( m_szDODVersion, "1.0.0.0" );

	m_nLanServerCount = 0;
	m_nLanUsers = 0;
	m_nServerCount = 0;
	m_nUsers = 0;

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
	
	authservers = NULL;
	titanservers = NULL;
	masterservers = NULL;
	memset( servers, 0, SERVER_HASH_SIZE * sizeof( sv_t * ) );
	bannedips = NULL;

	memset( mods, 0, MOD_HASH_SIZE * sizeof( modsv_t * ) );

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

	m_curtime = (int)Sys_FloatTime();
}

CHLMasterDlg::~CHLMasterDlg()
{
	FreeServers();
	FreeGameServers();
	FreeMods();
	if (net_socket)
	{
		closesocket (net_socket);
	}
	if (logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}
}

void CHLMasterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHLMasterDlg)
	DDX_Control(pDX, IDC_MOTD, m_hEditMOTD);
	DDX_Control(pDX, IDC_STATIC_LOADOUT, m_statLoadOut);
	DDX_Control(pDX, IDC_STATIC_TRANSACTIONS, m_statInTransactions);
	DDX_Control(pDX, IDC_STATIC_TIME, m_statTime);
	DDX_Control(pDX, IDC_STATIC_LOAD, m_statLoad);
	DDX_Control(pDX, IDC_STATIC_ACTIVE, m_statActiveInternet);
	DDX_Control(pDX, IDC_STATIC_ACTIVE2, m_statActiveLan);
	DDX_Control(pDX, IDC_STATIC_ACTIVE3, m_statActiveProxy);
	DDX_Control(pDX, IDC_STATIC_ACTIVE4, m_statActiveTotal);
	DDX_Check(pDX, IDC_SHOWTRAFFIC, m_bShowTraffic);
	DDX_Check(pDX, IDC_ALLOWOLDPROTOCOLS, m_bAllowOldProtocols);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CHLMasterDlg, CDialog)
	//{{AFX_MSG_MAP(CHLMasterDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_LIST, OnList)
	ON_BN_CLICKED(IDC_RELOAD_MOTD, OnReloadMotd)
	ON_BN_CLICKED(IDC_COMPUTESTATS, OnComputestats)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHLMasterDlg message handlers

BOOL CHLMasterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	NET_Init();
	Master_Init();

	// Add "About..." menu item to system menu.
	m_hEditMOTD.SetWindowText("No MOTD");
	// Read MOTD from MOTD.txt

	OnReloadMotd();

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

#if defined( _STEAM_HLMASTER )
	sprintf(szText, "Steam HLMaster %i/HL:%s/CS:%s/TFC:%s/DMC:%s/DOD:%s/Ricochet:%s/OpFor:%s", CURRENT_PROTOCOL, m_szHLVersion, m_szCSVersion, m_szTFCVersion, m_szDMCVersion, m_szDODVersion, m_szRicochetVersion, m_szOpForVersion );
#else
	sprintf(szText, "HL Master %s - %i/HL:%s/CS:%s ("__DATE__")", AdrToString(net_local_adr ), CURRENT_PROTOCOL, m_szHLVersion, m_szCSVersion );	
#endif

	SetWindowText(szText);

	m_tStartTime = Sys_FloatTime();

	MoveControls();

	if ( strstr( GetCommandLine(), "-fake" ) )
	{
		GenerateFakeServers();
	}

	UpdateCount();

	CWnd *wnd = GetDlgItem( IDC_COMPUTESTATS );

	if ( wnd )
	{
		wnd->SetFocus();
		return FALSE;
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CHLMasterDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CHLMasterDlg::OnPaint() 
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
HCURSOR CHLMasterDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CHLMasterDlg::NET_Init()
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
	address.sin_port = htons((unsigned short)net_hostport);
	if( bind (net_socket, (LPSOCKADDR)&address, sizeof(address)) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		Sys_Error ("NET_Init: bind to %i failed: %s %i", net_hostport, strerror(errno), err);
	}

	//
	// determine local name & address
	//
	NET_GetLocalAddress ();
}

void CHLMasterDlg::Master_Init()
{
	OpenNewLogfile ();
	ParseVersion();
	ParseServers();
}

/*
===============
TimeoutServers
Free any servers that haven't reported in last twelve minutes
===============
*/
void CHLMasterDlg::TimeoutServers (void)
{
	sv_t	*sv, **svp;
	
	static double tLastTimeOut = (double)0;
	double current;

	current = Sys_FloatTime();

	// Don't do this too often.
	if ( ( current - tLastTimeOut) < MIN_SV_TIMEOUT_INTERVAL)
		return;

	tLastTimeOut = current;

	int h;

	for ( h = 0; h < SERVER_HASH_SIZE; h++ )
	{
		svp = &servers[ h ];
		while (1)
		{
			sv = *svp;
			if (!sv)
				break;
			if ( ( current - sv->time ) < SERVER_TIMEOUT)
			{
				svp = &sv->next;
				continue;
			}
			*svp = sv->next;
			free (sv);
		}
	}
}

/*
===============
Nack
===============
*/
void CHLMasterDlg::Nack (char *comment, ...)
{
	char	string[2048];
	va_list	argptr;
	
	va_start (argptr, comment);
	vsprintf (string, comment, argptr);
	va_end (argptr);

	sprintf (reply, "%c\r\n%s", A2A_NACK, string);
	Sys_SendPacket (&packet_from, (unsigned char *)reply, strlen(reply)+1);
}


/*
================
RejectConnection(char *)

Rejects connection request and sends back a message
================
*/
void CHLMasterDlg::RejectConnection(netadr_t *adr, char *pszMessage)
{
	sprintf (reply, "%c%c%c%c%c\n%s\n", 255,255,255,255, A2C_PRINT, pszMessage);
	Sys_SendPacket( adr, (unsigned char *)reply, strlen(pszMessage)+7);
}

/*
================
RequestRestart(netadr_t *adr)

Rejects connection request with M2S_REQUESTRESTART message
================
*/
void CHLMasterDlg::RequestRestart(netadr_t *adr)
{
	sprintf (reply, "%c%c%c%c%c\r\n", 255,255,255,255, M2S_REQUESTRESTART);
	Sys_SendPacket (adr, (unsigned char *)reply, strlen(reply)+1);	
}

/*
=================
GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void CHLMasterDlg::Packet_GetChallenge (void)
{
	int		i;
	int		oldest;
	int		oldestTime;
	int     n;

	oldest = 0;
	oldestTime = 0x7fffffff;

    srand( (unsigned)time( NULL ) );

	msg_readcount = 1;

	int bucket = packet_from.sin_addr.S_un.S_un_b.s_b4;
	//int bucket = packet_from.sin_port & 0xff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (CompareBaseAdr (packet_from, challenges[bucket][i].adr))
			break;
		if (challenges[bucket][i].time < oldestTime)
		{
			oldestTime = challenges[bucket][i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		challenges[bucket][oldest].challenge = ( ( rand() << 16 ) | rand() ) & ~(1<<31);
		challenges[bucket][oldest].adr       = packet_from;
		challenges[bucket][oldest].time      = m_curtime;
		i = oldest;
	}
	// Issue a new Challenge #
	else if ( (m_curtime - challenges[bucket][i].time) > HB_TIMEOUT )
	{
		challenges[bucket][i].challenge = ( ( rand() << 16 ) | rand() ) & ~(1<<31);
		challenges[bucket][i].adr       = packet_from;
		challenges[bucket][i].time      = m_curtime;
	}

	sprintf(reply, "%c%c%c%c%c\n",
		255, 255, 255, 255, 
		M2A_CHALLENGE);
	
	n = 6;
	*(unsigned int *)&reply[n] = challenges[bucket][i].challenge;

	n += sizeof( unsigned int );
	
	Sys_SendPacket (&packet_from, (unsigned char *)reply, n);
}

void CHLMasterDlg::ListMods( void )
{
	modsv_t *p;
	int i = 1;
	int h;

	UTIL_Printf( "MODS ---------\r\n" );

	for ( h = 0; h < MOD_HASH_SIZE; h++ )
	{
		for ( p = mods[ h ]; p ; p = p->next, i++ )
		{
			UTIL_Printf( "%i %s\r\n",
				i, p->gamedir );
			UTIL_Printf( "   internet  %7i sv %7i pl %7i b %7i bs\r\n",
				p->ip_servers, p->ip_players, p->ip_bots, p->ip_bots_servers );
			UTIL_Printf( "   lan       %7i sv %7i pl %7i b %7i bs\r\n",
				p->lan_servers, p->lan_players, p->lan_bots, p->lan_bots_servers);
			UTIL_Printf( "   proxy     %7i sv %7i pl\r\n",
				p->proxy_servers, p->proxy_players );
		}
	}
}

void CHLMasterDlg::FreeMods( void )
{
	int h;
	modsv_t *p, *n;

	for ( h = 0; h < MOD_HASH_SIZE; h++ )
	{
		p = mods[h];
		while ( p )
		{
			n = p->next;
			free( p );
			p = n;
		}
		mods[h] = NULL;
	}
}

modsv_t *CHLMasterDlg::FindMod( const char *pgamedir )
{
	modsv_t *p;
	int h;

	if ( !pgamedir || !pgamedir[0] )
		return NULL;

	h = HashMod( pgamedir );
	p = mods[h];
	while ( p )
	{
		if ( !stricmp( pgamedir, p->gamedir ) )
			return p;

		p = p->next;
	}

	return NULL;
}

int CHLMasterDlg::HashMod( const char *pgamedir )
{
	// compute a hash value
	int c = 0;
	char szLowerCaseDir[ 1024 ];
	char *p;

	strcpy( szLowerCaseDir, pgamedir );
	strlwr( szLowerCaseDir );

	p = ( char * )szLowerCaseDir;
	while ( *p )
	{
		c += ( *p << 1 );
		c ^= *p;
		p++;
	}
	c = c % MOD_HASH_SIZE;
	return c;
}

// Loop through active server list and reset all mod counts
void CHLMasterDlg::AdjustCounts( void )
{
	modsv_t *s;
	sv_t *sv;
	int h;

	// First clear current mod counts;
	FreeMods();

	int sh;

	for ( sh = 0; sh < SERVER_HASH_SIZE; sh++ )
	{
		// Now go through servers and find all of the mods.
		sv = servers[ sh ];
		while ( sv )
		{
			s = FindMod( sv->gamedir.value );
			if ( !s )
			{
				// Add it
				h = HashMod( sv->gamedir.value );

				s = ( modsv_t * ) malloc( sizeof( modsv_t ) );
				memset( s, 0, sizeof( modsv_t ) );

				strcpy( s->gamedir, sv->gamedir.value );

				s->next = mods[h];
				mods[h] = s;
			}

			if ( sv->islan )
			{
				s->lan_servers++;
				s->lan_players += (sv->players - sv->bots);
				s->lan_bots += sv->bots;
				if ( sv->bots )
				{
					s->lan_bots_servers++;
				}
			}
			else if ( sv->isproxy )
			{
				s->proxy_servers++;
				s->proxy_players += (sv->players - sv->bots);
			}
			else
			{
				s->ip_servers++;
				s->ip_players += (sv->players - sv->bots);
				s->ip_bots += sv->bots;
				if ( sv->bots )
				{
					s->ip_bots_servers++;
				}
			}

			sv = sv->next;
		}
	}
}

/*
===============
Packet_Heartbeat
===============
*/
void CHLMasterDlg::Packet_Heartbeat (void)
{
	static char szSName[2048];
	
	sv_t	*sv;
	unsigned		num;
	int		seq;
	int nChallengeValue;
	char *gamedir;

	char *info;

	BOOL bNewServer = FALSE;
	int nprotocol;

	//UpdateData( TRUE );

	nChallengeValue = atoi(MSG_ReadString());
	
	seq = atoi(MSG_ReadString());

	//  read current users
	num = atoi(MSG_ReadString());

	nprotocol = atoi(MSG_ReadString()); // Throw away

	gamedir = MSG_ReadString();

	info = MSG_ReadString();

	if ( !m_bAllowOldProtocols && ( nprotocol != CURRENT_PROTOCOL ) )
	{
		RejectConnection(&packet_from, "Outdated protocol.");
		return;
	}

	int challenge_result = CheckChallenge( nChallengeValue, &packet_from );

	switch ( challenge_result )
	{
	case 1:
		RejectConnection(&packet_from, "Bad challenge.");
		return;
	case 2:
		RejectConnection(&packet_from, "No challenge for your address.");
		return;
	case 0:
	default:
		break;
	}

/*  find the server */
	sv = FindServerByAddress( &packet_from );

	if (!sv)
	{
		int h;
		sv = (sv_t *)malloc(sizeof(*sv));
		memset (sv, 0, sizeof(*sv));
		sv->address = packet_from;

		// Assign the unique id.
		sv->uniqueid = m_nUniqueID++;

		h = HashServer( &packet_from );

		sv->next = servers[ h ];
		servers[ h ] = sv;
		bNewServer = TRUE; // Try and send it the master's public key
	}

	sv->time = m_curtime;

	if ( gamedir && gamedir[0] && ( strlen( gamedir ) <= 254 ) )
	{
		SetCriteria( &sv->gamedir, gamedir );
	}
	else
	{
		SetCriteria( &sv->gamedir, "" );
	}

	sv->players = num;

	if ( strlen( info ) + 1 < sizeof( sv->info ) )
	{
		strcpy( sv->info, info );
	}
	else
	{
		strcpy( sv->info, "" );
	}
	sv->info_length = strlen( sv->info ) + 1;

	Peer_Heartbeat( sv );
}

int CHLMasterDlg::CheckChallenge( int challenge, netadr_t *adr )
{
	int i;

	int bucket = adr->sin_addr.S_un.S_un_b.s_b4;
	//int bucket = adr->sin_port & 0xff;

	// Don't care if it is a local address.
	for (i=0 ; i < MAX_CHALLENGES ; i++)
	{
		if (CompareBaseAdr ( *adr, challenges[bucket][i].adr ) )
		{
			if (challenge == challenges[bucket][i].challenge)
			{
				return 0;
			}
			else
			{
				return 1;
			}
			break;
		}
	}

	if (i >= MAX_CHALLENGES)
	{
		return 2;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLMasterDlg::Packet_Heartbeat2 (void)
{
	static char szSName[2048];
	
	sv_t	*sv;
	int num;
	int maxplayers;
	int bots;

	int challenge;
	char gamedir[ 64 ];
	char map[ 64 ];
	char os[ 2 ];
	int  dedicated;
	int	 password;
	int	 secure;
	int islan = 0;
	char	proxyaddress[ 128 ];
	int isproxytarget;
	int isproxy = 0;
	char version[ 32 ];

	char info[ MAX_SINFO ];

	BOOL bNewServer = FALSE;
	int protocol;

	//UpdateData( TRUE );

	strncpy( info, MSG_ReadString(), 2047 );
	info[ 2047 ] = '\0';

	if ( !info || !info[0] )
	{
		RejectConnection(&packet_from, "Outdated protocol.");
		return;
	}

	protocol   = atoi( Info_ValueForKey( info, "protocol" ) );
	challenge  = atoi( Info_ValueForKey( info, "challenge" ) );
	num		   = atoi( Info_ValueForKey( info, "players" ) );
	maxplayers = atoi( Info_ValueForKey( info, "max" ) );
	bots	   = atoi( Info_ValueForKey( info, "bots" ) );
	dedicated  = atoi( Info_ValueForKey( info, "dedicated" ) );
	password   = atoi( Info_ValueForKey( info, "password" ) );
	secure	   = atoi( Info_ValueForKey( info, "secure" ) );
	strncpy( gamedir, Info_ValueForKey( info, "gamedir" ), 63 );
	strncpy( map, Info_ValueForKey( info, "map" ), 63 );
	strncpy( os, Info_ValueForKey( info, "os" ), 1 );
	islan = atoi( Info_ValueForKey( info, "lan" ) );
	isproxy = atoi( Info_ValueForKey( info, "proxy" ) ) ? 1 : 0;
	isproxytarget = atoi( Info_ValueForKey( info, "proxytarget" ) ) ? 1 : 0;
	strncpy( proxyaddress, Info_ValueForKey( info, "proxyaddress" ), 127 );
	strncpy( version, Info_ValueForKey( info, "version" ), 31 );

	proxyaddress[ 127 ] = 0;
	gamedir[ 63 ]	= 0;
	map[ 63 ]		= 0;
	os[ 1 ]			= 0;
	version[ 31 ]   = 0;

	strlwr( gamedir );
	strlwr( map );
	strlwr( os );

	// protocol != 1 for Sony stand-alone game support...1.1.1.0 engine license (EricS)
	if ( !m_bAllowOldProtocols && ( protocol != CURRENT_PROTOCOL ) && ( protocol != 1 ) )
	{
		RejectConnection(&packet_from, "Outdated protocol.");

		if ( !islan )
		{
			return;
		}
	}

	// everyone in the current release sends up a version, so we know they're outdated if we don't get one.
	if ( version[0] == 0 )
	{
		RejectConnection(&packet_from, "Outdated Version. Please update your server.");
		return;
	}

#if defined( _STEAM_HLMASTER )

	// We need to check the version of the server based on the mod it's running.
	int reject = 0;

	// If they didn't send a version we already know they're out of date.
	if ( version[0] == 0 )
	{
		reject = 1;
	}
	else
	{
		// Steam appends its own version to the Mod version...we need to remove it.
		if ( strstr( version, "/" ) )
		{
			version[ strcspn( version, "/" ) ] = '\0';
		}

		// Check the version against the Mod version
		if ( !stricmp( gamedir, "valve" ) )
		{
			if ( stricmp( version, m_szHLVersion ) )
			{
				reject = 1;
			}
		}
		else if ( !stricmp( gamedir, "cstrike" ) )
		{
			if ( stricmp( version, m_szCSVersion ) )
			{
				reject = 1;
			}
		}
		else if ( !stricmp( gamedir, "tfc" ) )
		{
			if ( stricmp( version, m_szTFCVersion ) )
			{
				reject = 1;
			}
		}
		else if ( !stricmp( gamedir, "dmc" ) )
		{
			if ( stricmp( version, m_szDMCVersion ) )
			{
				reject = 1;
			}
		}
		else if ( !stricmp( gamedir, "opfor" ) )
		{
			if ( stricmp( version, m_szOpForVersion ) )
			{
				reject = 1;
			}
		}
		else if ( !stricmp( gamedir, "ricochet" ) )
		{
			if ( stricmp( version, m_szRicochetVersion ) )
			{
				reject = 1;
			}
		}
		else if ( !stricmp( gamedir, "dod" ) )
		{
			if ( stricmp( version, m_szDODVersion ) )
			{
				reject = 1;
			}
		}
	}

	// are they different?
	if ( reject )
	{
		RequestRestart(&packet_from);
		return;
	}

#endif

	int challenge_result = CheckChallenge( challenge, &packet_from );

	switch ( challenge_result )
	{
	case 1:
		RejectConnection(&packet_from, "Bad challenge.");
		return;
	case 2:
		RejectConnection(&packet_from, "No challenge for your address.");
		return;
	case 0:
	default:
		break;
	}

	/*  find the server */
	sv = FindServerByAddress( &packet_from );
	if (!sv)
	{
		int h;
		
		sv = (sv_t *)malloc(sizeof(*sv));
		memset (sv, 0, sizeof(*sv));
		
		sv->address = packet_from;

		// Assign the unique id.
		sv->uniqueid = m_nUniqueID++;

		h = HashServer( &packet_from );

		sv->next = servers[ h ];
		servers[ h ] = sv;
		bNewServer = TRUE; // Try and send it the master's public key
	}

	/*  update the current data */
	sv->time = m_curtime;

	SetCriteria( &sv->gamedir, gamedir );
	SetCriteria( &sv->map, map );
	strcpy( sv->os, os );

	sv->islan		= islan;
	sv->players		= num;
	sv->max			= maxplayers;
	sv->bots		= bots;
	sv->password	= password;
	sv->dedicated	= dedicated;
	sv->secure		= secure;
	strcpy( sv->info, info );
	sv->info_length = strlen( sv->info ) + 1;
	sv->isproxy		= isproxy;
	sv->isProxyTarget = isproxytarget;
	if ( isproxytarget )
	{
		SetCriteria( &sv->proxyTarget, proxyaddress );
	}

	Peer_Heartbeat2( sv );
}

/*
===============
Packet_Shutdown
===============
*/
void CHLMasterDlg::Packet_Shutdown (void)
{
	sv_t	*sv, **svp;

	int h;
	
	h = HashServer( &packet_from );

	svp = &servers[h];
	while (1)
	{
		sv = *svp;
		if (!sv)
			break;
		if (CompareAdr(packet_from, sv->address))
		{
			*svp = sv->next;
			Peer_Shutdown( sv );

			free (sv);
			return;
		}
		else
		{
			svp = &sv->next;
		}
	}
}

/*
===============
Packet_GetServers
===============
*/
void CHLMasterDlg::Packet_GetServers (void)
{
	sv_t	*sv;
	int		i;
	int h;
	int nomore;
	
	sprintf (reply, "%c%c%c%c%c\r\n", 255,255,255,255,M2A_SERVERS);
	i = 6;

	nomore = false;
	for ( h = 0; h < SERVER_HASH_SIZE; h++ )
	{
		for (sv=servers[h] ; sv ; sv=sv->next)
		{
			if ( sv->islan || sv->isproxy )
				continue;

			if (i+6 > sizeof(reply))
			{
				nomore = true;
				break;
			}
			
			memcpy (reply+i, &sv->address.sin_addr, 4);
			memcpy (reply+i+4, &sv->address.sin_port, 2);
			i += 6;
		}
		if ( nomore )
			break;
	}

	Sys_SendPacket (&packet_from, (unsigned char *)reply, i);	
}

/*
===============
Packet_GetBatch
===============
*/
void CHLMasterDlg::Packet_GetBatch (void)
{
	int		truenextid;

	msg_readcount = 1;

	// Read batch Start point
	truenextid = (int)MSG_ReadLong();

	Packet_GetBatch_Responder( truenextid, NULL );
}

/*
===============
Packet_RequestsBatch
===============
*/
void CHLMasterDlg::Packet_RequestsBatch (void)
{
	return;
}

int CHLMasterDlg::SizeMod( modsv_t *p )
{
	int c = 0;

	if ( !p )
		return 0;

	c += strlen( p->gamedir ) + 2;

	c += strlen( "ip" ) +  2;
	c += strlen( va( "%i", p->ip_players ) ) +  2;

	c += strlen( "is" ) +  2;
	c += strlen( va( "%i", p->ip_servers ) ) +  2;

	c += strlen( "lp" ) +  2;
	c += strlen( va( "%i", p->lan_players ) ) +  2;

	c += strlen( "ls" ) +  2;
	c += strlen( va( "%i", p->lan_servers ) ) +  2;

	c += strlen( "pp" ) +  2;
	c += strlen( va( "%i", p->proxy_players ) ) +  2;

	c += strlen( "ps" ) +  2;
	c += strlen( va( "%i", p->proxy_servers ) ) +  2;


	return c;
}

void CHLMasterDlg::Packet_Printf( int& curpos, char *fmt, ... )
{
	char	string[2048];
	va_list	argptr;
	int len;
	
	va_start (argptr, fmt);
	len = vsprintf (string, fmt, argptr) + 1;
	va_end (argptr);

	memcpy (reply+curpos, string, len );
	curpos += len;
}

/*
===============
Packet_GetModBatch2
===============
*/
void CHLMasterDlg::Packet_GetModBatch2 (void)
{
	modsv_t	*pMod = NULL;
	int		i = 0;
	char *pszNextMod;
	int h;

	msg_readcount = 3;

	// Read batch Start point
	pszNextMod = MSG_ReadString(); 
	if ( !pszNextMod || !pszNextMod[0] )  // First batch
	{
		// Bad
		UTIL_VPrintf( "Packet_GetModBatch2:  Malformed from %s\r\n", AdrToString (packet_from));
		return;
	}

	if ( !stricmp( pszNextMod, "start-of-list" ) )
	{
		h = 0;
		pMod = mods[h];
	}
	else
	{
		h = HashMod( pszNextMod );
		pMod = FindMod( pszNextMod );
		if ( pMod )
		{
			pMod = pMod->next;
		}
		else
		{
			// Start sending from start of this list. Sigh since the requested mod is now missing ( last server quit during query? )
			pMod = mods[h];
		}
	}

	sprintf (reply, "%c%c%c%c%c\r\n", 255,255,255,255, M2A_ACTIVEMODS2 );
	i += 6;

	for ( ; h < MOD_HASH_SIZE; )
	{
		for ( ; pMod ; pMod=pMod->next)
		{
			if ( i + SizeMod( pMod ) > ( sizeof( reply ) - 128 )  )
				goto finish;

			// Don't report about bogus mods
			if ( strlen( pMod->gamedir ) <= 0 )
				continue;

			Packet_Printf( i, "gamedir" );

			Packet_Printf( i, pMod->gamedir );

			Packet_Printf( i, "is" );

			Packet_Printf( i, va( "%i", pMod->ip_servers ) );

			Packet_Printf( i, "ip" );
		
			Packet_Printf( i, va( "%i", pMod->ip_players ) );

			Packet_Printf( i, "ls" );

			Packet_Printf( i, va( "%i", pMod->lan_servers ) );

			Packet_Printf( i, "lp" );
		
			Packet_Printf( i, va( "%i", pMod->lan_players ) );

			Packet_Printf( i, "ps" );

			Packet_Printf( i, va( "%i", pMod->proxy_servers ) );

			Packet_Printf( i, "pp" );
		
			Packet_Printf( i, va( "%i", pMod->proxy_players ) );

			Packet_Printf( i, "end" );

			Packet_Printf( i, "end" );
		}

		h++;
		pMod = mods[h];
	}

finish:

	// Now write the final one we got to, if any.
	if ( !pMod ) // Still more in list, had to abort early
	{
		const char *pszEnd = "end-of-list";
		memcpy (reply+i, pszEnd, strlen( pszEnd ) + 1 );
		i += strlen( pszEnd ) + 1;
	}
	else
	{
		const char *pszEnd = "more-in-list";
		memcpy (reply+i, pszEnd, strlen( pszEnd ) + 1 );
		i += strlen( pszEnd ) + 1;
	}

	Sys_SendPacket (&packet_from, (unsigned char *)reply, i);	
}


/*
===============
Packet_GetModBatch3
===============

  - This function should ONLY be used by our internal status tool, so we can alter it at whim


*/
void CHLMasterDlg::Packet_GetModBatch3 (void)
{
	modsv_t	*pMod = NULL;
	int		i = 0;
	char *pszNextMod;
	int h;

	msg_readcount = 3;

	// Read batch Start point
	pszNextMod = MSG_ReadString(); 
	if ( !pszNextMod || !pszNextMod[0] )  // First batch
	{
		// Bad
		UTIL_VPrintf( "Packet_GetModBatch3:  Malformed from %s\r\n", AdrToString (packet_from));
		return;
	}

	if ( !stricmp( pszNextMod, "start-of-list" ) )
	{
		h = 0;
		pMod = mods[h];
	}
	else
	{
		h = HashMod( pszNextMod );
		pMod = FindMod( pszNextMod );
		if ( pMod )
		{
			pMod = pMod->next;
		}
		else
		{
			// Start sending from start of this list. Sigh since the requested mod is now missing ( last server quit during query? )
			pMod = mods[h];
		}
	}

	sprintf (reply, "%c%c%c%c%c\r\n", 255,255,255,255, M2A_ACTIVEMODS3 );
	i += 6;

	for ( ; h < MOD_HASH_SIZE; )
	{
		for ( ; pMod ; pMod=pMod->next)
		{
			if ( i + SizeMod( pMod ) > ( sizeof( reply ) - 128 )  )
				goto finish;

			// Don't report about bogus mods
			if ( strlen( pMod->gamedir ) <= 0 )
				continue;

			Packet_Printf( i, "gamedir" );

			Packet_Printf( i, pMod->gamedir );

			Packet_Printf( i, "is" );

			Packet_Printf( i, va( "%i", pMod->ip_servers ) );

			Packet_Printf( i, "ib" );

			Packet_Printf( i, va( "%i", pMod->ip_bots ) );

			Packet_Printf( i, "ibs" );

			Packet_Printf( i, va( "%i", pMod->ip_bots_servers ) );

			Packet_Printf( i, "ip" );
		
			Packet_Printf( i, va( "%i", pMod->ip_players ) );

			Packet_Printf( i, "ls" );

			Packet_Printf( i, va( "%i", pMod->lan_servers ) );

			Packet_Printf( i, "lp" );
		
			Packet_Printf( i, va( "%i", pMod->lan_players ) );

			Packet_Printf( i, "lb" );

			Packet_Printf( i, va( "%i", pMod->lan_bots ) );

			Packet_Printf( i, "lbs" );

			Packet_Printf( i, va( "%i", pMod->lan_bots_servers ) );

			Packet_Printf( i, "ps" );

			Packet_Printf( i, va( "%i", pMod->proxy_servers ) );

			Packet_Printf( i, "pp" );
		
			Packet_Printf( i, va( "%i", pMod->proxy_players ) );

			Packet_Printf( i, "end" );

			Packet_Printf( i, "end" );
		}

		h++;
		pMod = mods[h];
	}

finish:

	// Now write the final one we got to, if any.
	if ( !pMod ) // Still more in list, had to abort early
	{
		const char *pszEnd = "end-of-list";
		memcpy (reply+i, pszEnd, strlen( pszEnd ) + 1 );
		i += strlen( pszEnd ) + 1;
	}
	else
	{
		const char *pszEnd = "more-in-list";
		memcpy (reply+i, pszEnd, strlen( pszEnd ) + 1 );
		i += strlen( pszEnd ) + 1;
	}

	Sys_SendPacket (&packet_from, (unsigned char *)reply, i);	
	

}



/*
===============
Packet_GetModBatch
===============
*/
void CHLMasterDlg::Packet_GetModBatch (void)
{
	modsv_t	*pMod = NULL;
	int		i = 0;
	char *pszNextMod;
	char szNumber[32];
	int h;

	msg_readcount = 3;

	// Read batch Start point
	pszNextMod = MSG_ReadString(); 
	if ( !pszNextMod || !pszNextMod[0] )  // First batch
	{
		// Bad
		UTIL_VPrintf( "Packet_GetModBatch:  Malformed from %s\r\n", AdrToString (packet_from));
		return;
	}

	if ( !stricmp( pszNextMod, "start-of-list" ) )
	{
		h = 0;
		pMod = mods[h];
	}
	else
	{
		h = HashMod( pszNextMod );
		pMod = FindMod( pszNextMod );
		if ( pMod )
		{
			pMod = pMod->next;
		}
		else
		{
			// Start sending from start of this list. Sigh since the requested mod is now missing ( last server quit during query? )
			pMod = mods[h];
		}
	}

	sprintf (reply, "%c%c%c%c%c\r\n", 255,255,255,255, M2A_ACTIVEMODS );
	i += 6;

	for ( ; h < MOD_HASH_SIZE; )
	{
		for ( ; pMod ; pMod=pMod->next)
		{
			if ( i + SizeMod( pMod ) > ( sizeof( reply ) - 32 )  )
				goto finish;

			// Don't report about bogus mods
			if ( strlen( pMod->gamedir ) <= 0 )
				continue;

			memcpy (reply+i, pMod->gamedir, strlen( pMod->gamedir ) + 1 );
			i += strlen( pMod->gamedir ) + 1;
			
			sprintf( szNumber, "%i", pMod->ip_servers );
			memcpy (reply+i, szNumber, strlen( szNumber ) + 1 );
			i += strlen( szNumber ) + 1;
			
			sprintf( szNumber, "%i", pMod->ip_players );
			memcpy (reply+i, szNumber, strlen( szNumber ) + 1 );
			i += strlen( szNumber ) + 1;
		}

		h++;
		pMod = mods[h];
	}

finish:

	// Now write the final one we got to, if any.
	if ( !pMod ) // Still more in list, had to abort early
	{
		const char *pszEnd = "end-of-list";
		memcpy (reply+i, pszEnd, strlen( pszEnd ) + 1 );
		i += strlen( pszEnd ) + 1;
	}
	else
	{
		const char *pszEnd = "more-in-list";
		memcpy (reply+i, pszEnd, strlen( pszEnd ) + 1 );
		i += strlen( pszEnd ) + 1;
	}

	Sys_SendPacket (&packet_from, (unsigned char *)reply, i);	
}

/*
===============
Packet_Ping
===============
*/
void CHLMasterDlg::Packet_Ping (void)
{
	sprintf (reply, "%c%c%c%c%c\r\n", 255,255,255,255, A2A_ACK);
	Sys_SendPacket (&packet_from, (unsigned char *)reply, strlen(reply)+1);	
}


/*
===============
Packet_Motd
===============
*/
void CHLMasterDlg::Packet_Motd (void)
{
	CString strMOTD;
	
	m_hEditMOTD.GetWindowText(strMOTD);
	sprintf ( reply, "%c%c%c%c%c\r\n%s", 255,255,255,255, M2A_MOTD, (char *)(LPCSTR)strMOTD );
	Sys_SendPacket (&packet_from, (unsigned char *)reply, strlen(reply)+1);	
}


/*
===============
PacketFilter
Discard the packet if it is from the banned list
===============
*/
BOOL CHLMasterDlg::PacketFilter (void)
{
	sv_t* banned;

	banned = bannedips;
	while ( banned )
	{
		if ( CompareBaseAdr( banned->address, packet_from ) )
			{
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
void CHLMasterDlg::PacketCommand (void)
{
	static char szChannel[1024];
	
	if ( !PacketFilter() )
		return;

	if ( m_bShowPackets ) 
	{
		UTIL_VPrintf ("%s\r\n", packet_data);
	}

	msg_readcount = 2;		/*  skip command and \r\n */
	
	if (packet_length > 1500)
	{
		UTIL_VPrintf ("Bad packet size: %i\r\n", packet_length);
		return;
	}
	
	// Check for WON Monitoring tool status request.
	if ( packet_length == 8 )
	{
		if ( packet_data[ 0 ] == 3 &&
			 packet_data[ 1 ] == 1 &&
			 packet_data[ 2 ] == 5 )
		{
			Packet_WONMonitor();
			return;
		}
	}

	switch (packet_data[0])
	{
// obseleted by S2M_HEARTBEAT2
//	case S2M_HEARTBEAT:
//		Packet_Heartbeat ();
//		break;
	case S2M_HEARTBEAT2:
		Packet_Heartbeat2 ();
		break;
	case S2M_SHUTDOWN:
		Packet_Shutdown ();
		break;
	case A2M_GET_SERVERS:
		Packet_GetServers ();
		break;	
	case A2M_GET_SERVERS_BATCH:
		Packet_GetBatch ();
		break;	
	case A2M_GET_SERVERS_BATCH2:
		Packet_GetBatch2 ();
		break;	
	case A2M_GETMASTERSERVERS:
		Packet_GetMasterServers ();
		break;	
	case A2A_PING:
		Packet_Ping ();
		break;	
	case A2M_GET_MOTD:
		Packet_Motd ();
		break;	
	case A2A_GETCHALLENGE:
		Packet_GetChallenge();
		break;
	case A2A_NACK:
		break;
	case A2M_GETACTIVEMODS:
		Packet_GetModBatch();
		break;
	case A2M_GETACTIVEMODS2:
		Packet_GetModBatch2();
		break;
	case A2M_GETACTIVEMODS3:
		Packet_GetModBatch3();
		break;
	case M2M_MSG:
		Packet_GetPeerMessage();
		break;
	default:
		/*
		if ( !strnicmp( ( char *)&packet_data[0], "users", strlen( "users" ) ) )
		{
			Packet_RequestsBatch();
			break;
		}
		*/
		UTIL_VPrintf ("Bad PacketCommand from %s\r\n", AdrToString (packet_from));
		break;
	}	
}

/*
===============
OpenNewLogfile
===============
*/
void CHLMasterDlg::OpenNewLogfile (void)
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
void CHLMasterDlg::CheckForNewLogfile (void)
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
void CHLMasterDlg::Sys_Error (char *string, ...)
{
	char szText[1024];
	va_list		argptr;

	if (net_socket)
		closesocket (net_socket);
	if (logfile)
		{
		fclose(logfile);
		logfile = NULL;
		}

	va_start (argptr, string);
	vsprintf(szText, string, argptr);
	va_end (argptr);

	UTIL_Printf ("\r\nSys_Error:\r\n");
	UTIL_Printf( szText );
	UTIL_Printf ("\r\n");

	AfxMessageBox( szText);

	exit (1);
}

void CHLMasterDlg::NET_GetLocalAddress (void)
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

void CHLMasterDlg::UTIL_VPrintf (char *msg, ...)
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

void CHLMasterDlg::UTIL_Printf (char *msg, ...)
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
void CHLMasterDlg::Sys_SendPacket (netadr_t *to, byte *data, int len)
{
	int ret;

	ret = sendto (net_socket, (char *)data, len, 0, (struct sockaddr *)to, sizeof(*to));
	if (ret == -1)
	{
		errno = WSAGetLastError();
		//UTIL_VPrintf ("ERROR: Sys_SendPacket: (%i) %s\r\n", errno, strerror(errno));
	}

	m_fBytesSent += len;
}

char *CHLMasterDlg::MSG_ReadString (void)
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

unsigned char CHLMasterDlg::MSG_ReadByte( void )
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

unsigned short CHLMasterDlg::MSG_ReadShort( void )
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

unsigned int CHLMasterDlg::MSG_ReadLong( void )
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
void CHLMasterDlg::GetPacketCommand (void)
{
	int		fromlen;

	fromlen = sizeof(packet_from);
	packet_length = recvfrom (net_socket, (char *)packet_data, sizeof(packet_data), 0, (struct sockaddr *)&packet_from, &fromlen);
	if (packet_length == -1)
	{
		/*
		int err = WSAGetLastError();
		UTIL_VPrintf ("ERROR: GetPacketCommand: %s\n", strerror(err));
		*/
		return;
	}
	packet_data[packet_length] = 0;		// so it can be printed as a string

	if (packet_length)
	{
		m_fBytesProcessed += (float)packet_length;
		m_nInTransactions++;
		PacketCommand ();
	}
}

void CHLMasterDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	// Resize edit control, too.	
	MoveControls();
}

double CHLMasterDlg::GetStartTime()
{
	return m_tStartTime;
}

void CHLMasterDlg::ResetTimer()
{
	m_tStartTime       = Sys_FloatTime();
	m_fBytesProcessed  = 0.0f;
	m_nInTransactions  = 0;
	m_fBytesSent       = 0.0f;
}

float CHLMasterDlg::GetBytesProcessed()
{
	return m_fBytesProcessed;
}

int CHLMasterDlg::GetInTransactions()
{
	return m_nInTransactions;
}

float CHLMasterDlg::GetOutBytesProcessed()
{
	return m_fBytesSent;
}

void CHLMasterDlg::OnList() 
{
	sv_t	*sv;
	int i = 1;

	i = 1;
	UTIL_Printf("------- Auth Servers  -----------\r\n");
	UTIL_Printf("#     %21s\r\n", "Address");

	for ( sv = authservers ; sv ; sv = sv->next )
	{
		if ( sv->isoldserver )
		{
			UTIL_Printf("OLD:  %5i %21s\r\n",
				i++,
				AdrToString(sv->address));
		}
		else
		{
			UTIL_Printf("%5i %21s\r\n",
				i++,
				AdrToString(sv->address));
		}
	}	

	UTIL_Printf("------- Titan Dir. Servers -----------\r\n");
	UTIL_Printf("#     %21s\r\n", "Address");

	for ( sv = titanservers ; sv ; sv = sv->next )
	{
		if ( sv->isoldserver )
		{
			UTIL_Printf("OLD:  %5i %21s\r\n",
				i++,
				AdrToString(sv->address));
		}
		else
		{
			UTIL_Printf("%5i %21s\r\n",
				i++,
				AdrToString(sv->address));
		}
	}	

	UTIL_Printf("------- Masters Servers -----------\r\n");
	UTIL_Printf("#     %21s\r\n", "Address");

	for ( sv = masterservers ; sv ; sv = sv->next )
	{
		if ( sv->isoldserver )
		{
			UTIL_Printf("OLD:  %5i %21s\r\n",
				i++,
				AdrToString(sv->address));
		}
		else
		{
			UTIL_Printf("%5i %21s\r\n",
				i++,
				AdrToString(sv->address));
		}
	}

	UTIL_Printf("-------------------------------\r\n");

	ListMods();
}

BOOL CHLMasterDlg::MSG_ReadData(int nLength, void *nBuffer)
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

void CHLMasterDlg::OnReloadMotd() 
{
	// TODO: Add your control notification handler code here
	FILE *fp;
	fp = fopen("motd.txt", "rb");
	if (fp)
	{
		int nSize;
		fseek(fp, 0, SEEK_END);
		nSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (nSize >= 2048)
		{
			m_hEditMOTD.SetWindowText("MOTD too big, 2048 bytes max.");
		}
		else
		{
			char motd[2048];
			memset(motd, 0, 2048);
			int nRead;
			nRead = fread( motd, nSize, 1, fp );
			motd[nSize] = '\0';
			m_hEditMOTD.SetWindowText(motd);
		}
		fclose(fp);
	}
}

void CHLMasterDlg::MoveControls()
{
	CEdit *pEdit;

	CRect r;

	GetClientRect( &r );

	CRect rcItem;

	int nListBottom = r.Height() - 120;

	rcItem = CRect( 10 , 220, r.Width() - 10, nListBottom );

	pEdit = (CEdit *)GetDlgItem(IDC_EDIT1);
	if (pEdit && pEdit->GetSafeHwnd())
	{
		pEdit->MoveWindow(&rcItem, TRUE);
	}

	int nCurTop = nListBottom + 10;

	nListBottom = r.Height() - 10;

	rcItem = CRect( 10 , nCurTop, r.Width() - 10, nListBottom );

	pEdit = (CEdit *)GetDlgItem(IDC_MOTD);
	if (pEdit && pEdit->GetSafeHwnd())
	{
		pEdit->MoveWindow(&rcItem, TRUE);
	}
}

/*
==================
unsigned char Nibble( char c )

Returns the 4 bit nibble for a hex character
==================
*/
unsigned char CHLMasterDlg::Nibble( char c )
{
	if ( ( c >= '0' ) &&
		 ( c <= '9' ) )
	{
		 return (unsigned char)(c - '0');
	}

	if ( ( c >= 'A' ) &&
		 ( c <= 'F' ) )
	{
		 return (unsigned char)(c - 'A' + 0x0a);
	}

	if ( ( c >= 'a' ) &&
		 ( c <= 'f' ) )
	{
		 return (unsigned char)(c - 'a' + 0x0a);
	}

	return 0x00;
}

/*
==================
HexConvert( char *pszInput, int nInputLength, unsigned char *pOutput )

Converts pszInput Hex string to nInputLength/2 binary
==================
*/
void CHLMasterDlg::HexConvert( char *pszInput, int nInputLength, unsigned char *pOutput )
{
	unsigned char *p;
	int i;
	char *pIn;

	p = pOutput;
	for (i = 0; i < nInputLength; i+=2 )
	{
		pIn = &pszInput[i];

		*p = (unsigned char)(Nibble( pIn[0] ) << 4 | Nibble( pIn[1] ));		

		p++;
	}
}

#define NUM_FAKE 10000

char *keys[] =
{
	"min",
	"max",
	"oc",
	"avlat",
	"empty",
	"full",
	"avconn",
	"plmax",
};

void CHLMasterDlg::GenerateFakeServers( void )
{
	sv_t	*sv;
	netadr_t adr;
	int i, j;
	char szModName[ 32 ];
	int c;
	char value[ 32 ];

	char info[ 1024 ];
	char temp[1024];

	memset( &adr, 0, sizeof( netadr_t ) );

	adr.sin_family = AF_INET;
	adr.sin_port   = ntohs( 27015 );
	
	for ( i = 0; i < NUM_FAKE; i++ )
	{
		sv = (sv_t *)malloc( sizeof(*sv) );
		
		memset (sv, 0, sizeof(*sv));

		// Create a fake internet address
		adr.sin_addr.S_un.S_un_b.s_b1 = (unsigned char)( rand() & 255 );
		adr.sin_addr.S_un.S_un_b.s_b2 = (unsigned char)( rand() & 255 );
		adr.sin_addr.S_un.S_un_b.s_b3 = (unsigned char)( rand() & 255 );
		adr.sin_addr.S_un.S_un_b.s_b4 = (unsigned char)( rand() & 255 );
		
		if ( i < 20 )
		{
	// Create a fake internet address
		adr.sin_addr.S_un.S_un_b.s_b1 = 200;
		adr.sin_addr.S_un.S_un_b.s_b2 = 200;
		adr.sin_addr.S_un.S_un_b.s_b3 = 2;
		adr.sin_addr.S_un.S_un_b.s_b4 = 2;

		}

		sv->address = adr;
		// Assign the unique id.
		sv->uniqueid = m_nUniqueID++;

		sv->islan = rand() % 4 ? 0 : 1;

		sv->isproxy = ( ( rand() % 100 ) > 95 ) ? 1 : 0;

		sprintf( szModName, "%s%03i", (int)( rand() %2 ) == 0 ? "mod" : "MOD", rand() % 10 );

		//if ( !(rand() % 3) )
			//sprintf( szModName, "tfc" );

		SetCriteria( &sv->gamedir, szModName );

		sv->max = rand() % 30 + 2;
		sv->players = rand() % sv->max;
		if ( sv->isproxy )
		{
			sv->bots = 0;
		}
		else
		{
			if (  sv->players )
			{
				sv->bots = rand() % sv->players;
			}
			else
			{
				sv->bots = 0;
			}
		}

		strcpy( sv->os, (int)( rand() %2 ) == 0 ? "l" : "w" );
		sprintf( temp, "map%03i", rand() % 10 );
		SetCriteria( &sv->map, temp );
		sv->dedicated = (int)( rand() %2 ) == 0 ? 1 : 0;

		c = rand() & 7;
		for ( j = 0; j < c; j++ )
		{
			sprintf( value, "%i", rand() % 100 );
			sprintf( info, "\\%s\\%s", keys[ j ], value );
			strcat( sv->info, info );
		}
		if ( strlen( sv->info ) > 0 )
		{
			strcat( sv->info, "\\" );
		}
		else
		{
			strcpy( sv->info, "" );
		}
		sv->info_length = strlen( sv->info ) + 1;

		sv->time = (int)( m_curtime - (double)( rand() % 1000 ) );

		int h;

		h = HashServer( &sv->address );

		sv->next = servers[ h ];
		servers[ h ] = sv;
	}
}

sv_t *CHLMasterDlg::AddServer( sv_t **pList, netadr_t *adr )
{
	sv_t *sv;

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
		sv = (sv_t *)malloc(sizeof(*sv));
		memset (sv, 0, sizeof(*sv));
		
		sv->next = *pList;
		sv->address = *adr;

		//  update the current data 
		sv->time = (int)Sys_FloatTime();
		*pList = sv;
	}	

	return sv;
}

void CHLMasterDlg::ParseServers()
{
	// Read from a file.
	char *pszBuffer = NULL;
	FILE *fp;
	int nSize;

#if 0
	if ( CheckParm( "-fake", NULL ) )
	{
		netadr_t localadr;
		memset( &localadr, 0, sizeof( localadr ) );
		localadr = StringToAdr( "63.75.210.16:27010" );
		AddServer( &masterservers, &localadr );
		return;
	}
#endif

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

	sv_t **pList;

	while ( 1 )
	{
		token.ParseNextToken();

		if (strlen(token.token) <= 0)
			break;

		if (!stricmp( token.token, "Auth" ) )
		{
			pList = &authservers;
		}
		else if (!stricmp( token.token, "Master" ) )
		{
			pList = &masterservers;
		}
		else if (!stricmp( token.token, "Titan" ) )
		{
			pList = &titanservers;
		}
		else if (!stricmp( token.token, "Banned" ) )
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
			sv_t *pServer;

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
			if ( pServer )
			{
				pServer->isoldserver = bIsOld ? 1 : 0;
			}
		}
	}

	delete[] pszBuffer;

}

void CHLMasterDlg::UpdateCount()
{
	// Now count servers and display current count:
	char szServerCount[128];
	sv_t	*sv;
	m_nServerCount = 0;
	int nCountMaster = 0;

	m_nUsers = 0;

	m_nLanServerCount = 0;
	m_nLanUsers = 0;
	m_nBotCount = 0;
	m_nLanBotCount = 0;
	
	int m_nProxyServerCount = 0;
	int m_nProxyUserCount = 0;

	int maxslots = 0;
	int empty = 0;
	float usersperserver = 0.0;
	
	int h;
	for ( h = 0; h < SERVER_HASH_SIZE; h++ )
	{
		for (sv=servers[ h ] ; sv ; sv=sv->next)
		{
			if ( sv->islan )
			{
				m_nLanServerCount++;
				m_nLanUsers += (sv->players - sv->bots);
			}
			else if ( sv->isproxy )
			{
				m_nProxyServerCount++;
				m_nProxyUserCount += (sv->players - sv->bots);
			}
			else
			{
				m_nServerCount++;
				m_nUsers += (sv->players - sv->bots);
			}

			if ( sv->players != 0 )
			{
				maxslots += sv->max;
			}
			else
			{
				empty++;
			}

			if ( sv->bots )
			{
				if ( sv->islan )
				{
					m_nLanBotCount+= sv->bots;
				}
				else
				{
					m_nBotCount+= sv->bots;
				}
			}
		}
	}
	for (sv=masterservers ; sv ; sv=sv->next)
	{
		nCountMaster++;
	}
	
	if ( maxslots > 0 )
	{
		usersperserver = (float)m_nUsers / (float)maxslots;
	}

	sprintf(szServerCount, "%7i sv %7i pl %7i bt", 
		m_nServerCount,
		m_nUsers,
		m_nBotCount);

	if (m_statActiveInternet.GetSafeHwnd())
		m_statActiveInternet.SetWindowText(szServerCount);

	sprintf(szServerCount, "%7i sv %7i pl %7i bt", 
		m_nLanServerCount, 
		m_nLanUsers,
		m_nLanBotCount);

	if (m_statActiveLan.GetSafeHwnd())
		m_statActiveLan.SetWindowText(szServerCount);

	sprintf(szServerCount, "%7i sv %7i pl", 
		m_nProxyServerCount,
		m_nProxyUserCount );

	if (m_statActiveProxy.GetSafeHwnd())
		m_statActiveProxy.SetWindowText(szServerCount);

	sprintf(szServerCount, "%7i sv %7i pl %7i bt %.2f (users/occupied server)", 
		m_nServerCount + m_nLanServerCount + m_nProxyServerCount, 
		m_nUsers + m_nLanUsers + m_nProxyUserCount, 
		m_nBotCount + m_nLanBotCount,
		usersperserver );

	if (m_statActiveTotal.GetSafeHwnd())
		m_statActiveTotal.SetWindowText(szServerCount);

	// Recompute mod totals
	AdjustCounts();

	CheckForNewLogfile ();
}

/*
===============
Packet_GetServers
===============
*/
void CHLMasterDlg::Packet_GetMasterServers()
{
	sv_t	*sv;
	int		i;
	BOOL bOldOnly = FALSE;
	char szVersion[ 1024 ];
	BOOL bSteam = FALSE;

	msg_readcount--;

	char *pVersion = NULL;
	char *pProduct = NULL;
	pVersion = MSG_ReadString();
	pProduct = MSG_ReadString();

	// did we get a product in the message?
	if ( !pProduct || !strlen(pProduct) )
		pProduct = NULL;

	strcpy( szVersion, "1.0.0.9" );

	// Do we have a valid version received?
	if ( pVersion )
	{
		// Steam appends its own version to the client version...we need to remove it.
		if ( strstr( pVersion, "/" ) )
		{
			pVersion[ strcspn( pVersion, "/" ) ] = '\0';
			bSteam = TRUE;
		}

		if ( strlen( pVersion ) != 0 )
		{
			BOOL bAllNumeric = TRUE;
			int nDots = 0;
			char *pDot;

			pDot = pVersion;
			while ( pDot && *pDot && bAllNumeric )
			{
				if ( *pDot == '.' )
				{
					nDots++;
				}
				else if ( !isdigit( *pDot )  )
				{
					bAllNumeric = FALSE;
				}

				pDot++;
			}

			// Must find three periods in version string
			// . . .
			if ( ( nDots == 3 ) && bAllNumeric )
			{
				strcpy( szVersion, pVersion );
			}
		}
	}

	// See what type of request it is?
	sprintf (reply, "%c%c%c%c%c", 255,255,255,255,M2A_MASTERSERVERS);
	i = 6;

	// Response format:
	// Auth Servers
	// Titan Servers
	// Master Servers

	strcat ( reply, "\r\n" );

	// which product are we dealing with?
	char * version = NULL;
	if ( pProduct && strstr( pProduct, "CSTRIKE" ) )
		version = m_szCSVersion;
	else if ( pProduct && strstr( pProduct, "DOD" ) )
		version = m_szDODVersion;
	else
		version = m_szHLVersion;

	// we don't want to check the version of the Steam clients
	if ( !bSteam && stricmp( szVersion, version ) )
	{
		// Post 1009 can handle the rejection
		// Otherwise, pass in the "old" servers
		if ( !stricmp( szVersion, "1.0.0.9" ) )
		{
			bOldOnly = TRUE;
		}
		// is the version being reported less than the current version and
		// is it not equal to 1.0.0.0, which is the version for BlueShift
		else if ( IsLessThan( szVersion, version ) && !IsEqualTo( szVersion, "1.0.0.0" ) )
		{
			// OBSOLETE CLIENT
			// Divider
			strcpy( ( char *)&reply[i], "outofdate" );

			i += strlen( "outofdate") + 1;

			Sys_SendPacket (&packet_from, (unsigned char *)reply, i);	
			return;
		}
	}

	for (sv = authservers ; sv ; sv=sv->next)
	{
		if ( bOldOnly && !sv->isoldserver )
			continue;

		if ( !bOldOnly && sv->isoldserver )
			continue;

		if (i+6 > sizeof(reply))
			break;
		
		memcpy (reply+i, &sv->address.sin_addr, 4);
		memcpy (reply+i+4, &sv->address.sin_port, 2);
		i += 6;
	}

	// Divider
	*(int *)&reply[i] = 0xffffffff;
	i += sizeof( int );

	for (sv = titanservers ; sv ; sv=sv->next)
	{
		if ( bOldOnly && !sv->isoldserver )
			continue;

		if ( !bOldOnly && sv->isoldserver )
			continue;

		if (i+6 > sizeof(reply))
			break;
		
		memcpy (reply+i, &sv->address.sin_addr, 4);
		memcpy (reply+i+4, &sv->address.sin_port, 2);
		i += 6;
	}

	// Divider
	*(int *)&reply[i] = 0xffffffff;
	i += sizeof( int );

	for (sv = masterservers ; sv ; sv=sv->next)
	{
		if ( bOldOnly && !sv->isoldserver )
			continue;

		if ( !bOldOnly && sv->isoldserver )
			continue;

		if (i+6 > sizeof(reply))
			break;
		
		memcpy (reply+i, &sv->address.sin_addr, 4);
		memcpy (reply+i+4, &sv->address.sin_port, 2);
		i += 6;
	}

	// Divider
	*(int *)&reply[i] = 0xffffffff;
	i += sizeof( int );

	Sys_SendPacket (&packet_from, (unsigned char *)reply, i);	
}

void CHLMasterDlg::FreeServers()
{
	sv_t	*sv, *next;

	sv = authservers;
	while (sv)
	{
		next = sv->next;
		free (sv);
		sv = next;
	}

	sv = titanservers;
	while (sv)
	{
		next = sv->next;
		free (sv);
		sv = next;
	}

	sv = masterservers;
	while (sv)
	{
		next = sv->next;
		free (sv);
		sv = next;
	}

	sv = bannedips;
	while (sv)
	{
		next = sv->next;
		free (sv);
		sv = next;
	}
}

void CHLMasterDlg::ParseVersion( void )
{
	// Read from a file.
	char *pszBuffer = NULL;
	FILE *fp;
	int nSize;

	fp = fopen("version.lst", "rb");
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
	
	CToken token(pszBuffer);
	token.SetCommentMode(TRUE);  // Skip comments when parsing.
	token.SetQuoteMode( TRUE );
	token.ParseNextToken();

	// Get the version of Half-Life
	if (strlen(token.token) <= 0)
	{
		strcpy( m_szHLVersion, "1.0.1.0" );
	}
	else
	{
		strcpy( m_szHLVersion, token.token );
	}

	token.ParseNextToken();

	// Get the version of Counter-Strike
	if (strlen(token.token) <= 0)
	{
		strcpy( m_szCSVersion, "1.0.0.0" );
	}
	else
	{
		strcpy( m_szCSVersion, token.token );
	}

#if defined( _STEAM_HLMASTER )

	token.ParseNextToken();

	// Get the version of TFC
	if (strlen(token.token) <= 0)
	{
		strcpy( m_szTFCVersion, "1.0.0.0" );
	}
	else
	{
		strcpy( m_szTFCVersion, token.token );
	}

	token.ParseNextToken();

	// Get the version of DMC
	if (strlen(token.token) <= 0)
	{
		strcpy( m_szDMCVersion, "1.0.0.0" );
	}
	else
	{
		strcpy( m_szDMCVersion, token.token );
	}

	token.ParseNextToken();

	// Get the version of OpFor
	if (strlen(token.token) <= 0)
	{
		strcpy( m_szOpForVersion, "1.0.0.0" );
	}
	else
	{
		strcpy( m_szOpForVersion, token.token );
	}

	token.ParseNextToken();

	// Get the version of Ricochet
	if (strlen(token.token) <= 0)
	{
		strcpy( m_szRicochetVersion, "1.0.0.0" );
	}
	else
	{
		strcpy( m_szRicochetVersion, token.token );
	}

#endif

	token.ParseNextToken();

	// Get the version of DOD
	if (strlen(token.token) <= 0)
	{
		strcpy( m_szDODVersion, "1.0.0.0" );
	}
	else
	{
		strcpy( m_szDODVersion, token.token );
	}

	token.ParseNextToken();

	// Get the protocol version
	if ( strlen(token.token) > 0 )
	{
		CURRENT_PROTOCOL = atoi( token.token );
		if ( CURRENT_PROTOCOL <= 0 )
		{
			CURRENT_PROTOCOL = PROTOCOL_VERSION;
		}
	}

	delete[] pszBuffer;
}

BOOL CHLMasterDlg::IsLessThan( const char *pszUser, const char *pszServer )
{
	// Turn 1.0.0.9 into 1009
	if ( VerToInt( pszUser ) < VerToInt( pszServer ) )
		return TRUE;

	return FALSE;
}

BOOL CHLMasterDlg::IsEqualTo( const char *pszStringA, const char *pszStringB )
{
	if ( VerToInt( pszStringA ) == VerToInt( pszStringB ) )
		return TRUE;

	return FALSE;
}

int CHLMasterDlg::VerToInt( const char *pszVersion )
{
	char szOut[ 32 ];
	const char *pIn;
	char *pOut;

	pIn = pszVersion;
	pOut = szOut;

	while ( *pIn )
	{
		if ( *pIn != '.' )
		{
			*pOut++ = *pIn;
		}
		pIn++;
	}
	*pOut = '\0';

	return atoi( szOut );
}

void CHLMasterDlg::Packet_WONMonitor( void )
{
	unsigned int		blob;
	int		i;
	
	msg_readcount = 3;

	blob = MSG_ReadData( sizeof( unsigned int ), &blob );

	UTIL_VPrintf ("Packet_WONMonitor from %s\r\n", AdrToString (packet_from));

	i = 0;

	reply[ i++ ] = 3;
	reply[ i++ ] = 1;
	reply[ i++ ] = 6;
	*( unsigned int * )&reply[ i ] = blob;
	i += sizeof( unsigned int );
	
	Sys_SendPacket ( &packet_from, (unsigned char *)reply, i );
}

void CHLMasterDlg::FreeGameServers( void )
{
	int h;
	sv_t *p, *n;

	for ( h = 0; h < SERVER_HASH_SIZE; h++ )
	{
		p = servers[h];
		while ( p )
		{
			n = p->next;
			free( p );
			p = n;
		}
		servers[h] = NULL;
	}
}

sv_t *CHLMasterDlg::FindServerByAddress( netadr_t *address )
{
	sv_t *p;
	int h;

	if ( !address )
		return NULL;

	h = HashServer( address );
	p = servers[h];
	while ( p )
	{
		if ( CompareAdr( *address, p->address ) )
		{
			return p;
		}

		p = p->next;
	}

	return NULL;
}

int CHLMasterDlg::HashServer( netadr_t *address )
{
	// compute a hash value
	unsigned int c = 0;
	char *p;
	int i;

	p = (char *)&address->sin_addr;
	
	for ( i = 0; i < 4; i++ )
	{
		c += ( *p << 1 );
		c ^= *p;
		p++;
	}

	p = (char *)&address->sin_port;

	for ( i = 0; i < 2; i++ )
	{
		c += ( *p << 1 );
		c ^= *p;
		p++;
	}

	c = c % SERVER_HASH_SIZE;
	return c;
}

//-----------------------------------------------------------------------------
// Purpose: Allows user to restrict list of server IP addresses returned.
//-----------------------------------------------------------------------------
typedef struct search_criteria_s
{
	int		usemap;
	string_criteria_t	map;
	int		usededicated;
	int		usesecure;
	int		uselinux;
	int		useempty;
	int		usefull;
	int		usegame;
	string_criteria_t	gamedir;
	
	int		wantproxy;
	int		useProxyTarget;
	string_criteria_t proxyTarget;
} search_criteria_t;

//-----------------------------------------------------------------------------
// Purpose: Just compares checksums & lengths
// Input  : *c1 - 
//			*c2 - 
// Output : inline bool
//-----------------------------------------------------------------------------
inline bool CriteriaAreEqual( string_criteria_t *c1, string_criteria_t *c2 )
{
	if ( c1->checksum != c2->checksum )
		return false;

	if ( c1->length != c2->length )
		return false;

	// Don't actually compare strings...change of collision on crc is very very low and who cares...
	//if ( !strcmp( c1->value, c2->value ) )
	//{
		//return true;
	//}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Request IP address list, including search criteria
//-----------------------------------------------------------------------------
void CHLMasterDlg::Packet_GetBatch2 (void)
{
	int		truenextid;
	char	info[ MAX_SINFO ];

	msg_readcount = 1;

	// Read batch Start point
	truenextid = (int)MSG_ReadLong();

	strncpy( info, MSG_ReadString(), MAX_SINFO - 1 );
	info[ MAX_SINFO - 1 ] = 0;

	// Make sure user requested some criteria
	if ( info[0] )
	{
		search_criteria_t criteria;

		// Read in and create criteria
		memset( &criteria, 0, sizeof( criteria ) );

		const char *map;
		map = Info_ValueForKey( info, "map" );
		if ( map && map[0] )
		{
			criteria.usemap = 1;
			SetCriteria( &criteria.map, map );
		}

		const char *gamedir;
		gamedir = Info_ValueForKey( info, "gamedir" );
		if ( gamedir && gamedir[ 0 ] )
		{
			criteria.usegame = 1;
			SetCriteria( &criteria.gamedir, gamedir );
		}

		const char *param;
		param = Info_ValueForKey( info, "dedicated" );
		if ( param && param[0] )
		{
			criteria.usededicated = 1;
		}

		param = Info_ValueForKey( info, "secure" );
		if ( param && param[0] )
		{
			criteria.usesecure = 1;
		}

		param = Info_ValueForKey( info, "full" );
		if ( param && param[0] )
		{
			criteria.usefull = 1;
		}
		param = Info_ValueForKey( info, "empty" );
		if ( param && param[0] )
		{
			criteria.useempty = 1;
		}
		param = Info_ValueForKey( info, "linux" );
		if ( param && param[0] )
		{
			criteria.uselinux = 1;
		}
		param = Info_ValueForKey( info, "proxy" );
		if ( param && param[0] )
		{
			criteria.wantproxy = 1;
		}

		param = Info_ValueForKey( info, "proxytarget" );
		if ( param && param[0] )
		{
			criteria.useProxyTarget = 1;
			SetCriteria( &criteria.proxyTarget, param );
		}
		
		Packet_GetBatch_Responder( truenextid, &criteria );
	}
	else
	{
		Packet_GetBatch_Responder( truenextid, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *server - server to evaluate
//			*criteria - list of search fields
// Output : int - 1 if server passes, 0 otherwise
//-----------------------------------------------------------------------------
int CHLMasterDlg::ServerPassesCriteria( sv_t *server, search_criteria_t *criteria )
{
	if ( !criteria )
		return 1;

	if ( criteria->usegame )
	{
		if ( !CriteriaAreEqual( &criteria->gamedir, &server->gamedir ) )
			return 0;
	}

	if ( criteria->usemap )
	{
		if ( !CriteriaAreEqual( &criteria->map, &server->map ) )
			return 0;
	}

	if ( criteria->usededicated )
	{
		if ( !server->dedicated )
			return 0;
	}

	if ( criteria->usesecure )
	{
		if ( !server->secure )
			return 0;
	}

	if ( criteria->uselinux )
	{
		if ( server->os[0] != 'l' )
			return 0;
	}

	if ( criteria->useempty )
	{
		if ( server->players == 0 )
			return 0;
	}

	if ( criteria->usefull )
	{
		if ( server->players == server->max )
			return 0;
	}

	// different logic for proxies. 
	if ( server->isproxy != criteria->wantproxy)
	{
			return 0;
	}

	if ( criteria->useProxyTarget )
	{
		if ( !CriteriaAreEqual( &server->proxyTarget, &criteria->proxyTarget ) )
		{
			return 0;
		}
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Constructs response to old or new style search string
// Input  : truenextid - 
//			*criteria - 
//-----------------------------------------------------------------------------
void CHLMasterDlg::Packet_GetBatch_Responder ( int truenextid, search_criteria_t *criteria )
{
	sv_t	*sv = NULL;
	int		i = 0;
	int     nextid;
	int		savepos;
	int		send_info = 0;

	send_info = truenextid & ( 1<<31 );
	nextid = truenextid & ~(1<<31);

	sprintf (reply, "%c%c%c%c%c\r\n", 255,255,255,255, M2A_SERVER_BATCH );
	i += 6;

	savepos = i;
	i += sizeof( int );
	
	int h = 0;

	// If nextid was not zero, find the server after the one requested:
	if ( nextid != 0 )
	{
		h = nextid % SERVER_HASH_SIZE;
	}
	
	int nomore = 0;

	if ( send_info )
	{
		for ( ; h < SERVER_HASH_SIZE; h++ )
		{
			sv = servers[ h ];
			for ( ; sv ; sv=sv->next)
			{
				if ( sv->islan )
				{
					continue;
				}

				if ( !ServerPassesCriteria( sv, criteria ) )
				{
					continue;
				}

				if ( ( i + 6 + sv->info_length ) >= sizeof( reply ) )
				{
					nomore = 1;
					break;
				}
				
				memcpy ( &reply[i], &sv->address.sin_addr, 4);
				i += 4;
				memcpy ( &reply[i], &sv->address.sin_port, 2);
				i += 2;
				
				{
					int len;
					len = sprintf( &reply[ i ], "%s", sv->info ) + 1;
					i += len;
				}
			}

			if ( nomore )
			{
				break;
			}
		}
	}
	else
	{
		for ( ; h < SERVER_HASH_SIZE; h++ )
		{
			sv = servers[ h ];
			for ( ; sv ; sv=sv->next)
			{
				if ( sv->islan )
				{
					continue;
				}

				if ( !ServerPassesCriteria( sv, criteria ) )
				{
					continue;
				}

				if ( i+6 >= sizeof( reply ) )
				{
					nomore = 1;
					break;
				}
				
				memcpy ( &reply[i], &sv->address.sin_addr, 4);
				i += 4;
				memcpy ( &reply[i], &sv->address.sin_port, 2);
				i += 2;
			}

			if ( nomore )
			{
				break;
			}
		}
	}

	// Now write the final one we got to, if any.
	if ( sv ) // Still more in list, had to abort early
	{
		// Don't get stuck in a loop
		if ( h == nextid )
			h++;

		truenextid = h;
		if ( send_info )
		{
			truenextid |= (1<<31);
		}
	}
	else // Last packet, no more servers...
	{
		truenextid = 0;
		if ( send_info )
		{
			truenextid |= (1<<31);
		}
	}

	*(int *)&reply[ savepos ] = truenextid;

	Sys_SendPacket (&packet_from, (unsigned char *)reply, i);	
}

void CAboutDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CHLMasterDlg::OnComputestats() 
{
	// TODO: Add your control notification handler code here
	UpdateCount();	
}
