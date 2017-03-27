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
//=============================================================================

#ifndef GL_CVARS_H
#define GL_CVARS_H

#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

// Stuff that's dealt with by the material system
extern  ConVar	mat_loadtextures;	// Can help load levels quickly for debugging.
extern	ConVar	mat_polyoffset;		// Offset value for decals glPolyOffset(value)
extern	ConVar	mat_overbright;		// Use overbright blending?
extern	ConVar	mat_envmapsize;		// Dimensions of square skybox bitmap (in 3D screen shots, not game textures)
extern	ConVar	mat_wireframe;		// Draw the world in wireframe mode
extern	ConVar	mat_luxels;			// Draw lightmaps as checkerboards
extern	ConVar	mat_showlightmappage;// set this to the lightmap page that you want to see on screen
									// set to -1 to show nothing.
extern	ConVar	mat_lightmapsonly;	// set to 1 to draw with g_materialDebugLightmap.
extern	ConVar	mat_normals;		// Draw the world with vertex normals
extern	ConVar	mat_bumpbasis;		// Draw the world with the bump basis vectors drawn
extern	ConVar	mat_leafvis;		// Draw a wireframe of the leaf volume
extern	ConVar	mat_picmip;			// How many mip to I skip for texture scalability.
extern	ConVar	mat_softwarelighting;	// Use softwarelighting?
extern	ConVar	mat_showbadnormals; // Use this to show out of range normal map texels.

extern ConVar  mat_envmaptgasize;
extern ConVar  mat_levelflush;

#endif	//GL_CVARS_H
