//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
// This is where all rendertaget declaration and creation related stuff is...
//
//=============================================================================

#include "materialsystem_global.h"
#include "imageloader.h"
#include "matrendertexture.h"
#include "convar.h"

//=============================================================================
//
// List of named renderable textures:
//
//  FB post-processing (refraction, etc.)
//   _rt_PowerOfTwoFB
//   _rt_FullFrameFB
//
//  Water rendering
//   _rt_WaterReflection
//   _rt_WaterRefraction
//
//  HDR post-processing
//   _rt_SmallHDR0
//   _rt_SmallHDR1
//
//  Camera render target
//   _rt_Camera
//=============================================================================

void GetPowerOfTwoRenderTargetDimensionsSmallerOrEqualToFrameBuffer( int &width, int &height )
{
	int fbWidth, fbHeight;
	MaterialSystem()->GetBackBufferDimensions( fbWidth, fbHeight );
	while( width > fbWidth )
	{
		width >>= 1;
	}
	while( height > fbHeight )
	{
		height >>= 1;
	}
}

ITexture *CreatePowerOfTwoFBTexture( )
{
	int width = 512;
	int height = 512;
	GetPowerOfTwoRenderTargetDimensionsSmallerOrEqualToFrameBuffer( width, height );
	return MaterialSystem()->CreateNamedRenderTargetTexture( "_rt_PowerOfTwoFB",
		width, height, MaterialSystem()->GetBackBufferFormat(), false );
}

ITexture *CreateFullFrameFBTexture( )
{
	int width, height;
	MaterialSystem()->GetBackBufferDimensions( width, height );

	return MaterialSystem()->CreateNamedRenderTargetTexture("_rt_FullFrameFB",
		width, height, MaterialSystem()->GetBackBufferFormat(), false );
}

// hook into engine's mat_picmip convar
ConVar mat_picmip( "mat_picmip", "0" );

void GetWaterRenderTargetDimensions( int &width, int &height )
{
	int fbWidth, fbHeight;
	MaterialSystem()->GetBackBufferDimensions( fbWidth, fbHeight );
	width = 512;
	height = 512;
	int picmip = mat_picmip.GetInt();
	while( picmip )
	{
		width >>= 1;
		height >>= 1;
		picmip--;
	}

	while( width > fbWidth )
	{
		width >>= 1;
	}
	while( height > fbHeight )
	{
		height >>= 1;
	}
}

ITexture *CreateWaterReflectionTexture( )
{
	int width, height;
	GetWaterRenderTargetDimensions( width, height );
	return MaterialSystem()->CreateNamedRenderTargetTexture( "_rt_WaterReflection",
		width, height, MaterialSystem()->GetBackBufferFormat(), false );
}

ITexture *CreateWaterRefractionTexture( )
{
	int width, height;
	GetWaterRenderTargetDimensions( width, height );
	return MaterialSystem()->CreateNamedRenderTargetTexture( "_rt_WaterRefraction",
		width, height, MaterialSystem()->GetBackBufferFormat(), false );
}

ITexture *CreateCameraTexture( )
{
	int width = 256;
	int height = 256;
	GetPowerOfTwoRenderTargetDimensionsSmallerOrEqualToFrameBuffer( width, height );
	return MaterialSystem()->CreateNamedRenderTargetTexture( "_rt_Camera",
		width, height, MaterialSystem()->GetBackBufferFormat(), false, false );
}

ITexture *CreateSmallHDR0Texture( )
{
	int width, height;
	MaterialSystem()->GetBackBufferDimensions( width, height );

	return MaterialSystem()->CreateNamedRenderTargetTexture( "_rt_SmallHDR0",
		CLAMP_BLUR_IMAGE_WIDTH( width / 4 ), CLAMP_BLUR_IMAGE_HEIGHT( height / 4 ),
		IMAGE_FORMAT_RGBA16161616, false );
}

ITexture *CreateSmallHDR1Texture( )
{
	int width, height;
	MaterialSystem()->GetBackBufferDimensions( width, height );

	return MaterialSystem()->CreateNamedRenderTargetTexture( "_rt_SmallHDR1",
		CLAMP_BLUR_IMAGE_WIDTH( width / 4 ), CLAMP_BLUR_IMAGE_HEIGHT( height / 4 ),
		IMAGE_FORMAT_RGBA16161616, false );
}

RenderTextureDescriptor_t rtDescriptors[] = 
{
	{ "_rt_PowerOfTwoFB",		CreatePowerOfTwoFBTexture },
	{ "_rt_FullFrameFB",		CreateFullFrameFBTexture },
	{ "_rt_WaterReflection",	CreateWaterReflectionTexture },
	{ "_rt_WaterRefraction",	CreateWaterRefractionTexture },
	{ "_rt_Camera",			    CreateCameraTexture },
	{ "_rt_SmallHDR0",			CreateSmallHDR0Texture },
	{ "_rt_SmallHDR1",			CreateSmallHDR1Texture },
	{ "",						NULL}
};

