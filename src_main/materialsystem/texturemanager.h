//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header:     $
// $NoKeywords: $
//=============================================================================

#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#ifdef _WIN32
#pragma once
#endif


class ITexture;
class ITextureInternal;
class IVTFTexture;
enum ImageFormat;

class ITextureManager
{
public:
	// Initialization + shutdown
	virtual void Init() = 0;
	virtual void Shutdown() = 0;

	// Creates a procedural texture
	// NOTE: Passing in NULL as a texture name will cause it to not
	// be able to be looked up by name using FindOrLoadTexture.
	// NOTE: Using compressed textures is not allowed; also 
	// Also, you may not get a texture with the requested size or format;
	// you'll get something close though.
	virtual ITextureInternal *CreateProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags ) = 0;

	// Creates a texture which is a render target
	virtual ITextureInternal *CreateRenderTargetTexture( int w, int h, ImageFormat fmt, bool depth, bool bAutoMipMap = false ) = 0;

	// Loads a texture from disk
	virtual ITextureInternal *FindOrLoadTexture( const char *pTextureName, bool bIsBump = false ) = 0;

	// Call this to reset the filtering state
	virtual void ResetTextureFilteringState() = 0;

	// Reload all textures
	virtual void ReloadTextures( void ) = 0;

	// These two are used when we lose our video memory due to a mode switch etc
	virtual void ReleaseTextures( void ) = 0;
	virtual void RestoreTextures( void ) = 0;

	// delete any texture that has a refcount <= 0
	virtual void RemoveUnusedTextures( void ) = 0;
	virtual void DebugPrintUsedTextures( void ) = 0;

	// Request a texture ID
	virtual int	RequestNextTextureID() = 0;

	// Get at a couple standard textures
	virtual ITextureInternal *ErrorTexture() = 0;
	virtual ITextureInternal *NormalizationCubemap() = 0;

	// Generates an error texture pattern
	virtual void GenerateErrorTexture( ITexture *pTexture, IVTFTexture *pVTFTexture ) = 0;

	// GR - named RT
	virtual ITextureInternal *CreateNamedRenderTargetTexture( const char *pRTName, int w, int h, ImageFormat fmt, bool depth, bool bClampTexCoords, bool bAutoMipMap ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------
inline ITextureManager *TextureManager()
{
	extern ITextureManager *g_pTextureManager;
	return g_pTextureManager;
}


#endif // TEXTUREMANAGER_H
