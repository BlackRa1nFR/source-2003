// net_ws.c
// Windows IP Support layer.

#include "winquake.h"
#include "sys.h"
#include "tier0/vcrmode.h"
#include "server.h"
#include "vstdlib/random.h"
#include "host.h"
#include "convar.h"
#include "vstdlib/ICommandLine.h"

#if defined( _WIN32 )

#include <winsock.h>

typedef int socklen_t;

#else

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "vstdlib/icommandline.h"

#define WSAEWOULDBLOCK		EWOULDBLOCK
#define WSAEMSGSIZE			EMSGSIZE
#define WSAEADDRNOTAVAIL	EADDRNOTAVAIL
#define WSAEAFNOSUPPORT		EAFNOSUPPORT
#define WSAECONNRESET		ECONNRESET
#define WSAECONNREFUSED     ECONNREFUSED
#define WSAEADDRINUSE EADDRINUSE

#define ioctlsocket ioctl
#define closesocket close

#undef SOCKET
typedef int SOCKET;
#define FAR
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Clear this out to not use threads
int	use_thread = 0;
static qboolean net_thread_initialized = false;

#define	PORT_ANY	-1

extern ConVar net_showpackets;
extern ConVar cl_showmessages;

static ConVar net_address	( "net_address", "" );
static ConVar ipname        ( "ip"   , "localhost" );
static ConVar iphostport    ( "ip_hostport", "0" );
static ConVar hostport      ( "hostport", "0" );
static ConVar defport	    ( "port", PORT_SERVER );
static ConVar ip_clientport ( "ip_clientport", "0" );
static ConVar clientport    ( "clientport", PORT_CLIENT );

ConVar	fakelag				( "fakelag", "0" );  // Lag all incoming network data (including loopback) by xxx ms.
ConVar	fakeloss			( "fakeloss", "0" ); // Act like we dropped the packet this % of the time.

qboolean noip		= false;    // Disable IP Support

netadr_t	net_local_adr;
netadr_t	net_from;

static byte net_message_buffer[ NET_MAX_MESSAGE ];
sizebuf_t	net_message;
static byte	in_message_buf[ NET_MAX_MESSAGE ];
static sizebuf_t   in_message;
static netadr_t    in_from;

int			ip_sockets[2] = { 0, 0 };
static		int net_sleepforever = 1;

#define	MAX_LOOPBACK 4

typedef struct
{
	byte	data[ NET_MAX_MESSAGE ];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[ MAX_LOOPBACK ];
	int			get;
	int			send;
} loopback_t;

static loopback_t	loopbacks[2];

typedef struct packetlag_s
{
	unsigned char *pPacketData; // Raw stream data is stored.
	int   nSize;
	netadr_t net_from;
	float receivedTime;
	struct packetlag_s *pNext;
	struct packetlag_s *pPrev;
} packetlag_t;

static packetlag_t g_pLagData[2];  // List of lag structures, if fakelag is set.

// Split long packets.  Anything over 1460 is failing on some routers
typedef struct
{
	int		currentSequence;
	int		splitCount;
	int		totalSize;
	char	buffer[ NET_MAX_MESSAGE ];	// This has to be big enough to hold the largest message
} LONGPACKET;

// Use this to pick apart the network stream, must be packed
#pragma pack(1)
typedef struct
{
	int		netID;
	int		sequenceNumber;
	short	packetID;
} SPLITPACKET;
#pragma pack()

#define MAX_ROUTEABLE_PACKET		1400
#define SPLIT_SIZE		(MAX_ROUTEABLE_PACKET - sizeof(SPLITPACKET))

#if defined( _WIN32 )
CRITICAL_SECTION net_cs;
#endif

void NET_ThreadLock( void )
{
#if defined( _WIN32 )
	if ( use_thread && net_thread_initialized )
	{
		EnterCriticalSection( &net_cs );
	}
#endif
}

void NET_ThreadUnlock( void )
{
#if defined( _WIN32 )
	if ( use_thread && net_thread_initialized )
	{
		LeaveCriticalSection( &net_cs );
	}
#endif
}

//=============================================================================

void NetadrToSockadr (netadr_t *a, struct sockaddr *s)
{
	Q_memset (s, 0, sizeof(*s));

	if (a->type == NA_BROADCAST)
	{
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if (a->type == NA_IP)
	{
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port = a->port;
	}
}

void SockadrToNetadr (struct sockaddr *s, netadr_t *a)
{
	if (s->sa_family == AF_INET)
	{
		a->type = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
}

unsigned short NET_HostToNetShort( unsigned short us_in )
{
	return htons( us_in );
}

unsigned short NET_NetToHostShort( unsigned short us_in )
{
	return ntohs( us_in );
}


qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return TRUE;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
			return true;
		return false;
	}
	return false;
}

qboolean	NET_CompareClassBAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return true;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] )
			return true;
		return false;
	}
	return false;
}

// reserved addresses are not routeable, so they can all be used in a LAN game
qboolean	NET_IsReservedAdr (netadr_t a)
{
	if (a.type == NA_LOOPBACK)
		return true;

	if (a.type == NA_IP)
	{
		if ( (a.ip[0] == 10) ||										// 10.x.x.x is reserved
			 (a.ip[0] == 127) ||									// 127.x.x.x 
			 (a.ip[0] == 172 && a.ip[1] >= 16 && a.ip[1] <= 31) ||	// 172.16.x.x  - 172.31.x.x 
			 (a.ip[0] == 192 && a.ip[1] >= 168) ) 					// 192.168.x.x
			return true;
		return false;
	}
	return false;
}
/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return TRUE;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
			return true;
		return false;
	}
	return false;
}

const char *NET_AdrToString (netadr_t a)
{
	static	char	s[64];

	Q_memset(s, 0, 64);

	if (a.type == NA_LOOPBACK)
		Q_snprintf (s, sizeof( s ), "loopback");
	else if (a.type == NA_IP)
		Q_snprintf (s, sizeof( s ), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

	return s;
}

const char	*NET_BaseAdrToString (netadr_t a)
{
	static	char	s[64];

	Q_memset(s, 0, 64);

	if (a.type == NA_LOOPBACK)
		Q_snprintf(s, sizeof( s ), "loopback");
	else if (a.type == NA_IP)
		Q_snprintf(s, sizeof( s ), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);
	return s;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *s - 
//			*sadr - 
// Output : qboolean	NET_StringToSockaddr
//-----------------------------------------------------------------------------
qboolean	NET_StringToSockaddr ( const char *s, struct sockaddr *sadr )
{
	struct hostent	*h;
	char	*colon;
	char	copy[128];
	
	Q_memset (sadr, 0, sizeof(*sadr));
	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	((struct sockaddr_in *)sadr)->sin_port = 0;

	Q_strcpy (copy, s);
	// strip off a trailing :port if present
	for (colon = copy ; *colon ; colon++)
		if (*colon == ':')
		{
			*colon = 0;
			((struct sockaddr_in *)sadr)->sin_port = htons((short)atoi(colon+1));	
		}
	
	if (copy[0] >= '0' && copy[0] <= '9' && Q_strstr( copy, "." ) )
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
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToAdr ( const char *s, netadr_t *a)
{
	struct sockaddr sadr;
	
	if (!Q_strcmp (s, "localhost"))
	{
		Q_memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	if (!NET_StringToSockaddr (s, &sadr))
		return false;
	
	SockadrToNetadr (&sadr, a);

	return true;
}

qboolean	NET_IsLocalAddress (netadr_t adr)
{
	return adr.type == NA_LOOPBACK;
}

#if defined( _WIN32 )
/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString (int code)
{
	switch (code)
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}
#else
/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString (int code)
{
	return strerror(code);
}
#endif

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/
void NET_TransferRawData( sizebuf_t *msg, unsigned char *pStart, int nSize )
{
	Q_memcpy ( msg->data, pStart, nSize);
	msg->cursize = nSize;
}

qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *in_from, sizebuf_t *msg )
{
	int		i;
	loopback_t	*loop;

	Q_memset (in_from, 0, sizeof(*in_from));
	in_from->type = NA_LOOPBACK;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
	{
		return false;
	}

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	NET_TransferRawData( msg, &loop->msgs[i].data[0], loop->msgs[i].datalen );
	return true;
}

void NET_SendLoopPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	int		i;
	loopback_t	*loop;

	NET_ThreadLock();

	loop = &loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	Q_memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;

	NET_ThreadUnlock();

#if 0//defined( _DEBUG )
	if ( length > MAX_ROUTEABLE_PACKET )
	{
		Con_DPrintf( "NET_SendLoopPacket:  Packet length (%i) > (%i) bytes\n", length, MAX_ROUTEABLE_PACKET );
	}
#endif
}

//=============================================================================

/*
==================
NET_RemoveFromPacketList(packetlag_t *pPacket)

Unlinks it from the current list.
==================
*/
void NET_RemoveFromPacketList(packetlag_t *pPacket)
{
	pPacket->pPrev->pNext = pPacket->pNext;
	pPacket->pNext->pPrev = pPacket->pPrev;

	pPacket->pNext = pPacket->pPrev = NULL;
}

int NET_CountLaggedList( packetlag_t *pList )
{
	int c = 0;
	packetlag_t *p;
	
	for (p = pList->pNext; p && p != pList; p = p->pNext )
	{
		c++;
	}

	return c;
}

/*
==================
NET_ClearLaggedList(packetlag_t *pList)

==================
*/
void NET_ClearLaggedList(packetlag_t *pList)
{
	packetlag_t *p, *n;
	
	for (p = pList->pNext; p && p != pList;)
	{
		n = p->pNext;

		NET_RemoveFromPacketList( p );

		// Delete the associated file.
		if ( p->pPacketData )
		{
			delete[] p->pPacketData;
			p->pPacketData = NULL;
		}
		delete p;
		p = n;
	}

	pList->pNext = pList->pPrev = pList;
}

/*
===================
NET_AddToLagged

===================
*/
void NET_AddToLagged(netsrc_t sock, packetlag_t *pList, packetlag_t *pPacket, netadr_t *net_from, sizebuf_t messagedata, float timestamp )
{
	unsigned char *pStart;

	if (pPacket->pPrev || pPacket->pNext)
	{
		Con_Printf("Packet already linked\n");
		return;
	}

	pPacket->pPrev = pList->pPrev;
	pList->pPrev->pNext = pPacket;
	pList->pPrev = pPacket;
	pPacket->pNext = pList;

	pStart = messagedata.data;
	pPacket->pPacketData = new unsigned char[ messagedata.cursize ];
	Q_memcpy( pPacket->pPacketData, pStart, messagedata.cursize );
	pPacket->nSize    = messagedata.cursize;
	pPacket->receivedTime = timestamp;   // Our time stamp.
	pPacket->net_from     = *net_from;
}

// Actual lag to use
static float gFakeLag = 0.0;

// How quickly we converge to a new value for fakelag
#define FAKELAG_CONVERGE	200  // ms per second

/*
==============================
NET_AdjustLag

==============================
*/
void NET_AdjustLag( void )
{
	static double lasttime = 0;
	double dt;
	float	diff;
	float	converge;

	if ( IsSinglePlayerGame() )
	{
		fakelag.SetValue( 0 );
		gFakeLag = 0.0f;
	}

	// Bound time step
	dt = host_time - lasttime;
	dt = max( 0.0, dt );
	dt = min( 0.1, dt );

	lasttime = host_time;

	// Already converged?
	if ( fakelag.GetFloat() == gFakeLag )
		return;

	// Figure out how far we have to go
	diff = fakelag.GetFloat() - gFakeLag;
	// How much can we converge this frame
	converge = FAKELAG_CONVERGE * dt;

	// Last step, go the whole way
	if ( converge > fabs( diff ) )
	{
		converge = fabs( diff );
	}

	// Invert if needed
	if ( diff < 0.0 )
	{
		converge = -converge;
	}

	// Converge toward fakelag.GetFloat()
	gFakeLag += converge;
}


qboolean    NET_LagPacket (qboolean newdata, netsrc_t sock, netadr_t *from, sizebuf_t *data)
{
	packetlag_t *pNewPacketLag;
	packetlag_t *pPacket;
	static int losscount[2];
	float	curtime;

	if (gFakeLag <= 0.0)
	{
		// Never leave any old msgs around
		NET_ClearLagData( true, true );
		return newdata;
	}

	curtime = host_time;

	if (newdata)
	{
		if ( fakeloss.GetFloat() )
		{
			losscount[sock]++;

			if ( fakeloss.GetFloat() > 0.0f )
			{
				// Act like we didn't hear anything if we are going to lose the packet.
				// Depends on random # generator.
				if (RandomInt(0,100) <= (int)fakeloss.GetFloat())
					return false;
			}
			else
			{
				int ninterval;

				ninterval = (int)(fabs( fakeloss.GetFloat() ) );
				ninterval = max( 2, ninterval );

				if ( ! ( losscount[sock] % ninterval ) )
				{
					return false;
				}
			}
		}

		pNewPacketLag = (packetlag_t *)new packetlag_t;
		memset( pNewPacketLag, 0, sizeof( *pNewPacketLag) );

		NET_AddToLagged(sock, &g_pLagData[sock], pNewPacketLag, from, *data, curtime );
	}

	// Now check the correct list and feed any message that is old enought.
	pPacket = g_pLagData[sock].pNext;

// Find an old enough packet.  If none are old enough, return false.
	while (pPacket != &g_pLagData[sock])
	{
		if (pPacket->receivedTime <= ( curtime - (gFakeLag/1000.0f)))
		{
			NET_RemoveFromPacketList(pPacket);
		
			NET_TransferRawData( &in_message, pPacket->pPacketData, pPacket->nSize );

			in_from = pPacket->net_from;
			
			if ( pPacket->pPacketData )
			{
				free ( pPacket->pPacketData );
			}
			delete pPacket;
			return true;
		}
		pPacket = pPacket->pNext;
	}
	return false;
}

#define MAX_SPLITPACKET_SPLITS ( NET_MAX_MESSAGE / SPLIT_SIZE )
#define SPLIT_PACKET_STALE_TIME		15.0f

class CSplitPacketEntry
{
public:
	CSplitPacketEntry()
	{
		memset( &from, 0, sizeof( from ) );

		int i;
		for ( i = 0; i < MAX_SPLITPACKET_SPLITS; i++ )
		{
			splitflags[ i ] = -1;
		}

		memset( &netsplit, 0, sizeof( netsplit ) );
		lastactivetime = 0.0f;
	}

public:
	netadr_t		from;
	int				splitflags[ MAX_SPLITPACKET_SPLITS ];
	LONGPACKET		netsplit;
	// host_time the last time any entry was received for this entry
	double			lastactivetime;
};

CUtlVector< CSplitPacketEntry >	g_SplitPacketEntries( 1, 1 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void NET_DiscardStaleSplitpackets( void )
{
	int i;
	for ( i = g_SplitPacketEntries.Count() - 1; i >= 0; i-- )
	{
		CSplitPacketEntry *entry = &g_SplitPacketEntries[ i ];
		Assert( entry );

		if ( host_time < ( entry->lastactivetime + SPLIT_PACKET_STALE_TIME ) )
			continue;

		g_SplitPacketEntries.Remove( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *from - 
// Output : CSplitPacketEntry
//-----------------------------------------------------------------------------
CSplitPacketEntry *NET_FindOrCreateSplitPacketEntry( netadr_t *from )
{
	int i, count = g_SplitPacketEntries.Count();
	CSplitPacketEntry *entry = NULL;
	for ( i = 0; i < count; i++ )
	{
		entry = &g_SplitPacketEntries[ i ];
		Assert( entry );

		if ( NET_CompareAdr( entry->from, *from ) )
			break;
	}

	if ( i >= count )
	{
		CSplitPacketEntry newentry;
		newentry.from = *from;

		g_SplitPacketEntries.AddToTail( newentry );

		entry = &g_SplitPacketEntries[ g_SplitPacketEntries.Count() - 1 ];
	}

	Assert( entry );
	return entry;
}

bool g_bForceShowMessages = false;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pData - 
//			size - 
//			*outSize - 
// Output : qboolean
//-----------------------------------------------------------------------------
qboolean NET_GetLong( netadr_t *from, byte *pData, int size, int *outSize )
{
	int				packetNumber, packetCount, sequenceNumber, offset;
	short			packetID;
	SPLITPACKET		*pHeader;
	
	CSplitPacketEntry *entry = NET_FindOrCreateSplitPacketEntry( from );
	Assert( entry );
	if ( !entry )
		return false;

	entry->lastactivetime = host_time;
	Assert( NET_CompareAdr( entry->from, *from ) );

	pHeader = ( SPLITPACKET * )pData;
	sequenceNumber	= pHeader->sequenceNumber;
	packetID		= pHeader->packetID;
	// High byte is packet number
	packetNumber	= ( packetID >> 8 );	
	// Low byte is number of total packets
	packetCount		= ( packetID & 0xff );	

	if ( packetNumber >= MAX_SPLITPACKET_SPLITS ||
		 packetCount > MAX_SPLITPACKET_SPLITS )
	{
		Con_Printf( "NET_GetLong:  Split packet from %s with too many split parts (number %i/ count %i) where %i is max count allowed\n", 
			NET_AdrToString( *from ), 
			packetNumber, 
			packetCount, 
			MAX_SPLITPACKET_SPLITS );
		return false;
	}

	// First packet in split series?
	if ( entry->netsplit.currentSequence == -1 || 
		sequenceNumber != entry->netsplit.currentSequence )
	{
		entry->netsplit.currentSequence	= sequenceNumber;
		entry->netsplit.splitCount		= packetCount;
	}

	size -= sizeof(SPLITPACKET);

	if ( entry->splitflags[ packetNumber ] != sequenceNumber )
	{
		// Last packet in sequence? set size
		if ( packetNumber == (packetCount-1) )
		{
			entry->netsplit.totalSize = (packetCount-1) * SPLIT_SIZE + size;
		}

		entry->netsplit.splitCount--;		// Count packet
		entry->splitflags[ packetNumber ] = sequenceNumber;

		if ( net_showpackets.GetInt() )
		{
			Con_Printf( "<-- Split packet %i of %i from %s\n", packetNumber, packetCount, NET_AdrToString( *from ) );
		}
	}
	else
	{
		Con_Printf( "NET_GetLong:  Ignoring duplicated split packet %i of %i ( %i bytes ) from %s\n", packetNumber + 1, packetCount, size, NET_AdrToString( *from ) );
	}


	// Copy the incoming data to the appropriate place in the buffer
	offset = (packetNumber * SPLIT_SIZE);
	memcpy( entry->netsplit.buffer + offset, pData + sizeof(SPLITPACKET), size );
	
	// Have we received all of the pieces to the packet?
	if ( entry->netsplit.splitCount <= 0 )
	{
		entry->netsplit.currentSequence = -1;	// Clear packet
		if ( entry->netsplit.totalSize > sizeof(entry->netsplit.buffer) )
		{
			Con_Printf("Split packet too large! %d bytes from %s\n", entry->netsplit.totalSize, NET_AdrToString( *from ) );
			return false;
		}

		memcpy( pData, entry->netsplit.buffer, entry->netsplit.totalSize );
		*outSize = entry->netsplit.totalSize;

		// Do this in release for now...
#if 1 // defined( _DEBUG )
		if ( packetCount >= 4 )
		{
			g_bForceShowMessages = true;
		}
#endif

		return true;
	}

	return false;
}

qboolean	NET_QueuePacket (netsrc_t sock)
{
	int				ret;
	struct sockaddr	from;
	int				fromlen;
	int				net_socket = 0;
	int				err;
	unsigned char	buf[ NET_MAX_MESSAGE ];

	net_socket = ip_sockets[sock];
	if (net_socket)
	{
		fromlen = sizeof(from);
		ret = g_pVCR->Hook_recvfrom(net_socket, (char *)buf, NET_MAX_MESSAGE, 0, (struct sockaddr *)&from, (int *)&fromlen );
		if ( ret != -1 )
		{
			SockadrToNetadr( &from, &in_from );

			if ( ret < NET_MAX_MESSAGE )
			{
				// Transfer data
				NET_TransferRawData( &in_message, buf, ret );

				// Check for split message
				if ( *(int *)in_message.data == -2 )
				{
					return NET_GetLong( &in_from, in_message.data, ret, &in_message.cursize );
				}

				// Lag the packet, if needed
				return NET_LagPacket( true, sock, &in_from, &in_message );
			}
			else
			{
				Con_DPrintf ( "NET_QueuePacket:  Oversize packet from %s\n", NET_AdrToString (in_from) );
			}
		}
		else
		{
#if defined( _WIN32 )
			err = WSAGetLastError();
#else
			err = errno;
#endif
			switch ( err )
			{
			case WSAEWOULDBLOCK:
			case WSAECONNRESET:
			case WSAECONNREFUSED:
			case WSAEMSGSIZE:
				break;
			default:
				// Let's continue even after errors
				Con_DPrintf ("NET_QueuePacket: %s\n", NET_ErrorString(err));
				break;
			}
		}
	}
	
	// Allow lagging system to return a packet
	return NET_LagPacket( false, sock, NULL, NULL );
}

typedef struct net_messages_s
{
	struct net_messages_s *next;
	qboolean preallocated;
	unsigned char *buffer;
	netadr_t from;
	int buffersize;
} net_messages_t;

net_messages_t *messages[2] = { NULL, NULL };

net_messages_t *NET_AllocMsg( int size );

// Create general message queues
#define NUM_MSG_QUEUES 40
#define MSG_QUEUE_SIZE 1536

net_messages_t *normalqueue = NULL;

void *hNetThread = NULL;
unsigned long dwNetThreadId;
//HANDLE hNetDone;

// sleeps until one of the net sockets has a message.  Lowers thread CPU usage immensely
int NET_Sleep( void )
{
	fd_set	fdset;
	int i;
	int sock;
	struct timeval tv;
	int number;

	FD_ZERO(&fdset);
	i = 0;
	for ( sock = 0; sock < 2; sock++ )
	{
		if ( ip_sockets[sock])
		{
			FD_SET( ip_sockets[sock] , &fdset); // network socket
			if ( ip_sockets[sock] > i )
				i = ip_sockets[sock];
		}
	}

	tv.tv_sec = 0;
	tv.tv_usec = 20 * 1000;

	// Block infinitely until a message is in the queue
	number = select( i+1, &fdset, NULL, NULL, net_sleepforever ? NULL : &tv );
	return number;
}

#if defined( _WIN32 )
DWORD NET_ThreadFunc( LPVOID pv )
{
	//Plat_RegisterThread("NET_ThreadFunc");

	qboolean done = false;
	qboolean queued;
	int i;
	net_messages_t *pmsg, *p;
	int sockets_ready;

	while ( 1 )
	{
		// Wait for messages
		sockets_ready = NET_Sleep();
		// Service messages
		//
		done = false;
		while ( !done && sockets_ready )
		{
			done = true;
			for ( i = 0; i < 2; i++ )
			{
				// Link
				NET_ThreadLock();

				queued = NET_QueuePacket( (netsrc_t)i );
				if ( queued )
				{
					done = false;

					// Copy in data
					pmsg = NET_AllocMsg( in_message.cursize );
					Q_memcpy( pmsg->buffer, in_message.data, in_message.cursize );
					pmsg->from = in_from;

					if ( !messages[ i ] )
					{
						pmsg->next = messages[ i ];
						messages[ i ] = pmsg;
					}
					else
					{
						p = messages[ i ];
						while ( p->next )
							p = p->next;

						p->next = pmsg;
						pmsg->next = NULL;
					}
				}

				NET_ThreadUnlock();
			}
		}

		Sys_Sleep( 1 );
	}

	return 0;
}
#endif

void NET_StartThread( void )
{
	if ( !use_thread )
		return;

	if ( net_thread_initialized )
		return;

	net_thread_initialized = true;

#if defined( _WIN32 )
	InitializeCriticalSection( &net_cs );

	hNetThread = CreateThread (NULL,  0, (LPTHREAD_START_ROUTINE) NET_ThreadFunc, 0, 0, &dwNetThreadId );
	if ( !hNetThread )
	{
		DeleteCriticalSection( &net_cs );
		net_thread_initialized = false;
		use_thread = 0;
		Sys_Error( "Couldn't initialize network thread, run without -net_thread\n" );
		return;
	}
#endif
}

void NET_StopThread( void )
{
	if ( !use_thread )
		return;

	if ( !net_thread_initialized )
		return;
#if defined( _WIN32 )
	TerminateThread( hNetThread, 0 );
	DeleteCriticalSection( &net_cs );
#endif

	net_thread_initialized = false;
}

void *net_malloc( size_t size )
{
	unsigned char *pbuffer;
	pbuffer = new unsigned char[ size ];
	memset( pbuffer, 0, size );
	return ( void * )pbuffer;
}

net_messages_t *NET_AllocMsg( int size )
{
	net_messages_t *pmsg;

	if ( ( size > MSG_QUEUE_SIZE ) || !normalqueue )
	{
		pmsg = ( net_messages_t *)net_malloc( sizeof( net_messages_t ) );
		pmsg->buffer = ( unsigned char *)net_malloc( size );
		pmsg->buffersize = size;
		pmsg->preallocated = false;

		return pmsg;
	}

	pmsg = normalqueue;
	normalqueue = normalqueue->next;

	pmsg->buffersize = size;

	return pmsg;
}

void NET_FreeMsg( net_messages_t *pmsg )
{
	if ( pmsg->preallocated )
	{
		// return to queue
		pmsg->next = normalqueue;
		normalqueue = pmsg;
		return;
	}

	delete[] pmsg->buffer;
	delete pmsg;
}

qboolean	NET_GetPacket (netsrc_t sock)
{
	net_messages_t *pmsg;

	// Assume we don't get a packet at all
	qboolean bret = false;

	NET_AdjustLag();

	NET_ThreadLock();

	NET_DiscardStaleSplitpackets();

	// If we got a message from the loopback system, see if it should be lagged.
	if ( NET_GetLoopPacket (sock, &in_from, &in_message) )
	{
		bret = NET_LagPacket (true, sock, &in_from, &in_message);
	}
	else
	// No loopback, if not threaded, see if we got any over wire?
	{
		if ( !use_thread )
		{
			// NET_QueuePacket deals with lagging if needed, so no need to recall that
			bret = NET_QueuePacket( sock );
		}

		// Didn't get any over wire ( or we are using threads ), see if lag system has one waiting
		if ( !bret )
		{
			bret = NET_LagPacket (false, sock, NULL, NULL);
		}
	}
	
	// Got a packet from fakelag system
	if ( bret )
	{
		Q_memcpy( net_message.data, in_message.data, in_message.cursize );
		net_message.cursize = in_message.cursize;
		net_from = in_from;
	}
	// Got packets from real network queue thread
	else if ( messages[ sock ] )
	{
		// Dequeue message
		pmsg = messages[ sock ];
		messages[ sock ] = messages[ sock ]->next;

		// Copy in data
		net_message.cursize = pmsg->buffersize;
		Q_memcpy( net_message.data, pmsg->buffer, pmsg->buffersize );
		net_from = pmsg->from;

		MSG_GetReadBuf()->Reset();

		NET_FreeMsg( pmsg );

		// Flag that there is a message waiting
		bret = true;
	}

	NET_ThreadUnlock();
	return bret;
}

void NET_AllocateQueues( void )
{
	int i;
	net_messages_t *p;

	for ( i = 0; i < NUM_MSG_QUEUES; i++ )
	{
		p = ( net_messages_t *)new net_messages_t;
		memset( p, 0, sizeof( *p ) );
		p->buffer = new unsigned char[ MSG_QUEUE_SIZE ];
		memset( p->buffer, 0, MSG_QUEUE_SIZE );

		p->preallocated = true;

		p->next = normalqueue;
		normalqueue = p;
	}

	NET_StartThread();
}

void NET_FlushQueues( void )
{
	int i; 
	net_messages_t *p, *n;

	NET_StopThread();

	for ( i = 0; i < 2; i++ )
	{
		p = messages[ i ];
		while ( p )
		{
			n = p->next;
			delete[] p->buffer;
			delete p;
			p = n;
		}
		messages[ i ] = NULL;
	}

	p = normalqueue;
	while ( p )
	{
		n = p->next;
		delete[] p->buffer;
		delete p;
		p = n;
	}
	normalqueue = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : sock - 
//			s - 
//			buf - 
//			len - 
//			flags - 
//			to - 
//			tolen - 
// Output : int
//-----------------------------------------------------------------------------
int NET_SendTo( bool verbose, SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR * to, int tolen )
{	
	int nSend = 0;
	
	// Don't send anything out in VCR mode.. it just annoys other people testing in multiplayer.
	if ( g_pVCR->GetMode() != VCR_Playback )
	{
		nSend = sendto( s, buf, len, flags, to, tolen );
	}

#if defined( _DEBUG )
	if ( verbose && 
		( nSend > 0 ) && 
		( tolen > MAX_ROUTEABLE_PACKET ) )
	{
		Con_DPrintf( "NET_SendTo:  Packet length (%i) > (%i) bytes\n", tolen, MAX_ROUTEABLE_PACKET );
	}
#endif
	return nSend;
}

#if defined( _DEBUG )

#include "FileSystem.h"
#include "FileSystem_Engine.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
char const *NET_GetDebugFilename( char const *prefix )
{
	static char filename[ MAX_OSPATH ];

	int i;

	for ( i = 0; i < 10000; i++ )
	{
		Q_snprintf( filename, sizeof( filename ), "debug/%s%04i.dat", prefix, i );
		if ( g_pFileSystem->FileExists( filename ) )
			continue;

		return filename;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//			*buf - 
//			len - 
//-----------------------------------------------------------------------------
void NET_StorePacket( char const *filename, byte const *buf, int len )
{
	FileHandle_t fh;

	g_pFileSystem->CreateDirHierarchy( "debug/", "GAME" );
	fh = g_pFileSystem->Open( filename, "wb" );
	if ( FILESYSTEM_INVALID_HANDLE != fh )
	{
		g_pFileSystem->Write( buf, len, fh );
		g_pFileSystem->Close( fh );
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *adr - 
// Output : char const
//-----------------------------------------------------------------------------
char const *NET_GetPlayerNameForAdr( netadr_t *adr )
{
	if ( !adr )
		return "NULL adr!!!";

	// Is it the client
#ifndef SWDS
	if ( NET_CompareAdr( *adr, cls.netchan.remote_address ) )
	{
		return cl.players[ cl.playernum ].name;
	}
#endif

	int i;
	for ( i = 0; i < svs.maxclients; i++ )
	{
		client_t *cl = &svs.clients[ i ];
		if ( NET_CompareAdr( *adr, cl->netchan.remote_address ) )
		{
			return cl->name;
		}
	}

	return "No player name for remote address!!!";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : sock - 
//			s - 
//			buf - 
//			len - 
//			flags - 
//			to - 
//			tolen - 
// Output : int
//-----------------------------------------------------------------------------
int NET_SendLong( netsrc_t sock, SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR * to, int tolen )
{
	static long gSequenceNumber = 1;

	if ( len > MAX_ROUTEABLE_PACKET )	// Do we need to break this packet up?
	{
		// yep
		char packet[MAX_ROUTEABLE_PACKET];
		int totalSent, ret, size, packetCount, packetNumber;
		SPLITPACKET *pPacket;
		int originalSize = len;

		gSequenceNumber++;
		if ( gSequenceNumber < 0 )
		{
			gSequenceNumber = 1;
		}

		pPacket = (SPLITPACKET *)packet;
		pPacket->netID = -2;
		pPacket->sequenceNumber = gSequenceNumber;
		packetNumber = 0;
		totalSent = 0;
		packetCount = (len + SPLIT_SIZE - 1) / SPLIT_SIZE;

#if defined( _DEBUG )
		if ( packetCount > 3 )
		{
			char const *filename = NET_GetDebugFilename( "splitpacket" );
			if ( filename )
			{
				Con_Printf( "Saving split packet of %i bytes and %i packets to file %s\n",
					len, packetCount, filename );
	
				NET_StorePacket( filename, (byte const *)buf, len );
			}
			else
			{
				Con_Printf( "Too many files in debug directory, clear out old data!\n" );
			}
		}
#endif

		while ( len > 0 )
		{
			size = min( SPLIT_SIZE, len );

			pPacket->packetID = ( packetNumber << 8 ) + packetCount;
			
			memcpy( packet + sizeof(SPLITPACKET), buf + (packetNumber * SPLIT_SIZE), size );
			
			ret = NET_SendTo( false, s, packet, size + sizeof(SPLITPACKET), flags, to, tolen );
			if ( ret < 0 )
			{
				return ret;
			}

			if ( ret >= size )
			{
				totalSent += size;
			}
	
			len -= size;
			packetNumber++;

			// FIXME:  This was 15, but if you have a lot of packets, that will pause the server for a long time
#ifdef _WIN32
			Sleep( 1 );
#elif _LINUX
			usleep( 1 );
#endif

// Always bitch about split packets in debug
#if !defined( _DEBUG )
			if ( net_showpackets.GetInt() )
#endif
			{
				netadr_t adr;
				memset( &adr, 0, sizeof( adr ) );

				SockadrToNetadr( (struct sockaddr *)to, &adr );

				Con_Printf( "Split packet %i/%i (size %i dest:  %s:%s)\n",
					packetNumber, 
					packetCount, 
					originalSize, 
					NET_GetPlayerNameForAdr( &adr ), 
					NET_AdrToString( adr ) );
			}
		}
		
		return totalSent;
	}
	else
	{
		int nSend = 0;
		nSend = NET_SendTo( true, s, buf, len, flags, to, tolen );
		return nSend;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : sock - 
//			length - 
//			*data - 
//			to - 
// Output : void NET_SendPacket
//-----------------------------------------------------------------------------
void NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	int		ret;
	struct sockaddr	addr;
	int		net_socket;

	if ( to.type == NA_LOOPBACK )
	{
		NET_SendLoopPacket (sock, length, data, to);
		return;
	}

	if (to.type == NA_BROADCAST)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
			return;
	}
	else if (to.type == NA_IP)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
			return;
	}
	else
	{
		Sys_Error ("NET_SendPacket: bad address type");
		return;
	}

	NetadrToSockadr (&to, &addr);

	ret = NET_SendLong( sock, net_socket, (const char *)data, length, 0, &addr, sizeof(addr) );
	if (ret == -1)
	{
		int err;
		
#if defined( _WIN32 )
		err = WSAGetLastError();
#else
		err = errno;
#endif
		// wouldblock is silent
		if (err == WSAEWOULDBLOCK)
			return;

		if ( err == WSAECONNRESET )
			return;

		// some PPP links dont allow broadcasts
		if ( (err == WSAEADDRNOTAVAIL) && ( to.type == NA_BROADCAST ) )
			return;

		if (cls.state == ca_dedicated)	// let dedicated servers continue after errors
		{
			Con_Printf ("NET_SendPacket ERROR: %s\n", NET_ErrorString(err));
		}
		else
		{
			if ( err == WSAEADDRNOTAVAIL )
			{
				Con_DPrintf ("NET_SendPacket Warning: %s : %s\n", NET_ErrorString(err), NET_AdrToString (to));
			}
			else
			{
				Sys_Error ("NET_SendPacket ERROR: %s\n", NET_ErrorString(err));
			}
		}
	}
}

/*
====================
NET_IPSocket
====================
*/
int NET_IPSocket ( const char *net_interface, int& port)
{
	int					newsocket;
	struct sockaddr_in	address;
	unsigned long		_true = 1;
	int					i = 1;
	int					err;

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
#if defined( _WIN32 )
		err = WSAGetLastError();
#else
		err = errno;
#endif
		if (err != WSAEAFNOSUPPORT)
			Con_Printf ("WARNING: UDP_OpenSocket: socket: %s", NET_ErrorString(err));
		return 0;
	}

	// make it non-blocking
	if (ioctlsocket (newsocket, FIONBIO, &_true) == -1)
	{
#if defined( _WIN32 )
		err = WSAGetLastError();
#else
		err = errno;
#endif	
		Con_Printf ("WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString(err));
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == -1)
	{
#if defined( _WIN32 )
		err = WSAGetLastError();
#else
		err = errno;
#endif	
		Con_Printf ("WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString(err));
		return 0;
	}

	// make it reusable
	if ( CommandLine()->FindParm( "-reuse" ) )
	{
		if (setsockopt(newsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&_true, sizeof(qboolean)) == -1)
		{
#if defined( _WIN32 )
			err = WSAGetLastError();
#else
			err = errno;
#endif
			Con_Printf ("WARNING: UDP_OpenSocket: setsockopt SO_REUSEADDR: %s\n", NET_ErrorString(err));
			return 0;
		}
	}

	if (!net_interface || !net_interface[0] || !stricmp(net_interface, "localhost"))
		address.sin_addr.s_addr = INADDR_ANY;
	else
		NET_StringToSockaddr (net_interface, (struct sockaddr *)&address);

	address.sin_family = AF_INET;

	int portmax = 10;
	bool success = false;
	int p;
	for ( p = 0; p < portmax; p++ )
	{
		if (port == PORT_ANY)
		{
			address.sin_port = 0;
		}
		else
		{
			address.sin_port = htons((short)( port + p ));
		}

		if ( bind (newsocket, (struct sockaddr *)&address, sizeof(address)) != -1 )
		{
			success = true;
			if ( port != PORT_ANY && p != 0 )
			{
				Con_DPrintf( "UDP Socket bound to non-default port %i because original port was already in use.\n", port + p );
				port = port + p;
			}
			break;
		}

#if defined( _WIN32 )
		err = WSAGetLastError();
#else
		err = errno;
#endif	

		if ( port == PORT_ANY || err != WSAEADDRINUSE )
		{
			Con_Printf ("WARNING: UDP_OpenSocket: bind: %s\n", NET_ErrorString(err));
			closesocket (newsocket);
			return 0;
		}

		// Try next port
	}

	if ( !success )
	{
		Con_Printf( "WARNING: UDP_OpenSocket: unable to bind socket\n" );
		closesocket( newsocket );
		return 0;
	}

	/*
	if ( getsockopt( newsocket, IPPROTO_IP, IP_TOS, (char *)&tos, &toslen ) == -1 )
	{
		err = WSAGetLastError();
		Con_Printf ("WARNING: UDP_OpenSocket: getsockopt IP_TOS: %s\n", NET_ErrorString(err));
	}
	else
	{
		tos = SERVICETYPE_GUARANTEED;
		setsockopt( newsocket, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof( tos ) );
	}
	*/

	return newsocket;
}

/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP (void)
{
	ConVar	*ip;
	int		port;
	int		dedicated;
	int     sv_port = 0, cl_port = 0;
	static qboolean bFirst = true;

	ip = &ipname;

	dedicated = cls.state == ca_dedicated;

	NET_ThreadLock();

	if (!ip_sockets[NS_SERVER])
	{
		port = iphostport.GetInt();
		if (!port)
		{
			port = hostport.GetInt();
			if (!port)
			{
				port = defport.GetInt();
			}
		}

		ip_sockets[NS_SERVER] = NET_IPSocket (ip->GetString(), port);
		if (!ip_sockets[NS_SERVER] && dedicated)
		{
			Sys_Error ("Couldn't allocate dedicated server IP port");
		}

		sv_port = port;
	}

	NET_ThreadUnlock();

	// dedicated servers don't need client ports
	if (cls.state == ca_dedicated)
		return;

	NET_ThreadLock();

	if (!ip_sockets[NS_CLIENT])
	{
		port = ip_clientport.GetInt();
		if (!port)
		{
			port = clientport.GetInt();
			if (!port)
				port = PORT_ANY;
		}

		ip_sockets[NS_CLIENT] = NET_IPSocket (ip->GetString(), port);
		if (!ip_sockets[NS_CLIENT])
		{
			port = PORT_ANY;
			ip_sockets[NS_CLIENT] = NET_IPSocket (ip->GetString(), port);
		}

		cl_port = port;
	}

	NET_ThreadUnlock();

	if ( bFirst )
	{
		bFirst = false;
		Con_Printf( "Networking ports (%i sv / %i cl)\n", sv_port, cl_port );
	}
}

char const *NET_GetHostName()
{
	static char buff[ 512 ];
	gethostname(buff, 512);
	return buff;
}

/*
================
NET_GetLocalAddress

Returns the servers' ip address as a string.
================
*/
void NET_GetLocalAddress (void)
{
	char	buff[512];
	struct sockaddr_in	address;
	int		namelen;
	int     net_error = 0;
	Q_memset( &net_local_adr, 0, sizeof(netadr_t));

	if ( noip )
	{
		Con_Printf("TCP/IP Disabled.\n");
	}
	else
	{
		// If we have changed the ip var from the command line, use that instead.
		if (Q_strcmp(ipname.GetString(), "localhost"))
		{
			Q_strcpy(buff, ipname.GetString());
		}
		else
		{
			gethostname(buff, 512);
		}

		// Ensure that it doesn't overrun the buffer
		buff[512-1] = 0;

		NET_StringToAdr (buff, &net_local_adr);
		
		namelen = sizeof(address);
		if ( getsockname (ip_sockets[NS_SERVER], (struct sockaddr *)&address, (socklen_t *)&namelen) != 0)
		{
#if defined( _WIN32 )
			net_error = WSAGetLastError();
#else
			net_error = errno;
#endif	
			noip = true;
			Con_Printf("Could not get TCP/IP address, TCP/IP disabled\nReason:  %s\n", NET_ErrorString(net_error));
		}
		else
		{
			net_local_adr.port = address.sin_port;
			Con_Printf("Server IP address %s\n", NET_AdrToString (net_local_adr) );

			net_address.SetValue( va( "%s", NET_AdrToString( net_local_adr ) ) );
		}
	}
}

static int net_configured = 0;

/*
====================
NET_IsConfigured

Is winsock ip initialized?
====================
*/
int     NET_IsConfigured( void )
{
	return net_configured;
}

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void	NET_Config (qboolean multiplayer)
{
	int		i;
	static	qboolean	old_config;
	static qboolean bFirst = true;

	if (old_config == multiplayer)
		return;

	old_config = multiplayer;

	if (!multiplayer)
	{	// shut down any existing sockets
		NET_ThreadLock();

		for (i=0 ; i<2 ; i++)
		{
			if (ip_sockets[i])
			{
				closesocket (ip_sockets[i]);
				ip_sockets[i] = 0;
			}
		}

		NET_ThreadUnlock();
	}
	else
	{	// open sockets

		if ( !noip )
		{
			NET_OpenIP ();
		}

		// Get our local address, if possible
		if ( bFirst )
		{
			bFirst = false;
			NET_GetLocalAddress ();
		}
	}

	net_configured = multiplayer ? 1 : 0;
}

/*
====================
NET_Init

====================
*/
void NET_Init ()
{
	int i;

	// Parameters.

#if defined( _WIN32 )
	if ( CommandLine()->FindParm( "-netthread" ) )
	{
		use_thread = 1;
	}
#endif

	if (CommandLine()->FindParm( "-netsleep" ) )
	{
		net_sleepforever = 0;
	}

	if (CommandLine()->FindParm("-noip"))
	{
		noip = true;
	}

	int hPort = CommandLine()->ParmValue( "-port", -1 );
	if ( hPort != -1 )
	{
		hostport.SetValue( hPort );
	}

	//
	// init the message buffer
	//
	net_message.maxsize = sizeof(net_message_buffer);
	net_message.data = net_message_buffer;

	in_message.maxsize = sizeof(in_message_buf);
	in_message.data = in_message_buf;

	for (i = 0; i < 2; i++)
	{
		g_pLagData[i].pNext = g_pLagData[i].pPrev = &g_pLagData[i];  // List of lag structures, if fakelag is set.
	}

	NET_AllocateQueues();
}

void NET_ClearLagData( qboolean bClient, qboolean bServer )
{
	NET_ThreadLock();

	if ( bClient )
	{
		NET_ClearLaggedList(&g_pLagData[NS_CLIENT]);
	}

	if ( bServer )
	{
		NET_ClearLaggedList(&g_pLagData[NS_SERVER]);
	}

	NET_ThreadUnlock();
}


/*
====================
NET_Shutdown

====================
*/
void	NET_Shutdown (void)
{
	NET_ThreadLock();

	NET_ClearLaggedList(&g_pLagData[0]);
	NET_ClearLaggedList(&g_pLagData[1]);

	NET_ThreadUnlock();

	NET_Config(false);

	// Clear out any messages that are left over.
	NET_FlushQueues();
}
