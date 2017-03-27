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

#ifndef COLORFORMATDX8_H
#define COLORFORMATDX8_H

#include <PixelWriter.h>

//-----------------------------------------------------------------------------
// convert back and forth from D3D format to ImageFormat, regardless of
// whether it's supported or not
//-----------------------------------------------------------------------------
ImageFormat D3DFormatToImageFormat( D3DFORMAT format );
D3DFORMAT ImageFormatToD3DFormat( ImageFormat format );


//-----------------------------------------------------------------------------
// Initializes the color format informat; call it every time display mode changes
//-----------------------------------------------------------------------------
void InitializeColorInformation( UINT displayAdapter, D3DDEVTYPE deviceType, 
								 ImageFormat displayFormat );

//-----------------------------------------------------------------------------
// Returns true if compressed textures are supported
//-----------------------------------------------------------------------------
bool D3DSupportsCompressedTextures();

//-----------------------------------------------------------------------------
// Returns closest supported format
//-----------------------------------------------------------------------------
ImageFormat FindNearestSupportedFormat( ImageFormat format );
ImageFormat FindNearestRenderTargetFormat( ImageFormat format );

//-----------------------------------------------------------------------------
// Returns true if the hardware wants 16 bit textures.
//-----------------------------------------------------------------------------
bool Prefer16BitTextures();

//-----------------------------------------------------------------------------
// Finds the nearest supported depth buffer format
//-----------------------------------------------------------------------------
D3DFORMAT FindNearestSupportedDepthFormat(D3DFORMAT depthFormat, D3DFORMAT renderTargetFormat = (D3DFORMAT)-1);

#endif // COLORFORMATDX8_H