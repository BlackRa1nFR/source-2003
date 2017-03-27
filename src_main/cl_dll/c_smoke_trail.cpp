//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Contains the smoke trail effect
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_smoke_trail.h"
#include "fx.h"
#include "engine/IVDebugOverlay.h"
#include "engine/ienginesound.h"
#include "c_te_effect_dispatch.h"
#include "glow_overlay.h"
#include "fx_explosion.h"

//
// CRocketTrailParticle
//

class CRocketTrailParticle : public CSimpleEmitter
{
public:
	
	CRocketTrailParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CRocketTrailParticle *Create( const char *pDebugName )
	{
		return new CRocketTrailParticle( pDebugName );
	}

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -8.0f );

		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
		}

		return pParticle->m_flRoll;
	}

	//Alpha
	virtual float UpdateAlpha( SimpleParticle *pParticle, float timeDelta )
	{
		return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
	}

private:
	CRocketTrailParticle( const CRocketTrailParticle & );
};


// Datatable.. this can have all the smoketrail parameters when we need it to.
IMPLEMENT_CLIENTCLASS_DT(C_SmokeTrail, DT_SmokeTrail, SmokeTrail)
	RecvPropFloat(RECVINFO(m_SpawnRate)),
	RecvPropVector(RECVINFO(m_StartColor)),
	RecvPropVector(RECVINFO(m_EndColor)),
	RecvPropFloat(RECVINFO(m_ParticleLifetime)),
	RecvPropFloat(RECVINFO(m_StopEmitTime)),
	RecvPropFloat(RECVINFO(m_MinSpeed)),
	RecvPropFloat(RECVINFO(m_MaxSpeed)),
	RecvPropFloat(RECVINFO(m_StartSize)),
	RecvPropFloat(RECVINFO(m_EndSize)),
	RecvPropFloat(RECVINFO(m_SpawnRadius)),
	RecvPropInt(RECVINFO(m_bEmit)),
	RecvPropInt(RECVINFO(m_nAttachment)),	
	RecvPropFloat(RECVINFO(m_Opacity)),
END_RECV_TABLE()

// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_SmokeTrail::C_SmokeTrail()
{
	m_MaterialHandle[0] = NULL;
	m_MaterialHandle[1] = NULL;
	
	m_SpawnRate = 10;
	m_ParticleSpawn.Init(10);
	m_StartColor.Init(0.5, 0.5, 0.5);
	m_EndColor.Init(0,0,0);
	m_ParticleLifetime = 5;
	m_StopEmitTime = 0;	// No end time
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_VelocityOffset.Init();
	m_Opacity = 0.5f;

	m_bEmit = true;

	m_nAttachment	= -1;

	m_pSmokeEmitter = NULL;
	m_pParticleMgr	= NULL;
}

C_SmokeTrail::~C_SmokeTrail()
{
	if ( m_pParticleMgr )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SmokeTrail::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	C_BaseEntity *pEnt = pAttachedTo->GetBaseEntity();
	if (pEnt && (m_nAttachment > 0))
	{
		pEnt->GetAttachment( m_nAttachment, *pAbsOrigin, *pAbsAngles );
	}
	else
	{
		BaseClass::GetAimEntOrigin( pAttachedTo, pAbsOrigin, pAbsAngles );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEmit - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::SetEmit(bool bEmit)
{
	m_bEmit = bEmit;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rate - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::SetSpawnRate(float rate)
{
	m_SpawnRate = rate;
	m_ParticleSpawn.Init(rate);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start( &g_ParticleMgr, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SmokeTrail::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	if(!pParticleMgr->AddEffect( &m_ParticleEffect, this ))
		return;

	m_pParticleMgr	= pParticleMgr;
	m_pSmokeEmitter = CSimpleEmitter::Create("smokeTrail");
	
	if ( !m_pSmokeEmitter )
	{
		Assert( false );
		return;
	}

	m_pSmokeEmitter->SetSortOrigin( GetAbsOrigin() );
	m_pSmokeEmitter->SetNearClip( 64.0f, 128.0f );

	m_MaterialHandle[0] = m_pSmokeEmitter->GetPMaterial( "particle/particle_smokegrenade" );
	m_MaterialHandle[1] = m_pSmokeEmitter->GetPMaterial( "particle/particle_noisesphere" );
	
	m_ParticleSpawn.Init( m_SpawnRate );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::Update( float fTimeDelta )
{
	if ( !m_pSmokeEmitter )
		return;

	Vector	offsetColor;

	// Add new particles
	if( m_bEmit )
	{
		if ( ( m_StopEmitTime == 0 ) || ( m_StopEmitTime > gpGlobals->curtime ) )
		{
	  	    float tempDelta = fTimeDelta;

			SimpleParticle	*pParticle;
			Vector			offset;

			while( m_ParticleSpawn.NextEvent( tempDelta ) )
		    {
				offset.Random( -m_SpawnRadius, m_SpawnRadius );
				offset += GetAbsOrigin();
				
				pParticle = (SimpleParticle *) m_pSmokeEmitter->AddParticle( sizeof( SimpleParticle ), m_MaterialHandle[random->RandomInt(0,1)], offset );

				if ( pParticle != NULL )
				{
					pParticle->m_flLifetime		= 0.0f;
					pParticle->m_flDieTime		= m_ParticleLifetime;

					pParticle->m_vecVelocity.Random( -1.0f, 1.0f );
					pParticle->m_vecVelocity *= random->RandomFloat( m_MinSpeed, m_MaxSpeed );
					
					offsetColor = m_StartColor * random->RandomFloat( 0.5f, 1.5f );

					offsetColor[0] = clamp( offsetColor[0], 0.0f, 1.0f );
					offsetColor[1] = clamp( offsetColor[1], 0.0f, 1.0f );
					offsetColor[2] = clamp( offsetColor[2], 0.0f, 1.0f );

					pParticle->m_uchColor[0]	= offsetColor[0]*255.0f;
					pParticle->m_uchColor[1]	= offsetColor[1]*255.0f;
					pParticle->m_uchColor[2]	= offsetColor[2]*255.0f;
					
					pParticle->m_uchStartSize	= m_StartSize;
					pParticle->m_uchEndSize		= m_EndSize;
					
					float alpha = random->RandomFloat( m_Opacity*0.75f, m_Opacity*1.25f );

					if ( alpha > 1.0f )
						alpha = 1.0f;
					if ( alpha < 0.0f )
						alpha = 0.0f;

					pParticle->m_uchStartAlpha	= alpha * 255; 
					pParticle->m_uchEndAlpha	= 0;
					
					pParticle->m_flRoll			= random->RandomInt( 0, 360 );
					pParticle->m_flRollDelta	= random->RandomFloat( -1.0f, 1.0f );
				}
            }
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pBaseParticle - 
//			*pDraw - 
//			&sortKey - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SmokeTrail::SimulateAndRender(Particle *pBaseParticle, ParticleDraw *pDraw, float &sortKey)
{
	return false;
}

//==================================================
// RocketTrail
//==================================================

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(RocketTrail, C_RocketTrail);

// Datatable.. this can have all the smoketrail parameters when we need it to.
IMPLEMENT_CLIENTCLASS_DT(C_RocketTrail, DT_RocketTrail, RocketTrail)
	RecvPropFloat(RECVINFO(m_SpawnRate)),
	RecvPropVector(RECVINFO(m_StartColor)),
	RecvPropVector(RECVINFO(m_EndColor)),
	RecvPropFloat(RECVINFO(m_ParticleLifetime)),
	RecvPropFloat(RECVINFO(m_StopEmitTime)),
	RecvPropFloat(RECVINFO(m_MinSpeed)),
	RecvPropFloat(RECVINFO(m_MaxSpeed)),
	RecvPropFloat(RECVINFO(m_StartSize)),
	RecvPropFloat(RECVINFO(m_EndSize)),
	RecvPropFloat(RECVINFO(m_SpawnRadius)),
	RecvPropInt(RECVINFO(m_bEmit)),
	RecvPropInt(RECVINFO(m_nAttachment)),	
	RecvPropFloat(RECVINFO(m_Opacity)),
	RecvPropInt(RECVINFO(m_bDamaged)),
END_RECV_TABLE()

// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_RocketTrail::C_RocketTrail()
{
	m_MaterialHandle[0] = NULL;
	m_MaterialHandle[1] = NULL;
	
	m_SpawnRate = 10;
	m_ParticleSpawn.Init(10);
	m_StartColor.Init(0.5, 0.5, 0.5);
	m_EndColor.Init(0,0,0);
	m_ParticleLifetime = 5;
	m_StopEmitTime = 0;	// No end time
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_VelocityOffset.Init();
	m_Opacity = 0.5f;

	m_bEmit		= true;
	m_bDamaged	= false;

	m_nAttachment	= -1;

	m_pRocketEmitter = NULL;
	m_pParticleMgr	= NULL;
}

C_RocketTrail::~C_RocketTrail()
{
	if ( m_pParticleMgr )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_RocketTrail::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	C_BaseEntity *pEnt = pAttachedTo->GetBaseEntity();
	if (pEnt && (m_nAttachment > 0))
	{
		pEnt->GetAttachment( m_nAttachment, *pAbsOrigin, *pAbsAngles );
	}
	else
	{
		BaseClass::GetAimEntOrigin( pAttachedTo, pAbsOrigin, pAbsAngles );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEmit - 
//-----------------------------------------------------------------------------
void C_RocketTrail::SetEmit(bool bEmit)
{
	m_bEmit = bEmit;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rate - 
//-----------------------------------------------------------------------------
void C_RocketTrail::SetSpawnRate(float rate)
{
	m_SpawnRate = rate;
	m_ParticleSpawn.Init(rate);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_RocketTrail::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start( &g_ParticleMgr, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_RocketTrail::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_RocketTrail::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	if(!pParticleMgr->AddEffect( &m_ParticleEffect, this ))
		return;

	m_pParticleMgr	= pParticleMgr;
	m_pRocketEmitter = CRocketTrailParticle::Create("smokeTrail");
	if ( !m_pRocketEmitter )
	{
		Assert( false );
		return;
	}

	m_pRocketEmitter->SetSortOrigin( GetAbsOrigin() );
	m_pRocketEmitter->SetNearClip( 64.0f, 128.0f );

	m_MaterialHandle[0] = m_pRocketEmitter->GetPMaterial( "particle/particle_smokegrenade" );
	m_MaterialHandle[1] = m_pRocketEmitter->GetPMaterial( "particle/particle_noisesphere" );
	
	m_ParticleSpawn.Init( m_SpawnRate );

	m_vecLastPosition = GetAbsOrigin();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_RocketTrail::Update( float fTimeDelta )
{
	if ( !m_pRocketEmitter )
		return;

	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash" );
	pSimple->SetSortOrigin( GetAbsOrigin() );
	
	SimpleParticle *pParticle;
	Vector			forward, offset;
	float			scale = 1.5f;

	AngleVectors( GetAbsAngles(), &forward );
	
	forward.Negate();

	float flScale = random->RandomFloat( scale-0.5f, scale+0.5f );

	//
	// Flash
	//

	int i;

	for ( i = 1; i < 9; i++ )
	{
		offset = GetAbsOrigin() + (forward * (i*2.0f*scale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/muzzleflash%d", random->RandomInt(1,4) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.01f;

		pParticle->m_vecVelocity.Init();

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 128;

		pParticle->m_uchStartSize	= (random->RandomFloat( 6.0f, 9.0f ) * (12-(i))/9) * flScale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	// Add new particles (undamaged version)
	if ( m_bEmit )
	{
		Vector	moveDiff	= GetAbsOrigin() - m_vecLastPosition;
		float	moveLength	= VectorNormalize( moveDiff );

		int	numPuffs = moveLength / ( m_StartSize / 2.0f );

		//debugoverlay->AddLineOverlay( m_vecLastPosition, GetAbsOrigin(), 255, 0, 0, true, 2.0f ); 
		
		//FIXME: More rational cap here, perhaps
		if ( numPuffs > 50 )
			numPuffs = 50;

		SimpleParticle	*pParticle;
		Vector			offset;
		Vector			offsetColor;
		float			step = moveLength / numPuffs;

		//Fill in the gaps
		for ( i = 1; i < numPuffs+1; i++ )
		{
			offset = m_vecLastPosition + ( moveDiff * step * i );

			//debugoverlay->AddBoxOverlay( offset, -Vector(2,2,2), Vector(2,2,2), vec3_angle, i*4, i*4, i*4, true, 4.0f );
			
			pParticle = (SimpleParticle *) m_pRocketEmitter->AddParticle( sizeof( SimpleParticle ), m_MaterialHandle[random->RandomInt(0,1)], offset );

			if ( pParticle != NULL )
			{
				pParticle->m_flLifetime		= 0.0f;
				pParticle->m_flDieTime		= m_ParticleLifetime + random->RandomFloat(m_ParticleLifetime*0.9f,m_ParticleLifetime*1.1f);

				pParticle->m_vecVelocity.Random( -1.0f, 1.0f );
				pParticle->m_vecVelocity *= random->RandomFloat( m_MinSpeed, m_MaxSpeed );
				
				offsetColor = m_StartColor * random->RandomFloat( 0.75f, 1.25f );

				offsetColor[0] = clamp( offsetColor[0], 0.0f, 1.0f );
				offsetColor[1] = clamp( offsetColor[1], 0.0f, 1.0f );
				offsetColor[2] = clamp( offsetColor[2], 0.0f, 1.0f );

				pParticle->m_uchColor[0]	= offsetColor[0]*255.0f;
				pParticle->m_uchColor[1]	= offsetColor[1]*255.0f;
				pParticle->m_uchColor[2]	= offsetColor[2]*255.0f;
				
				pParticle->m_uchStartSize	= m_StartSize * random->RandomFloat( 0.75f, 1.25f );
				pParticle->m_uchEndSize		= m_EndSize * random->RandomFloat( 1.0f, 1.25f );
				
				float alpha = random->RandomFloat( m_Opacity*0.75f, m_Opacity*1.25f );

				if ( alpha > 1.0f )
					alpha = 1.0f;
				if ( alpha < 0.0f )
					alpha = 0.0f;

				pParticle->m_uchStartAlpha	= alpha * 255; 
				pParticle->m_uchEndAlpha	= 0;
				
				pParticle->m_flRoll			= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
			}
		}
	}
	
	if ( m_bDamaged )
	{
		SimpleParticle	*pParticle;
		Vector			offset;
		Vector			offsetColor;

		CSmartPtr<CEmberEffect>	pEmitter = CEmberEffect::Create("C_RocketTrail::damaged");

		pEmitter->SetSortOrigin( GetAbsOrigin() );

		PMaterialHandle flameMaterial = m_pRocketEmitter->GetPMaterial( VarArgs( "sprites/flamelet%d", random->RandomInt( 1, 4 ) ) );
		
		// Flames from the rocket
		for ( i = 0; i < 8; i++ )
		{
			offset = RandomVector( -8, 8 ) + GetAbsOrigin();

			pParticle = (SimpleParticle *) pEmitter->AddParticle( sizeof( SimpleParticle ), flameMaterial, offset );

			if ( pParticle != NULL )
			{
				pParticle->m_flLifetime		= 0.0f;
				pParticle->m_flDieTime		= 0.25f;

				pParticle->m_vecVelocity.Random( -1.0f, 1.0f );
				pParticle->m_vecVelocity *= random->RandomFloat( 32, 128 );
				
				offsetColor = m_StartColor * random->RandomFloat( 0.75f, 1.25f );

				offsetColor[0] = clamp( offsetColor[0], 0.0f, 1.0f );
				offsetColor[1] = clamp( offsetColor[1], 0.0f, 1.0f );
				offsetColor[2] = clamp( offsetColor[2], 0.0f, 1.0f );

				pParticle->m_uchColor[0]	= offsetColor[0]*255.0f;
				pParticle->m_uchColor[1]	= offsetColor[1]*255.0f;
				pParticle->m_uchColor[2]	= offsetColor[2]*255.0f;
				
				pParticle->m_uchStartSize	= 8.0f;
				pParticle->m_uchEndSize		= 32.0f;
				
				pParticle->m_uchStartAlpha	= 255;
				pParticle->m_uchEndAlpha	= 0;
				
				pParticle->m_flRoll			= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
			}
		}
	}

	m_vecLastPosition = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pBaseParticle - 
//			*pDraw - 
//			&sortKey - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_RocketTrail::SimulateAndRender(Particle *pBaseParticle, ParticleDraw *pDraw, float &sortKey)
{
	return false;
}

SporeEffect::SporeEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
}


SporeEffect* SporeEffect::Create( const char *pDebugName )
{
	return new SporeEffect( pDebugName );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
void SporeEffect::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	float	speed = VectorNormalize( pParticle->m_vecVelocity );
	Vector	offset;

	speed -= ( 64.0f * timeDelta );

	offset.Random( -0.5f, 0.5f );

	pParticle->m_vecVelocity += offset;
	VectorNormalize( pParticle->m_vecVelocity );

	pParticle->m_vecVelocity *= speed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
Vector SporeEffect::UpdateColor( SimpleParticle *pParticle, float timeDelta )
{
	Vector	color;
	float	ramp = ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) );//1.0f - ( pParticle->m_flLifetime / pParticle->m_flDieTime );

	color[0] = ( (float) pParticle->m_uchColor[0] * ramp ) / 255.0f;
	color[1] = ( (float) pParticle->m_uchColor[1] * ramp ) / 255.0f;
	color[2] = ( (float) pParticle->m_uchColor[2] * ramp ) / 255.0f;

	return color;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float SporeEffect::UpdateAlpha( SimpleParticle *pParticle, float timeDelta )
{
	return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
}

//==================================================
// C_SporeExplosion
//==================================================

EXPOSE_PROTOTYPE_EFFECT( SporeExplosion, C_SporeExplosion );

IMPLEMENT_CLIENTCLASS_DT( C_SporeExplosion, DT_SporeExplosion, SporeExplosion )
	RecvPropFloat(RECVINFO(m_flSpawnRate)),
	RecvPropFloat(RECVINFO(m_flParticleLifetime)),
	RecvPropFloat(RECVINFO(m_flStartSize)),
	RecvPropFloat(RECVINFO(m_flEndSize)),
	RecvPropFloat(RECVINFO(m_flSpawnRadius)),
END_RECV_TABLE()

C_SporeExplosion::C_SporeExplosion( void )
{
	m_pParticleMgr			= NULL;

	m_flSpawnRate			= 10;
	m_flParticleLifetime	= 5;
	m_flStartSize			= 35;
	m_flEndSize				= 55;
	m_flSpawnRadius			= 2;
	m_pSporeEffect			= NULL;

	m_teParticleSpawn.Init( 32 );

	m_bEmit = true;
}

C_SporeExplosion::~C_SporeExplosion()
{
	if ( m_pParticleMgr != NULL )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SporeExplosion::OnDataChanged( DataUpdateType_t updateType )
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
bool C_SporeExplosion::ShouldDraw( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SporeExplosion::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	//Add us into the effect manager
	if( pParticleMgr->AddEffect( &m_ParticleEffect, this ) == false )
		return;

	//Create our main effect
	m_pSporeEffect = SporeEffect::Create( "C_SporeExplosion" );
	
	if ( m_pSporeEffect == NULL )
		return;

	m_hMaterial	= m_pSporeEffect->GetPMaterial( "particle/fire" );

	m_pSporeEffect->SetSortOrigin( GetAbsOrigin() );
	m_pSporeEffect->SetNearClip( 64, 128 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SporeExplosion::AddParticles( void )
{
	//Spores
	Vector	offset;
	Vector	dir;

	//Get our direction
	AngleVectors( GetAbsAngles(), &dir );

	SimpleParticle	*sParticle;

	for ( int i = 0; i < 4; i++ )
	{
		//Add small particle to the effect's origin
		offset.Random( -32.0f, 32.0f );
		sParticle = (SimpleParticle *) m_pSporeEffect->AddParticle( sizeof(SimpleParticle), m_hMaterial, GetAbsOrigin()+offset );

		if ( sParticle == NULL )
			return;

		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= 2.0f;

		sParticle->m_flRoll			= 0;
		sParticle->m_flRollDelta	= 0;

		sParticle->m_uchColor[0]	= 225;
		sParticle->m_uchColor[1]	= 140;
		sParticle->m_uchColor[2]	= 64;
		sParticle->m_uchStartAlpha	= Helper_RandomInt( 128, 255 );
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= Helper_RandomInt( 1, 2 );
		sParticle->m_uchEndSize		= 1;

		sParticle->m_vecVelocity	= dir * Helper_RandomFloat( 128.0f, 256.0f );
	}

	//Add smokey bits
	offset.Random( -16.0f, 16.0f );
	sParticle = (SimpleParticle *) m_pSporeEffect->AddParticle( sizeof(SimpleParticle), m_pSporeEffect->GetPMaterial( "particle/particle_noisesphere"), GetAbsOrigin()+offset );

	if ( sParticle == NULL )
		return;

	sParticle->m_flLifetime		= 0.0f;
	sParticle->m_flDieTime		= 1.0f;

	sParticle->m_flRoll			= Helper_RandomFloat( 0, 360 );
	sParticle->m_flRollDelta	= Helper_RandomFloat( -2.0f, 2.0f );

	sParticle->m_uchColor[0]	= 225;
	sParticle->m_uchColor[1]	= 140;
	sParticle->m_uchColor[2]	= 64;
	sParticle->m_uchStartAlpha	= Helper_RandomInt( 32, 64 );
	sParticle->m_uchEndAlpha	= 0;
	sParticle->m_uchStartSize	= 32;
	sParticle->m_uchEndSize		= 64;

	sParticle->m_vecVelocity	= dir * Helper_RandomFloat( 64.0f, 128.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_SporeExplosion::Update( float fTimeDelta )
{
	float tempDelta = fTimeDelta;
	
	while ( m_teParticleSpawn.NextEvent( tempDelta ) )
	{
		AddParticles();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			*pDraw - 
//			&sortKey - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SporeExplosion::SimulateAndRender( Particle *pBaseParticle, ParticleDraw *pDraw, float &sortKey )
{
	StandardParticle_t *pParticle = (StandardParticle_t*) pBaseParticle;

	//Update its lifetime
	pParticle->m_Lifetime += pDraw->GetTimeDelta();
	
	if( pParticle->m_Lifetime > m_flParticleLifetime )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RPGShotDownCallback( const CEffectData &data )
{
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "BaseExplosionEffect.Sound", &data.m_vOrigin );


	if ( CExplosionOverlay *pOverlay = new CExplosionOverlay )
	{
		pOverlay->m_flLifetime	= 0;
		pOverlay->m_vPos		= data.m_vOrigin;
		pOverlay->m_nSprites	= 1;
		
		pOverlay->m_vBaseColors[0].Init( 1.0f, 0.9f, 0.7f );

		pOverlay->m_Sprites[0].m_flHorzSize = 0.01f;
		pOverlay->m_Sprites[0].m_flVertSize = pOverlay->m_Sprites[0].m_flHorzSize*0.5f;

		pOverlay->Activate();
	}
}

DECLARE_CLIENT_EFFECT( "RPGShotDown", RPGShotDownCallback );