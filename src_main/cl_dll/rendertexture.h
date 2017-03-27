//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// Define local cl_dll hooks to the renderable textures in material system
//
//=============================================================================

#ifndef RENDERTARGETS_H
#define RENDERTARGETS_H

ITexture *GetPowerOfTwoFrameBufferTexture( void );
ITexture *GetFullFrameFrameBufferTexture( void );
ITexture *GetWaterReflectionTexture( void );
ITexture *GetWaterRefractionTexture( void );
ITexture *GetCameraTexture( void );
ITexture *GetSmallBufferHDR0( void );
ITexture *GetSmallBufferHDR1( void );

#endif // RENDERTARGETS_H
