//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "particles_simple.h"
#include "particlemgr.h"
#include "particle_collision.h"
#include "env_objecteffects.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSmokeParticles::Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags )
{
	// See if we've specified a direction
	m_ParticleCollision.Setup( origin, direction, angularSpread, minSpeed, maxSpeed, gravity, dampen );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CObjectSmokeParticles::SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey )
{
	ObjectSmokeParticle *pParticle = (ObjectSmokeParticle *) pInParticle;
	float timeDelta = pDraw->GetTimeDelta();

	//Render
	Vector	tPos;

	TransformParticle( g_ParticleMgr.GetModelView(), pParticle->m_Pos, tPos );
	sortKey = (int) tPos.z;

	//Render it
	RenderParticle_ColorSizeAngle(
		pDraw,
		tPos,
		UpdateColor( pParticle, timeDelta ),
		UpdateAlpha( pParticle, timeDelta ) * GetAlphaDistanceFade( tPos, m_flNearClipMin, m_flNearClipMax ),
		UpdateScale( pParticle, timeDelta ),
		UpdateRoll( pParticle, timeDelta ) );

	//Update velocity
	UpdateVelocity( pParticle, timeDelta );
	pParticle->m_Pos += (pParticle->m_vecVelocity * timeDelta);
	pParticle->m_vecVelocity += pParticle->m_vecAcceleration * 2 * timeDelta;
	//pParticle->m_Pos += (pParticle->m_vecAcceleration * (0.5f * timeDelta * timeDelta));

	//Should this particle die?
	pParticle->m_flLifetime += timeDelta;

	if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectFireParticles::Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CObjectFireParticles::SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey )
{
	ObjectFireParticle *pParticle = (ObjectFireParticle *) pInParticle;
	float timeDelta = pDraw->GetTimeDelta();

	// Lost our parent?
	if ( !pParticle->m_hParent )
		return false;

	// Update position to match our parent
	Vector vecFire;
	QAngle angFire;
	if ( pParticle->m_hParent->GetAttachment( pParticle->m_iAttachmentPoint, vecFire, angFire ) )
	{
		pParticle->m_Pos = vecFire;
	}

	// Render
	Vector	tPos;
	TransformParticle( g_ParticleMgr.GetModelView(), pParticle->m_Pos, tPos );
	sortKey = (int) tPos.z;

	// Render it
	RenderParticle_ColorSizeAngle(
		pDraw,
		tPos,
		UpdateColor( pParticle, timeDelta ),
		UpdateAlpha( pParticle, timeDelta ) * GetAlphaDistanceFade( tPos, m_flNearClipMin, m_flNearClipMax ),
		UpdateScale( pParticle, timeDelta ),
		UpdateRoll( pParticle, timeDelta ) );

	// Should this particle die?
	pParticle->m_flLifetime += timeDelta;

	if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		return false;

	return true;
}
