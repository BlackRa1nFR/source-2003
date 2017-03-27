// filter.h
#if !defined( FILTER_H )
#define FILTER_H
#ifdef _WIN32
#pragma once
#endif

typedef struct
{
	unsigned	mask;
	unsigned	compare;
	float       banEndTime; // 0 for permanent ban
	float       banTime;
} ipfilter_t;

typedef struct
{
	unsigned int userid;
	float	banEndTime;
	float	banTime;
} userfilter_t;

#define	MAX_IPFILTERS	    1024
#define	MAX_USERFILTERS	    1024

#endif // FILTER_H