//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DECAL_CLIP_H
#define DECAL_CLIP_H
#ifdef _WIN32
#pragma once
#endif


#include "vector.h"
#include "vector2d.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "decal_private.h"
#include "gl_model_private.h"


#define MAX_DECALCLIPVERT		32


class CDecalVert
{
public:
	Vector		m_vPos;

	// These are the texcoords for the decal itself
	Vector2D	m_tCoords;

	// Lightmap texcoords for the decal.
	Vector2D	m_LMCoords;
};


// Clip pOutVerts/nStartVerts into the decal's texture space.
CDecalVert* R_DoDecalSHClip( 
	CDecalVert *pInVerts,
	CDecalVert *pOutVerts, 
	decal_t *pDecal, 
	int nStartVerts, 
	int *pVertCount );

// Generate clipped vertex list for decal pdecal projected onto polygon psurf
CDecalVert* R_DecalVertsClip( 
	CDecalVert *pOutVerts, 
	decal_t *pDecal, 
	int surfID, 
	IMaterial *pMaterial, 
	int *pVertCount );

// Compute the unscaled basis for the decal.
void R_DecalComputeBasis( 
	Vector const& surfaceNormal, 
	Vector const* pSAxis, 
	Vector* textureSpaceBasis );

// Compute the basis for the decal and scale the axes so the whole decal fits
// into the (0,0) - (1,1) range.
void R_SetupDecalTextureSpaceBasis(
	decal_t *pDecal,
	Vector &vSurfNormal,
	IMaterial *pMaterial,
	Vector textureSpaceBasis[3],
	float decalWorldScale[2] );


#endif // DECAL_CLIP_H
