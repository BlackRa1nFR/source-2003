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

#ifndef GL_RSURF_H
#define GL_RSURF_H
#pragma once

typedef struct model_s model_t;
typedef struct msurface_s msurface_t;

extern void GL_BeginBuildingLightmaps( model_t *m );
extern void GL_EndBuildingLightmaps( void );
extern void GL_CreateSurfaceLightmap( msurface_t *surf );

extern void LM_AddSurface( msurface_t *pSurface );
extern void LM_DrawSurfaces( void );

#endif // GL_RSURF_H
