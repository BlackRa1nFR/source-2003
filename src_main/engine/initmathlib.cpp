//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include <mathlib.h>
#include "convar.h" // ConVar define
#include "view.h"
#include "gl_cvars.h" // mat_overbright
#include "cmd.h" // Cmd_*
#include "console.h"  // Con_Printf


static bool s_bAllowMMX = true;
static bool s_bAllow3DNow = true;
static bool s_bAllowSSE = true;
static bool s_bAllowSSE2 = true;

void InitMathlib( void )
{
	MathLib_Init( v_gamma.GetFloat(), v_texgamma.GetFloat(), 
		v_brightness.GetFloat(), mat_overbright.GetInt(), s_bAllow3DNow, s_bAllowSSE, s_bAllowSSE2, s_bAllowMMX );
}

/*
===============
R_MMX
===============
*/
void R_MMX(void)
{
	if (Cmd_Argc() == 1)
	{
		s_bAllowMMX = true;
	}
	else
	{
		s_bAllowMMX  = atoi(Cmd_Argv(1)) ? true : false;
	}

	InitMathlib();
	Con_Printf( "MMX code is %s\n", MathLib_MMXEnabled() ? "enabled" : "disabled" );
}

/*
===============
R_SSE
===============
*/
void R_SSE(void)
{
	if (Cmd_Argc() == 1)
	{
		s_bAllowSSE = true;
	}
	else
	{
		s_bAllowSSE  = atoi(Cmd_Argv(1)) ? true : false;
	}

	InitMathlib();
	Con_Printf( "SSE code is %s\n", MathLib_SSEEnabled() ? "enabled" : "disabled" );
}

/*
===============
R_SSE2
===============
*/
void R_SSE2(void)
{
	if (Cmd_Argc() == 1)
	{
		s_bAllowSSE2 = true;
	}
	else
	{
		s_bAllowSSE2 = atoi(Cmd_Argv(1)) ? true : false;
	}

	InitMathlib();
	Con_Printf( "SSE2 code is %s\n", MathLib_SSE2Enabled() ? "enabled" : "disabled" );
}

/*
===============
R_3DNow
===============
*/
void R_3DNow(void)
{
	if (Cmd_Argc() == 1)
	{
		s_bAllow3DNow = true;
	}
	else
	{
		s_bAllow3DNow  = atoi(Cmd_Argv(1)) ? true : false;
	}

	InitMathlib();
	Con_Printf( "3DNow code is %s\n", MathLib_3DNowEnabled() ? "enabled" : "disabled" );
}

static ConCommand r_mmx("r_mmx", R_MMX );
static ConCommand r_sse("r_sse", R_SSE );
static ConCommand r_sse2("r_sse2", R_SSE2 );
static ConCommand r_3dnow("r_3dnow", R_3DNow);
