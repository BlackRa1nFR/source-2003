//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Contains the splash effect
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_splash.h"
#include "c_tracer.h"
#include "iefx.h"
#include "decals.h"
#include "engine/IEngineSound.h"
#include "view.h"

#define SPLASH_MAX_FALL_DIST 1000
#define SPLASH_SOUND_DELAY	 0.3

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(Splash, C_Splash);

// Datatable.. this can have all the smoketrail parameters when we need it to.
IMPLEMENT_CLIENTCLASS_DT(C_Splash, DT_Splash, CSplash)
	RecvPropFloat(RECVINFO(m_flSpawnRate)),
	RecvPropVector(RECVINFO(m_vStartColor)),
	RecvPropVector(RECVINFO(m_vEndColor)),
	RecvPropFloat(RECVINFO(m_flParticleLifetime)),
	RecvPropFloat(RECVINFO(m_flStopEmitTime)),
	RecvPropFloat(RECVINFO(m_flSpeed)),
	RecvPropFloat(RECVINFO(m_flSpeedRange)),
	RecvPropFloat(RECVINFO(m_flWidthMin)),
	RecvPropFloat(RECVINFO(m_flWidthMax)),
	RecvPropFloat(RECVINFO(m_flNoise)),
	RecvPropInt(RECVINFO(m_bEmit)),
	RecvPropInt(RECVINFO(m_nNumDecals)),
END_RECV_TABLE()

// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_Splash::C_Splash()
{
	m_pParticleMgr = NULL;
	m_MaterialHandle = 0;
	
	m_flSpawnRate = 10;
	m_ParticleSpawn.Init(10);
	m_vStartColor.Init(0.5, 0.5, 0.5);
	m_vEndColor.Init(0,0,0);
	m_flParticleLifetime = 5;
	m_flStopEmitTime	= 0;	// No end time
	m_flSpeed		= 20;
	m_flSpeedRange	= 10;
	m_flWidthMin	= 2;
	m_flWidthMax	= 8;
	m_flNoise		= 0.1;
	m_Pos.Init();

	m_bEmit = true;
	m_nNumDecals = 1;
}

void C_Splash::SetPos(const Vector &pos, bool bInitial)
{
	m_Pos = pos;
}


void C_Splash::SetEmit(bool bEmit)
{
	m_bEmit = bEmit;
}


void C_Splash::SetSpawnRate(float rate)
{
	m_flSpawnRate = rate;
	m_ParticleSpawn.Init(rate);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_Splash::InitParticleCollisions(void)
{
	float flMinSpeed = m_flSpeed - m_flSpeedRange;
	float flMaxSpeed = m_flSpeed + m_flSpeedRange;

	Vector vForward;
	AngleVectors(GetAbsAngles(),&vForward);
	m_ParticleCollision.Setup( GetAbsOrigin(), &vForward, m_flNoise, flMinSpeed, flMaxSpeed, 500, 0.5 );

}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_Splash::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if(updateType == DATA_UPDATE_CREATED)
	{
		SetPos(GetAbsOrigin(), true);
		Start(&g_ParticleMgr, NULL);
	}
	InitParticleCollisions();
}


bool C_Splash::ShouldDraw()
{
	return true;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_Splash::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	if(!pParticleMgr->AddEffect( &m_ParticleEffect, this ))
		return;

	m_Pos				= GetAbsOrigin();
	m_flStartEmitTime	= gpGlobals->curtime;

	// Trace down to see where particles should disappear
	trace_t tr;
	UTIL_TraceLine( m_Pos+Vector(0,0,1), m_Pos+Vector(0,0,-SPLASH_MAX_FALL_DIST), 
		MASK_SOLID_BRUSHONLY|CONTENTS_WATER, NULL, COLLISION_GROUP_NONE, &tr );
	m_flSplashEndZ = tr.endpos.z;

	m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial("particle/particle_smokegrenade");
	
	m_nDecalsRemaining	= m_nNumDecals;
	m_flNextSoundTime	= 0;


	m_ParticleSpawn.Init(m_flSpawnRate);
	m_pParticleMgr = pParticleMgr;

	InitParticleCollisions();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_Splash::PlaySplashSound(Vector vPos)
{
	if (gpGlobals->curtime > m_flNextSoundTime)
	{
		CLocalPlayerFilter filter;
		EmitSound( filter, index, "Splash.SplashSound", &vPos );
		m_flNextSoundTime = gpGlobals->curtime + SPLASH_SOUND_DELAY;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_Splash::Update(float fTimeDelta)
{
	if(!m_pParticleMgr)
	{
		return;
	}

	m_Pos = GetAbsOrigin();

	// Add new particles.
	if(m_bEmit)
	{
		if (m_flStopEmitTime == 0						|| 
			m_flStopEmitTime >  gpGlobals->curtime		)
		{
	  	    float tempDelta = fTimeDelta;
		    while(m_ParticleSpawn.NextEvent(tempDelta))
		    {
				SimpleParticle *pParticle = 
				    (SimpleParticle*)m_ParticleEffect.AddParticle( sizeof(StandardParticle_t), m_MaterialHandle );

				if (pParticle)
				{
					Vector vForward;
					AngleVectors(GetAbsAngles(),&vForward);

					//Make a new particle
					vForward.x += random->RandomFloat( -m_flNoise, m_flNoise );
					vForward.y += random->RandomFloat( -m_flNoise, m_flNoise );
					vForward.z += random->RandomFloat( -m_flNoise, m_flNoise );

					Vector vNewPos	=  GetAbsOrigin();
					Vector vNewVel	=  vForward*(m_flSpeed + random->RandomFloat(-m_flSpeedRange,m_flSpeedRange));
					pParticle->m_Pos			= vNewPos;
					pParticle->m_vecVelocity	= vNewVel;
					pParticle->m_flLifetime		= 0;
				}
            }
		}
	}
	
	// Decay amount of leak over time
	if (m_flStopEmitTime != 0)
	{
		float lifetimePercent = 1-((gpGlobals->curtime - m_flStartEmitTime) / (m_flStopEmitTime - m_flStartEmitTime));
		m_ParticleSpawn.ResetRate(m_flSpawnRate*lifetimePercent);
	}
	else
	{
		m_ParticleSpawn.ResetRate(m_flSpawnRate);
	}
}



//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool C_Splash::SimulateAndRender(Particle *pParticle, ParticleDraw *pDraw, float &sortKey)
{
	SimpleParticle* pSimpleParticle = (SimpleParticle*)pParticle;
	
	//Should this particle die?
	pSimpleParticle->m_flLifetime += pDraw->GetTimeDelta();
	if ( pSimpleParticle->m_flLifetime >= m_flParticleLifetime )
	{
		return false;
	}

	// Calculate color
	float lifetimePercent = pSimpleParticle->m_flLifetime / m_flParticleLifetime;
	float color[3];
	color[0] = m_vStartColor[0] + (m_vEndColor[0] - m_vStartColor[0]) * lifetimePercent;
	color[1] = m_vStartColor[1] + (m_vEndColor[1] - m_vStartColor[1]) * lifetimePercent;
	color[2] = m_vStartColor[2] + (m_vEndColor[2] - m_vStartColor[2]) * lifetimePercent;
	color[3] = 1.0;

	float scale = random->RandomFloat( 0.02, 0.08 );

	// NOTE: We need to do everything in screen space
	Vector  delta;
	Vector	start;
	TransformParticle(g_ParticleMgr.GetModelView(), pSimpleParticle->m_Pos, start);

	Vector3DMultiply( CurrentWorldToViewMatrix(), pSimpleParticle->m_vecVelocity, delta );

	delta[0] *= scale;
	delta[1] *= scale;
	delta[2] *= scale;

	// See c_tracer.* for this method
	Tracer_Draw( pDraw, start, delta, random->RandomInt( m_flWidthMin, m_flWidthMax ), color );

	//Simulate the movement with collision
	const float	timeDelta = pDraw->GetTimeDelta();
	trace_t trace;
	
	if (m_ParticleCollision.MoveParticle( pSimpleParticle->m_Pos, pSimpleParticle->m_vecVelocity, NULL, timeDelta, &trace ))
	{
		// If particle hits horizontal surface kill it soon
		if (DotProduct(trace.plane.normal, CurrentViewUp())>0.8)
		{
			pSimpleParticle->m_flLifetime = m_flParticleLifetime-0.2;
		}

		// Drop a decal if any remaining
		if (m_nDecalsRemaining>0)
		{
			C_BaseEntity *ent = cl_entitylist->GetEnt( 0 );
			if ( ent )
			{
				int index = decalsystem->GetDecalIndexForName( "Splash" );
				if ( index >= 0 )
				{
					color32 rgbaColor = {255*m_vStartColor[0],255*m_vStartColor[1],255*m_vStartColor[2],150};
					effects->DecalColorShoot( index, 0, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), trace.endpos, 0, 0, rgbaColor);
				}
				m_nDecalsRemaining--;
			}
		}
		PlaySplashSound(trace.endpos);
	}
	return true;
}

void C_Splash::Think(void)
{
	Release();
}


C_Splash::~C_Splash()
{
	if(m_pParticleMgr)
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}
