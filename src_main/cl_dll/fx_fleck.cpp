//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "FX_Fleck.h"

//
// CFleckParticles
//

//-----------------------------------------------------------------------------
// Purpose: Test for surrounding collision surfaces for quick collision testing for the particle system
// Input  : &origin - starting position
//			*dir - direction of movement (if NULL, will do a point emission test in four directions)
//			angularSpread - looseness of the spread
//			minSpeed - minimum speed
//			maxSpeed - maximum speed
//			gravity - particle gravity for the sytem
//			dampen - dampening amount on collisions
//			flags - extra information
//-----------------------------------------------------------------------------
void CFleckParticles::Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags )
{
	//See if we've specified a direction
	m_ParticleCollision.Setup( origin, direction, angularSpread, minSpeed, maxSpeed, gravity, dampen );
}

//-----------------------------------------------------------------------------
// Purpose: Simulate and render the particle in this system
// Input  : *pInParticle - particle to consider
//			*pDraw - drawing utilities
//			&sortKey - sorting key
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFleckParticles::SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey )
{
	FleckParticle *pParticle = (FleckParticle *) pInParticle;

	const float	timeDelta = pDraw->GetTimeDelta();

	//Should this particle die?
	pParticle->m_flLifetime += timeDelta;

	if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		return false;
	
	//Render
	Vector	tPos;

	TransformParticle( g_ParticleMgr.GetModelView(), pParticle->m_Pos, tPos );
	sortKey = (int) tPos.z;
	
	Vector	color;
	color[0] = pParticle->m_uchColor[0] / 255.0f;
	color[1] = pParticle->m_uchColor[1] / 255.0f;
	color[2] = pParticle->m_uchColor[2] / 255.0f;

	pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;

	//Render it
	RenderParticle_ColorSizeAngle(
		pDraw,
		tPos,
		color,
		1.0f - (pParticle->m_flLifetime / pParticle->m_flDieTime),
		pParticle->m_uchSize,
		pParticle->m_flRoll );

	//Simulate the movement with collision
	trace_t trace;
	m_ParticleCollision.MoveParticle( pParticle->m_Pos, pParticle->m_vecVelocity, &pParticle->m_flRollDelta, timeDelta, &trace );

	return true;
}
