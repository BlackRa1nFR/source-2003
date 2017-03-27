//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LIGHTCACHE_H
#define LIGHTCACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"

#define MAXLOCALLIGHTS 4

class IHandleEntity;
struct dworldlight_t;												   
FORWARD_DECLARE_HANDLE( LightCacheHandle_t ); 

struct LightingState_t
{
	Vector		r_boxcolor[6];
	Vector		totalboxcolor[6];
	int			numlights;
	dworldlight_t *locallight[MAXLOCALLIGHTS];
};

enum 
{
	LIGHTCACHEFLAGS_STATIC		= 0x1,
	LIGHTCACHEFLAGS_DYNAMIC		= 0x2,
	LIGHTCACHEFLAGS_LIGHTSTYLE	= 0x4,
};

#define MIN_LIGHTING_VALUE	(20.0/256.0f)

class ITexture;

ITexture *LightcacheGet( LightCacheHandle_t cache, LightingState_t& lightingState,
				    unsigned int flags = ( LIGHTCACHEFLAGS_STATIC | 
						                   LIGHTCACHEFLAGS_DYNAMIC | 
					                       LIGHTCACHEFLAGS_LIGHTSTYLE ) );
ITexture *LightcacheGet( const Vector& origin, LightingState_t& lightingState );

// Reset the light cache.
void R_StudioInitLightingCache( void );

// Compute the comtribution of D- and E- lights at a point + normal
void ComputeDynamicLighting( const Vector& pt, const Vector* pNormal, Vector& color );

// Computes an average color (of sorts) at a particular point + optional normal
void ComputeLighting( const Vector& pt, const Vector* pNormal, bool bClamp, Vector& color );

// Finds ambient lights
dworldlight_t* FindAmbientLight();

// Precache lighting
LightCacheHandle_t CreateStaticLightingCache( const Vector& origin );
void ClearStaticLightingCache();

// Computes the static vertex lighting term from a large number of spherical samples
bool ComputeVertexLightingFromSphericalSamples( const Vector& vecVertex, 
	const Vector &vecNormal, IHandleEntity *pIgnoreEnt, Vector *pLinearColor );

#endif // LIGHTCACHE_H
