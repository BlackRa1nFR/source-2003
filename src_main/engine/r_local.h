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
// r_local.h -- private refresh defs 
//=============================================================================

#ifndef R_LOCAL_H
#define R_LOCAL_H
#pragma once

#include "gl_model_private.h"

extern	int		r_framecount;

// NOTE: We store all 256 here so that 255 can be accessed easily...
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value
extern	int		d_lightstyleframe[256];	// Frame when the light style value changed
extern ConVar	r_speeds;
extern ConVar	r_speedsquiet;
extern ConVar	r_lightmapcolorscale;
extern ConVar	r_decals;
extern ConVar	mp_decals;
extern ConVar	r_lightmap;
extern ConVar	r_lightstyle;
extern ConVar	mat_fullbright;
extern ConVar	mat_drawflat;
extern ConVar	mat_reversedepth;

extern int		r_dlightframecount;
extern int		r_dlightchanged;	// which ones changed
extern int		r_dlightactive;		// which ones are active

class IClientEntity;

colorVec R_LightPoint (Vector& p);

// returns surfID
int R_LightVec (const Vector& start, const Vector& end, bool bUseLightStyles, Vector& c, float *textureS = NULL, float *textureT = NULL, float *lightmapS = NULL, float *lightmapT = NULL);

// This is to allow us to do R_LightVec on a brush model
void R_LightVecUseModel(model_t* pModel = 0);

void R_InitStudio( void );
void R_LoadSkys (void);
void R_DecalInit ( void );
void R_AnimateLight (void);

void MarkDLightsOnStaticProps( void );

//-----------------------------------------------------------------------------
// Method to get at the light style value
//-----------------------------------------------------------------------------

inline float LightStyleValue( int style )
{
	extern float g_lightmapScale;
	return g_lightmapScale * ( float )d_lightstylevalue[style] / 264.0f;
}

#endif // R_LOCAL_H
