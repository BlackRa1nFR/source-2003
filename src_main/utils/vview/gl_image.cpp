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
#include "tgaloader.h"
#include "qfiles.h"
#include "gl_image.h"
#include "gl_model.h"
#include "engine.h"
#include "cmdlib.h"
#include <string.h>

// locals
static image_t		gltextures[MAX_GLTEXTURES];
static int			numgltextures = 0;

typedef enum 
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
} imagetype_t;


glstate_t gl_state = {0};

int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

static image_t *LoadTexture( const char *name, int *width, int *height );
static void W_CleanupName( const char *in, char *out, int length );

static g_ExternalTextureId = TEXNUM_IMAGES+MAX_GLTEXTURES+1;
unsigned int TextureID_Alloc( void )
{
	return g_ExternalTextureId++;
}

void TextureID_Free( unsigned int textureId )
{
	// don't free for now
}


void TextureID_Bind( unsigned int textureId )
{
	GL_Bind( textureId );
}

#if 0
void GL_EnableMultitexture( qboolean enable )
{
	if ( !qglSelectTextureSGIS )
		return;

	if ( enable )
	{
		GL_SelectTexture( GL_TEXTURE1_SGIS );
		qglEnable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	else
	{
		GL_SelectTexture( GL_TEXTURE1_SGIS );
		qglDisable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	GL_SelectTexture( GL_TEXTURE0_SGIS );
	GL_TexEnv( GL_REPLACE );
}

void GL_SelectTexture( GLenum texture )
{
	int tmu;

	if ( !qglSelectTextureSGIS )
		return;

	if ( texture == GL_TEXTURE0_SGIS )
		tmu = 0;
	else
		tmu = 1;

	if ( tmu == gl_state.currenttmu )
		return;

	gl_state.currenttmu = tmu;

	if ( tmu == 0 )
		qglSelectTextureSGIS( GL_TEXTURE0_SGIS );
	else
		qglSelectTextureSGIS( GL_TEXTURE1_SGIS );
}

void GL_TexEnv( GLenum mode )
{
	static int lastmodes[2] = { -1, -1 };

	if ( mode != lastmodes[gl_state.currenttmu] )
	{
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		lastmodes[gl_state.currenttmu] = mode;
	}
}
#endif

void GL_Bind (int texnum)
{
#if 0
	extern	image_t	*draw_chars;

	if (gl_nobind->value && draw_chars)		// performance evaluation option
		texnum = draw_chars->texnum;
#endif

	if ( gl_state.currenttextures[gl_state.currenttmu] == texnum)
		return;
	gl_state.currenttextures[gl_state.currenttmu] = texnum;
	qglBindTexture (GL_TEXTURE_2D, texnum);
}

#if 0
void GL_MBind( GLenum target, int texnum )
{
	GL_SelectTexture( target );
	if ( target == GL_TEXTURE0_SGIS )
	{
		if ( gl_state.currenttextures[0] == texnum )
			return;
	}
	else
	{
		if ( gl_state.currenttextures[1] == texnum )
			return;
	}
	GL_Bind( texnum );
}
#endif

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for (i=0 ; i<outwidth ; i++)
	{
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for (i=0 ; i<outwidth ; i++)
	{
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++)
		{
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}

int		upload_width, upload_height;
qboolean uploaded_paletted = false;

qboolean GL_Upload32 (unsigned *data, int width, int height,  qboolean mipmap)
{
	int			samples;
	unsigned	scaled[512*512];
//	unsigned char paletted_texture[256*256];
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	int comp;

	uploaded_paletted = false;

	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;
#if 0
	if (gl_round_down->value && scaled_width > width && mipmap)
		scaled_width >>= 1;
	if (gl_round_down->value && scaled_height > height && mipmap)
		scaled_height >>= 1;
#endif

	// let people sample down the world textures for speed
	if (mipmap)
	{
		scaled_width >>= (int)gl_picmip->value;
		scaled_height >>= (int)gl_picmip->value;
	}

	// don't ever bother with >512 textures
	if (scaled_width > 512)
		scaled_width = 512;
	if (scaled_height > 512)
		scaled_height = 512;

	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;

	upload_width = scaled_width;
	upload_height = scaled_height;

	if (scaled_width * scaled_height > sizeof(scaled)/4)
		engine.Error("GL_Upload32: too big");

	// scan the texture for any non-255 alpha
	c = width*height;
	scan = ((byte *)data) + 3;
	samples = gl_solid_format;
	for (i=0 ; i<c ; i++, scan += 4)
	{
		if ( *scan != 255 )
		{
			samples = gl_alpha_format;
			break;
		}
	}

	if (samples == gl_solid_format)
	    comp = gl_tex_solid_format;
	else if (samples == gl_alpha_format)
	    comp = gl_tex_alpha_format;
	else {
	    engine.Con_Printf (PRINT_ALL,
			   "Unknown number of texture components %i\n",
			   samples);
	    comp = samples;
	}

#if 0
	if (mipmap)
		gluBuild2DMipmaps (GL_TEXTURE_2D, samples, width, height, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	else if (scaled_width == width && scaled_height == height)
		qglTexImage2D (GL_TEXTURE_2D, 0, comp, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	else
	{
		gluScaleImage (GL_RGBA, width, height, GL_UNSIGNED_BYTE, trans,
			scaled_width, scaled_height, GL_UNSIGNED_BYTE, scaled);
		qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	}
#else

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
// JAY: No paletted textures for now
#if 0
			if ( qglColorTableEXT && gl_ext_palettedtexture->value && samples == gl_solid_format )
			{
				uploaded_paletted = true;
				GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) data, scaled_width, scaled_height );
				qglTexImage2D( GL_TEXTURE_2D,
							  0,
							  GL_COLOR_INDEX8_EXT,
							  scaled_width,
							  scaled_height,
							  0,
							  GL_COLOR_INDEX,
							  GL_UNSIGNED_BYTE,
							  paletted_texture );
			}
			else
#endif
			{
				qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			}
			goto done;
		}
		memcpy (scaled, data, width*height*4);
	}
	else
		GL_ResampleTexture (data, width, height, scaled, scaled_width, scaled_height);

	// gamma correct here
//	GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap );
#if 0
	if ( qglColorTableEXT && gl_ext_palettedtexture->value && ( samples == gl_solid_format ) )
	{
		uploaded_paletted = true;
		GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) scaled, scaled_width, scaled_height );
		qglTexImage2D( GL_TEXTURE_2D,
					  0,
					  GL_COLOR_INDEX8_EXT,
					  scaled_width,
					  scaled_height,
					  0,
					  GL_COLOR_INDEX,
					  GL_UNSIGNED_BYTE,
					  paletted_texture );
	}
	else
#endif
	{
		qglTexImage2D( GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled );
	}

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
#if 0
			if ( qglColorTableEXT && gl_ext_palettedtexture->value && samples == gl_solid_format )
			{
				uploaded_paletted = true;
				GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) scaled, scaled_width, scaled_height );
				qglTexImage2D( GL_TEXTURE_2D,
							  miplevel,
							  GL_COLOR_INDEX8_EXT,
							  scaled_width,
							  scaled_height,
							  0,
							  GL_COLOR_INDEX,
							  GL_UNSIGNED_BYTE,
							  paletted_texture );
			}
			else
#endif
			{
				qglTexImage2D (GL_TEXTURE_2D, miplevel, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
			}
		}
	}
done: ;
#endif


	if (mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

	return (samples == gl_alpha_format);
}

image_t *GL_FindEmptyImage( const char *name, int width, int height )
{
	image_t		*image;
	int			i;

	// find a free image_t
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!image->gl_texturenum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			engine.Error( "MAX_GLTEXTURES" );
		numgltextures++;
	}
	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		engine.Error( "Draw_LoadPic: \"%s\" is too long", name);
	strcpy (image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->gl_texturenum = TEXNUM_IMAGES + (image - gltextures);
	GL_Bind(image->gl_texturenum);

	return image;
}
	
/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic( const char *name, byte *pic, int width, int height, imagetype_t type, int bits )
{
	image_t *image = GL_FindEmptyImage( name, width, height );
	GL_Upload32 ((unsigned *)pic, width, height, true/*(image->type != it_pic && image->type != it_sky)*/ );
	image->upload_width = upload_width;		// after power of 2 and scales
	image->upload_height = upload_height;

	return image;
}


/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t	*GL_FindImage( const char *name, imagetype_t type )
{
	image_t *image;

	int		i, len;
	int		width, height;

	if (!name)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");
	len = strlen(name);
	if (len<5)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);

	// look for it
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	return LoadTexture( name, &width, &height );
}


image_t *GL_EmptyTexture( void )
{
	static image_t *pEmpty = NULL;

	if ( !pEmpty )
	{
#define EMPTY_SIZE			16
		static byte checker[EMPTY_SIZE * EMPTY_SIZE * 4];
		byte *pImage = checker;
		for ( int i = 0; i < EMPTY_SIZE; i++ )
		{
			for ( int j = 0; j < EMPTY_SIZE; j++ )
			{
//				int solid = ((i&1) ^ (j&1));
				int solid = ((i&0xfff8) ^ (j&0xfff8));
				if ( solid )
				{
					pImage[0] = 255;
					pImage[1] = 40;
					pImage[2] = 255;
					pImage[3] = 0;
				}
				else
				{
					pImage[0] = 0;
					pImage[1] = 0;
					pImage[2] = 0;
					pImage[3] = 0;
				}
				pImage += 4;
			}
		}

		pEmpty = GL_LoadPic( "-*empty*-", checker, EMPTY_SIZE, EMPTY_SIZE, it_wall, 24 );
	}

	return pEmpty;
}


image_t	*GL_LoadImage( const char *name )
{
	return GL_FindImage( name, it_wall );
}

// ---------------------------------------------------
//
// textures.c
//
// ---------------------------------------------------
image_t *LoadTexture( const char *name, int *width, int *height )
{
	byte *pic;
	image_t *image = NULL;
	char tmp[1024];
	sprintf( tmp, "..\\materials\\%s.tga", name );
	COM_FixSlashes( tmp );

	ImageFormat format;
	float gamma;

	if ( TGALoader::GetInfo( tmp, width, height, &format, &gamma ) )
	{
		int imageSize = ImageLoader::GetMemRequired( *width, *height, IMAGE_FORMAT_RGB888, true );
		pic = (byte *)engine.MemAllocTemp( imageSize );
	
		if ( TGALoader::Load( pic, tmp, *width, *height, IMAGE_FORMAT_RGB888, gamma, true ) )
		{
			image = GL_FindEmptyImage( name, *width, *height );
			if ( image )
			{
				qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
				qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

				int tmpW = *width;
				int tmpH = *height;
				byte *pdata = pic;
				int mipLevel = 0;
				while ( 1 )
				{

					qglTexImage2D( GL_TEXTURE_2D, mipLevel, gl_solid_format, tmpW, tmpH, 0, GL_RGB, GL_UNSIGNED_BYTE, pdata );
					if ( tmpW == 1 && tmpH == 1 )
						break;
					pdata += ImageLoader::GetMemRequired( tmpW, tmpH, IMAGE_FORMAT_RGB888, false );

					mipLevel++;
					tmpW >>= 1;
					tmpH >>= 1;
					if ( tmpW < 1 )
						tmpW = 1;
					if ( tmpH < 1 )
						tmpH = 1;
				}

			}
		}
		engine.MemFreeTemp( pic );
	}

	return image;
}

