//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
// Renderable textures declaration and creation related stuff
//
//=============================================================================
#ifndef MATRENDERTEXTURE_H
#define MATRENDERTEXTURE_H

// RT creation callback
typedef ITexture *(*RenderTextureCreateFunc_t)( );

typedef struct
{
	char rtName[50];
	RenderTextureCreateFunc_t rtCreator;
} RenderTextureDescriptor_t;

// List of RT descriptors
extern RenderTextureDescriptor_t rtDescriptors[];

#endif // MATRENDERTEXTURE_H
