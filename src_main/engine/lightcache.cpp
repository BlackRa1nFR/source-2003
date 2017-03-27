//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "lightcache.h"
#include "cmodel_engine.h"
#include "istudiorender.h"
#include "enginestats.h"
#include "studio_internal.h"
#include "bspfile.h"
#include "cdll_engine_int.h"
#include "r_local.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "l_studio.h"
#include "debugoverlay.h"
#include <float.h>
#include "worldsize.h"
#include "ispatialpartitioninternal.h"
#include "staticpropmgr.h"
#include "cmodel_engine.h"
#include "icliententitylist.h"
#include "icliententity.h"
#include "enginetrace.h"


// this should be prime to make the hash better
#define MAX_CACHE_ENTRY		3709
#define MAX_CACHE_BUCKETS	MAX_CACHE_ENTRY
#define FRAME_CACHE_COUNT	32

float Engine_WorldLightDistanceFalloff( const dworldlight_t *wl, const Vector& delta, bool bNoRadiusCheck = false );
float Engine_WorldLightAngle( const dworldlight_t *wl, const Vector& lnormal, const Vector& snormal, const Vector& delta );
void RecomputeStaticLighting();

#define MAX_LIGHTSTYLE_BITS	 ( (MAX_LIGHTSTYLES + 0x7) >> 3 )
#define MAX_LIGHTSTYLE_BYTES ( MAX_LIGHTSTYLE_BITS << 3 )

#define NUMVERTEXNORMALS	162
static Vector	s_raddir[NUMVERTEXNORMALS] = {
#include "anorms.h"
};
static Vector s_radcolor[NUMVERTEXNORMALS];

static int s_LightcacheFrame = -1;


//-----------------------------------------------------------------------------
// Cache used to compute which lightcache entries computed this frame
// may be able to be used temporarily for lighting other objects in the 
// case where we've got too many new lightcache samples in a single frame
//-----------------------------------------------------------------------------
struct CacheInfo_t
{
	int x;
	int y;
	int z;
	int leaf;
};


//-----------------------------------------------------------------------------
// Flags to pass into LightIntensityAndDirectionAtPoint + LightIntensityAndDirectionInBox
//-----------------------------------------------------------------------------
enum LightIntensityFlags_t
{
	LIGHT_NO_OCCLUSION_CHECK = 0x1,
	LIGHT_NO_RADIUS_CHECK = 0x2,
	LIGHT_OCCLUDE_VS_PROPS = 0x4,
};


//-----------------------------------------------------------------------------
// Lightcache entry
//-----------------------------------------------------------------------------
struct LightingStateInfo_t
{
	float	m_pIllum[MAXLOCALLIGHTS];
	bool 	m_LightingStateHasSkylight;
};

struct lightcache_t : public LightingStateInfo_t
{
	// bucket singly linked list
	lightcache_t*	next;
	lightcache_t**	bucket;

	// lru links
	lightcache_t*	lru_prev;
	lightcache_t*	lru_next;

	int				x,y,z,leaf;

	// If this is the same as r_framecount, then don't bother recalculating this cache entry (unless the flags are different than last time)
	int				m_LastFrameUpdated;

	// cache for static lighting . . never changes after cache creation
	// FIXME: Don't really need this for static props since we have this in a vertex buffer.
	LightingState_t	m_StaticLightingState;		

	// cache for light styles
	LightingState_t m_LightStyleLightingState;
	int				m_LightStyleFrameUpdated;	// This is the last frame that static lighting was updated.
	// FIXME: could just use m_LightStyleWorldLights.Count() if we are a static prop
	bool			m_HasLightstyle;	// This is set up once at cache creation time.
										// If this is false, then we don't use m_LightStyleLightingState.
	unsigned char	m_pLightstyles[MAX_LIGHTSTYLE_BYTES];
	ITexture *		m_pEnvCubemapTexture;

	// cache for dynamic lighting
	LightingState_t m_DynamicLightingState;
	int				m_DynamicFrameUpdated; // This is the last frame that dynamic lighting was updated.

	lightcache_t()
	{
		m_pEnvCubemapTexture = NULL;
	}
};

struct PropLightcache_t : public lightcache_t
{
	Vector			m_Origin;
	unsigned int	m_Flags; // corresponds to LIGHTCACHEFLAGS_*
	// stuff for pushing lights onto static props
	int				m_DLightFrame;	// This keeps up with whether or not we have dlights this frame,
									// and is also used by the "pushdlights" code to know if the 
									// m_DLightActive flags need to be cleared or not.
	int				m_DLightActive; // bit field for which dlights affect us.
									// If m_DLightFrame == r_dlightframecount, then this definitely doesn't need to be recomputed.
									// If ( m_DLightActive & r_dlightchanged ) != 0, then 
									// this cache entry needs to be rebuilt.
									// ??? : Also, if any of the active lights have changed since 
									// m_DLightFrame (via ???, then the cache needs to be update.)
	CUtlVector<short> m_LightStyleWorldLights; // This is a list of lights that affect this static prop cache entry.

	PropLightcache_t()
	{
		m_Flags = 0;
	}
};


ConVar 	r_worldlights	( "r_worldlights", "2" ); // number of world lights to use per vertex
ConVar 	r_radiosity		( "r_radiosity", "2" );		// do full radiosity calc?
ConVar	r_worldlightmin ( "r_worldlightmin", "0.005" );
ConVar	r_avglight		("r_avglight", "1");
static ConVar  r_lightdebug	("r_lightdebug", "0");
static ConVar  r_minnewsamples	("r_minnewsamples", "3");
static ConVar  r_maxnewsamples	("r_maxnewsamples", "6");
static ConVar  r_maxsampledist	("r_maxsampledist", "128");

static lightcache_t lightcache[MAX_CACHE_ENTRY];
static lightcache_t	*lightbuckets[MAX_CACHE_BUCKETS];

static CClassMemoryPool<PropLightcache_t>	s_PropCache( 256, CClassMemoryPool<lightcache_t>::GROW_SLOW );
 
// A memory pool of lightcache entries that is 
static int cached_r_worldlights = -1;
static int cached_r_radiosity = -1;
static int cached_r_avglight = -1;
static int cached_mat_fullbright = -1;
static PropLightcache_t* s_pFirstProp = NULL;

// head and tail sentinels of the LRU
static lightcache_t	lightlru, lightlrutail;

// Used to convert RGB colors to greyscale intensity
static Vector s_Grayscale( 0.299f, 0.587f, 0.114f ); 

#define BIT_SET( a, b ) ((a)[(b)>>3] & (1<<((b)&7)))


//-----------------------------------------------------------------------------
// Purpose: Set up the LRU
//-----------------------------------------------------------------------------
void R_StudioInitLightingCache( void )
{
	s_LightcacheFrame = -1;

	memset( lightcache, 0, sizeof(lightcache) );
	memset( lightbuckets, 0, sizeof(lightbuckets) );

	lightcache_t *last = &lightlru;
	// Link every node into the LRU
	int i;
	for ( i = 0; i < MAX_CACHE_ENTRY-1; i++)
	{
		lightcache[i].lru_prev = last;
		lightcache[i].lru_next = &lightcache[i+1];
		last = &lightcache[i];
	}
	// terminate the lru list
	lightcache[i].lru_prev = last;
	lightcache[i].lru_next = &lightlrutail;

	// point lru to entire list
	memset( &lightlru, 0, sizeof(lightlru) );
	memset( &lightlrutail, 0, sizeof(lightlrutail) );

	// link the sentinels
	lightlru.lru_next = lightcache;
	lightlrutail.lru_prev = &lightcache[i];

	cached_r_worldlights = r_worldlights.GetInt();
	cached_r_radiosity = r_radiosity.GetInt();
	cached_r_avglight = r_avglight.GetInt();
	cached_mat_fullbright = mat_fullbright.GetInt();

	// Recompute all static lighting
	RecomputeStaticLighting();
}


//-----------------------------------------------------------------------------
// Purpose: Moves this cache entry to the end of the lru, i.e. marks it recently used
// Input  : *pcache - 
//-----------------------------------------------------------------------------
static void LightcacheMark( lightcache_t *pcache )
{
	// don't link in static lighting
	if ( !pcache->lru_next && !pcache->lru_prev )
		return;

	// already at tail
	if ( lightlrutail.lru_prev == pcache )
		return;

	// unlink pcache
	pcache->lru_prev->lru_next = pcache->lru_next;
	pcache->lru_next->lru_prev = pcache->lru_prev;
	
	// link to tail
	// patch backward link
	lightlrutail.lru_prev->lru_next = pcache;
	pcache->lru_prev = lightlrutail.lru_prev;
	
	// patch forward link
	pcache->lru_next = &lightlrutail;
	lightlrutail.lru_prev = pcache;
}


//-----------------------------------------------------------------------------
// Purpose: Unlink a cache entry from its current bucket
// Input  : *pcache - 
//-----------------------------------------------------------------------------
static void LightcacheUnlink( lightcache_t *pcache )
{
	lightcache_t **pbucket = pcache->bucket;
	
	// not used yet?
	if ( !pbucket )
		return;

	// unlink it
	lightcache_t *plist = *pbucket;

	if ( plist == pcache )
	{
		// head of bucket?  move bucket down
		*pbucket = pcache->next;
	}
	else
	{
		bool found = false;
		// walk the bucket
		while ( plist )
		{
			// if next is pcache, unlink pcache
			if ( plist->next == pcache )
			{
				plist->next = pcache->next;
				found = true;
				break;
			}
			plist = plist->next;
		}
		assert(found);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Get the least recently used cache entry
//-----------------------------------------------------------------------------
static lightcache_t *LightcacheGetLRU( void )
{
	// grab head
	lightcache_t *pcache = lightlru.lru_next;

	// move to tail
	LightcacheMark( pcache );

	// unlink from the bucket
	LightcacheUnlink( pcache );

	pcache->leaf = -1;
	return pcache;
}


// number of bits per grid in x, y, z
#define		HASH_GRID_SIZEX		5
#define		HASH_GRID_SIZEY		5
#define		HASH_GRID_SIZEZ		7

//-----------------------------------------------------------------------------
// Purpose: Quick & Dirty hashing function to bucket the cube in 4d parameter space
//-----------------------------------------------------------------------------
static int LightcacheHashKey( int x, int y, int z, int leaf )
{
	unsigned int key = (((x<<20) + (y<<8) + z) ^ (leaf));
	key =  key % MAX_CACHE_BUCKETS;
	return (int)key;
}

//-----------------------------------------------------------------------------
// Precache lighting
//-----------------------------------------------------------------------------
void ComputeLightcacheBounds( const Vector &vecOrigin, Vector *pMins, Vector *pMaxs )
{
	bool bXPos = (vecOrigin[0] >= 0);
	bool bYPos = (vecOrigin[1] >= 0);
	bool bZPos = (vecOrigin[2] >= 0);

	int ix = ((int)fabs(vecOrigin[0])) >> HASH_GRID_SIZEX;
	int iy = ((int)fabs(vecOrigin[1])) >> HASH_GRID_SIZEY;
	int iz = ((int)fabs(vecOrigin[2])) >> HASH_GRID_SIZEZ;

	pMins->x = (bXPos ? ix : -(ix + 1)) << HASH_GRID_SIZEX;	
	pMins->y = (bYPos ? iy : -(iy + 1)) << HASH_GRID_SIZEY;	
	pMins->z = (bZPos ? iz : -(iz + 1)) << HASH_GRID_SIZEZ;	
	
	pMaxs->x = pMins->x + (1 << HASH_GRID_SIZEX );
	pMaxs->y = pMins->y + (1 << HASH_GRID_SIZEY );
	pMaxs->z = pMins->z + (1 << HASH_GRID_SIZEZ );

	Assert( (pMins->x <= vecOrigin.x) && (pMins->y <= vecOrigin.y) && (pMins->z <= vecOrigin.z) );
	Assert( (pMaxs->x >= vecOrigin.x) && (pMaxs->y >= vecOrigin.y) && (pMaxs->z >= vecOrigin.z) );
}



//-----------------------------------------------------------------------------
// Compute the lightcache bucket given a position
//-----------------------------------------------------------------------------
static lightcache_t* FindInCache( int bucket, int x, int y, int z, int leaf )
{
	// loop over the entries in this bucket
	lightcache_t *pcache;
	for ( pcache = lightbuckets[bucket]; pcache; pcache = pcache->next )
	{
		// hit?
		if (pcache->x == x && pcache->y == y && pcache->z == z && pcache->leaf == leaf )
		{
			return pcache;
		}
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Links to a bucket
//-----------------------------------------------------------------------------
static inline void LinkToBucket( int bucket, lightcache_t* pcache )
{
	pcache->next = lightbuckets[bucket];
	lightbuckets[bucket] = pcache;

	// point back to the bucket
	pcache->bucket = &lightbuckets[bucket];
}


//-----------------------------------------------------------------------------
// Links in a new lightcache entry
//-----------------------------------------------------------------------------
static lightcache_t* NewLightcacheEntry( int bucket )
{
	// re-use the LRU cache entry
	lightcache_t* pcache = LightcacheGetLRU();
	LinkToBucket( bucket, pcache );
	return pcache;
}


//-----------------------------------------------------------------------------
// Finds ambient lights
//-----------------------------------------------------------------------------
dworldlight_t* FindAmbientLight()
{
	// find any ambient lights
	for (int i = 0; i < host_state.worldmodel->brush.numworldlights; i++)
	{
		if (host_state.worldmodel->brush.worldlights[i].type == emit_skyambient)
		{
			return &host_state.worldmodel->brush.worldlights[i];
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Computes the ambient term from a particular surface
//-----------------------------------------------------------------------------
static void ComputeAmbientFromSurface( int surfID, dworldlight_t* pSkylight, 
									   Vector& radcolor )
{
	if (IS_SURF_VALID( surfID ) )
	{
		// If we hit the sky, use the sky ambient
		if (MSurf_Flags( surfID ) & SURFDRAW_SKY)
		{
			if (pSkylight)
			{
				// add in sky ambient
				VectorCopy( pSkylight->intensity, radcolor );
			}
		}
		else
		{
			Vector reflectivity;
			MSurf_TexInfo( surfID )->material->GetReflectivity( reflectivity );
			VectorMultiply( radcolor, reflectivity, radcolor );
		}
	}
}


//-----------------------------------------------------------------------------
// Computes the ambient term from a large number of spherical samples
//-----------------------------------------------------------------------------
static void ComputeAmbientFromSphericalSamples( const Vector& start, 
						Vector* lightBoxColor )
{
	// find any ambient lights
	dworldlight_t *pSkylight = FindAmbientLight();

	// sample world by casting N rays distributed across a sphere
	Vector upend;
	int i;
	for ( i = 0; i < NUMVERTEXNORMALS; i++)
	{
		// FIXME: a good optimization would be to scale this per leaf
		VectorMA( start, COORD_EXTENT * 1.74, s_raddir[i], upend );

		// Now that we've got a ray, see what surface we've hit
		int surfID = R_LightVec (start, upend, false, s_radcolor[i] );
		if (!IS_SURF_VALID(surfID) )
			continue;

		ComputeAmbientFromSurface( surfID, pSkylight, s_radcolor[i] );
	}

	// accumulate samples into radiant box
	const Vector* pBoxDirs = g_pStudioRender->GetAmbientLightDirections();
	for (int j = g_pStudioRender->GetNumAmbientLightSamples(); --j >= 0; )
	{
		float c, t;
		t = 0;

		lightBoxColor[j][0] = 0;
		lightBoxColor[j][1] = 0;
		lightBoxColor[j][2] = 0;

		for (i = 0; i < NUMVERTEXNORMALS; i++)
		{
			c = DotProduct( s_raddir[i], pBoxDirs[j] );
			if (c > 0)
			{
				t += c;
				VectorMA( lightBoxColor[j], c, s_radcolor[i], lightBoxColor[j] );
			}
		}
		VectorMultiply( lightBoxColor[j], 1/t, lightBoxColor[j] );
	}
}


//-----------------------------------------------------------------------------
// Computes the ambient term from 6 cardinal directions
//-----------------------------------------------------------------------------
static void ComputeAmbientFromAxisAlignedSamples( const Vector& start, 
						Vector* lightBoxColor )
{
	Vector upend;

	// find any ambient lights
	dworldlight_t *pSkylight = FindAmbientLight();

	// sample world only along cardinal axes
	const Vector* pBoxDirs = g_pStudioRender->GetAmbientLightDirections();
	for (int i = 0; i < 6; i++)
	{
		VectorMA( start, COORD_EXTENT * 1.74, pBoxDirs[i], upend );

		// Now that we've got a ray, see what surface we've hit
		int surfID = R_LightVec (start, upend, false, lightBoxColor[i] );
		if (!IS_SURF_VALID( surfID ) )
			continue;

		ComputeAmbientFromSurface( surfID, pSkylight, lightBoxColor[i] );

	}
}


//-----------------------------------------------------------------------------
// Computes the ambient lighting at a point, and sets the lightstyles bitfield
//-----------------------------------------------------------------------------
static void R_StudioGetAmbientLightForPoint( const Vector& start, Vector* pLightBoxColor, 
		bool bIsStaticProp )
{
	int i;
	if ( mat_fullbright.GetInt() == 1 )
	{
		for (i = g_pStudioRender->GetNumAmbientLightSamples(); --i >= 0; )
		{
			VectorFill( pLightBoxColor[i], 1.0 );
		}
		return;
	}

	switch( r_radiosity.GetInt() )
	{
	case 1:
		ComputeAmbientFromAxisAlignedSamples( start, pLightBoxColor );
		break;

	case 2:
		ComputeAmbientFromSphericalSamples( start, pLightBoxColor );
		break;

	case 3:
		if (bIsStaticProp)
			ComputeAmbientFromSphericalSamples( start, pLightBoxColor );
		else
			ComputeAmbientFromAxisAlignedSamples( start, pLightBoxColor );
		break;

	default:
		// assume no bounced light from the world
		for (i = g_pStudioRender->GetNumAmbientLightSamples(); --i >= 0; )
		{
			VectorFill( pLightBoxColor[i], 0 );
		}
	}
}


//-----------------------------------------------------------------------------
// This filter bumps against the world + all but one prop
//-----------------------------------------------------------------------------
class CTraceFilterWorldAndProps : public ITraceFilter
{
public:
	CTraceFilterWorldAndProps( IHandleEntity *pHandleEntity ) : m_pIgnoreProp( pHandleEntity ) {}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		// We only bump against props + we ignore one particular prop
		if ( !StaticPropMgr()->IsStaticProp( pHandleEntity ) )
			return false;

		return ( pHandleEntity != m_pIgnoreProp );
	}
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_EVERYTHING_FILTER_PROPS;
	}

private:
	IHandleEntity *m_pIgnoreProp;
};



//-----------------------------------------------------------------------------
// This method returns the effective intensity of a light as seen from
// a particular point. PVS is used to speed up the task.
//-----------------------------------------------------------------------------
static float LightIntensityAndDirectionAtPoint( dworldlight_t* pLight, 
	const Vector& mid, int fFlags, IHandleEntity *pIgnoreEnt, Vector *pDirection ) 
{
	CTraceFilterWorldOnly worldTraceFilter;
	CTraceFilterWorldAndProps propTraceFilter( pIgnoreEnt );
	ITraceFilter *pTraceFilter = &worldTraceFilter;
	if (fFlags & LIGHT_OCCLUDE_VS_PROPS)
	{
		pTraceFilter = &propTraceFilter;
	}

	// Special case lights
	switch (pLight->type)
	{
	case emit_skylight:
		{
			// There can be more than one skylight, but we should only
			// ever be affected by one of them (multiple ones are created from
			// a single light in vrad)

			// check to see if you can hit the sky texture
			Vector end;
			VectorMA( mid, -COORD_EXTENT * 1.74f, pLight->normal, end ); // max_range * sqrt(3)

			trace_t tr;
			Ray_t ray;
			ray.Init( mid, end );
			g_pEngineTraceClient->TraceRay( ray, MASK_OPAQUE, pTraceFilter, &tr );

			// Here, we didn't hit the sky, so we must be in shadow
			if ( !(tr.surface.flags & SURF_SKY) )
				return 0.0f;

			// fudge delta and dist for skylights
			VectorFill( *pDirection, 0 );
			return 1.0f;
		}

	case emit_skyambient:
		// always ignore these
		return 0.0f;
	}

	// all other lights

	// check distance
	VectorSubtract( pLight->origin, mid, *pDirection );
	float ratio = Engine_WorldLightDistanceFalloff( pLight, *pDirection, (fFlags & LIGHT_NO_RADIUS_CHECK) != 0 );

	// Add in light style component
	ratio *= LightStyleValue( pLight->style );

	// Early out for really low-intensity lights
	// That way we don't need to ray-cast or normalize
	float intensity = max( pLight->intensity[0], pLight->intensity[1] );
	intensity = max(intensity, pLight->intensity[2] );

	// This is about 1/256
	if (intensity * ratio < r_worldlightmin.GetFloat() )
		return 0.0f;

	float dist = VectorNormalize( *pDirection );

	if ( fFlags & LIGHT_NO_OCCLUSION_CHECK )
		return ratio;

	trace_t pm;
	Ray_t ray;
	ray.Init( mid, pLight->origin );
	g_pEngineTraceClient->TraceRay( ray, MASK_OPAQUE, pTraceFilter, &pm );

	// hack
	if ( (1.f-pm.fraction) * dist > 8 )
	{
		if (r_lightdebug.GetInt() == 1)
		{
			CDebugOverlay::AddLineOverlay( mid, pm.endpos, 255, 0, 0, true, 3 );
		}

		return 0.f;
	}

	return ratio;
}


//-----------------------------------------------------------------------------
// This method returns the effective intensity of a light as seen within
// a particular box...
// a particular point. PVS is used to speed up the task.
//-----------------------------------------------------------------------------
static float LightIntensityAndDirectionInBox( dworldlight_t* pLight, 
	const Vector &mid, const Vector &mins, const Vector &maxs, int fFlags, 
	Vector *pDirection ) 
{
	// Choose the point closest on the box to the light to get max intensity
	// within the box....

	// NOTE: Here, we do radius check to check to see if we should even care about the light
	// because we want to check the closest point in the box
	switch (pLight->type)
	{
	case emit_point:
	case emit_spotlight:	// directional & positional
		{
			Vector vecClosestPoint;
			vecClosestPoint.Init();
			for ( int i = 0; i < 3; ++i )
			{
				vecClosestPoint[i] = clamp( pLight->origin[i], mins[i], maxs[i] );
			}

			vecClosestPoint -= pLight->origin;
			if ( vecClosestPoint.LengthSqr() > pLight->radius * pLight->radius )
				return 0;
		}
		break;
	}

	return LightIntensityAndDirectionAtPoint( pLight, mid, fFlags | LIGHT_NO_RADIUS_CHECK, NULL, pDirection );
}

	
//-----------------------------------------------------------------------------
// Computes the static vertex lighting term from a large number of spherical samples
//-----------------------------------------------------------------------------
bool ComputeVertexLightingFromSphericalSamples( const Vector& vecVertex, 
	const Vector &vecNormal, IHandleEntity *pIgnoreEnt, Vector *pLinearColor )
{
	// Check to see if this vertex is in solid
	trace_t tr;
	CTraceFilterWorldAndProps filter( pIgnoreEnt );
	Ray_t ray;
	ray.Init( vecVertex, vecVertex );
	g_pEngineTraceClient->TraceRay( ray, MASK_OPAQUE, &filter, &tr );
	if ( tr.startsolid || tr.allsolid )
		return false;

	pLinearColor->Init( 0, 0, 0 );

	// find any ambient lights
	dworldlight_t *pSkylight = FindAmbientLight();

	// sample world by casting N rays distributed across a sphere
	float t = 0.0f;
	Vector upend, color;
	int i;
	for ( i = 0; i < NUMVERTEXNORMALS; i++)
	{
		float flDot = DotProduct( vecNormal, s_raddir[i] );
		if ( flDot < 0.0f )
			continue;

		// FIXME: a good optimization would be to scale this per leaf
		VectorMA( vecVertex, COORD_EXTENT * 1.74, s_raddir[i], upend );

		// Now that we've got a ray, see what surface we've hit
		int surfID = R_LightVec( vecVertex, upend, false, color );
		if ( !IS_SURF_VALID(surfID) )
			continue;

		// FIXME: Maybe make sure we aren't obstructed by static props?
		// To do this, R_LightVec would need to return distance of hit...
		// Or, we need another arg to R_LightVec to return black when hitting a static prop
		ComputeAmbientFromSurface( surfID, pSkylight, color );

		t += flDot;
		VectorMA( *pLinearColor, flDot, color, *pLinearColor );
	}

	if (t != 0.0f)
	{
		*pLinearColor /= t;
	}

	// Now deal with direct lighting
	bool bHasSkylight = false;

	// Figure out the PVS info for this location
 	int leaf = CM_PointLeafnum( vecVertex );
	byte* pVis = CM_ClusterPVS( CM_LeafCluster( leaf ) );

	// Now add in the direct lighting
	Vector vecDirection;
	for (i = 0; i < host_state.worldmodel->brush.numworldlights; ++i)
	{
		dworldlight_t *wl = &host_state.worldmodel->brush.worldlights[i];

		// FIXME: This is sort of a hack; only one skylight is allowed in the
		// lighting...
		if ((wl->type == emit_skylight) && bHasSkylight)
			continue;

		// only do it if the entity can see into the lights leaf
		if ((wl->cluster < 0) || (!BIT_SET( pVis, wl->cluster )) )
			continue;

		float flRatio = LightIntensityAndDirectionAtPoint( wl, vecVertex, LIGHT_OCCLUDE_VS_PROPS, pIgnoreEnt, &vecDirection );

		// No light contribution? Get outta here!
		if ( flRatio <= 0.0f )
			continue;

		// Check if we've got a skylight
		if ( wl->type == emit_skylight )
			bHasSkylight = true;

		// Figure out spotlight attenuation
		float flAngularRatio = Engine_WorldLightAngle( wl, wl->normal, vecNormal, vecDirection );

		// Add in the direct lighting
		VectorMAInline( *pLinearColor, flAngularRatio * flRatio, wl->intensity, *pLinearColor );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Finds the minimum light
//-----------------------------------------------------------------------------
static int FindDarkestWorldLight( int numLights, float* pLightIllum, float newIllum )
{
	// FIXME: make the list sorted?
	int minLightIndex = -1;
	float minillum = newIllum;
	for (int j = 0; j < numLights; ++j)
	{
		// only check ones dimmer than have already been checked
		if (pLightIllum[j] < minillum)
		{
			minillum = pLightIllum[j];
			minLightIndex = j;
		}
	}

	return minLightIndex;
}


//-----------------------------------------------------------------------------
// Adds a world light to the ambient cube
//-----------------------------------------------------------------------------
static void	AddWorldLightToLightCube( dworldlight_t* pWorldLight, 
										Vector* pBoxColor,
										const Vector& direction,
										float ratio )
{
	if (ratio == 0.0f)
		return;

	// add whatever didn't stay in the list to lightBoxColor
	// FIXME: This method is a guess, I don't know how it should be done
	const Vector* pBoxDir = g_pStudioRender->GetAmbientLightDirections();

	for (int j = g_pStudioRender->GetNumAmbientLightSamples(); --j >= 0; )
	{
		float t = DotProduct( pBoxDir[j], direction );
		if (t > 0)
		{
			VectorMAInline( pBoxColor[j], ratio * t, pWorldLight->intensity, pBoxColor[j] );
		}
	}
}


//-----------------------------------------------------------------------------
// Adds a world light to the list of lights
//-----------------------------------------------------------------------------
static void	AddWorldLightToLightingState( dworldlight_t* pWorldLight, 
	LightingState_t& lightingState, LightingStateInfo_t& info,
	const Vector& bucketOrigin, byte* pBucketVisibility, bool dynamic = false, 
	bool bIgnoreVis = false )
{

	// FIXME: This is sort of a hack; only one skylight is allowed in the
	// lighting...
	if ((pWorldLight->type == emit_skylight) && info.m_LightingStateHasSkylight)
		return;

	// only do it if the entity can see into the lights leaf
	if( !bIgnoreVis )
	{
		if ((pWorldLight->cluster < 0) || (!BIT_SET( pBucketVisibility, pWorldLight->cluster )) )
			return;
	}

	// Get the lighting ratio
	Vector direction;

	float ratio;

	if (!dynamic)
	{
		ratio = LightIntensityAndDirectionAtPoint( pWorldLight, bucketOrigin, 0, NULL, &direction );
	}
	else
	{
		Vector mins, maxs;
		ComputeLightcacheBounds( bucketOrigin, &mins, &maxs );
		ratio = LightIntensityAndDirectionInBox( pWorldLight, bucketOrigin, mins, maxs, dynamic ? LIGHT_NO_OCCLUSION_CHECK : 0, &direction );
	}

	// No light contribution? Get outta here!
	if ( ratio <= 0.0f )
		return;

	// Check if we've got a skylight
	if (pWorldLight->type == emit_skylight)
		info.m_LightingStateHasSkylight = true;

	// Figure out spotlight attenuation
	float angularRatio = Engine_WorldLightAngle( pWorldLight, pWorldLight->normal, direction, direction );

	// Add it to the total light box color
	// NOTE: This is a workaround for the crazy gamma solution we gotta use on DX7
	// parts. If we add dynamic lights to the total light color, it can cause other
	// parts of the model to actually become darker as the dynamic light moves around!
	if (!dynamic)
		AddWorldLightToLightCube( pWorldLight, lightingState.totalboxcolor, direction, ratio * angularRatio );

	// Use standard RGB to gray conversion
	float illum = ratio * DotProduct( pWorldLight->intensity, s_Grayscale );

	// It can't be a local light if it's too dark
	if (illum >= r_worldlightmin.GetFloat()) // FIXME: tune this value
	{
		// if remaining slots, add to list
		if (lightingState.numlights < r_worldlights.GetInt())
		{
			// save pointer to world light
			lightingState.locallight[lightingState.numlights] = pWorldLight;
			info.m_pIllum[lightingState.numlights] = illum;
			++lightingState.numlights;
			return;
		}

		// no remaining slots
		// find the dimmest existing light and replace 
		int minLightIndex = FindDarkestWorldLight( lightingState.numlights, info.m_pIllum, dynamic ? 100000 : illum );
		if (minLightIndex != -1)
		{
			// FIXME: We're sorting by ratio here instead of illum cause we either
			// have to store more memory or do more computations; I'm not
			// convinced it's any better of a metric though, so ratios for now...

			// found a light was was dimmer, swap it with the current light
			swap( pWorldLight, lightingState.locallight[minLightIndex] );
			swap( illum, info.m_pIllum[minLightIndex] );

			// FIXME: Avoid these recomputations 
			// But I don't know how to do it without storing a ton of data
			// per cache entry... yuck!

			// NOTE: We know the dot product can't be zero or illum would have been 0 to start with!
			ratio = illum / DotProduct( pWorldLight->intensity, s_Grayscale );

			if (pWorldLight->type == emit_skylight)
			{
				VectorFill( direction, 0 );
				angularRatio = 1.0f;
			}
			else
			{
				VectorSubtract( pWorldLight->origin, bucketOrigin, direction );
				VectorNormalize( direction );

				// Recompute the ratios
				angularRatio = Engine_WorldLightAngle( pWorldLight, pWorldLight->normal, direction, direction );
			}
		}
	}

	// Add the low light to the ambient box color
	AddWorldLightToLightCube( pWorldLight, lightingState.r_boxcolor, direction, ratio * angularRatio );
}


//-----------------------------------------------------------------------------
// Construct a world light from a dynamic light
//-----------------------------------------------------------------------------
static void WorldLightFromDynamicLight( dlight_t const& dynamicLight, 
									   dworldlight_t& worldLight )
{
	VectorCopy( dynamicLight.origin, worldLight.origin );
	worldLight.type = emit_point;

	worldLight.intensity[0] = TexLightToLinear( dynamicLight.color.r, dynamicLight.color.exponent );
	worldLight.intensity[1] = TexLightToLinear( dynamicLight.color.g, dynamicLight.color.exponent );
	worldLight.intensity[2] = TexLightToLinear( dynamicLight.color.b, dynamicLight.color.exponent );
	worldLight.style = dynamicLight.style;

	// Compute cluster associated with the dynamic light
	worldLight.cluster = CM_LeafCluster( CM_PointLeafnum(worldLight.origin) );

	// Assume a quadratic attenuation factor; atten so we hit minlight
	// at radius away...
	float minlight = max( dynamicLight.minlight, MIN_LIGHTING_VALUE );
	float radius = dynamicLight.radius;
	if (radius <= 0.0f)
	{
		// No radius? then don't attenuate
		worldLight.constant_attn = 1.0f;
		worldLight.linear_attn = 0;
		worldLight.quadratic_attn = 0;
	}
	else
	{
		worldLight.constant_attn = 0;
		worldLight.linear_attn = 0; 
		worldLight.quadratic_attn = 1.0f / (minlight * radius * radius);
	}

	// Set the max radius
	worldLight.radius = dynamicLight.radius;

	// Spotlights...
	if (dynamicLight.m_OuterAngle > 0.0f)
	{
		worldLight.type = emit_spotlight;
		VectorCopy( dynamicLight.m_Direction, worldLight.normal );
		worldLight.stopdot = cos( dynamicLight.m_InnerAngle * M_PI / 180.0f );
		worldLight.stopdot2 = cos( dynamicLight.m_OuterAngle * M_PI / 180.0f );
	}
}


//-----------------------------------------------------------------------------
// Add in dynamic worldlights (lightstyles)
//-----------------------------------------------------------------------------
static void AddLightStyles( LightingStateInfo_t& info, LightingState_t& lightingState, 
			const Vector& origin, int leaf, byte* pVis )
{
	// Next, add each world light with a lightstyle into the lighting state,
	// ejecting less relevant local lights + folding them into the ambient cube
	for ( int i = 0; i < host_state.worldmodel->brush.numworldlights; ++i)
	{
		dworldlight_t *wl = &host_state.worldmodel->brush.worldlights[i];
		if (wl->style == 0)
			continue;

		// This is an optimization to avoid decompressing Vis twice
		if (!pVis)
		{
			// Figure out the PVS info for this location
 			pVis = CM_ClusterPVS( CM_LeafCluster( leaf ) );
		}

		// Now add that world light into our list of worldlights
		AddWorldLightToLightingState( wl, lightingState, info, origin, pVis );
	}
}


//-----------------------------------------------------------------------------
// Add in dynamic worldlights (lightstyles)
//-----------------------------------------------------------------------------
static void AddLightStylesForStaticProp( PropLightcache_t *pcache, LightingState_t& lightingState )
{
	// Next, add each world light with a lightstyle into the lighting state,
	// ejecting less relevant local lights + folding them into the ambient cube
	for( int i = 0; i < pcache->m_LightStyleWorldLights.Count(); ++i )
	{
		Assert( pcache->m_LightStyleWorldLights[i] >= 0 );
		Assert( pcache->m_LightStyleWorldLights[i] < host_state.worldmodel->brush.numworldlights );
		dworldlight_t *wl = &host_state.worldmodel->brush.worldlights[pcache->m_LightStyleWorldLights[i]];
		Assert( wl->style != 0 );

		// Now add that world light into our list of worldlights
		AddWorldLightToLightingState( wl, lightingState, *pcache, pcache->m_Origin, NULL, 
			false /*dynamic*/, true /*ignorevis*/ );
	}
}


//-----------------------------------------------------------------------------
// Add DLights + ELights to the dynamic lighting
//-----------------------------------------------------------------------------
static dworldlight_t s_pDynamicLight[MAX_DLIGHTS];

static void AddDLights( LightingStateInfo_t& info, LightingState_t& lightingState, 
			const Vector& origin, int leaf, byte* pVis )
{
	// Next, add each world light with a lightstyle into the lighting state,
	// ejecting less relevant local lights + folding them into the ambient cube
	dlight_t* dl = cl_dlights;
	for ( int i=0; i<MAX_DLIGHTS; ++i, ++dl )
	{
		// If the light's not active, then continue
		if ( (r_dlightactive & (1 << i)) == 0 )
			continue;

		// If the light doesn't affect models, then continue
		if (dl->flags & (DLIGHT_NO_MODEL_ILLUMINATION | DLIGHT_DISPLACEMENT_MASK)) 
			continue;

		// This is an optimization to avoid decompressing Vis twice
		if (!pVis)
		{
			// Figure out the PVS info for this location
 			pVis = CM_ClusterPVS( CM_LeafCluster( leaf ) );
		}

		// Construct a world light representing the dynamic light
		// we're making a static list here because the lighting state
		// contains a set of pointers to dynamic lights
		WorldLightFromDynamicLight( *dl, s_pDynamicLight[i] );

		// Now add that world light into our list of worldlights
		AddWorldLightToLightingState( &s_pDynamicLight[i], lightingState,
			info, origin, pVis, true );
	}
}

static void AddELights( LightingStateInfo_t& info, LightingState_t& lightingState, 
			const Vector& origin, int leaf, byte* pVis )
{
	// Next, add each world light with a lightstyle into the lighting state,
	// ejecting less relevant local lights + folding them into the ambient cube
	dlight_t* dl = cl_elights;
	for ( int i=0; i<MAX_ELIGHTS; ++i, ++dl )
	{
		// If the light's not active, then continue
		if ( dl->radius == 0 )
			continue;

		// If the light doesn't affect models, then continue
		if (dl->flags & (DLIGHT_NO_MODEL_ILLUMINATION | DLIGHT_DISPLACEMENT_MASK))
			continue;

		// This is an optimization to avoid decompressing Vis twice
		if (!pVis)
		{
			// Figure out the PVS info for this location
 			pVis = CM_ClusterPVS( CM_LeafCluster( leaf ) );
		}

		// Construct a world light representing the dynamic light
		// we're making a static list here because the lighting state
		// contains a set of pointers to dynamic lights
		WorldLightFromDynamicLight( *dl, s_pDynamicLight[i] );

		// Now add that world light into our list of worldlights
		AddWorldLightToLightingState( &s_pDynamicLight[i], lightingState,
			info, origin, pVis, true );
	}
}


//-----------------------------------------------------------------------------
// Given static + dynamic lighting, figure out the total light
//-----------------------------------------------------------------------------
static void AddDynamicLighting( lightcache_t* pCache, LightingState_t& lightingState, 
			const Vector& origin, int leaf, byte* pVis = 0 )
{
	if (pCache->m_LastFrameUpdated != r_framecount)
	{
		// First factor in the cache into the current lighting state..
		LightingStateInfo_t info;
		
		// Copy over the static illumination values into the dynamic cache; we're gonna destructively modify them
		memcpy( &pCache->m_LightStyleLightingState, &pCache->m_StaticLightingState, sizeof( LightingState_t ) );
		// FIXME: do we need these two lines?
		memcpy( info.m_pIllum, pCache->m_pIllum, pCache->m_StaticLightingState.numlights * sizeof(float) );
		info.m_LightingStateHasSkylight = pCache->m_LightingStateHasSkylight;

		// Next, add each world light with a lightstyle into the lighting state,
		// ejecting less relevant local lights + folding them into the ambient cube
		AddLightStyles( info, pCache->m_LightStyleLightingState, origin, leaf, pVis );

		memcpy( &pCache->m_DynamicLightingState, &pCache->m_LightStyleLightingState, sizeof( LightingState_t ) );
		// Next, add each dlight one at a time 
		AddDLights( info, pCache->m_DynamicLightingState, origin, leaf, pVis );

		// Finally, add in elights
 		AddELights( info, pCache->m_DynamicLightingState, origin, leaf, pVis );

		pCache->m_LastFrameUpdated = r_framecount;
	}

	memcpy( &lightingState, &pCache->m_DynamicLightingState, sizeof(LightingState_t) );
}


//-----------------------------------------------------------------------------
// Adds a world light to the list of lights
//-----------------------------------------------------------------------------
static void	AddWorldLightToLightingStateForStaticProps( dworldlight_t* pWorldLight, 
	LightingState_t& lightingState, LightingStateInfo_t& info, PropLightcache_t *pCache,
	bool dynamic = false )
{
	// FIXME: This is sort of a hack; only one skylight is allowed in the
	// lighting...
	if ((pWorldLight->type == emit_skylight) && info.m_LightingStateHasSkylight)
		return;

	// Get the lighting ratio
	float ratio;
	Vector direction;

	// FIXME: Try this for non dlights too, don't want to do it yet. Scary change this close to E3
	if (!dynamic)
	{
		ratio = LightIntensityAndDirectionAtPoint( pWorldLight, pCache->m_Origin, 0, NULL, &direction );
	}
	else
	{
		Vector mins, maxs;
		ComputeLightcacheBounds( pCache->m_Origin, &mins, &maxs );
		ratio = LightIntensityAndDirectionInBox( pWorldLight, pCache->m_Origin, 
			mins, maxs, LIGHT_NO_OCCLUSION_CHECK, &direction );
	}

	// No light contribution? Get outta here!
	if ( ratio <= 0.0f )
		return;

	// Check if we've got a skylight
	if (pWorldLight->type == emit_skylight)
		info.m_LightingStateHasSkylight = true;

	// Figure out spotlight attenuation
	float angularRatio = Engine_WorldLightAngle( pWorldLight, pWorldLight->normal, direction, direction );

	// Add it to the total light box color
	// NOTE: This is a workaround for the crazy gamma solution we gotta use on DX7
	// parts. If we add dynamic lights to the total light color, it can cause other
	// parts of the model to actually become darker as the dynamic light moves around!
	if (!dynamic)
	{
		AddWorldLightToLightCube( pWorldLight, lightingState.totalboxcolor, direction, ratio * angularRatio );
	}

	// Use standard RGB to gray conversion
	float illum = ratio * DotProduct( pWorldLight->intensity, s_Grayscale );

	// It can't be a local light if it's too dark
	if (illum >= r_worldlightmin.GetFloat()) // FIXME: tune this value
	{
		// if remaining slots, add to list
		if (lightingState.numlights < r_worldlights.GetInt())
		{
			// save pointer to world light
			lightingState.locallight[lightingState.numlights] = pWorldLight;
			info.m_pIllum[lightingState.numlights] = illum;
			++lightingState.numlights;
			return;
		}

		// no remaining slots
		// find the dimmest existing light and replace 
		int minLightIndex = FindDarkestWorldLight( lightingState.numlights, info.m_pIllum, dynamic ? 100000 : illum );
		if (minLightIndex != -1)
		{
			// FIXME: We're sorting by ratio here instead of illum cause we either
			// have to store more memory or do more computations; I'm not
			// convinced it's any better of a metric though, so ratios for now...

			// found a light was was dimmer, swap it with the current light
			swap( pWorldLight, lightingState.locallight[minLightIndex] );
			swap( illum, info.m_pIllum[minLightIndex] );

			// FIXME: Avoid these recomputations 
			// But I don't know how to do it without storing a ton of data
			// per cache entry... yuck!

			// NOTE: We know the dot product can't be zero or illum would have been 0 to start with!
			ratio = illum / DotProduct( pWorldLight->intensity, s_Grayscale );

			if (pWorldLight->type == emit_skylight)
			{
				VectorFill( direction, 0 );
				angularRatio = 1.0f;
			}
			else
			{
				VectorSubtract( pWorldLight->origin, pCache->m_Origin, direction );
				VectorNormalize( direction );

				// Recompute the ratios
				angularRatio = Engine_WorldLightAngle( pWorldLight, pWorldLight->normal, direction, direction );
			}
		}
	}

	// Add the low light to the ambient box color
	AddWorldLightToLightCube( pWorldLight, lightingState.r_boxcolor, direction, ratio * angularRatio );
}

static void AddDLightsForStaticProps( LightingStateInfo_t& info, LightingState_t& lightingState, 
			PropLightcache_t *pCache  )
{
	// no lights this frame!
	if( pCache->m_DLightFrame != r_dlightframecount )
	{
		return;
	}
	
	// Next, add each world light with a lightstyle into the lighting state,
	// ejecting less relevant local lights + folding them into the ambient cube
	dlight_t* dl = cl_dlights;
	for ( int i=0; i<MAX_DLIGHTS; ++i, ++dl )
	{
		// If the light doesn't affect this model, then continue.
		if( !( pCache->m_DLightActive & ( 1 << i ) ) )
		{
			continue;
		}

		// Construct a world light representing the dynamic light
		// we're making a static list here because the lighting state
		// contains a set of pointers to dynamic lights
		WorldLightFromDynamicLight( *dl, s_pDynamicLight[i] );

		// Now add that world light into our list of worldlights
		AddWorldLightToLightingStateForStaticProps( &s_pDynamicLight[i], lightingState,
			info, pCache, true );
	}
}

//-----------------------------------------------------------------------------
// Add static lighting to the lighting state
//-----------------------------------------------------------------------------
static void AddStaticLighting( lightcache_t* pCache, const Vector& origin, byte* pVis )
{
	// First, blat out the lighting state
	int i;
	pCache->m_StaticLightingState.numlights = 0;
	for ( i = g_pStudioRender->GetNumAmbientLightSamples(); --i >= 0; )
	{
		VectorCopy( pCache->m_StaticLightingState.r_boxcolor[i], pCache->m_StaticLightingState.totalboxcolor[i] );
	}
	pCache->m_LightingStateHasSkylight = false;

	// Next, add each static light one at a time into the lighting state,
	// ejecting less relevant local lights + folding them into the ambient cube
	// Also, we need to add *all* new lights into the total box color
	for (i = 0; i < host_state.worldmodel->brush.numworldlights; ++i)
	{
		dworldlight_t *wl = &host_state.worldmodel->brush.worldlights[i];

#ifdef TROIKA
		// Go ahead and add lightstyles in here.
		if (wl->style != 0)
		{
			int byte = wl->style >> 3;
			int bit = wl->style & 0x7;
			pCache->m_pLightstyles[byte] |= (1 << bit);
			pCache->m_HasLightstyle = true;
		}
#else
		// Don't add lights with lightstyles... treat them as dynamic lights instead
		if (wl->style != 0)
			continue;
#endif // TROIKA

		// Now add that world light into our list of worldlights
		AddWorldLightToLightingState( wl, pCache->m_StaticLightingState, *pCache, origin, pVis, false, false );
	}
}


//-----------------------------------------------------------------------------
// Checks to see if the lightstyles are valid for this cache entry
//-----------------------------------------------------------------------------
static bool IsCachedLightStylesValid( lightcache_t* pCache )
{
	if (!pCache->m_HasLightstyle)
		return true;

	// FIXME: Can this start at 1, or is 0 required?
	for (int i = 1; i < MAX_LIGHTSTYLES; ++i)
	{
		int byte = i >> 3;
		int bit = i & 0x7;
		if (pCache->m_pLightstyles[byte] & ( 1 << bit ))
		{
			if (d_lightstyleframe[i] > pCache->m_LastFrameUpdated)
				return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Compute the lightcache origin
//-----------------------------------------------------------------------------
static inline void ComputeLightcacheOrigin( int x, int y, int z, Vector& org )
{
	int ix = x << HASH_GRID_SIZEX;
	int iy = y << HASH_GRID_SIZEY;
	int iz = z << HASH_GRID_SIZEZ;
	org.Init( ix, iy, iz );
}


//-----------------------------------------------------------------------------
// Find a lightcache entry within the requested radius from a point
//-----------------------------------------------------------------------------
static int FindRecentCacheEntryWithinRadius( int count, CacheInfo_t* pCache, const Vector& origin, float radius )
{
	radius *= radius;

	// Try to find something within the radius of an existing new sample
	int minIndex = -1;
	for (int i = 0; i < count; ++i)
	{
		Vector delta;
		ComputeLightcacheOrigin( pCache[i].x, pCache[i].y, pCache[i].z, delta );
		delta -= origin;
		float distSq = delta.LengthSqr();
		if (distSq < radius )
		{
			minIndex = i;
			radius = distSq;
		}
	}

	return minIndex;
}


//-----------------------------------------------------------------------------
// Frame-based lightcache
//-----------------------------------------------------------------------------
static lightcache_t* CheckFrameCache( int x, int y, int z, int leaf )
{
	// Keep track of the number of new cache entries computed every frame
	// Make sure it's no greater than our max count 
	static int newCacheEntryCount = 0;
	static CacheInfo_t frameCache[FRAME_CACHE_COUNT];
	static bool noLightingLimit = false;

	if ( r_framecount != s_LightcacheFrame )
	{
		noLightingLimit = ( s_LightcacheFrame == -1 );
		s_LightcacheFrame = r_framecount;
		newCacheEntryCount = 0;
	}

	if (noLightingLimit)
		return NULL;

	if (newCacheEntryCount < r_minnewsamples.GetInt() )
	{
		// We've not added enough samples this frame, just add it automagically
		Assert( newCacheEntryCount < FRAME_CACHE_COUNT );
		frameCache[newCacheEntryCount].x = x;
		frameCache[newCacheEntryCount].y = y;
		frameCache[newCacheEntryCount].z = z;
		frameCache[newCacheEntryCount].leaf = leaf;
		++newCacheEntryCount;

		return NULL;
	}

	Vector org;
	ComputeLightcacheOrigin( x, y, z, org ); 

	int maxSamples = r_maxnewsamples.GetInt();
	if (maxSamples > FRAME_CACHE_COUNT)
		maxSamples = FRAME_CACHE_COUNT;

	int closest = -1;
	if (newCacheEntryCount < maxSamples )
	{
		float minDist = r_maxsampledist.GetFloat();
		closest = FindRecentCacheEntryWithinRadius( newCacheEntryCount, frameCache,
			org, minDist );

		if (closest < 0)
		{
			// Didn't find anything close enough, add a new entry
			Assert( newCacheEntryCount < FRAME_CACHE_COUNT );
			frameCache[newCacheEntryCount].x = x;
			frameCache[newCacheEntryCount].y = y;
			frameCache[newCacheEntryCount].z = z;
			frameCache[newCacheEntryCount].leaf = leaf;
			++newCacheEntryCount;

			return NULL;
		}
	}

	if (closest < 0)
	{
		// We've hit our hard limit on the number of new samples this frame
		// We must find the closest sample and just deal
		closest = FindRecentCacheEntryWithinRadius( newCacheEntryCount, frameCache,
			org, COORD_EXTENT );

		Assert( closest >= 0 );
	}

	CacheInfo_t& cache = frameCache[closest];
	int bucket = LightcacheHashKey( cache.x, cache.y, cache.z, cache.leaf );
	lightcache_t *pcache = FindInCache(bucket, cache.x, cache.y, cache.z, cache.leaf);
	Assert( pcache );

	return pcache;
}


//-----------------------------------------------------------------------------
// Draw the lightcache box for debugging
//-----------------------------------------------------------------------------
static void DebugRenderLightcache( int x, int y, int z, LightingState_t& lightingState, const Vector* pOrigin = NULL )
{
	int ix = x << HASH_GRID_SIZEX;
	int iy = y << HASH_GRID_SIZEY;
	int iz = z << HASH_GRID_SIZEZ;

	Vector center;
	Vector org, mins, maxs;
	if (!pOrigin)
	{
		org.Init( ix, iy, iz );
		mins.Init( 0, 0, 0 );
		maxs.Init( 1 << HASH_GRID_SIZEX, 1 << HASH_GRID_SIZEY, 1 << HASH_GRID_SIZEZ );
	}
	else
	{
		org = *pOrigin;
		mins.Init( -5, -5, -5 );
		maxs.Init( 5, 5, 5 );
	}
	center = mins + maxs;
	center *= 0.5f;
	center += org;

	CDebugOverlay::AddBoxOverlay( org, mins, maxs, vec3_angle, 255, 255, 255, 0, 0.001f );
	for (int j = 0; j < lightingState.numlights; ++j)
	{
		CDebugOverlay::AddLineOverlay( center, lightingState.locallight[j]->origin, 0, 0, 255, true, 0.001f );
	}
}


//-----------------------------------------------------------------------------
// Identify lighting errors
//-----------------------------------------------------------------------------
bool IdentifyLightingErrors( int leaf, LightingState_t& lightingState )
{
	if (r_lightdebug.GetInt() == 3)
	{
		if (CM_LeafContents(leaf) == CONTENTS_SOLID)
		{
			// Try another choice...
			lightingState.r_boxcolor[0].Init( 1, 0, 0 );
			lightingState.r_boxcolor[1].Init( 1, 0, 0 );
			lightingState.r_boxcolor[2].Init( 1, 0, 0 );
			lightingState.r_boxcolor[3].Init( 1, 0, 0 );
			lightingState.r_boxcolor[4].Init( 1, 0, 0 );
			lightingState.r_boxcolor[5].Init( 1, 0, 0 );
			lightingState.numlights = 0;
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Compute the cache...
//-----------------------------------------------------------------------------
static byte* ComputeStaticLightingForCacheEntry( lightcache_t *pcache, const Vector& origin, int leaf, bool bStaticProp = false )
{
	// Figure out the PVS info for this location
 	byte* pVis = CM_ClusterPVS( CM_LeafCluster( leaf ) );

	R_StudioGetAmbientLightForPoint( origin, pcache->m_StaticLightingState.r_boxcolor,
		bStaticProp );

	// get direct lighting from world light sources (point lights, etc.)
	AddStaticLighting( pcache, origin, pVis );

	return pVis;
}


static void BuildStaticLightingCacheLightStyleInfo( PropLightcache_t* pcache )
{
	byte *pVis = NULL;
	Assert( pcache->m_LightStyleWorldLights.Count() == 0 );
	pcache->m_HasLightstyle = false;
	// clear lightstyles
	memset( pcache->m_pLightstyles, 0, MAX_LIGHTSTYLE_BYTES );
	for ( short i = 0; i < host_state.worldmodel->brush.numworldlights; ++i)
	{
		dworldlight_t *wl = &host_state.worldmodel->brush.worldlights[i];
		if (wl->style == 0)
			continue;

		// This is an optimization to avoid decompressing Vis twice
		if (!pVis)
		{
			// Figure out the PVS info for this static prop
 			pVis = CM_ClusterPVS( CM_LeafCluster( pcache->leaf ) );
		}
		// FIXME: Could do better here if we had access to the list of leaves that this
		// static prop is in.  For now, we use the lighting origin.
		if( pVis[ wl->cluster >> 3 ] & ( 1 << ( wl->cluster & 7 ) ) )
		{
			// FIXME: Need to take maximum illumination into consideration so that we 
			// can cull out lights that are far away.
			pcache->m_LightStyleWorldLights.AddToTail( i );

			int byte = wl->style >> 3;
			int bit = wl->style & 0x7;
			pcache->m_pLightstyles[byte] |= ( 1 << bit );
			pcache->m_HasLightstyle = true;
		}
	}
}

static ITexture *FindEnvCubemapForPoint( const Vector& origin )
{
	MEASURE_TIMED_STAT( ENGINE_STATS_FIND_ENVCUBEMAP_TIME );
	model_t *pWorldModel = host_state.worldmodel;
	if( pWorldModel->brush.m_nCubemapSamples > 0 )
	{
		int smallestIndex = 0;
		Vector blah = origin - pWorldModel->brush.m_pCubemapSamples[0].origin;
		float smallestDist = DotProduct( blah, blah );
		int i;
		for( i = 1; i < pWorldModel->brush.m_nCubemapSamples; i++ )
		{
			Vector blah = origin - pWorldModel->brush.m_pCubemapSamples[i].origin;
			float dist = DotProduct( blah, blah );
			if( dist < smallestDist )
			{
				smallestDist = dist;
				smallestIndex = i;
			}
		}
		
		return pWorldModel->brush.m_pCubemapSamples[smallestIndex].pTexture;
	}
	else
	{
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Precache lighting
//-----------------------------------------------------------------------------
LightCacheHandle_t CreateStaticLightingCache( const Vector& origin )
{
	PropLightcache_t* pcache = s_PropCache.Alloc();

	pcache->m_Origin = origin;
	pcache->m_Flags = 0;

	// initialize this to point to our current origin
	pcache->x = ((unsigned int)origin[0]) >> HASH_GRID_SIZEX;
	pcache->y = ((unsigned int)origin[1]) >> HASH_GRID_SIZEY;
	pcache->z = ((unsigned int)origin[2]) >> HASH_GRID_SIZEZ;
	pcache->leaf = CM_PointLeafnum(origin);

	// Add the prop to the list of props
	pcache->lru_next = s_pFirstProp;
	pcache->lru_prev = NULL;
	pcache->next = NULL;
	pcache->bucket = NULL;
	s_pFirstProp = pcache;
	pcache->m_Flags = 0; // must set this to zero so that this cache entry will be invalid.
	pcache->m_pEnvCubemapTexture = FindEnvCubemapForPoint( origin );
	BuildStaticLightingCacheLightStyleInfo( pcache );
	return (LightCacheHandle_t)pcache;
}


//-----------------------------------------------------------------------------
// Clears the prop lighting cache
//-----------------------------------------------------------------------------
void ClearStaticLightingCache()
{
	s_PropCache.Clear();
	s_pFirstProp = NULL;
}


//-----------------------------------------------------------------------------
// Recomputes all static prop lighting
//-----------------------------------------------------------------------------
void RecomputeStaticLighting()
{
	lightcache_t* pcache = s_pFirstProp;
	for( ; pcache; pcache = pcache->lru_next )
	{
		// Compute the static lighting
		PropLightcache_t* pPropCache = (PropLightcache_t*)pcache;
		pPropCache->m_Flags = 0;
		LightingState_t dummyLightingState;
		LightcacheGet( ( LightCacheHandle_t )pcache, dummyLightingState, LIGHTCACHEFLAGS_STATIC );
	}
}

static void ZeroLightingState( LightingState_t *pLightingState )
{
	int i;
	for( i = 0; i < 6; i++ )
	{
		pLightingState->r_boxcolor[i].Init();
		pLightingState->totalboxcolor[i].Init();
	}
	for( i = 0; i < MAXLOCALLIGHTS; i++ )
	{
		pLightingState->locallight[i] = NULL;
	}
	pLightingState->numlights = 0;
}

//-----------------------------------------------------------------------------
// Gets the lightcache entry for a static prop
//-----------------------------------------------------------------------------
ITexture *LightcacheGet( LightCacheHandle_t cache, LightingState_t& lightingState, 
				    unsigned int flags )
{
	PropLightcache_t *pcache = ( PropLightcache_t * )cache;
	Assert( pcache );

/*
	if( IdentifyLightingErrors( pcache->leaf, lightingState ) )
		return false;
*/

	bool bRecalcStaticLighting = false;
	bool bRecalcLightStyles = false;
	bool bRecalcDLights = false;

	if( flags != pcache->m_Flags )
	{
		// This should happen often, but if the flags change, blow away all of the lighting state
		// in this cache entry.
		pcache->m_Flags = flags;
		bRecalcStaticLighting = true;
		bRecalcLightStyles = true;
		bRecalcDLights = true;
	}
	else if( pcache->m_LastFrameUpdated == r_framecount )
	{
		// do nothing. . . get out of here since we already did this this frame.
		lightingState = pcache->m_DynamicLightingState;
		return pcache->m_pEnvCubemapTexture;
	}
	else if( !IsCachedLightStylesValid( pcache ) )
	{
		bRecalcLightStyles = true;
	}

	if( bRecalcLightStyles )
	{
		// Since the dlight cache includes lightstyles, we have to recacl the dlight cache if lightstyles change.
		bRecalcDLights = true;
	}
	
	// FIXME: We recalc these all the time, which is okay for HL2 usage patterns.
	// Do we need to keep frame-to-frame coherency here?
	bRecalcDLights = true;
	
	pcache->m_LastFrameUpdated = r_framecount;
	
	byte *pVis = NULL;
	if( flags & LIGHTCACHEFLAGS_STATIC )
	{
		if( bRecalcStaticLighting )
		{
			pVis = ComputeStaticLightingForCacheEntry( pcache, pcache->m_Origin, pcache->leaf, true );
		}
	}
	else
	{
		ZeroLightingState( &pcache->m_StaticLightingState );
	}

	if( flags & LIGHTCACHEFLAGS_LIGHTSTYLE )
	{
		if( bRecalcLightStyles )
		{
			memcpy( &pcache->m_LightStyleLightingState, &pcache->m_StaticLightingState, sizeof( LightingState_t ) );
			// We will add the lightstyles to the static lighting in place, so copy.
			AddLightStylesForStaticProp( pcache, pcache->m_LightStyleLightingState );
	 		g_EngineStats.IncrementCountedStat( ENGINE_STATS_LIGHTCACHE_STATICPROP_LIGHTSTYLE_MISSES, 1 );
		}
	}
	else
	{
		memcpy( &pcache->m_LightStyleLightingState, &pcache->m_StaticLightingState, sizeof( LightingState_t ) );
	}

	if( flags & LIGHTCACHEFLAGS_DYNAMIC )
	{
		if( bRecalcDLights )
		{
			memcpy( &pcache->m_DynamicLightingState, &pcache->m_LightStyleLightingState, sizeof( LightingState_t ) );
			AddDLightsForStaticProps( *( LightingStateInfo_t *)pcache, pcache->m_DynamicLightingState, pcache );
		}
	}
	else
	{
		memcpy( &pcache->m_DynamicLightingState, &pcache->m_LightStyleLightingState, sizeof( LightingState_t ) );
	}
	
	memcpy( &lightingState, &pcache->m_DynamicLightingState, sizeof( LightingState_t ) );
	
	return pcache->m_pEnvCubemapTexture;
}


//-----------------------------------------------------------------------------
// Get or create the lighting information for this point
// This is the version for dynamic objects.
//-----------------------------------------------------------------------------
ITexture *LightcacheGet( const Vector& origin, LightingState_t& lightingState )
{
	// Initialize the lighting cache, if necessary
	if (cached_r_worldlights != r_worldlights.GetInt() ||
		cached_r_radiosity != r_radiosity.GetInt() ||
		cached_r_avglight != r_avglight.GetInt() ||
		cached_mat_fullbright != mat_fullbright.GetInt() )
	{
		R_StudioInitLightingCache();
	}

	// generate the hashing vars
	int leaf = CM_PointLeafnum(origin);

/*
	if (IdentifyLightingErrors(leaf, lightingState))
		return false;
*/

	int x = ((unsigned int)origin[0]) >> HASH_GRID_SIZEX;
	int y = ((unsigned int)origin[1]) >> HASH_GRID_SIZEY;
	int z = ((unsigned int)origin[2]) >> HASH_GRID_SIZEZ;

	// convert vars to hash key / bucket id
	int bucket = LightcacheHashKey( x, y, z, leaf );

	// See if we've already computed the light in this location
	lightcache_t *pcache = FindInCache(bucket, x, y, z, leaf);
	if ( pcache && IsCachedLightStylesValid( pcache ) )
	{
		// move to tail of LRU
		LightcacheMark( pcache );

		// Given a cache entry, set the lighting state
		// which we'll do by overriding the cached values (which include static
		// lighting only) with dynamic lights if they affect this area.
		AddDynamicLighting( pcache, lightingState, origin, leaf );

		if (r_lightdebug.GetInt() == 1)
		{
			DebugRenderLightcache( x, y, z, lightingState );
		}

		return pcache->m_pEnvCubemapTexture;
	}

 	g_EngineStats.IncrementCountedStat( ENGINE_STATS_LIGHTCACHE_MISSES, 1 );

	if (!pcache)
	{
		// Try to find a new lightcache sample that was generated this frame
		// that's close enough to be used temporarily	
		pcache = CheckFrameCache( x, y, z, leaf );
		if (pcache)
		{
			LightcacheMark( pcache );
			AddDynamicLighting( pcache, lightingState, origin, pcache->leaf );
			return pcache->m_pEnvCubemapTexture;
		}
	}

	if (!pcache)
	{
		// If there's nothing appropriate from the frame cache, make a new entry
		pcache = NewLightcacheEntry(bucket);

		// initialize this to point to our current origin
		pcache->x = x;
		pcache->y = y;
		pcache->z = z;
		pcache->leaf = leaf;
	}
	else
	{
		LightcacheMark( pcache );
	}

	// Figure out which env_cubemap is used for this cache entry.
	pcache->m_pEnvCubemapTexture = FindEnvCubemapForPoint( origin );
	
	// Compute the static portion of the cache
	byte* pVis = ComputeStaticLightingForCacheEntry( pcache, origin, leaf );

	// Given a cache entry, set the lighting state
	// which we'll do by overriding the cached values (which include static
	// lighting only) with dynamic lights if they affect this area.
	AddDynamicLighting( pcache, lightingState, origin, 0, pVis );

	if (r_lightdebug.GetInt() == 2)
	{
		DebugRenderLightcache( x, y, z, lightingState );
	}

	return pcache->m_pEnvCubemapTexture;
}



//-----------------------------------------------------------------------------
// This is the version for large dynamic objects.
//-----------------------------------------------------------------------------
ITexture *LightcacheGet( const Vector& origin, const Vector &mins, const Vector &maxs,LightingState_t& lightingState )
{
	return NULL;
}


//-----------------------------------------------------------------------------
// Compute the comtribution of D- and E- lights at a point + normal
//-----------------------------------------------------------------------------
void ComputeDynamicLighting( const Vector& pt, const Vector* pNormal, Vector& color )
{
	// Next, add each world light with a lightstyle into the lighting state,
	// ejecting less relevant local lights + folding them into the ambient cube
	static Vector ambientTerm[6] = 
	{
		Vector(0,0,0),
		Vector(0,0,0),
		Vector(0,0,0),
		Vector(0,0,0),
		Vector(0,0,0),
		Vector(0,0,0)
	};

	int lightCount = 0;
	LightDesc_t* pLightDesc = (LightDesc_t*)stackalloc( (MAX_DLIGHTS + MAX_ELIGHTS) * sizeof(LightDesc_t) );

	int i;
	dlight_t* dl = cl_dlights;
	for ( i=0; i<MAX_DLIGHTS; ++i, ++dl )
	{
		// If the light's not active, then continue
		if ( (r_dlightactive & (1 << i)) == 0 )
			continue;

		// If the light doesn't affect models, then continue
		if (dl->flags & (DLIGHT_NO_MODEL_ILLUMINATION | DLIGHT_DISPLACEMENT_MASK))
			continue;

		// Construct a world light representing the dynamic light
		// we're making a static list here because the lighting state
		// contains a set of pointers to dynamic lights
		dworldlight_t worldLight;
		WorldLightFromDynamicLight( *dl, worldLight );
		WorldLightToMaterialLight( &worldLight, pLightDesc[lightCount] );
		++lightCount;
	}

	// Next, add each world light with a lightstyle into the lighting state,
	// ejecting less relevant local lights + folding them into the ambient cube
	dl = cl_elights;
	for ( i=0; i<MAX_ELIGHTS; ++i, ++dl )
	{
		// If the light's not active, then continue
		if ( dl->radius == 0 )
			continue;

		// If the light doesn't affect models, then continue
		if (dl->flags & (DLIGHT_NO_MODEL_ILLUMINATION | DLIGHT_DISPLACEMENT_MASK))
			continue;

		// Construct a world light representing the dynamic light
		// we're making a static list here because the lighting state
		// contains a set of pointers to dynamic lights
		dworldlight_t worldLight;
		WorldLightFromDynamicLight( *dl, worldLight );
		WorldLightToMaterialLight( &worldLight, pLightDesc[lightCount] );
		++lightCount;
	}

	g_pStudioRender->ComputeLighting( ambientTerm, lightCount,
		pLightDesc, pt, *pNormal, color );

	stackfree(pLightDesc);
}


//-----------------------------------------------------------------------------
// Is Dynamic Light?
//-----------------------------------------------------------------------------
static bool IsDynamicLight( dworldlight_t *pWorldLight )
{
	// NOTE: This only works because we're using some implementation details
	// that the dynamic lights are stored in a little static array
	return ( pWorldLight >= s_pDynamicLight && pWorldLight < &s_pDynamicLight[MAX_DLIGHTS] );
}


//-----------------------------------------------------------------------------
// Computes an average color (of sorts) at a particular point + optional normal
//-----------------------------------------------------------------------------
void ComputeLighting( const Vector& pt, const Vector* pNormal, bool bClamp, Vector& color )
{
	LightingState_t lightingState;
	LightcacheGet( pt, lightingState );

	int i;
	if ( pNormal )
	{
		LightDesc_t* pLightDesc = (LightDesc_t*)stackalloc( lightingState.numlights * sizeof(LightDesc_t) );

		for ( i=0; i < lightingState.numlights; ++i )
		{
			// Construct a world light representing the dynamic light
			// we're making a static list here because the lighting state
			// contains a set of pointers to dynamic lights
			WorldLightToMaterialLight( lightingState.locallight[i], pLightDesc[i] );
		}

		g_pStudioRender->ComputeLighting( lightingState.r_boxcolor, lightingState.numlights,
			pLightDesc, pt, *pNormal, color );
	}
	else
	{
		Vector direction;

		for ( i = 0; i < lightingState.numlights; ++i )
		{
			if ( IsDynamicLight( lightingState.locallight[i] ) )
				continue;

			float ratio = LightIntensityAndDirectionAtPoint( lightingState.locallight[i], pt, LIGHT_NO_OCCLUSION_CHECK, NULL, &direction );
			float angularRatio = Engine_WorldLightAngle( lightingState.locallight[i], lightingState.locallight[i]->normal, direction, direction );
			AddWorldLightToLightCube( lightingState.locallight[i], lightingState.r_boxcolor, direction, ratio * angularRatio );
		}

		color.Init( 0, 0, 0 );
		for ( i = 0; i < 6; ++i )
		{
			color += lightingState.r_boxcolor[i];
		}
		color /= 6.0f;
	}

	if (bClamp)
	{
		if (color.x > 1.0f)
			color.x = 1.0f;
		if (color.y > 1.0f)
			color.y = 1.0f;
		if (color.z > 1.0f)
			color.z = 1.0f;
	}
}


#if 0
// This is used to measure the effectiveness of the bucketing/hashing function
void LightcacheMetric_f( void )
{
	int counts[MAX_CACHE_BUCKETS];
	int total = 0;
	int totalEmpty = 0;
	int maxBucket = 0;

	for ( int i = 0; i < MAX_CACHE_BUCKETS; i++ )
	{
		lightcache_t *pcache = lightbuckets[i];
		int count = 0;
		while ( pcache )
		{
			count++;
			pcache = pcache->next;
		}
		total += count;
		counts[i] = count;
		if ( !count )
			totalEmpty++;
		if ( count > maxBucket )
			maxBucket = count;
	}

	Con_Printf("Light; %d in cache, (%d/%d used) %d max\n", total, MAX_CACHE_BUCKETS-totalEmpty, MAX_CACHE_BUCKETS, maxBucket );
}
#endif

static byte *s_pDLightVis = NULL;

extern int r_dlightframecount; // hack

// All dlights that affect a static prop must mark that static prop every frame.
class MarkStaticPropLightsEmumerator : public IPartitionEnumerator
{
public:
	void SetLightID( int nLightID )
	{
		m_nLightID = nLightID;
	}

	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		Assert( StaticPropMgr()->IsStaticProp( pHandleEntity ) );

		PropLightcache_t *pCache = 
			( PropLightcache_t * )StaticPropMgr()->GetLightCacheHandleForStaticProp( pHandleEntity );
		
		if( !s_pDLightVis )
		{
			s_pDLightVis = CM_ClusterPVS( CM_LeafCluster( CM_PointLeafnum( cl_dlights[m_nLightID].origin ) ) );
		}

		if( !StaticPropMgr()->IsPropInPVS( pHandleEntity, s_pDLightVis ) )
		{
			return ITERATION_CONTINUE;
		}

#ifdef _DEBUG
		if( r_lightdebug.GetInt() == 4 )
		{
			Vector mins( -5, -5, -5 );
			Vector maxs( 5, 5, 5 );
			CDebugOverlay::AddLineOverlay( cl_dlights[m_nLightID].origin, pCache->m_Origin, 0, 0, 255, true, 0.001f );
			CDebugOverlay::AddBoxOverlay( pCache->m_Origin, mins, maxs, vec3_angle, 255, 0, 0, 0, 0.001f );
		}
#endif
		
		if( pCache->m_DLightFrame != r_dlightframecount )
		{
			pCache->m_DLightFrame = r_dlightframecount;
			pCache->m_DLightActive = ( 1 << m_nLightID );
		}
		else
		{
			pCache->m_DLightActive |= ( 1 << m_nLightID );
		}
		return ITERATION_CONTINUE;
	}

private:
	int m_nLightID;
};

static MarkStaticPropLightsEmumerator s_MarkStaticPropLightsEnumerator;

void MarkDLightsOnStaticProps( void )
{
	dlight_t	*l = cl_dlights;
	for (int i=0 ; i<MAX_DLIGHTS ; i++, l++)
	{
		if (l->flags & (DLIGHT_NO_MODEL_ILLUMINATION | DLIGHT_DISPLACEMENT_MASK))
			continue;
		if (l->die < cl.gettime() || !l->radius)
			continue;
		// If the light's not active, then continue
		if ( (r_dlightactive & (1 << i)) == 0 )
			continue;
		
#ifdef _DEBUG
		if( r_lightdebug.GetInt() == 4 )
		{
			Vector mins( -5, -5, -5 );
			Vector maxs( 5, 5, 5 );
			CDebugOverlay::AddBoxOverlay( l->origin, mins, maxs, vec3_angle, 255, 255, 255, 0, 0.001f );
		}
#endif
		s_pDLightVis = NULL;
		s_MarkStaticPropLightsEnumerator.SetLightID( i );
		SpatialPartition()->EnumerateElementsInSphere( PARTITION_ENGINE_STATIC_PROPS, 
			l->origin, l->radius, true, &s_MarkStaticPropLightsEnumerator );
	}
}

