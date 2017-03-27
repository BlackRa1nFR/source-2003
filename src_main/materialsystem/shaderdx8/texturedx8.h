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

#ifndef TEXTUREDX8_H
#define TEXTUREDX8_H

#include "ImageLoader.h"
#include "locald3dtypes.h"

class CPixelWriter;


//-----------------------------------------------------------------------------
// Returns the size of texture memory
//-----------------------------------------------------------------------------
int ComputeTextureMemorySize( const GUID &nDeviceId, D3DDEVTYPE deviceType );


//-----------------------------------------------------------------------------
// Texture creation
//-----------------------------------------------------------------------------
IDirect3DBaseTexture *CreateD3DTexture( int width, int height, ImageFormat dstFormat, 
								         int numLevels, int creationFlags );


//-----------------------------------------------------------------------------
// Texture destruction
//-----------------------------------------------------------------------------
void DestroyD3DTexture( IDirect3DBaseTexture *pTex );

int GetD3DTextureRefCount( IDirect3DBaseTexture *pTex );


//-----------------------------------------------------------------------------
// Stats...
//-----------------------------------------------------------------------------
int TextureCount();


//-----------------------------------------------------------------------------
// Texture image upload
//-----------------------------------------------------------------------------

void LoadTexture( int bindId, int copy, IDirect3DBaseTexture*& pTexture, int level, 
				  D3DCUBEMAP_FACES cubeFaceID, ImageFormat dstFormat, 
				  int width, int height, ImageFormat srcFormat, void *pSrcData );

//-----------------------------------------------------------------------------
// Upload to a sub-piece of a texture
//-----------------------------------------------------------------------------

void LoadSubTexture( int bindId, int copy, IDirect3DBaseTexture* pTexture, int level, 
					 D3DCUBEMAP_FACES cubeFaceID, int xOffset, int yOffset, int width, int height, 
					 ImageFormat srcFormat, int srcStride, void *pSrcData );

//-----------------------------------------------------------------------------
// Lock, unlock a texture...
//-----------------------------------------------------------------------------

bool LockTexture( int bindId, int copy, IDirect3DBaseTexture* pTexture, int level, 
	D3DCUBEMAP_FACES cubeFaceID, int xOffset, int yOffset, int width, int height,
	CPixelWriter& writer );

void UnlockTexture( int bindId, int copy, IDirect3DBaseTexture* pTexture, int level, 
	D3DCUBEMAP_FACES cubeFaceID );

#endif // TEXTUREDX8_H