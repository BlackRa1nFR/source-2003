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
// The dx8 implementation of the texture allocation
//=============================================================================

#include "locald3dtypes.h"
#include "TextureDX8.h"
#include "ShaderAPIDX8_Global.h"
#include "ColorFormatDX8.h"
#include "IShaderUtil.h"
#include "materialsystem/IMaterialSystem.h"
#include "UtlVector.h"
#include "CMaterialSystemStats.h"
#include "recording.h"
#include "ShaderAPI.h"
#include "filesystem.h"
#include "locald3dtypes.h"
#include "tier0/memdbgon.h"

#ifdef _WIN32
#pragma warning (disable:4189 4701)
#endif


static int s_TextureCount = 0;

//-----------------------------------------------------------------------------
// Stats...
//-----------------------------------------------------------------------------

int TextureCount()
{
	return s_TextureCount;
}


static HRESULT GetLevelDesc( IDirect3DBaseTexture* pBaseTexture, UINT level, D3DSURFACE_DESC* pDesc )
{
	HRESULT hr;
	switch( pBaseTexture->GetType() )
	{
	case D3DRTYPE_TEXTURE:
		hr = ( ( IDirect3DTexture * )pBaseTexture )->GetLevelDesc( level, pDesc );
		break;
	case D3DRTYPE_CUBETEXTURE:
		hr = ( ( IDirect3DCubeTexture * )pBaseTexture )->GetLevelDesc( level, pDesc );
		break;
	default:
		return ( HRESULT )-1;
		break;
	}
	return hr;
}

static HRESULT GetSurfaceFromTexture( IDirect3DBaseTexture* pBaseTexture, UINT level, 
									  D3DCUBEMAP_FACES cubeFaceID, IDirect3DSurface** ppSurfLevel )
{
	HRESULT hr;

	switch( pBaseTexture->GetType() )
	{
	case D3DRTYPE_TEXTURE:
		hr = ( ( IDirect3DTexture * )pBaseTexture )->GetSurfaceLevel( level, ppSurfLevel );
		break;
	case D3DRTYPE_CUBETEXTURE:
		hr = ( ( IDirect3DCubeTexture * )pBaseTexture )->GetCubeMapSurface( cubeFaceID, level, ppSurfLevel );
		break;
	default:
		return ( HRESULT )-1;
		break;
	}
	return hr;
}

//-----------------------------------------------------------------------------
// Gets the image format of a texture
//-----------------------------------------------------------------------------

static ImageFormat GetImageFormat( IDirect3DBaseTexture* pTexture )
{
	D3DSURFACE_DESC desc;
	HRESULT hr = GetLevelDesc( pTexture, 0, &desc );
	
	if (!FAILED(hr))
	{
		return D3DFormatToImageFormat( desc.Format );
	}

	// Bogus baby!
	return (ImageFormat)-1;
}


//-----------------------------------------------------------------------------
// Allocates the D3DTexture
//-----------------------------------------------------------------------------
IDirect3DBaseTexture* CreateD3DTexture( int width, int height, 
		ImageFormat dstFormat, int numLevels, int creationFlags )
{
	bool isCubeMap = (creationFlags & TEXTURE_CREATE_CUBEMAP) != 0;
	bool isRenderTarget = (creationFlags & TEXTURE_CREATE_RENDERTARGET) != 0;
	bool managed = (creationFlags & TEXTURE_CREATE_MANAGED) != 0;
	bool isDepthBuffer = (creationFlags & TEXTURE_CREATE_DEPTHBUFFER) != 0;
	bool isDynamic = (creationFlags & TEXTURE_CREATE_DYNAMIC) != 0;
	bool bAutoMipMap = (creationFlags & TEXTURE_CREATE_AUTOMIPMAP) != 0;

	// NOTE: This function shouldn't be used for creating depth buffers!
	Assert( !isDepthBuffer );

	D3DFORMAT d3dFormat;

	// move this up a level?
	if (!isRenderTarget)
		d3dFormat = ImageFormatToD3DFormat( FindNearestSupportedFormat( dstFormat ) );
	else
		d3dFormat = ImageFormatToD3DFormat( FindNearestRenderTargetFormat( dstFormat) );
	if ( d3dFormat == -1 )
	{
		Warning("ShaderAPIDX8::CreateD3DTexture: Invalid color format!\n");
		return 0;
	}

	IDirect3DBaseTexture* pBaseTexture = NULL;
	IDirect3DTexture* pD3DTexture = NULL;
	IDirect3DCubeTexture* pD3DCubeTexture = NULL;
	HRESULT hr;
	DWORD usage = 0;
	if( isRenderTarget )
	{
		usage |= D3DUSAGE_RENDERTARGET;
	}
	if ( isDynamic )
	{
		usage |= D3DUSAGE_DYNAMIC;
	}
	if( bAutoMipMap )
	{
		usage |= D3DUSAGE_AUTOGENMIPMAP;
	}

	if( isCubeMap )
	{
		hr = D3DDevice()->CreateCubeTexture( 
			width, numLevels, usage, d3dFormat, managed ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT, 
			&pD3DCubeTexture,NULL 
			);
		pBaseTexture = pD3DCubeTexture;
	}
	else
	{
		hr = D3DDevice()->CreateTexture( width, height, numLevels, 
			usage, d3dFormat, managed ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT, &pD3DTexture,NULL 
			);
		pBaseTexture = pD3DTexture;
	}

    if ( FAILED(hr) )
	{
		switch( hr )
		{
		case D3DERR_INVALIDCALL:
			Warning( "ShaderAPIDX8::CreateD3DTexture: D3DERR_INVALIDCALL\n" );
			break;
		case D3DERR_OUTOFVIDEOMEMORY:
			// This conditional is here so that we don't complain when testing
			// how much video memory we have. . this is kinda gross.
			if (managed)
				Warning( "ShaderAPIDX8::CreateD3DTexture: D3DERR_OUTOFVIDEOMEMORY\n" );
			break;
		case E_OUTOFMEMORY:
			Warning( "ShaderAPIDX8::CreateD3DTexture: E_OUTOFMEMORY\n" );
			break;
		default:
			break;
		}
		return 0;
	}

	++s_TextureCount;

	return pBaseTexture;
}


//-----------------------------------------------------------------------------
// Texture destruction
//-----------------------------------------------------------------------------
void DestroyD3DTexture( IDirect3DBaseTexture* pTex )
{
	if (pTex)
	{
		int ref = pTex->Release();
		Assert( ref == 0 );
		--s_TextureCount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTex - 
// Output : int
//-----------------------------------------------------------------------------
int GetD3DTextureRefCount( IDirect3DBaseTexture *pTex )
{
	if ( !pTex )
		return 0;

	pTex->AddRef();
	int ref = pTex->Release();

	return ref;
}

//-----------------------------------------------------------------------------
// See version 13 for a function that converts a texture to a mipmap (ConvertToMipmap)
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Lock, unlock a texture...
//-----------------------------------------------------------------------------

static RECT s_LockedSrcRect;
static D3DLOCKED_RECT s_LockedRect;
#ifdef _DEBUG
static bool s_bInLock = false;
#endif

bool LockTexture( int bindId, int copy, IDirect3DBaseTexture* pTexture, int level, 
	D3DCUBEMAP_FACES cubeFaceID, int xOffset, int yOffset, int width, int height,
	CPixelWriter& writer )
{
	Assert( !s_bInLock );
	
	IDirect3DSurface* pSurf;
	HRESULT hr = GetSurfaceFromTexture( pTexture, level, cubeFaceID, &pSurf );
	if (FAILED(hr))
		return false;

	s_LockedSrcRect.left = xOffset;
	s_LockedSrcRect.right = xOffset + width;
	s_LockedSrcRect.top = yOffset;
	s_LockedSrcRect.bottom = yOffset + height;

	RECORD_COMMAND( DX8_LOCK_TEXTURE, 6 );
	RECORD_INT( bindId );
	RECORD_INT( copy );
	RECORD_INT( level );
	RECORD_INT( cubeFaceID );
	RECORD_STRUCT( &s_LockedSrcRect, sizeof(s_LockedSrcRect) );
	RECORD_INT( D3DLOCK_NOSYSLOCK );

	hr = pSurf->LockRect( &s_LockedRect, &s_LockedSrcRect, D3DLOCK_NOSYSLOCK );
	pSurf->Release();

	if (FAILED(hr))
		return false;

	writer.SetPixelMemory( GetImageFormat(pTexture), s_LockedRect.pBits, s_LockedRect.Pitch );

	MaterialSystemStats()->IncrementCountedStat( 
		MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_DOWNLOADED,
		ShaderUtil()->GetMemRequired( width, height, GetImageFormat(pTexture), false) );
	MaterialSystemStats()->IncrementCountedStat( 
		MATERIAL_SYSTEM_STATS_TEXTURE_TEXELS_DOWNLOADED,
		width * height );
	MaterialSystemStats()->IncrementCountedStat( 
		MATERIAL_SYSTEM_STATS_TEXTURE_UPLOADS, 1 );

#ifdef _DEBUG
	s_bInLock = true;
#endif
	return true;
}

void UnlockTexture( int bindId, int copy, IDirect3DBaseTexture* pTexture, int level, 
	D3DCUBEMAP_FACES cubeFaceID )
{
	Assert( s_bInLock );

	IDirect3DSurface* pSurf;
	HRESULT hr = GetSurfaceFromTexture( pTexture, level, cubeFaceID, &pSurf );
	if (FAILED(hr))
		return;

#ifdef RECORD_TEXTURES 
	int width = s_LockedSrcRect.right - s_LockedSrcRect.left;
	int height = s_LockedSrcRect.bottom - s_LockedSrcRect.top;
	int imageFormatSize = ImageLoader::SizeInBytes( GetImageFormat( pTexture ) );
	Assert( imageFormatSize != 0 );
	int validDataBytesPerRow = imageFormatSize * width;
	int storeSize = validDataBytesPerRow * height;
	static CUtlVector< unsigned char > tmpMem;
	if( tmpMem.Size() < storeSize )
	{
		tmpMem.AddMultipleToTail( storeSize - tmpMem.Size() );
	}
	unsigned char *pDst = tmpMem.Base();
	unsigned char *pSrc = ( unsigned char * )s_LockedRect.pBits;
	RECORD_COMMAND( DX8_SET_TEXTURE_DATA, 3 );
	RECORD_INT( validDataBytesPerRow );
	RECORD_INT( height );
	int i;
	for( i = 0; i < height; i++ )
	{
		memcpy( pDst, pSrc, validDataBytesPerRow );
		pDst += validDataBytesPerRow;
		pSrc += s_LockedRect.Pitch;
	}
	RECORD_STRUCT( tmpMem.Base(), storeSize );
#endif // RECORD_TEXTURES 
	
	RECORD_COMMAND( DX8_UNLOCK_TEXTURE, 4 );
	RECORD_INT( bindId );
	RECORD_INT( copy );
	RECORD_INT( level );
	RECORD_INT( cubeFaceID );

	hr = pSurf->UnlockRect(  );
	pSurf->Release();
#ifdef _DEBUG
	s_bInLock = false;
#endif
}

//-----------------------------------------------------------------------------
// Compute texture size based on compression
//-----------------------------------------------------------------------------

static inline int DetermineGreaterPowerOfTwo( int val )
{
	int num = 1;
	while (val > num)
	{
		num <<= 1;
	}

	return num;
}

inline int DeterminePowerOfTwo( int val )
{
	int pow = 0;
	while ((val & 0x1) == 0x0)
	{
		val >>= 1;
		++pow;
	}

	return pow;
}


//-----------------------------------------------------------------------------
// A faster blitter which works in many cases
//-----------------------------------------------------------------------------

// NOTE: IF YOU CHANGE THIS, CHANGE THE VERSION IN PLAYBACK.CPP!!!!
// OPTIMIZE??: could lock the texture directly instead of the surface in dx9.
static void FastBlitTextureBits( int bindId, int copy, IDirect3DBaseTexture* pDstTexture, int level, 
							 D3DCUBEMAP_FACES cubeFaceID,
	                         int xOffset, int yOffset, 
	                         int width, int height, ImageFormat srcFormat, int srcStrideInBytes,
	                         unsigned char *pSrcData, ImageFormat dstFormat )
{
	// Get the level of the texture we want to write into
	IDirect3DSurface* pTextureLevel;
	HRESULT hr = GetSurfaceFromTexture( pDstTexture, level, cubeFaceID, &pTextureLevel );
	if (!FAILED(hr))
	{
		RECT srcRect;
		D3DLOCKED_RECT lockedRect;
		srcRect.left = xOffset;
		srcRect.right = xOffset + width;
		srcRect.top = yOffset;
		srcRect.bottom = yOffset + height;

#ifndef RECORD_TEXTURES
		RECORD_COMMAND( DX8_LOCK_TEXTURE, 6 );
		RECORD_INT( bindId );
		RECORD_INT( copy );
		RECORD_INT( level );
		RECORD_INT( cubeFaceID );
		RECORD_STRUCT( &srcRect, sizeof(srcRect) );
		RECORD_INT( D3DLOCK_NOSYSLOCK );
#endif

		if( FAILED( pTextureLevel->LockRect( &lockedRect, &srcRect, D3DLOCK_NOSYSLOCK ) ) )
		{
			Warning( "CShaderAPIDX8::BlitTextureBits: couldn't lock texture rect\n" );
			pTextureLevel->Release();
			return;
		}

		// garymcthack : need to make a recording command for this.
		unsigned char *pImage = (unsigned char *)lockedRect.pBits;
		ShaderUtil()->ConvertImageFormat( (unsigned char *)pSrcData, srcFormat,
							pImage, dstFormat, width, height, srcStrideInBytes, lockedRect.Pitch );

#ifndef RECORD_TEXTURES
		RECORD_COMMAND( DX8_UNLOCK_TEXTURE, 4 );
		RECORD_INT( bindId );
		RECORD_INT( copy );
		RECORD_INT( level );
		RECORD_INT( cubeFaceID );
#endif

		if( FAILED( pTextureLevel->UnlockRect( ) ) ) 
		{
			Warning( "CShaderAPIDX8::BlitTextureBits: couldn't unlock texture rect\n" );
			pTextureLevel->Release();
			return;
		}
		
		MaterialSystemStats()->IncrementCountedStat( 
			MATERIAL_SYSTEM_STATS_TEXTURE_BYTES_DOWNLOADED,
			ShaderUtil()->GetMemRequired( width, height, dstFormat, false) );
		MaterialSystemStats()->IncrementCountedStat( 
			MATERIAL_SYSTEM_STATS_TEXTURE_TEXELS_DOWNLOADED,
			width * height );
		MaterialSystemStats()->IncrementCountedStat( 
			MATERIAL_SYSTEM_STATS_TEXTURE_UPLOADS, 1 );

		pTextureLevel->Release();
	}
}

//-----------------------------------------------------------------------------
// Blit in bits
//-----------------------------------------------------------------------------

// FIXME: How do I blit from D3DPOOL_SYSTEMMEM to D3DPOOL_MANAGED?  I used to use CopyRects for this.  UpdateSurface doesn't work because it can't blit to anything besides D3DPOOL_DEFAULT.
// We use this only in the case where we need to create a < 4x4 miplevel for a compressed texture.  We end up creating a 4x4 system memory texture, and blitting it into the proper miplevel.
// 6) LockRects should be used for copying between SYSTEMMEM and
// MANAGED.  For such a small copy, you'd avoid a significant 
// amount of overhead from the old CopyRects code.  Ideally, you 
// should just lock the bottom of MANAGED and generate your 
// sub-4x4 data there.
	
// NOTE: IF YOU CHANGE THIS, CHANGE THE VERSION IN PLAYBACK.CPP!!!!
static void BlitTextureBits( int bindId, int copy, IDirect3DBaseTexture* pDstTexture, int level, 
							 D3DCUBEMAP_FACES cubeFaceID,
	                         int xOffset, int yOffset, 
	                         int width, int height, ImageFormat srcFormat, int srcStrideInBytes,
	                         unsigned char *pSrcData, ImageFormat dstFormat )
{
#ifdef RECORD_TEXTURES
	RECORD_COMMAND( DX8_BLIT_TEXTURE_BITS, 13 );
	RECORD_INT( bindId );
	RECORD_INT( copy );
	RECORD_INT( level );
	RECORD_INT( cubeFaceID );
	RECORD_INT( xOffset );
	RECORD_INT( yOffset );
	RECORD_INT( width );
	RECORD_INT( height );
	RECORD_INT( srcFormat );
	RECORD_INT( srcStrideInBytes );
	RECORD_INT( dstFormat );
	// strides are in bytes.
	int srcDataSize;
	if( srcStrideInBytes == 0 )
	{
		srcDataSize = ImageLoader::GetMemRequired( width, height, srcFormat, false );
	}
	else
	{
		srcDataSize = srcStrideInBytes * height;
	}
	RECORD_INT( srcDataSize );
	RECORD_STRUCT( pSrcData, srcDataSize );
#endif // RECORD_TEXTURES
	
	// WOO HOO!!!!  In dx9, we don't have to do this little dance to get the smaller mip levels
	// into the texture.
	FastBlitTextureBits( bindId, copy, pDstTexture, level, cubeFaceID, xOffset, yOffset,
		width, height, srcFormat, srcStrideInBytes, pSrcData, dstFormat );
}


//-----------------------------------------------------------------------------
// Texture image upload
//-----------------------------------------------------------------------------
void LoadTexture( int bindId, int copy, IDirect3DBaseTexture*& pTexture, 
				  int level, D3DCUBEMAP_FACES cubeFaceID, ImageFormat dstFormat, 
				  int width, int height, ImageFormat srcFormat, void *pSrcData )
{
	Assert( pSrcData );

	// Get the format we should actually use
	dstFormat = FindNearestSupportedFormat(dstFormat);

	// Allocate the texture if we haven't yet.
	if (!pTexture)
	{
		Assert( copy == 0 );

		int numLevels = (level == 0) ? 1 : 0;

		RECORD_COMMAND( DX8_CREATE_TEXTURE, 7 );
		RECORD_INT( bindId );
		RECORD_INT( width );
		RECORD_INT( height );
		RECORD_INT( ImageFormatToD3DFormat(dstFormat) );
		RECORD_INT( numLevels );
		RECORD_INT( false );
		RECORD_INT( 1 );

		// FIXME: Number of levels in the mipmap? Right now it makes em all.
		pTexture = CreateD3DTexture( width, height, dstFormat, numLevels, TEXTURE_CREATE_MANAGED );

		// FIXME: Error handling?
		if (!pTexture)
			return;
	}
	else
	{
		// Determine the image format of the texture....
		ImageFormat format = GetImageFormat( pTexture );
		Assert (format == dstFormat);
	}

	// Copy in the bits...
	BlitTextureBits( bindId, copy, pTexture, level, cubeFaceID, 0, 0, width, height, srcFormat, 0,
					(unsigned char*)pSrcData, dstFormat );
}

//-----------------------------------------------------------------------------
// Upload to a sub-piece of a texture
//-----------------------------------------------------------------------------

void LoadSubTexture( int bindId, int copy, IDirect3DBaseTexture* pTexture, 
					 int level, D3DCUBEMAP_FACES cubeFaceID, int xOffset, int yOffset, 
	                 int width, int height, ImageFormat srcFormat, int srcStrideInBytes, void *pSrcData )
{
	Assert( pSrcData );

	// Can't write to the texture if we haven't allocated one yet.
	if (!pTexture)
		return;

	// Determine the image format of the texture....
	ImageFormat dstFormat = GetImageFormat( pTexture );
	if (dstFormat == -1)
		return;

	// Copy in the bits...
	BlitTextureBits( bindId, copy, pTexture, level, cubeFaceID, xOffset, yOffset, width, height, srcFormat, 
					srcStrideInBytes, (unsigned char*)pSrcData, dstFormat );
}


//-----------------------------------------------------------------------------
// Returns the size of texture memory, in MB
//-----------------------------------------------------------------------------

#ifdef _DEBUG
//#define DONT_CHECK_MEM
#endif


int ComputeTextureMemorySize( const GUID &nDeviceGUID, D3DDEVTYPE deviceType )
{
	FileHandle_t file = FileSystem()->Open( "vidcfg.bin", "rb", "EXECUTABLE_PATH" );
	if ( file )
	{
		GUID deviceId;
		int texSize;
		FileSystem()->Read( &deviceId, sizeof(deviceId), file );
		FileSystem()->Read( &texSize, sizeof(texSize), file );
		FileSystem()->Close( file );
		if ( nDeviceGUID == deviceId )
		{
			return texSize;
		}
	}
	// How much texture memory?
	if (deviceType == D3DDEVTYPE_REF)
		return 64 * 1024 * 1024;


	// Sadly, the only way to compute texture memory size
	// is to allocate a crapload of textures until we can't any more
	ImageFormat fmt = FindNearestSupportedFormat( IMAGE_FORMAT_BGR565 );
	int textureSize = ShaderUtil()->GetMemRequired( 256, 256, fmt, false );

	int totalSize = 0;
	CUtlVector< IDirect3DBaseTexture* > textures;

#ifndef DONT_CHECK_MEM
	while (true)
	{
		RECORD_COMMAND( DX8_CREATE_TEXTURE, 7 );
		RECORD_INT( textures.Count() );
		RECORD_INT( 256 );
		RECORD_INT( 256 );
		RECORD_INT( ImageFormatToD3DFormat(fmt) );
		RECORD_INT( 1 );
		RECORD_INT( false );
		RECORD_INT( 1 );

		IDirect3DBaseTexture* pTex = CreateD3DTexture( 256, 256, fmt, 1, 0 );
		if (!pTex)
			break;
		totalSize += textureSize;

		textures.AddToTail( pTex );
	} 

	// Free all the temp textures
	for (int i = textures.Size(); --i >= 0; )
	{
		RECORD_COMMAND( DX8_DESTROY_TEXTURE, 1 );
		RECORD_INT( i );

		DestroyD3DTexture( textures[i] );
	}
#else
	totalSize = 102236160;
#endif
	file = FileSystem()->Open( "vidcfg.bin", "wb", "EXECUTABLE_PATH" );
	if ( file )
	{
		FileSystem()->Write( &nDeviceGUID, sizeof(GUID), file );
		FileSystem()->Write( &totalSize, sizeof(totalSize), file );
		FileSystem()->Close( file );
	}

	return totalSize;
}



