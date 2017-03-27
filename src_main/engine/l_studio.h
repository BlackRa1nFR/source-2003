
//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Export functions from l_studio.cpp
//
// $NoKeywords: $
//=============================================================================

#ifndef L_STUDIO_H
#define L_STUDIO_H

struct LightDesc_t;
class IStudioRender;

extern IStudioRender *g_pStudioRender;

vcollide_t *Mod_VCollide( model_t *pModel );
void UpdateStudioRenderConfig( void );
void InitStudioRender( void );
void ShutdownStudioRender( void );
unsigned short& FirstShadowOnModelInstance( ModelInstanceHandle_t handle );

//-----------------------------------------------------------------------------
// Converts world lights to materialsystem lights
//-----------------------------------------------------------------------------
bool WorldLightToMaterialLight( dworldlight_t* worldlight, LightDesc_t& light );

//-----------------------------------------------------------------------------
// Computes the center of the studio model for illumination purposes
//-----------------------------------------------------------------------------
void R_StudioCenter( studiohdr_t* pStudioHdr, const matrix3x4_t &matrix, Vector& center );

#endif

