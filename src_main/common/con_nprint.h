#if !defined( CON_NPRINTH )
#define CON_NPRINTH
#ifdef _WIN32
#pragma once
#endif

typedef struct con_nprint_s
{
	int		index;			// Row #
	float	time_to_live;	// # of seconds before it dissappears
	float	color[ 3 ];		// RGB colors ( 0.0 -> 1.0 scale )
	bool	fixed_width_font;
} con_nprint_t;

void Con_NPrintf( int idx, char *fmt, ... );
void Con_NXPrintf( struct con_nprint_s *info, char *fmt, ... );

#endif