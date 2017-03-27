//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef GL_TEXTURE_H
#define GL_TEXTURE_H
#pragma once


#define TEX_TYPE_NONE	0
#define TEX_TYPE_ALPHA	1
#define TEX_TYPE_LUM	2
#define TEX_TYPE_ALPHA_GRADIENT 3
#define TEX_TYPE_RGBA	4

#define TEX_IS_ALPHA(type)		((type)==TEX_TYPE_ALPHA||(type)==TEX_TYPE_ALPHA_GRADIENT||(type)==TEX_TYPE_RGBA)

IMaterial	*GL_LoadMaterial( const char *pName );
void		GL_UnloadMaterial( IMaterial *pMaterial );



#endif // GL_TEXTURE_H
