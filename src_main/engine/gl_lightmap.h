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
// $NoKeywords: $
//=============================================================================

#ifndef GL_LIGHTMAP_H
#define GL_LIGHTMAP_H

#ifdef _WIN32
#pragma once
#endif

#define	MAX_LIGHTMAPS	256

// NOTE: This is used for a hack that deals with viewmodels creating dlights 
// behind walls
#define DLIGHT_BEHIND_PLANE_DIST	-15

extern qboolean g_RebuildLightmaps;

extern int r_dlightactive;

void R_AddDynamicLights( int surfID, const Vector& entity_origin );
void R_BuildLightMap( int surfID, const Vector& entity_origin );
void R_RedownloadAllLightmaps( const Vector& entity_origin  );
void GL_RebuildLightmaps( void );
void FASTCALL R_RenderDynamicLightmaps (int surfID, const Vector& entity_origin );
int R_AddLightmapPolyChain( int surfID );
int R_AddLightmapSurfaceChain( int surfID );
void R_SetLightmapBlendingMode( void );

#endif // GL_LIGHTMAP_H
