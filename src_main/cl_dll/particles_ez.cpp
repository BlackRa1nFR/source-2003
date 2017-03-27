//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "particles_ez.h"
#include "IGameSystem.h"


// Singletons for each type of particle system.
static CSmartPtr<CSimpleEmitter> g_pSimpleSingleton;
static CSmartPtr<CEmberEffect> g_pEmberSingleton;
static CSmartPtr<CFireSmokeEffect> g_pFireSmokeSingleton;
static CSmartPtr<CFireParticle> g_pFireSingleton;
static CSmartPtr<CLitSmokeEmitter> g_pLitSmokeSingleton;


class CEZParticleInit : public CAutoGameSystem
{
public:
	template< class T >
	CSmartPtr<T> InitSingleton( CSmartPtr<T> pEmitter )
	{
		if ( !pEmitter )
		{
			Error( "InitSingleton: pEmitter is NULL" );
		}
	
		pEmitter->GetBinding().SetDrawThruLeafSystem( false ); // Draw in DrawSingletons instead.
		pEmitter->SetSortOrigin( Vector( 0, 0, 0 ) );
		return pEmitter;
	}

	virtual void LevelInitPreEntity()
	{
		g_pSimpleSingleton = InitSingleton( CSimpleEmitter::Create( "Simple Particle Singleton" ) );
		g_pEmberSingleton = InitSingleton( CEmberEffect::Create( "Ember Particle Singleton" ) );
		g_pFireSmokeSingleton = InitSingleton( CFireSmokeEffect::Create( "Fire Smoke Particle Singleton" ) );
		g_pFireSingleton = InitSingleton( CFireParticle::Create( "Fire Particle Singleton" ) );
		g_pLitSmokeSingleton = InitSingleton( CLitSmokeEmitter::Create( "Lit Smoke Particle Singleton" ) );
	}


	virtual void LevelShutdownPreEntity()
	{
		g_pSimpleSingleton = NULL;
		g_pEmberSingleton = NULL;
		g_pFireSmokeSingleton = NULL;
		g_pFireSingleton = NULL;
		g_pLitSmokeSingleton = NULL;
	}
};

static CEZParticleInit g_EZParticleInit;


template<class T>
inline void CopyParticle( const T *pSrc, T *pDest )
{
	if ( pDest )
	{
		// Copy the particle, but don't screw up the linked list it's in.
		Particle *pPrev = pDest->m_pPrev;
		Particle *pNext = pDest->m_pNext;
		
		*pDest = *pSrc;
		
		pDest->m_pPrev = pPrev;
		pDest->m_pNext = pNext;
	}
}
		


void AddSimpleParticle( const SimpleParticle *pParticle, PMaterialHandle hMaterial )
{
	if ( g_pSimpleSingleton.IsValid() )
	{
		SimpleParticle *pNew = g_pSimpleSingleton->AddSimpleParticle( hMaterial, pParticle->m_Pos );
		CopyParticle( pParticle, pNew );
	}
}


void AddEmberParticle( const SimpleParticle *pParticle, PMaterialHandle hMaterial )
{
	if ( g_pEmberSingleton.IsValid() )
	{
		SimpleParticle *pNew = g_pEmberSingleton->AddSimpleParticle( hMaterial, pParticle->m_Pos );
		CopyParticle( pParticle, pNew );
	}
}


void AddFireSmokeParticle( const SimpleParticle *pParticle, PMaterialHandle hMaterial )
{
	if ( g_pFireSmokeSingleton.IsValid() )
	{
		SimpleParticle *pNew = g_pFireSmokeSingleton->AddSimpleParticle( hMaterial, pParticle->m_Pos );
		CopyParticle( pParticle, pNew );
	}
}


void AddFireParticle( const SimpleParticle *pParticle, PMaterialHandle hMaterial )
{
	if ( g_pFireSingleton.IsValid() )
	{
		SimpleParticle *pNew = g_pFireSingleton->AddSimpleParticle( hMaterial, pParticle->m_Pos );
		CopyParticle( pParticle, pNew );
	}
}


void AddLitSmokeParticle( const CLitSmokeEmitter::LitSmokeParticle *pParticle, PMaterialHandle hMaterial )
{
	if ( g_pLitSmokeSingleton.IsValid() )
	{
		CLitSmokeEmitter::LitSmokeParticle *pNew = (CLitSmokeEmitter::LitSmokeParticle*)g_pLitSmokeSingleton->AddParticle( 
			sizeof( CLitSmokeEmitter::LitSmokeParticle ),
			hMaterial, 
			pParticle->m_Pos );

		CopyParticle( pParticle, pNew );
	}
}


void DrawParticleSingletons()
{
	if ( g_pSimpleSingleton.IsValid() )
		g_pSimpleSingleton->GetBinding().DrawModel( 1 );

	if ( g_pEmberSingleton.IsValid() )
		g_pEmberSingleton->GetBinding().DrawModel( 1 );

	if ( g_pFireSmokeSingleton.IsValid() )
		g_pFireSmokeSingleton->GetBinding().DrawModel( 1 );
	
	if ( g_pFireSingleton.IsValid() )
		g_pFireSingleton->GetBinding().DrawModel( 1 );

	if ( g_pLitSmokeSingleton.IsValid() )
		g_pLitSmokeSingleton->GetBinding().DrawModel( 1 );
}



