// netadr.h
#ifndef NETADR_H
#define NETADR_H
#ifdef _WIN32
#pragma once
#endif

typedef enum
{ 
	NA_UNUSED,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX,
} netadrtype_t;

typedef struct netadr_s
{
	netadrtype_t	type;
	unsigned char	ip[4];
	unsigned char	ipx[10];
	unsigned short	port;
} netadr_t;

#endif // NETADR_H
