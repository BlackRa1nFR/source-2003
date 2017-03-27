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

#include "staticpropmgr.h"
#include "convar.h"
#include "vcollide_parse.h"
#include "engine/icollideable.h"
#include "iclientunknown.h"
#include "iclientrenderable.h"
#include "gamebspfile.h"
#include "engine/ivmodelrender.h"
#include "engine/IClientLeafSystem.h"
#include "ispatialpartitioninternal.h"
#include "utlbuffer.h"
#include "utlvector.h"
#include "gl_model_private.h"
#include "gl_matsysiface.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "lightcache.h"
#include "tier0/vprof.h"
#include "render.h"
#include "cmodel_engine.h"
#include "ModelInfo.h"
#include "cdll_engine_int.h"
#include "tier0/dbg.h"


//-----------------------------------------------------------------------------
// Convars!
//-----------------------------------------------------------------------------
static ConVar r_DrawSpecificStaticProp( "r_DrawSpecificStaticProp", "-1" );
static ConVar r_drawstaticprops( "r_drawstaticprops", "1" );
static ConVar r_colorstaticprops( "r_colorstaticprops", "0" );
static ConVar vcollide_wireframe( "vcollide_wireframe", "0" );


//-----------------------------------------------------------------------------
// All static props have these bits set (to differentiate them from edict indices)
//-----------------------------------------------------------------------------
enum
{
	// This bit will be set in GetRefEHandle for all static props
	STATICPROP_EHANDLE_MASK = 0x40000000
};


//-----------------------------------------------------------------------------
// A default physics property for non-vphysics static props
//-----------------------------------------------------------------------------
static const objectparams_t g_PhysDefaultObjectParams =
{
	NULL,
	1.0, //mass
	1.0, // inertia
	0.1f, // damping
	0.1f, // rotdamping
	0.5, // rotIntertiaLimit
	"DEFAULT",
	NULL,// game data
	0.f, // volume (leave 0 if you don't have one or call physcollision->CollideVolume() to compute it)
	1.0f, // drag coefficient
	1.0f, // rolling drag
	true,// enable collisions?
};


//-----------------------------------------------------------------------------
// A static prop
//-----------------------------------------------------------------------------
class CStaticProp : public IClientUnknown, public IClientRenderable, public ICollideable
{
public:
	CStaticProp();
	~CStaticProp();

// IHandleEntity overrides
public:
	void SetRefEHandle( const CBaseHandle &handle );
	const CBaseHandle& GetRefEHandle() const;
	
// IClientUnknown overrides.
public:
	virtual IClientUnknown*		GetIClientUnknown()		{ return this; }
	virtual ICollideable*		GetClientCollideable()	{ return this; }
	virtual IClientNetworkable*	GetClientNetworkable()	{ return NULL; }
	virtual IClientRenderable*	GetClientRenderable()	{ return this; }
	virtual IClientEntity*		GetIClientEntity()		{ return NULL; }
	virtual C_BaseEntity*		GetBaseEntity()			{ return NULL; }
	virtual IClientThinkable*	GetClientThinkable()	{ return NULL; }

public:
	// These methods return a *world-aligned* box relative to the absorigin of the entity.
	virtual const Vector&	WorldAlignMins( ) const;
	virtual const Vector&	WorldAlignMaxs( ) const;

	// These methods return a box defined in the space of the entity
 	virtual const Vector&	EntitySpaceMins( ) const;
	virtual const Vector&	EntitySpaceMaxs( ) const;

	// Returns the center of the entity
	virtual bool			IsBoundsDefinedInEntitySpace() const;

	// custom collision test
	virtual bool			TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	// Perform hitbox test, returns true *if hitboxes were tested at all*!!
	virtual bool			TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	// Returns the BRUSH model index if this is a brush model. Otherwise, returns -1.
	virtual int				GetCollisionModelIndex();

	// Return the model, if it's a studio model.
	virtual const model_t*	GetCollisionModel();

	// Get angles and origin.
	virtual const Vector&	GetCollisionOrigin();
	virtual const QAngle&	GetCollisionAngles();

	// Return a SOLID_ define.
	virtual SolidType_t		GetSolid() const;
	virtual int				GetSolidFlags() const;

	// Gets at the entity handle associated with the collideable
	virtual IHandleEntity	*GetEntityHandle() { return this; }

	virtual int				GetCollisionGroup() { return COLLISION_GROUP_NONE; }

// IClientRenderable overrides.
public:
	virtual int				GetBody() { return 0; }
	virtual const Vector&	GetRenderOrigin( );
	virtual const QAngle&	GetRenderAngles( );
	virtual bool			ShouldDraw();
	virtual bool			IsTransparent( void );
	virtual const model_t*	GetModel( );
	virtual int				DrawModel( int flags );
	virtual void			ComputeFxBlend( );
	virtual int				GetFxBlend( );
	virtual void			GetColorModulation( float* color );
	virtual bool			LODTest();
	virtual bool			SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );
	virtual void			SetupWeights( void );
	virtual void			DoAnimationEvents( void );
	virtual void			GetRenderBounds( Vector& mins, Vector& maxs );
	virtual const Vector&	EntitySpaceSurroundingMins() const;
	virtual const Vector&	EntitySpaceSurroundingMaxs() const;
	virtual const Vector&	WorldSpaceSurroundingMins() const;
	virtual const Vector&	WorldSpaceSurroundingMaxs() const;
	virtual bool			ShouldCacheRenderInfo();
	virtual bool			ShouldReceiveShadows();
	virtual bool			UsesFrameBufferTexture();

public:
	bool Init( int index, StaticPropLump_t &lump, model_t *pModel );

	// KD Tree
	void InsertPropIntoKDTree();
	void RemovePropFromKDTree();

	void PrecacheLighting( );
	void RecomputeStaticLighting( );

	int LeafCount() const;
	int FirstLeaf() const;
	LightCacheHandle_t GetLightCacheHandle() const;
	ModelInstanceHandle_t GetModelInstance() const;
	void SetModelInstance( ModelInstanceHandle_t handle );
	void SetRenderHandle( ClientRenderHandle_t handle );
	void CleanUpRenderHandle( );
	ClientRenderHandle_t GetRenderHandle() const;
	void SetAlpha( unsigned char alpha );

	// Create VPhysics representation
	void CreateVPhysics( IPhysicsEnvironment *physenv, IVPhysicsKeyHandler *pDefaults, void *pGameData );

private:
	Vector					m_Origin;
	QAngle					m_Angles;
	model_t*				m_pModel;
	SpatialPartitionHandle_t	m_Partition;
	ModelInstanceHandle_t	m_ModelInstance;
	unsigned char			m_Alpha;
	unsigned char			m_nSolidType;
	unsigned char			m_Skin;
	unsigned short			m_FirstLeaf;
	unsigned short			m_LeafCount;
 	CBaseHandle				m_EntHandle;	// FIXME: Do I need client + server handles?
	LightCacheHandle_t		m_LightCacheHandle;
	ClientRenderHandle_t	m_RenderHandle;

	// bbox is the same for both GetBounds and GetRenderBounds since static props never move.
	// GetRenderBounds is interpolated data, and GetBounds is last networked.
	Vector					m_RenderBBoxMin;
	Vector					m_RenderBBoxMax;
	matrix3x4_t				m_ModelToWorld;

	// FIXME: This sucks. Need to store the lighting origin off
	// because the time at which the static props are unserialized
	// doesn't necessarily match the time at which we can initialize the light cache
	Vector					m_LightingOrigin;
};


//-----------------------------------------------------------------------------
// The engine's static prop manager
//-----------------------------------------------------------------------------
class CStaticPropMgr : public IStaticPropMgrEngine, public IStaticPropMgrClient, public IStaticPropMgrServer
{
public:
	// constructor, destructor
	CStaticPropMgr();
	virtual ~CStaticPropMgr();

	// methods of IStaticPropMgrEngine
	virtual bool Init();
	virtual void Shutdown();
	virtual void LevelInit();
	virtual void LevelInitClient();
	virtual void LevelShutdown();
	virtual void LevelShutdownClient();
	virtual bool IsPropInPVS( IHandleEntity *pHandleEntity, const byte *pVis ) const;
	virtual ICollideable *GetStaticProp( IHandleEntity *pHandleEntity );
	virtual void RecomputeStaticLighting( );
 	virtual LightCacheHandle_t GetLightCacheHandleForStaticProp( IHandleEntity *pHandleEntity );
	virtual bool IsStaticProp( IHandleEntity *pHandleEntity ) const;
	virtual bool IsStaticProp( CBaseHandle handle ) const;
	virtual int GetStaticPropIndex( IHandleEntity *pHandleEntity ) const;

	// methods of IStaticPropMgrClient
	virtual void ComputePropOpacity( const Vector &viewOrigin );
	virtual void TraceRayAgainstStaticProp( const Ray_t& ray, int staticPropIndex, trace_t& tr );
	virtual void AddDecalToStaticProp( Vector const& rayStart, Vector const& rayEnd,
					int staticPropIndex, int decalIndex, bool doTrace, trace_t& tr );
	virtual void AddShadowToStaticProp( unsigned short shadowHandle, IClientRenderable* pRenderable );
	virtual void RemoveAllShadowsFromStaticProp( IClientRenderable* pRenderable );
	virtual void GetStaticPropMaterialColorAndLighting( trace_t* pTrace,
					int staticPropIndex, Vector& lighting, Vector& matColor );
	virtual void CreateVPhysicsRepresentations( IPhysicsEnvironment	*physenv, IVPhysicsKeyHandler *pDefaults, void *pGameData );

	// methods of IStaticPropMgrServer

private:
	void OutputLevelStats( void );
 	void PrecacheLighting( );

	// Methods associated with unserializing static props
	void UnserializeModelDict( CUtlBuffer& buf );
	void UnserializeLeafList( CUtlBuffer& buf );
	void UnserializeModels( CUtlBuffer& buf );
	void UnserializeStaticProps();

	int HandleEntityToIndex( IHandleEntity *pHandleEntity ) const;

private:
	// Unique static prop models
	struct StaticPropDict_t
	{
		model_t* m_pModel;
	};

	// Static props that fade use this data to fade
	struct StaticPropFade_t
	{
		int		m_Model;
		float	m_MinDistSq;
		float	m_MaxDistSq;
		float	m_FalloffFactor;
	};

	// The list of all static props
	CUtlVector <StaticPropDict_t>	m_StaticPropDict;
	CUtlVector <CStaticProp>		m_StaticProps;
	CUtlVector <StaticPropLeafLump_t> m_StaticPropLeaves;

	// Static props that fade...
	CUtlVector<StaticPropFade_t>	m_StaticPropFade;

	bool							m_bLevelInitialized;
	bool							m_bClientInitialized;
};


//-----------------------------------------------------------------------------
// Expose Interface to the game + client DLLs.
//-----------------------------------------------------------------------------
static CStaticPropMgr	s_StaticPropMgr;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CStaticPropMgr, IStaticPropMgrClient, INTERFACEVERSION_STATICPROPMGR_CLIENT, s_StaticPropMgr);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CStaticPropMgr, IStaticPropMgrServer, INTERFACEVERSION_STATICPROPMGR_SERVER, s_StaticPropMgr);


//-----------------------------------------------------------------------------
//
// Static prop
//
//-----------------------------------------------------------------------------
CStaticProp::CStaticProp() : m_pModel(0), m_Alpha(255)
{
	m_ModelInstance = MODEL_INSTANCE_INVALID;
	m_Partition = PARTITION_INVALID_HANDLE;
	m_EntHandle = INVALID_EHANDLE_INDEX;
	m_RenderHandle = INVALID_CLIENT_RENDER_HANDLE;
	m_LightCacheHandle = 0;
}

CStaticProp::~CStaticProp()
{
	RemovePropFromKDTree( );
	if (m_ModelInstance != MODEL_INSTANCE_INVALID)
	{
		modelrender->DestroyInstance( m_ModelInstance );
	}
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
bool CStaticProp::Init( int index, StaticPropLump_t &lump, model_t *pModel )
{
	m_EntHandle = index | STATICPROP_EHANDLE_MASK; 
	m_Partition = PARTITION_INVALID_HANDLE;

	VectorCopy( lump.m_Origin, m_Origin );
	VectorCopy( lump.m_Angles, m_Angles );
	m_pModel = pModel;
	m_FirstLeaf = lump.m_FirstLeaf;
	m_LeafCount = lump.m_LeafCount;
	m_nSolidType = lump.m_Solid;
	m_Alpha = 255;
	m_Skin = (unsigned char)lump.m_Skin;

	// Cache the collision bounding box since it'll never change.
	modelinfo->GetModelRenderBounds( m_pModel, 0, m_RenderBBoxMin, m_RenderBBoxMax );

	// Cache the model to world matrix since it never changes.
	AngleMatrix( lump.m_Angles, lump.m_Origin, m_ModelToWorld );

	// FIXME: Sucky, but unless we want to re-read the static prop lump when the client is
	// initialized (possible, but also gross), we need to cache off the illum center now
	if (lump.m_Flags & STATIC_PROP_USE_LIGHTING_ORIGIN)
	{
		m_LightingOrigin = lump.m_LightingOrigin;
	}
	else
	{
		modelinfo->GetIlluminationPoint( m_pModel, m_Origin, m_Angles, &m_LightingOrigin );
	}

	return true;
}


//-----------------------------------------------------------------------------
// EHandle
//-----------------------------------------------------------------------------
void CStaticProp::SetRefEHandle( const CBaseHandle &handle )
{
	// Only the static prop mgr should be setting this...
	Assert( 0 );
}


const CBaseHandle& CStaticProp::GetRefEHandle() const
{
	return m_EntHandle;
}


//-----------------------------------------------------------------------------
// These methods return a *world-aligned* box relative to the absorigin of the entity.
//-----------------------------------------------------------------------------
const Vector& CStaticProp::WorldAlignMins( ) const
{
	// FIXME!
	return m_pModel->mins;
}

const Vector& CStaticProp::WorldAlignMaxs( ) const
{
	// FIXME!
	return m_pModel->maxs;
}


//-----------------------------------------------------------------------------
// These methods return a box defined in the space of the entity
//-----------------------------------------------------------------------------
const Vector& CStaticProp::EntitySpaceMins( ) const
{
	// NOTE: There's some trickiness here about which space this is defined in.
	// It's essential that this box be defined in *local* space as opposed to
	// world space for the transformed box traces to work.

	// Therefore, we can't use the bounding box stored in the spatial partition
	// system, as that's a AABB.
	return m_pModel->mins;
}

const Vector& CStaticProp::EntitySpaceMaxs( ) const
{
	return m_pModel->maxs;
}


//-----------------------------------------------------------------------------
// Returns the center of the entity
//-----------------------------------------------------------------------------
bool CStaticProp::IsBoundsDefinedInEntitySpace() const
{
	// FIXME!
	return true;
}


//-----------------------------------------------------------------------------
// Data accessors
//-----------------------------------------------------------------------------
const Vector& CStaticProp::GetRenderOrigin( void )
{
	return m_Origin;
}

const QAngle& CStaticProp::GetRenderAngles( void )
{
	return m_Angles;
}

bool CStaticProp::IsTransparent( void )
{
	return (m_Alpha < 255) || modelinfo->IsTranslucent(m_pModel);
}

bool CStaticProp::ShouldDraw()
{
	return true;
}


//-----------------------------------------------------------------------------
// Render setup
//-----------------------------------------------------------------------------
bool CStaticProp::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	if (!m_pModel)
		return false;

	// Just copy it on down baby
	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( m_pModel );
	for (int i = 0; i < pStudioHdr->numbones; i++) 
	{
		MatrixCopy( m_ModelToWorld, pBoneToWorldOut[i] );
	}

	return true;
}

void	CStaticProp::SetupWeights( void )
{
}

void	CStaticProp::DoAnimationEvents( void )
{
}


//-----------------------------------------------------------------------------
// Render baby!
//-----------------------------------------------------------------------------
const model_t* CStaticProp::GetModel( )
{
	return m_pModel;
}


//-----------------------------------------------------------------------------
// Accessors
//-----------------------------------------------------------------------------
inline int CStaticProp::LeafCount() const 
{ 
	return m_LeafCount; 
}

inline int CStaticProp::FirstLeaf() const 
{ 
	return m_FirstLeaf; 
}

inline LightCacheHandle_t CStaticProp::GetLightCacheHandle() const 
{ 
	return m_LightCacheHandle; 
}

inline ModelInstanceHandle_t CStaticProp::GetModelInstance() const
{
	return m_ModelInstance;
}

inline void CStaticProp::SetModelInstance( ModelInstanceHandle_t handle )
{
	m_ModelInstance = handle;
}

inline void CStaticProp::SetRenderHandle( ClientRenderHandle_t handle )
{
	m_RenderHandle = handle;
}

inline ClientRenderHandle_t CStaticProp::GetRenderHandle() const
{
	return m_RenderHandle;
}

void CStaticProp::CleanUpRenderHandle( )
{
	if ( m_RenderHandle != INVALID_CLIENT_RENDER_HANDLE )
	{
#ifndef SWDS
		clientleafsystem->RemoveRenderable( m_RenderHandle );
#endif
		m_RenderHandle = INVALID_CLIENT_RENDER_HANDLE;
	}
}


//-----------------------------------------------------------------------------
// Determine alpha and blend amount for transparent objects based on render state info
//-----------------------------------------------------------------------------
inline void CStaticProp::SetAlpha( unsigned char alpha )
{
	m_Alpha = alpha;
}

void CStaticProp::ComputeFxBlend( )
{
	// Do nothing, we already have it in m_Alpha
}

int CStaticProp::GetFxBlend( )
{
	return m_Alpha;
}

void CStaticProp::GetColorModulation( float* color )
{
	color[0] = color[1] = color[2] = 1.0f;
}


//-----------------------------------------------------------------------------
// Returns false if the entity shouldn't be drawn due to LOD.
//-----------------------------------------------------------------------------
bool CStaticProp::LODTest()
{
	return true;
}


//-----------------------------------------------------------------------------
// custom collision test
//-----------------------------------------------------------------------------
bool CStaticProp::TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	Assert(0);
	return false;
}


//-----------------------------------------------------------------------------
// Perform hitbox test, returns true *if hitboxes were tested at all*!!
//-----------------------------------------------------------------------------
bool CStaticProp::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	return false;
}


//-----------------------------------------------------------------------------
// Returns the BRUSH model index if this is a brush model. Otherwise, returns -1.
//-----------------------------------------------------------------------------
int	CStaticProp::GetCollisionModelIndex()
{
	return -1;
}


//-----------------------------------------------------------------------------
// Return the model, if it's a studio model.
//-----------------------------------------------------------------------------
const model_t* CStaticProp::GetCollisionModel()
{
	return m_pModel;
}


//-----------------------------------------------------------------------------
// Get angles and origin.
//-----------------------------------------------------------------------------
const Vector& CStaticProp::GetCollisionOrigin()
{
	return m_Origin;
}

const QAngle& CStaticProp::GetCollisionAngles()
{
	return m_Angles;
}


//-----------------------------------------------------------------------------
// Return a SOLID_ define.
//-----------------------------------------------------------------------------
SolidType_t CStaticProp::GetSolid() const
{
	return (SolidType_t)m_nSolidType;
}

int	CStaticProp::GetSolidFlags() const
{
	return 0;
}

bool CStaticProp::UsesFrameBufferTexture( void )
{
	studiohdr_t *pmodel  = modelinfo->GetStudiomodel( m_pModel );
	if ( !pmodel )
		return false;

	return ( pmodel->flags & STUDIOHDR_FLAGS_USES_FB_TEXTURE ) ? true : false;
}


void CStaticProp::GetRenderBounds( Vector& mins, Vector& maxs )
{
	mins = m_RenderBBoxMin;
	maxs = m_RenderBBoxMax;
}

const Vector& CStaticProp::EntitySpaceSurroundingMins() const
{
	return m_RenderBBoxMin;
}

const Vector& CStaticProp::EntitySpaceSurroundingMaxs() const
{
	return m_RenderBBoxMax;
}

const Vector& CStaticProp::WorldSpaceSurroundingMins() const
{
	Assert(0);
	return vec3_origin;
}

const Vector& CStaticProp::WorldSpaceSurroundingMaxs() const
{
	Assert(0);
	return vec3_origin;
}

bool CStaticProp::ShouldReceiveShadows()
{ 
	return g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 90; 
}


bool CStaticProp::ShouldCacheRenderInfo()
{
	return true;
}

		
void CStaticProp::PrecacheLighting( )
{
#ifndef SWDS
	m_LightCacheHandle = CreateStaticLightingCache( m_LightingOrigin );
	if( m_ModelInstance == MODEL_INSTANCE_INVALID )
	{
		modelrender->UseLightcache( &m_LightCacheHandle );
		m_ModelInstance = modelrender->CreateInstance( this, true );
		modelrender->UseLightcache( NULL );
	}
#endif
}

static void RandomColorFromModeInstanceHandle( ModelInstanceHandle_t in, Vector &color )
{
	srand( in );
	rand();
	rand();
	color[0] = ( float )rand() / ( float )RAND_MAX;
	color[1] = ( float )rand() / ( float )RAND_MAX;
	color[2] = ( float )rand() / ( float )RAND_MAX;
	VectorNormalize( color );
}

void CStaticProp::RecomputeStaticLighting( void )
{
#ifndef SWDS
	Assert( m_ModelInstance != MODEL_INSTANCE_INVALID );
	modelrender->UseLightcache( &m_LightCacheHandle );
	modelrender->RecomputeStaticLighting( this, m_ModelInstance );
	modelrender->UseLightcache( NULL );
#endif
}

int CStaticProp::DrawModel( int flags )
{
#ifndef SWDS
	VPROF_BUDGET( "CStaticProp::DrawModel", VPROF_BUDGETGROUP_STATICPROP_RENDERING );

	if( !r_drawstaticprops.GetBool() )
	{
		return 0;
	}

#ifdef _DEBUG
	if (r_DrawSpecificStaticProp.GetInt() >= 0)
	{
		if ( (m_EntHandle.ToInt() & (~STATICPROP_EHANDLE_MASK) ) != r_DrawSpecificStaticProp.GetInt())
			return 0;
	}
#endif

	if ((m_Alpha == 0) || (!m_pModel))
		return 0;

	if( r_colorstaticprops.GetBool() )
	{
		Vector color;
		RandomColorFromModeInstanceHandle( m_ModelInstance, color );
		VectorCopy( color.Base(), r_colormod );
	}

	modelrender->UseLightcache( &m_LightCacheHandle );

#ifdef _DEBUG
	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( m_pModel );
	Assert( pStudioHdr );
	if( !( pStudioHdr->flags & STUDIOHDR_FLAGS_STATIC_PROP ) )
	{
		return 0;
	}
#endif

	flags |= STUDIO_STATIC_LIGHTING;
#if 0
	Vector mins;
	Vector maxs;
	VectorAdd( m_RenderBBoxMin, m_Origin, mins );
	VectorAdd( m_RenderBBoxMax, m_Origin, maxs );
#endif	
	int drawn = modelrender->DrawModel( 
		flags, 
		this,
		m_ModelInstance,
		-1,		// no entity index
		m_pModel,
		m_Origin,
		m_Angles,
		0,		// sequence
		m_Skin,	// skin
		0,		// body
		0,		// hitboxset
#if 1
		NULL, NULL,
#else
		&mins, &maxs,
#endif
		&m_ModelToWorld
		);

	modelrender->UseLightcache();

	if ( vcollide_wireframe.GetBool() )
	{
		if ( m_pModel && m_nSolidType == SOLID_VPHYSICS )
		{
			// This works because VCollideForModel only uses modelindex for mod_brush
			// and props are always mod_Studio.
			vcollide_t * pCollide = CM_VCollideForModel( -1, m_pModel ); 
			if ( pCollide && pCollide->solidCount == 1 )
			{
				static color32 debugColor = {0,255,255,0};
				DebugDrawPhysCollide( pCollide->solids[0], NULL, m_ModelToWorld, debugColor );
			}
		}
	}

	return drawn;
#else
	return 0;
#endif
}


//-----------------------------------------------------------------------------
// KD Tree
//-----------------------------------------------------------------------------
void CStaticProp::InsertPropIntoKDTree()
{
	Assert( m_Partition == PARTITION_INVALID_HANDLE );
	if ( m_nSolidType == SOLID_NONE )
		return;

	// Compute the bbox of the prop
	// FIXME: mins/maxs isn't an absbox, right?
	Vector mins, maxs;
	VectorAdd( m_pModel->mins, m_Origin, mins );
	VectorAdd( m_pModel->maxs, m_Origin, maxs );

	// If it's using vphysics, get a good AABB
	if ( m_nSolidType == SOLID_VPHYSICS )
	{
		vcollide_t *pCollide = CM_VCollideForModel( -1, m_pModel );
		if ( pCollide )
		{
			physcollision->CollideGetAABB( mins, maxs, pCollide->solids[0], m_Origin, m_Angles );
		}
	}

	// add the entity to the KD tree so we will collide against it
	m_Partition = SpatialPartition()->CreateHandle( this, 
		PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_STATIC_PROPS | 
		PARTITION_ENGINE_SOLID_EDICTS | PARTITION_ENGINE_STATIC_PROPS, 
		mins, maxs );

	Assert( m_Partition != PARTITION_INVALID_HANDLE );
}

void CStaticProp::RemovePropFromKDTree()
{
	// Release the spatial partition handle
	if ( m_Partition != PARTITION_INVALID_HANDLE )
	{
		SpatialPartition()->DestroyHandle( m_Partition );
		m_Partition = PARTITION_INVALID_HANDLE;
	}
}


//-----------------------------------------------------------------------------
// Create VPhysics representation
//-----------------------------------------------------------------------------
void CStaticProp::CreateVPhysics( IPhysicsEnvironment *pPhysEnv, IVPhysicsKeyHandler *pDefaults, void *pGameData )
{
	if ( m_nSolidType == SOLID_NONE )
		return;

	vcollide_t temp;
	solid_t solid;
	CPhysCollide* pTempStorage;
	vcollide_t *pVCollide = NULL;

	if ( m_pModel && m_nSolidType == SOLID_VPHYSICS )
	{
		// This works because VCollideForModel only uses modelindex for mod_brush
		// and props are always mod_Studio.
		pVCollide = CM_VCollideForModel( -1, m_pModel ); 
	}

	// If there's no collide, we need a bbox...
	if (!pVCollide)
	{
		temp.solidCount = 1;
		temp.pKeyValues = "";
		temp.solids = &pTempStorage;
		temp.solids[0] = physcollision->BBoxToCollide( m_pModel->mins, m_pModel->maxs );
		if (!temp.solids[0])
			return;

		pVCollide = &temp;

		solid.params = g_PhysDefaultObjectParams;
	}
	else
	{
		IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pVCollide->pKeyValues );
		while ( !pParse->Finished() )
		{
			const char *pBlock = pParse->GetCurrentBlockName();
			if ( !strcmpi( pBlock, "solid" ) )
			{
				pParse->ParseSolid( &solid, pDefaults );
				break;
			}
			else
			{
				pParse->SkipBlock();
			}
		}
		physcollision->VPhysicsKeyParserDestroy( pParse );
	}

	solid.params.enableCollisions = true;
	solid.params.pGameData = pGameData;
	solid.params.pName = "prop_static";

	int surfaceData = physprop->GetSurfaceIndex( solid.surfaceprop );
	pPhysEnv->CreatePolyObjectStatic( pVCollide->solids[0], 
		surfaceData, m_Origin, m_Angles, &solid.params );
	//PhysCheckAdd( pPhys, "Static" );
}


//-----------------------------------------------------------------------------
// Expose IStaticPropMgr to the engine
//-----------------------------------------------------------------------------
IStaticPropMgrEngine* StaticPropMgr()
{
	return &s_StaticPropMgr;
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CStaticPropMgr::CStaticPropMgr()
{
	m_bLevelInitialized = false;
	m_bClientInitialized = false;
}

CStaticPropMgr::~CStaticPropMgr()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CStaticPropMgr::Init()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStaticPropMgr::Shutdown()
{
	if ( !m_bLevelInitialized )
		return;

	LevelShutdown();
}

//-----------------------------------------------------------------------------
// Unserialize static prop model dictionary
//-----------------------------------------------------------------------------
void CStaticPropMgr::UnserializeModelDict( CUtlBuffer& buf )
{
	int count = buf.GetInt();
	while ( --count >= 0 )
	{
		StaticPropDictLump_t lump;
		buf.Get( &lump, sizeof(StaticPropDictLump_t) );
		
		StaticPropDict_t dict;
		dict.m_pModel = (model_t *)modelloader->GetModelForName(
			lump.m_Name, IModelLoader::FMODELLOADER_STATICPROP );
		m_StaticPropDict.AddToTail( dict );
	}
}

void CStaticPropMgr::UnserializeLeafList( CUtlBuffer& buf )
{
	int nCount = buf.GetInt();
	m_StaticPropLeaves.Purge();
	if ( nCount > 0 )
	{
		m_StaticPropLeaves.AddMultipleToTail( nCount );
		buf.Get( m_StaticPropLeaves.Base(), nCount * sizeof(StaticPropLeafLump_t) );
	}
}

void CStaticPropMgr::UnserializeModels( CUtlBuffer& buf )
{
	// Version check
	if ( Mod_GameLumpVersion( GAMELUMP_STATIC_PROPS ) < 4 )
	{
		Warning("Really old map format! Static props can't be loaded...\n");
		return;
	}

	int count = buf.GetInt();

	// Gotta preallocate the static props here so no rellocations take place
	// the leaf list stores pointers to these tricky little guys.
	m_StaticProps.AddMultipleToTail(count);
	for ( int i = 0; i < count; ++i )
	{
		StaticPropLump_t lump;
		buf.Get( &lump, sizeof(StaticPropLump_t) );
 		m_StaticProps[i].Init( i, lump, m_StaticPropDict[lump.m_PropType].m_pModel );

		// For distance-based fading, keep a list of the things that need
		// to be faded out. Not sure if this is the optimal way of doing it
		// but it's easy for now; we'll have to test later how large this list gets.
		// If it's <100 or so, we should be fine
		if (lump.m_Flags & STATIC_PROP_FLAG_FADES)
		{
			int idx = m_StaticPropFade.AddToTail();
			StaticPropFade_t& fade = m_StaticPropFade[idx];
			fade.m_Model = i;
			fade.m_MinDistSq = lump.m_FadeMinDist * lump.m_FadeMinDist;
			fade.m_MaxDistSq = lump.m_FadeMaxDist * lump.m_FadeMaxDist;
			if (fade.m_MaxDistSq != fade.m_MinDistSq)
				fade.m_FalloffFactor = 255.0f / (fade.m_MaxDistSq - fade.m_MinDistSq);
			else
				fade.m_FalloffFactor = 255.0f;
		}

		// Add the prop to the K-D tree for collision
		m_StaticProps[i].InsertPropIntoKDTree( );
	}
}

void CStaticPropMgr::OutputLevelStats( void )
{
	// STATS
	int i;
	int totalVerts = 0;
	for( i = 0; i < m_StaticProps.Count(); i++ )
	{
		CStaticProp *pStaticProp = &m_StaticProps[i];
		model_t *pModel = (model_t*)pStaticProp->GetModel();
		if( !pModel )
		{
			continue;
		}
		Assert( pModel->type == mod_studio );
		studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( pModel );
		int bodyPart;
		for( bodyPart = 0; bodyPart < pStudioHdr->numbodyparts; bodyPart++ )
		{
			mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( bodyPart );
			int model;
			for( model = 0; model < pBodyPart->nummodels; model++ )
			{
				mstudiomodel_t *pModel = pBodyPart->pModel( model );
				totalVerts += pModel->numvertices;
			}
		}
	}
	Warning( "%d static prop instances in map\n", ( int )m_StaticProps.Count() );
	Warning( "%d static prop models in map\n", ( int )m_StaticPropDict.Count() );
	Warning( "%d static prop verts in map\n", ( int )totalVerts );
}


//-----------------------------------------------------------------------------
// Unserialize static props
//-----------------------------------------------------------------------------
void CStaticPropMgr::UnserializeStaticProps()
{
	// Unserialize static props, insert them into the appropriate leaves
	int size = Mod_GameLumpSize( GAMELUMP_STATIC_PROPS );
	if (!size)
		return;

	CUtlBuffer buf( 0, size );
	if (Mod_LoadGameLump( GAMELUMP_STATIC_PROPS, buf.PeekPut(), size ))
	{
		UnserializeModelDict( buf );
		UnserializeLeafList( buf );
		UnserializeModels( buf );
	}
}


//-----------------------------------------------------------------------------
// Level init, shutdown
//-----------------------------------------------------------------------------
void CStaticPropMgr::LevelInit()
{
	if ( m_bLevelInitialized )
	{
		return;
	}

	Assert( !m_bClientInitialized );
	m_bLevelInitialized = true;

	// Read in static props that have been compiled into the bsp file
	UnserializeStaticProps();

	//	OutputLevelStats();
}

void CStaticPropMgr::LevelInitClient()
{
	Assert( m_bLevelInitialized );
	Assert( !m_bClientInitialized );
#ifndef SWDS
	if (cls.state != ca_dedicated)
	{
		// Since the client will be ready at a later time than the server
		// to set up its data, we need a separate call to handle that
		int nCount = m_StaticProps.Count();
		for ( int i = 0; i < nCount; ++i )
		{
			CStaticProp &prop = m_StaticProps[i];

			// Add the prop to all the leaves it lies in
			ClientRenderHandle_t handle = clientleafsystem->CreateRenderableHandle( &m_StaticProps[i], true );
			clientleafsystem->AddRenderableToLeaves( handle, prop.LeafCount(), (unsigned short*)&m_StaticPropLeaves[prop.FirstLeaf()] ); 
			m_StaticProps[i].SetRenderHandle( handle );
		}

		PrecacheLighting();

		m_bClientInitialized = true;
	}
#endif
}

void CStaticPropMgr::LevelShutdownClient()
{
	if ( !m_bClientInitialized )
		return;

	Assert( m_bLevelInitialized );

	for (int i = m_StaticProps.Count(); --i >= 0; )
	{
		m_StaticProps[i].CleanUpRenderHandle( );
	}

#ifndef SWDS
	// Make sure static prop lightcache is reset
	ClearStaticLightingCache();
#endif

	m_bClientInitialized = false;
}

void CStaticPropMgr::LevelShutdown()
{
	if ( !m_bLevelInitialized )
		return;

	// Deal with client-side stuff, if appropriate
	if ( m_bClientInitialized )
	{
		LevelShutdownClient();
	}

	m_bLevelInitialized = false;

	int i;
	for ( i = m_StaticPropDict.Count(); --i >= 0; )
	{
		modelloader->ReleaseModel( m_StaticPropDict[i].m_pModel, IModelLoader::FMODELLOADER_STATICPROP );
	}

	m_StaticProps.Purge();
	m_StaticPropDict.Purge();
	m_StaticPropFade.Purge();
}


//-----------------------------------------------------------------------------
// Create physics representations of props
//-----------------------------------------------------------------------------
void CStaticPropMgr::CreateVPhysicsRepresentations( IPhysicsEnvironment	*pPhysEnv, IVPhysicsKeyHandler *pDefaults, void *pGameData )
{
	// Walk through the static props + make collideable thingies for them.
	int nCount = m_StaticProps.Count();
	for ( int i = nCount; --i >= 0; )
	{
		m_StaticProps[i].CreateVPhysics( pPhysEnv, pDefaults, pGameData );
	}
}


//-----------------------------------------------------------------------------
// Handles to props
//-----------------------------------------------------------------------------
inline int CStaticPropMgr::HandleEntityToIndex( IHandleEntity *pHandleEntity ) const
{
	Assert( IsStaticProp( pHandleEntity ) );
	return pHandleEntity->GetRefEHandle().ToInt() & (~STATICPROP_EHANDLE_MASK);
}

ICollideable *CStaticPropMgr::GetStaticProp( IHandleEntity *pHandleEntity )
{
	int nIndex = pHandleEntity ? pHandleEntity->GetRefEHandle().ToInt() : 0;
	return ( nIndex & STATICPROP_EHANDLE_MASK ) ? &m_StaticProps[nIndex & (~STATICPROP_EHANDLE_MASK)] : 0;
}


//-----------------------------------------------------------------------------
// Are we a static prop?
//-----------------------------------------------------------------------------
bool CStaticPropMgr::IsStaticProp( IHandleEntity *pHandleEntity ) const
{
	return (!pHandleEntity) || ( (pHandleEntity->GetRefEHandle().ToInt() & STATICPROP_EHANDLE_MASK) != 0 );
}

bool CStaticPropMgr::IsStaticProp( CBaseHandle handle ) const
{
	return (handle.ToInt() & STATICPROP_EHANDLE_MASK) != 0;
}

int CStaticPropMgr::GetStaticPropIndex( IHandleEntity *pHandleEntity ) const
{
	return HandleEntityToIndex( pHandleEntity );
}


//-----------------------------------------------------------------------------
// Compute static lighting
//-----------------------------------------------------------------------------
void CStaticPropMgr::PrecacheLighting( )
{
	int i = m_StaticProps.Count();
	while ( --i >= 0 )
	{
		m_StaticProps[i].PrecacheLighting( );
	}
}

void CStaticPropMgr::RecomputeStaticLighting( )
{
	int i = m_StaticProps.Count();
	while ( --i >= 0 )
	{
		m_StaticProps[i].RecomputeStaticLighting();
	}
}


//-----------------------------------------------------------------------------
// Is the prop in the PVS?
//-----------------------------------------------------------------------------
bool CStaticPropMgr::IsPropInPVS( IHandleEntity *pHandleEntity, const byte *pVis ) const
{
	// Strip off the bits
	int nIndex = HandleEntityToIndex( pHandleEntity );

	// Get the prop
	const CStaticProp &prop = m_StaticProps[nIndex];

	int i;
	int end = prop.FirstLeaf() + prop.LeafCount();
	for( i = prop.FirstLeaf(); i < end; i++ )
	{
		Assert( i >= 0 && i < m_StaticPropLeaves.Count() );
		int clusterID = CM_LeafCluster( m_StaticPropLeaves[i].m_Leaf );
		if( pVis[ clusterID >> 3 ] & ( 1 << ( clusterID & 7 ) ) )
		{
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Returns the lightcache handle
//-----------------------------------------------------------------------------
LightCacheHandle_t CStaticPropMgr::GetLightCacheHandleForStaticProp( IHandleEntity *pHandleEntity )
{
	int nIndex = HandleEntityToIndex(pHandleEntity);
	return m_StaticProps[ nIndex ].GetLightCacheHandle();
}


//-----------------------------------------------------------------------------
// System to update prop opacity
//-----------------------------------------------------------------------------
void CStaticPropMgr::ComputePropOpacity( const Vector &viewOrigin )
{
	// We need to recompute translucency information for all static props
	int i;
#ifndef SWDS
	for (i = m_StaticPropDict.Size(); --i >= 0; )
	{
		if (modelinfoclient->ModelHasMaterialProxy( m_StaticPropDict[i].m_pModel ))
		{
			modelinfoclient->RecomputeTranslucency( m_StaticPropDict[i].m_pModel );
		}
	}
#endif

	// Distance-based fading.
	// Step over the list of all things that want to be faded out and recompute alpha

	// Not sure if this is a fast enough way of doing it
	// but it's easy for now; we'll have to test later how large this list gets.
	// If it's <100 or so, we should be fine
	Vector v;
	for ( i = m_StaticPropFade.Count(); --i >= 0; )
	{
		StaticPropFade_t& fade = m_StaticPropFade[i];
 		CStaticProp& prop = m_StaticProps[fade.m_Model];

		// Calculate distance (badly)
		VectorSubtract( prop.GetRenderOrigin(), viewOrigin, v );

#ifndef SWDS
		ClientRenderHandle_t renderHandle = prop.GetRenderHandle();
		float sqDist = v.LengthSqr();
		if ( sqDist < fade.m_MaxDistSq )
		{
			if ((fade.m_MinDistSq >= 0) && (sqDist > fade.m_MinDistSq))
			{
				clientleafsystem->ChangeRenderableRenderGroup( renderHandle, RENDER_GROUP_TRANSLUCENT_ENTITY );
				prop.SetAlpha( fade.m_FalloffFactor * (fade.m_MaxDistSq - sqDist) );
			}
			else
			{
				// We can stick the prop into the opaque list
				clientleafsystem->ChangeRenderableRenderGroup( renderHandle, RENDER_GROUP_OPAQUE_ENTITY );
				prop.SetAlpha( 255 );
			}
		}
		else
		{
			// We can stick the prop into the opaque list now; we're not drawing it!
			clientleafsystem->ChangeRenderableRenderGroup( renderHandle, RENDER_GROUP_OPAQUE_ENTITY );
			prop.SetAlpha( 0 );
		}
#endif
	}
}


//-----------------------------------------------------------------------------
// Purpose: Trace a ray against the specified static Prop. Returns point of intersection in trace_t
//-----------------------------------------------------------------------------
void CStaticPropMgr::TraceRayAgainstStaticProp( const Ray_t& ray, int staticPropIndex, trace_t& tr )
{
#ifndef SWDS
	// Get the prop
	CStaticProp& prop = m_StaticProps[staticPropIndex];

	if (prop.GetSolid() != SOLID_NONE)
	{
		// FIXME: Better bloat?
		// Bloat a little bit so we get the intersection
		Ray_t temp = ray;
		temp.m_Delta *= 1.1f;
		g_pEngineTraceClient->ClipRayToEntity( temp, MASK_ALL, &prop, &tr );
	}
	else
	{
		// no collision
		tr.fraction = 1.0f;
	}
#endif
}


//-----------------------------------------------------------------------------
// Adds decals to static props, returns point of decal in trace_t
//-----------------------------------------------------------------------------
void CStaticPropMgr::AddDecalToStaticProp( Vector const& rayStart, Vector const& rayEnd,
		int staticPropIndex, int decalIndex, bool doTrace, trace_t& tr )
{
#ifndef SWDS
	// Invalid static prop? Blow it off! 
	if (staticPropIndex >= m_StaticProps.Size())
	{
		memset( &tr, 0, sizeof(trace_t) );
		tr.fraction = 1.0f;
		return;
	}

	Ray_t ray;
	ray.Init( rayStart, rayEnd );
	if (doTrace)
	{
		// Trace the ray against the prop
		TraceRayAgainstStaticProp( ray, staticPropIndex, tr );
		if (tr.fraction == 1.0f)
			return;
	}

	// Get the prop
	CStaticProp& prop = m_StaticProps[staticPropIndex];

	// Found the point, now lets apply the decals
	if (prop.GetModelInstance() == MODEL_INSTANCE_INVALID)
	{
		prop.SetModelInstance( modelrender->CreateInstance( &prop ) );
	}

	// Choose a new ray along which to project the decal based on
	// surface normal. This prevents decal skewing
	bool noPokethru = false;
	if (doTrace && (prop.GetSolid() == SOLID_VPHYSICS) && !tr.startsolid && !tr.allsolid)
	{
		Vector temp;
		VectorSubtract( tr.endpos, tr.plane.normal, temp );
		ray.Init( tr.endpos, temp );
		noPokethru = true;
	}

	// FIXME: Pass in decal up?
	// FIXME: What to do about the body parameter?
	Vector up(0, 0, 1);
	modelrender->AddDecal( prop.GetModelInstance(), ray, up, decalIndex, 0, noPokethru );
#endif
}


//-----------------------------------------------------------------------------
// Adds/removes shadows from static props
//-----------------------------------------------------------------------------
void CStaticPropMgr::AddShadowToStaticProp( unsigned short shadowHandle, IClientRenderable* pRenderable )
{
	Assert( dynamic_cast<CStaticProp*>(pRenderable) != 0 );

	CStaticProp* pProp = static_cast<CStaticProp*>(pRenderable);

	// If we don't have a model instance, we need one.
	if (pProp->GetModelInstance() == MODEL_INSTANCE_INVALID)
	{
		pProp->SetModelInstance( modelrender->CreateInstance( pProp ) );
	}
#ifndef SWDS
	g_pShadowMgr->AddShadowToModel( shadowHandle, pProp->GetModelInstance() );
#endif
}

void CStaticPropMgr::RemoveAllShadowsFromStaticProp( IClientRenderable* pRenderable )
{
#ifndef SWDS
	Assert( dynamic_cast<CStaticProp*>(pRenderable) != 0 );
	CStaticProp* pProp = static_cast<CStaticProp*>(pRenderable);
	if (pProp->GetModelInstance() != MODEL_INSTANCE_INVALID)
	{
		g_pShadowMgr->RemoveAllShadowsFromModel( pProp->GetModelInstance() );
	}
#endif
}


//-----------------------------------------------------------------------------
// Gets the lighting + material color of a static prop
//-----------------------------------------------------------------------------
void CStaticPropMgr::GetStaticPropMaterialColorAndLighting( trace_t* pTrace,
	int staticPropIndex, Vector& lighting, Vector& matColor )
{
#ifndef SWDS
	// Invalid static prop? Blow it off! 
	if (staticPropIndex >= m_StaticProps.Size())
	{
		lighting.Init( 0, 0, 0 );
		matColor.Init( 1, 1, 1 );
		return;
	}

	// Get the prop
	CStaticProp& prop = m_StaticProps[staticPropIndex];

	// Ask the model info about what we need to know
	modelinfoclient->GetModelMaterialColorAndLighting( (model_t*)prop.GetModel(), 
		prop.GetRenderOrigin(), prop.GetRenderAngles(), pTrace, lighting, matColor );
#endif
}


//-----------------------------------------------------------------------------
// Little debugger tool to report which prop we're looking at
//-----------------------------------------------------------------------------
void Cmd_PropCrosshair_f (void)
{
	Vector endPoint;
	VectorMA( r_origin, COORD_EXTENT * 1.74f, vpn, endPoint );

	Ray_t ray;
	ray.Init( r_origin, endPoint );

	trace_t tr;
	CTraceFilterWorldAndPropsOnly traceFilter;
	g_pEngineTraceServer->TraceRay( ray, MASK_ALL, &traceFilter, &tr );

	if ( tr.hitbox > 0 ) 
		Msg( "hit prop %d\n", tr.hitbox + 1 );
	else
		Msg( "didn't hit a prop\n" );
}

static ConCommand prop_crosshair( "prop_crosshair", Cmd_PropCrosshair_f );

