// cl_servercache.h
#if !defined( CL_SERVERCACHE_H )
#define CL_SERVERCACHE_H
#ifdef _WIN32
#pragma once
#endif

// Server cache to replace slist command.
#define MAX_LOCAL_SERVERS 16

#define MAX_LOCAL_SERVERINFOSTRING 2048 
typedef struct
{
	int			inuse;
	netadr_t	adr;
	char		info[ MAX_LOCAL_SERVERINFOSTRING ];
} server_cache_t;

#endif