//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Deals with all questions of color format under DX8
//=============================================================================

#include "locald3dtypes.h"
#include "ShaderAPIDX8_Global.h"
#include "ImageLoader.h"
#include "IShaderUtil.h"
#include "tier0/dbg.h"
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Figures out what texture formats we support
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------

// Texture formats supported by DX8 driver
static D3DFORMAT	g_D3DColorFormat[NUM_IMAGE_FORMATS];
static D3DFORMAT	g_D3DRenderTargetFormat[NUM_IMAGE_FORMATS];
static UINT			g_DisplayAdapter;
static D3DDEVTYPE	g_DeviceType;
static ImageFormat	g_DeviceFormat;
static bool			g_bSupportsD24S8;
static bool			g_bSupportsD24X8;
static bool			g_bSupportsD16;


//-----------------------------------------------------------------------------
// convert back and forth from D3D format to ImageFormat, regardless of
// whether it's supported or not
//-----------------------------------------------------------------------------

ImageFormat D3DFormatToImageFormat( D3DFORMAT format )
{
	switch(format)
	{
	case D3DFMT_R8G8B8:
		return IMAGE_FORMAT_BGR888;
	case D3DFMT_A8R8G8B8:
		return IMAGE_FORMAT_BGRA8888;
	case D3DFMT_X8R8G8B8:
		return IMAGE_FORMAT_BGRX8888;
	case D3DFMT_R5G6B5:
		return IMAGE_FORMAT_BGR565;
	case D3DFMT_X1R5G5B5:
		return IMAGE_FORMAT_BGRX5551;
	case D3DFMT_A1R5G5B5:
		return IMAGE_FORMAT_BGRA5551;
	case D3DFMT_A4R4G4B4:
		return IMAGE_FORMAT_BGRA4444;
	case D3DFMT_L8:
		return IMAGE_FORMAT_I8;
	case D3DFMT_A8L8:
		return IMAGE_FORMAT_IA88;
	case D3DFMT_P8:
		return IMAGE_FORMAT_P8;
	case D3DFMT_A8:
		return IMAGE_FORMAT_A8;
	case D3DFMT_DXT1:
		return IMAGE_FORMAT_DXT1;
	case D3DFMT_DXT3:
		return IMAGE_FORMAT_DXT3;
	case D3DFMT_DXT5:
		return IMAGE_FORMAT_DXT5;
	case D3DFMT_V8U8:
		return IMAGE_FORMAT_UV88;
	case D3DFMT_Q8W8V8U8:
		return IMAGE_FORMAT_UVWQ8888;
	case D3DFMT_A16B16G16R16F:
		return IMAGE_FORMAT_RGBA16161616F;
	// GR - HDR
	case D3DFMT_A16B16G16R16:
		return IMAGE_FORMAT_RGBA16161616;
	}

	Assert( 0 );
	// Bogus baby!
	return (ImageFormat)-1;
}

D3DFORMAT ImageFormatToD3DFormat( ImageFormat format )
{
	// This doesn't care whether it's supported or not
	switch(format)
	{
	case IMAGE_FORMAT_BGR888:
		return D3DFMT_R8G8B8;
	case IMAGE_FORMAT_BGRA8888:
		return D3DFMT_A8R8G8B8;
	case IMAGE_FORMAT_BGRX8888:
		return D3DFMT_X8R8G8B8;
	case IMAGE_FORMAT_BGR565:
		return D3DFMT_R5G6B5;
	case IMAGE_FORMAT_BGRX5551:
		return D3DFMT_X1R5G5B5;
	case IMAGE_FORMAT_BGRA5551:
		return D3DFMT_A1R5G5B5;
	case IMAGE_FORMAT_BGRA4444:
		return D3DFMT_A4R4G4B4;
	case IMAGE_FORMAT_I8:
		return D3DFMT_L8;
	case IMAGE_FORMAT_IA88:
		return D3DFMT_A8L8;
	case IMAGE_FORMAT_P8:
		return D3DFMT_P8;
	case IMAGE_FORMAT_A8:
		return D3DFMT_A8;
	case IMAGE_FORMAT_DXT1:
	case IMAGE_FORMAT_DXT1_ONEBITALPHA:
		return D3DFMT_DXT1;
	case IMAGE_FORMAT_DXT3:
		return D3DFMT_DXT3;
	case IMAGE_FORMAT_DXT5:
		return D3DFMT_DXT5;
	case IMAGE_FORMAT_UV88:
		return D3DFMT_V8U8;
	case IMAGE_FORMAT_UVWQ8888:
		return D3DFMT_Q8W8V8U8;
	case IMAGE_FORMAT_RGBA16161616F:
		return D3DFMT_A16B16G16R16F;
	// GR - HDR
	case IMAGE_FORMAT_RGBA16161616:
		return D3DFMT_A16B16G16R16;
	}

	Assert( 0 );
	return (D3DFORMAT)-1;
}



//-----------------------------------------------------------------------------
// Determines what formats we actually *do* support
//-----------------------------------------------------------------------------

static bool TestTextureFormat( D3DFORMAT format, bool isRenderTarget ) 
{
	// See if we can do it!
    HRESULT hr = D3D()->CheckDeviceFormat( 
		g_DisplayAdapter, g_DeviceType, ImageFormatToD3DFormat(g_DeviceFormat),
        (!isRenderTarget) ? 0 : D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, format);

    return SUCCEEDED( hr );
}

D3DFORMAT GetNearestD3DColorFormat( ImageFormat fmt, bool use16BitTextures, bool isRenderTarget )
{
	switch(fmt)
	{
	case IMAGE_FORMAT_RGBA8888:
	case IMAGE_FORMAT_ABGR8888:
	case IMAGE_FORMAT_ARGB8888:
	case IMAGE_FORMAT_BGRA8888:
		if (use16BitTextures && TestTextureFormat(D3DFMT_A4R4G4B4, isRenderTarget))
			return D3DFMT_A4R4G4B4;
		if (TestTextureFormat(D3DFMT_A8R8G8B8, isRenderTarget))
			return D3DFMT_A8R8G8B8;
		if (TestTextureFormat(D3DFMT_A4R4G4B4, isRenderTarget))
			return D3DFMT_A4R4G4B4;
		break;

	case IMAGE_FORMAT_BGRX8888:
		// We want this format to return exactly it's equivalent so that
		// when we create render targets to blit to from the framebuffer,
		// the CopyRect won't fail due to format mismatches.
		if (TestTextureFormat(D3DFMT_X8R8G8B8, isRenderTarget))
			return D3DFMT_X8R8G8B8;

		// fall through. . . .
	case IMAGE_FORMAT_RGB888:
	case IMAGE_FORMAT_BGR888:
		if (use16BitTextures && TestTextureFormat(D3DFMT_R5G6B5, isRenderTarget))
			return D3DFMT_R5G6B5;
		if (TestTextureFormat(D3DFMT_R8G8B8, isRenderTarget))
			return D3DFMT_R8G8B8;
		if (TestTextureFormat(D3DFMT_X8R8G8B8, isRenderTarget))
			return D3DFMT_X8R8G8B8;
		if (TestTextureFormat(D3DFMT_A8R8G8B8, isRenderTarget))
			return D3DFMT_A8R8G8B8;
		if (TestTextureFormat(D3DFMT_R5G6B5, isRenderTarget))
			return D3DFMT_R5G6B5;
		if (TestTextureFormat(D3DFMT_X1R5G5B5, isRenderTarget))
			return D3DFMT_X1R5G5B5;
		if (TestTextureFormat(D3DFMT_A1R5G5B5, isRenderTarget))
			return D3DFMT_A1R5G5B5;
		break;

	case IMAGE_FORMAT_BGR565:
	case IMAGE_FORMAT_RGB565:
		if (TestTextureFormat(D3DFMT_R5G6B5, isRenderTarget))
			return D3DFMT_R5G6B5;
		if (TestTextureFormat(D3DFMT_X1R5G5B5, isRenderTarget))
			return D3DFMT_X1R5G5B5;
		if (TestTextureFormat(D3DFMT_A1R5G5B5, isRenderTarget))
			return D3DFMT_A1R5G5B5;
		if (TestTextureFormat(D3DFMT_R8G8B8, isRenderTarget))
			return D3DFMT_R8G8B8;
		if (TestTextureFormat(D3DFMT_X8R8G8B8, isRenderTarget))
			return D3DFMT_X8R8G8B8;
		if (TestTextureFormat(D3DFMT_A8R8G8B8, isRenderTarget))
			return D3DFMT_A8R8G8B8;
		break;

	case IMAGE_FORMAT_BGRX5551:
		if (TestTextureFormat(D3DFMT_X1R5G5B5, isRenderTarget))
			return D3DFMT_X1R5G5B5;
		if (TestTextureFormat(D3DFMT_A1R5G5B5, isRenderTarget))
			return D3DFMT_A1R5G5B5;
		if (TestTextureFormat(D3DFMT_R5G6B5, isRenderTarget))
			return D3DFMT_R5G6B5;
		if (TestTextureFormat(D3DFMT_R8G8B8, isRenderTarget))
			return D3DFMT_R8G8B8;
		if (TestTextureFormat(D3DFMT_X8R8G8B8, isRenderTarget))
			return D3DFMT_X8R8G8B8;
		if (TestTextureFormat(D3DFMT_A8R8G8B8, isRenderTarget))
			return D3DFMT_A8R8G8B8;
		break;

	case IMAGE_FORMAT_BGRA5551:
		if (TestTextureFormat(D3DFMT_A1R5G5B5, isRenderTarget))
			return D3DFMT_A1R5G5B5;
		if (TestTextureFormat(D3DFMT_A4R4G4B4, isRenderTarget))
			return D3DFMT_A4R4G4B4;
		if (TestTextureFormat(D3DFMT_A8R8G8B8, isRenderTarget))
			return D3DFMT_A8R8G8B8;
		break;

	case IMAGE_FORMAT_BGRA4444:
		if (TestTextureFormat(D3DFMT_A4R4G4B4, isRenderTarget))
			return D3DFMT_A4R4G4B4;
		if (TestTextureFormat(D3DFMT_A8R8G8B8, isRenderTarget))
			return D3DFMT_A8R8G8B8;
		break;

	case IMAGE_FORMAT_I8:
		if (TestTextureFormat(D3DFMT_L8, isRenderTarget))
			return D3DFMT_L8;
		if (TestTextureFormat(D3DFMT_A8R8G8B8, isRenderTarget))
			return D3DFMT_A8R8G8B8;
		break;

	case IMAGE_FORMAT_IA88:
		if (TestTextureFormat(D3DFMT_A8L8, isRenderTarget))
			return D3DFMT_A8L8;
		if (TestTextureFormat(D3DFMT_A8R8G8B8, isRenderTarget))
			return D3DFMT_A8R8G8B8;
		break;

	case IMAGE_FORMAT_A8:
		if (TestTextureFormat(D3DFMT_A8, isRenderTarget))
			return D3DFMT_A8;
		break;

	case IMAGE_FORMAT_P8:
		if (TestTextureFormat(D3DFMT_P8, isRenderTarget))
			return D3DFMT_P8;
		break;

	case IMAGE_FORMAT_DXT1:
	case IMAGE_FORMAT_DXT1_ONEBITALPHA:
		if (TestTextureFormat(D3DFMT_DXT1, isRenderTarget))
			return D3DFMT_DXT1;
		break;

	case IMAGE_FORMAT_DXT3:
		if (TestTextureFormat(D3DFMT_DXT3, isRenderTarget))
			return D3DFMT_DXT3;
		break;

	case IMAGE_FORMAT_DXT5:
		if (TestTextureFormat(D3DFMT_DXT5, isRenderTarget))
			return D3DFMT_DXT5;
		break;

	case IMAGE_FORMAT_UV88:
		if (TestTextureFormat(D3DFMT_V8U8, isRenderTarget ))
			return D3DFMT_V8U8;
		break;

	case IMAGE_FORMAT_UVWQ8888:
		if (TestTextureFormat(D3DFMT_Q8W8V8U8, isRenderTarget ))
			return D3DFMT_Q8W8V8U8;
		break;

	case IMAGE_FORMAT_RGBA16161616F:
		if( TestTextureFormat( D3DFMT_A16B16G16R16F, isRenderTarget ) )
			return D3DFMT_A16B16G16R16F;
		break;
	// GR - HDR
	case IMAGE_FORMAT_RGBA16161616:
		if( TestTextureFormat( D3DFMT_A16B16G16R16, isRenderTarget ) )
			return D3DFMT_A16B16G16R16;
		break;
	}

	return (D3DFORMAT)-1;
}

bool Prefer16BitTextures()
{
	return (ShaderUtil()->ImageFormatInfo(g_DeviceFormat).m_NumBytes == 2);
}


void InitializeColorInformation( UINT displayAdapter, D3DDEVTYPE deviceType, 
								 ImageFormat displayFormat )
{
	g_DisplayAdapter = displayAdapter;
	g_DeviceType = deviceType;
	g_DeviceFormat = displayFormat;

	bool use16BitTextures = Prefer16BitTextures(); 

	int fmt = 0;
	while( fmt < NUM_IMAGE_FORMATS )
	{
		if( fmt == IMAGE_FORMAT_BGR565 )
		{
			Warning( "" );
		}
		// By default, we know nothing about it
		g_D3DColorFormat[fmt] = GetNearestD3DColorFormat( (ImageFormat)fmt, use16BitTextures, false );
		Assert( g_D3DColorFormat[fmt] != D3DFMT_UNKNOWN );
		g_D3DRenderTargetFormat[fmt] = GetNearestD3DColorFormat( (ImageFormat)fmt, use16BitTextures, true );
		Assert( g_D3DRenderTargetFormat[fmt] != D3DFMT_UNKNOWN );
		++fmt;
	}

	// Check the depth formats
    HRESULT hr = D3D()->CheckDeviceFormat( 
		g_DisplayAdapter, g_DeviceType, ImageFormatToD3DFormat(g_DeviceFormat),
        D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8);

	g_bSupportsD24S8 = !FAILED(hr);

    hr = D3D()->CheckDeviceFormat( 
		g_DisplayAdapter, g_DeviceType, ImageFormatToD3DFormat(g_DeviceFormat),
        D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X8);

	g_bSupportsD24X8 = !FAILED(hr);

    hr = D3D()->CheckDeviceFormat( 
		g_DisplayAdapter, g_DeviceType, ImageFormatToD3DFormat(g_DeviceFormat),
        D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D16);

	g_bSupportsD16 = !FAILED(hr);
}


//-----------------------------------------------------------------------------
// Returns true if compressed textures are supported
//-----------------------------------------------------------------------------
bool D3DSupportsCompressedTextures()
{
	return	(g_D3DColorFormat[IMAGE_FORMAT_DXT1] != -1) &&
			(g_D3DColorFormat[IMAGE_FORMAT_DXT3] != -1) &&
			(g_D3DColorFormat[IMAGE_FORMAT_DXT5] != -1);
}


//-----------------------------------------------------------------------------
// Returns closest supported format
//-----------------------------------------------------------------------------
ImageFormat FindNearestSupportedFormat( ImageFormat format )
{
	return D3DFormatToImageFormat(g_D3DColorFormat[ format ]);
}

ImageFormat FindNearestRenderTargetFormat( ImageFormat format )
{
	return D3DFormatToImageFormat(g_D3DRenderTargetFormat[ format ]);
}


//-----------------------------------------------------------------------------
// Returns true if compressed textures are supported
//-----------------------------------------------------------------------------
bool D3DSupportsDepthTexture(D3DFORMAT format)
{
	// See if we can do it!
    HRESULT hr = D3D()->CheckDeviceFormat( 
		g_DisplayAdapter, g_DeviceType, ImageFormatToD3DFormat(g_DeviceFormat),
        D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, format);

	return !FAILED(hr);
}


//-----------------------------------------------------------------------------
// Returns true if the depth format is compatible with the display
//-----------------------------------------------------------------------------
static inline bool IsDepthFormatCompatible(D3DFORMAT depthFormat, D3DFORMAT renderTargetFormat)
{
	// Verify that the depth format is compatible.
	HRESULT hr = D3D()->CheckDepthStencilMatch( 
		g_DisplayAdapter, g_DeviceType,
		ImageFormatToD3DFormat(g_DeviceFormat),
		renderTargetFormat,
		depthFormat);
	return !FAILED(hr);
}

//-----------------------------------------------------------------------------
// Finds the nearest supported depth buffer format
//-----------------------------------------------------------------------------
D3DFORMAT FindNearestSupportedDepthFormat(D3DFORMAT depthFormat, D3DFORMAT renderTargetFormat)
{
	// This is the default case, used for rendering to the main render target
	if ((int)renderTargetFormat == -1)
	{
		renderTargetFormat = ImageFormatToD3DFormat(g_DeviceFormat);
	}

	switch (depthFormat)
	{
	case D3DFMT_D24S8:
		if (g_bSupportsD24S8 && IsDepthFormatCompatible(D3DFMT_D24S8, renderTargetFormat))
			return D3DFMT_D24S8;

		if (g_bSupportsD24X8 && IsDepthFormatCompatible(D3DFMT_D24X8, renderTargetFormat))
			return D3DFMT_D24X8;

		if( g_bSupportsD16 && IsDepthFormatCompatible(D3DFMT_D16, renderTargetFormat))
			return D3DFMT_D16;

		break;

	case D3DFMT_D24X8:
		if (g_bSupportsD24X8 && IsDepthFormatCompatible(D3DFMT_D24X8, renderTargetFormat))
			return D3DFMT_D24X8;

		if (g_bSupportsD24S8 && IsDepthFormatCompatible(D3DFMT_D24S8, renderTargetFormat))
			return D3DFMT_D24S8;

		if( g_bSupportsD16 && IsDepthFormatCompatible(D3DFMT_D16, renderTargetFormat))
			return D3DFMT_D16;

		break;

	case D3DFMT_D16:
		if (g_bSupportsD16 && IsDepthFormatCompatible(D3DFMT_D16, renderTargetFormat))
			return D3DFMT_D16;

		if (g_bSupportsD24X8 && IsDepthFormatCompatible(D3DFMT_D24X8, renderTargetFormat))
			return D3DFMT_D24X8;

		if (g_bSupportsD24S8 && IsDepthFormatCompatible(D3DFMT_D24S8, renderTargetFormat))
			return D3DFMT_D24S8;

		break;
	}

	Assert(0);
	return D3DFMT_D16;
}


        