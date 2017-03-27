// disable data conversion warnings

#ifndef GLQUAKE_H
#define GLQUAKE_H
#pragma once

#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#include "basetypes.h"
#include "quakedef.h"
#include "render.h"
#include "client.h"
#include "materialsystem/imaterialvar.h"
#include "bspfile.h"

void Shader_BeginRendering ();
void Shader_SwapBuffers( void );

extern	Vector		modelorg;

//
// view origin
//
extern	Vector	vup;
extern	Vector	vpn;
extern	Vector	vright;
extern	Vector	r_origin;

//
// screen size info
//
class	IMaterial;
extern	IMaterial*	g_materialEmpty;

extern	IMaterial*	g_materialWireframe;

extern	IMaterial*	g_materialTranslucentSingleColor;
extern	IMaterial*	g_materialTranslucentVertexColor;

extern	IMaterial*	g_materialWorldWireframe;
extern IMaterial*	g_materialWorldWireframeZBuffer;
extern	IMaterial*	g_materialBrushWireframe;
extern	IMaterial*	g_materialDecalWireframe;
extern	IMaterial*	g_materialDebugLightmap;
extern	IMaterial*	g_materialDebugLightmapZBuffer;
extern	IMaterial*	g_materialDebugLuxels;
extern	IMaterial*	g_materialLeafVisWireframe;
extern	IMaterial*	g_materialVolumetricFog;
extern	IMaterial*	g_pMaterialWireframeVertexColor;
extern	IMaterial*	g_pMaterialWireframeVertexColorIgnoreZ;
extern	IMaterial*  g_pMaterialLightSprite;
extern	IMaterial*  g_pMaterialShadowBuild;
extern	IMaterial*  g_pMaterialMRMWireframe;
extern	IMaterial*  g_pMaterialWriteZ;

extern	ConVar	r_norefresh;
extern	ConVar	r_speeds;
extern	ConVar	r_speedsquiet;
extern  ConVar	r_lightmapcolorscale;
extern	ConVar	r_decals;
extern	ConVar	mp_decals;
extern	ConVar	r_lightmap;
extern	ConVar	r_lightstyle;
extern	ConVar	r_dynamic;

extern  ConVar  r_lod_noupdate;

extern	ConVar	mat_fullbright;
extern	ConVar	mat_drawflat;
extern	ConVar	mat_reversedepth;
extern  ConVar	mat_norendering;


#endif // GLQUAKE_H
