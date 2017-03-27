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
// Texture implementation
//=============================================================================

#include "materialsystem_global.h"
#include "shaderapi.h"
#include "itextureinternal.h"
#include "utlsymbol.h"
#include "time.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "imageloader.h"
#include "tgaloader.h"
#include "tgawriter.h"
#ifdef _WIN32
#include "direct.h"
#endif
#include "colorspace.h"
#include "string.h"
#include <malloc.h>
#include <stdlib.h>
#include "utlmemory.h"
#include "IHardwareConfigInternal.h"
#include "filesystem.h"
#include "vstdlib/strtools.h"
#include "vtf/vtf.h"
#include "materialsystem/materialsystem_config.h"
#include "mempool.h"
#include "texturemanager.h"
#include "utlbuffer.h"
#include "pixelwriter.h"
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Internal texture flags
//-----------------------------------------------------------------------------
enum InternalTextureFlags
{
	TEXTUREFLAGS_ERROR = (TEXTUREFLAGS_LASTFLAG << 1),
	TEXTUREFLAGS_ALLOCATED = (TEXTUREFLAGS_LASTFLAG << 2),
};


//-----------------------------------------------------------------------------
// Use Warning to show texture flags.
//-----------------------------------------------------------------------------
static void PrintFlags( unsigned int flags )
{
	if( flags & TEXTUREFLAGS_NOMIP )
	{
		Warning( "TEXTUREFLAGS_NOMIP|" );
	}
	if( flags & TEXTUREFLAGS_NOLOD )
	{
		Warning( "TEXTUREFLAGS_NOLOD|" );
	}
	if( flags & TEXTUREFLAGS_NOCOMPRESS )
	{
		Warning( "TEXTUREFLAGS_NOCOMPRESS|" );
	}
	if( flags & TEXTUREFLAGS_POINTSAMPLE )
	{
		Warning( "TEXTUREFLAGS_POINTSAMPLE|" );
	}
	if( flags & TEXTUREFLAGS_TRILINEAR )
	{
		Warning( "TEXTUREFLAGS_TRILINEAR|" );
	}
	if( flags & TEXTUREFLAGS_CLAMPS )
	{
		Warning( "TEXTUREFLAGS_CLAMPS|" );
	}
	if( flags & TEXTUREFLAGS_CLAMPT )
	{
		Warning( "TEXTUREFLAGS_CLAMPT|" );
	}
	if( flags & TEXTUREFLAGS_HINT_DXT5 )
	{
		Warning( "TEXTUREFLAGS_HINT_DXT5|" );
	}
	if( flags & TEXTUREFLAGS_ANISOTROPIC )
	{
		Warning( "TEXTUREFLAGS_ANISOTROPIC|" );
	}
	if( flags & TEXTUREFLAGS_PROCEDURAL )
	{
		Warning( "TEXTUREFLAGS_PROCEDURAL|" );
	}
	if( flags & TEXTUREFLAGS_MINMIP )
	{
		Warning( "TEXTUREFLAGS_MINMIP|" );
	}
	if( flags & TEXTUREFLAGS_SINGLECOPY )
	{
		Warning( "TEXTUREFLAGS_SINGLECOPY|" );
	}
}


//-----------------------------------------------------------------------------
// Base texture class
//-----------------------------------------------------------------------------

class CTexture : public ITextureInternal
{
public:
	CTexture();
	virtual ~CTexture();

	virtual const char *GetName( void ) const;

	// Stats about the texture itself
	virtual ImageFormat GetImageFormat() const;
	virtual int GetMappingWidth() const;
	virtual int GetMappingHeight() const;
	virtual int GetActualWidth() const;
	virtual int GetActualHeight() const;
	virtual int GetNumAnimationFrames() const;
	virtual bool IsTranslucent() const;
	virtual void GetReflectivity( Vector& reflectivity );

	// Reference counting
	virtual void IncrementReferenceCount( );
	virtual void DecrementReferenceCount( );
	virtual int GetReferenceCount( );

	// Used to modify the texture bits (procedural textures only)
	virtual void SetTextureRegenerator( ITextureRegenerator *pTextureRegen );

	// Little helper polling methods
	virtual bool IsNormalMap( ) const;
	virtual bool IsCubeMap( void ) const;
	virtual bool IsRenderTarget( ) const;
	virtual bool IsProcedural() const;
	virtual bool IsMipmapped() const;
	virtual bool IsError() const;

	// Various ways of initializing the texture
	void InitRenderTarget( int w, int h, ImageFormat fmt, bool depth );
	void InitFileTexture( const char *pTextureFile );
	void InitProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags );

	// Releases the texture's hw memory
	void Release();

	// Sets the filtering modes on the texture we're modifying
	void SetFilteringAndClampingMode( );
	void Download( Rect_t *pRect = NULL );

	// Used by tools.... loads up the non-fallback information about the texture 
	virtual void Precache();

	// FIXME: Bogus methods... can we please delete these?
	virtual void GetLowResColorSample( float s, float t, float *color ) const;

	virtual int GetApproximateVidMemBytes( void ) const;

	// Stretch blit the framebuffer into this texture.
	virtual void CopyFrameBufferToMe( void );

protected:
	void ReconstructTexture();
	void ReconstructPartialTexture( Rect_t *pRect );
	bool HasBeenAllocated() const;

	// Initializes/shuts down the texture
	void Init( int w, int h, ImageFormat fmt, int iFlags, int iFrameCount );
	void Shutdown();

	// Sets the texture name
	void SetName( const char* pName );

	// Assigns/releases texture IDs for our animation frames
	void AssignTextureIDs();
	void ReleaseTextureIDs();

	// Calculates info about whether we can make the texture smaller and by how much
	// Returns the number of skipped mip levels
	int ComputeActualSize( bool bIgnorePicmip = false );

	// Computes the actual format of the texture given a desired src format
	ImageFormat ComputeActualFormat( ImageFormat srcFormat );

	// Compute the actual mip count based on the actual size
	int ComputeActualMipCount( ) const;

	// Creates/releases the shader api texture
	void AllocateShaderAPITextures( );
	void FreeShaderAPITextures();

	// Download bits
	void DownloadTexture(Rect_t *pRect);
	void ReconstructTextureBits(Rect_t *pRect);

	// Gets us modifying a particular frame of our texture
	void Modify( int iFrame );

	// Sets the texture clamping state on the currently modified frame
	void SetWrapState( );

	// Sets the texture filtering state on the currently modified frame
	void SetFilterState();

	// Sets the texture as the render target
	void Bind( TextureStage_t stage, int nFrame );

	// Set this texture as a render target
	bool SetRenderTarget();

	// Loads the texture bits from a file.
	IVTFTexture *LoadTextureBitsFromFile( );

	// Generates the procedural bits
	IVTFTexture *ReconstructProceduralBits( );
	IVTFTexture *ReconstructPartialProceduralBits( Rect_t *pRect, Rect_t *pActualRect );

	// Sets up debugging texture bits, if appropriate
	bool SetupDebuggingTextures( IVTFTexture *pTexture );

	// Generate a texture that shows the various mip levels
	void GenerateShowMipLevelsTextures( IVTFTexture *pTexture );

	void Cleanup( void );

	// Converts a source image read from disk into its actual format
	void ConvertToActualFormat( IVTFTexture *pTexture );

	// Builds the low-res image from the texture 
	void BuildLowResTexture( IVTFTexture *pTexture );
	void CopyLowResImageToTexture( IVTFTexture *pTexture );
	
	void GetDownloadFaceCount( int &nFirstFace, int &nFaceCount );
	void ComputeMipLevelSubRect( Rect_t* pSrcRect, int nMipLevel, Rect_t *pSubRect );

	IVTFTexture *GetScratchVTFTexture();

protected:
#ifdef _DEBUG
	char *m_pDebugName;
#endif

	CUtlSymbol m_Name;

	// mappingWidth/Height and actualWidth/Height only differ 
	// if g_config.skipMipLevels != 0, or if the card has a hard limit
	// on the maximum texture size

	// This is the iWidth/iHeight for the data that m_pImageData points to.
	unsigned short m_nMappingWidth;
	unsigned short m_nMappingHeight;

	// This is the iWidth/iHeight for whatever is downloaded to the card.
	unsigned short m_nActualWidth;	// this can go away?  needed for procedural
	unsigned short m_nActualHeight;	// this can go away?  needed for procedural

	// Mip count when it's actually used
	unsigned short m_nActualMipCount;

	// This is the *desired* image format, which may or may not represent reality
	ImageFormat m_ImageFormat;

	// The set of texture ids for each animation frame
	int *m_pTexIDs;
	unsigned short m_nFrameCount;

	// Refcount + flags
	short m_nRefCount;
	unsigned int m_nFlags;

	// Reflectivity vector
	Vector m_vecReflectivity;

	// lowresimage info - used for getting color data from a texture
	// without having a huge system mem overhead.
	// FIXME: We should keep this in compressed form. .is currently decompressed at
	// loadtime.
	unsigned char m_LowResImageWidth;
	unsigned char m_LowResImageHeight;
	unsigned char *m_pLowResImage;

	ITextureRegenerator *m_pTextureRegenerator;

	// Fixed-size allocator
//	DECLARE_FIXEDSIZE_ALLOCATOR( CTexture );
public:
	// GR - named RT
	void InitNamedRenderTarget( const char *pRTName, int w, int h, ImageFormat fmt, bool depth, bool clampTexCoords, bool bAutoMipMap );
};


//-----------------------------------------------------------------------------
// Fixed-size allocator
//-----------------------------------------------------------------------------
//DEFINE_FIXEDSIZE_ALLOCATOR( CTexture, 1024, true );


//-----------------------------------------------------------------------------
// Static instance of VTF texture
//-----------------------------------------------------------------------------
static IVTFTexture *s_pVTFTexture = NULL;


//-----------------------------------------------------------------------------
// Class factory methods
//-----------------------------------------------------------------------------
ITextureInternal *ITextureInternal::CreateFileTexture( const char *pFileName, bool bNormalMap )
{
	CTexture *pTex = new CTexture;
	pTex->InitFileTexture( pFileName );
	return pTex;
}

ITextureInternal *ITextureInternal::CreateProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags )
{
	CTexture *pTex = new CTexture;
	pTex->InitProceduralTexture( pTextureName, w, h, fmt, nFlags );
	pTex->IncrementReferenceCount();
	return pTex;
}

ITextureInternal *ITextureInternal::CreateRenderTarget( int w, int h, ImageFormat fmt, bool bDepth )
{
	CTexture *pTex = new CTexture;
	pTex->InitRenderTarget( w, h, fmt, bDepth );
	pTex->IncrementReferenceCount();
	return pTex;
}

// GR - named RT
ITextureInternal *ITextureInternal::CreateNamedRenderTarget( const char *pRTName, int w, int h, ImageFormat fmt, bool bDepth, bool bClampTexCoords, bool bAutoMipMap )
{
	CTexture *pTex = new CTexture;
	pTex->InitNamedRenderTarget( pRTName, w, h, fmt, bDepth, bClampTexCoords, bAutoMipMap );
	pTex->IncrementReferenceCount();
	return pTex;
}

void ITextureInternal::Destroy( ITextureInternal *pTex )
{
	CTexture *pBaseTexture = (CTexture*)pTex;
	if (pBaseTexture)
		delete pBaseTexture;
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CTexture::CTexture() : m_ImageFormat((ImageFormat)-1)
{
	m_nActualMipCount = 0;
	m_nMappingWidth = 0;
	m_nMappingHeight = 0;
	m_nActualWidth = 0;
	m_nActualHeight = 0;
	m_nRefCount = 0;
	m_nFlags = 0;
	m_pTexIDs = NULL;
	m_nFrameCount = 0;
	VectorClear( m_vecReflectivity );
	m_pTextureRegenerator = NULL;

	m_LowResImageWidth = 0;
	m_LowResImageHeight = 0;
	m_pLowResImage = 0;

#ifdef _DEBUG
	m_pDebugName = NULL;
#endif
}

CTexture::~CTexture()
{
#ifdef _DEBUG
	if( m_nRefCount != 0 )
	{
		Warning( "Reference Count(%d) != 0 in ~CTexture for texture \"%s\"\n", m_nRefCount, m_Name.String() );
	}
	if (m_pDebugName)
	{
		delete [] m_pDebugName;
	}
#endif

	Shutdown();
}


//-----------------------------------------------------------------------------
// Initializes the texture
//-----------------------------------------------------------------------------
void CTexture::Init( int w, int h, ImageFormat fmt, int iFlags, int iFrameCount )
{
	Assert( iFrameCount > 0 );

	// Note these are the *desired* values
	m_nMappingWidth = w;
	m_nMappingHeight = h;
	m_ImageFormat = fmt;

	m_nFrameCount = iFrameCount;
	m_nFlags = iFlags;

	// We don't know the actual iWidth + iHeight until we get it ready to render
	m_nActualWidth = m_nActualHeight = 0;
	m_nActualMipCount = 0;

	ReleaseTextureIDs();
	AssignTextureIDs( );
}


//-----------------------------------------------------------------------------
// Shuts down the texture
//-----------------------------------------------------------------------------
void CTexture::Shutdown()
{
	// Clean up the low-res texture
	if (m_pLowResImage)
	{
		delete[] m_pLowResImage;
		m_pLowResImage = 0;
	}

	// Frees the texture regen class
	if (m_pTextureRegenerator)
	{
		m_pTextureRegenerator->Release();
		m_pTextureRegenerator = NULL;
	}

	// This deletes the textures,
	FreeShaderAPITextures();
	ReleaseTextureIDs();
}

void CTexture::Release()
{
	FreeShaderAPITextures();
}


IVTFTexture *CTexture::GetScratchVTFTexture()
{
	if (!s_pVTFTexture)
	{
		s_pVTFTexture = CreateVTFTexture();
	}
	return s_pVTFTexture;
}


//-----------------------------------------------------------------------------
//
// Various initialization methods
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Creates a render target texture
//-----------------------------------------------------------------------------
void CTexture::InitRenderTarget( int w, int h, ImageFormat fmt, bool bDepth )
{
	static int id = 0;
	char pName[128];
	sprintf(pName, "__render_target_%d", id );
	++id;
	SetName(pName);

	int nFlags = TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOCOMPRESS | 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_RENDERTARGET;
	int nFrameCount = 1;

	if (bDepth)
	{
		nFlags |= TEXTUREFLAGS_DEPTHRENDERTARGET;
		++nFrameCount;
	}

	Init( w, h, fmt, nFlags, nFrameCount );
}

//-----------------------------------------------------------------------------
// Creates named render target texture
//-----------------------------------------------------------------------------
void CTexture::InitNamedRenderTarget( const char *pRTName, int w, int h, ImageFormat fmt, bool bDepth, 
									 bool clampTexCoords, bool bAutoMipMap )
{
	SetName(pRTName);

	int nFlags = TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOCOMPRESS | TEXTUREFLAGS_RENDERTARGET;
	if( clampTexCoords )
	{
		nFlags |= TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT;
	}
	int nFrameCount = 1;

	if (bDepth)
	{
		nFlags |= TEXTUREFLAGS_DEPTHRENDERTARGET;
		++nFrameCount;
	}

	if( bAutoMipMap )
	{
		nFlags |= TEXTURE_CREATE_AUTOMIPMAP;
	}

	Init( w, h, fmt, nFlags, nFrameCount );
}


//-----------------------------------------------------------------------------
// Creates a procedural texture
//-----------------------------------------------------------------------------
void CTexture::InitProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags )
{
	// Compressed textures aren't allowed for procedural textures
	Assert( !ImageLoader::IsCompressed(fmt) );

	// We shouldn't be asking for render targets here
	Assert( (nFlags & (TEXTUREFLAGS_RENDERTARGET | TEXTUREFLAGS_DEPTHRENDERTARGET)) == 0 );

	SetName( pTextureName );

	// Eliminate flags that are inappropriate...
	nFlags &= ~TEXTUREFLAGS_HINT_DXT5 | TEXTUREFLAGS_ONEBITALPHA | TEXTUREFLAGS_EIGHTBITALPHA | 
		TEXTUREFLAGS_RENDERTARGET | TEXTUREFLAGS_DEPTHRENDERTARGET;

	// Insert required flags
	nFlags |= TEXTUREFLAGS_PROCEDURAL | TEXTUREFLAGS_NOCOMPRESS;
	int nAlphaBits = ImageLoader::ImageFormatInfo(fmt).m_NumAlphaBits;
	if (nAlphaBits > 1)
		nFlags |= TEXTUREFLAGS_EIGHTBITALPHA;
	else if (nAlphaBits == 1)
		nFlags |= TEXTUREFLAGS_ONEBITALPHA;
	
	// Procedural textures are always one frame only
	Init( w, h, fmt, nFlags, 1 );
}


//-----------------------------------------------------------------------------
// Creates a file texture
//-----------------------------------------------------------------------------
void CTexture::InitFileTexture( const char *pTextureFile )
{
	// For files, we only really know about the file name
	// At any time, the file contents could change, and we could have
	// a different size, number of frames, etc.
	SetName( pTextureFile );
}


//-----------------------------------------------------------------------------
// Assigns/releases texture IDs for our animation frames
//-----------------------------------------------------------------------------
void CTexture::AssignTextureIDs()
{
	Assert( !m_pTexIDs );
	Assert( m_nFrameCount > 0 );
	m_pTexIDs = new int[m_nFrameCount];
	for (int i = 0; i < m_nFrameCount; ++i)
	{
		m_pTexIDs[i] = TextureManager()->RequestNextTextureID();
	}
}

void CTexture::ReleaseTextureIDs()
{
	FreeShaderAPITextures();
	if (m_pTexIDs)
	{
		delete[] m_pTexIDs;
		m_pTexIDs = NULL;
	}
}


//-----------------------------------------------------------------------------
// Creates the texture
//-----------------------------------------------------------------------------
void CTexture::AllocateShaderAPITextures( )
{
	Assert( !HasBeenAllocated() );

	int i;
	int nCount = m_nFrameCount;

	int nCreateFlags = 0;
	if (m_nFlags & TEXTUREFLAGS_ENVMAP)
	{
		if (HardwareConfig()->SupportsCubeMaps())
			nCreateFlags |= TEXTURE_CREATE_CUBEMAP;
	}

	if (m_nFlags & TEXTUREFLAGS_RENDERTARGET)
	{
		nCreateFlags |= TEXTURE_CREATE_RENDERTARGET;

		// This here is simply so we can use a different call to
		// create the depth texture below
		if( nCount == 2 && ( m_nFlags & TEXTUREFLAGS_DEPTHRENDERTARGET ) )
		{
			--nCount;
		}
	}
	else
	{
		// If it's not a render target, use the texture manager in dx
		nCreateFlags |= TEXTURE_CREATE_MANAGED;
	}

	// This is sort of hacky... should we store the # of copies in the VTF?
	int nCopies = 1;
	if ( IsProcedural() && !( m_nFlags & TEXTUREFLAGS_SINGLECOPY ))
		nCopies = 6;

	// Iterate over all animated texture frames
	for( i = 0; i < nCount; ++i )
	{
		// FIXME: That 6 there is heuristically what I came up with what I
		// need to get eyes not to stall on map alyx3. We need a better way
		// of determining how many copies of the texture we should store.
		g_pShaderAPI->CreateTexture( m_pTexIDs[i], 
			m_nActualWidth, m_nActualHeight, m_ImageFormat, m_nActualMipCount,
			nCopies, nCreateFlags );
	}

	// Create the depth render target buffer
	if (m_nFlags & TEXTUREFLAGS_DEPTHRENDERTARGET)
	{	
		g_pShaderAPI->CreateDepthTexture( m_pTexIDs[1], m_ImageFormat, m_nActualWidth, m_nActualHeight );
	}

	m_nFlags |= TEXTUREFLAGS_ALLOCATED;
}


//-----------------------------------------------------------------------------
// Releases the texture's hw memory
//-----------------------------------------------------------------------------
void CTexture::FreeShaderAPITextures()
{
	if (m_pTexIDs && HasBeenAllocated())
	{
		// Release the frames
		for( int i = m_nFrameCount; --i >= 0; )
		{
			if( g_pShaderAPI->IsTexture( m_pTexIDs[i] ) )
			{
				g_pShaderAPI->DeleteTexture( m_pTexIDs[i] );
			}
		}
	}
	m_nFlags &= ~TEXTUREFLAGS_ALLOCATED;
}


//-----------------------------------------------------------------------------
// Computes the actual format of the texture
//-----------------------------------------------------------------------------
ImageFormat CTexture::ComputeActualFormat( ImageFormat srcFormat )
{
	ImageFormat dstFormat;
	bool bIsCompressed = ImageLoader::IsCompressed( srcFormat );
	if( g_config.bCompressedTextures && HardwareConfig()->SupportsCompressedTextures() && bIsCompressed )
	{
		// don't do anything since we are already in a compressed format.
		dstFormat = g_pShaderAPI->GetNearestSupportedFormat( srcFormat );
		Assert( dstFormat == srcFormat );
		return dstFormat;
	}

	// NOTE: This piece of code is only called when compressed textures are
	// turned off, or if the source texture is not compressed.

	// We use the TEXTUREFLAGS_EIGHTBITALPHA and TEXTUREFLAGS_ONEBITALPHA flags
	// to decide how many bits of alpha we need; vtex checks the alpha channel
	// for all white, etc.
	if( srcFormat == IMAGE_FORMAT_UVWQ8888 )
	{
		dstFormat = g_pShaderAPI->GetNearestSupportedFormat( IMAGE_FORMAT_UVWQ8888 );
	} 
	else if( srcFormat == IMAGE_FORMAT_UV88 )
	{
		dstFormat = g_pShaderAPI->GetNearestSupportedFormat( IMAGE_FORMAT_UV88 );
	} 
	else if( m_nFlags & TEXTUREFLAGS_EIGHTBITALPHA )
	{
		dstFormat = g_pShaderAPI->GetNearestSupportedFormat( IMAGE_FORMAT_BGRA8888 );
	}
	else if( m_nFlags & TEXTUREFLAGS_ONEBITALPHA )
	{
		// hack - need to pick formats that have one bit of alpha where appopriate.
		if( g_pShaderAPI->Prefer16BitTextures() ) // garymcthack - move into hwconfigapi
		{
			dstFormat = g_pShaderAPI->GetNearestSupportedFormat( IMAGE_FORMAT_BGRA5551 );
			if( dstFormat == -1 )
			{
				// garymcthack - This is bugly. . why doesn't nvidia support D3DFMT_X1R5G5B5?
				dstFormat = g_pShaderAPI->GetNearestSupportedFormat( IMAGE_FORMAT_BGRX5551 );
			}
		}
		else
		{
			dstFormat = g_pShaderAPI->GetNearestSupportedFormat( IMAGE_FORMAT_BGRA8888 );
		}
	}
	else
	{
		dstFormat = g_pShaderAPI->GetNearestSupportedFormat( IMAGE_FORMAT_BGR888 );
	}
	return dstFormat;
}


//-----------------------------------------------------------------------------
// Compute the actual mip count based on the actual size
//-----------------------------------------------------------------------------
int CTexture::ComputeActualMipCount( ) const
{
	if( m_nFlags & TEXTUREFLAGS_ENVMAP )
	{
		if( !HardwareConfig()->SupportsMipmappedCubemaps() )
		{
			return 1;
		}
	}

	if( m_nFlags & TEXTUREFLAGS_NOMIP )
	{
		return 1;
	}

	// Minmip means use mip levels down to 16x16
	if( m_nFlags & TEXTUREFLAGS_MINMIP )
	{
		int nNumMipLevels = 1;
		int h = m_nActualWidth;
		int w = m_nActualHeight;
		while (	( w > 16 ) && ( h > 16 ) )
		{
			++nNumMipLevels;
			w >>= 1; 
			h >>= 1;
		}
		return nNumMipLevels;
	}

	return ImageLoader::GetNumMipMapLevels( m_nActualWidth, m_nActualHeight );
}


//-----------------------------------------------------------------------------
// Calculates info about whether we can make the texture smaller and by how much
//-----------------------------------------------------------------------------
int CTexture::ComputeActualSize( bool bIgnorePicmip )
{
	// Must skip mip levels if the texture is too large for our board to handle
	m_nActualWidth = m_nMappingWidth;
	m_nActualHeight = m_nMappingHeight;
	int nSkip = 0;
	while ( (m_nActualWidth > HardwareConfig()->MaxTextureWidth()) ||
			(m_nActualHeight > HardwareConfig()->MaxTextureHeight()) )
	{
		m_nActualWidth >>= 1;
		m_nActualHeight >>= 1;
		++nSkip;
	}

	// Skip more if we aren't LODing our texture
	if ( !bIgnorePicmip && (m_nFlags & TEXTUREFLAGS_NOLOD) == 0)
	{
		// Keep skipping until skipMipLevels is satisfied
		while( m_nActualWidth > 4 && m_nActualHeight > 4 )
		{
			if ( nSkip >= g_config.skipMipLevels )
				break;

			m_nActualWidth >>= 1;
			m_nActualHeight >>= 1;
			++nSkip;
		}
	}

	Assert( m_nActualWidth > 0 && m_nActualHeight > 0 );

	// Now that we've got the actual size, we can figure out the mip count
	m_nActualMipCount = ComputeActualMipCount();

	// Returns the number we skipped
	return nSkip;
}


//-----------------------------------------------------------------------------
// Used to modify the texture bits (procedural textures only)
//-----------------------------------------------------------------------------
void CTexture::SetTextureRegenerator( ITextureRegenerator *pTextureRegen )
{
	// NOTE: These can only be used by procedural textures
	Assert( IsProcedural() );

	if (m_pTextureRegenerator)
		m_pTextureRegenerator->Release();
	m_pTextureRegenerator = pTextureRegen; 
}


//-----------------------------------------------------------------------------
// Gets us modifying a particular frame of our texture
//-----------------------------------------------------------------------------
void CTexture::Modify( int iFrame )
{
	Assert( iFrame >= 0 && iFrame < m_nFrameCount );
	g_pShaderAPI->ModifyTexture( m_pTexIDs[iFrame] );
}


//-----------------------------------------------------------------------------
// Sets the texture clamping state on the currently modified frame
//-----------------------------------------------------------------------------
void CTexture::SetWrapState( )
{
	// Clamp mode in S
	if( m_nFlags & TEXTUREFLAGS_CLAMPS )
		g_pShaderAPI->TexWrap( SHADER_TEXCOORD_S, SHADER_TEXWRAPMODE_CLAMP );
	else
		g_pShaderAPI->TexWrap( SHADER_TEXCOORD_S, SHADER_TEXWRAPMODE_REPEAT );

	// Clamp mode in T
	if( m_nFlags & TEXTUREFLAGS_CLAMPT )
		g_pShaderAPI->TexWrap( SHADER_TEXCOORD_T, SHADER_TEXWRAPMODE_CLAMP );
	else
		g_pShaderAPI->TexWrap( SHADER_TEXCOORD_T, SHADER_TEXWRAPMODE_REPEAT );
}


//-----------------------------------------------------------------------------
// Sets the texture filtering state on the currently modified frame
//-----------------------------------------------------------------------------
void CTexture::SetFilterState()
{
	// Turns off filtering when we're point sampling
	if( m_nFlags & TEXTUREFLAGS_POINTSAMPLE )
	{
		g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_NEAREST );
		g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_NEAREST );
		return;
	}

	bool bEnableMipmapping = g_config.bMipMapTextures;
	if( m_nFlags & TEXTUREFLAGS_NOMIP )
		bEnableMipmapping = false;
		
	if( g_config.bFilterTextures )
	{
		if( !bEnableMipmapping )
		{
			g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR );
			g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
			return;
		}

		// Determing the filtering mode
		bool bIsAnisotropic = false, bIsTrilinear = false;
		if ( g_config.m_nForceAnisotropicLevel && HardwareConfig()->MaximumAnisotropicLevel() )
		{
			bIsAnisotropic = true;
		}
		else if ( g_config.m_bForceTrilinear )
		{
			bIsTrilinear = true;
		}
		else if ( !g_config.m_bForceBilinear )
		{
			bIsAnisotropic = (( m_nFlags & TEXTUREFLAGS_ANISOTROPIC ) != 0) && HardwareConfig()->MaximumAnisotropicLevel();
			bIsTrilinear = ( m_nFlags & TEXTUREFLAGS_TRILINEAR ) != 0;
		}

		if ( bIsAnisotropic )
		{		    
			g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_ANISOTROPIC );
			g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_ANISOTROPIC );
		}
		else
		{
			// force trilinear if we are on a dx8 card or above
			if( bIsTrilinear )
			{
				g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR_MIPMAP_LINEAR );
				g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
			}
			else
			{
				g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_LINEAR_MIPMAP_NEAREST );
				g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_LINEAR );
			}
		}
	}
	else
	{
		// Here, filtering textures is off
		if( bEnableMipmapping )
		{
			g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_NEAREST_MIPMAP_NEAREST );
			g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_NEAREST );
		}
		else
		{
			g_pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_NEAREST );
			g_pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_NEAREST );
		}
	}
}


//-----------------------------------------------------------------------------
// Download bits main entry point!!
//-----------------------------------------------------------------------------
void CTexture::DownloadTexture( Rect_t *pRect )
{
	// No downloading necessary if there's no graphics
	if( !g_pShaderAPI->IsUsingGraphics() )
		return;

	// We don't know the actual size of the texture at this stage...
	if (!pRect)
	{
		ReconstructTexture( );
	}
	else
	{
		ReconstructPartialTexture( pRect );
	}

	// Iterate over all the frames and set the appropriate wrapping + filtering state
	SetFilteringAndClampingMode();
}

void CTexture::Download( Rect_t *pRect )
{
	// Only download the bits if we can...
	if ( g_pShaderAPI->CanDownloadTextures() )
	{
		DownloadTexture(pRect);
	}
}


//-----------------------------------------------------------------------------
// Sets the texture as the render target
//-----------------------------------------------------------------------------
void CTexture::Bind( TextureStage_t stage, int iFrame )
{	
	if( g_pShaderAPI->IsUsingGraphics() )
	{
		if( iFrame < 0 || iFrame >= m_nFrameCount )
		{
			// FIXME: Use the well-known 'error' id instead of frame 0
			iFrame = 0;
//			Assert(0);
		}
		g_pShaderAPI->BindTexture( stage, m_pTexIDs[iFrame] );
	}
}



//-----------------------------------------------------------------------------
// Set this texture as a render target
//-----------------------------------------------------------------------------
bool CTexture::SetRenderTarget()
{
	if ((m_nFlags & TEXTUREFLAGS_RENDERTARGET) == 0)
		return false;

	unsigned int bindID = m_pTexIDs[0];
	unsigned int depthID = (unsigned int)SHADER_RENDERTARGET_DEPTHBUFFER;
	if (m_nFlags & TEXTUREFLAGS_DEPTHRENDERTARGET)
		depthID = m_pTexIDs[1];
	g_pShaderAPI->SetRenderTarget( bindID, depthID );
	return true;
}


//-----------------------------------------------------------------------------
// Reference counting
//-----------------------------------------------------------------------------
void CTexture::IncrementReferenceCount( void )
{
	++m_nRefCount;
}

void CTexture::DecrementReferenceCount( void )
{
	--m_nRefCount;

	/* FIXME: Probably have to remove this from the texture manager too..?
	if (IsProcedural() && (m_nRefCount < 0))
		delete this;
	*/
}

int CTexture::GetReferenceCount( )
{
	return m_nRefCount;
}


//-----------------------------------------------------------------------------
// Various accessor methods
//-----------------------------------------------------------------------------
const char* CTexture::GetName( ) const
{
	return m_Name.String();
}

void CTexture::SetName( const char* pName )
{
	// convert to a symbol
	m_Name = pName;

#ifdef _DEBUG
	if (m_pDebugName)
		delete [] m_pDebugName;
	int iLen = Q_strlen( pName ) + 1;
	m_pDebugName = new char[iLen];
	memcpy( m_pDebugName, pName, iLen );
#endif
}

ImageFormat CTexture::GetImageFormat()	const
{
	return m_ImageFormat;
}

int CTexture::GetMappingWidth()	const
{
	return m_nMappingWidth;
}

int CTexture::GetMappingHeight() const
{
	return m_nMappingHeight;
}

int CTexture::GetActualWidth() const
{
	return m_nActualWidth;
}

int CTexture::GetActualHeight()	const
{
	return m_nActualHeight;
}

int CTexture::GetNumAnimationFrames() const
{
	return m_nFrameCount;
}

void CTexture::GetReflectivity( Vector& reflectivity )
{
	VectorCopy( m_vecReflectivity, reflectivity );
}


//-----------------------------------------------------------------------------
// Little helper polling methods
//-----------------------------------------------------------------------------
bool CTexture::IsTranslucent() const
{
	return (m_nFlags & (TEXTUREFLAGS_ONEBITALPHA | TEXTUREFLAGS_EIGHTBITALPHA)) != 0;
}

bool CTexture::IsNormalMap( void ) const
{
	return ( ( m_nFlags & TEXTUREFLAGS_NORMAL ) != 0 );
}

bool CTexture::IsCubeMap( void ) const
{
	return ( ( m_nFlags & TEXTUREFLAGS_ENVMAP ) != 0 );
}

bool CTexture::IsRenderTarget( void ) const
{
	return ( ( m_nFlags & TEXTUREFLAGS_RENDERTARGET ) != 0 );
}

bool CTexture::IsProcedural() const
{
	return ( (m_nFlags & TEXTUREFLAGS_PROCEDURAL) != 0 );
}

bool CTexture::IsMipmapped() const
{
	return ( (m_nFlags & TEXTUREFLAGS_NOMIP) == 0 );
}

bool CTexture::IsError() const
{
	return ( (m_nFlags & TEXTUREFLAGS_ERROR) != 0 );
}

bool CTexture::HasBeenAllocated() const
{
	return ( (m_nFlags & TEXTUREFLAGS_ALLOCATED) != 0 );
}

//-----------------------------------------------------------------------------
// Sets the filtering + clamping modes on the texture
//-----------------------------------------------------------------------------
void CTexture::SetFilteringAndClampingMode( )
{
	for( int iFrame = 0; iFrame < m_nFrameCount; ++iFrame )
	{
		// Indicate we're changing state with respect to a particular frame
		Modify( iFrame );

		// Send the appropriate wrap/clamping modes to the shaderapi.
		SetWrapState();

		// Set the filtering mode for the texture after downloading it.
		// NOTE: Apparently, the filter state cannot be set until after download
		SetFilterState();
	}
}


//-----------------------------------------------------------------------------
// Used by tools.... loads up the non-fallback information about the texture 
//-----------------------------------------------------------------------------
void CTexture::Precache()
{
	// We only have to do something in the case of a file texture
	if (IsRenderTarget() || IsProcedural())
		return;

	// Blow off env_cubemap too...
	if (!Q_strnicmp( m_Name.String(), "env_cubemap", 12 ))
		return;

	if( HasBeenAllocated() )
	{
		return;
	}
	
	int nHeaderSize;
	IVTFTexture *pVTFTexture = GetScratchVTFTexture();

	// The texture name doubles as the relative file name
	// It's assumed to have already been set by this point	
	// Compute the cache name
	char pCacheFileName[MATERIAL_MAX_PATH];
	Q_snprintf( pCacheFileName, MATERIAL_MAX_PATH, "materials/%s.vtf", m_Name.String() );

	nHeaderSize = VTFFileHeaderSize();
	unsigned char *pMem = (unsigned char *)stackalloc( nHeaderSize );
	CUtlBuffer buf( pMem, nHeaderSize );

	FileHandle_t fileHandle = g_pFileSystem->Open( pCacheFileName, "rb" );
	if ( fileHandle == FILESYSTEM_INVALID_HANDLE )
	{
		goto precacheFailed;
	}
	
	// read the header first.. it's faster!!
	g_pFileSystem->Read( buf.Base(), nHeaderSize, fileHandle );
	g_pFileSystem->Close(fileHandle);

	// Unserialize the header
	if (!pVTFTexture->Unserialize( buf, true ))
	{
		Warning( "Error reading material \"%s\"\n", pCacheFileName );
		goto precacheFailed;
	}

	// NOTE: Don't set the image format in case graphics are active
	VectorCopy( pVTFTexture->Reflectivity(), m_vecReflectivity );
	m_nMappingWidth = pVTFTexture->Width();
	m_nMappingHeight = pVTFTexture->Height();
	m_nFlags = pVTFTexture->Flags();
	m_nFrameCount = pVTFTexture->FrameCount();
	return;

precacheFailed:
	m_vecReflectivity.Init( 0, 0, 0 );
	m_nMappingWidth = 32;
	m_nMappingHeight = 32;
	m_nFlags = TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_ERROR;
	m_nFrameCount = 1;
}



//-----------------------------------------------------------------------------
// Builds the low-res image from the texture 
//-----------------------------------------------------------------------------
void CTexture::BuildLowResTexture( IVTFTexture *pTexture )
{
	if( m_pLowResImage )
	{
		delete [] m_pLowResImage;
		m_pLowResImage = NULL;
	}

	if( pTexture->LowResWidth() == 0 || pTexture->LowResHeight() == 0 )
	{
		m_LowResImageWidth = m_LowResImageHeight = 0;
		return;
	}

	m_LowResImageWidth = pTexture->LowResWidth();
	m_LowResImageHeight = pTexture->LowResHeight();
	m_pLowResImage = new unsigned char[m_LowResImageWidth * m_LowResImageHeight * 3];

#ifdef _DEBUG
	bool retVal = 
#endif
		ImageLoader::ConvertImageFormat( pTexture->LowResImageData(), pTexture->LowResFormat(), 
			m_pLowResImage, IMAGE_FORMAT_RGB888, m_LowResImageWidth, m_LowResImageHeight );
	Assert( retVal );	
}


//-----------------------------------------------------------------------------
// Generate a texture that shows the various mip levels
//-----------------------------------------------------------------------------
void CTexture::GenerateShowMipLevelsTextures( IVTFTexture *pTexture )
{
	for (int iFrame = 0; iFrame < pTexture->FrameCount(); ++iFrame )
	{
		for (int iFace = 0; iFace < pTexture->FaceCount(); ++iFace )
		{
			for (int iMip = 0; iMip < pTexture->MipCount(); ++iMip )
			{
				int red =	( ( iMip + 1 ) & 1 ) ? 255 : 0;
				int green = ( ( iMip + 1 ) & 2 ) ? 255 : 0;
				int blue =	( ( iMip + 1 ) & 4 ) ? 255 : 0;

				CPixelWriter pixelWriter;
				pixelWriter.SetPixelMemory( pTexture->Format(), 
					pTexture->ImageData( iFrame, iFace, iMip ), pTexture->RowSizeInBytes( iMip ) );
				
				int nWidth, nHeight;
				pTexture->ComputeMipLevelDimensions( iMip, &nWidth, &nHeight );
				for (int y = 0; y < nHeight; ++y)
				{
					pixelWriter.Seek( 0, y );
					for (int x = 0; x < nWidth; ++x)
					{
						pixelWriter.WritePixel( red, green, blue, 255 );
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Generate a texture that shows the various mip levels
//-----------------------------------------------------------------------------
void CTexture::CopyLowResImageToTexture( IVTFTexture *pTexture )
{
	int nFlags = pTexture->Flags();
	nFlags |= TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_POINTSAMPLE;
	nFlags &= ~(TEXTUREFLAGS_TRILINEAR | TEXTUREFLAGS_ANISOTROPIC | TEXTUREFLAGS_HINT_DXT5);
	nFlags &= ~(TEXTUREFLAGS_NORMAL | TEXTUREFLAGS_ENVMAP);
	nFlags &= ~(TEXTUREFLAGS_ONEBITALPHA | TEXTUREFLAGS_EIGHTBITALPHA);

	Assert( pTexture->FrameCount() == 1 );

	Init( pTexture->Width(), pTexture->Height(), IMAGE_FORMAT_BGR888, nFlags, 1 );
	pTexture->Init( m_LowResImageWidth, m_LowResImageHeight, IMAGE_FORMAT_BGR888, nFlags, 1 );

	// Don't bother computing the actual size; it's actually equal to the low-res size
	// With only one mip level
	m_nActualWidth = m_LowResImageWidth;
	m_nActualHeight = m_LowResImageHeight;
	m_nActualMipCount = 1;

	// Copy the row-res image into the VTF Texture
	CPixelWriter pixelWriter;
	pixelWriter.SetPixelMemory( pTexture->Format(), 
		pTexture->ImageData( 0, 0, 0 ), pTexture->RowSizeInBytes( 0 ) );
	
	unsigned char *pLowResImage = m_pLowResImage;

	for (int y = 0; y < m_LowResImageHeight; ++y)
	{
		pixelWriter.Seek( 0, y );
		for (int x = 0; x < m_LowResImageWidth; ++x)
		{
			int red = pLowResImage[0];
			int green = pLowResImage[1];
			int blue = pLowResImage[2];
			pLowResImage += 3;

			pixelWriter.WritePixel( red, green, blue, 255 );
		}
	}
}



//-----------------------------------------------------------------------------
// Sets up debugging texture bits, if appropriate
//-----------------------------------------------------------------------------
bool CTexture::SetupDebuggingTextures( IVTFTexture *pTexture )
{
	if (pTexture->Flags() & TEXTUREFLAGS_NODEBUGOVERRIDE)
		return false;

	if (g_config.bShowMipLevels)
	{
		// This mode shows the mip levels as different colors

		// Create the actual texture (don't use compressed textures here
		// because we don't want to spend all that time compressing them)
		Init( pTexture->Width(), pTexture->Height(), IMAGE_FORMAT_BGR888, pTexture->Flags(), pTexture->FrameCount() );
		pTexture->Init( pTexture->Width(), pTexture->Height(), IMAGE_FORMAT_BGR888, pTexture->Flags(), pTexture->FrameCount() );

		// Compute the actual texture size + format based on card information
		ComputeActualSize();

		GenerateShowMipLevelsTextures( pTexture );
		return true;
	}
	else if( g_config.bShowLowResImage && pTexture->FrameCount() == 1 && 
		pTexture->FaceCount() == 1 && ((pTexture->Flags() & TEXTUREFLAGS_NORMAL) == 0) &&
		m_LowResImageWidth != 0 && m_LowResImageHeight != 0 )
	{
		// This mode just uses the low res texture
		CopyLowResImageToTexture( pTexture );
		return true;
	}

	return false;
}
		

//-----------------------------------------------------------------------------
// Converts the texture to the actual format
//-----------------------------------------------------------------------------
void CTexture::ConvertToActualFormat( IVTFTexture *pVTFTexture )
{
	if( !g_pShaderAPI->IsUsingGraphics() )
	{
		return;
	}

	ImageFormat dstFormat = ComputeActualFormat( pVTFTexture->Format() );
	if( m_ImageFormat != dstFormat )
	{
		pVTFTexture->ConvertImageFormat( dstFormat, false );
		m_ImageFormat = dstFormat;
	}
}


//-----------------------------------------------------------------------------
// Loads the texture bits from a file.
//-----------------------------------------------------------------------------
IVTFTexture *CTexture::LoadTextureBitsFromFile( )
{
	int nHeaderSize, nMipSkipCount, nFileSize;
	IVTFTexture *pVTFTexture = GetScratchVTFTexture();

	// The texture name doubles as the relative file name
	// It's assumed to have already been set by this point	
	// Compute the cache name
	char pCacheFileName[MATERIAL_MAX_PATH];
	Q_snprintf( pCacheFileName, MATERIAL_MAX_PATH, "materials/%s.vtf", m_Name.String() );

	CUtlBuffer buf;
	FileHandle_t fileHandle = g_pFileSystem->Open( pCacheFileName, "rb" );
	if ( fileHandle == FILESYSTEM_INVALID_HANDLE )
	{
		if (Q_strnicmp( m_Name.String(), "env_cubemap", 12 ))
		{
			Warning( "\"%s\": can't be found on disk\n", pCacheFileName );
		}
		goto fileLoadFailed;
	}
	
	nHeaderSize = VTFFileHeaderSize();
	buf.EnsureCapacity( nHeaderSize );

	// read the header first.. it's faster!!
	g_pFileSystem->Read( buf.Base(), nHeaderSize, fileHandle );
	buf.SeekPut( CUtlBuffer::SEEK_HEAD, nHeaderSize );

	// Unserialize the header
	if (!pVTFTexture->Unserialize( buf, true ))
	{
		Warning( "Error reading material \"%s\"\n", pCacheFileName );
		g_pFileSystem->Close(fileHandle);
		goto fileLoadFailed;
	}

	// Seek the reading back to the front of the buffer...
	buf.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );

	// Gotta do this before we can check if the version is valid
	Init( pVTFTexture->Width(), pVTFTexture->Height(), pVTFTexture->Format(), 
		pVTFTexture->Flags(), pVTFTexture->FrameCount() );
	VectorCopy( pVTFTexture->Reflectivity(), m_vecReflectivity );

	// Compute the actual texture size + format based on card information
	nMipSkipCount = ComputeActualSize();

	// Determine how much of the file to read in
	nFileSize = pVTFTexture->FileSize( nMipSkipCount );
	buf.EnsureCapacity( nFileSize );

	// Here, the rest of the file (that we care about) gets read in
	g_pFileSystem->Read( buf.PeekPut(), nFileSize - nHeaderSize, fileHandle );
	g_pFileSystem->Close(fileHandle);

	// Read in that much of the file...
	// NOTE: Skipping mip levels here will cause the size to be changed...
	if ( !pVTFTexture->Unserialize( buf, false, nMipSkipCount ))
	{
		Warning( "Error reading in file \"%s\"\n", pCacheFileName );
		goto fileLoadFailed;
	}

	// Build the low-res texture
	BuildLowResTexture(pVTFTexture);

	// Try to set up debugging textures, if we're in a debugging mode
	if (!IsProcedural())
	{
		SetupDebuggingTextures( pVTFTexture );
	}

	ConvertToActualFormat( pVTFTexture );

	return pVTFTexture;

fileLoadFailed:
	// This will make a checkerboard texture to indicate failure
	pVTFTexture->Init( 32, 32, IMAGE_FORMAT_BGRA8888, m_nFlags, 1 );
	Init( pVTFTexture->Width(), pVTFTexture->Height(), pVTFTexture->Format(), 
		pVTFTexture->Flags(), pVTFTexture->FrameCount() );
	m_vecReflectivity.Init( 0.5f, 0.5f, 0.5f );

	// NOTE: For mat_picmip to work, we can't must use the same size (32x32)
	// Which should work since every card can handle textures of that size
	m_nActualWidth = pVTFTexture->Width();
	m_nActualHeight = pVTFTexture->Height();
	m_nActualMipCount = 1;

	TextureManager()->GenerateErrorTexture( this, pVTFTexture );
	ConvertToActualFormat( pVTFTexture );

	// Deactivate procedural texture...
	m_nFlags &= ~TEXTUREFLAGS_PROCEDURAL;
	m_nFlags |= TEXTUREFLAGS_ERROR;

	return pVTFTexture;
}


//-----------------------------------------------------------------------------
// Computes subrect for a particular miplevel
//-----------------------------------------------------------------------------
void CTexture::ComputeMipLevelSubRect( Rect_t* pSrcRect, int nMipLevel, Rect_t *pSubRect )
{
	if (nMipLevel == 0)
	{
		*pSubRect = *pSrcRect;
		return;
	}

	float flInvShrink = 1.0f / (float)(1 << nMipLevel);
	pSubRect->x = pSrcRect->x * flInvShrink;
	pSubRect->y = pSrcRect->y * flInvShrink;
	pSubRect->width = (int)ceil( (pSrcRect->x + pSrcRect->width) * flInvShrink ) - pSubRect->x;
	pSubRect->height = (int)ceil( (pSrcRect->y + pSrcRect->height) * flInvShrink ) - pSubRect->y;
}


//-----------------------------------------------------------------------------
// Computes the face count + first face
//-----------------------------------------------------------------------------
void CTexture::GetDownloadFaceCount( int &nFirstFace, int &nFaceCount )
{
	nFaceCount = 1;
	nFirstFace = 0;
	if (IsCubeMap())
	{
		if (HardwareConfig()->SupportsCubeMaps())
		{
			nFaceCount = (CUBEMAP_FACE_COUNT-1);
		}
		else
		{
			// This will cause us to use the spheremap instead of the cube faces
			// in the case where we don't support cubemaps
			nFirstFace = CUBEMAP_FACE_SPHEREMAP;
		}
	}
}


//-----------------------------------------------------------------------------
// Generates the procedural bits
//-----------------------------------------------------------------------------
IVTFTexture *CTexture::ReconstructPartialProceduralBits( Rect_t *pRect, Rect_t *pActualRect )
{
	// Create the texture
	IVTFTexture *pVTFTexture = GetScratchVTFTexture();

	// Figure out the actual size for this texture based on the current mode
	ComputeActualSize();

	// Initialize the texture
	pVTFTexture->Init( m_nActualWidth, m_nActualHeight, 
		ComputeActualFormat( m_ImageFormat ), m_nFlags, m_nFrameCount );

	// Figure out how many mip levels we're skipping...
	int nSizeFactor = GetMappingWidth() / GetActualWidth();
	int nMipSkipCount = 0;
	while (nSizeFactor > 1)
	{
		nSizeFactor >>= 1;
		++nMipSkipCount;
	}

	// Determine a rectangle appropriate for the actual size...
	// It must bound all partially-covered pixels..
	ComputeMipLevelSubRect( pRect, nMipSkipCount, pActualRect );

	// Generate the bits from the installed procedural regenerator
	if (m_pTextureRegenerator)
	{
		m_pTextureRegenerator->RegenerateTextureBits( this, pVTFTexture, pActualRect );
	}
	else
	{
		// In this case, we don't have one, so just use a checkerboard...
		TextureManager()->GenerateErrorTexture( this, pVTFTexture );
	}

	return pVTFTexture;
}


//-----------------------------------------------------------------------------
// Regenerates the bits of a texture within a particular rectangle
//-----------------------------------------------------------------------------
void CTexture::ReconstructPartialTexture( Rect_t *pRect )
{
	// FIXME: for now, only procedural textures can handle sub-rect specification.
	Assert(IsProcedural());

	// Also, we need procedural textures that have only a single copy!!
	// Otherwise this partial upload will not occur on all copies
	Assert( m_nFlags & TEXTUREFLAGS_SINGLECOPY );

	Rect_t vtfRect;
	IVTFTexture *pVTFTexture = ReconstructPartialProceduralBits( pRect, &vtfRect );

	// Make sure we've allocated the API textures
	if (!HasBeenAllocated())
	{
		AllocateShaderAPITextures();
	}

	int nFaceCount, nFirstFace;
	GetDownloadFaceCount( nFirstFace, nFaceCount );
	
	// Blit down portions of the various VTF frames into the board memory
	int nStride;
	Rect_t mipRect;
	for (int iFrame = 0; iFrame < m_nFrameCount; ++iFrame )
	{
		Modify(iFrame);

		for( int iFace = 0; iFace < nFaceCount; ++iFace )
		{
			for( int iMip = 0; iMip < m_nActualMipCount;  ++iMip )
			{
				pVTFTexture->ComputeMipLevelSubRect( &vtfRect, iMip, &mipRect );
				nStride = pVTFTexture->RowSizeInBytes( iMip );
				unsigned char *pBits = pVTFTexture->ImageData( iFrame, iFace + nFirstFace, iMip, mipRect.x, mipRect.y );
				g_pShaderAPI->TexSubImage2D( iMip, iFace, mipRect.x, mipRect.y, 
					mipRect.width, mipRect.height, pVTFTexture->Format(), nStride, pBits );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Generates the procedural bits
//-----------------------------------------------------------------------------
IVTFTexture *CTexture::ReconstructProceduralBits( )
{
	// Create the texture
	IVTFTexture *pVTFTexture = GetScratchVTFTexture();

	// Figure out the actual size for this texture based on the current mode
	ComputeActualSize();

	// Initialize the texture
	pVTFTexture->Init( m_nActualWidth, m_nActualHeight, 
		ComputeActualFormat( m_ImageFormat ), m_nFlags, m_nFrameCount );

	// Generate the bits from the installed procedural regenerator
	if (m_pTextureRegenerator)
	{
		Rect_t rect;
		rect.x = 0; rect.y = 0;
		rect.width = m_nActualWidth; 
		rect.height = m_nActualHeight; 
		m_pTextureRegenerator->RegenerateTextureBits( this, pVTFTexture, &rect );
	}
	else
	{
		// In this case, we don't have one, so just use a checkerboard...
		TextureManager()->GenerateErrorTexture( this, pVTFTexture );
	}

	return pVTFTexture;
}


//-----------------------------------------------------------------------------
// Sets the filtering modes on the texture we're modifying
//-----------------------------------------------------------------------------
void CTexture::ReconstructTexture()
{
	int oldWidth = m_nActualWidth;
	int oldHeight = m_nActualHeight;
	int oldMipCount = m_nActualMipCount;
	int oldFrameCount = m_nFrameCount;

	// FIXME: Should RenderTargets be a special case of Procedural?
	IVTFTexture *pVTFTexture = NULL;
	if (IsProcedural())
	{
		// This will call the installed texture bit regeneration interface
		pVTFTexture = ReconstructProceduralBits( );
	}
	else if (IsRenderTarget())
	{
		// Compute the actual size + format based on the current mode
		ComputeActualSize( true );
	}
	else
	{
		// First try to load the data from disk...
		// NOTE: Reloading the texture bits can cause the texture size, frames,
		// format, pretty much *anything* can change.
		pVTFTexture = LoadTextureBitsFromFile();
	}

	if( !HasBeenAllocated() ||
		m_nActualWidth != oldWidth ||
		m_nActualHeight != oldHeight ||
		m_nActualMipCount != oldMipCount ||
		m_nFrameCount != oldFrameCount )
	{
		// This is necessary because when we reload, we may discover there
		// are more frames of a texture animation, for example, which means
		// we can't rely on having the same number of texture frames.
		if( HasBeenAllocated() )
		{
			FreeShaderAPITextures();
		}

		// Next, create the shader api textures
		AllocateShaderAPITextures();
	}

	if (IsRenderTarget())
		return;

	int nFaceCount, nFirstFace;
	GetDownloadFaceCount( nFirstFace, nFaceCount );
	
	// Blit down the various VTF frames into the board memory
	int nWidth, nHeight;
	for (int iFrame = 0; iFrame < m_nFrameCount; ++iFrame )
	{
		Modify(iFrame);

		for( int iFace = 0; iFace < nFaceCount; ++iFace )
		{
			for( int iMip = 0; iMip < m_nActualMipCount;  ++iMip )
			{
				pVTFTexture->ComputeMipLevelDimensions( iMip, &nWidth, &nHeight );
				unsigned char *pBits = pVTFTexture->ImageData( iFrame, iFace + nFirstFace, iMip );
				g_pShaderAPI->TexImage2D( iMip, iFace, m_ImageFormat, nWidth, nHeight, 
					pVTFTexture->Format(), pBits );
			}
		}
	}
}


void CTexture::GetLowResColorSample( float s, float t, float *color ) const
{
#if 1
	if( m_LowResImageWidth <= 0 || m_LowResImageHeight <= 0 )
	{
	//	Warning( "Programming error: GetLowResColorSample \"%s\": %dx%d\n", 
	//		m_pName, ( int )m_LowResImageWidth, ( int )m_LowResImageHeight );
		return;
	}

	// force s and t into [0,1)
	if( s < 0.0f )
	{
		s = ( 1.0f - ( float )( int )s ) + s;
	}
	if( t < 0.0f )
	{
		t = ( 1.0f - ( float )( int )t ) + t;
	}
	s = s - ( float )( int )s;
	t = t - ( float )( int )t;
	
	s *= m_LowResImageWidth;
	t *= m_LowResImageHeight;
	
	int wholeS, wholeT;
	wholeS = ( int )s;
	wholeT = ( int )t;
	float fracS, fracT;
	fracS = s - ( float )( int )s;
	fracT = t - ( float )( int )t;
	
	// filter twice in the s dimension.
	float sColor[2][3];
	int wholeSPlusOne = ( wholeS + 1 ) % m_LowResImageWidth;
	int wholeTPlusOne = ( wholeT + 1 ) % m_LowResImageHeight;
	sColor[0][0] = ( 1.0f - fracS ) * ( m_pLowResImage[( wholeS + wholeT * m_LowResImageWidth ) * 3 + 0] * ( 1.0f / 255.0f ) );
	sColor[0][1] = ( 1.0f - fracS ) * ( m_pLowResImage[( wholeS + wholeT * m_LowResImageWidth ) * 3 + 1] * ( 1.0f / 255.0f ) );
	sColor[0][2] = ( 1.0f - fracS ) * ( m_pLowResImage[( wholeS + wholeT * m_LowResImageWidth ) * 3 + 2] * ( 1.0f / 255.0f ) );
	sColor[0][0] += fracS * ( m_pLowResImage[( wholeSPlusOne + wholeT * m_LowResImageWidth ) * 3 + 0] * ( 1.0f / 255.0f ) );
	sColor[0][1] += fracS * ( m_pLowResImage[( wholeSPlusOne + wholeT * m_LowResImageWidth ) * 3 + 1] * ( 1.0f / 255.0f ) );
	sColor[0][2] += fracS * ( m_pLowResImage[( wholeSPlusOne + wholeT * m_LowResImageWidth ) * 3 + 2] * ( 1.0f / 255.0f ) );

	sColor[1][0] = ( 1.0f - fracS ) * ( m_pLowResImage[( wholeS + wholeTPlusOne * m_LowResImageWidth ) * 3 + 0] * ( 1.0f / 255.0f ) );
	sColor[1][1] = ( 1.0f - fracS ) * ( m_pLowResImage[( wholeS + wholeTPlusOne * m_LowResImageWidth ) * 3 + 1] * ( 1.0f / 255.0f ) );
	sColor[1][2] = ( 1.0f - fracS ) * ( m_pLowResImage[( wholeS + wholeTPlusOne * m_LowResImageWidth ) * 3 + 2] * ( 1.0f / 255.0f ) );
	sColor[1][0] += fracS * ( m_pLowResImage[( wholeSPlusOne + wholeTPlusOne * m_LowResImageWidth ) * 3 + 0] * ( 1.0f / 255.0f ) );
	sColor[1][1] += fracS * ( m_pLowResImage[( wholeSPlusOne + wholeTPlusOne * m_LowResImageWidth ) * 3 + 1] * ( 1.0f / 255.0f ) );
	sColor[1][2] += fracS * ( m_pLowResImage[( wholeSPlusOne + wholeTPlusOne * m_LowResImageWidth ) * 3 + 2] * ( 1.0f / 255.0f ) );

	color[0] = sColor[0][0] * ( 1.0f - fracT ) + sColor[1][0] * fracT;
	color[1] = sColor[0][1] * ( 1.0f - fracT ) + sColor[1][1] * fracT;
	color[2] = sColor[0][2] * ( 1.0f - fracT ) + sColor[1][2] * fracT;
#endif
}

int CTexture::GetApproximateVidMemBytes( void ) const
{
	ImageFormat format = GetImageFormat();
	int width = GetActualWidth();
	int height = GetActualHeight();
	int numFrames = GetNumAnimationFrames();
	bool isMipmapped = IsMipmapped();

	return numFrames * ImageLoader::GetMemRequired( width, height, format, isMipmapped );
}

void CTexture::CopyFrameBufferToMe( void )
{
	Assert( m_pTexIDs && m_nFrameCount >= 1 );
	if( m_pTexIDs && m_nFrameCount >= 1 )
	{
		g_pShaderAPI->CopyRenderTargetToTexture( m_pTexIDs[0] );
	}
}
