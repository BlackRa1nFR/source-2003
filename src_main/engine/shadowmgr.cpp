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
// Interface to the engine system responsible for dealing with shadows
//=============================================================================

#include "ShadowMgr.h"
#include "utllinkedlist.h"
#include "utlvector.h"
#include "interface.h"
#include "VMatrix.h"
#include "BSPTreeData.h"
#include "tier0/dbg.h"
#include "materialsystem/IMaterial.h"
#include "gl_model_private.h"
#include "gl_rsurf.h"
#include "gl_matsysiface.h"
#include "materialsystem/IMesh.h"
#include "ConVar.h"
#include "GLQuake.h"
#include "EngineStats.h"
#include "UtlBidirectionalSet.h"
#include "l_studio.h"
#include "IStudioRender.h"
#include "engine/IVModelRender.h"
#include "collisionutils.h"
#include "debugoverlay.h"
#include "tier0/vprof.h"


//-----------------------------------------------------------------------------
// Shadow-related functionality exported by the engine
//
// We have two shadow-related caches in this system
//	1) A surface cache. We keep track of which surfaces the shadows can 
//		potentially hit. The computation of the surface cache should be
//		as fast as possible
//	2) A surface vertex cache. Once we know what surfaces the shadow
//		hits, we caompute the actual polygons using a clip. This is only
//		useful for shadows that we know don't change too frequently, so
//		we pass in a flag when making the shadow to indicate whether the
//		vertex cache should be used or not. The assumption is that the client
//		of this system should know whether the shadows are always changing or not
//
//	The first cache is generated when the shadow is initially projected, and
//  the second cache is generated when the surfaces are actually being rendered.
//
// For rendering, I assign a sort order ID to all materials used by shadow
// decals. The sort order serves the identical purpose to the material's EnumID
// but I remap those IDs so I can keep a small list of decals to render with
// that enum ID (the other option would be to allocate an array with a number
// of elements == to the number of material enumeration IDs, which is pretty large).
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// forward decarations
//-----------------------------------------------------------------------------
extern int r_surfacevisframe;
extern IStudioRender *g_pStudioRender;


#define BACKFACE_EPSILON	0.01f


// Max number of vertices per shadow decal
enum
{
	SHADOW_VERTEX_SMALL_CACHE_COUNT = 8,
	SHADOW_VERTEX_LARGE_CACHE_COUNT = 32,
	SHADOW_VERTEX_TEMP_COUNT = 48,
	MAX_CLIP_PLANE_COUNT = 4
};


//-----------------------------------------------------------------------------
// Used to clip the shadow decals
//-----------------------------------------------------------------------------
struct ShadowClipState_t
{
	int m_CurrVert;
	int	m_TempCount;
	int	m_ClipCount;
	ShadowVertex_t	m_pTempVertices[SHADOW_VERTEX_TEMP_COUNT];
	ShadowVertex_t*	m_ppClipVertices[2][SHADOW_VERTEX_TEMP_COUNT];
};

//-----------------------------------------------------------------------------
// Implementation of IShadowMgr
//-----------------------------------------------------------------------------
class CShadowMgr : public IShadowMgrInternal, ISpatialLeafEnumerator
{
public:
	// constructor
	CShadowMgr();

	// Methods inherited from IShadowMgr
	virtual ShadowHandle_t CreateShadow( IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, int creationFlags );
	virtual void DestroyShadow( ShadowHandle_t handle );
	virtual void SetShadowMaterial( ShadowHandle_t handle, IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy );
	virtual void EnableShadow( ShadowHandle_t handle, bool bEnable );
	virtual void ProjectShadow( ShadowHandle_t handle, const Vector& origin,
		const Vector& projectionDir, const VMatrix& worldToShadow, const Vector2D& size,
		float fallofCoeff, float falloffOffset, float falloffAmount );
	virtual int ProjectAndClipVertices( ShadowHandle_t handle, int count, 
		Vector** ppPosition, ShadowVertex_t*** ppOutVertex );

	virtual void AddShadowToBrushModel( ShadowHandle_t handle, model_t* pModel, 	
										const Vector& origin, const QAngle& angles );
	virtual void RemoveAllShadowsFromBrushModel( model_t* pModel );
	virtual void AddShadowToModel( ShadowHandle_t shadow, ModelInstanceHandle_t handle );
	virtual void RemoveAllShadowsFromModel( ModelInstanceHandle_t handle );
 	virtual const ShadowInfo_t& GetInfo( ShadowHandle_t handle );

	// Methods inherited from IShadowMgrInternal
	virtual void AddShadowsOnSurfaceToRenderList( ShadowDecalHandle_t decalHandle );
	virtual void RenderShadows( const VMatrix* pModelToWorld );
	virtual void ClearShadowRenderList();
	virtual unsigned char ComputeDarkness( float z, const ShadowInfo_t& info ) const;
	virtual void SetModelShadowState( ModelInstanceHandle_t instance );
	virtual unsigned short InvalidShadowIndex( );

	// Methods of ISpatialLeafEnumerator
	virtual bool EnumerateLeaf( int leaf, int context );

	// Sets the texture coordinate range for a shadow...
	virtual void SetShadowTexCoord( ShadowHandle_t handle, float x, float y, float w, float h );

	// Set extra clip planes related to shadows...
	// These are used to prevent pokethru and back-casting
	virtual void ClearExtraClipPlanes( ShadowHandle_t shadow );
	virtual void AddExtraClipPlane( ShadowHandle_t shadow, const Vector& normal, float dist );

	// Gets the first model associated with a shadow
	unsigned short& FirstModelInShadow( ShadowHandle_t h ) { return m_Shadows[h].m_FirstModel; }
	
	// Set the darkness falloff bias
	virtual void SetFalloffBias( ShadowHandle_t shadow, unsigned char ucBias );

private:
	enum
	{
		SHADOW_DISABLED = (SHADOW_LAST_FLAG << 1),
	};

	struct ShadowVertexSmallList_t
	{
		ShadowVertex_t	m_Verts[SHADOW_VERTEX_SMALL_CACHE_COUNT];
	};

	struct ShadowVertexLargeList_t
	{
		ShadowVertex_t	m_Verts[SHADOW_VERTEX_LARGE_CACHE_COUNT];
	};

	// A cache entries' worth of vertices....
	struct ShadowVertexCache_t
	{
		unsigned short	m_Count;
		ShadowHandle_t	m_Shadow;
		unsigned short	m_CachedVerts;
		ShadowVertex_t*	m_pVerts;
	};

	// Shadow state
	struct Shadow_t : public ShadowInfo_t
	{
		Vector			m_ProjectionDir;
		IMaterial*		m_pMaterial;		// material for rendering surfaces
		IMaterial*		m_pModelMaterial;	// material for rendering models
		void*			m_pBindProxy;
		int				m_Flags;
		unsigned short	m_SortOrder;

		// Extra clip planes
		unsigned short	m_ClipPlaneCount;
		Vector			m_ClipPlane[MAX_CLIP_PLANE_COUNT];
		float			m_ClipDist[MAX_CLIP_PLANE_COUNT];

		// First model the shadow is projected onto
		unsigned short	m_FirstModel;
		
		// First shadow decal the shadow has
		unsigned short	m_FirstDecal;
	};

	// Each surface has one of these, they reference the main shadow
	// projector and cached off shadow decals.
	struct ShadowDecal_t
	{
		int					m_SurfID;
		ShadowHandle_t		m_Shadow;
		DispShadowHandle_t	m_DispShadow;
		unsigned short		m_ShadowVerts;
		unsigned short		m_ShadowListIndex;

		// This is a handle of the next shadow decal to be rendered
		ShadowDecalHandle_t	m_NextRender;
	};

	// This structure is used when building new shadow information
	struct ShadowBuildInfo_t
	{
		ShadowHandle_t	m_Shadow;
		Ray_t*			m_pRay;
		Vector			m_ProjectionDirection;
	};

	// This structure contains rendering information
	struct ShadowRenderInfo_t
	{
		int		m_VertexCount;
		int		m_IndexCount;
		int		m_Count;
		int*	m_pCache;
		int		m_DispCount;
		const VMatrix* m_pModelToWorld;
		DispShadowHandle_t*	m_pDispCache;
	};

	// Structures used to assign sort order handles
	struct SortOrderInfo_t
	{
		int	m_MaterialEnum;
		int	m_RefCount;
	};

	typedef void (*ShadowDebugFunc_t)( ShadowHandle_t shadowHandle, const Vector &vecCentroid );

private:
	// These functions deal with creation of render sort ids
	void SetMaterial( Shadow_t& shadow, IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy );
	void CleanupMaterial( Shadow_t& shadow );

	// These functions add/remove shadow decals to surfaces
	ShadowDecalHandle_t AddShadowDecalToSurface( int surfID, ShadowHandle_t handle );
	void RemoveShadowDecalFromSurface( int surfID, ShadowDecalHandle_t decalHandle );

	// Adds the surface to the list for this shadow
	void AddDecalToShadowList( ShadowHandle_t handle, ShadowDecalHandle_t decalHandle );

	// Removes the shadow to the list of surfaces
	void RemoveDecalFromShadowList( ShadowHandle_t handle, ShadowDecalHandle_t decalHandle );

	// Actually projects + clips vertices
	int ProjectAndClipVertices( const Shadow_t& shadow, const VMatrix& worldToShadow, 
		int count, Vector** ppPosition, ShadowVertex_t*** ppOutVertex );

	// These functions hook/unhook shadows up to surfaces + vice versa
	ShadowDecalHandle_t AddSurfaceToShadow( ShadowHandle_t handle, int surfID );
	void RemoveSurfaceFromShadow( ShadowHandle_t handle, int surfID );
	void RemoveAllSurfacesFromShadow( ShadowHandle_t handle );
	void RemoveAllShadowsFromSurface( int surfID );

	// Deals with model shadow management
	void RemoveAllModelsFromShadow( ShadowHandle_t handle );

	// Applies the shadow to a surface
	void ApplyShadowToSurface( ShadowBuildInfo_t& build, int surfID );

	// Applies the shadow to a displacement
	void ApplyShadowToDisplacement( ShadowBuildInfo_t& build, IDispInfo *pDispInfo );

	// Renders shadows that all share a material enumeration
	void RenderShadowList( ShadowDecalHandle_t decalHandle, const VMatrix* pModelToWorld );

	// Should we cache vertices?
	bool ShouldCacheVertices( const ShadowDecal_t& decal );

	// Generates a list displacement shadow vertices to render
	bool GenerateDispShadowRenderInfo( ShadowDecal_t& decal, ShadowRenderInfo_t& info );

	// Generates a list shadow vertices to render
	bool GenerateNormalShadowRenderInfo( ShadowDecal_t& decal, ShadowRenderInfo_t& info );

	// Generates a list shadow vertices to render
	void GenerateShadowRenderInfo( ShadowDecalHandle_t decalHandle, ShadowRenderInfo_t& info );

	// Adds normal shadows to the mesh builder
	int AddNormalShadowsToMeshBuilder( CMeshBuilder& meshBuilder, ShadowRenderInfo_t& info );

	// Adds displacement shadows to the mesh builder
	int AddDisplacementShadowsToMeshBuilder( CMeshBuilder& meshBuilder, 
									ShadowRenderInfo_t& info, int baseIndex );

	// Does the actual work of computing shadow vertices
	bool ComputeShadowVertices( ShadowDecal_t& decal, const VMatrix* pModelToWorld, ShadowVertexCache_t* pVertexCache );

	// Project vertices into shadow space
	bool ProjectVerticesIntoShadowSpace( const VMatrix& modelToShadow, float maxDist,
		int count, Vector** ppPosition, ShadowClipState_t& clip );

	// Copies vertex info from the clipped vertices
	void CopyClippedVertices( int count, ShadowVertex_t** ppSrcVert, ShadowVertex_t* pDstVert );

	// Allocate, free vertices
	ShadowVertex_t* AllocateVertices( ShadowVertexCache_t& cache, int count );
	void FreeVertices( ShadowVertexCache_t& cache );

	// Gets at cache entry...
	ShadowVertex_t* GetCachedVerts( const ShadowVertexCache_t& cache );

	// Clears out vertices in the temporary cache
	void ClearTempCache( );

	// Renders debugging information
	void RenderDebuggingInfo( const ShadowRenderInfo_t &info, ShadowDebugFunc_t func );


private:
	// List of all shadows (one per cast shadow)
	CUtlLinkedList<Shadow_t, ShadowHandle_t> m_Shadows;
	
	// List of all shadow decals (one per surface hit by a shadow)
	CUtlLinkedList<ShadowDecal_t, ShadowDecalHandle_t> m_ShadowDecals;

	// List of all shadow decals associated with a particular shadow
	CUtlLinkedList<ShadowDecalHandle_t, unsigned short> m_ShadowSurfaces;

	// List of queued decals waiting to be rendered....
	CUtlVector<ShadowDecalHandle_t>	m_RenderQueue;

	// Used to assign sort order handles
	CUtlLinkedList<SortOrderInfo_t, unsigned short>	m_SortOrderIds;

	// A cache of shadow vertex data...
	CUtlLinkedList<ShadowVertexCache_t, unsigned short>	m_VertexCache;

	// This is temporary, not saved off....
	CUtlVector<ShadowVertexCache_t>	m_TempVertexCache;

	// Vertex data
	CUtlLinkedList<ShadowVertexSmallList_t, unsigned short>	m_SmallVertexList;
	CUtlLinkedList<ShadowVertexLargeList_t, unsigned short>	m_LargeVertexList;

	// Model-shadow association
	CBidirectionalSet< ModelInstanceHandle_t, ShadowHandle_t, unsigned short >	m_ShadowsOnModels;

	// The number of decals we're gonna need to render
	int	m_DecalsToRender;
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CShadowMgr s_ShadowMgr;
IShadowMgrInternal* g_pShadowMgr = &s_ShadowMgr;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CShadowMgr, IShadowMgr, 
			ENGINE_SHADOWMGR_INTERFACE_VERSION, s_ShadowMgr);


//-----------------------------------------------------------------------------
// ConVar
//-----------------------------------------------------------------------------
ConVar r_shadows("r_shadows", "1");
static ConVar r_shadowwireframe("r_shadowwireframe", "0" );
static ConVar r_shadowids("r_shadowids", "0" );


//-----------------------------------------------------------------------------
// Shadows on model instances
//-----------------------------------------------------------------------------
unsigned short& FirstShadowOnModel( ModelInstanceHandle_t h )
{
	// See l_studio.cpp
	return FirstShadowOnModelInstance( h );
}

unsigned short& FirstModelInShadow( ShadowHandle_t h )
{
	return s_ShadowMgr.FirstModelInShadow(h); 
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CShadowMgr::CShadowMgr()
{
	m_ShadowsOnModels.Init( ::FirstShadowOnModel, ::FirstModelInShadow );
	ClearShadowRenderList();
}

//-----------------------------------------------------------------------------
// Create, destroy material sort order ids...
//-----------------------------------------------------------------------------
void CShadowMgr::SetMaterial( Shadow_t& shadow, IMaterial* pMaterial, IMaterial* pModelMaterial, void *pBindProxy )
{
	shadow.m_pMaterial = pMaterial;
	shadow.m_pModelMaterial = pModelMaterial;
	shadow.m_pBindProxy = pBindProxy;

	// We're holding onto this material
	pMaterial->IncrementReferenceCount();
	pModelMaterial->IncrementReferenceCount();

	// Search the sort order handles for an enumeration id match
	int materialEnum = (int)pMaterial;
	for (unsigned short i = m_SortOrderIds.Head(); i != m_SortOrderIds.InvalidIndex();
		i = m_SortOrderIds.Next(i) )
	{
		// Found a match, lets increment the refcount of this sort order id
		if (m_SortOrderIds[i].m_MaterialEnum == materialEnum)
		{
			++m_SortOrderIds[i].m_RefCount;
			shadow.m_SortOrder = i;
			return;
		}
	}

	// Didn't find it, lets assign a new sort order ID, with a refcount of 1
	shadow.m_SortOrder = m_SortOrderIds.AddToTail();
	m_SortOrderIds[shadow.m_SortOrder].m_MaterialEnum  = materialEnum;
	m_SortOrderIds[shadow.m_SortOrder].m_RefCount = 1;

	// Make sure the render queue has as many entries as the max sort order id.
	int count = m_RenderQueue.Count();
	while( count < m_SortOrderIds.MaxElementIndex() )
	{
		m_RenderQueue.AddToTail( SHADOW_DECAL_HANDLE_INVALID );
		++count;
	}
}

void CShadowMgr::CleanupMaterial( Shadow_t& shadow )
{
	// Decrease the sort order reference count
	if (--m_SortOrderIds[shadow.m_SortOrder].m_RefCount <= 0)
	{
		// No one referencing the sort order number?
		// Then lets clean up the sort order id
		m_SortOrderIds.Remove(shadow.m_SortOrder);
	}

	// We're done with this material
	shadow.m_pMaterial->DecrementReferenceCount();
	shadow.m_pModelMaterial->DecrementReferenceCount();
}


//-----------------------------------------------------------------------------
// For the model shadow list
//-----------------------------------------------------------------------------
unsigned short CShadowMgr::InvalidShadowIndex( )
{
	return m_ShadowsOnModels.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Create, destroy shadows
//-----------------------------------------------------------------------------
ShadowHandle_t CShadowMgr::CreateShadow( IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, int creationFlags )
{
	ShadowHandle_t h = m_Shadows.AddToTail();

	Shadow_t& shadow = m_Shadows[h];
	SetMaterial( shadow, pMaterial, pModelMaterial, pBindProxy );
	shadow.m_Flags = creationFlags;
	shadow.m_FirstDecal = m_ShadowDecals.InvalidIndex();
	shadow.m_FirstModel = m_ShadowsOnModels.InvalidIndex(); 
	shadow.m_ProjectionDir.Init( 0, 0, 1 );
	shadow.m_TexOrigin.Init( 0, 0 );
	shadow.m_TexSize.Init( 1, 1 );
	shadow.m_ClipPlaneCount = 0;
	shadow.m_FalloffBias = 0;
	MatrixSetIdentity( shadow.m_WorldToShadow );
	return h;
}

void CShadowMgr::DestroyShadow( ShadowHandle_t handle )
{
	CleanupMaterial( m_Shadows[handle] );
	RemoveAllSurfacesFromShadow( handle );
	RemoveAllModelsFromShadow( handle );
	m_Shadows.Remove(handle);
}


//-----------------------------------------------------------------------------
// Resets the shadow material (useful for shadow LOD.. doing blobby at distance) 
//-----------------------------------------------------------------------------
void CShadowMgr::SetShadowMaterial( ShadowHandle_t handle, IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy )
{
	Shadow_t& shadow = m_Shadows[handle];
	if ( (shadow.m_pMaterial != pMaterial) || (shadow.m_pModelMaterial != pModelMaterial) || (shadow.m_pBindProxy != pBindProxy) )
	{
		CleanupMaterial( shadow );
		SetMaterial( shadow, pMaterial, pModelMaterial, pBindProxy );
	}
}


//-----------------------------------------------------------------------------
// Sets the texture coordinate range for a shadow...
//-----------------------------------------------------------------------------
void CShadowMgr::SetShadowTexCoord( ShadowHandle_t handle, float x, float y, float w, float h )
{
	Shadow_t& shadow = m_Shadows[handle];
	shadow.m_TexOrigin.Init( x, y );
	shadow.m_TexSize.Init( w, h );
}


//-----------------------------------------------------------------------------
// Set extra clip planes related to shadows...
//-----------------------------------------------------------------------------
void CShadowMgr::ClearExtraClipPlanes( ShadowHandle_t h )
{
	m_Shadows[h].m_ClipPlaneCount = 0;
}

void CShadowMgr::AddExtraClipPlane( ShadowHandle_t h, const Vector& normal, float dist )
{
	Shadow_t& shadow = m_Shadows[h];
	Assert( shadow.m_ClipPlaneCount < MAX_CLIP_PLANE_COUNT - 1 );

	VectorCopy( normal, shadow.m_ClipPlane[shadow.m_ClipPlaneCount] );
	shadow.m_ClipDist[shadow.m_ClipPlaneCount] = dist;
	++shadow.m_ClipPlaneCount;
}


//-----------------------------------------------------------------------------
// Gets at information about a particular shadow
//-----------------------------------------------------------------------------
const ShadowInfo_t& CShadowMgr::GetInfo( ShadowHandle_t handle )
{
	return m_Shadows[handle];
}


//-----------------------------------------------------------------------------
// Gets at cache entry...
//-----------------------------------------------------------------------------
ShadowVertex_t* CShadowMgr::GetCachedVerts( const ShadowVertexCache_t& cache )
{
	if (cache.m_Count == 0)
		return 0 ;

	if (cache.m_pVerts)
		return cache.m_pVerts;

	if (cache.m_Count <= SHADOW_VERTEX_SMALL_CACHE_COUNT)
		return m_SmallVertexList[cache.m_CachedVerts].m_Verts;

	return m_LargeVertexList[cache.m_CachedVerts].m_Verts;
}

//-----------------------------------------------------------------------------
// Allocates, cleans up vertex cache vertices
//-----------------------------------------------------------------------------
inline ShadowVertex_t* CShadowMgr::AllocateVertices( ShadowVertexCache_t& cache, int count )
{
	cache.m_pVerts = 0;
	if (count <= SHADOW_VERTEX_SMALL_CACHE_COUNT)
	{
		cache.m_Count = count;
		cache.m_CachedVerts = m_SmallVertexList.AddToTail( );
		return m_SmallVertexList[cache.m_CachedVerts].m_Verts;
	}
	else if (count <= SHADOW_VERTEX_LARGE_CACHE_COUNT)
	{
		cache.m_Count = count;
		cache.m_CachedVerts = m_LargeVertexList.AddToTail( );
		return m_LargeVertexList[cache.m_CachedVerts].m_Verts;
	}

	cache.m_Count = count;
	if (count > 0)
	{
		cache.m_pVerts = new ShadowVertex_t[count];
	}
	cache.m_CachedVerts = m_LargeVertexList.InvalidIndex();
	return cache.m_pVerts;
}

inline void CShadowMgr::FreeVertices( ShadowVertexCache_t& cache )
{
	if (cache.m_Count == 0)
		return;

	if (cache.m_pVerts)
	{
		delete[] cache.m_pVerts;
	}
	else if (cache.m_Count <= SHADOW_VERTEX_SMALL_CACHE_COUNT)
	{
		m_SmallVertexList.Remove( cache.m_CachedVerts );
	}
	else
	{
		m_LargeVertexList.Remove( cache.m_CachedVerts );
	}
}


//-----------------------------------------------------------------------------
// Clears out vertices in the temporary cache
//-----------------------------------------------------------------------------
void CShadowMgr::ClearTempCache( )
{
	// Clear out the vertices
	for (int i = m_TempVertexCache.Count(); --i >= 0; ) 
	{
		FreeVertices( m_TempVertexCache[i] );
	}

	m_TempVertexCache.RemoveAll();
}


//-----------------------------------------------------------------------------
// Adds the surface to the list for this shadow
//-----------------------------------------------------------------------------
void CShadowMgr::AddDecalToShadowList( ShadowHandle_t handle, ShadowDecalHandle_t decalHandle )
{
	// Add the shadow to the list of surfaces affected by this shadow
	unsigned short idx = m_ShadowSurfaces.Alloc( true );
	m_ShadowSurfaces[idx] = decalHandle;
	if (m_Shadows[handle].m_FirstDecal != SHADOW_DECAL_HANDLE_INVALID) 
		m_ShadowSurfaces.LinkBefore( m_Shadows[handle].m_FirstDecal, idx );
	m_Shadows[handle].m_FirstDecal = idx;
	m_ShadowDecals[decalHandle].m_ShadowListIndex = idx;
}


//-----------------------------------------------------------------------------
// Removes the shadow to the list of surfaces
//-----------------------------------------------------------------------------
void CShadowMgr::RemoveDecalFromShadowList( ShadowHandle_t handle, ShadowDecalHandle_t decalHandle )
{
	int idx = m_ShadowDecals[decalHandle].m_ShadowListIndex;

	// Make sure the list of shadow decals for a single shadow is ok
	if ( m_Shadows[handle].m_FirstDecal == idx )
		m_Shadows[handle].m_FirstDecal = m_ShadowSurfaces.Next(idx);

	// Remove it from the shadow surfaces list
	m_ShadowSurfaces.Free(idx);

	// Blat out the decal index
	m_ShadowDecals[decalHandle].m_ShadowListIndex = SHADOW_DECAL_HANDLE_INVALID;
}


//-----------------------------------------------------------------------------
// Adds the shadow decal reference to the surface
//-----------------------------------------------------------------------------
inline ShadowDecalHandle_t CShadowMgr::AddShadowDecalToSurface( int surfID, ShadowHandle_t handle )
{
	ShadowDecalHandle_t decalHandle = m_ShadowDecals.Alloc( true );
	ShadowDecal_t& decal = m_ShadowDecals[decalHandle];

	decal.m_SurfID = surfID;
	m_ShadowDecals.LinkBefore( MSurf_ShadowDecals( surfID ), decalHandle );
	MSurf_ShadowDecals( surfID ) = decalHandle;

	// Hook the shadow into the displacement system....
	if (!SurfaceHasDispInfo( surfID ))
		decal.m_DispShadow = DISP_SHADOW_HANDLE_INVALID;
	else
		decal.m_DispShadow = MSurf_DispInfo( surfID )->AddShadowDecal( handle );

	decal.m_Shadow = handle;
	decal.m_ShadowVerts = m_VertexCache.InvalidIndex();
	decal.m_NextRender = SHADOW_DECAL_HANDLE_INVALID;
	decal.m_ShadowListIndex = SHADOW_DECAL_HANDLE_INVALID;

	AddDecalToShadowList( handle, decalHandle );

	return decalHandle;
}

inline void CShadowMgr::RemoveShadowDecalFromSurface( int surfID, ShadowDecalHandle_t decalHandle )
{
	// Clean up its shadow verts if it has any
	ShadowDecal_t& decal = m_ShadowDecals[decalHandle];
	if (decal.m_ShadowVerts != m_VertexCache.InvalidIndex())
	{
		FreeVertices( m_VertexCache[decal.m_ShadowVerts] );
		m_VertexCache.Remove(decal.m_ShadowVerts);
		decal.m_ShadowVerts = m_VertexCache.InvalidIndex();
	}

	// Clean up displacement...
	if ( decal.m_DispShadow != DISP_SHADOW_HANDLE_INVALID )
		MSurf_DispInfo( decal.m_SurfID )->RemoveShadowDecal( decal.m_DispShadow );

	// Make sure the list of shadow decals on a surface is set up correctly
	if ( MSurf_ShadowDecals( surfID ) == decalHandle )
		MSurf_ShadowDecals( surfID ) = m_ShadowDecals.Next(decalHandle);

	RemoveDecalFromShadowList( decal.m_Shadow, decalHandle );

	// Kill the shadow decal
	m_ShadowDecals.Free( decalHandle );
}


//-----------------------------------------------------------------------------
// Adds the shadow decal reference to the surface
// This causes a shadow decal to be made
//-----------------------------------------------------------------------------
ShadowDecalHandle_t CShadowMgr::AddSurfaceToShadow( ShadowHandle_t handle, int surfID )
{
#ifdef _DEBUG
	// Make sure the surface has the shadow on it exactly once...
	ShadowDecalHandle_t	dh = MSurf_ShadowDecals( surfID );
	while (dh != m_ShadowDecals.InvalidIndex() )
	{
		Assert ( m_ShadowDecals[dh].m_Shadow != handle );
		dh = m_ShadowDecals.Next(dh);
	}
#endif

	// Create a shadow decal for this surface and add it to the surface
	ShadowDecalHandle_t decalHandle = AddShadowDecalToSurface( surfID, handle );
	return decalHandle;
}

void CShadowMgr::RemoveSurfaceFromShadow( ShadowHandle_t handle, int surfID )
{
	// Find the decal associated with the handle that lies on the surface

	// FIXME: Linear search; bleah.
	// Luckily the search is probably over only a couple items at most
	// Linear searching over the shadow surfaces so we can remove the entry
	// in the shadow surface list if we find a match
	ASSERT_SURF_VALID( surfID );
	unsigned short i = m_Shadows[handle].m_FirstDecal;
	while (i != m_ShadowSurfaces.InvalidIndex() )
	{
		ShadowDecalHandle_t decalHandle = m_ShadowSurfaces[i];
		if ( m_ShadowDecals[decalHandle].m_SurfID == surfID )
		{
			// Found a match! There should be at most one shadow decal
			// associated with a particular shadow per surface
			RemoveShadowDecalFromSurface( surfID, decalHandle );

			// FIXME: Could check the shadow doesn't appear again in the list
			return;			
		}

		i = m_ShadowSurfaces.Next(i);
	}

#ifdef _DEBUG
	// Here, the shadow didn't have the surface in its list
	// let's make sure the surface doesn't think it's got the shadow in its list
	ShadowDecalHandle_t	dh = MSurf_ShadowDecals( surfID );
	while (dh != m_ShadowDecals.InvalidIndex() )
	{
		Assert ( m_ShadowDecals[dh].m_Shadow != handle );
		dh = m_ShadowDecals.Next(dh);
	}

#endif
}

void CShadowMgr::RemoveAllSurfacesFromShadow( ShadowHandle_t handle )
{
	// Iterate over all the decals associated with a particular shadow
	// Remove the decals from the surfaces they are associated with
	unsigned short i = m_Shadows[handle].m_FirstDecal;
	unsigned short next;
	while ( i != m_ShadowSurfaces.InvalidIndex() )
	{
		ShadowDecalHandle_t decalHandle = m_ShadowSurfaces[i];

		next = m_ShadowSurfaces.Next(i);

		RemoveShadowDecalFromSurface( m_ShadowDecals[decalHandle].m_SurfID, decalHandle );

		i = next;
	}

	m_Shadows[handle].m_FirstDecal = SHADOW_DECAL_HANDLE_INVALID;
}

void CShadowMgr::RemoveAllShadowsFromSurface( int surfID )
{
	// Iterate over all the decals associated with a particular shadow
	// Remove the decals from the surfaces they are associated with
	ShadowDecalHandle_t	dh = MSurf_ShadowDecals( surfID );
	while (dh != m_ShadowDecals.InvalidIndex() )
	{
		// Remove this shadow from the surface 
		ShadowDecalHandle_t next = m_ShadowDecals.Next(dh);

		// Remove the surface from the shadow
		RemoveShadowDecalFromSurface( m_ShadowDecals[dh].m_SurfID, dh );

		dh = next;
	}

	MSurf_ShadowDecals( surfID ) = m_ShadowDecals.InvalidIndex();
}


//-----------------------------------------------------------------------------
// Shadow/model association
//-----------------------------------------------------------------------------
void CShadowMgr::AddShadowToModel( ShadowHandle_t handle, ModelInstanceHandle_t model )
{
	// FIXME: Add culling here based on the model bbox
	// and the shadow bbox
	// FIXME:
	/*
	// Trivial bbox reject.
	Vector bbMin, bbMax;
	pDisp->GetBoundingBox( bbMin, bbMax );
	if( decalinfo->m_Position.x - decalinfo->m_Size < bbMax.x && decalinfo->m_Position.x + decalinfo->m_Size > bbMin.x && 
		decalinfo->m_Position.y - decalinfo->m_Size < bbMax.y && decalinfo->m_Position.y + decalinfo->m_Size > bbMin.y && 
		decalinfo->m_Position.z - decalinfo->m_Size < bbMax.z && decalinfo->m_Position.z + decalinfo->m_Size > bbMin.z )
	*/

	m_ShadowsOnModels.AddElementToBucket( model, handle );
}

void CShadowMgr::RemoveAllShadowsFromModel( ModelInstanceHandle_t model )
{
	m_ShadowsOnModels.RemoveBucket( model );
}

void CShadowMgr::RemoveAllModelsFromShadow( ShadowHandle_t handle )
{
	m_ShadowsOnModels.RemoveElement( handle );
}


//-----------------------------------------------------------------------------
// Shadow state...
//-----------------------------------------------------------------------------
void CShadowMgr::SetModelShadowState( ModelInstanceHandle_t instance )
{
#ifndef SWDS
	g_pStudioRender->ClearAllShadows();
	if (instance != MODEL_INSTANCE_INVALID && r_shadows.GetInt())
	{
		unsigned short i = m_ShadowsOnModels.FirstElement( instance );
		while ( i != m_ShadowsOnModels.InvalidIndex() )
		{
			Shadow_t& shadow = m_Shadows[m_ShadowsOnModels.Element(i)];

			if (r_shadowwireframe.GetInt() == 0)
				g_pStudioRender->AddShadow( shadow.m_pModelMaterial, shadow.m_pBindProxy );
			else
				g_pStudioRender->AddShadow( g_pMaterialMRMWireframe, NULL );

			i = m_ShadowsOnModels.NextElement(i);
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Applies the shadow to a surface
//-----------------------------------------------------------------------------
void CShadowMgr::ApplyShadowToSurface( ShadowBuildInfo_t& build, int surfID )
{
	// We've found a potential surface to add to the shadow
	// At this point, we want to do fast culling to see whether we actually
	// should apply the shadow or not before actually adding it to any lists

	// FIXME: implement
	// Put the texture extents into shadow space; see if there's an intersection
	// If not, we can early out


	// To do this, we're gonna want to project the surface into the space of the decal
	// Therefore, we want to produce a surface->world transformation, and a
	// world->shadow/light space transformation
	// Then we transform the surface points into shadow space and apply the projection
	// in shadow space.

	/*
	// Get the texture associated with this surface
	mtexinfo_t* tex = pSurface->texinfo;

	Vector4D &textureU = tex->textureVecsTexelsPerWorldUnits[0];
	Vector4D &textureV = tex->textureVecsTexelsPerWorldUnits[1];

	// project decal center into the texture space of the surface
	float s = DotProduct( decalinfo->m_Position, textureU.AsVector3D() ) + 
		textureU.w - surf->textureMins[0];
	float t = DotProduct( decalinfo->m_Position, textureV.AsVector3D() ) + 
		textureV.w - surf->textureMins[1];
	*/



	// Otherwise add the shadow to the surface
	// Don't do any more computation at the moment, only do it if
	// we end up rendering the surface later on
	AddSurfaceToShadow( build.m_Shadow, surfID );
}


//-----------------------------------------------------------------------------
// Applies the shadow to a displacement
//-----------------------------------------------------------------------------
void CShadowMgr::ApplyShadowToDisplacement( ShadowBuildInfo_t& build, IDispInfo *pDispInfo )
{
	// FIXME:
	/*
	// Trivial bbox reject.
	Vector bbMin, bbMax;
	pDisp->GetBoundingBox( bbMin, bbMax );
	if( decalinfo->m_Position.x - decalinfo->m_Size < bbMax.x && decalinfo->m_Position.x + decalinfo->m_Size > bbMin.x && 
		decalinfo->m_Position.y - decalinfo->m_Size < bbMax.y && decalinfo->m_Position.y + decalinfo->m_Size > bbMin.y && 
		decalinfo->m_Position.z - decalinfo->m_Size < bbMax.z && decalinfo->m_Position.z + decalinfo->m_Size > bbMin.z )
	*/

	AddSurfaceToShadow( build.m_Shadow, pDispInfo->GetParent() );
}


//-----------------------------------------------------------------------------
// Allows us to disable particular shadows
//-----------------------------------------------------------------------------
void CShadowMgr::EnableShadow( ShadowHandle_t handle, bool bEnable )
{
	if (!bEnable)
	{
		// We need to remove the shadow from all surfaces it may currently be in
		RemoveAllSurfacesFromShadow( handle );
		RemoveAllModelsFromShadow( handle );

		m_Shadows[handle].m_Flags |= SHADOW_DISABLED;
	}
	else
	{
		// FIXME: Could make this recompute the cache...
		m_Shadows[handle].m_Flags &= ~SHADOW_DISABLED;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the darkness falloff bias
// Input  : shadow - 
//			ucBias - 
//-----------------------------------------------------------------------------
void CShadowMgr::SetFalloffBias( ShadowHandle_t shadow, unsigned char ucBias )
{
	m_Shadows[shadow].m_FalloffBias = ucBias;
}

//-----------------------------------------------------------------------------
// Recursive routine to find surface to apply a decal to.  World coordinates of 
// the decal are passed in r_recalpos like the rest of the engine.  This should 
// be called through R_DecalShoot()
//-----------------------------------------------------------------------------
void CShadowMgr::ProjectShadow( ShadowHandle_t handle, const Vector& origin,
	const Vector& projectionDir, const VMatrix& worldToShadow, const Vector2D& size,
	float maxHeight, float falloffOffset, float falloffAmount )
{
	g_EngineStats.IncrementCountedStat( ENGINE_STATS_SHADOWS_MOVED, 1 );

	// First, we need to remove the shadow from all surfaces it may
	// currently be in; in other words we're invalidating the shadow surface cache
	RemoveAllSurfacesFromShadow( handle );
	RemoveAllModelsFromShadow( handle );

	// Don't bother with this shadow if it's disabled
	if ( m_Shadows[handle].m_Flags & SHADOW_DISABLED )
		return;

	// Don't compute the surface cache if shadows are off..
	if (!r_shadows.GetInt())
		return;

	// Set the falloff coefficient
	m_Shadows[handle].m_FalloffOffset = falloffOffset;
	VectorCopy( projectionDir, m_Shadows[handle].m_ProjectionDir );

	// We need to know about surfaces in leaves hit by the ray...
	// We'd like to stop iterating as soon as the entire swept volume
	// enters a solid leaf; that may be hard to determine. Instead,
	// we should stop iterating when the ray center enters a solid leaf?
	AssertFloatEquals( projectionDir.LengthSqr(), 1.0f, 1e-3 );

	// The maximum ray distance is equal to the distance it takes the
	// falloff to get to 15%.
	m_Shadows[handle].m_MaxDist = maxHeight; //sqrt( coeff / 0.10f ) + falloffOffset;
	m_Shadows[handle].m_FalloffAmount = falloffAmount;

	Ray_t ray;
	VectorCopy( origin, ray.m_Start );
	VectorMultiply( projectionDir, m_Shadows[handle].m_MaxDist, ray.m_Delta );
	ray.m_StartOffset.Init( 0, 0, 0 );

	float maxsize = max( size.x, size.y ) * 0.5f;
	ray.m_Extents.Init( maxsize, maxsize, maxsize );
	ray.m_IsRay = false;
	ray.m_IsSwept = true;

	Shadow_t& shadow = m_Shadows[handle];
	MatrixCopy( worldToShadow, shadow.m_WorldToShadow );

	// We're hijacking the surface vis frame to make sure we enumerate
	// surfaces only once; 
	++r_surfacevisframe;

	// Clear out the displacement tags also
	DispInfo_ClearAllTags( host_state.worldmodel->brush.hDispInfos );

	ShadowBuildInfo_t build;
	build.m_Shadow = handle;
	build.m_pRay = &ray;
	VectorCopy( projectionDir, build.m_ProjectionDirection );

	g_pToolBSPTree->EnumerateLeavesAlongRay( ray, this, (int)&build );
}

bool CShadowMgr::EnumerateLeaf( int leaf, int context )
{
	ShadowBuildInfo_t* pBuild = (ShadowBuildInfo_t*)context;

	mleaf_t* pLeaf = &host_state.worldmodel->brush.leafs[leaf];

	// Iterate over all surfaces in the leaf, check for backfacing
	// and apply the shadow to the surface if it's not backfaced.
	// Note that this really only indicates that the shadow may potentially
	// sit on the surface; when we render, we'll actually do the clipping
	// computation and at that point we'll remove surfaces that don't
	// actually hit the surface
	for ( int i = 0; i < pLeaf->nummarksurfaces; i++ )
	{
		int surfID = host_state.worldmodel->brush.marksurfaces[pLeaf->firstmarksurface + i];
		
		// only process each surface once;
		if( MSurf_VisFrame( surfID ) == r_surfacevisframe )
			continue;

		MSurf_VisFrame( surfID ) = r_surfacevisframe;
		Assert( !MSurf_DispInfo( surfID ) );

		// Backface cull
		bool inFront;
		if ( (MSurf_Flags( surfID ) & SURFDRAW_NOCULL) == 0 )
		{
			if ( DotProduct(MSurf_Plane( surfID ).normal, pBuild->m_ProjectionDirection) > -BACKFACE_EPSILON )
				continue;

			inFront = true;
		}
		else
		{
			// Avoid edge-on shadows regardless.
			float dot = DotProduct(MSurf_Plane( surfID ).normal, pBuild->m_ProjectionDirection);
			if (fabs(dot) < BACKFACE_EPSILON)
				continue;

			inFront = (dot < 0); 
		}

		// Here, it's front facing...
		// Discard stuff on the wrong side of the ray start
		if (inFront)
		{
			if (DotProduct(	MSurf_Plane( surfID ).normal, pBuild->m_pRay->m_Start) < MSurf_Plane( surfID ).dist)
				continue;
		}
		else
		{
			if (DotProduct(	MSurf_Plane( surfID ).normal, pBuild->m_pRay->m_Start) > MSurf_Plane( surfID ).dist)
				continue;
		}

		ApplyShadowToSurface( *pBuild, surfID );
	}

	// Add the decal to each displacement in the leaf it touches.
	for( CDispIterator it=pLeaf->GetDispIterator(); it.IsValid(); )
	{
		CDispLeafLink *pCur = it.Inc();
		IDispInfo *pDisp = static_cast<IDispInfo*>( pCur->m_pDispInfo );

		// Make sure the decal hasn't already been added to it.
		if( pDisp->GetTag() )
			continue;
		pDisp->SetTag();

		ApplyShadowToDisplacement( *pBuild, pDisp );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Adds a shadow to a brush model
//-----------------------------------------------------------------------------
void CShadowMgr::AddShadowToBrushModel( ShadowHandle_t handle, model_t* pModel, 	
									const Vector& origin, const QAngle& angles )
{
	// Don't compute the surface cache if shadows are off..
	if (!r_shadows.GetInt())
		return;

	// Transform the shadow ray direction into model space
	Vector shadowDirInModelSpace;
	matrix3x4_t worldToModel;
	AngleIMatrix( angles, worldToModel );
	VectorRotate( m_Shadows[handle].m_ProjectionDir, worldToModel, shadowDirInModelSpace );

	// Just add all non-backfacing brush surfaces to the list of potential
	// surfaces that we may be casting a shadow onto.
	int surfID = pModel->brush.firstmodelsurface;
	for (int i=0; i<pModel->brush.nummodelsurfaces; ++i, ++surfID)
	{
		// Don't bother with nodraw surfaces
		if( MSurf_Flags( surfID ) & SURFDRAW_NODRAW )
			continue;
			
		// Don't bother with backfacing surfaces
		if ( (MSurf_Flags( surfID ) & SURFDRAW_NOCULL) == 0 )
		{
			float dot = DotProduct (shadowDirInModelSpace, MSurf_Plane( surfID ).normal );
			if ( dot > 0 )
				continue;
		}

		// FIXME: We may want to do some more high-level per-surface culling
		// If so, it'll be added to ApplyShadowToSurface. Call it instead.
		AddSurfaceToShadow( handle, surfID );
	}
}

				 
//-----------------------------------------------------------------------------
// Removes all shadows from a brush model
//-----------------------------------------------------------------------------
void CShadowMgr::RemoveAllShadowsFromBrushModel( model_t* pModel )
{
	int surfID = pModel->brush.firstmodelsurface;
	for (int i=0; i<pModel->brush.nummodelsurfaces; ++i, ++surfID)
	{
		RemoveAllShadowsFromSurface( surfID );
	}
}


//-----------------------------------------------------------------------------
// Adds the shadow decals on the surface to a queue of things to render
//-----------------------------------------------------------------------------
void CShadowMgr::AddShadowsOnSurfaceToRenderList( ShadowDecalHandle_t decalHandle )
{
	// Don't compute the surface cache if shadows are off..
	if (!r_shadows.GetInt())
		return;

	// Add all surface decals into the appropriate render lists
//	ShadowDecalHandle_t decalHandle = MSurf_ShadowDecals( surfID );
	while( decalHandle != m_ShadowDecals.InvalidIndex() )
	{
		ShadowDecal_t& shadowDecal = m_ShadowDecals[decalHandle];

		// Hook the decal into the render list
		int sortOrder = m_Shadows[shadowDecal.m_Shadow].m_SortOrder;
		m_ShadowDecals[decalHandle].m_NextRender = m_RenderQueue[sortOrder];
		m_RenderQueue[sortOrder] = decalHandle;

		decalHandle = m_ShadowDecals.Next(decalHandle);

		// We've got one more decal to render
		++m_DecalsToRender;
	}
}

void CShadowMgr::ClearShadowRenderList()
{
	// Clear out the render list
	if (m_RenderQueue.Count() > 0)
	{
		memset( m_RenderQueue.Base(), 0xFF, m_RenderQueue.Count() * sizeof(ShadowDecalHandle_t) );
	}
	m_DecalsToRender = 0;
}

void CShadowMgr::RenderShadows( const VMatrix* pModelToWorld )
{
	VPROF_BUDGET( "CShadowMgr::RenderShadows", VPROF_BUDGETGROUP_SHADOW_RENDERING );
	// Iterate through all sort ids and render
	for (int i = 0; i < m_RenderQueue.Count(); ++i)
	{
		if (m_RenderQueue[i] != m_ShadowDecals.InvalidIndex())
		{
			RenderShadowList(m_RenderQueue[i], pModelToWorld);
		}
	}

	// Clear out the render list, we've rendered it now
	ClearShadowRenderList();
}


//-----------------------------------------------------------------------------
// A 2D sutherland-hogdman clipper
//-----------------------------------------------------------------------------

class CClipTop
{
public:
	static inline bool Inside( ShadowVertex_t const& vert )				{ return vert.m_TexCoord.y < 1;}
	static inline float Clip( const Vector& one, const Vector& two )	{ return (1 - one.y) / (two.y - one.y);}
	static inline bool IsPlane()	{return false;}
	static inline bool IsAbove()	{return false;}
};

class CClipLeft
{
public:
	static inline bool Inside( ShadowVertex_t const& vert )				{ return vert.m_TexCoord.x > 0;}
	static inline float Clip( const Vector& one, const Vector& two )	{ return one.x / (one.x - two.x);}
	static inline bool IsPlane()	{return false;}
	static inline bool IsAbove()	{return false;}
};

class CClipRight
{
public:
	static inline bool Inside( ShadowVertex_t const& vert )				{return vert.m_TexCoord.x < 1;}
	static inline float Clip( const Vector& one, const Vector& two )	{return (1 - one.x) / (two.x - one.x);}
	static inline bool IsPlane()	{return false;}
	static inline bool IsAbove()	{return false;}
};

class CClipBottom
{
public:
	static inline bool Inside( ShadowVertex_t const& vert )				{return vert.m_TexCoord.y > 0;}
	static inline float Clip( const Vector& one, const Vector& two )	{return one.y / (one.y - two.y);}
	static inline bool IsPlane()	{return false;}
	static inline bool IsAbove()	{return false;}
};

class CClipAbove
{
public:
	static inline bool Inside( ShadowVertex_t const& vert )				{return vert.m_TexCoord.z > 0;}
	static inline float Clip( const Vector& one, const Vector& two )	{return one.z / (one.z - two.z);}
	static inline bool IsPlane()	{return false;}
	static inline bool IsAbove()	{return true;}
};

class CClipPlane
{
public:
	static inline bool Inside( ShadowVertex_t const& vert )						
	{
		return DotProduct( vert.m_Position, *m_pNormal ) < m_Dist;
	}

	static inline float Clip( const Vector& one, const Vector& two )	
	{
		Vector dir;
		VectorSubtract( two, one, dir );
		return IntersectRayWithPlane( one, dir, *m_pNormal, m_Dist );
	}

	static inline bool IsAbove()	{return false;}
	static inline bool IsPlane()	{return true;}

	static void SetPlane( const Vector& normal, float dist )
	{
		m_pNormal = &normal;
		m_Dist = dist;
	}


private:
	static const Vector *m_pNormal;
	static float  m_Dist;
};

const Vector *CClipPlane::m_pNormal;
float  CClipPlane::m_Dist;

static inline void ClampTexCoord( ShadowVertex_t *pInVertex, ShadowVertex_t *pOutVertex )
{
	if ( fabs(pInVertex->m_TexCoord[0]) < 1e-3 )
		pOutVertex->m_TexCoord[0] = 0.0f;
	else if ( fabs(pInVertex->m_TexCoord[0] - 1.0f) < 1e-3 )
		pOutVertex->m_TexCoord[0] = 1.0f;

	if ( fabs(pInVertex->m_TexCoord[1]) < 1e-3 )
		pOutVertex->m_TexCoord[1] = 0.0f;
	else if ( fabs(pInVertex->m_TexCoord[1] - 1.0f) < 1e-3 )
		pOutVertex->m_TexCoord[1] = 1.0f;
}

template <class Clipper>
static inline void Intersect( ShadowVertex_t* pStart, ShadowVertex_t* pEnd, ShadowVertex_t* pOut, bool startInside, Clipper& clipper )
{
	// Clip the edge to the clip plane
	float t;
	if (!Clipper::IsPlane())
	{
		if (!Clipper::IsAbove())
		{
			t = Clipper::Clip( pStart->m_TexCoord, pEnd->m_TexCoord );
			VectorLerp( pStart->m_TexCoord, pEnd->m_TexCoord, t, pOut->m_TexCoord );
		}
		else
		{
			t = Clipper::Clip( pStart->m_TexCoord, pEnd->m_TexCoord );
			VectorLerp( pStart->m_TexCoord, pEnd->m_TexCoord, t, pOut->m_TexCoord );

			// This is a special thing we do here to avoid hard-edged shadows
			if (startInside)
				ClampTexCoord( pEnd, pOut );
			else
				ClampTexCoord( pStart, pOut );
		}
	}
	else
	{
		t = Clipper::Clip( pStart->m_Position, pEnd->m_Position );
		VectorLerp( pStart->m_TexCoord, pEnd->m_TexCoord, t, pOut->m_TexCoord );
	}

	VectorLerp( pStart->m_Position, pEnd->m_Position, t, pOut->m_Position );
}

template <class Clipper>
static void ShadowClip( ShadowClipState_t& clip, Clipper& clipper )
{
	if (clip.m_ClipCount == 0)
		return;

	// Ye Olde Sutherland-Hodgman clipping algorithm
	int numOutVerts = 0;
	ShadowVertex_t** pSrcVert = clip.m_ppClipVertices[clip.m_CurrVert];
	ShadowVertex_t** pDestVert = clip.m_ppClipVertices[!clip.m_CurrVert];

	int numVerts = clip.m_ClipCount;
	ShadowVertex_t* pStart = pSrcVert[numVerts-1];
	bool startInside = Clipper::Inside( *pStart );
	for (int i = 0; i < numVerts; ++i)
	{
		ShadowVertex_t* pEnd = pSrcVert[i];
		bool endInside = Clipper::Inside( *pEnd );
		if (endInside)
		{
			if (!startInside)
			{
				// Started outside, ended inside, need to clip the edge
				assert( clip.m_TempCount <= SHADOW_VERTEX_TEMP_COUNT );
				
				// Allocate a new clipped vertex 
				pDestVert[numOutVerts] = &clip.m_pTempVertices[clip.m_TempCount++];

				// Clip the edge to the clip plane
				Intersect( pStart, pEnd, pDestVert[numOutVerts], startInside, clipper ); 
				++numOutVerts;
			}
			pDestVert[numOutVerts++] = pEnd;
		}
		else
		{
			if (startInside)
			{
				// Started inside, ended outside, need to clip the edge
				assert( clip.m_TempCount <= SHADOW_VERTEX_TEMP_COUNT ); 

				// Allocate a new clipped vertex 
				pDestVert[numOutVerts] = &clip.m_pTempVertices[clip.m_TempCount++];

				// Clip the edge to the clip plane
				Intersect( pStart, pEnd, pDestVert[numOutVerts], startInside, clipper ); 
				++numOutVerts;
			}
		}
		pStart = pEnd;
		startInside = endInside;
	}

	// Switch source lists
	clip.m_CurrVert = 1 - clip.m_CurrVert;
	clip.m_ClipCount = numOutVerts;
	assert( clip.m_ClipCount <= SHADOW_VERTEX_TEMP_COUNT ); 
}


//-----------------------------------------------------------------------------
// Project vertices into shadow space
//-----------------------------------------------------------------------------
bool CShadowMgr::ProjectVerticesIntoShadowSpace( const VMatrix& modelToShadow, 
	float maxDist, int count, Vector** ppPosition, ShadowClipState_t& clip )
{
	bool insideVolume = false;

	// Create vertices to clip to...
	for (int i = 0; i < count; ++i )
	{
		Assert( ppPosition[i] );

		VectorCopy( *ppPosition[i], clip.m_pTempVertices[i].m_Position );

		// Project the points into shadow texture space
		// FIXME: Change this multiplication if we do an actual projection
		Vector3DMultiplyPosition( modelToShadow, *ppPosition[i], clip.m_pTempVertices[i].m_TexCoord );

		// Set up clipping coords...
		clip.m_ppClipVertices[0][i] = &clip.m_pTempVertices[i];

		if (clip.m_pTempVertices[i].m_TexCoord[2] < maxDist )
			insideVolume = true;
	}

	clip.m_TempCount = clip.m_ClipCount = count;
	clip.m_CurrVert = 0;

	return insideVolume;
}


//-----------------------------------------------------------------------------
// Projects + clips shadows
//-----------------------------------------------------------------------------
int CShadowMgr::ProjectAndClipVertices( const Shadow_t& shadow, const VMatrix& worldToShadow,
	int count, Vector** ppPosition, ShadowVertex_t*** ppOutVertex )
{
	static ShadowClipState_t clip;
	if (!ProjectVerticesIntoShadowSpace( worldToShadow, shadow.m_MaxDist, count, ppPosition, clip ))
		return 0;

	// Clippers...
	CClipTop top;
	CClipBottom bottom;
	CClipLeft left;
	CClipRight right;
	CClipAbove above;
	CClipPlane plane;

	// Sutherland-hodgman clip
	ShadowClip( clip, top );
	ShadowClip( clip, bottom );
	ShadowClip( clip, left );
	ShadowClip( clip, right );
	ShadowClip( clip, above );

	// Planes to suppress back-casting
	for (int i = 0; i < shadow.m_ClipPlaneCount; ++i)
	{
		plane.SetPlane( shadow.m_ClipPlane[i], shadow.m_ClipDist[i] );
		ShadowClip( clip, plane );
	}

	// FIXME: Add method here to prevent poke-thru

	if (clip.m_ClipCount < 3)
		return 0;

	// Return a pointer to the array of clipped vertices...
	Assert(ppOutVertex);
	*ppOutVertex = clip.m_ppClipVertices[clip.m_CurrVert];
	return clip.m_ClipCount;
}


//-----------------------------------------------------------------------------
// Accessor for use by the displacements
//-----------------------------------------------------------------------------
int CShadowMgr::ProjectAndClipVertices( ShadowHandle_t handle, int count, 
	Vector** ppPosition, ShadowVertex_t*** ppOutVertex )
{
	return ProjectAndClipVertices( m_Shadows[handle], 
		m_Shadows[handle].m_WorldToShadow, count, ppPosition, ppOutVertex );
}


//-----------------------------------------------------------------------------
// Copies vertex info from the clipped vertices
//-----------------------------------------------------------------------------
inline void CShadowMgr::CopyClippedVertices( int count, ShadowVertex_t** ppSrcVert, ShadowVertex_t* pDstVert )
{
	for (int i = 0; i < count; ++i)
	{
		VectorCopy( ppSrcVert[i]->m_Position, pDstVert[i].m_Position );
		VectorCopy( ppSrcVert[i]->m_TexCoord, pDstVert[i].m_TexCoord );

		// Make sure it's been clipped
		Assert( ppSrcVert[i]->m_TexCoord[0] >= -1e-3f );
		Assert( ppSrcVert[i]->m_TexCoord[0] - 1.0f <= 1e-3f );
		Assert( ppSrcVert[i]->m_TexCoord[1] >= -1e-3f );
		Assert( ppSrcVert[i]->m_TexCoord[1] - 1.0f <= 1e-3f );
	}
}


//-----------------------------------------------------------------------------
// Does the actual work of computing shadow vertices
//-----------------------------------------------------------------------------
bool CShadowMgr::ComputeShadowVertices( ShadowDecal_t& decal, 
			const VMatrix* pModelToWorld, ShadowVertexCache_t* pVertexCache )
{
	g_EngineStats.IncrementCountedStat( ENGINE_STATS_SHADOWS_CLIPPED, 1 );

	// Prepare for the clipping
	Vector** ppVec = (Vector**)stackalloc( MSurf_VertCount( decal.m_SurfID ) * sizeof(Vector*) );
	for (int i = 0; i < MSurf_VertCount( decal.m_SurfID ); ++i )
	{
		int vertIndex = host_state.worldmodel->brush.vertindices[MSurf_FirstVertIndex( decal.m_SurfID )+i];
		ppVec[i] = &host_state.worldmodel->brush.vertexes[vertIndex].position;
	}

	// Compute the modelToShadow transform.
	// In the case of the world, just use worldToShadow...
	VMatrix* pModelToShadow = &m_Shadows[decal.m_Shadow].m_WorldToShadow;
	VMatrix temp;
	if (pModelToWorld)
	{
		MatrixMultiply( *pModelToShadow, *pModelToWorld, temp );
		pModelToShadow = &temp;
	}

	// Create vertices to clip to...
	ShadowVertex_t** ppSrcVert;
	int clipCount = ProjectAndClipVertices( m_Shadows[decal.m_Shadow], *pModelToShadow, 
		MSurf_VertCount( decal.m_SurfID ), ppVec, &ppSrcVert );
	if (clipCount == 0)
	{
		pVertexCache->m_Count = 0;
		return false;
	}
	
	// Allocate the vertices we're going to use for the decal
	ShadowVertex_t* pDstVert = AllocateVertices( *pVertexCache, clipCount );
	Assert( pDstVert );

	// Copy the clipped vertices into the cache
	CopyClippedVertices( clipCount, ppSrcVert, pDstVert );

	// Indicate which shadow this is related to
	pVertexCache->m_Shadow = decal.m_Shadow;

	return true;
}



//-----------------------------------------------------------------------------
// Should we cache vertices?
//-----------------------------------------------------------------------------
inline bool CShadowMgr::ShouldCacheVertices( const ShadowDecal_t& decal )
{
	return (m_Shadows[decal.m_Shadow].m_Flags & SHADOW_CACHE_VERTS) != 0;
}


//-----------------------------------------------------------------------------
// Generates a list displacement shadow vertices to render
//-----------------------------------------------------------------------------
inline bool CShadowMgr::GenerateDispShadowRenderInfo( ShadowDecal_t& decal, ShadowRenderInfo_t& info )
{
	int v, i;
	if (!MSurf_DispInfo( decal.m_SurfID )->ComputeShadowFragments( decal.m_DispShadow, v, i ))
		return false;

	info.m_VertexCount += v;
	info.m_IndexCount += i;
	info.m_pDispCache[info.m_DispCount++] = decal.m_DispShadow;

	return true;
}


//-----------------------------------------------------------------------------
// Generates a list shadow vertices to render
//-----------------------------------------------------------------------------
inline bool CShadowMgr::GenerateNormalShadowRenderInfo( ShadowDecal_t& decal, ShadowRenderInfo_t& info )
{
	// Look for a cache hit
	ShadowVertexCache_t* pVertexCache;
	if (decal.m_ShadowVerts != m_VertexCache.InvalidIndex())
	{
		// Ok, we've already computed the data, lets use it
		info.m_pCache[info.m_Count] = decal.m_ShadowVerts;
		pVertexCache = &m_VertexCache[decal.m_ShadowVerts];
	}
	else
	{
		// In this case, we gotta recompute the shadow decal vertices
		// and maybe even store it into the cache....
		bool shouldCacheVerts = ShouldCacheVertices( decal );
		if (shouldCacheVerts)
		{
			decal.m_ShadowVerts = m_VertexCache.AddToTail();
			info.m_pCache[info.m_Count] = decal.m_ShadowVerts;
			pVertexCache = &m_VertexCache[decal.m_ShadowVerts];
		}
		else
		{
			int i = m_TempVertexCache.AddToTail();
			info.m_pCache[info.m_Count] = -i-1;
			pVertexCache = &m_TempVertexCache[i];
			Assert( info.m_pCache[info.m_Count] < 0 );
		}

		// Compute the shadow vertices
		// If no vertices were created, indicate this surface should be removed from the cache
		if (!ComputeShadowVertices( decal, info.m_pModelToWorld, pVertexCache ))
			return false;
	}

	// Update vertex, index, and decal counts
	info.m_VertexCount += pVertexCache->m_Count;
	info.m_IndexCount += 3 * (pVertexCache->m_Count - 2);
	++info.m_Count;
	
	return true;
}


//-----------------------------------------------------------------------------
// Generates a list shadow vertices to render
//-----------------------------------------------------------------------------
void CShadowMgr::GenerateShadowRenderInfo( ShadowDecalHandle_t decalHandle, ShadowRenderInfo_t& info )
{
	info.m_VertexCount = 0;
	info.m_IndexCount = 0;
	info.m_Count = 0;
	info.m_DispCount = 0;

	// Keep the lists only full of valid decals; that way we can preserve
	// the render lists in the case that we discover a shadow isn't needed.
	while ( decalHandle != m_ShadowDecals.InvalidIndex() )
	{
		ShadowDecal_t& decal = m_ShadowDecals[decalHandle];

		bool keepShadow;
		if (decal.m_DispShadow != DISP_SHADOW_HANDLE_INVALID)
		{
			// Handle shadows on displacements...
			keepShadow = GenerateDispShadowRenderInfo( decal, info );
		}
		else
		{
			// Handle shadows on normal surfaces
			keepShadow = GenerateNormalShadowRenderInfo( decal, info );
		}

		// Retire the surface if the shadow didn't actually hit it
		ShadowDecalHandle_t next = m_ShadowDecals[decalHandle].m_NextRender;
		if ( !keepShadow && ShouldCacheVertices( decal ) )
		{
			// If no triangles were generated
			// (the decal was completely clipped off)
			// In this case, remove the decal from the surface cache
			// so next time it'll be faster (for cached decals)
			RemoveSurfaceFromShadow( decal.m_Shadow, decal.m_SurfID );

			// NOTE: Code to keep the shadow lists up to date is located in
			// version 12
		}
		decalHandle = next;
	}
}


//-----------------------------------------------------------------------------
// Converts z value to darkness
//-----------------------------------------------------------------------------
unsigned char CShadowMgr::ComputeDarkness( float z, const ShadowInfo_t& info ) const
{
	// NOTE: 0 means black, non-zero adds towards white...
	z -= info.m_FalloffOffset;

	float zFallofDist = info.m_MaxDist - info.m_FalloffOffset;
	if ( zFallofDist < 0 )
		zFallofDist = 0;

	if (z >= zFallofDist)
		return clamp( info.m_FalloffAmount + info.m_FalloffBias, 0, 255 );
	else if (z < 0)
		return info.m_FalloffBias;

	z /= zFallofDist;
	return (unsigned char)clamp(info.m_FalloffBias + (info.m_FalloffAmount * z), 0, 255);
}


//-----------------------------------------------------------------------------
// Adds normal shadows to the mesh builder
//-----------------------------------------------------------------------------
int CShadowMgr::AddNormalShadowsToMeshBuilder( CMeshBuilder& meshBuilder, ShadowRenderInfo_t& info )
{
	// Step through the cache and add all shadows on normal surfaces
	int baseIndex = 0;
	for (int i = 0; i < info.m_Count; ++i)
	{
		// Two loops here, basically to minimize the # of if statements we need
		ShadowVertexCache_t* pVertexCache;
		if (info.m_pCache[i] < 0)
		{
			pVertexCache = &m_TempVertexCache[-info.m_pCache[i]-1];
		}
		else
		{
			pVertexCache = &m_VertexCache[info.m_pCache[i]];
		}

		ShadowVertex_t* pVerts = GetCachedVerts( *pVertexCache );
		Shadow_t& shadow = m_Shadows[pVertexCache->m_Shadow];

		int j;
		int vCount = pVertexCache->m_Count;
		for ( j = 0; j < vCount - 2; ++j )
		{
			meshBuilder.Position3fv( pVerts[j].m_Position.Base() );

			// Transform + offset the texture coords
			Vector2D texCoord;
			Vector2DMultiply( pVerts[j].m_TexCoord.AsVector2D(), shadow.m_TexSize, texCoord );
			texCoord += shadow.m_TexOrigin;
			meshBuilder.TexCoord2fv( 0, texCoord.Base() );
			
			unsigned char c = ComputeDarkness( pVerts[j].m_TexCoord.z, shadow );
			meshBuilder.Color4ub( c, c, c, c );

			meshBuilder.AdvanceVertex();

			meshBuilder.Index( baseIndex );
			meshBuilder.AdvanceIndex();
			meshBuilder.Index( j + baseIndex + 1 );
			meshBuilder.AdvanceIndex();
			meshBuilder.Index( j + baseIndex + 2 );
			meshBuilder.AdvanceIndex();
		}

		for ( ; j < vCount; ++j )
		{
			meshBuilder.Position3fv( pVerts[j].m_Position.Base() );

			Vector2D texCoord;
			Vector2DMultiply( pVerts[j].m_TexCoord.AsVector2D(), shadow.m_TexSize, texCoord );
			texCoord += shadow.m_TexOrigin;
			meshBuilder.TexCoord2fv( 0, texCoord.Base() );

			unsigned char c = ComputeDarkness( pVerts[j].m_TexCoord.z, shadow );
			meshBuilder.Color4ub( c, c, c, c );

			meshBuilder.AdvanceVertex();
		}

		// Update the base index
		baseIndex += vCount; 
	}

	return baseIndex;
}


//-----------------------------------------------------------------------------
// Adds displacement shadows to the mesh builder
//-----------------------------------------------------------------------------
int CShadowMgr::AddDisplacementShadowsToMeshBuilder( CMeshBuilder& meshBuilder, 
								ShadowRenderInfo_t& info, int baseIndex )
{
	// Step through the cache and add all shadows on displacement surfaces
	for (int i = 0; i < info.m_DispCount; ++i)
	{
		baseIndex = DispInfo_AddShadowsToMeshBuilder( meshBuilder, info.m_pDispCache[i], baseIndex );
	}

	return baseIndex;
}


//-----------------------------------------------------------------------------
// The following methods will display debugging info in the middle of each shadow decal
//-----------------------------------------------------------------------------
static void DrawShadowID( ShadowHandle_t shadowHandle, const Vector &vecCentroid )
{
#ifndef SWDS
	char buf[32];
	sprintf(buf, "%d", shadowHandle );
	CDebugOverlay::AddTextOverlay( vecCentroid, 0, buf );
#endif
}

void CShadowMgr::RenderDebuggingInfo( const ShadowRenderInfo_t &info, ShadowDebugFunc_t func )
{
	// Step through the cache and add all shadows on normal surfaces
	for (int i = 0; i < info.m_Count; ++i)
	{
		ShadowVertexCache_t* pVertexCache;
		if (info.m_pCache[i] < 0)
		{
			pVertexCache = &m_TempVertexCache[-info.m_pCache[i]-1];
		}
		else
		{
			pVertexCache = &m_VertexCache[info.m_pCache[i]];
		}

		ShadowVertex_t* pVerts = GetCachedVerts( *pVertexCache );

		Vector vecNormal;
		float flTotalArea = 0.0f;
		Vector vecCentroid(0,0,0);
		Vector vecApex = pVerts[0].m_Position;
		int vCount = pVertexCache->m_Count;

		for ( int j = 0; j < vCount - 2; ++j )
		{
			Vector v1 = pVerts[j + 1].m_Position;
			Vector v2 = pVerts[j + 2].m_Position;
			CrossProduct( v2 - v1, v1 - vecApex, vecNormal );
			float flArea = vecNormal.Length();
			flTotalArea += flArea;
			vecCentroid += (vecApex + v1 + v2) * flArea / 3.0f;
		}

		if (flTotalArea)
		{
			vecCentroid /= flTotalArea;
		}

		func( pVertexCache->m_Shadow, vecCentroid );
	}
}


//-----------------------------------------------------------------------------
// Renders shadows that all share a material enumeration
//-----------------------------------------------------------------------------
void CShadowMgr::RenderShadowList( ShadowDecalHandle_t decalHandle, const VMatrix* pModelToWorld )
{
	MEASURE_TIMED_STAT( ENGINE_STATS_SHADOW_RENDER_TIME );

	// Set the render state...
	Shadow_t& shadow = m_Shadows[m_ShadowDecals[decalHandle].m_Shadow];

	if (r_shadowwireframe.GetInt() == 0)
		materialSystemInterface->Bind( shadow.m_pMaterial, shadow.m_pBindProxy );
	else
		materialSystemInterface->Bind( g_materialWorldWireframe );

	// Blow away the temporary vertex cache (for normal surfaces)
	ClearTempCache();

	// Set up rendering info structure
	ShadowRenderInfo_t info;
	info.m_pCache = (int*)stackalloc( m_DecalsToRender * sizeof(int) );
	info.m_pDispCache = (DispShadowHandle_t*)stackalloc( m_DecalsToRender * sizeof(DispShadowHandle_t) );
	info.m_pModelToWorld = pModelToWorld;

	// Iterate over all decals in the decal list and generate polygon lists
	// Creating them from scratch if their shadow poly cache is invalid
	GenerateShadowRenderInfo(decalHandle, info);
	Assert( info.m_Count <= m_DecalsToRender );
	Assert( info.m_DispCount <= m_DecalsToRender );

	// Now that the vertex lists are created, render them
	IMesh* pMesh = materialSystemInterface->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, info.m_VertexCount, info.m_IndexCount );

	// Add in shadows from both normal surfaces + displacement surfaces
	int baseIndex = AddNormalShadowsToMeshBuilder( meshBuilder, info );
	AddDisplacementShadowsToMeshBuilder( meshBuilder, info, baseIndex );

	meshBuilder.End();
	pMesh->Draw();

	if (r_shadowids.GetInt() != 0)
	{
		RenderDebuggingInfo( info, DrawShadowID );
	}

	g_EngineStats.IncrementCountedStat( ENGINE_STATS_SHADOW_ACTUAL_TRIANGLES, info.m_IndexCount / 3 );

	stackfree(info.m_pCache);
	stackfree(info.m_pDispCache);
}

