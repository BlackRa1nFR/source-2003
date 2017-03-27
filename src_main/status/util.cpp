// Util.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
==============================
TIME_SecondsToHMS

==============================
*/
void TIME_SecondsToHMS( double in, int *w, int *d, int *h, int *m, double *s )
{
	int days = 0, weeks = 0;
	int hours = 0, minutes, seconds;
	double secs;
	
	seconds = (int)(in);
	secs = in;
	minutes = seconds / 60;
	
	if ( minutes )
	{
		seconds -= (minutes * 60);
		secs -= ( minutes * 60 );
		hours = minutes / 60;
		if (hours)
		{
			minutes -= (hours * 60);
		}
	}

	if ( hours >= 24 )
	{
		days = hours / 24;
		hours -= days * 24;

		weeks = days / 7;
		days -= weeks * 7;
	}

	if ( w )
	{
		*w = weeks;
	}
	if ( d )
	{
		*d = days;
	}
	if ( h )
	{
		*h = hours;
	}
	if ( m )
	{
		*m = minutes;
	}
	if ( s ) 
	{
		*s = secs;
	}
}

/*
==============================
NET_SplitAddress

==============================
*/
void NET_SplitAddress(char *in, char *out, int *port, int defport)
{
	char *p;

	// Find the colon
	p = strstr(in, ":");
	if (p)
	{
		strncpy ( out, in, p - in );
		out[p - in] = '\0';

		p++;
		*port = atoi( p );
	}
	else
	{
		strcpy( out, in );
		*port = defport;
	}
}