//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include <stdlib.h>
#include <malloc.h>
#include "materialsystem_global.h"
#include "string.h"
#include "shaderapi.h"
#include "materialsystem/materialsystem_config.h"
#include "texturemanager.h"

#include "vstdlib/strtools.h"
#include "utlvector.h"
#include "itextureinternal.h"
#include "vtf/vtf.h"
#include "pixelwriter.h"
#include "basetypes.h"
#include "utlbuffer.h"
#include "filesystem.h"
#include "tier0/memdbgon.h"

// Render texture declaration
#include "matrendertexture.h"

#define ERROR_TEXTURE_SIZE	32
#define WHITE_TEXTURE_SIZE  1
#define BLACK_TEXTURE_SIZE  1
#define NORMALIZATION_CUBEMAP_SIZE  32


//-----------------------------------------------------------------------------
//
// Various procedural texture regeneration classes
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Creates a checkerboard texture
//-----------------------------------------------------------------------------
class CCheckerboardTexture : public ITextureRegenerator
{
public:
	CCheckerboardTexture( int nCheckerSize, color32 color1, color32 color2 ) :
		m_nCheckerSize( nCheckerSize ), m_Color1(color1), m_Color2(color2)
	{
	}

	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
	{
		for (int iFrame = 0; iFrame < pVTFTexture->FrameCount(); ++iFrame )
		{
			for (int iFace = 0; iFace < pVTFTexture->FaceCount(); ++iFace )
			{
				// Fill mip 0 with a checkerboard
				CPixelWriter pixelWriter;
				pixelWriter.SetPixelMemory( pVTFTexture->Format(), 
					pVTFTexture->ImageData( iFrame, iFace, 0 ), pVTFTexture->RowSizeInBytes( 0 ) );
				
				int nWidth = pVTFTexture->Width();
				int nHeight = pVTFTexture->Height();
				for (int y = 0; y < nHeight; ++y)
				{
					pixelWriter.Seek( 0, y );
					for (int x = 0; x < nWidth; ++x)
					{
						if ((x & m_nCheckerSize) ^ (y & m_nCheckerSize))
							pixelWriter.WritePixel( m_Color1.r, m_Color1.g, m_Color1.b, m_Color1.a );
						else
							pixelWriter.WritePixel( m_Color2.r, m_Color2.g, m_Color2.b, m_Color2.a );
					}
				}
			}
		}
	}

	virtual void Release()
	{
		delete this;
	}

private:
	int		m_nCheckerSize;
	color32 m_Color1;
	color32 m_Color2;
};

static void CreateCheckerboardTexture( ITextureInternal *pTexture, int nCheckerSize, color32 color1, color32 color2 )
{
	ITextureRegenerator *pRegen = new CCheckerboardTexture( nCheckerSize, color1, color2 );
	pTexture->SetTextureRegenerator( pRegen );
}


//-----------------------------------------------------------------------------
// Creates a solid texture
//-----------------------------------------------------------------------------
class CSolidTexture : public ITextureRegenerator
{
public:
	CSolidTexture( color32 color ) : m_Color(color)
	{
	}

	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
	{
		int nMipCount = pTexture->IsMipmapped() ? pVTFTexture->MipCount() : 1;
		for (int iFrame = 0; iFrame < pVTFTexture->FrameCount(); ++iFrame )
		{
			for (int iFace = 0; iFace < pVTFTexture->FaceCount(); ++iFace )
			{
				for (int iMip = 0; iMip < nMipCount; ++iMip )
				{
					CPixelWriter pixelWriter;
					pixelWriter.SetPixelMemory( pVTFTexture->Format(), 
						pVTFTexture->ImageData( iFrame, iFace, iMip ), pVTFTexture->RowSizeInBytes( iMip ) );
					
					int nWidth, nHeight;
					pVTFTexture->ComputeMipLevelDimensions( iMip, &nWidth, &nHeight );
					for (int y = 0; y < nHeight; ++y)
					{
						pixelWriter.Seek( 0, y );
						for (int x = 0; x < nWidth; ++x)
						{
							pixelWriter.WritePixel( m_Color.r, m_Color.g, m_Color.b, m_Color.a );
						}
					}
				}
			}
		}
	}

	virtual void Release()
	{
		delete this;
	}

private:
	color32 m_Color;
};

static void CreateSolidTexture( ITextureInternal *pTexture, color32 color )
{
	ITextureRegenerator *pRegen = new CSolidTexture( color );
	pTexture->SetTextureRegenerator( pRegen );
}


//-----------------------------------------------------------------------------
// Creates a normalization cubemap texture
//-----------------------------------------------------------------------------
class CNormalizationCubemap : public ITextureRegenerator
{
public:
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
	{
		// Normalization cubemap doesn't make sense on low-end hardware
		// So we won't construct a spheremap out of this
		CPixelWriter pixelWriter;

		Vector direction;
		for (int iFace = 0; iFace < 6; ++iFace)
		{
			pixelWriter.SetPixelMemory( pVTFTexture->Format(), 
				pVTFTexture->ImageData( 0, iFace, 0 ), pVTFTexture->RowSizeInBytes( 0 ) );
			
			int nWidth = pVTFTexture->Width();
			int nHeight = pVTFTexture->Height();

			float flInvWidth = 2.0f / (float)(nWidth-1);
			float flInvHeight = 2.0f / (float)(nHeight-1);

			for (int y = 0; y < nHeight; ++y)
			{
				float v = y * flInvHeight - 1.0f;

				pixelWriter.Seek( 0, y );
				for (int x = 0; x < nWidth; ++x)
				{
					float u = x * flInvWidth - 1.0f;
					float oow = 1.0f / sqrt( 1.0f + u*u + v*v );

					int ix = (int)(255 * 0.5 * (u*oow + 1.0f) + 0.5f);
					ix = clamp( ix, 0, 255 );
					int iy = (int)(255 * 0.5 * (v*oow + 1.0f) + 0.5f);
					iy = clamp( iy, 0, 255 );
					int iz = (int)(255 * 0.5 * (oow + 1.0f) + 0.5f);
					iz = clamp( iz, 0, 255 );

					switch (iFace)
					{
					case CUBEMAP_FACE_RIGHT:
						pixelWriter.WritePixel( iz, 255 - iy, 255 - ix, 255 );
						break;
					case CUBEMAP_FACE_LEFT:
						pixelWriter.WritePixel( 255 - iz, 255 - iy, ix, 255 );
						break;
					case CUBEMAP_FACE_BACK:	
						pixelWriter.WritePixel( ix, iz, iy, 255 );
						break;
					case CUBEMAP_FACE_FRONT:
						pixelWriter.WritePixel( ix, 255 - iz, 255 - iy, 255 );
						break;
					case CUBEMAP_FACE_UP:
						pixelWriter.WritePixel( ix, 255 - iy, iz, 255 );
						break;
					case CUBEMAP_FACE_DOWN:
						pixelWriter.WritePixel( 255 - ix, 255 - iy, 255 - iz, 255 );
						break;
					default:
						break;
					}
				}
			}
		}
	}

	// NOTE: The normalization cubemap regenerator is stateless
	// so there's no need to allocate + deallocate them
	virtual void Release() {}
};

static void CreateNormalizationCubemap( ITextureInternal *pTexture )
{
	// NOTE: The normalization cubemap regenerator is stateless
	// so there's no need to allocate + deallocate them
	static CNormalizationCubemap s_NormalizationCubemap;
	pTexture->SetTextureRegenerator( &s_NormalizationCubemap );
}


//-----------------------------------------------------------------------------
// Implementation of the texture manager
//-----------------------------------------------------------------------------
class CTextureManager : public ITextureManager
{
public:
	CTextureManager( void );

	// Initialization + shutdown
	virtual void Init();
	virtual void Shutdown();

	virtual ITextureInternal *CreateProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags );
	virtual ITextureInternal *CreateRenderTargetTexture( int w, int h, ImageFormat fmt, bool depth, bool bAutoMipMap );
	virtual ITextureInternal *FindOrLoadTexture( const char *textureName, bool isBump = false );

	virtual void ResetTextureFilteringState();
	void ReloadTextures( void );

	// These two are used when we lose our video memory due to a mode switch etc
	void ReleaseTextures( void );
	void RestoreTextures( void );

	// delete any texture that has a refcount <= 0
	void RemoveUnusedTextures( void );
	void DebugPrintUsedTextures( void );

	// Request a texture ID
	virtual int	RequestNextTextureID();

	// Get at a couple standard textures
	virtual ITextureInternal *ErrorTexture();
	virtual ITextureInternal *NormalizationCubemap();

	// Generates an error texture pattern
	virtual void GenerateErrorTexture( ITexture *pTexture, IVTFTexture *pVTFTexture );

protected:
	ITextureInternal *FindTexture( const char *textureName, bool isBump = false );
	ITextureInternal *LoadTexture( const char *textureName, bool isBump );

	// GR - named RT
	ITextureInternal *CreateWellKnownNamedRenderTexture( const char *textureName );

	// Restores a single texture
	void RestoreTexture( ITextureInternal* pTex );

	CUtlVector<ITextureInternal *> m_TextureList;
	int m_iNextTexID;

	ITextureInternal *m_pErrorTexture;
	ITextureInternal *m_pBlackTexture;
	ITextureInternal *m_pWhiteTexture;
	ITextureInternal *m_pNormalizationCubemap;

	// Used to generate various error texture patterns when necessary
	CCheckerboardTexture *m_pErrorRegen;

	// Used to determine when to reset filtering state..
	bool m_bEnableMipmapping;
	bool m_bEnableFiltering;

public:
	// GR - named RT
	virtual ITextureInternal *CreateNamedRenderTargetTexture( const char *pRTName, int w, int h, ImageFormat fmt, bool depth, bool bClampTexCoords, bool bAutoMipMap );
};


//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------
static CTextureManager s_TextureManager;
ITextureManager *g_pTextureManager = &s_TextureManager;


//-----------------------------------------------------------------------------
// Texture manager
//-----------------------------------------------------------------------------
CTextureManager::CTextureManager( void )
{
}


//-----------------------------------------------------------------------------
// Initialization + shutdown
//-----------------------------------------------------------------------------
void CTextureManager::Init()
{
	color32 color, color2;
	m_iNextTexID = 4096;

	m_bEnableMipmapping = true;
	m_bEnableFiltering = true;

	// Create the default (error) texture
	m_pErrorTexture = CreateProceduralTexture( "error", 
		ERROR_TEXTURE_SIZE, ERROR_TEXTURE_SIZE, IMAGE_FORMAT_BGRA8888, TEXTUREFLAGS_NOMIP );
	color.r = color.g = color.b = 0; color.a = 128;
	color2.r = color2.b = color2.a = 255; color2.g = 0;
	CreateCheckerboardTexture( m_pErrorTexture, 4, color, color2 );

	m_pErrorRegen = new CCheckerboardTexture( 4, color, color2 );

	// Create a white texture
	m_pWhiteTexture = CreateProceduralTexture( "white",
		WHITE_TEXTURE_SIZE, WHITE_TEXTURE_SIZE, IMAGE_FORMAT_BGRX8888, 0 );
	color.r = color.g = color.b = color.a = 255;
	CreateSolidTexture( m_pWhiteTexture, color );

	// Create a black texture
	m_pBlackTexture = CreateProceduralTexture( "black",
		BLACK_TEXTURE_SIZE, BLACK_TEXTURE_SIZE, IMAGE_FORMAT_BGRX8888, 0 );
	color.r = color.g = color.b = 0;
	CreateSolidTexture( m_pBlackTexture, color );

	// Create a normalization cubemap
	m_pNormalizationCubemap = CreateProceduralTexture( "normalize",
		NORMALIZATION_CUBEMAP_SIZE, NORMALIZATION_CUBEMAP_SIZE, IMAGE_FORMAT_BGRX8888,
		TEXTUREFLAGS_ENVMAP | TEXTUREFLAGS_NOMIP );
	CreateNormalizationCubemap( m_pNormalizationCubemap );
}

void CTextureManager::Shutdown()
{
	// These checks added because it's possible for shutdown to be called before the material system is 
	// fully initialized.
	if( m_pWhiteTexture )			m_pWhiteTexture->DecrementReferenceCount();
	if( m_pNormalizationCubemap )	m_pNormalizationCubemap->DecrementReferenceCount();
	if( m_pBlackTexture )			m_pBlackTexture->DecrementReferenceCount();
	if( m_pErrorTexture )			m_pErrorTexture->DecrementReferenceCount();
	ReleaseTextures();
	if( m_pErrorRegen )				m_pErrorRegen->Release();

	for (int i = m_TextureList.Count(); --i >= 0; )
	{
		ITextureInternal::Destroy( m_TextureList[i] );
	}
	m_TextureList.RemoveAll();
}


//-----------------------------------------------------------------------------
// Generates an error texture pattern
//-----------------------------------------------------------------------------
void CTextureManager::GenerateErrorTexture( ITexture *pTexture, IVTFTexture *pVTFTexture )
{
	m_pErrorRegen->RegenerateTextureBits( pTexture, pVTFTexture, NULL );
}


//-----------------------------------------------------------------------------
// Releases all textures (cause we've lost video memory)
//-----------------------------------------------------------------------------
void CTextureManager::ReleaseTextures( void )
{
	for (int i = m_TextureList.Count(); --i >= 0; )
	{
		// Release the texture...
		m_TextureList[i]->Release();
	}	
}


//-----------------------------------------------------------------------------
// Request a texture ID
//-----------------------------------------------------------------------------
int CTextureManager::RequestNextTextureID()
{
	// FIXME: Deal better with texture ids
	// The range between 19000 and 21000 are used for standard textures + lightmaps
	if (m_iNextTexID == 19000)
		m_iNextTexID = 21000;

	return m_iNextTexID++;
}


//-----------------------------------------------------------------------------
// Restores a single texture
//-----------------------------------------------------------------------------
void CTextureManager::RestoreTexture( ITextureInternal* pTexture )
{
	// Put the texture back onto the board
	pTexture->Download( );
}


//-----------------------------------------------------------------------------
// Restore all textures (cause we've got video memory again)
//-----------------------------------------------------------------------------
void CTextureManager::RestoreTextures( void )
{
	for (int i = m_TextureList.Count(); --i >= 0; )
	{
		// Restore the texture...
		RestoreTexture( m_TextureList[i] );
	}
}


//-----------------------------------------------------------------------------
// Reloads all textures
//-----------------------------------------------------------------------------
void CTextureManager::ReloadTextures( void )
{
	for (int i = m_TextureList.Count(); --i >= 0; )
	{
		// Put the texture back onto the board
		m_TextureList[i]->Download( );
	}
}


//-----------------------------------------------------------------------------
// Get at a couple standard textures
//-----------------------------------------------------------------------------
ITextureInternal *CTextureManager::ErrorTexture()
{
	return m_pErrorTexture;
}

ITextureInternal *CTextureManager::NormalizationCubemap()
{
	return m_pNormalizationCubemap; 
}


//-----------------------------------------------------------------------------
// Creates a procedural texture
//-----------------------------------------------------------------------------
ITextureInternal *CTextureManager::CreateProceduralTexture( const char *pTextureName, 
									int w, int h, ImageFormat fmt, int nFlags )
{
	ITextureInternal *pNewTexture = ITextureInternal::CreateProceduralTexture( pTextureName, w, h, fmt, nFlags );
	if ( !pNewTexture )
		return NULL;

	// Add it to the list of textures so it can be restored, etc.
	m_TextureList.AddToTail( pNewTexture );

	// NOTE: This will download the texture only if the shader api is ready
	pNewTexture->Download();

	return pNewTexture;
}

//-----------------------------------------------------------------------------
// FIXME: Need some better understanding of when textures should be added to
// the texture dictionary here. Is it only for files, for example?
// Texture dictionary...
//-----------------------------------------------------------------------------
ITextureInternal *CTextureManager::LoadTexture( const char *pTextureName, bool bIsBump )
{
	ITextureInternal *pNewTexture = ITextureInternal::CreateFileTexture( pTextureName, bIsBump );
	if( pNewTexture )
	{
		// Stick the texture onto the board
		pNewTexture->Download();

		// FIXME: If there's been an error loading, we don't also want this error...

		// After it's been downloaded, we know if it's a normal map or not
		if( pNewTexture->IsNormalMap() && !bIsBump )
		{
			Warning( "texture \"%s\" compiled as a normal map but used as a regular texture\n", pTextureName );
		}
		if( !pNewTexture->IsNormalMap() && bIsBump )
		{
			// FIXME FIXME: put this back in once I figure out why things aren't compiling as normal maps.
			//			Warning( "texture \"%s\" needs to be compiled as a normal map\n", pTextureName );
		}
	}

	return pNewTexture;
}

ITextureInternal *CTextureManager::FindTexture( const char *pTextureName, bool bIsBump )
{
	if (!pTextureName || pTextureName[0] == 0)
		return NULL;

	// FIXME: Use a UtlDict here, get log(n)
	for (int i = m_TextureList.Count(); --i >= 0; )
	{
		if( Q_strnicmp( m_TextureList[i]->GetName(), pTextureName, 1024 ) == 0 )
		{
			if ( m_TextureList[i]->IsNormalMap() == bIsBump )
				return m_TextureList[i];
		}
	}
	return NULL;
}

ITextureInternal *CTextureManager::FindOrLoadTexture( const char *pTextureName, bool bIsBump )
{
	ITextureInternal *pTexture = FindTexture( pTextureName, bIsBump );
	if( !pTexture )
	{
		// Check if it's a named render texture
		pTexture = CreateWellKnownNamedRenderTexture( pTextureName );
		if( !pTexture )
		{
			pTexture = LoadTexture( pTextureName, bIsBump );
			if( pTexture )
			{
				m_TextureList.AddToTail( pTexture );
			}
		}
	}
	return pTexture;
}

//=============================================================================
// Attempts to create named render texture from the list.
// Returns NULL is named render texture isn't declared here.
//=============================================================================
ITextureInternal *CTextureManager::CreateWellKnownNamedRenderTexture( const char *pTextureName )
{
	for( int i = 0; rtDescriptors[i].rtCreator != NULL; i++ )
	{
		if( Q_strnicmp( rtDescriptors[i].rtName, pTextureName, 50 ) == 0 )
			return (ITextureInternal *)rtDescriptors[i].rtCreator( );
	}

#ifdef _DEBUG
	if( Q_strnicmp( pTextureName, "_rt", 3 ) == 0 )
	{
		Assert( 0 );
	}
#endif

	// Not found in the list
	return NULL;
}

//-----------------------------------------------------------------------------
// Creates a texture that's a render target
//-----------------------------------------------------------------------------
ITextureInternal *CTextureManager::CreateRenderTargetTexture( int w, int h, ImageFormat fmt, bool bDepth, bool bAutoMipMap )
{
	ITextureInternal *pNewTexture = ITextureInternal::CreateRenderTarget( w, h, fmt, bDepth );
	if ( !pNewTexture )
		return NULL;

	// Add the render target to the list of textures
	// that way it'll get cleaned up correctly in case of a task switch
	m_TextureList.AddToTail( pNewTexture );

	// NOTE: This will download the texture only if the shader api is ready
	pNewTexture->Download();

	return pNewTexture;
}

//-----------------------------------------------------------------------------
// GR - Creates named RT texture
//-----------------------------------------------------------------------------
ITextureInternal *CTextureManager::CreateNamedRenderTargetTexture( const char *pRTName, int w, int h, ImageFormat fmt, bool bDepth, bool bClampTexCoords, bool bAutoMipMap )
{
	ITextureInternal *pNewTexture = ITextureInternal::CreateNamedRenderTarget( pRTName, w, h, fmt, bDepth, bClampTexCoords, bAutoMipMap );
	if ( !pNewTexture )
		return NULL;

	// Add the render target to the list of textures
	// that way it'll get cleaned up correctly in case of a task switch
	m_TextureList.AddToTail( pNewTexture );

	// NOTE: This will download the texture only if the shader api is ready
	pNewTexture->Download();

	return pNewTexture;
}

void CTextureManager::ResetTextureFilteringState( )
{
	// Look for a change in state....
	if ( (m_bEnableMipmapping != g_config.bMipMapTextures) ||
		(m_bEnableFiltering != g_config.bFilterTextures) )
	{
		m_bEnableMipmapping = g_config.bMipMapTextures;
		m_bEnableFiltering = g_config.bFilterTextures;

		for (int i = m_TextureList.Count(); --i >= 0; )
		{
			m_TextureList[i]->SetFilteringAndClampingMode( );
		}
	}
}


void CTextureManager::RemoveUnusedTextures( void )
{
	for (int i = m_TextureList.Count(); --i >= 0; )
	{

#ifdef _DEBUG
		if( m_TextureList[i]->GetReferenceCount() < 0 )
		{
			Warning( "NeedsRemoval: pTexture->m_referenceCount < 0 for %s\n", m_TextureList[i]->GetName() );
		}
#endif

		if( m_TextureList[i]->GetReferenceCount() <= 0 )
		{
			ITextureInternal::Destroy( m_TextureList[i] );
			m_TextureList.FastRemove(i);
		}
	}
}

void CTextureManager::DebugPrintUsedTextures( void )
{
	for (int i = m_TextureList.Count(); --i >= 0; )
	{
		ITextureInternal *pTexture = m_TextureList[i];
		Warning( "Texture \"%s\" has refcount %d\n", pTexture->GetName(), pTexture->GetReferenceCount() );
	}
}
