//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Contains some client side classes related to particle emission
//-----------------------------------------------------------------------------
// $Revision: $
// $NoKeywords: $
//=============================================================================

//
// This module implements the particle manager for the client DLL.
// In a nutshell, to create your own effect, implement the ParticleEffect 
// interface and call CParticleMgr::AddEffect to add your effect. Then you can 
// add particles and simulate and render them.

/*

Particle manager documentation
-----------------------------------------------------------------------------

All particle effects are managed by a class called CParticleMgr. It tracks 
the list of particles, manages their materials, sorts the particles, and
has callbacks to render them.

Conceptually, CParticleMgr is NOT part of VEngine's entity system. It does
not care about entities, only particle effects. Usually, the two are implemented
together, but you should be aware the CParticleMgr talks to you through its
own interfaces and does not talk to entities. Thus, it is possible to have
particle effects that are not entities.

To make a particle effect, you need two things: 

1. An implementation of the IParticleEffect interface. This is how CParticleMgr 
   talks to you for things like rendering and updating your effect.

2. A (member) variable of type CParticleEffectBinding. This allows CParticleMgr to 
   store its internal data associated with your effect.

Once you have those two things, you call CParticleMgr::AddEffect and pass them
both in. You will then get updates through IParticleEffect::Update, and you will
be asked to render your particles with IParticleEffect::SimulateAndRender.

When you want to remove the effect, call CParticleEffectBinding::SetRemoveFlag(), which
tells CParticleMgr to remove the effect next chance it gets.

Example class:

	class CMyEffect : public IParticleEffect
	{
	public:
		// Call this to start the effect by adding it to the particle manager.
		void			Start()
		{
			g_ParticleMgr.AddEffect( &m_ParticleEffect, this );
		}

		// implementation of IParticleEffect functions go here...

	public:
		CParticleEffectBinding	m_ParticleEffect;
	};



How the particle effects are integrated with the entity system
-----------------------------------------------------------------------------

There are two helper classes that you can use to create particles for your
entities. Each one is useful under different conditions.

1. CSimpleEmitter is a class that does some of the dirty work of using particles.
   If you want, you can just instantiate one of these with CSimpleEmitter::Create
   and call its AddParticle functions to add particles. When you are done and 
   want to 'free' it, call its Release function rather than deleting it, and it
   will wait until all of its particles have gone away before removing itself
   (so you don't have to write code to wait for all of the particles to go away).

   In most cases, it is the easiest and most clear to use CSimpleEmitter or
   derive a class from it, then use that class from inside an entity that wants
   to make particles.

   CSimpleEmitter and derived classes handle adding themselves to the particle
   manager, tracking how many particles in the effect are active, and 
   rendering the particles.

   CSimpleEmitter has code to simulate and render particles in a generic fashion,
   but if you derive a class from it, you can override some of its behavior
   with virtuals like UpdateAlpha, UpdateScale, UpdateColor, etc..

   Example code:
		CSimpleEmitter *pEmitter = CSimpleEmitter::Create();
		
		CEffectMaterialHandle hMaterial = pEmitter->GetCEffectMaterial( "mymaterial" );
		
		for( int i=0; i < 100; i++ )
			pEmitter->AddParticle( hMaterial, RandomVector(0,10), 4 );

		pEmitter->Release();

2. Some older effects derive from C_BaseParticleEffect and implement an entity 
   and a particle system at the same time. This gets nasty and is not encouraged anymore.

*/


#ifndef PARTICLEMGR_H
#define PARTICLEMGR_H


#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "vector.h"
#include "vmatrix.h"
#include "Mathlib.h"
#include "iclientrenderable.h"
#include "clientleafsystem.h"
#include "tier0/fasttimer.h"
#include "utllinkedlist.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class IParticleEffect;
struct Particle;
class ParticleDraw;
class CMeshBuilder;
class CMemoryPool;
class CEffectMaterial;


#define INVALID_MATERIAL_HANDLE	NULL


// Various stats.
extern int			g_nParticlesDrawn;
extern CCycleCount	g_ParticleTimer;


//-----------------------------------------------------------------------------
// The basic particle description; all particles need to inherit from this.
//-----------------------------------------------------------------------------

struct Particle
{
	Particle *m_pPrev, *m_pNext;

	// If m_Pos isn't used to store the world position, then implement IParticleEffect::GetParticlePosition()
	Vector m_Pos;			// Position of the particle in world space
};


//-----------------------------------------------------------------------------
// This is the CParticleMgr's reference to a material in the material system.
// Particles are sorted by material.
//-----------------------------------------------------------------------------

typedef IMaterial* PMaterialHandle;	// For b/w compatibility.

// Each effect stores a list of particles associated with each material. The list is 
// hashed on the IMaterial pointer.
class CEffectMaterial
{
public:
	CEffectMaterial();

public:
	IMaterial *m_pMaterial;
	Particle m_Particles;
	CEffectMaterial *m_pHashedNext;
};


//-----------------------------------------------------------------------------
// interface IParticleEffect:
//
// This is the interface that particles effects must implement. The effect is 
// responsible for starting itself and calling CParticleMgr::AddEffect, then it 
// will get the callbacks it needs to simulate and render the particles.
//-----------------------------------------------------------------------------

class IParticleEffect
{
// Overridables.
public:
	
	virtual			~IParticleEffect() {}
	
	// Called at the beginning of a frame to precalculate data for rendering 
	// the particles. If you manage your own list of particles and want to 
	// simulate them all at once, you can do that here and just render them in 
	// the SimulateAndRender call.
	virtual void	Update( float fTimeDelta ) {}
	
	// Called once for the entire effect before the batch of SimulateAndRender() calls.
	// For particle systems using FLAGS_CAMERASPACE (the default), effectMatrix transforms the particles from
	// world space into camera space. You can change this matrix if you want your particles relative to something
	// else like an attachment's space.
	virtual void	StartRender( VMatrix &effectMatrix ) {}

	// Simulate the particle and render it.
	// Return false if you want to remove the particle.
	// Set sortKey to the distance of the particle and they will be incrementally sorted each frame.
	virtual bool	SimulateAndRender(Particle *pParticle, ParticleDraw* pParticleDraw, float &sortKey)=0;

	// Implementing this is optional. It is called when an effect is removed. It is useful if
	// you hold onto pointers to the particles you created (so when this is called, you should
	// clean up your data so you don't reference the particles again).
	// NOTE: after calling this, the particle manager won't touch the IParticleEffect
	// or its associated CParticleEffectBinding anymore.
	virtual void	NotifyRemove() {}

	// This method notifies the effect a particle is about to be deallocated.
	// Implementations should *not* actually deallocate it.
	// NOTE: The particle effect's GetNumActiveParticles is updated BEFORE this is called
	//       so if GetNumActiveParticles returns 0, then you know this is the last particle
	//       in the system being removed.
	virtual void	NotifyDestroyParticle( Particle* pParticle ) {}

	// Fill in the origin used to sort this entity.
	virtual void	GetSortOrigin( Vector &vSortOrigin ) = 0;

	// Fill in the origin used to sort this entity.
	virtual void	GetParticlePosition( Particle *pParticle, Vector& worldpos ) { worldpos = pParticle->m_Pos; }
};


// In order to create a particle effect, you must have one of these around and
// implement IParticleEffect. Pass them both into CParticleMgr::AddEffect and you
// are good to go.
class CParticleEffectBinding : public CDefaultClientRenderable
{
friend class CParticleMgr;

public:
					CParticleEffectBinding();
					~CParticleEffectBinding();


// Helper functions to setup, add particles, etc..
public:

	// Use this to specify materials when adding particles. 
	// Returns the index of the material it found or added.
	// Returns INVALID_MATERIAL_HANDLE if it couldn't find or add a material.
	IMaterial*		FindOrAddMaterial( const char *CEffectMaterialName );

	// Is the handle valid?
	bool			IsMaterialHandleValid( IMaterial *CEffectMaterial ) const;

	// Allocate particles. The Particle manager will automagically
	// deallocate them when the IParticleEffect SimulateAndRender() method 
	// returns false. The first argument is the size of the particle
	// structure in bytes
	Particle*		AddParticle( int sizeInBytes, IMaterial *CEffectMaterial );

	// Allows you to dynamically change the particle material
	// NOTE: Do *NOT* call this during SimulateAndRender!
	void			SetParticleMaterial( Particle* pParticle, IMaterial *CEffectMaterial );

	// This is an optional call you can make if you want to manually manage the effect's
	// bounding box. If your particle system moves around through the world, this may be
	// necessary because if you don't do it, and it moves into where the player can see
	// it, the particle manager won't know it has become visible since the bounding box
	// only gets updated while rendering.
	//
	// After you make this call, the particle manager will no longer update the bounding
	// box automatically if bDisableAutoUpdate is true.
	void			SetBBox( const Vector &bbMin, const Vector &bbMax, bool bDisableAutoUpdate = true );

	// This expands the bbox to contain the specified point.
	void			EnlargeBBoxToContain( const Vector &pt );

	// The EZ particle singletons use this - they don't want to be added to all the leaves and drawn through the
	// leaf system - they are specifically told to draw each frame at a certain point.
	void			SetDrawThruLeafSystem( int bDraw )				{ SetFlag( FLAGS_DRAW_THRU_LEAF_SYSTEM, bDraw ); }

	// Call this to have the effect removed whenever it safe to do so.
	// This is a lot safer than calling CParticleMgr::RemoveEffect.
	int				GetRemoveFlag()									{ return GetFlag( FLAGS_REMOVE ); }
	void			SetRemoveFlag()									{ SetFlag( FLAGS_REMOVE, 1 ); }

	// Set this flag to tell the particle manager to simulate your particles even
	// if the particle system isn't visible. Tempents and fast effects can always use
	// this if they want since they want to simulate their particles until they go away.
	// This flag is ON by default.
	int				GetAlwaysSimulate()								{ return GetFlag( FLAGS_ALWAYSSIMULATE ); }
	void			SetAlwaysSimulate( int bAlwaysSimulate )		{ SetFlag( FLAGS_ALWAYSSIMULATE, bAlwaysSimulate ); }

	// Set if the effect was drawn the previous frame.
	// This can be used by particle effect classes
	// to decide whether or not they want to spawn
	// new particles - if they weren't drawn, then
	// they can 'freeze' the particle system to avoid
	// overhead.
	int				WasDrawnPrevFrame()								{ return GetFlag( FLAGS_DRAWN_PREVFRAME ); }
	void			SetWasDrawnPrevFrame( int bWasDrawnPrevFrame )	{ SetFlag( FLAGS_DRAWN_PREVFRAME, bWasDrawnPrevFrame ); }

	// When the effect is in camera space mode, then the transforms are setup such that
	// the particles are specified in camera space rather than world space. This makes it
	// faster to specify the particles - transform the center by CParticleMgr::GetModelView
	// then add to X and Y to build the quad.
	//
	// Effects that want to specify verts in world space should set this to false and
	// ignore CParticleMgr::GetModelView.
	//
	// Camera space mode is ON by default.
	int				IsEffectCameraSpace()							{ return GetFlag( FLAGS_CAMERASPACE ); }
	void			SetEffectCameraSpace( int bCameraSpace )		{ SetFlag( FLAGS_CAMERASPACE, bCameraSpace ); }

	// Get the current number of particles in the effect.
	int				GetNumActiveParticles();


private:

	// Change flags..
	void			SetFlag( int flag, int bOn )	{ if( bOn ) m_Flags |= flag; else m_Flags &= ~flag; }
	int				GetFlag( int flag )				{ return m_Flags & flag; }

	void			Init( CParticleMgr *pMgr, IParticleEffect *pSim );
	void			Term();

	// Get rid of the specified particle.
	void			RemoveParticle( Particle *pParticle );

	// Flushes the rendering batch if it's at the end of the buffer.
	void			TestFlushBatch( 
						bool bOnlySimulate, 
						IMesh *pMesh, 
						CMeshBuilder &builder, 
						int &nParticlesInCurrentBatch );

	void			StartDrawMaterialParticles(
						bool bOnlySimulate,
						CEffectMaterial *CEffectMaterial,
						float flTimeDelta,
						IMesh* &pMesh,
						CMeshBuilder &builder,
						ParticleDraw &particleDraw,
						bool bWireframe );

	int				DrawMaterialParticles( 
						bool bBucketSort,
						bool bOnlySimulate, 
						CEffectMaterial *CEffectMaterial, 
						float flTimeDelta,
						Vector &bbMin,
						Vector &bbMax,
						bool &bboxSet,
						bool bWireframe
						 );

	void			RenderStart( VMatrix &mTempModel, VMatrix &mTempView );
	void			RenderEnd( VMatrix &mModel, VMatrix &mView );

	void			BBoxCalcStart( bool bBucketSort, Vector &bbMin, Vector &bbMax );
	void			BBoxCalcEnd( bool bBucketSort, bool bboxSet, Vector &bbMin, Vector &bbMax );
	
	void			DoBucketSort( 
		CEffectMaterial *CEffectMaterial, 
		float *zCoords, 
		int nZCoords,
		float minZ,
		float maxZ );

	int				GetRemovalInProgressFlag()					{ return GetFlag( FLAGS_REMOVALINPROGRESS ); }
	void			SetRemovalInProgressFlag()					{ SetFlag( FLAGS_REMOVALINPROGRESS, 1 ); }

	// BBox is recalculated before it's put into the tree for the first time.
	int				GetFirstUpdateFlag()						{ return GetFlag( FLAGS_FIRSTUPDATE ); }
	void			SetFirstUpdateFlag( int bFirstUpdate )		{ SetFlag( FLAGS_FIRSTUPDATE, bFirstUpdate ); }

	// If this is true, then the bbox is calculated from particle positions. This works
	// fine if you always simulate (SetAlwaysSimulateFlag) so the system can become visible
	// if it moves into the PVS. If you don't use this, then you should call SetBBox each
	// Update to tell the particle manager where your entity is.
	int				GetAutoUpdateBBox()							{ return GetFlag( FLAGS_AUTOUPDATEBBOX ); }
	void			SetAutoUpdateBBox( int bAutoUpdate )		{ SetFlag( FLAGS_AUTOUPDATEBBOX, bAutoUpdate ); }

	int				WasDrawn()									{ return GetFlag( FLAGS_DRAWN ); }
	void			SetDrawn( int bDrawn )						{ SetFlag( FLAGS_DRAWN, bDrawn ); }

	// Update m_Min/m_Max. Returns false and sets the bbox to (0,0,0) if there are no particles.
	bool			RecalculateBoundingBox();

	CEffectMaterial* GetEffectMaterial( IMaterial *CEffectMaterial );


// IClientRenderable overrides.
public:		

	virtual const Vector&			GetRenderOrigin( void );
	virtual const QAngle&			GetRenderAngles( void );
	virtual void					GetRenderBounds( Vector& mins, Vector& maxs );
	virtual bool					ShouldCacheRenderInfo();
	virtual bool					ShouldDraw( void );
	virtual bool					IsTransparent( void );
	virtual int						DrawModel( int flags );


private:

	enum
	{
		FLAGS_REMOVE =				(1<<0),	// Set in SetRemoveFlag
		FLAGS_REMOVALINPROGRESS =	(1<<1), // Set while the effect is being removed to prevent
											// infinite recursion.
		FLAGS_FIRSTUPDATE =			(1<<2),	// This is set until the effect's bbox has been updated once.
		FLAGS_AUTOUPDATEBBOX =		(1<<3),	// Update bbox automatically? Cleared in SetBBox.
		FLAGS_ALWAYSSIMULATE =		(1<<4), // See SetAlwaysSimulate.
		FLAGS_DRAWN =				(1<<5),	// Set if the effect is drawn through the leaf system.
		FLAGS_DRAWN_PREVFRAME =		(1<<6),	// Set if the effect was drawn the previous frame.
											// This can be used by particle effect classes
											// to decide whether or not they want to spawn
											// new particles - if they weren't drawn, then
											// they can 'freeze' the particle system to avoid
											// overhead.
		FLAGS_CAMERASPACE =			(1<<7),	// See SetEffectCameraSpace.
		FLAGS_DRAW_THRU_LEAF_SYSTEM=(1<<8)	// This is the default - do the effect's visibility through the leaf system.
	};

	ClientRenderHandle_t			m_hRender; // link into the leaf system
	
	// Bounding box. Stored in WORLD space.
	Vector							m_Min;
	Vector							m_Max;
	
	// Number of active particles.
	unsigned short					m_nActiveParticles;

	// See CParticleMgr::m_FrameCode.
	unsigned short					m_FrameCode;

	// For CParticleMgr's list index.
	unsigned short					m_ListIndex;

	IParticleEffect					*m_pSim;
	CParticleMgr					*m_pParticleMgr;
	
	// Combination of the CParticleEffectBinding::FLAGS_ flags.
	int								m_Flags;

	// Materials this effect is using.
	enum { EFFECT_MATERIAL_HASH_SIZE = 8 };
	CEffectMaterial *m_EffectMaterialHash[EFFECT_MATERIAL_HASH_SIZE];
	
	// For faster iteration.
	CUtlLinkedList<CEffectMaterial*, unsigned short> m_Materials;
};



class CParticleMgr
{
friend class CParticleEffectBinding;

public:

	// Vertex shader registers 45 through 85 are used to hold point-sourced light information.
	enum 
	{
		MAX_POINTSOURCE_LIGHTS = 20
	};

	// These entries are always in the array of lights that get sent to the card, so
	// they can be used directly rather than calling AllocatePointSourceLight().
	enum 
	{
		POINTSOURCE_INDEX_BOTTOM=0,		// underlit particles
		POINTSOURCE_INDEX_TOP=2,
		POINTSOURCE_NUM_DEFAULT_ENTRIES=2
	};

	class CPointSourceLight
	{
	public:
		Vector		m_vPos;
		float		m_flIntensity;
	};


public:

					CParticleMgr();
	virtual			~CParticleMgr();

	// Call at init time to preallocate the bucket of particles.
	bool			Init(unsigned long nPreallocatedParticles, IMaterialSystem *CEffectMaterial);

	// Shutdown - free everything.
	void			Term();

	// Add and remove effects from the active list.
	// Note: once you call AddEffect, CParticleEffectBinding will automatically call
	//       RemoveEffect in its destructor.
	// Note: it's much safer to call CParticleEffectBinding::SetRemoveFlag instead of
	//       CParticleMgr::RemoveEffect.
	bool			AddEffect( CParticleEffectBinding *pEffect, IParticleEffect *pSim );
	void			RemoveEffect( CParticleEffectBinding *pEffect );

	// Called at level shutdown to free all the lingering particle effects (usually
	// CParticleEffect-derived effects that can linger with noone holding onto them).
	void			RemoveAllEffects();

	// This should be called at the start of the frame.
	void			IncrementFrameCode();
	
	// This updates all the particle effects and inserts them into the leaves.
	void			Update( float fTimeDelta );

	// This is called after rendering all the particle systems to simulate effects 
	// that weren't drawn.
	void			SimulateUndrawnEffects();

	// Returns the modelview matrix
	VMatrix&		GetModelView();

	// Allocate a new point-sourced light. Returns the index you need to specify to the vertex
	// shader to index this light. If the list is overflowed, then POINTSOURCE_INDEX_BOTTOM is returned.
	int				AllocatePointSourceLight( const Vector &vPos, float flIntensity );

	Particle		*AllocParticle( int size );
	void			FreeParticle( Particle * );

	IMaterial*		GetPMaterial( const char *CEffectMaterialName );

private:
	// Call Update() on all the effects.
	void			UpdateAllEffects( float flTimeDelta );

	// Initialize the array of point-sourced lights.
	void			InitializePointSourceLights();


public:

	CPointSourceLight const *		GetPointSourceLights() const;
	int								GetNumPointSourceLights() const;


private:

	// Frame code, used to prevent CParticleEffects from simulating multiple times per frame.
	// Their DrawModel can be called multiple times per frame because of water reflections,
	// but we only want to simulate the particles once.
	unsigned short					m_FrameCode;

	bool							m_bUpdatingEffects;

	// These are filled in as the effect Update() calls are made.
	CPointSourceLight				m_PointSourceLights[MAX_POINTSOURCE_LIGHTS];
	int								m_nPointSourceLights;

	// All the active effects.
	CUtlLinkedList<CParticleEffectBinding*, unsigned short>		m_Effects;

	IMaterialSystem					*m_pMaterialSystem;

	// Store the concatenated modelview matrix
	VMatrix							m_mModelView;
	
	// This is a big array that is used to allocate and index the particles.
	CMemoryPool						*m_pParticleBucket;
};



// Helper functions to abstract out the particle testbed app.
float	Helper_GetTime();
float	Helper_GetFrameTime();
float	Helper_RandomFloat( float minVal, float maxVal );
int		Helper_RandomInt( int minVal, int maxVal );



// ------------------------------------------------------------------------ //
// CParticleMgr inlines
// ------------------------------------------------------------------------ //

inline VMatrix& CParticleMgr::GetModelView()
{
	return m_mModelView;
}

inline CParticleMgr::CPointSourceLight const * CParticleMgr::GetPointSourceLights() const
{
	return m_PointSourceLights;
}

inline int CParticleMgr::GetNumPointSourceLights() const
{
	return m_nPointSourceLights;
}


// ------------------------------------------------------------------------ //
// GLOBALS
// ------------------------------------------------------------------------ //

extern CParticleMgr g_ParticleMgr;




//-----------------------------------------------------------------------------
// StandardParticle_t; this is just one type of particle
// effects may implement their own particle data structures
//-----------------------------------------------------------------------------

struct StandardParticle_t : public Particle
{
	// Color and alpha values are 0 - 1
	void			SetColor(float r, float g, float b);
	void			SetAlpha(float a);

	Vector			m_Velocity;
	
	// How this is used is up to the effect's discretion. Some use it for how long it has been alive
	// and others use it to count down until the particle disappears.
	float			m_Lifetime;

	unsigned char	m_EffectData;	// Data specific to the IParticleEffect. This can be used to distinguish between
									// different types of particles the effect is simulating.
	unsigned short	m_EffectDataWord;

	unsigned char	m_Color[4];		// RGBA - not all effects need to use this.
};


// ------------------------------------------------------------------------ //
// Transform a particle.
// ------------------------------------------------------------------------ //

inline void TransformParticle(const VMatrix &vMat, const Vector &vIn, Vector &vOut)
{
	//vOut = vMat.VMul4x3(vIn);
	vOut.x = vMat.m[0][0]*vIn.x + vMat.m[0][1]*vIn.y + vMat.m[0][2]*vIn.z + vMat.m[0][3];
	vOut.y = vMat.m[1][0]*vIn.x + vMat.m[1][1]*vIn.y + vMat.m[1][2]*vIn.z + vMat.m[1][3];
	vOut.z = vMat.m[2][0]*vIn.x + vMat.m[2][1]*vIn.y + vMat.m[2][2]*vIn.z + vMat.m[2][3];
}


// ------------------------------------------------------------------------ //
// CEffectMaterial inlines
// ------------------------------------------------------------------------ //

inline void StandardParticle_t::SetColor(float r, float g, float b)
{
	m_Color[0] = (unsigned char)(r * 255.9f);
	m_Color[1] = (unsigned char)(g * 255.9f);
	m_Color[2] = (unsigned char)(b * 255.9f);
}

inline void StandardParticle_t::SetAlpha(float a)
{
	m_Color[3] = (unsigned char)(a * 255.9f);
}



#endif


