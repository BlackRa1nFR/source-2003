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
// Dynamic lightmap code
//=============================================================================
	   
#include "glquake.h"
#include "gl_lightmap.h"
#include "view.h"
#include "gl_cvars.h"
#include "zone.h"
#include "gl_water.h"
#include "r_local.h"
#include "gl_model_private.h"
#include "bumpvects.h"
#include "gl_matsysiface.h"
#include "DispChain.h"
#include <float.h>
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "materialsystem/imesh.h"
#include "EngineStats.h"
#include "tier0/dbg.h"
#include "tier0/vprof.h"
#include "lightcache.h"

Vector4D blocklights[NUM_BUMP_VECTS+1][ MAX_LIGHTMAP_DIM_INCLUDING_BORDER * MAX_LIGHTMAP_DIM_INCLUDING_BORDER ];

ConVar r_avglightmap("r_avglightmap", "0" );


//-----------------------------------------------------------------------------
// Adds a single dynamic light
//-----------------------------------------------------------------------------
static void AddSingleDynamicLight( dlight_t& dl, int surfID, const Vector& entity_origin )
{
	// NOTE: See version 66 in source control to see the old version

	// If this brush has moved, the planes are still in the same place 
	// (they are part of the world), so the light has to be moved back by the 
	// amount the brush has moved

	// If this is needed, it will have to be computed elsewhere as
	// baseline.origin will change after a save/restore
	Vector local, impact;
	VectorSubtract( dl.origin, entity_origin, impact );

	// Find the perpendicular distance to the surface we're lighting
	float perpDistSq = DotProduct (impact, MSurf_Plane( surfID ).normal) - MSurf_Plane( surfID ).dist;
	if (perpDistSq < DLIGHT_BEHIND_PLANE_DIST)
		return;
	perpDistSq *= perpDistSq;

	// If the perp distance > radius of light, blow it off
	float lightDistSq = dl.radius * dl.radius;
	if (lightDistSq <= perpDistSq)
		return;

	// Spotlight early outs...
	if (dl.m_OuterAngle)
	{
		if (dl.m_OuterAngle < 180.0f)
		{
			// Can't light anything from the rear...
			if (DotProduct(dl.m_Direction, MSurf_Plane( surfID ).normal) >= 0.0f)
				return;
		}
	}

	// Transform the light center point into (u,v) space of the lightmap
	mtexinfo_t* tex = MSurf_TexInfo( surfID );
	local[0] = DotProduct (impact, tex->lightmapVecsLuxelsPerWorldUnits[0].AsVector3D()) + 
			   tex->lightmapVecsLuxelsPerWorldUnits[0][3];
	local[1] = DotProduct (impact, tex->lightmapVecsLuxelsPerWorldUnits[1].AsVector3D()) + 
			   tex->lightmapVecsLuxelsPerWorldUnits[1][3];

	// Now put the center points into the space of the lightmap rectangle
	// defined by the lightmapMins + lightmapExtents
	local[0] -= MSurf_LightmapMins( surfID )[0];
	local[1] -= MSurf_LightmapMins( surfID )[1];
	
	// Figure out the quadratic attenuation factor...
	Vector intensity;
	float lightStyleValue = LightStyleValue( dl.style );
	intensity[0] = TexLightToLinear( dl.color.r, dl.color.exponent ) * lightStyleValue;
	intensity[1] = TexLightToLinear( dl.color.g, dl.color.exponent ) * lightStyleValue;
	intensity[2] = TexLightToLinear( dl.color.b, dl.color.exponent ) * lightStyleValue;

	float minlight = max( MIN_LIGHTING_VALUE, dl.minlight );
	float ooQuadraticAttn = lightDistSq * minlight;
	float ooRadiusSq = 1.0f / lightDistSq;

	// Compute a color at each luxel
	// We want to know the square distance from luxel center to light
	// so we can compute an 1/r^2 falloff in light color
	int smax = MSurf_LightmapExtents( surfID )[0] + 1;
	int tmax = MSurf_LightmapExtents( surfID )[1] + 1;
	for (int t=0; t<tmax; ++t)
	{
		float td = (local[1] - t) * tex->worldUnitsPerLuxel;
		
		for (int s=0; s<smax; ++s)
		{
			float sd = (local[0] - s) * tex->worldUnitsPerLuxel;

			float inPlaneDistSq = sd * sd + td * td;
			float totalDistSq = inPlaneDistSq + perpDistSq;
			if (totalDistSq < lightDistSq)
			{
				// at least all floating point only happens when a luxel is lit.
				float scale = (totalDistSq != 0.0f) ? ooQuadraticAttn / totalDistSq : 1.0f;

				// Apply a little extra attenuation
				scale *= (1.0f - totalDistSq * ooRadiusSq);

				if (scale > 2.0f)
					scale = 2.0f;

				int idx = t*smax + s;

				// Compute the base lighting just as is done in the non-bump case...
				blocklights[0][idx][0] += scale * intensity[0];
				blocklights[0][idx][1] += scale * intensity[1];
				blocklights[0][idx][2] += scale * intensity[2];
			}
		}
	}
}												


//-----------------------------------------------------------------------------
// Adds a dynamic light to the bumped lighting
//-----------------------------------------------------------------------------
static void AddSingleDynamicLightToBumpLighting( dlight_t& dl, int surfID, 
	const Vector& entity_origin, Vector* pBumpBasis, const Vector& luxelBasePosition )
{
	Vector		local, impact;

	// If this brush has moved, the planes are still in the same place (they are part of the world), so the light has to 
	// be moved back by the amount the brush has moved
	// If this is needed, it will have to be computed elsewhere as baseline.origin 
	// will change after a save/restore

	VectorSubtract( dl.origin, entity_origin, impact );

	// NOTE: Dist can be negative because muzzle flashes can actually get behind walls
	// since the gun isn't checked for collision tests.
	float perpDistSq = DotProduct (impact, MSurf_Plane( surfID ).normal) - MSurf_Plane( surfID ).dist;
	if (perpDistSq < DLIGHT_BEHIND_PLANE_DIST)
		return;

	perpDistSq *= perpDistSq;

	// If the perp distance > radius of light, blow it off
	float lightDistSq = dl.radius * dl.radius;
	if (lightDistSq <= perpDistSq)
		return;

	// FIXME: For now, only elights can be spotlights
	// the lightmap computation could get expensive for spotlights...
	Assert( dl.m_OuterAngle == 0.0f );

	// Transform the light center point into (u,v) space of the lightmap
	mtexinfo_t *pTexInfo = MSurf_TexInfo( surfID );
	local[0] = DotProduct (impact, pTexInfo->lightmapVecsLuxelsPerWorldUnits[0].AsVector3D()) + 
			   pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][3];
	local[1] = DotProduct (impact, pTexInfo->lightmapVecsLuxelsPerWorldUnits[1].AsVector3D()) + 
			   pTexInfo->lightmapVecsLuxelsPerWorldUnits[1][3];

	// Now put the center points into the space of the lightmap rectangle
	// defined by the lightmapMins + lightmapExtents
	local[0] -= MSurf_LightmapMins( surfID )[0];
	local[1] -= MSurf_LightmapMins( surfID )[1];

	// Figure out the quadratic attenuation factor...
	Vector intensity;
	float lightStyleValue = LightStyleValue( dl.style );
	intensity[0] = TexLightToLinear( dl.color.r, dl.color.exponent ) * lightStyleValue;
	intensity[1] = TexLightToLinear( dl.color.g, dl.color.exponent ) * lightStyleValue;
	intensity[2] = TexLightToLinear( dl.color.b, dl.color.exponent ) * lightStyleValue;

	float minlight = max( MIN_LIGHTING_VALUE, dl.minlight );
	float ooQuadraticAttn = lightDistSq * minlight;
	float ooRadiusSq = 1.0f / lightDistSq;

	// The algorithm here is necessary to make dynamic lights live in the
	// same world as the non-bumped dynamic lights. Therefore, we compute
	// the intensity of the flat lightmap the exact same way here as when
	// we've got a non-bumped surface.

	// Then, I compute an actual light direction vector per luxel (FIXME: !!expensive!!)
	// and compute what light would have to come in along that direction
	// in order to produce the same illumination on the flat lightmap. That's
	// computed by dividing the flat lightmap color by n dot l.
	Vector lightDirection, texelWorldPosition;
	bool useLightDirection = (dl.m_OuterAngle != 0.0f) &&
		(fabs(dl.m_Direction.LengthSqr() - 1.0f) < 1e-3);
	if (useLightDirection)
		VectorMultiply( dl.m_Direction, -1.0f, lightDirection );

	// Since there's a scale factor used when going from world to luxel,
	// we gotta undo that scale factor when going from luxel to world
	float fixupFactor = pTexInfo->worldUnitsPerLuxel * pTexInfo->worldUnitsPerLuxel;

	// Compute a color at each luxel
	// We want to know the square distance from luxel center to light
	// so we can compute an 1/r^2 falloff in light color
	int smax = MSurf_LightmapExtents( surfID )[0] + 1;
	int tmax = MSurf_LightmapExtents( surfID )[1] + 1;
	for (int t=0; t<tmax; ++t)
	{
		float td = (local[1] - t) * pTexInfo->worldUnitsPerLuxel;
		
		// Move along the v direction
		VectorMA( luxelBasePosition, t * fixupFactor, pTexInfo->lightmapVecsLuxelsPerWorldUnits[1].AsVector3D(), 
			texelWorldPosition );

		for (int s=0; s<smax; ++s)
		{
			float sd = (local[0] - s) * pTexInfo->worldUnitsPerLuxel;

			float inPlaneDistSq = sd * sd + td * td;
			float totalDistSq = inPlaneDistSq + perpDistSq;

			if (totalDistSq < lightDistSq)
			{
				// at least all floating point only happens when a luxel is lit.
				float scale = (totalDistSq != 0.0f) ? ooQuadraticAttn / totalDistSq : 1.0f;

				// Apply a little extra attenuation
				scale *= (1.0f - totalDistSq * ooRadiusSq);

				if (scale > 2.0f)
					scale = 2.0f;

				int idx = t*smax + s;

				// Compute the base lighting just as is done in the non-bump case...
				VectorMA( blocklights[0][idx].AsVector3D(), scale, intensity, blocklights[0][idx].AsVector3D() );

				if (!useLightDirection)
				{
					VectorSubtract( impact, texelWorldPosition, lightDirection );
					VectorNormalize( lightDirection );
				}
				
				float lDotN = DotProduct( lightDirection, MSurf_Plane( surfID ).normal );
				if (lDotN < 1e-3)
					lDotN = 1e-3;
				scale /= lDotN;

				int i;
				for( i = 1; i < NUM_BUMP_VECTS + 1; i++ )
				{
					float dot = DotProduct( lightDirection, pBumpBasis[i-1] );
					if( dot <= 0.0f )
						continue;
					
					VectorMA( blocklights[i][idx].AsVector3D(), dot * scale, intensity, 
						blocklights[i][idx].AsVector3D() );
				}
			}
		}

		// Move along u
		VectorMA( texelWorldPosition, fixupFactor, 
			pTexInfo->lightmapVecsLuxelsPerWorldUnits[0].AsVector3D(), texelWorldPosition );

	}
}


//-----------------------------------------------------------------------------
// Compute the bumpmap basis for this surface
//-----------------------------------------------------------------------------
static void R_ComputeSurfaceBasis( int surfID, Vector *pBumpNormals, Vector &luxelBasePosition )
{
	// Get the bump basis vects in the space of the surface.
	Vector sVect, tVect;
	VectorCopy( MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[0].AsVector3D(), sVect );
	VectorNormalize( sVect );
	VectorCopy( MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[1].AsVector3D(), tVect );
	VectorNormalize( tVect );
	GetBumpNormals( sVect, tVect, MSurf_Plane( surfID ).normal, MSurf_Plane( surfID ).normal, pBumpNormals );

	// Compute the location of the first luxel in worldspace

	// Since there's a scale factor used when going from world to luxel,
	// we gotta undo that scale factor when going from luxel to world
	float fixupFactor = 
		MSurf_TexInfo( surfID )->worldUnitsPerLuxel * 
		MSurf_TexInfo( surfID )->worldUnitsPerLuxel;

	// The starting u of the surface is surf->lightmapMins[0];
	// since N * P + D = u, N * P = u - D, therefore we gotta move (u-D) along uvec
	VectorMultiply( MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[0].AsVector3D(),
		(MSurf_LightmapMins( surfID )[0] - MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[0][3]) * fixupFactor,
		luxelBasePosition );

	// Do the same thing for the v direction.
	VectorMA( luxelBasePosition, 
		(MSurf_LightmapMins( surfID )[1] - 
		MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[1][3]) * fixupFactor,
		MSurf_TexInfo( surfID )->lightmapVecsLuxelsPerWorldUnits[1].AsVector3D(),
		luxelBasePosition );

	// Move out in the direction of the plane normal...
	VectorMA( luxelBasePosition, MSurf_Plane( surfID ).dist, MSurf_Plane( surfID ).normal, luxelBasePosition ); 
}


//-----------------------------------------------------------------------------
// Purpose: add dynamic lights to this surface's lightmap
// Input  : *surf - 
//-----------------------------------------------------------------------------
void R_AddDynamicLights( int surfID, const Vector& entity_origin, bool needsBumpmap )
{
	ASSERT_SURF_VALID( surfID );
	Vector bumpNormals[3];
	bool computedBumpBasis = false;
	Vector luxelBasePosition;

	// Displacements do dynamic lights different
	if( SurfaceHasDispInfo( surfID ) )
	{
		MSurf_DispInfo( surfID )->AddDynamicLights( blocklights[0] );
		return;
	}

	int	lnum;
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		// If the light's not active, then continue
		if ( (r_dlightactive & (1 << lnum)) == 0 )
			continue;

		// not lit by this light
		if ( !(MSurf_DLightBits( surfID ) & (1<<lnum) ) )
			continue;

		// This light doesn't affect the world
		if ( cl_dlights[lnum].flags & DLIGHT_NO_WORLD_ILLUMINATION)
			continue;

		// We ignore displacement lights here; this is for brushes only
		if ( cl_dlights[lnum].flags & DLIGHT_DISPLACEMENT_MASK )
			continue;

		if (!needsBumpmap)
		{
			AddSingleDynamicLight( cl_dlights[lnum], surfID, entity_origin );
			continue;
		}

		// Here, I'm precomputing things needed by bumped lighting that
		// are the same for a surface...
		if (!computedBumpBasis)
		{
			R_ComputeSurfaceBasis( surfID, bumpNormals, luxelBasePosition );
			computedBumpBasis = true;
		}

		AddSingleDynamicLightToBumpLighting( cl_dlights[lnum], surfID, 
			entity_origin, bumpNormals, luxelBasePosition );
	}
}


// Fixed point (8.8) color/intensity ratios
#define I_RED		((int)(0.299*255))
#define I_GREEN		((int)(0.587*255))
#define I_BLUE		((int)(0.114*255))


//-----------------------------------------------------------------------------
// Sets all elements in a lightmap to a particular opaque greyscale value
//-----------------------------------------------------------------------------
static void InitLMSamples( Vector4D *pSamples, int nSamples, float value )
{
	for( int i=0; i < nSamples; i++ )
	{
		pSamples[i][0] = pSamples[i][1] = pSamples[i][2] = value;
		pSamples[i][3] = 1.0f;
	}
}



//-----------------------------------------------------------------------------
// Computes the lightmap size
//-----------------------------------------------------------------------------
static int ComputeLightmapSize( int surfID )
{
	int smax = ( MSurf_LightmapExtents( surfID )[0] ) + 1;
	int tmax = ( MSurf_LightmapExtents( surfID )[1] ) + 1;
	int size = smax * tmax;

	int nMaxSize = MSurf_MaxLightmapSizeWithBorder( surfID );
	if (size > nMaxSize * nMaxSize)
	{
		Con_Printf("Bad lightmap extents on material \"%s\"\n", 
			materialSortInfoArray[MSurf_MaterialSortID( surfID )].material->GetName());
		return 0;
	}
	
	return size;
}


//-----------------------------------------------------------------------------
// Compute the portion of the lightmap generated from lightstyles
//-----------------------------------------------------------------------------
static void AccumulateLightstyles( colorRGBExp32* pLightmap, int lightmapSize, float scalar ) 
{
	Assert( pLightmap );
	for (int i=0; i<lightmapSize ; ++i)
	{
		blocklights[0][i][0] += scalar * TexLightToLinear( pLightmap[i].r, pLightmap[i].exponent );
		blocklights[0][i][1] += scalar * TexLightToLinear( pLightmap[i].g, pLightmap[i].exponent );
		blocklights[0][i][2] += scalar * TexLightToLinear( pLightmap[i].b, pLightmap[i].exponent );
	}
}

static void AccumulateLightstylesFlat( colorRGBExp32* pLightmap, int lightmapSize, float scalar ) 
{
	Assert( pLightmap );
	for (int i=0; i<lightmapSize ; ++i)
	{
		blocklights[0][i][0] += scalar * TexLightToLinear( pLightmap->r, pLightmap->exponent );
		blocklights[0][i][1] += scalar * TexLightToLinear( pLightmap->g, pLightmap->exponent );
		blocklights[0][i][2] += scalar * TexLightToLinear( pLightmap->b, pLightmap->exponent );
	}
}

static void AccumulateBumpedLightstyles( colorRGBExp32* pLightmap, int lightmapSize, float scalar ) 
{
	colorRGBExp32 *pBumpedLightmaps[3];
	pBumpedLightmaps[0] = pLightmap + lightmapSize;
	pBumpedLightmaps[1] = pLightmap + 2 * lightmapSize;
	pBumpedLightmaps[2] = pLightmap + 3 * lightmapSize;

	// I chose to split up the loops this way because it was the best tradeoff
	// based on profiles between cache miss + loop overhead
	for (int i=0 ; i<lightmapSize ; ++i)
	{
		blocklights[0][i][0] += scalar * TexLightToLinear( pLightmap[i].r, pLightmap[i].exponent );
		blocklights[0][i][1] += scalar * TexLightToLinear( pLightmap[i].g, pLightmap[i].exponent );
		blocklights[0][i][2] += scalar * TexLightToLinear( pLightmap[i].b, pLightmap[i].exponent );

		blocklights[1][i][0] += scalar * TexLightToLinear( pBumpedLightmaps[0][i].r, pBumpedLightmaps[0][i].exponent );
		blocklights[1][i][1] += scalar * TexLightToLinear( pBumpedLightmaps[0][i].g, pBumpedLightmaps[0][i].exponent );
		blocklights[1][i][2] += scalar * TexLightToLinear( pBumpedLightmaps[0][i].b, pBumpedLightmaps[0][i].exponent );
	}
	for ( i=0 ; i<lightmapSize ; ++i)
	{
		blocklights[2][i][0] += scalar * TexLightToLinear( pBumpedLightmaps[1][i].r, pBumpedLightmaps[1][i].exponent );
		blocklights[2][i][1] += scalar * TexLightToLinear( pBumpedLightmaps[1][i].g, pBumpedLightmaps[1][i].exponent );
		blocklights[2][i][2] += scalar * TexLightToLinear( pBumpedLightmaps[1][i].b, pBumpedLightmaps[1][i].exponent );

		blocklights[3][i][0] += scalar * TexLightToLinear( pBumpedLightmaps[2][i].r, pBumpedLightmaps[2][i].exponent );
		blocklights[3][i][1] += scalar * TexLightToLinear( pBumpedLightmaps[2][i].g, pBumpedLightmaps[2][i].exponent );
		blocklights[3][i][2] += scalar * TexLightToLinear( pBumpedLightmaps[2][i].b, pBumpedLightmaps[2][i].exponent );
	}
}


//-----------------------------------------------------------------------------
// Compute the portion of the lightmap generated from lightstyles
//-----------------------------------------------------------------------------
static void ComputeLightmapFromLightstyle( msurfacelighting_t *pLighting, bool computeLightmap, 
				bool computeBumpmap, int lightmapSize, bool hasBumpmapLightmapData )
{
	colorRGBExp32 *pLightmap = pLighting->m_pSamples;

	// Compute iteration range
	int minmap, maxmap;
#ifdef USE_CONVARS
	if( r_lightmap.GetInt() != -1 )
	{
		minmap = r_lightmap.GetInt();
		maxmap = minmap + 1;
	}
	else
#endif
	{
		minmap = 0; maxmap = MAXLIGHTMAPS;
	}

	for (int maps = minmap; maps < maxmap && pLighting->m_nStyles[maps] != 255; ++maps)
	{
#ifdef _DEBUG
		if( r_lightstyle.GetInt() != -1 && pLighting->m_nStyles[maps] != r_lightstyle.GetInt())
		{
			continue;
		}
#endif

		float scalar = LightStyleValue( pLighting->m_nStyles[maps] );
		if (scalar != 0.0f)
		{
			if( computeBumpmap )
			{
				AccumulateBumpedLightstyles( pLightmap, lightmapSize, scalar );
			}
			else if( computeLightmap )
			{
#ifdef USE_CONVARS
				if (r_avglightmap.GetInt())
				{
					pLightmap = pLighting->AvgLightColor(maps);
					AccumulateLightstylesFlat( pLightmap, lightmapSize, scalar );
				}
				else
				{
					AccumulateLightstyles( pLightmap, lightmapSize, scalar );
				}
#else
				pLightmap = pLighting->AvgLightColor(maps); 
				AccumulateLightstylesFlat( pLightmap, lightmapSize, scalar );
#endif
			}
		}

		// skip to next lightmap. If we store lightmap data, we need to jump forward 4
		pLightmap += hasBumpmapLightmapData ? lightmapSize * ( NUM_BUMP_VECTS + 1 ) : lightmapSize;
	}
}

//-----------------------------------------------------------------------------
// Update the lightmaps...
//-----------------------------------------------------------------------------

static void UpdateLightmapTextures( int surfID, bool needsBumpmap )
{
	ASSERT_SURF_VALID( surfID );
	// Displacement surfaces can change lightmap alpha for their shaders.
	if( SurfaceHasDispInfo( surfID ) )
	{
		int w, h;
		materialSystemInterface->GetLightmapPageSize( materialSortInfoArray[MSurf_MaterialSortID( surfID )].lightmapPageID, &w, &h );
		MSurf_DispInfo( surfID )->UpdateLightmapAlpha( &blocklights[0][0], sizeof(blocklights[0])/sizeof(blocklights[0][0]) );
	}

	if( materialSortInfoArray )
	{
		int lightmapSize[2];
		int offsetIntoLightmapPage[2];
		lightmapSize[0] = ( MSurf_LightmapExtents( surfID )[0] ) + 1;
		lightmapSize[1] = ( MSurf_LightmapExtents( surfID )[1] ) + 1;
		offsetIntoLightmapPage[0] = MSurf_OffsetIntoLightmapPage( surfID )[0];
		offsetIntoLightmapPage[1] = MSurf_OffsetIntoLightmapPage( surfID )[1];
		Assert( MSurf_MaterialSortID( surfID ) >= 0 && 
			MSurf_MaterialSortID( surfID ) < g_NumMaterialSortBins );
		// FIXME: Should differentiate between bumped and unbumped since the perf characteristics
		// are completely different?
		g_EngineStats.IncrementCountedStat( ENGINE_STATS_LIGHTMAP_LUXEL_UPDATES, lightmapSize[0] * lightmapSize[1] );
		if( needsBumpmap )
		{
			materialSystemInterface->UpdateLightmap( materialSortInfoArray[MSurf_MaterialSortID( surfID )].lightmapPageID,
				lightmapSize, offsetIntoLightmapPage, 
				&blocklights[0][0][0], &blocklights[1][0][0], &blocklights[2][0][0], &blocklights[3][0][0] );
		}
		else
		{
			materialSystemInterface->UpdateLightmap( materialSortInfoArray[MSurf_MaterialSortID( surfID )].lightmapPageID,
				lightmapSize, offsetIntoLightmapPage, 
				&blocklights[0][0][0], NULL, NULL, NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Build the blocklights array for a given surface and copy to dest
//			Combine and scale multiple lightmaps into the 8.8 format in blocklights
// Input  : *psurf - surface to rebuild
//			*dest - texture pointer to receive copy in lightmap texture format
//			stride - stride of *dest memory
//-----------------------------------------------------------------------------
void R_BuildLightMap( int surfID, const Vector& entity_origin )
{
	MEASURE_TIMED_STAT( ENGINE_STATS_DYNAMIC_LIGHTMAP_BUILD );

	int bumpID;

	bool needsBumpmap = SurfNeedsBumpedLightmaps( surfID );
	bool needsLightmap = SurfNeedsLightmap( surfID );

	if( !needsBumpmap && !needsLightmap )
	{
		return;
	}
	
	bool hasBumpmap = SurfHasBumpedLightmaps( surfID );
	bool hasLightmap = SurfHasLightmap( surfID );

	if( materialSortInfoArray )
	{
		Assert( MSurf_MaterialSortID( surfID ) >= 0 && 
			    MSurf_MaterialSortID( surfID )  < g_NumMaterialSortBins );
		// hack - need to give -1 (white texture) a name
		if( materialSortInfoArray[MSurf_MaterialSortID( surfID )].lightmapPageID == -1 )
		{
			return;
		}
	}
	
	// Mark the surface with the particular cached light values...
	msurfacelighting_t *pLighting = SurfaceLighting( surfID );

	// Retire dlights that are no longer active
	pLighting->m_fDLightBits &= r_dlightactive;
	pLighting->m_nLastComputedFrame = r_framecount;

	int size = ComputeLightmapSize( surfID );
	if (size == 0)
		return;

	// clear to no light
	if( needsLightmap )
	{
		// set to full bright if no light data
		InitLMSamples( blocklights[0], size, hasLightmap ? 0.0f : 1.0f );
	}

	if( needsBumpmap && hasBumpmap )
	{
		// set to full bright if no light data
		for( bumpID = 1; bumpID < NUM_BUMP_VECTS + 1; bumpID++ )
		{
			InitLMSamples( blocklights[bumpID], size, hasBumpmap ? 0.0f : 1.0f );
		}
	}

	// add all the lightmaps
	// Here, it's got the data it needs. So use it!
	if( ( hasLightmap && needsLightmap ) || ( hasBumpmap && needsBumpmap ) )
	{
		ComputeLightmapFromLightstyle( pLighting, ( hasLightmap && needsLightmap ),
			( hasBumpmap && needsBumpmap ), size, hasBumpmap );
	}
	else if( !hasBumpmap && needsBumpmap && hasLightmap )
	{
		// make something up for the bumped lights if you need them but don't have the data
		// if you have a lightmap, use that, otherwise fullbright
		ComputeLightmapFromLightstyle( pLighting, true, false, size, hasBumpmap );

		for( bumpID = 0; bumpID < ( hasBumpmap ? ( NUM_BUMP_VECTS + 1 ) : 1 ); bumpID++ )
		{
			for (int i=0 ; i<size ; i++)
			{
				blocklights[bumpID][i].AsVector3D() = blocklights[0][i].AsVector3D();
			}
		}
	}
	else if( needsBumpmap && !hasLightmap )
	{
		// set to full bright if no light data
		InitLMSamples( blocklights[1], size, 0.0f );
		InitLMSamples( blocklights[2], size, 0.0f );
		InitLMSamples( blocklights[3], size, 0.0f );
	}
	else if( !needsBumpmap && !needsLightmap )
	{
	}
	else if( needsLightmap && !hasLightmap )
	{
	}
	else
	{
		Assert( 0 );
	}

	// add all the dynamic lights
	if( ( needsLightmap || needsBumpmap ) && ( pLighting->m_nDLightFrame == r_framecount ) )
	{
		R_AddDynamicLights ( surfID, entity_origin, needsBumpmap );
	}

	// Update the texture state
	UpdateLightmapTextures( surfID, needsBumpmap );
}


void R_RedownloadAllLightmaps( const Vector& entity_origin )
{
#ifdef _DEBUG
	static bool initializedBlockLights = false;
	if (!initializedBlockLights)
	{
		memset( &blocklights[0][0][0], 0, MAX_LIGHTMAP_DIM_INCLUDING_BORDER * MAX_LIGHTMAP_DIM_INCLUDING_BORDER * (NUM_BUMP_VECTS + 1) * sizeof( Vector ) );
		initializedBlockLights = true;
	}
#endif

	int surfID;

	double st = Sys_FloatTime();

	for( surfID = 0; surfID < host_state.worldmodel->brush.numsurfaces; surfID++ )
	{
		ASSERT_SURF_VALID( surfID );
		R_BuildLightMap( surfID, entity_origin );
	}

	float elapsed = ( float )( Sys_FloatTime() - st ) * 1000.0;
	if ( elapsed > 1000 )
	{
		Con_Printf( "R_RedownloadAllLightmaps took %.3f msec!\n", elapsed );
	}

	g_RebuildLightmaps = false;
}

//-----------------------------------------------------------------------------
// Purpose: flag the lightmaps as needing to be rebuilt (gamma change)
//-----------------------------------------------------------------------------
qboolean g_RebuildLightmaps = false;

void GL_RebuildLightmaps( void )
{
	g_RebuildLightmaps = true;
}


//-----------------------------------------------------------------------------
// Purpose: Update the in-RAM texture for the given surface's lightmap
// Input  : *fa - surface pointer
//-----------------------------------------------------------------------------

// Only enable this if you are testing lightstyle performance.
//#define UPDATE_LIGHTSTYLES_EVERY_FRAME

void FASTCALL R_RenderDynamicLightmaps ( int surfID, const Vector& entity_origin )
{
	VPROF_BUDGET( "R_RenderDynamicLightmaps", VPROF_BUDGETGROUP_DLIGHT_RENDERING );
	ASSERT_SURF_VALID( surfID );

	int fSurfFlags = MSurf_Flags( surfID );

	if( fSurfFlags & SURFDRAW_NOLIGHT )
		return;

	// check for lightmap modification
	bool bChanged = false;
	msurfacelighting_t *pLighting = SurfaceLighting( surfID );
	if( fSurfFlags & SURFDRAW_HASLIGHTSYTLES )
	{
#ifdef UPDATE_LIGHTSTYLES_EVERY_FRAME
		if( pLighting->m_nStyles[0] != 0 || pLighting->m_nStyles[1] != 255 )
		{
			bChanged = true;
		}
#endif
		for( int maps = 0; maps < MAXLIGHTMAPS && pLighting->m_nStyles[maps] != 255; maps++ )
		{
			if( d_lightstyleframe[pLighting->m_nStyles[maps]] > pLighting->m_nLastComputedFrame )
			{
				bChanged = true;
				break;
			}
		}
	}
	// was it dynamic this frame (pLighting->m_nDLightFrame == r_framecount) 
	// or dynamic previously (pLighting->m_fDLightBits)
	if ( bChanged || ( pLighting->m_nDLightFrame == r_framecount ) || pLighting->m_fDLightBits )
	{
#ifdef USE_CONVARS
		if( r_dynamic.GetInt() )
#endif
		{
			R_BuildLightMap( surfID, entity_origin );
		}
	}
}
