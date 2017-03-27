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

#ifndef GL_IMAGE_H
#define GL_IMAGE_H
#pragma once

typedef struct
{
	float inverse_intensity;
//	qboolean fullscreen;

	int     prev_mode;

	unsigned char *d_16to8table;

	int lightmap_textures;

	int	currenttextures[2];
	int currenttmu;

	float camera_separation;
	qboolean stereo_enabled;

	unsigned char originalRedGammaTable[256];
	unsigned char originalGreenGammaTable[256];
	unsigned char originalBlueGammaTable[256];
} glstate_t;

extern glstate_t gl_state;

typedef struct image_s image_t;

extern image_t		*GL_EmptyTexture( void );
extern image_t		*GL_LoadImage( const char *name );

extern void GL_Bind( int texnum );
extern unsigned int TextureID_Alloc( void );
extern void TextureID_Free( unsigned int textureId );
extern void TextureID_Bind( unsigned int textureId );

extern int		gl_solid_format;
extern int		gl_alpha_format;

extern int		gl_tex_solid_format;
extern int		gl_tex_alpha_format;

extern int		gl_filter_min;
extern int		gl_filter_max;


#endif // GL_IMAGE_H
