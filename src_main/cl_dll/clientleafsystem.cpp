//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//=============================================================================
#include "cbase.h"
#include "ClientLeafSystem.h"
#include "UtlBidirectionalSet.h"
#include "BSPTreeData.h"
#include "model_types.h"
#include "IVRenderView.h"
#include "tier0/vprof.h"
#include "DetailObjectSystem.h"
#include "engine/IStaticPropMgr.h"


static ConVar cl_drawleaf("cl_drawleaf", "-1");
static ConVar r_PortalTestEnts( "r_PortalTestEnts", "0", 0, "Clip entities against portal frustums." );
		    
//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
class CClientLeafSystem : public IClientLeafSystem, public ISpatialLeafEnumerator
{
public:
	// constructor, destructor
	CClientLeafSystem();
	virtual ~CClientLeafSystem();

	// Methods of IClientSystem
	bool Init() { return true; }
	void Shutdown() {}

	void PreRender();
	void Update( float frametime ) 
	{
	}

	void LevelInitPreEntity();
	void LevelInitPostEntity() {}
	void LevelShutdownPreEntity();
	void LevelShutdownPostEntity();

	virtual void OnSave() {}
	virtual void OnRestore() {}

// Methods of IClientLeafSystem
public:
	
	virtual ClientRenderHandle_t AddRenderable( IClientRenderable* pRenderable, RenderGroup_t group );
	virtual ClientRenderHandle_t CreateRenderableHandle( IClientRenderable* pRenderable, bool bIsStaticProp );
	virtual void RemoveRenderable( ClientRenderHandle_t handle );
	virtual bool IsRenderableVisible( ClientRenderHandle_t handle );
	// FIXME: There's an incestuous relationship between DetailObjectSystem
	// and the ClientLeafSystem. Maybe they should be the same system?
	virtual void GetDetailObjectsInLeaf( int leaf, int& firstDetailObject, int& detailObjectCount );
 	virtual void SetDetailObjectsInLeaf( int leaf, int firstDetailObject, int detailObjectCount );
	virtual void DrawDetailObjectsInLeaf( int leaf, int frameNumber, int& nFirstDetailObject, int& nDetailObjectCount );
	virtual bool ShouldDrawDetailObjectsInLeaf( int leaf, int frameNumber );
	virtual void RenderableMoved( ClientRenderHandle_t handle );
	virtual void DetectChanges( ClientRenderHandle_t handle, RenderGroup_t group );
	virtual void ComputeTranslucentRenderLeaf( int count, int *pLeafList, int *pLeafFogVolumeList, int frameNumber );
	virtual void CollateRenderablesInViewModelRenderGroup( CUtlVector< IClientRenderable * >& list );
	virtual void CollateRenderablesInLeaf( int leaf, int worldListLeafIndex, SetupRenderInfo_t &info );
	virtual void DrawStaticProps( bool enable );
	virtual void DrawSmallEntities( bool enable );
	virtual bool IsRenderableUnderWater( WorldListInfo_t& info, 
									     CRenderList::CEntry *pRenderListEntry );
	virtual bool IsRenderableAboveWater( WorldListInfo_t& info, 
									     CRenderList::CEntry *pRenderListEntry );

	// Adds a renderable to a set of leaves
	virtual void AddRenderableToLeaves( ClientRenderHandle_t handle, int nLeafCount, unsigned short *pLeaves );

	// The following methods are related to shadows...
	virtual ClientLeafShadowHandle_t AddShadow( unsigned short userId );
	virtual void RemoveShadow( ClientLeafShadowHandle_t h );

	virtual void ProjectShadow( ClientLeafShadowHandle_t handle, const Vector& origin, 
					const Vector& dir, const Vector2D& size, float maxDist );

	// Find all shadow casters in a set of leaves
	virtual void EnumerateShadowsInLeaves( int leafCount, int* pLeaves, IClientLeafShadowEnum* pEnum );

	// methods of ISpatialLeafEnumerator
public:

	bool EnumerateLeaf( int leaf, int context );

	// Adds a shadow to a leaf
	void AddShadowToLeaf( int leaf, ClientLeafShadowHandle_t handle );

	// Singleton instance...
	static CClientLeafSystem s_ClientLeafSystem;

private:
	// Creates a new renderable
	ClientRenderHandle_t NewRenderable( IClientRenderable* pRenderable, RenderGroup_t type, int flags = 0 );

	// Adds a renderable to the list of renderables
	void AddRenderableToLeaf( int leaf, ClientRenderHandle_t handle );

	// Returns -1 if the renderable spans more than one area. If it's totally in one area, then this returns the leaf.
	short GetRenderableArea( ClientRenderHandle_t handle );

	// insert, remove renderables from leaves
	void InsertIntoTree( ClientRenderHandle_t handle );
	void RemoveFromTree( ClientRenderHandle_t handle );

	// Insert translucent renderables into list of translucent objects
	void InsertTranslucentRenderable( IClientRenderable* pRenderable,
		int& count, IClientRenderable** pList, float* pDist );

	// Used to change renderables from translucent to opaque
	// Only really used by the static prop fading...
	void ChangeRenderableRenderGroup( ClientRenderHandle_t handle, RenderGroup_t group );

	// Adds a shadow to a leaf/removes shadow from renderable
	void AddShadowToRenderable( ClientRenderHandle_t renderHandle, ClientLeafShadowHandle_t shadowHandle );
	void RemoveShadowFromRenderables( ClientLeafShadowHandle_t handle );

	// Adds a shadow to a leaf/removes shadow from leaf
	void RemoveShadowFromLeaves( ClientLeafShadowHandle_t handle );

	// Methods associated with the various bi-directional sets
	static unsigned short& FirstRenderableInLeaf( int leaf ) 
	{ 
		return s_ClientLeafSystem.m_Leaf[leaf].m_FirstElement;
	}

	static unsigned short& FirstLeafInRenderable( unsigned short renderable ) 
	{ 
		return s_ClientLeafSystem.m_Renderables[renderable].m_LeafList;
	}

	static unsigned short& FirstShadowInLeaf( int leaf ) 
	{ 
		return s_ClientLeafSystem.m_Leaf[leaf].m_FirstShadow;
	}

	static unsigned short& FirstLeafInShadow( ClientLeafShadowHandle_t shadow ) 
	{ 
		return s_ClientLeafSystem.m_Shadows[shadow].m_FirstLeaf;
	}

	static unsigned short& FirstShadowOnRenderable( unsigned short renderable ) 
	{ 
		return s_ClientLeafSystem.m_Renderables[renderable].m_FirstShadow;
	}

	static unsigned short& FirstRenderableInShadow( ClientLeafShadowHandle_t shadow ) 
	{ 
		return s_ClientLeafSystem.m_Shadows[shadow].m_FirstRenderable;
	}


private:
	enum
	{
		RENDER_FLAGS_TWOPASS = 0x1,
		RENDER_FLAGS_STATIC_PROP = 0x2,
		RENDER_FLAGS_BRUSH_MODEL = 0x4,

		RENDER_FLAGS_UNDERWATER	= 0x8,
		RENDER_FLAGS_ABOVEWATER	= 0x10,
	};

	// All the information associated with a particular handle
	struct RenderableInfo_t
	{
		IClientRenderable*	m_pRenderable;
		int					m_RenderFrame;	// which frame did I render it in?
		int					m_RenderFrame2;
		int					m_EnumCount;	// Have I been added to a particular shadow yet?
		unsigned short		m_LeafList;		// What leafs is it in?
		unsigned short		m_RenderLeaf;	// What leaf do I render in?
		unsigned char		m_Flags;		// rendering flags
		unsigned char		m_RenderGroup;	// RenderGroup_t type
		unsigned short		m_FirstShadow;	// The first shadow caster that cast on it
		unsigned short		m_CachedRenderInfo;	// In m_CachedRenderInfos. Only set for
												// changeable renderables (IClientRenderable::CacheRenderInfo() == true).
		short m_Area;	// -1 if the renderable spans multiple areas.
	};

	// The leaf contains an index into a list of renderables
	struct ClientLeaf_t
	{
		unsigned short	m_FirstElement;
		unsigned short	m_FirstShadow;

		// An optimization for detail objects since there are tens
		// of thousands of them, and since we're assuming they lie in
		// exactly one leaf (a bogus assumption, but too bad)
		unsigned short	m_FirstDetailProp;
		unsigned short	m_DetailPropCount;
		int				m_DetailPropRenderFrame;
	};

	// Shadow information
	struct ShadowInfo_t
	{
		unsigned short	m_FirstLeaf;
		unsigned short	m_FirstRenderable;
		int				m_EnumCount;
		unsigned short	m_Shadow;
	};

	class CCachedRenderInfo
	{
	public:
		Vector					m_vOrigin;
		Vector					m_Mins;
		Vector					m_Maxs;
	};

	// Stores data associated with each leaf.
	CUtlVector< ClientLeaf_t >	m_Leaf;

	// Stores all unique non-detail renderables
	CUtlLinkedList< RenderableInfo_t, ClientRenderHandle_t >	m_Renderables;

	// Information associated with shadows registered with the client leaf system
	CUtlLinkedList< ShadowInfo_t, ClientLeafShadowHandle_t >	m_Shadows;

	// Maintains the list of all renderables in a particular leaf
	CBidirectionalSet< int, ClientRenderHandle_t, unsigned short >	m_RenderablesInLeaf;

	// Maintains a list of all shadows in a particular leaf 
	CBidirectionalSet< int, ClientLeafShadowHandle_t, unsigned short >	m_ShadowsInLeaf;

	// Maintains a list of all shadows cast on a particular renderable
	CBidirectionalSet< ClientRenderHandle_t, ClientLeafShadowHandle_t, unsigned short >	m_ShadowsOnRenderable;

	// Cached rendering info
	CUtlLinkedList< CCachedRenderInfo, unsigned short >		m_CachedRenderInfos;

	// Should I draw static props?
	bool m_DrawStaticProps;
	bool m_DrawSmallObjects;

	// A little enumerator to help us when adding shadows to renderables
	int	m_ShadowEnum;
};


//-----------------------------------------------------------------------------
// Expose IClientLeafSystem to the client dll.
//-----------------------------------------------------------------------------
CClientLeafSystem CClientLeafSystem::s_ClientLeafSystem;
IClientLeafSystem *g_pClientLeafSystem = &CClientLeafSystem::s_ClientLeafSystem;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientLeafSystem, IClientLeafSystem, CLIENTLEAFSYSTEM_INTERFACE_VERSION, CClientLeafSystem::s_ClientLeafSystem );


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CClientLeafSystem::CClientLeafSystem() : m_DrawStaticProps(true), m_DrawSmallObjects(true)
{
	// Set up the bi-directional lists...
	m_RenderablesInLeaf.Init( FirstRenderableInLeaf, FirstLeafInRenderable );
	m_ShadowsInLeaf.Init( FirstShadowInLeaf, FirstLeafInShadow ); 
	m_ShadowsOnRenderable.Init( FirstShadowOnRenderable, FirstRenderableInShadow );
}

CClientLeafSystem::~CClientLeafSystem()
{
}

//-----------------------------------------------------------------------------
// Activate, deactivate static props
//-----------------------------------------------------------------------------
void CClientLeafSystem::DrawStaticProps( bool enable )
{
	m_DrawStaticProps = enable;
}

void CClientLeafSystem::DrawSmallEntities( bool enable )
{
	m_DrawSmallObjects = enable;
}


//-----------------------------------------------------------------------------
// Level init, shutdown
//-----------------------------------------------------------------------------
void CClientLeafSystem::LevelInitPreEntity()
{
	m_Renderables.EnsureCapacity( 1024 );
	m_RenderablesInLeaf.EnsureCapacity( 1024 );
	m_ShadowsInLeaf.EnsureCapacity( 256 );
	m_ShadowsOnRenderable.EnsureCapacity( 256 );

	// Add all the leaves we'll need
	int leafCount = engine->LevelLeafCount();
	m_Leaf.EnsureCapacity( leafCount );

	ClientLeaf_t newLeaf;
	newLeaf.m_FirstElement = m_RenderablesInLeaf.InvalidIndex();
	newLeaf.m_FirstShadow = m_ShadowsInLeaf.InvalidIndex();
	newLeaf.m_FirstDetailProp = 0;
	newLeaf.m_DetailPropCount = 0;
	newLeaf.m_DetailPropRenderFrame = -1;
	while ( --leafCount >= 0 )
	{
		m_Leaf.AddToTail( newLeaf );
	}
}

void CClientLeafSystem::LevelShutdownPreEntity()
{
}

void CClientLeafSystem::LevelShutdownPostEntity()
{
	m_Renderables.Purge();
	m_RenderablesInLeaf.Purge();
	m_Shadows.Purge();
	m_Leaf.Purge();
	m_ShadowsInLeaf.Purge();
	m_ShadowsOnRenderable.Purge();
}


//-----------------------------------------------------------------------------
// This is what happens before rendering a particular view
//-----------------------------------------------------------------------------
void CClientLeafSystem::PreRender()
{
	// Iterate through all renderables and tell them to compute their FX blend
	unsigned short i = m_Renderables.Head();
	while ( i != m_Renderables.InvalidIndex() )
	{
		IClientRenderable *pRenderable = m_Renderables[i].m_pRenderable;
		pRenderable->ComputeFxBlend();

		i = m_Renderables.Next(i);
	}
}


//-----------------------------------------------------------------------------
// Creates a new renderable
//-----------------------------------------------------------------------------
ClientRenderHandle_t CClientLeafSystem::NewRenderable( IClientRenderable* pRenderable, RenderGroup_t type, int flags )
{
	Assert( pRenderable );

	ClientRenderHandle_t handle = m_Renderables.AddToTail();
	RenderableInfo_t &info = m_Renderables[handle];

	// We need to know if it's a brush model for shadows
	int modelType = modelinfo->GetModelType( pRenderable->GetModel() );
	if (modelType == mod_brush)
		flags |= RENDER_FLAGS_BRUSH_MODEL;

	info.m_pRenderable = pRenderable;
	info.m_RenderFrame = -1;
	info.m_RenderFrame2 = -1;
	info.m_FirstShadow = m_ShadowsOnRenderable.InvalidIndex();
	info.m_LeafList = m_RenderablesInLeaf.InvalidIndex();
	info.m_Flags = flags;
	info.m_RenderGroup = (unsigned char)type;
	info.m_EnumCount = 0;
	info.m_RenderLeaf = 0xFFFF;
	
	if( pRenderable->ShouldCacheRenderInfo() )
	{
		// Setup its cached render info. Give it some garbage values so it will detect
		// a change first time DetectChanges is called.
		info.m_CachedRenderInfo = m_CachedRenderInfos.AddToTail();
		
		CCachedRenderInfo &cachedInfo = m_CachedRenderInfos.Element( info.m_CachedRenderInfo );
		cachedInfo.m_vOrigin.Init( 1e24, 1e24, 1e24 );
	}
	else
	{
		info.m_CachedRenderInfo = m_CachedRenderInfos.InvalidIndex();
	}

	return handle;
}

ClientRenderHandle_t CClientLeafSystem::CreateRenderableHandle( IClientRenderable* pRenderable, bool bIsStaticProp )
{
	// FIXME: The argument is unnecessary if we could get this next line to work
	// the reason why we can't is because currently there are IClientRenderables
	// which don't correctly implement GetRefEHandle.

	//bool bIsStaticProp = staticpropmgr->IsStaticProp( pRenderable->GetIClientUnknown() );

	// Add the prop to all the leaves it lies in
	RenderGroup_t group = pRenderable->IsTransparent() ? RENDER_GROUP_TRANSLUCENT_ENTITY : RENDER_GROUP_OPAQUE_ENTITY;

	bool bTwoPass = false;
	if (group == RENDER_GROUP_TRANSLUCENT_ENTITY)
		bTwoPass = modelinfo->IsTranslucentTwoPass( pRenderable->GetModel() );

	int flags = bIsStaticProp ? RENDER_FLAGS_STATIC_PROP : 0;
	if (bTwoPass)
		flags |= RENDER_FLAGS_TWOPASS;

	return NewRenderable( pRenderable, group, flags );
}


//-----------------------------------------------------------------------------
// Used to change renderables from translucent to opaque
//-----------------------------------------------------------------------------
void CClientLeafSystem::ChangeRenderableRenderGroup( ClientRenderHandle_t handle, RenderGroup_t group )
{
	RenderableInfo_t &info = m_Renderables[handle];
	info.m_RenderGroup = (unsigned char)group;
}


//-----------------------------------------------------------------------------
// Add/remove renderable
//-----------------------------------------------------------------------------

ClientRenderHandle_t CClientLeafSystem::AddRenderable( IClientRenderable* pRenderable, RenderGroup_t group )
{
	bool twoPass = false;
	if ( group == RENDER_GROUP_TWOPASS )
	{
		twoPass = true;
		group = RENDER_GROUP_TRANSLUCENT_ENTITY;
	}

	ClientRenderHandle_t handle = NewRenderable( pRenderable, group, twoPass ? RENDER_FLAGS_TWOPASS : 0 );
	InsertIntoTree( handle );
	return handle;
}

void CClientLeafSystem::RemoveRenderable( ClientRenderHandle_t handle )
{
	// This can happen upon level shutdown
	if (!m_Renderables.IsValidIndex(handle))
		return;

	// Free its cached info?
	RenderableInfo_t &info = m_Renderables[handle];
	if( info.m_CachedRenderInfo != m_CachedRenderInfos.InvalidIndex() )
	{
		m_CachedRenderInfos.Remove( info.m_CachedRenderInfo );
	}

	RemoveFromTree( handle );
	m_Renderables.Remove( handle );
}

bool CClientLeafSystem::IsRenderableVisible( ClientRenderHandle_t handle )
{
	return m_Renderables[handle].m_LeafList != m_RenderablesInLeaf.InvalidIndex();
}


short CClientLeafSystem::GetRenderableArea( ClientRenderHandle_t handle )
{
	RenderableInfo_t *pRenderable = &m_Renderables[handle];
	if ( pRenderable->m_LeafList == m_RenderablesInLeaf.InvalidIndex() )
		return false;

	int leaves[128];
	int nLeaves = 0;

	for ( int i=m_RenderablesInLeaf.FirstBucket( handle ); i != m_RenderablesInLeaf.InvalidIndex(); i = m_RenderablesInLeaf.NextBucket( i ) )
	{
		leaves[nLeaves++] = m_RenderablesInLeaf.Bucket( i );
		if ( nLeaves >= 128 )
			break;
	}

	// Now ask the 
	return engine->GetLeavesArea( leaves, nLeaves );
}


//-----------------------------------------------------------------------------
// Indicates which leaves detail objects are in
//-----------------------------------------------------------------------------
void CClientLeafSystem::SetDetailObjectsInLeaf( int leaf, int firstDetailObject,
											    int detailObjectCount )
{
	m_Leaf[leaf].m_FirstDetailProp = firstDetailObject;
	m_Leaf[leaf].m_DetailPropCount = detailObjectCount;
}

//-----------------------------------------------------------------------------
// Returns the detail objects in a leaf
//-----------------------------------------------------------------------------
void CClientLeafSystem::GetDetailObjectsInLeaf( int leaf, int& firstDetailObject,
											    int& detailObjectCount )
{
	firstDetailObject = m_Leaf[leaf].m_FirstDetailProp;
	detailObjectCount = m_Leaf[leaf].m_DetailPropCount;
}


//-----------------------------------------------------------------------------
// Create/destroy shadows...
//-----------------------------------------------------------------------------
ClientLeafShadowHandle_t CClientLeafSystem::AddShadow( unsigned short userId )
{
	ClientLeafShadowHandle_t idx = m_Shadows.AddToTail();
	m_Shadows[idx].m_Shadow = userId;
	m_Shadows[idx].m_FirstLeaf = m_ShadowsInLeaf.InvalidIndex();
	m_Shadows[idx].m_FirstRenderable = m_ShadowsOnRenderable.InvalidIndex();
	m_Shadows[idx].m_EnumCount = 0;
	return idx;
}

void CClientLeafSystem::RemoveShadow( ClientLeafShadowHandle_t handle )
{
	// Remove the shadow from all leaves + renderables...
	RemoveShadowFromLeaves( handle );
	RemoveShadowFromRenderables( handle );

	// Blow away the handle
	m_Shadows.Remove( handle );
}

//-----------------------------------------------------------------------------
// Adds a shadow to a leaf/removes shadow from renderable
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddShadowToRenderable( ClientRenderHandle_t renderHandle, 
										ClientLeafShadowHandle_t shadowHandle )
{
	m_ShadowsOnRenderable.AddElementToBucket( renderHandle, shadowHandle );

	// Also, do some stuff specific to the particular types of renderables

	// If the renderable is a brush model, then add this shadow to it
	if (m_Renderables[renderHandle].m_Flags & RENDER_FLAGS_BRUSH_MODEL)
	{
		IClientRenderable* pRenderable = m_Renderables[renderHandle].m_pRenderable;
		g_pClientShadowMgr->AddShadowToReceiver( m_Shadows[shadowHandle].m_Shadow,
			pRenderable, SHADOW_RECEIVER_BRUSH_MODEL );
	}
	else if (m_Renderables[renderHandle].m_Flags & RENDER_FLAGS_STATIC_PROP)
	{
		IClientRenderable* pRenderable = m_Renderables[renderHandle].m_pRenderable;
		g_pClientShadowMgr->AddShadowToReceiver( m_Shadows[shadowHandle].m_Shadow,
			pRenderable, SHADOW_RECEIVER_STATIC_PROP );
	}

}

void CClientLeafSystem::RemoveShadowFromRenderables( ClientLeafShadowHandle_t handle )
{
	m_ShadowsOnRenderable.RemoveElement( handle );
}


//-----------------------------------------------------------------------------
// Adds a shadow to a leaf/removes shadow from leaf
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddShadowToLeaf( int leaf, ClientLeafShadowHandle_t shadow )
{
	m_ShadowsInLeaf.AddElementToBucket( leaf, shadow ); 

	// Add the shadow exactly once to all renderables in the leaf
	unsigned short i = m_RenderablesInLeaf.FirstElement( leaf );
	while ( i != m_RenderablesInLeaf.InvalidIndex() )
	{
		ClientRenderHandle_t renderable = m_RenderablesInLeaf.Element(i);
		RenderableInfo_t& info = m_Renderables[renderable];

		// Add each shadow exactly once to each renderable
		if (info.m_EnumCount != m_ShadowEnum)
		{
			if ( info.m_pRenderable->ShouldReceiveShadows() )
			{
				AddShadowToRenderable( renderable, shadow );
				info.m_EnumCount = m_ShadowEnum;
			}
		}

		i = m_RenderablesInLeaf.NextElement(i);
	}
}

void CClientLeafSystem::RemoveShadowFromLeaves( ClientLeafShadowHandle_t handle )
{
	m_ShadowsInLeaf.RemoveElement( handle );
}


//-----------------------------------------------------------------------------
// Adds a shadow to all leaves along a ray
//-----------------------------------------------------------------------------
class CShadowLeafEnum : public ISpatialLeafEnumerator
{
public:
	bool EnumerateLeaf( int leaf, int context )
	{
		CClientLeafSystem::s_ClientLeafSystem.AddShadowToLeaf( leaf, (ClientLeafShadowHandle_t)context );
		return true;
	}
};

void CClientLeafSystem::ProjectShadow( ClientLeafShadowHandle_t handle, const Vector& origin, 
					const Vector& dir, const Vector2D& size, float maxDist )
{
	// Remove the shadow from any leaves it current exists in
	RemoveShadowFromLeaves( handle );
	RemoveShadowFromRenderables( handle );

	// This will help us to avoid adding the shadow multiple times to a renderable
	++m_ShadowEnum;

	// Create a ray starting at the origin, with a boxsize == to the
	// maximum size, and cast it along the direction of the shadow
	// Then mark each leaf that the ray hits with the shadow
	Ray_t ray;
	VectorCopy( origin, ray.m_Start );
	VectorMultiply( dir, maxDist, ray.m_Delta );
	ray.m_StartOffset.Init( 0, 0, 0 );

	float maxsize = max( size.x, size.y ) * 0.5f;
	ray.m_Extents.Init( maxsize, maxsize, maxsize );
	ray.m_IsRay = false;
	ray.m_IsSwept = true;

	CShadowLeafEnum leafEnum;
	ISpatialQuery* pQuery = engine->GetBSPTreeQuery();
	pQuery->EnumerateLeavesAlongRay( ray, &leafEnum, handle );
}


//-----------------------------------------------------------------------------
// Find all shadow casters in a set of leaves
//-----------------------------------------------------------------------------
void CClientLeafSystem::EnumerateShadowsInLeaves( int leafCount, int* pLeaves, IClientLeafShadowEnum* pEnum )
{
	if (leafCount == 0)
		return;

	// This will help us to avoid enumerating the shadow multiple times
	++m_ShadowEnum;

	for (int i = 0; i < leafCount; ++i)
	{
		int leaf = pLeaves[i];

		// Add all shadows in the leaf to the renderable...
		unsigned short j = m_ShadowsInLeaf.FirstElement( leaf );
		while ( j != m_ShadowsInLeaf.InvalidIndex() )
		{
			ClientLeafShadowHandle_t shadow = m_ShadowsInLeaf.Element(j);
			ShadowInfo_t& info = m_Shadows[shadow];

			// Add each shadow exactly once to each renderable
			if (info.m_EnumCount != m_ShadowEnum)
			{
				if (!pEnum->EnumShadow(info.m_Shadow))
					return;

				info.m_EnumCount = m_ShadowEnum;
			}

			j = m_ShadowsInLeaf.NextElement(j);
		}
	}
}


//-----------------------------------------------------------------------------
// Adds a renderable to a leaf
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddRenderableToLeaf( int leaf, ClientRenderHandle_t renderable )
{
	m_RenderablesInLeaf.AddElementToBucket( leaf, renderable );

	if ( m_Renderables[renderable].m_pRenderable->ShouldReceiveShadows() )
	{
		// Add all shadows in the leaf to the renderable...
		unsigned short i = m_ShadowsInLeaf.FirstElement( leaf );
		while (i != m_ShadowsInLeaf.InvalidIndex() )
		{
			ClientLeafShadowHandle_t shadow = m_ShadowsInLeaf.Element(i);
			ShadowInfo_t& info = m_Shadows[shadow];

			// Add each shadow exactly once to each renderable
			if (info.m_EnumCount != m_ShadowEnum)
			{
				AddShadowToRenderable( renderable, shadow );
				info.m_EnumCount = m_ShadowEnum;
			}

			i = m_ShadowsInLeaf.NextElement(i);
		}
	}
}


//-----------------------------------------------------------------------------
// Adds a renderable to a set of leaves
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddRenderableToLeaves( ClientRenderHandle_t handle, int nLeafCount, unsigned short *pLeaves )
{ 
	for (int j = 0; j < nLeafCount; ++j)
	{
		AddRenderableToLeaf( pLeaves[j], handle ); 
	}
	m_Renderables[handle].m_Area = GetRenderableArea( handle );
}


//-----------------------------------------------------------------------------
// Inserts an element into the tree
//-----------------------------------------------------------------------------
bool CClientLeafSystem::EnumerateLeaf( int leaf, int context )
{
	ClientRenderHandle_t handle = (ClientRenderHandle_t)context;
	AddRenderableToLeaf( leaf, handle );
	return true;
}

void CClientLeafSystem::InsertIntoTree( ClientRenderHandle_t handle )
{
	// When we insert into the tree, increase the shadow enumerator
	// to make sure each shadow is added exactly once to each renderable
	++m_ShadowEnum;

	Vector mins, maxs;

	// NOTE: The render bounds here are relative to the renderable's coordinate system
	IClientRenderable* pRenderable = m_Renderables[handle].m_pRenderable;
	pRenderable->GetRenderBounds( mins, maxs );

	// FIXME: Should I just use a sphere here?
	// Another option is to pass the OBB down the tree; makes for a better fit
	// Generate a world-aligned AABB
	Vector absMins, absMaxs;
	const QAngle& angles = pRenderable->GetRenderAngles();
	if (angles == vec3_angle)
	{
		VectorAdd( mins, pRenderable->GetRenderOrigin(), absMins );
		VectorAdd( maxs, pRenderable->GetRenderOrigin(), absMaxs );
	}
	else
	{
		matrix3x4_t	boxToWorld;
		AngleMatrix( angles, pRenderable->GetRenderOrigin(), boxToWorld );
		TransformAABB( boxToWorld, mins, maxs, absMins, absMaxs );
	}
	Assert( absMins.IsValid() && absMaxs.IsValid() );

	ISpatialQuery* pQuery = engine->GetBSPTreeQuery();
	pQuery->EnumerateLeavesInBox( absMins, absMaxs, this, handle );

	// Cache off the area it's sitting in.
	m_Renderables[handle].m_Area = GetRenderableArea( handle );
}

//-----------------------------------------------------------------------------
// Removes an element from the tree
//-----------------------------------------------------------------------------
void CClientLeafSystem::RemoveFromTree( ClientRenderHandle_t handle )
{
	m_RenderablesInLeaf.RemoveElement( handle );

	// Remove all shadows cast onto the object
	m_ShadowsOnRenderable.RemoveBucket( handle );

	// If the renderable is a brush model, then remove all shadows from it
	if (m_Renderables[handle].m_Flags & RENDER_FLAGS_BRUSH_MODEL)
	{
		g_pClientShadowMgr->RemoveAllShadowsFromReceiver( 
			m_Renderables[handle].m_pRenderable, SHADOW_RECEIVER_BRUSH_MODEL );
	}
}


//-----------------------------------------------------------------------------
// Call this when the renderable moves
//-----------------------------------------------------------------------------
void CClientLeafSystem::RenderableMoved( ClientRenderHandle_t handle )
{
	RemoveFromTree( handle );
	InsertIntoTree( handle );

	// Remember the state when we added it.
	RenderableInfo_t *pInfo = &m_Renderables[handle];
	if( pInfo->m_CachedRenderInfo != m_CachedRenderInfos.InvalidIndex() )
	{
		CCachedRenderInfo &cachedInfo = m_CachedRenderInfos.Element( pInfo->m_CachedRenderInfo );
		cachedInfo.m_vOrigin = pInfo->m_pRenderable->GetRenderOrigin();
		pInfo->m_pRenderable->GetRenderBounds( cachedInfo.m_Mins, cachedInfo.m_Maxs );
	}
}

bool CClientLeafSystem::IsRenderableUnderWater( WorldListInfo_t& info, 
											   CRenderList::CEntry *pRenderListEntry )
{
	if( pRenderListEntry->m_RenderHandle != DETAIL_PROP_RENDER_HANDLE )
	{
		RenderableInfo_t *pInfo = &m_Renderables[pRenderListEntry->m_RenderHandle];
		return !!( pInfo->m_Flags & RENDER_FLAGS_UNDERWATER );
	}
	else
	{
		int iLeaf = pRenderListEntry->m_iWorldListInfoLeaf;
		return info.m_pLeafFogVolume[iLeaf] != -1;
	}
}

bool CClientLeafSystem::IsRenderableAboveWater( WorldListInfo_t& info, 
											   CRenderList::CEntry *pRenderListEntry )
{
	if( pRenderListEntry->m_RenderHandle != DETAIL_PROP_RENDER_HANDLE )
	{
		RenderableInfo_t *pInfo = &m_Renderables[pRenderListEntry->m_RenderHandle];
		return !!( pInfo->m_Flags & RENDER_FLAGS_ABOVEWATER );
	}
	else
	{
		int iLeaf = pRenderListEntry->m_iWorldListInfoLeaf;
		return info.m_pLeafFogVolume[iLeaf] == -1;
	}
}

void CClientLeafSystem::DetectChanges( ClientRenderHandle_t handle, RenderGroup_t group )
{
	RenderableInfo_t *pInfo = &m_Renderables[handle];

	bool twoPass = false;
	if ( group == RENDER_GROUP_TWOPASS )
	{
		twoPass = true;
		group = RENDER_GROUP_TRANSLUCENT_ENTITY;
	}

	if ( twoPass )
	{
		pInfo->m_Flags |= RENDER_FLAGS_TWOPASS;
	}
	else
	{
		pInfo->m_Flags &= ~RENDER_FLAGS_TWOPASS;
	}

	// If it's not caching its changes, just relink it into the tree every time.
	if( pInfo->m_CachedRenderInfo == m_CachedRenderInfos.InvalidIndex() )
	{
		pInfo->m_RenderGroup = group;
		RenderableMoved( handle );
		return;
	}

	// FIXME: Check angles here

	// Compare the cached version to the current state.
	CCachedRenderInfo &cachedInfo = m_CachedRenderInfos.Element( pInfo->m_CachedRenderInfo );
	
	const Vector &vOrigin = pInfo->m_pRenderable->GetRenderOrigin();
	Vector vMins, vMaxs;
	pInfo->m_pRenderable->GetRenderBounds( vMins, vMaxs );
	if( vOrigin != cachedInfo.m_vOrigin ||
		vMins != cachedInfo.m_Mins ||
		vMaxs != cachedInfo.m_Maxs )
	{
		// Ok, it's changed, relink it in the tree.
		RenderableMoved( handle );
	}

	pInfo->m_RenderGroup = group;
}


//-----------------------------------------------------------------------------
// Detail system marks 
//-----------------------------------------------------------------------------
void CClientLeafSystem::DrawDetailObjectsInLeaf( int leaf, int nFrameNumber, int& nFirstDetailObject, int& nDetailObjectCount )
{
	ClientLeaf_t &leafInfo = m_Leaf[leaf];
	leafInfo.m_DetailPropRenderFrame = nFrameNumber;
	nFirstDetailObject = leafInfo.m_FirstDetailProp;
	nDetailObjectCount = leafInfo.m_DetailPropCount;
}


//-----------------------------------------------------------------------------
// Are we close enough to this leaf to draw detail props?
//-----------------------------------------------------------------------------
inline bool CClientLeafSystem::ShouldDrawDetailObjectsInLeaf( int leaf, int frameNumber )
{
	return m_Leaf[leaf].m_DetailPropRenderFrame == frameNumber;
}

void CClientLeafSystem::ComputeTranslucentRenderLeaf( int count, int *pLeafList, int *pLeafFogVolumeList, int frameNumber )
{
	VPROF( "CDetailObjectSystem::BuildDetailObjectRenderLists" );

	// For better sorting, we're gonna choose the leaf that is closest to
	// the camera. The leaf list passed in here is sorted front to back
	for (int i = 0; i < count; ++i )
	{
		int leaf = pLeafList[i];

		// iterate over all elements in this leaf
		unsigned short idx = m_RenderablesInLeaf.FirstElement(leaf);
		while (idx != m_RenderablesInLeaf.InvalidIndex())
		{
			RenderableInfo_t& info = m_Renderables[m_RenderablesInLeaf.Element(idx)];
			if( info.m_RenderFrame != frameNumber )
			{
				if( info.m_RenderGroup == RENDER_GROUP_TRANSLUCENT_ENTITY )
				{
					info.m_RenderLeaf = leaf;
				}
				// clear out the water flags when we first see this object in a frame.
				info.m_RenderFrame = frameNumber;
				info.m_Flags &= ~( RENDER_FLAGS_UNDERWATER | RENDER_FLAGS_ABOVEWATER );
			}
			if( pLeafFogVolumeList[i] == -1 )
			{
				info.m_Flags |= RENDER_FLAGS_ABOVEWATER;
			}
			else
			{
				info.m_Flags |= RENDER_FLAGS_UNDERWATER;
			}
			
			idx = m_RenderablesInLeaf.NextElement(idx); 
		}
	}
}

inline void AddRenderableToRenderList( 
	CRenderList &renderList, 
	IClientRenderable *pRenderable, 
	int iLeaf,
	RenderGroup_t group,
	ClientRenderHandle_t renderHandle,
	bool twoPass = false )
{
#ifdef _DEBUG
	if (cl_drawleaf.GetInt() >= 0)
	{
		if (iLeaf != cl_drawleaf.GetInt())
			return;
	}
#endif

	Assert( group >= 0 && group < RENDER_GROUP_COUNT );
	
	int &curCount = renderList.m_RenderGroupCounts[group];
	if( curCount < CRenderList::MAX_GROUP_ENTITIES )
	{
		Assert( (iLeaf >= 0) && (iLeaf <= 65535) );

		CRenderList::CEntry *pEntry = &renderList.m_RenderGroups[group][curCount];
		pEntry->m_pRenderable = pRenderable;
		pEntry->m_iWorldListInfoLeaf = iLeaf;
		pEntry->m_TwoPass = twoPass;
		pEntry->m_RenderHandle = renderHandle;
		curCount++;

	}
	else
	{
		engine->Con_NPrintf( 10, "Warning: overflowed CRenderList group %d", group );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : renderList - 
//			renderGroup - 
//-----------------------------------------------------------------------------
void CClientLeafSystem::CollateRenderablesInViewModelRenderGroup( CUtlVector< IClientRenderable * >& list )
{
	RenderGroup_t renderGroup = RENDER_GROUP_VIEW_MODEL;
	ClientRenderHandle_t handle = m_Renderables.Head();
	while ( handle != m_Renderables.InvalidIndex() )
	{
		RenderableInfo_t& renderable = m_Renderables[handle];
		if ( renderable.m_RenderGroup == renderGroup )
		{
			list.AddToTail( renderable.m_pRenderable );
		}
		handle = m_Renderables.Next( handle );
	}
}

void CClientLeafSystem::CollateRenderablesInLeaf( int leaf, int worldListLeafIndex,	SetupRenderInfo_t &info )
{
	// Collate everything.
	unsigned short idx = m_RenderablesInLeaf.FirstElement(leaf);
	for ( ;idx != m_RenderablesInLeaf.InvalidIndex(); idx = m_RenderablesInLeaf.NextElement(idx) )
	{
		ClientRenderHandle_t handle = m_RenderablesInLeaf.Element(idx);
		RenderableInfo_t& renderable = m_Renderables[handle];

		// Early out on static props if we don't want to render them
		if ((!m_DrawStaticProps) && (renderable.m_Flags & RENDER_FLAGS_STATIC_PROP))
			continue;

		// Early out if we're told to not draw small objects (top view only,
		// that's why we don't check the z component).
		if (!m_DrawSmallObjects)
		{
			CCachedRenderInfo& cachedInfo =  m_CachedRenderInfos[renderable.m_CachedRenderInfo];
			float sizeX = cachedInfo.m_Maxs.x - cachedInfo.m_Mins.x;
			float sizeY = cachedInfo.m_Maxs.y - cachedInfo.m_Mins.y;
			if ((sizeX < 50.f) && (sizeY < 50.f))
				continue;
		}


		// Don't hit the same ent in multiple leaves twice.
		if ( renderable.m_RenderGroup != RENDER_GROUP_TRANSLUCENT_ENTITY )
		{
			if( renderable.m_RenderFrame2 == info.m_nRenderFrame )
				continue;

			renderable.m_RenderFrame2 = info.m_nRenderFrame;
		}


		Vector mins, maxs;
		const Vector &vOrigin = renderable.m_pRenderable->GetRenderOrigin();
		renderable.m_pRenderable->GetRenderBounds( mins, maxs );
		mins += vOrigin;
		maxs += vOrigin;

		// If the renderable is inside an area, cull it using the frustum for that area.
		if ( r_PortalTestEnts.GetInt() )
		{
			if ( renderable.m_Area != -1 )
			{
				if ( !engine->DoesBoxTouchAreaFrustum( mins, maxs, renderable.m_Area ) )
					continue;
			}
		}

#ifdef TF2_CLIENT_DLL
		if (info.m_flRenderDistSq != 0.0f)
		{
			if ((maxs.z - mins.z) < 100)
			{
				Vector vCenter;
				VectorLerp( mins, maxs, 0.5f, vCenter );

				float flDistSq = info.m_vecRenderOrigin.DistToSqr( vCenter );
				if (info.m_flRenderDistSq <= flDistSq)
					continue;
			}
		}
#endif

		if( renderable.m_RenderGroup == RENDER_GROUP_TRANSLUCENT_ENTITY )
		{
			// Translucent entities already have had ComputeTranslucentRenderLeaf called on them
			// so m_RenderLeaf should be set to the nearest leaf, so that's what we want here.
			if( renderable.m_RenderLeaf == leaf )
			{
				if( renderable.m_pRenderable->LODTest() )
				{
					bool twoPass = (renderable.m_Flags & RENDER_FLAGS_TWOPASS) != 0; 
					AddRenderableToRenderList( *info.m_pRenderList, renderable.m_pRenderable, 
						worldListLeafIndex, (RenderGroup_t)renderable.m_RenderGroup, handle, twoPass );

					// Add to both lists if it's a two-pass model... 
					if (twoPass)
					{
						AddRenderableToRenderList( *info.m_pRenderList, renderable.m_pRenderable, 
							worldListLeafIndex, RENDER_GROUP_OPAQUE_ENTITY, handle, twoPass );
					}
				}
			}
		}
		else
		{
			// Don't hit the same ent in multiple leaves twice.
			if( renderable.m_pRenderable->LODTest() )
			{
				AddRenderableToRenderList( *info.m_pRenderList, renderable.m_pRenderable, 
					worldListLeafIndex, (RenderGroup_t)renderable.m_RenderGroup, handle);
			}
		}
	}

	// Do detail objects.
	// These don't have render handles!
	if( info.m_bDrawDetailObjects && ShouldDrawDetailObjectsInLeaf( leaf, info.m_nRenderFrame ) )
	{
		idx = m_Leaf[leaf].m_FirstDetailProp;
		int count = m_Leaf[leaf].m_DetailPropCount;
		while( --count >= 0 )
		{
			IClientRenderable* pRenderable = DetailObjectSystem()->GetDetailModel(idx);

			// FIXME: This if check here is necessary because the detail object system also maintains
			// lists of sprites...
			if (pRenderable)
			{
				if( pRenderable->IsTransparent() )
				{
					// Lots of the detail entities are invsible so avoid sorting them and all that.
					if( pRenderable->GetFxBlend() > 0 )
					{
						AddRenderableToRenderList( *info.m_pRenderList, pRenderable, 
							worldListLeafIndex, RENDER_GROUP_TRANSLUCENT_ENTITY, DETAIL_PROP_RENDER_HANDLE );
					}
				}
				else
				{
					AddRenderableToRenderList( *info.m_pRenderList, pRenderable, 
						worldListLeafIndex, RENDER_GROUP_OPAQUE_ENTITY, DETAIL_PROP_RENDER_HANDLE );
				}
			}
			++idx;
		}
	}
}
