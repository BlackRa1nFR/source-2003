// UTIL.c : implementation file
//

#include "stdafx.h"
#include "MasterTest.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/
typedef unsigned char byte;
BOOL bigendien = FALSE;

short   ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   ShortNoSwap (short l)
{
	return l;
}

int32    LongSwap (int32 l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int32)b1<<24) + ((int32)b2<<16) + ((int32)b3<<8) + b4;
}

int32     LongNoSwap (int32 l)
{
	return l;
}

float FloatSwap (float f)
{
	union
	{
		float   f;
		byte    b[4];
	} dat1, dat2;
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}


// WINSOCK ERROR MESSAGES
char *UTIL_GetSocketErrorText(int iError)
{
	switch (iError)
	{
	case WSAEINTR:
		return "WSAEINTR";
	case WSAEBADF:
		return "WSAEBADF";
	case WSAEACCES:
		return "WSAEACCES";
	case WSAEFAULT:
		return "WSAEFAULT";
	case WSAEINVAL:
		return "WSAEINVAL";
	case WSAEMFILE:
		return "WSAEMFILE";
	case WSAEWOULDBLOCK:
		return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS:
		return "WSAEINPROGRESS";
	case WSAEALREADY:
		return "WSAEALREADY";
	case WSAENOTSOCK:
		return "WSAENOTSOCK";
	case WSAEDESTADDRREQ:
		return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE:
		return "WSAEMSGSIZE";
	case WSAEPROTOTYPE:
		return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT:
		return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT:
		return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT:
		return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP:
		return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT:
		return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT:
		return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE:
		return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL:
		return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN:
		return "WSAENETDOWN";
	case WSAENETUNREACH:
		return "WSAENETUNREACH";
	case WSAENETRESET:
		return "WSAENETRESET";
	case WSAECONNABORTED:
		return "WSAECONNABORTED";
	case WSAECONNRESET:
		return "WSAECONNRESET";
	case WSAENOBUFS:
		return "WSAENOBUFS";
	case WSAEISCONN:
		return "WSAEISCONN";
	case WSAENOTCONN:
		return "WSAENOTCONN";
	case WSAESHUTDOWN:
		return "WSAESHUTDOWN";
	case WSAETOOMANYREFS:
		return "WSAETOOMANYREFS";
	case WSAETIMEDOUT:
		return "WSAETIMEDOUT";
	case WSAECONNREFUSED:
		return "WSAECONNREFUSED";
	case WSAELOOP:
		return "WSAELOOP";
	case WSAENAMETOOLONG:
		return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN:
		return "WSAEHOSTDOWN";
	case WSAEHOSTUNREACH:
		return "WSAEHOSTUNREACH";
	case WSAENOTEMPTY:
		return "WSAENOTEMPTY";
	case WSAEPROCLIM:
		return "WSAEPROCLIM";
	case WSAEUSERS:
		return "WSAEUSERS";
	case WSAEDQUOT:
		return "WSAEDQUOT";
	case WSAESTALE:
		return "WSAESTALE";
	case WSAEREMOTE:
		return "WSAEREMOTE";
	case WSASYSNOTREADY:
		return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED:
		return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED:
		return "WSANOTINITIALISED";
	case WSAEDISCON:
		return "WSAEDISCON";
/*
	case WSAENOMORE:
		return "WSAENOMORE";
	case WSAECANCELLED:
		return "WSAECANCELLED";
	case WSAEINVALIDPROCTABLE:
		return "WSAEINVALIDPROCTABLE";
	case WSAEINVALIDPROVIDER:
		return "WSAEINVALIDPROVIDER";
	case WSAEPROVIDERFAILEDINIT:
		return "WSAEPROVIDERFAILEDINIT";
	case WSASYSCALLFAILURE:
		return "WSASYSCALLFAILURE";
	case WSASERVICE_NOT_FOUND:
		return "WSASERVICE_NOT_FOUND";
	case WSATYPE_NOT_FOUND:
		return "WSATYPE_NOT_FOUND";
	case WSA_E_NO_MORE:
		return "WSA_E_NO_MORE";
	case WSA_E_CANCELLED:
		return "WSA_E_CANCELLED";
	case WSAEREFUSED:
		return "WSAEREFUSED";
	case WSAHOST_NOT_FOUND:
		return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN:
		return "WSATRY_AGAIN";
	case WSANO_RECOVERY:
		return "WSANO_RECOVERY";
	case WSANO_DATA:
		return "WSANO_DATA";
*/
	default:
		return "UNKNOWN CODE";
	};
}
