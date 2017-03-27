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
// Semi-secret texture interface used by shader implementations
//=============================================================================

#ifndef ITEXTUREINTERNAL_H
#define ITEXTUREINTERNAL_H

#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/itexture.h"

class Vector;
enum TextureStage_t;

enum RenderTargetType_t
{
	NO_RENDER_TARGET = 0,
	RENDER_TARGET = 1,
	RENDER_TARGET_WITH_DEPTH = 2,
};

class ITextureInternal : public ITexture
{
public:
	virtual void Bind( TextureStage_t stage, int frameNum = 0 ) = 0;

	// Methods associated with reference counting
	virtual int GetReferenceCount() = 0;

	virtual void GetReflectivity( Vector& reflectivity ) = 0;

	// Set this as the render target, return false for failure
	virtual bool SetRenderTarget() = 0;

	// Is this guy a render target?
	virtual bool IsRenderTarget() const = 0;
	virtual bool IsCubeMap() const = 0;
	virtual bool IsNormalMap() const = 0;
	virtual bool IsProcedural() const = 0;
	virtual bool IsError() const = 0;

	// Releases the texture's hw memory
	virtual void Release() = 0;

	// Resets the texture's filtering and clamping mode
	virtual void SetFilteringAndClampingMode() = 0;

	// Used by tools.... loads up the non-fallback information about the texture 
	virtual void Precache() = 0;

	// Stretch blit the framebuffer into this texture.
	virtual void CopyFrameBufferToMe( void ) = 0;

	// Creates a new texture
	static ITextureInternal *CreateFileTexture( const char *pFileName, bool  );
	static ITextureInternal *CreateProceduralTexture( const char *pTextureName, int w, int h, ImageFormat fmt, int nFlags );
	static ITextureInternal *CreateRenderTarget( int w, int h, ImageFormat fmt, bool bDepth );
	static void Destroy( ITextureInternal *pTexture );

	// GR - named RT interface
	static ITextureInternal *CreateNamedRenderTarget( const char *pRTName, 
		int w, int h, ImageFormat fmt, bool bDepth, bool bClampTexCoords, 
		bool bAutoMipMap );
};

#endif // ITEXTUREINTERNAL_H
