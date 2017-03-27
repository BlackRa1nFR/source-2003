//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Id: $
//
// Contains the particle storm effect
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_particle_storm.h"
#include "particles_simple.h"

#define PSTORM_FLASH_INTENSITY 2000

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(CParticleStorm, C_ParticleStorm);

IMPLEMENT_CLIENTCLASS_DT(C_ParticleStorm, DT_ParticleStorm, CParticleStorm)
	RecvPropFloat(RECVINFO(m_SpawnRate)),
	RecvPropVector(RECVINFO(m_StartColor)),
	RecvPropVector(RECVINFO(m_EndColor)),
	RecvPropFloat(RECVINFO(m_ParticleLifetime)),
	RecvPropFloat(RECVINFO(m_MinSpeed)),
	RecvPropFloat(RECVINFO(m_MaxSpeed)),
	RecvPropFloat(RECVINFO(m_StartSize)),
	RecvPropFloat(RECVINFO(m_EndSize)),
	RecvPropFloat(RECVINFO(m_SpawnRadius)),
	RecvPropInt(RECVINFO(m_bEmit)),
	RecvPropVector(RECVINFO(m_vecAttachVel)),
	RecvPropFloat(RECVINFO(m_flSuck)),
	RecvPropFloat(RECVINFO(m_flDim)),
END_RECV_TABLE()

// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_ParticleStorm::C_ParticleStorm()
{
	m_pParticleMgr = NULL;
	m_MaterialHandle = 0;
	
	m_SpawnRate = 10;
	m_ParticleSpawn.Init(10);
	m_StartColor.Init(0.5, 0.5, 0.5);
	m_EndColor.Init(0,0,0);
	m_ParticleLifetime = 5;
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_VelocityOffset.Init();

	m_Pos.Init();
	m_LastPos.Init();

	m_bEmit = true;
}


C_ParticleStorm::~C_ParticleStorm()
{
}


void C_ParticleStorm::SetPos(const Vector &pos, bool bInitial)
{
	m_Pos = pos;

	if(bInitial)
		m_LastPos = pos;
}


void C_ParticleStorm::SetEmit(bool bEmit)
{
	m_bEmit = bEmit;
}


void C_ParticleStorm::SetSpawnRate(float rate)
{
	m_SpawnRate = rate;
	m_ParticleSpawn.Init(rate);
}


void C_ParticleStorm::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if(updateType == DATA_UPDATE_CREATED)
	{
		SetPos(GetAbsOrigin(), true);
		Start(&g_ParticleMgr, NULL);
	}
}


bool C_ParticleStorm::ShouldDraw()
{
	return true;
}


void C_ParticleStorm::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	if(!pParticleMgr->AddEffect( &m_ParticleEffect, this ))
		return;

	m_Pos = GetAbsOrigin();

	m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial("particle/dx8_particle_cloud");
	
	// Figure out how we need to draw.
	IMaterial *pMaterial = m_MaterialHandle;
	if( pMaterial )
	{
		m_Renderer.Init( pMaterial );
	}

	m_ParticleSpawn.Init(m_SpawnRate);

	m_pParticleMgr = pParticleMgr;
}


void C_ParticleStorm::Update(float fTimeDelta)
{
	if(!m_pParticleMgr)
		return;

	m_Pos = GetAbsOrigin();

	// Add new particles.
	if(m_bEmit)
	{
		float tempDelta = fTimeDelta;
		while(m_ParticleSpawn.NextEvent(tempDelta))
		{
			StandardParticle_t *pParticle = 
				(StandardParticle_t*)m_ParticleEffect.AddParticle( sizeof(StandardParticle_t), m_MaterialHandle );
			if (pParticle)
			{
				pParticle->m_Pos = (m_Pos + (m_LastPos - m_Pos) * FRand(0,1)) + RandomVector(-m_SpawnRadius, m_SpawnRadius);
				pParticle->m_Velocity = m_Pos - pParticle->m_Pos;
				pParticle->m_Lifetime = 0;

				if (pParticle->m_Pos.z < m_Pos.z)
				{
					pParticle->m_Pos.z = m_Pos.z;
				}
			}
		}
	}
	m_LastPos = m_Pos;

	// Draw.
	m_Renderer.DirectionalLight().m_vPos = m_Pos;
	if (m_flDim > 0)
	{
		m_Renderer.DirectionalLight().m_flIntensity = 0;
	}
	else
	{
		m_Renderer.DirectionalLight().m_vColor		= 2*m_EndColor;
		m_Renderer.DirectionalLight().m_flIntensity = PSTORM_FLASH_INTENSITY;
		m_flDim = 1;
	}

	m_Renderer.Update(m_pParticleMgr);
}

bool C_ParticleStorm::SimulateAndRender(Particle *pBaseParticle, ParticleDraw *pDraw, float &sortKey)
{
	StandardParticle_t *pParticle = (StandardParticle_t*)pBaseParticle;

	// Update its lifetime.
	pParticle->m_Lifetime += pDraw->GetTimeDelta();
	if(pParticle->m_Lifetime > m_ParticleLifetime)
	{
		return false;
	}

	// Draw.
	float lifetimePercent = pParticle->m_Lifetime / m_ParticleLifetime;
	Vector color = m_StartColor + (m_EndColor - m_StartColor) * lifetimePercent;
	float sinLifetime = (float)sin(lifetimePercent * 3.14159);

	Vector tPos;
	TransformParticle(m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos);
	sortKey = tPos.z;

	
	m_Renderer.RenderParticle_AddColor(pDraw,pParticle->m_Pos,tPos,		
						255 * sinLifetime * GetAlphaDistanceFade(tPos, 10, 75),
						FLerp(m_StartSize, m_EndSize, lifetimePercent),255*color);


	pParticle->m_Velocity = m_flSuck*(m_Pos - pParticle->m_Pos);

	// If I'm sucking in compensate for movement of the entity being tracked
	if (m_flSuck > 0)
	{
		float flDist = (m_Pos - pParticle->m_Pos).Length();

		// If I'm in the center region, track the center perfectly 
		if (flDist < 120)
		{
			pParticle->m_Velocity += m_vecAttachVel;

			// If I'm near (but not in the center) adjust my lifetime so that my color and size match
			if (flDist < 50 && flDist > 30)
			{
				float flScale = 1 - flDist/(m_SpawnRadius);
				pParticle->m_Lifetime = 0.05*( flScale * m_ParticleLifetime)+ 0.95*pParticle->m_Lifetime;
			}
		}
	}
	
	// Move it (this comes after rendering to make it clear that moving the particle here won't change
	// its rendering for this frame since m_TransformedPos has already been set).
	pParticle->m_Pos = pParticle->m_Pos + pParticle->m_Velocity * pDraw->GetTimeDelta();
	return true;
}

