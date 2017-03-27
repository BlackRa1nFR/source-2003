//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "particles_simple.h"
#include "c_baseentity.h"
#include "baseparticleentity.h"
#include "engine/IVDebugOverlay.h"

//==================================================
// SporeSmokeEffect
//==================================================

class SporeSmokeEffect : public CSimpleEmitter
{
public:
	SporeSmokeEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static SporeSmokeEffect* Create( const char *pDebugName );

	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual float UpdateAlpha( SimpleParticle *pParticle, float timeDelta );

private:
	SporeSmokeEffect( const SporeSmokeEffect & );
};


SporeSmokeEffect* SporeSmokeEffect::Create( const char *pDebugName )
{
	return new SporeSmokeEffect( pDebugName );
}

//==================================================
// C_SporeTrail
//==================================================

class C_SporeTrail : public C_BaseParticleEntity
{
public:
	DECLARE_CLASS( C_SporeTrail, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();
	
	C_SporeTrail( void );
	virtual	~C_SporeTrail( void );

public:
	void	SetEmit( bool bEmit );


// C_BaseEntity
public:
	virtual	void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void );
	virtual void	GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles );

// IPrototypeAppEffect
public:
	virtual void	Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs );

// IParticleEffect
public:
	virtual void	Update( float fTimeDelta );
	virtual bool	SimulateAndRender( Particle *pParticle, ParticleDraw *pDraw, float &sortKey );
	virtual void	StartRender( VMatrix &effectMatrix );

public:
	Vector	m_vecEndColor;

	float	m_flSpawnRate;
	float	m_flParticleLifetime;
	float	m_flStartSize;
	float	m_flEndSize;
	float	m_flSpawnRadius;

	Vector	m_vecVelocityOffset;

	bool	m_bEmit;

private:
	C_SporeTrail( const C_SporeTrail & );

	void			AddParticles( void );

	PMaterialHandle		m_hMaterial;
	TimedEvent			m_teParticleSpawn;
	//CSmartPtr<SporeSmokeEffect> m_pSmokeEffect;

	Vector			m_vecPos;
	Vector			m_vecLastPos;	// This is stored so we can spawn particles in between the previous and new position
									// to eliminate holes in the trail.

	VMatrix			m_mAttachmentMatrix;
	CParticleMgr		*m_pParticleMgr;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float SporeSmokeEffect::UpdateAlpha( SimpleParticle *pParticle, float timeDelta )
{
	//return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
	return (pParticle->m_uchStartAlpha/255.0f) + ( (float)(pParticle->m_uchEndAlpha/255.0f) - (float)(pParticle->m_uchStartAlpha/255.0f) ) * (pParticle->m_flLifetime / pParticle->m_flDieTime);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
void SporeSmokeEffect::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
}

//==================================================
// C_SporeTrail
//==================================================

IMPLEMENT_CLIENTCLASS_DT( C_SporeTrail, DT_SporeTrail, SporeTrail )
	RecvPropFloat(RECVINFO(m_flSpawnRate)),
	RecvPropVector(RECVINFO(m_vecEndColor)),
	RecvPropFloat(RECVINFO(m_flParticleLifetime)),
	RecvPropFloat(RECVINFO(m_flStartSize)),
	RecvPropFloat(RECVINFO(m_flEndSize)),
	RecvPropFloat(RECVINFO(m_flSpawnRadius)),
	RecvPropInt(RECVINFO(m_bEmit)),
END_RECV_TABLE()

C_SporeTrail::C_SporeTrail( void )
{
	m_pParticleMgr			= NULL;
	//m_pSmokeEffect			= SporeSmokeEffect::Create( "C_SporeTrail" );

	m_flSpawnRate			= 10;
	m_flParticleLifetime	= 5;
	m_flStartSize			= 35;
	m_flEndSize				= 55;
	m_flSpawnRadius			= 2;

	m_teParticleSpawn.Init( 5 );
	m_vecEndColor.Init();
	m_vecPos.Init();
	m_vecLastPos.Init();
	m_vecVelocityOffset.Init();

	m_bEmit = true;
}

C_SporeTrail::~C_SporeTrail()
{
	if( m_pParticleMgr )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEmit - 
//-----------------------------------------------------------------------------
void C_SporeTrail::SetEmit( bool bEmit )
{
	m_bEmit = bEmit;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SporeTrail::OnDataChanged( DataUpdateType_t updateType )
{
	C_BaseEntity::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start( &g_ParticleMgr, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SporeTrail::ShouldDraw( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SporeTrail::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	if( pParticleMgr->AddEffect( &m_ParticleEffect, this ) == false )
		return;

	m_hMaterial	= pParticleMgr->GetPMaterial( "particle/particle_noisesphere" );	
	m_pParticleMgr = pParticleMgr;
	m_teParticleSpawn.Init( 64 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SporeTrail::AddParticles( void )
{
	Vector	offset = RandomVector( -4.0f, 4.0f );

	//Make a new particle
	SimpleParticle *sParticle = (SimpleParticle *) m_ParticleEffect.AddParticle( sizeof(SimpleParticle), m_hMaterial );//m_pSmokeEffect->AddParticle( sizeof(SimpleParticle), m_hMaterial, GetAbsOrigin()+offset );

	if ( sParticle == NULL )
		return;

	sParticle->m_Pos			= offset;
	sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );
	sParticle->m_flRollDelta	= Helper_RandomFloat( -2.0f, 2.0f );

	sParticle->m_flLifetime		= 0.0f;
	sParticle->m_flDieTime		= 0.5f;

	sParticle->m_uchColor[0]	= 225;
	sParticle->m_uchColor[1]	= 140;
	sParticle->m_uchColor[2]	= 64;
	sParticle->m_uchStartAlpha	= Helper_RandomInt( 64, 128 );
	sParticle->m_uchEndAlpha	= 0;

	sParticle->m_uchStartSize	= 1.0f;
	sParticle->m_uchEndSize		= 1.0f;
	
	sParticle->m_vecVelocity	= RandomVector( -8.0f, 8.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_SporeTrail::Update( float fTimeDelta )
{
	if ( m_pParticleMgr == NULL )
		return;

	//Add new particles
	if ( m_bEmit )
	{
		float tempDelta = fTimeDelta;
		
		while ( m_teParticleSpawn.NextEvent( tempDelta ) )
		{
			AddParticles();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &effectMatrix - 
//-----------------------------------------------------------------------------
void C_SporeTrail::StartRender( VMatrix &effectMatrix )
{
	effectMatrix = effectMatrix * m_mAttachmentMatrix;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			*pDraw - 
//			&sortKey - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SporeTrail::SimulateAndRender( Particle *pBaseParticle, ParticleDraw *pDraw, float &sortKey )
{
	if ( m_bEmit == false )
		return false;

	SimpleParticle *pParticle = (SimpleParticle *) pBaseParticle;
	float timeDelta = pDraw->GetTimeDelta();

	//Render
	Vector	tPos;
	TransformParticle( m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos );
	sortKey = tPos.z;

	Vector	color = Vector( 1.0f, 1.0f, 1.0f );

	//Render it
	RenderParticle_ColorSize(
		pDraw,
		tPos,
		color,
		1.0f,
		4 );

	/*
	Vector pos = m_mAttachmentMatrix.GetTranslation();

	debugoverlay->AddLineOverlay( tPos, pos, 255, 0, 0, true, 0 ); 
	debugoverlay->AddBoxOverlay( tPos, -Vector( 1, 1, 1 ), Vector( 1, 1, 1 ), QAngle( 0.0f, 0.0f, 0.0f ), 255, 0, 0, 0, 0 );
	*/
	
	//Update velocity
	//UpdateVelocity( pParticle, timeDelta );
	pParticle->m_Pos += pParticle->m_vecVelocity * timeDelta;

	//Should this particle die?
	pParticle->m_flLifetime += timeDelta;

	if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SporeTrail::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	C_BaseEntity *pEnt = pAttachedTo->GetBaseEntity();
	
	pEnt->GetAttachment( 1, *pAbsOrigin, *pAbsAngles );

	matrix3x4_t	matrix;

	AngleMatrix( *pAbsAngles, *pAbsOrigin, matrix );

	m_mAttachmentMatrix = matrix;
}
