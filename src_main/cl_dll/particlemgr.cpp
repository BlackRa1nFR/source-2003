//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Contains some client side classes related to particle emission
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "particlemgr.h"
#include "particledraw.h"
#include "materialsystem/imesh.h"
#include "mempool.h"
#include "IClientMode.h"
#include "view_scene.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int			g_nParticlesDrawn;
CCycleCount	g_ParticleTimer;

static ConVar r_DrawParticles("r_DrawParticles", "1", 0, "Enable/disable particle rendering");
static ConVar particle_simulateoverflow( "particle_simulateoverflow", "0", 0, "Used for stress-testing particle systems. Randomly denies creation of particles." );


#define NUM_PARTICLES_PER_BATCH 200
#define NUM_VERTS_PER_BATCH		(NUM_PARTICLES_PER_BATCH * 4)

#define BUCKET_SORT_EVERY_N		8			// It does a bucket sort for each material approximately every N times.
#define MAX_TOTAL_PARTICLES		(16*1024)	// Max particles in the bucket sort.


//-----------------------------------------------------------------------------
//
// Particle manager implementation
//
//-----------------------------------------------------------------------------

#define PARTICLE_SIZE	128

CParticleMgr g_ParticleMgr;
bool g_bParticleMgrConstructed = false; // makes sure it isn't used from constructors.




//-----------------------------------------------------------------------------
// List functions.
//-----------------------------------------------------------------------------

inline void UnlinkParticle( Particle *pParticle )
{
	pParticle->m_pPrev->m_pNext = pParticle->m_pNext;
	pParticle->m_pNext->m_pPrev = pParticle->m_pPrev;
}

inline void InsertParticleBefore( Particle *pInsert, Particle *pNext )
{
	// link pCur before pPrev
	pInsert->m_pNext = pNext;
	pInsert->m_pPrev = pNext->m_pPrev;
	pInsert->m_pNext->m_pPrev = pInsert->m_pPrev->m_pNext = pInsert;
}

inline void InsertParticleAfter( Particle *pInsert, Particle *pPrev )
{
	pInsert->m_pPrev = pPrev;
	pInsert->m_pNext = pPrev->m_pNext;

	pInsert->m_pNext->m_pPrev = pInsert->m_pPrev->m_pNext = pInsert;
}

inline void SwapParticles( Particle *pPrev, Particle *pCur )
{
	// unlink pCur
	UnlinkParticle( pCur );
	InsertParticleBefore( pCur, pPrev );
}


//-----------------------------------------------------------------------------
// CEffectMaterial.
//-----------------------------------------------------------------------------

CEffectMaterial::CEffectMaterial()
{
	m_Particles.m_pNext = m_Particles.m_pPrev = &m_Particles;
}

					
//-----------------------------------------------------------------------------
// CParticleEffectBinding.
//-----------------------------------------------------------------------------

CParticleEffectBinding::CParticleEffectBinding()
{
	m_pParticleMgr = NULL;
	m_pSim = NULL;
	
	m_Flags = 0;
	SetAutoUpdateBBox( true );
	SetFirstUpdateFlag( true );
	SetAlwaysSimulate( true );
	SetEffectCameraSpace( true );
	SetDrawThruLeafSystem( true );

	// default bbox
	m_Min.Init( -50, -50, -50 );
	m_Max.Init( 50, 50, 50 );

	m_nActiveParticles = 0;

	m_hRender = INVALID_CLIENT_RENDER_HANDLE;
	m_FrameCode = 0;
	m_ListIndex = 0xFFFF; 

	memset( m_EffectMaterialHash, 0, sizeof( m_EffectMaterialHash ) );
}


CParticleEffectBinding::~CParticleEffectBinding()
{
	if( m_pParticleMgr )
		m_pParticleMgr->RemoveEffect( this );

	Term();
}


const Vector& CParticleEffectBinding::GetRenderOrigin( void )
{
	static Vector vSortOrigin;
	m_pSim->GetSortOrigin( vSortOrigin );
	return vSortOrigin;
}


const QAngle& CParticleEffectBinding::GetRenderAngles( void )
{
	return vec3_angle;
}


void CParticleEffectBinding::GetRenderBounds( Vector& mins, Vector& maxs )
{
	Vector vSortOrigin;
	m_pSim->GetSortOrigin( vSortOrigin );

	// Convert to local space (around the sort origin).
	mins = m_Min - vSortOrigin;
	maxs = m_Max - vSortOrigin;
}

bool CParticleEffectBinding::ShouldCacheRenderInfo()
{
	return true;
}

bool CParticleEffectBinding::ShouldDraw( void )
{
	return GetFlag( FLAGS_DRAW_THRU_LEAF_SYSTEM ) != 0;
}


bool CParticleEffectBinding::IsTransparent( void )
{
	return true;
}


inline void CParticleEffectBinding::StartDrawMaterialParticles(
	bool bOnlySimulate,
	CEffectMaterial *pMaterial,
	float flTimeDelta,
	IMesh* &pMesh,
	CMeshBuilder &builder,
	ParticleDraw &particleDraw,
	bool bWireframe )
{
	// Setup the ParticleDraw and bind the material.
	if( bOnlySimulate )
	{
		particleDraw.Init( NULL, pMaterial->m_pMaterial, flTimeDelta );
	}
	else
	{
		if( bWireframe )
		{
			IMaterial *pMaterial = m_pParticleMgr->m_pMaterialSystem->FindMaterial( "debug/debugparticlewireframe", NULL );
			m_pParticleMgr->m_pMaterialSystem->Bind( pMaterial, NULL );
		}
		else
		{
			m_pParticleMgr->m_pMaterialSystem->Bind( pMaterial->m_pMaterial, m_pParticleMgr );
		}

		pMesh = m_pParticleMgr->m_pMaterialSystem->GetDynamicMesh( true );

		builder.Begin( pMesh, MATERIAL_QUADS, NUM_VERTS_PER_BATCH );
		particleDraw.Init( &builder, pMaterial->m_pMaterial, flTimeDelta );
	}
}


void CParticleEffectBinding::BBoxCalcStart( bool bBucketSort, Vector &bbMin, Vector &bbMax )
{
	if( bBucketSort )
	{
		if( GetAutoUpdateBBox() )
		{
			bbMin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
			bbMax.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
		}
	}
	else
	{
		// If we're not doing the full bbox update, then it just chooses some small
		// percentage of the particles and pushes out our current bbox.
		bbMin = m_Min;
		bbMax = m_Max;
	}
}


void CParticleEffectBinding::BBoxCalcEnd( bool bBucketSort, bool bboxSet, Vector &bbMin, Vector &bbMax )
{
	if( bBucketSort )
	{
		if( GetAutoUpdateBBox() )
		{
			if( bboxSet )
			{
				m_Min = bbMin;
				m_Max = bbMax;
			}
			else
			{
				m_Min.Init( 0, 0, 0 );
				m_Max.Init( 0, 0, 0 );
			}
		}
	}
	else
	{
		// Take whatever our bbox was + pushing out from other particles.
		m_Min = bbMin;
		m_Max = bbMax;
	}
}


int CParticleEffectBinding::DrawModel( int flags )
{
	VPROF_BUDGET( "CParticleEffectBinding::DrawModel", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
#ifndef PARTICLEPROTOTYPE_APP
	if ( !r_DrawParticles.GetInt() )
		return 0;
#endif

	// Flags here is 0 if the system is only being simulated (ie: it shouldn't
	// actually try to render anything).
	bool bOnlySimulate = !flags;


	// If we're in commander mode and it's trying to draw the effect,
	// exit out. If the effect has FLAGS_ALWAYSSIMULATE set, then it'll come back
	// in here and simulate at the end of the frame.
	if( !g_pClientMode->ShouldDrawParticles() )
		if( !bOnlySimulate )
			return 0;


	// Track how much time we spend in here.
	CTimeAdder adder( &g_ParticleTimer );


	SetDrawn( true );
	
	
	// Don't do anything if there are no particles.
	if( !m_nActiveParticles )
		return 1;

	
	// Reset the transformation matrix to identity.
	VMatrix mTempModel, mTempView;
	if( !bOnlySimulate )
		RenderStart( mTempModel, mTempView );


	// Setup to redo our bbox?
	bool bBucketSort = random->RandomInt( 0, BUCKET_SORT_EVERY_N ) == 0;

	Vector bbMin(0,0,0), bbMax(0,0,0);
	BBoxCalcStart( bBucketSort, bbMin, bbMax );


	// Set frametime to zero if we've already rendered this frame.
	float flFrameTime = 0;
	if ( m_FrameCode != m_pParticleMgr->m_FrameCode )
	{
		m_FrameCode = m_pParticleMgr->m_FrameCode;
		flFrameTime = Helper_GetFrameTime();
	}


	// For each material, render...
	// This does an incremental bubble sort. It only does one pass every frame, and it will shuffle 
	// unsorted particles one step towards where they should be.
	bool bboxSet = false;
	bool bWireframe = false;
	FOR_EACH_LL( m_Materials, iMaterial )
	{
		CEffectMaterial *pMaterial = m_Materials[iMaterial];
		
		if( !bOnlySimulate )
		{
			if( pMaterial->m_pMaterial && pMaterial->m_pMaterial->NeedsFrameBufferTexture() )
			{
				UpdateRefractTexture();
			}
		}
		
		DrawMaterialParticles( 
			bBucketSort,
			bOnlySimulate, 
			pMaterial, 
			flFrameTime,
			bbMin,
			bbMax,
			bboxSet,
			bWireframe );
	}

	extern ConVar mat_wireframe;

	if( mat_wireframe.GetBool() )
	{
		bWireframe = true;
		FOR_EACH_LL( m_Materials, iDrawMaterial )
		{
			CEffectMaterial *pMaterial = m_Materials[iDrawMaterial];
			
			DrawMaterialParticles( 
				bBucketSort,
				bOnlySimulate, 
				pMaterial, 
				flFrameTime,
				bbMin,
				bbMax,
				bboxSet,
				bWireframe );
		}
	}
	
	BBoxCalcEnd( bBucketSort, bboxSet, bbMin, bbMax );
	
	
	if( !bOnlySimulate )
		RenderEnd( mTempModel, mTempView );
	
	return 1;
}


IMaterial* CParticleEffectBinding::FindOrAddMaterial( const char *pMaterialName )
{
	if ( !m_pParticleMgr )
	{
		return NULL;
	}

	return m_pParticleMgr->GetPMaterial( pMaterialName );
}


bool CParticleEffectBinding::IsMaterialHandleValid( IMaterial *pMaterial ) const
{
	return pMaterial != NULL;
}


Particle* CParticleEffectBinding::AddParticle( int sizeInBytes, IMaterial *pMaterial )
{
	// Get the bucket for this material's particles.
	if ( !pMaterial )
	{
		Assert( false );
		return NULL;
	}

	// We've currently clamped the particle size to PARTICLE_SIZE,
	// we may need to change this algorithm if we get particles with
	// widely varying size
	Assert( sizeInBytes + sizeof( Particle ) <= PARTICLE_SIZE );

	// This is for testing - simulate it running out of memory.
	if ( particle_simulateoverflow.GetInt() )
	{
		if ( rand() % 10 <= 6 )
			return NULL;
	}
	
	// Allocate the puppy. We are actually allocating space for the
	// internals + the actual data
	Particle* pParticle = m_pParticleMgr->AllocParticle( PARTICLE_SIZE );
	if( !pParticle )
		return NULL;

	// Link it in
	CEffectMaterial *pEffectMat = GetEffectMaterial( pMaterial );
	InsertParticleAfter( pParticle, &pEffectMat->m_Particles );

	++m_nActiveParticles;
	return pParticle;
}

void CParticleEffectBinding::SetParticleMaterial( Particle* pParticle, IMaterial *pMaterial )
{
	// Make sure the material exists...
	if( !pMaterial )
		return;

	// Link it in.
	UnlinkParticle( pParticle );

	CEffectMaterial *pEffectMat = GetEffectMaterial( pMaterial );
	InsertParticleAfter( pParticle, &pEffectMat->m_Particles );
}

void CParticleEffectBinding::SetBBox( const Vector &bbMin, const Vector &bbMax, bool bDisableAutoUpdate )
{
	m_Min = bbMin;
	m_Max = bbMax;
	
	if ( bDisableAutoUpdate )
		SetAutoUpdateBBox( false );
}


void CParticleEffectBinding::EnlargeBBoxToContain( const Vector &pt )
{
	VectorMin( pt, m_Min, m_Min );
	VectorMax( pt, m_Max, m_Max );
}


int CParticleEffectBinding::GetNumActiveParticles()
{
	return m_nActiveParticles;
}


int CParticleEffectBinding::DrawMaterialParticles( 
	bool bBucketSort,
	bool bOnlySimulate, 
	CEffectMaterial *pMaterial, 
	float flTimeDelta,
	Vector &bbMin,
	Vector &bbMax,
	bool &bboxSet,
	bool bWireframe
	 )
{
	// Setup everything.
	CMeshBuilder builder;
	ParticleDraw particleDraw;
	IMesh *pMesh = NULL;
	StartDrawMaterialParticles( bOnlySimulate, pMaterial, flTimeDelta, pMesh, builder, particleDraw, bWireframe );

	float zCoords[MAX_TOTAL_PARTICLES];
	int nZCoords = 0;
	float minZ = 1e24, maxZ = -1e24;
	int nParticlesInCurrentBatch = 0;
	int nParticlesDrawn = 0;


	int iParticle = 0;
	Particle *pNext;
	float curZ, prevZ=0;
	for( Particle *pCur=pMaterial->m_Particles.m_pNext; 
		pCur != &pMaterial->m_Particles; 
		pCur=pNext )
	{
		pNext = pCur->m_pNext;
	
		// Draw this guy.
		Particle *pParticle = pCur;
		if( m_pSim->SimulateAndRender(pParticle, &particleDraw, curZ) )
		{
			++g_nParticlesDrawn;

			// Either update the bucket sort info or update the incremental sort.
			if( bBucketSort )
			{
				if( curZ < minZ ) minZ = curZ;
				if( curZ > maxZ ) maxZ = curZ;
				if( nZCoords < MAX_TOTAL_PARTICLES )
				{
					zCoords[nZCoords] = curZ;
					++nZCoords;
				}

				// Update bounding box 
				// FIXME: Move simulation to Update()?
				Vector worldpos;
				m_pSim->GetParticlePosition( pParticle, worldpos );
				VectorMin( bbMin, worldpos, bbMin );
				VectorMax( bbMax, worldpos, bbMax );
				bboxSet = true;
			}
			else
			{
				// Since we're not doing the full bounding box update, just test every eighth
				// particle and make the bbox bigger if necessary.
				if( (iParticle++ & 7) == 0 )
				{
					Vector worldpos;
					m_pSim->GetParticlePosition( pParticle, worldpos );
					VectorMin( bbMin, worldpos, bbMin );
					VectorMax( bbMax, worldpos, bbMax );
				}

				// Swap with the previous particle (incremental sort)?
				if( pCur != pMaterial->m_Particles.m_pNext && prevZ > curZ )
				{
					SwapParticles( pCur->m_pPrev, pCur );
				}
				else
				{
					prevZ = curZ;
				}
			}
			
			++nParticlesDrawn;
		}
		else
		{
			// They want to remove the particle.
			RemoveParticle(pCur);
		}

		TestFlushBatch( bOnlySimulate, pMesh, builder, nParticlesInCurrentBatch );
	}

	if( bBucketSort )
	{
		DoBucketSort( pMaterial, zCoords, nZCoords, minZ, maxZ );
	}

	// Flush out any remaining particles.
	if( !bOnlySimulate )
		builder.End( false, true );
	
	return nParticlesDrawn;
}


void CParticleEffectBinding::RenderStart( VMatrix &tempModel, VMatrix &tempView )
{
	if( IsEffectCameraSpace() )
	{
		// Store matrices off so we can restore them in RenderEnd().
		m_pParticleMgr->m_pMaterialSystem->GetMatrix(MATERIAL_VIEW, (float*)tempView.m);
		m_pParticleMgr->m_pMaterialSystem->GetMatrix(MATERIAL_MODEL, (float*)tempModel.m);

		// We're gonna assume the model matrix was identity and blow it off
		// This means that the particle positions are all specified in world space
		// which makes bounding box computations faster. We use the transpose here
		// because our math libraries need matrices that are transpose of those
		// returned by the material system
		m_pParticleMgr->m_mModelView = tempView.Transpose();

		// The particle renderers want to do things in camera space
		m_pParticleMgr->m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
		m_pParticleMgr->m_pMaterialSystem->LoadIdentity();

		m_pParticleMgr->m_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
		m_pParticleMgr->m_pMaterialSystem->LoadIdentity();
	}
	else
	{
		m_pParticleMgr->m_mModelView.Identity();
	}

	// Let the particle effect do any per-frame setup/processing here
	m_pSim->StartRender( m_pParticleMgr->m_mModelView );
}


void CParticleEffectBinding::RenderEnd( VMatrix &tempModel, VMatrix &tempView )
{
	if( IsEffectCameraSpace() )
	{
		// Reset the model matrix.
		m_pParticleMgr->m_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
		m_pParticleMgr->m_pMaterialSystem->LoadMatrix( (float*)tempModel.m );

		// Reset the view matrix.
		m_pParticleMgr->m_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
		m_pParticleMgr->m_pMaterialSystem->LoadMatrix( (float*)tempView.m );
	}
}


void CParticleEffectBinding::DoBucketSort( CEffectMaterial *pMaterial, float *zCoords, int nZCoords, float minZ, float maxZ )
{
	// Do an O(N) bucket sort. This helps the sort when there are lots of particles.
	#define NUM_BUCKETS	32
	Particle buckets[NUM_BUCKETS];
	for( int iBucket=0; iBucket < NUM_BUCKETS; iBucket++ )
	{
		buckets[iBucket].m_pPrev = buckets[iBucket].m_pNext = &buckets[iBucket];
	}
	
	// Sort into buckets.
	int iCurParticle = 0;
	Particle *pNext, *pCur;
	for( pCur=pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pNext )
	{
		pNext = pCur->m_pNext;
		if( iCurParticle >= nZCoords )
			break;

		// Remove it..
		UnlinkParticle( pCur );

		// Add it to the appropriate bucket.
		float flPercent = (zCoords[iCurParticle] - minZ) / (maxZ - minZ);
		int iAddBucket = (int)( flPercent * (NUM_BUCKETS - 0.0001f) );
		iAddBucket = NUM_BUCKETS - iAddBucket - 1;
		Assert( iAddBucket >= 0 && iAddBucket < NUM_BUCKETS );

		InsertParticleAfter( pCur, &buckets[iAddBucket] );

		++iCurParticle;
	}

	// Put the buckets back into the main list.
	for( int iReAddBucket=0; iReAddBucket < NUM_BUCKETS; iReAddBucket++ )
	{
		Particle *pListHead = &buckets[iReAddBucket];
		for( pCur=pListHead->m_pNext; pCur != pListHead; pCur=pNext )
		{
			pNext = pCur->m_pNext;
			InsertParticleAfter( pCur, &pMaterial->m_Particles );
			--iCurParticle;
		}
	}

	Assert(iCurParticle==0);
}


void CParticleEffectBinding::Init( CParticleMgr *pMgr, IParticleEffect *pSim )
{
	// Must Term before reinitializing.
	Assert( !m_pSim && !m_pParticleMgr );

	m_pSim = pSim;
	m_pParticleMgr = pMgr;
}


void CParticleEffectBinding::Term()
{
	if( !m_pParticleMgr || !g_bParticleMgrConstructed )
		return;

	// Free materials.
	FOR_EACH_LL( m_Materials, iMaterial )
	{
		CEffectMaterial *pMaterial = m_Materials[iMaterial];

		// Remove all particles tied to this effect.
		Particle *pNext = NULL;
		for(Particle *pCur = pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pNext )
		{
			pNext = pCur->m_pNext;
			
			RemoveParticle( pCur );
		}
		
		pMaterial->m_pMaterial->DecrementReferenceCount();
		delete pMaterial;
	}	
	m_Materials.Purge();

	memset( m_EffectMaterialHash, 0, sizeof( m_EffectMaterialHash ) );
}


void CParticleEffectBinding::RemoveParticle( Particle *pParticle )
{
	UnlinkParticle( pParticle );
	
	// Important that this is updated BEFORE NotifyDestroyParticle is called.
	--m_nActiveParticles;
	Assert( m_nActiveParticles >= 0 );

	// Let the effect do any necessary cleanup
	m_pSim->NotifyDestroyParticle(pParticle);

	// Remove it from the list of particles and deallocate
	m_pParticleMgr->FreeParticle(pParticle);
}


void CParticleEffectBinding::TestFlushBatch( bool bOnlySimulate, IMesh *pMesh, CMeshBuilder &builder, int &nParticlesInCurrentBatch )
{
	++nParticlesInCurrentBatch;
	if( nParticlesInCurrentBatch >= NUM_PARTICLES_PER_BATCH )
	{
		if( !bOnlySimulate )
		{
			builder.End( false, true );
			builder.Begin( pMesh, MATERIAL_QUADS, NUM_VERTS_PER_BATCH );
		}

		nParticlesInCurrentBatch = 0;
	}
}


bool CParticleEffectBinding::RecalculateBoundingBox()
{
	if( m_nActiveParticles == 0 )
	{
		m_pSim->GetSortOrigin( m_Min );
		m_Max = m_Min;
		return false;
	}

	m_Min.Init(  1e28,  1e28,  1e28 );
	m_Max.Init( -1e28, -1e28, -1e28 );

	FOR_EACH_LL( m_Materials, iMaterial )
	{
		CEffectMaterial *pMaterial = m_Materials[iMaterial];
		
		for( Particle *pCur=pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pCur->m_pNext )
		{
			Vector vPos;
			m_pSim->GetParticlePosition( pCur, vPos );
			VectorMin( m_Min, vPos, m_Min );
			VectorMax( m_Max, vPos, m_Max );
			m_Max.Max( vPos );
		}
	}

	return true;
}


CEffectMaterial* CParticleEffectBinding::GetEffectMaterial( IMaterial *pMaterial )
{
	// Hash the IMaterial pointer.
	unsigned long index = (((unsigned long)pMaterial) >> 6) % EFFECT_MATERIAL_HASH_SIZE;
	for ( CEffectMaterial *pCur=m_EffectMaterialHash[index]; pCur; pCur = pCur->m_pHashedNext )
	{
		if ( pCur->m_pMaterial == pMaterial )
			return pCur;
	}

	CEffectMaterial *pEffectMat = new CEffectMaterial;
	pEffectMat->m_pMaterial = pMaterial;
	pEffectMat->m_pHashedNext = m_EffectMaterialHash[index];
	m_EffectMaterialHash[index] = pEffectMat;

	m_Materials.AddToTail( pEffectMat );
	return pEffectMat;
}


//-----------------------------------------------------------------------------
// CParticleMgr
//-----------------------------------------------------------------------------

CParticleMgr::CParticleMgr()
{
	m_pParticleBucket = NULL;
	m_bUpdatingEffects = false;

	g_bParticleMgrConstructed = true;
	InitializePointSourceLights();

	m_FrameCode = 1;
}

CParticleMgr::~CParticleMgr()
{
	Term();
	g_bParticleMgrConstructed = false;
}


//-----------------------------------------------------------------------------
// Initialization and shutdown
//-----------------------------------------------------------------------------
bool CParticleMgr::Init(unsigned long count, IMaterialSystem *pMaterials)
{
	Term();

	m_pMaterialSystem = pMaterials;

	m_pParticleBucket = new CMemoryPool( PARTICLE_SIZE, count, CMemoryPool::GROW_NONE );
	if ( !m_pParticleBucket )
		return false;

	return true;
}

void CParticleMgr::Term()
{
	// Free all the effects.
	int iNext;
	for ( int i=m_Effects.Head(); i != m_Effects.InvalidIndex(); i = iNext )
	{
		iNext = m_Effects.Next( i );
		m_Effects[i]->m_pSim->NotifyRemove();
	}
	m_Effects.Purge();

	if ( m_pMaterialSystem )
	{
		m_pMaterialSystem->UncacheUnusedMaterials();
	}
	m_pMaterialSystem = NULL;

	delete m_pParticleBucket;
	m_pParticleBucket = NULL;
}

Particle *CParticleMgr::AllocParticle( int size )
{
	if ( !m_pParticleBucket )
		return NULL;

	return (Particle *)m_pParticleBucket->Alloc( size );
}

void CParticleMgr::FreeParticle( Particle *pParticle )
{
	Assert( m_pParticleBucket );
	m_pParticleBucket->Free( pParticle );
}

//-----------------------------------------------------------------------------
// Adds and removes effects from our global list
//-----------------------------------------------------------------------------

bool CParticleMgr::AddEffect( CParticleEffectBinding *pEffect, IParticleEffect *pSim )
{
	// If you hit this, you're using something that adds itself to 
	// the CParticleMgr in its constructor, which is evil.
	Assert( g_bParticleMgrConstructed );

#ifdef _DEBUG
	FOR_EACH_LL( m_Effects, i )
	{
		if( m_Effects[i]->m_pSim == pSim )
		{
			Assert( !"CParticleMgr::AddEffect: added same effect twice" );
			return false;
		}
	}
#endif

	pEffect->Init( this, pSim );

	// Add it to the leaf system.
#if !defined( PARTICLEPROTOTYPE_APP )
	pEffect->m_hRender = ClientLeafSystem()->CreateRenderableHandle( pEffect );
#endif

	pEffect->m_ListIndex = m_Effects.AddToTail( pEffect );
	return true;
}


void CParticleMgr::RemoveEffect( CParticleEffectBinding *pEffect )
{
	// This prevents certain recursive situations where a NotifyRemove
	// call can wind up triggering another one, usually in an effect's
	// destructor.
	if( pEffect->GetRemovalInProgressFlag() )
		return;
	pEffect->SetRemovalInProgressFlag();

	// Don't call RemoveEffect while inside an IParticleEffect's Update() function.
	// Return false from the Update function instead.
	Assert( !m_bUpdatingEffects );

	// Take it out of the leaf system.
	if( pEffect->m_hRender != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->RemoveRenderable( pEffect->m_hRender );
	}

	int listIndex = pEffect->m_ListIndex;
	if ( pEffect->m_pSim )
	{
		pEffect->m_pSim->NotifyRemove();
	}
	m_Effects.Remove( listIndex );
}


void CParticleMgr::RemoveAllEffects()
{
	int iNext;
	for ( int i=m_Effects.Head(); i != m_Effects.InvalidIndex(); i = iNext )
	{
		iNext = m_Effects.Next( i );
		RemoveEffect( m_Effects[i] );
	}
}


void CParticleMgr::IncrementFrameCode()
{
	VPROF( "CParticleMgr::IncrementFrameCode()" );

	++m_FrameCode;
	if ( m_FrameCode == 0 )
	{
		// Reset all the CParticleEffectBindings..
		FOR_EACH_LL( m_Effects, i )
		{
			m_Effects[i]->m_FrameCode = 0;
		}

		m_FrameCode = 1;
	}
}


//-----------------------------------------------------------------------------
// Main rendering loop
//-----------------------------------------------------------------------------
void CParticleMgr::Update( float flTimeDelta )
{
	g_nParticlesDrawn = 0;
	g_ParticleTimer.Init();

	if(!m_pMaterialSystem)
	{
		Assert(false);
		return;
	}

	// Clear the list of pointsourced lights so it can be rebuilt.
	InitializePointSourceLights();

	// Update all the effects.
	UpdateAllEffects( flTimeDelta );
}


void CParticleMgr::SimulateUndrawnEffects()
{
	VPROF("CParticleMgr::SimulateUndrawnEffects");

	// Simulate all effects that weren't drawn (if they have their 'always simulate' flag set).
	FOR_EACH_LL( m_Effects, i )
	{
		CParticleEffectBinding *pEffect = m_Effects[i];
		
		// Tell the effect if it was drawn or not.
		pEffect->SetWasDrawnPrevFrame( pEffect->WasDrawn() );
		
		if( !pEffect->WasDrawn() && pEffect->GetAlwaysSimulate() )
		{
			pEffect->DrawModel( 0 );
		}
	}
}


int CParticleMgr::AllocatePointSourceLight( const Vector &vPos, float flIntensity )
{
	if( m_nPointSourceLights < 0 || m_nPointSourceLights >= MAX_POINTSOURCE_LIGHTS )
		return POINTSOURCE_INDEX_BOTTOM;

	m_PointSourceLights[m_nPointSourceLights].m_vPos = vPos;
	m_PointSourceLights[m_nPointSourceLights].m_flIntensity = flIntensity;
	++m_nPointSourceLights;

	return (m_nPointSourceLights - 1) * 2; // 2 registers per light
}


void CParticleMgr::UpdateAllEffects( float flTimeDelta )
{
	m_bUpdatingEffects = true;

	if( flTimeDelta > 0.1f )
		flTimeDelta = 0.1f;

	IClientLeafSystem *pLeafSystem = ClientLeafSystem();
	
	FOR_EACH_LL( m_Effects, iEffect )
	{
		CParticleEffectBinding *pEffect = m_Effects[iEffect];

		// Don't update this effect if it will be removed.
		if( !pEffect->GetRemoveFlag() )
		{
			// If this is a new effect, then update its bbox so it goes in the
			// right leaves (if it has particles).
			if( pEffect->GetFirstUpdateFlag() )
			{
				if( pEffect->RecalculateBoundingBox() )
					pEffect->SetFirstUpdateFlag( false );
			}

			// This flag will get set to true if the effect is drawn through the
			// leaf system.
			pEffect->SetDrawn( false );

#if !defined( PARTICLEPROTOTYPE_APP )			
			pLeafSystem->DetectChanges( pEffect->m_hRender, RENDER_GROUP_TRANSLUCENT_ENTITY );
#endif
			
			// Update the effect.
			pEffect->m_pSim->Update( flTimeDelta );
		}
	}

	m_bUpdatingEffects = false;

	// Remove any effects that were flagged to be removed.
	int iNext;
	for ( int i=m_Effects.Head(); i != m_Effects.InvalidIndex(); i=iNext )
	{
		iNext = m_Effects.Next( i );
		CParticleEffectBinding *pEffect = m_Effects[i];

		if( pEffect->GetRemoveFlag() )
		{
			RemoveEffect( pEffect );
		}
	}
}

void CParticleMgr::InitializePointSourceLights()
{
	// Fill in the default entries.
	static float flFarthest = 10000;

	m_PointSourceLights[POINTSOURCE_INDEX_BOTTOM/2].m_vPos.Init( 0, 0, -flFarthest );
	m_PointSourceLights[POINTSOURCE_INDEX_BOTTOM/2].m_flIntensity = flFarthest*flFarthest*4;

	m_PointSourceLights[POINTSOURCE_INDEX_TOP/2].m_vPos.Init( 0, 0, flFarthest );
	m_PointSourceLights[POINTSOURCE_INDEX_TOP/2].m_flIntensity = flFarthest*flFarthest*4;

	m_nPointSourceLights = POINTSOURCE_NUM_DEFAULT_ENTRIES;
}


IMaterial* CParticleMgr::GetPMaterial( const char *pMaterialName )
{
	if( !m_pMaterialSystem )
	{
		Assert(false);
		return NULL;
	}
	
	IMaterial *pIMaterial = m_pMaterialSystem->FindMaterial(pMaterialName, NULL);
	m_pMaterialSystem->Bind( pIMaterial, this );

	return pIMaterial;
}


// ------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------ //
float Helper_GetTime()
{
	#if defined( PARTICLEPROTOTYPE_APP )
		static bool bStarted = false;
		static CCycleCount startTimer;
		if( !bStarted )
		{
			bStarted = true;
			startTimer.Sample();
		}

		CCycleCount curCount;
		curCount.Sample();

		CCycleCount elapsed;
		CCycleCount::Sub( curCount, startTimer, elapsed );

		return (float)elapsed.GetSeconds();
	#else
		return gpGlobals->curtime;
	#endif
}


float Helper_RandomFloat( float minVal, float maxVal )
{
	#if defined( PARTICLEPROTOTYPE_APP )
		return Lerp( (float)rand() / RAND_MAX, minVal, maxVal );
	#else
		return random->RandomFloat( minVal, maxVal );
	#endif
}


int Helper_RandomInt( int minVal, int maxVal )
{
	#if defined( PARTICLEPROTOTYPE_APP )
		return minVal + (rand() * (maxVal - minVal)) / RAND_MAX;
	#else
		return random->RandomInt( minVal, maxVal );
	#endif
}


float Helper_GetFrameTime()
{
	#if defined( PARTICLEPROTOTYPE_APP )
		extern float g_ParticleAppFrameTime;
		return g_ParticleAppFrameTime;
	#else
		return gpGlobals->frametime;
	#endif
}
