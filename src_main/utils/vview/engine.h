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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef ENGINE_H
#define ENGINE_H
#pragma once

#define PRINT_ALL	0

typedef struct 
{
	void	(*Error)( const char *pError, ... );
	void	(*Con_Printf)( int printCode, const char *pFmt, ... );
	void	*(*MemAllocTemp)( size_t count );
	void	*(*MemAlloc)( size_t count );
	void	(*MemFreeTemp)( void *pMem );
	int		(*LoadFile)( const char *pFileName, void **pBuffer );
	void	(*FreeFile)( void *pBuffer );
} engine_interface_t;

extern engine_interface_t engine;

typedef struct
{
	const char *string;
	float		value;
} cvar_t;

extern cvar_t *r_novis, *gl_lockpvs, *gl_picmip;

extern void COM_FileBase( const char *in, char *out );
extern char *com_gamedir;
extern char *com_defaultgamedir;
extern int	registration_sequence;

// engine stuff
#define MAX_LIGHTMAPS		128
#define TEXNUM_LIGHTMAPS	100
#define TEXNUM_IMAGES		(TEXNUM_LIGHTMAPS + MAX_LIGHTMAPS)
#define MAX_GLTEXTURES		2000

extern "C" void COM_FixSlashes( char *pname );
#define COM_DefaultExtension(a,b) SetExtension(a,a,b)

// qgl
#define qglBindTexture	glBindTexture
#define qglTexImage2D	glTexImage2D
#define qglTexParameterf	glTexParameterf

extern int d_lightstylevalue[256];
extern float	TextureToLinear( int c );
extern int		LinearToTexture( float f );
extern float	TexLightToLinear( int c, int exponent );
extern float	LinearToVertexLight( float f );
extern int		LinearToLightmap( float f );
extern int		LinearToScreenGamma( float f );

#ifdef MATHLIB_H
int BoxOnPlaneSide (Vector& emins, Vector& emaxs, struct cplane_s *plane);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))
#endif	// MATHLIB_H


#endif // ENGINE_H
