//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "cmdlib.h"
#include "mathlib.h"
#include "cmodel.h"
#include "tier0/dbg.h"

extern "C" int CLoadFile( const char *pFileName, void **pBuffer )
{
	return LoadFile( (char *)pFileName, pBuffer );
}

void Con_Printf( int printCode, const char *pFmt, ... )
{
    va_list argptr; 
    
    va_start( argptr, pFmt );
    vprintf( pFmt, argptr );
    va_end( argptr );
}

engine_interface_t engine = 
{
	Error,
	Con_Printf,
	malloc,
	malloc,
	free,
	CLoadFile,
	free,
};

cvar_t temp1, temp2, temp3;
cvar_t *r_novis = &temp1, *gl_lockpvs = &temp2, *gl_picmip = &temp3;

int	registration_sequence = 0;

// This is the base engine + mod-specific game dir (e.g. "d:/tf2/mytfmod/")
extern "C" char		gamedir[1024];	

// This is the base engine + base game directory (e.g. "e:/hl2/hl2/", or "d:/tf2/tf2/" )
extern "C" char		basegamedir[1024];

char *com_gamedir = gamedir;
char *com_defaultgamedir = basegamedir;


void COM_FileBase( const char *in, char *out )
{
	ExtractFileBase( (char *)in, out );
}


void BuildGammaTable (float g);
int		d_lightstylevalue[256];	// 8.8 fraction of base light value
void EngineInit( void )
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
		d_lightstylevalue[i] = 264;		// normal light value

	BuildGammaTable( 2.4 );
}

// build a lightmap texture to combine with surface texture, adjust for src*dst+dst*src, ramp reprogramming, etc
byte		texgammatable[256];	// palette is sent through this to convert to screen gamma
float		texlighttolinear[256];	// texlight (0..255) to linear (0..4)
float		texturetolinear[256];	// texture (0..255) to linear (0..1)
int			lineartotexture[1024];	// linear (0..1) to texture (0..255)
int			lineartoscreen[1024];	// linear (0..1) to gamma corrected vertex light (0..255)
float		power2_n[256];			// 2**(index - 128)

float		lineartovertex[4092];	// linear (0..4) to screen corrected vertex space (0..1?)
int			lineartolightmap[4092];	// linear (0..4) to screen corrected texture value (0..255)
float		v_blend[4];		// rgba 0.0 - 1.0

void BuildGammaTable (float g)
{
	int		i, inf;
	float	g1, g3;
	cvar_t v_brightness, gl_overbright, v_texgamma, v_gamma;

	v_brightness.value = 0;
	gl_overbright.value = 0;
	v_texgamma.value = 2.2;
	v_gamma.value = 2.4;

	if (g < 1.8)
		g = 1.8;

	if (g > 3.0) 
		g = 3.0;

	g = 1.0 / g;
//	g1 = v_texgamma.value * g; 
	g1 = g; 

	if (v_brightness.value <= 0.0) 
	{
		g3 = 0.125;
	}
	else if (v_brightness.value > 1.0) 
	{
		g3 = 0.05;
	}
	else 
	{
		g3 = 0.125 - (v_brightness.value * v_brightness.value) * 0.075;
	}

	for (i=0 ; i<256 ; i++)
	{
		inf = 255 * pow ( i/255.0, g1 ); 
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		texgammatable[i] = inf;
	}

	for( i = 0; i < 256; i++ )
	{
		power2_n[i] = pow( 2.0f, i - 128 );
	}

	for (i=0 ; i<1024 ; i++)
	{
		float f;

		f = i / 1023.0;

		// scale up
		if (v_brightness.value > 1.0)
			f = f * v_brightness.value;

		// shift up
		if (f <= g3)
			f = (f / g3) * 0.125;
		else 
			f = 0.125 + ((f - g3) / (1.0 - g3)) * 0.875;

		// convert linear space to desired gamma space
		inf = 255 * pow ( f, g ); 

		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		lineartoscreen[i] = inf;
	}

	/*
	for (i=0 ; i<1024 ; i++)
	{
		// convert from screen gamma space to linear space
		lineargammatable[i] = 1023 * pow ( i/1023.0, v_gamma.value );
		// convert from linear gamma space to screen space
		screengammatable[i] = 1023 * pow ( i/1023.0, 1.0 / v_gamma.value );
	}
	*/

	for (i=0 ; i<256 ; i++)
	{
		// convert from nonlinear texture space (0..255) to linear space (0..1)
		texturetolinear[i] =  pow( i / 255.0, v_texgamma.value );
	}

	for (i=0 ; i<1024 ; i++)
	{
		// convert from linear space (0..1) to nonlinear texture space (0..255)
		lineartotexture[i] =  pow( i / 1023.0, 1.0 / v_texgamma.value ) * 255;
	}

	for (i=0 ; i<256 ; i++)
	{
		float f;

		// convert from nonlinear lightmap space (0..255) to linear space (0..4)
		f =  (i / 255.0) * sqrt( 4 );
		f = f * f;

		texlighttolinear[i] = f;
	}

	{
		float f, overbrightFactor;

		// Can't do overbright without texcombine
		// UNDONE: Add GAMMA ramp to rectify this
#if 0
		if ( !gl_texcombine )
		{
			Cvar_Set( "gl_overbright", "0" );
		}
#endif
		if ( gl_overbright.value == 2.0 )
		{
			overbrightFactor = 0.5;
		}
		else if ( gl_overbright.value == 4.0 )
		{
			overbrightFactor = 0.25;
		}
		else
		{
			overbrightFactor = 1.0;
		}

		for (i=0 ; i<4092 ; i++)
		{
			// convert from linear 0..4 (x1024) to screen corrected vertex space (0..1?)
			f = pow ( i/1024.0, 1.0 / v_gamma.value );

			lineartovertex[i] = f * overbrightFactor;
			if (lineartovertex[i] > 1)
				lineartovertex[i] = 1;

			lineartolightmap[i] = f * 255 * overbrightFactor;
			if (lineartolightmap[i] > 255)
				lineartolightmap[i] = 255;
		}
	}
}


// convert texture to linear 0..1 value
float TextureToLinear( int c )
{
	if (c < 0)
		return 0;
	if (c > 255)
		return 1.0;

	return texturetolinear[c];
}

// convert texture to linear 0..1 value
int LinearToTexture( float f )
{
	int i;
	i = f * 1023;	// assume 0..1 range
	if (i < 0)
		i = 0;
	if (i > 1023)
		i = 1023;

	return lineartotexture[i];
}

float TexLightToLinear( int c, int exponent )
{
	// optimize me
//	return ( float )c * ( float )pow( 2.0f, exponent ) * ( 1.0f / 255.0f );
	return ( float )c * power2_n[exponent+128] * ( 1.0f / 255.0f );
}


float LinearToVertexLight( float f )
{
	int i;
	i = f * 1024;	// assume 0..4 range
	if (i < 0)
		i = 0;
	if (i > 4091)
		i = 4091;

	return lineartovertex[i];
}

int LinearToLightmap( float f )
{
	int i;
	i = f * 1024;	// assume 0..4 range
	if (i < 0)
		i = 0;
	if (i > 4091)
		i = 4091;

	return lineartolightmap[i];
}

// converts 0..1 linear value to screen gamma (0..255)
int LinearToScreenGamma( float f )
{
	int i;
	i = f * 1023;	// assume 0..1 range
	if (i < 0)
		i = 0;
	if (i > 1023)
		i = 1023;

	return lineartoscreen[i];
}

#if 0
// this is the slow, general version
int BoxOnPlaneSide2 (Vector& emins, Vector& emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	Vector	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}
#endif

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
#if !id386 || defined __linux__ 
int BoxOnPlaneSide (Vector& emins, Vector& emaxs, struct cplane_s *p)
{
	float	dist1, dist2;
	int		sides;

// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}
	
// general case
	switch (p->signbits)
	{
	case 0:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		assert( 0 );
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	assert( sides != 0 );

	return sides;
}
#else
#pragma warning( disable: 4035 )

__declspec( naked ) int BoxOnPlaneSide (Vector& emins, Vector& emaxs, struct cplane_s *p)
{
	static int bops_initialized;
	static int Ljmptab[8];

	__asm {

		push ebx
			
		cmp bops_initialized, 1
		je  initialized
		mov bops_initialized, 1
		
		mov Ljmptab[0*4], offset Lcase0
		mov Ljmptab[1*4], offset Lcase1
		mov Ljmptab[2*4], offset Lcase2
		mov Ljmptab[3*4], offset Lcase3
		mov Ljmptab[4*4], offset Lcase4
		mov Ljmptab[5*4], offset Lcase5
		mov Ljmptab[6*4], offset Lcase6
		mov Ljmptab[7*4], offset Lcase7
			
initialized:

		mov edx,ds:dword ptr[4+12+esp]
		mov ecx,ds:dword ptr[4+4+esp]
		xor eax,eax
		mov ebx,ds:dword ptr[4+8+esp]
		mov al,ds:byte ptr[17+edx]
		cmp al,8
		jge Lerror
		fld ds:dword ptr[0+edx]
		fld st(0)
		jmp dword ptr[Ljmptab+eax*4]
Lcase0:
		fmul ds:dword ptr[ebx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ebx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase1:
		fmul ds:dword ptr[ecx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ebx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase2:
		fmul ds:dword ptr[ebx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ecx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase3:
		fmul ds:dword ptr[ecx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ecx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase4:
		fmul ds:dword ptr[ebx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ebx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase5:
		fmul ds:dword ptr[ecx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ebx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase6:
		fmul ds:dword ptr[ebx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ecx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase7:
		fmul ds:dword ptr[ecx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ecx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
LSetSides:
		faddp st(2),st(0)
		fcomp ds:dword ptr[12+edx]
		xor ecx,ecx
		fnstsw ax
		fcomp ds:dword ptr[12+edx]
		and ah,1
		xor ah,1
		add cl,ah
		fnstsw ax
		and ah,1
		add ah,ah
		add cl,ah
		pop ebx
		mov eax,ecx
		ret
Lerror:
		int 3
	}
}
#pragma warning( default: 4035 )
#endif
