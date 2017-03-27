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

#include "stdafx.h"
#include "qfiles.h"
#include "mathlib.h"
#include "gl_model.h"
#include "gl_image.h"
#include "engine.h"
#include <string.h>

#include "gl_rsurf.h"


#define BLOCK_WIDTH		256
#define BLOCK_HEIGHT	256

#define GL_LIGHTMAP_FORMAT	GL_RGBA
#define LIGHTMAP_BYTES		4

static void		LM_InitBlock( void );
static void		LM_UploadBlock( qboolean dynamic );
static qboolean	LM_AllocBlock (int w, int h, int *x, int *y);

static Vector 	blocklights[ 34 * 34 ];

extern void R_SetCacheState( msurface_t *surf );
extern void R_BuildLightMap( msurface_t *surf, byte *dest, int stride );

cvar_t r_lightmap = {"r_lightmap", 0 };
cvar_t r_lightstyle = {"r_lightstyle", 0 };

typedef struct
{
	int internal_format;
	int	current_lightmap_texture;

	msurface_t	*lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
} gllightmapstate_t;

static gllightmapstate_t gl_lms;

void LM_AddSurface( msurface_t *pSurface )
{
	if ( pSurface->lightmaptexturenum < 0 )
		return;

	pSurface->lightmapchain = gl_lms.lightmap_surfaces[ pSurface->lightmaptexturenum ];
	gl_lms.lightmap_surfaces[ pSurface->lightmaptexturenum ] = pSurface;
}


void RenderLight( model_t *pModel, msurface_t *pSurface )
{
	int i, v;
	int *pEdge = pModel->surfedges + pSurface->firstedge;

	glBegin( GL_POLYGON );
	for ( i = 0; i < pSurface->numedges; i++ )
	{
		int edge = *pEdge++;
		float s, t;

		if ( edge < 0 )
		{
			v = 1;
			edge = -edge;
		}
		else
		{
			v = 0;
		}
		Vector& pVert = pModel->vertexes[ pModel->edges[edge].v[v] ].position;

		s = DotProduct( pVert.Base(), pSurface->texinfo->lightmapVecsLuxelsPerWorldUnits[0]) + 
			pSurface->texinfo->lightmapVecsLuxelsPerWorldUnits[0][3];
		s = s + pSurface->light_s - pSurface->lightmapMins[0] + 0.5;
		s *= 1.0f/BLOCK_WIDTH;

		t = DotProduct( pVert.Base(), pSurface->texinfo->lightmapVecsLuxelsPerWorldUnits[1]) + 
			pSurface->texinfo->lightmapVecsLuxelsPerWorldUnits[1][3];
		t = t + pSurface->light_t - pSurface->lightmapMins[1] + 0.5;
		t *= 1.0f/BLOCK_HEIGHT;

		glTexCoord2f( s, t );
		glVertex3f( pVert[0], pVert[1], pVert[2] );
	}
	glEnd();
}


void LM_DrawSurfaces( void )
{
	msurface_t *pSurface;
	int i;
	extern model_t *r_worldmodel;

	glEnable( GL_BLEND );
	glColor3ub( 255, 255, 255 );
	glBlendFunc( GL_DST_COLOR, GL_ZERO );
	for ( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		pSurface = gl_lms.lightmap_surfaces[i];
		gl_lms.lightmap_surfaces[i] = NULL;
		if ( pSurface )
		{
			GL_Bind( gl_state.lightmap_textures + i );
			while ( pSurface )
			{
				RenderLight( r_worldmodel, pSurface );
				pSurface = pSurface->lightmapchain;
			}
		}
	}
	glDisable( GL_BLEND );
}

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

static void LM_InitBlock( void )
{
	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );
}

static void LM_UploadBlock( qboolean dynamic )
{
	int texture;

	if ( dynamic )
	{
		texture = 0;
	}
	else
	{
		texture = gl_lms.current_lightmap_texture;
	}

	GL_Bind( gl_state.lightmap_textures + texture );
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if 0
	if ( dynamic )
	{
		int height = 0;
		int i;

		for ( i = 0; i < BLOCK_WIDTH; i++ )
		{
			if ( gl_lms.allocated[i] > height )
				height = gl_lms.allocated[i];
		}

		qglTexSubImage2D( GL_TEXTURE_2D, 
						  0,
						  0, 0,
						  BLOCK_WIDTH, height,
						  GL_LIGHTMAP_FORMAT,
						  GL_UNSIGNED_BYTE,
						  gl_lms.lightmap_buffer );
	}
	else
#endif
	{
		qglTexImage2D( GL_TEXTURE_2D, 
					   0, 
					   gl_lms.internal_format,
					   BLOCK_WIDTH, BLOCK_HEIGHT, 
					   0, 
					   GL_LIGHTMAP_FORMAT, 
					   GL_UNSIGNED_BYTE, 
					   gl_lms.lightmap_buffer );
		if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
		{
			engine.Error( "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
			gl_lms.current_lightmap_texture = 0;
		}
	}
	memset( gl_lms.lightmap_buffer, 255, 4*BLOCK_WIDTH*BLOCK_HEIGHT );
}

// returns a texture number and the position inside it
static qboolean LM_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = BLOCK_HEIGHT;

	for (i=0 ; i<BLOCK_WIDTH-w ; i++)
	{
		best2 = 0;

		for (j=0 ; j<w ; j++)
		{
			if (gl_lms.allocated[i+j] >= best)
				break;
			if (gl_lms.allocated[i+j] > best2)
				best2 = gl_lms.allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
		return false;

	for (i=0 ; i<w ; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}


//-----------------------------------------------------------------------------
// SURFACES
//-----------------------------------------------------------------------------

/*
==================
GL_BeginBuildingLightmaps

==================
*/
void GL_BeginBuildingLightmaps( model_t *m )
{
//	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
//	int				i;
	unsigned		dummy[BLOCK_WIDTH * BLOCK_HEIGHT];

	memset( gl_lms.allocated, 0, sizeof(gl_lms.allocated) );
	memset( gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces) );

#if 0
	r_framecount = 1;		// no dlightcache

	GL_EnableMultitexture( true );
	GL_SelectTexture( GL_TEXTURE1_SGIS );

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;
#endif

	if (!gl_state.lightmap_textures)
	{
		gl_state.lightmap_textures	= TEXNUM_LIGHTMAPS;
//		gl_state.lightmap_textures	= gl_state.texture_extension_number;
//		gl_state.texture_extension_number = gl_state.lightmap_textures + MAX_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 0;

	gl_lms.internal_format = gl_tex_solid_format;

	/*
	** initialize the dynamic lightmap texture
	*/
	GL_Bind( gl_state.lightmap_textures + 0 );
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage2D( GL_TEXTURE_2D, 
				   0, 
				   gl_lms.internal_format,
				   BLOCK_WIDTH, BLOCK_HEIGHT, 
				   0, 
				   GL_LIGHTMAP_FORMAT, 
				   GL_UNSIGNED_BYTE, 
				   dummy );
}

/*
=======================
GL_EndBuildingLightmaps
=======================
*/
void GL_EndBuildingLightmaps (void)
{
	LM_UploadBlock( false );
//	GL_EnableMultitexture( false );
}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap (msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (SURFDRAW_NOLIGHT))
	{
		surf->lightmaptexturenum = -1;
		return;
	}

	smax = surf->lightmapExtents[0] + 1;
	tmax = surf->lightmapExtents[1] + 1;

	if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
	{
		LM_UploadBlock( false );
		LM_InitBlock();
		if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
		{
			engine.Error( "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	base = gl_lms.lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
}


/*
** R_SetCacheState
*/
void R_SetCacheState( msurface_t *surf )
{
#if 0
	int maps;

	for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
		 maps++)
	{
		surf->cached_light[maps] = r_newrefdef.lightstyles[surf->styles[maps]].white;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Build the blocklights array for a given surface and copy to dest
//			Combine and scale multiple lightmaps into the 8.8 format in blocklights
// Input  : *psurf - surface to rebuild
//			*dest - texture pointer to receive copy in lightmap texture format
//			stride - stride of *dest memory
//-----------------------------------------------------------------------------
void R_BuildLightMap( msurface_t *psurf, byte *dest, int stride )
{
	int			smax, tmax;
	int			i, j, k, size;
	colorRGBExp32	*lightmap;
	Vector		*bl;
	float		scale;
	int			maps;

//	psurf->cached_dlight = (psurf->dlightbits & r_dlightactive);
//	psurf->dlightbits &= r_dlightactive;

	smax = psurf->lightmapExtents[0] + 1;
	tmax = psurf->lightmapExtents[1] + 1;
	size = smax*tmax;
	lightmap = psurf->samples;
	if ( smax > 34 || tmax > 34 )
	{
		engine.Error( "BAD SURFACE EXTENTS!\n" );
		return;
	}
// set to full bright if no light data
	if ( !lightmap )//r_fullbright.value == 1 || !cl.worldmodel->lightdata )
	{
		for (i=0 ; i<size ; i++)
		{
			blocklights[i][0] = 1.0;
			blocklights[i][1] = 1.0;
			blocklights[i][2] = 1.0;
		}
		goto store;
	}
// clear to no light
	for (i=0 ; i<size ; i++)
	{
		blocklights[i][0] = 0;
		blocklights[i][1] = 0;
		blocklights[i][2] = 0;
	}

	// add all the lightmaps
	if (lightmap)
	{
		if (0)//r_lightmap.value == -1 && r_lightstyle.value == -1)
		{
			for (maps = 0 ; maps < MAXLIGHTMAPS && psurf->styles[maps] != 255 ;
				 maps++)
			{
				scale = d_lightstylevalue[psurf->styles[maps]] * (1.0/264.0);
			
				psurf->cached_light[maps] = scale;	// 8.8 fraction
				for (i=0 ; i<size ; i++)
				{
					blocklights[i][0] += TexLightToLinear( lightmap[i].r, lightmap[i].exponent ) * scale;
					blocklights[i][1] += TexLightToLinear( lightmap[i].g, lightmap[i].exponent ) * scale;
					blocklights[i][2] += TexLightToLinear( lightmap[i].b, lightmap[i].exponent ) * scale;
				}
				lightmap += size;	// skip to next lightmap
			}
		}
		else 
		{
			scale = 1.0;
			for (maps = 0 ; maps < MAXLIGHTMAPS && psurf->styles[maps] != 255; maps++)
			{
				if (maps == r_lightmap.value || psurf->styles[maps] == r_lightstyle.value)
				{
					for (i=0 ; i<size ; i++)
					{
						blocklights[i][0] += TexLightToLinear( lightmap[i].r, lightmap[i].exponent ) * scale;
						blocklights[i][1] += TexLightToLinear( lightmap[i].g, lightmap[i].exponent ) * scale;
						blocklights[i][2] += TexLightToLinear( lightmap[i].b, lightmap[i].exponent ) * scale;
					}
				}
				lightmap += size;	// skip to next lightmap
			}
		}
	}

// add all the dynamic lights
#if 0
	if (psurf->dlightframe == r_framecount)
		R_AddDynamicLights (psurf);
#endif

store:
// bound, invert, and shift
	stride -= (smax<<2);
	bl = blocklights;
	for (i=0 ; i<tmax ; i++, dest += stride)
	{
		for (j=0 ; j<smax ; j++)
		{
			for(k=0;k<3;k++)
			{
				dest[k] = LinearToLightmap( ((float *)bl)[k] );
			}
	
			bl++;

			dest[3] = 255;
			dest += 4;
		}
	}
}
