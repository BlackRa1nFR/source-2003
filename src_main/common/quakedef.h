//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
// Primary header for engine
#if !defined( QUAKEDEF_H )
#define QUAKEDEF_H
#ifdef _WIN32
#pragma once
#endif

// OPTIONAL DEFINES
//#define	PARANOID		// speed sapping error checking
#include "basetypes.h"
#include "commonmacros.h"

#ifdef USECRTMEMDEBUG
#include "crtmemdebug.h"
#endif

// C Standard Library Includes
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "qlimits.h"

//===========================================
// FIXME, remove more of these?
#include "cache.h"
#include "sysexternal.h"
#include "mathlib.h"
#include "progs.h"
#include "common.h"
#include "cvar.h"
#include "cmd.h"
#include "protocol.h"
#include "render.h"
#include "client.h"
#include "gl_model.h"
#include "conprint.h"

//=============================================================================
// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use
typedef struct engineparms_s
{
	char	*basedir;		// Base / Mod game directory
	int		argc;
	char	**argv;
	void	*membase;
	int		memsize;
} engineparms_t;
extern	engineparms_t host_parms;

//=============================================================================

//
// host
// FIXME, move all this crap somewhere elase
//

extern	ConVar		developer;
extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;
extern	int			host_framecount;	// incremented every frame, never reset
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset
extern qboolean		isDedicated;

void Host_Error (char *error, ...);
void Host_EndGame (char *message, ...);

// user message
#define MAX_USER_MSG_DATA 192

// build info
// day counter from 10/24/96
extern int build_number( void );

#endif // QUAKEDEF_H