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

#ifndef GL_SHADER_H
#define GL_SHADER_H
#pragma once

#include "quakedef.h"

struct decal_t;

void R_ShaderDrawWorld( void );
void R_ShaderSceneBegin( void );
void R_ShaderSceneEnd( void );
void R_ShaderForceLightmapRebuild( void );
void R_ShaderDrawDecal( decal_t *decal, float *inVerts, int numVerts );
void LoadShaderSystem( void );
bool ShaderSystemLoaded( void );

void MaterialSystem_RegisterLightmapSurfaces( void );

#endif // GL_SHADER_H
