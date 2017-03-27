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
#include "particle_simple3D.h"
#include "view.h"

// Defined in pm_math.c
float anglemod( float a );

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CSmartPtr<CSimple3DEmitter> CSimple3DEmitter::Create( const char *pDebugName )	
{
	CSimple3DEmitter* pSimple3DEmitter = new CSimple3DEmitter( pDebugName );

	// Do in world space
	pSimple3DEmitter->m_ParticleEffect.SetEffectCameraSpace( false );
	return pSimple3DEmitter;
}


//-----------------------------------------------------------------------------
// Purpose: Simulate and render the particle in this system
// Input  : *pInParticle - particle to consider
//			*pDraw - drawing utilities
//			&sortKey - sorting key
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSimple3DEmitter::SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey )
{
	Particle3D *pParticle = (Particle3D *) pInParticle;

	const float	timeDelta = pDraw->GetTimeDelta();

	//Should this particle die?
	pParticle->m_flLifetime += timeDelta;

	if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		return false;
	
	sortKey = CurrentViewForward().Dot( CurrentViewOrigin() - pParticle->m_Pos );

	// -------------------------------------------------------
	//  Set color based on direction towards camera
	// -------------------------------------------------------
	Vector	color;
	Vector vFaceNorm;
	Vector vCameraToFace = (pParticle->m_Pos - CurrentViewOrigin());
	AngleVectors(pParticle->m_vAngles,&vFaceNorm);
	float flFacing = DotProduct(vCameraToFace,vFaceNorm);

	if (flFacing <= 0)
	{
		color[0] = pParticle->m_uchFrontColor[0] / 255.0f;
		color[1] = pParticle->m_uchFrontColor[1] / 255.0f;
		color[2] = pParticle->m_uchFrontColor[2] / 255.0f;
	}
	else
	{
		color[0] = pParticle->m_uchBackColor[0] / 255.0f;
		color[1] = pParticle->m_uchBackColor[1] / 255.0f;
		color[2] = pParticle->m_uchBackColor[2] / 255.0f;
	}

	// Angular rotation
	pParticle->m_vAngles.x += pParticle->m_flAngSpeed * timeDelta;
	pParticle->m_vAngles.y += pParticle->m_flAngSpeed * timeDelta;
	pParticle->m_vAngles.z += pParticle->m_flAngSpeed * timeDelta;

	//Render it in world space
	RenderParticle_ColorSizeAngles(
		pDraw,
		pParticle->m_Pos,
		color,
		1.0f - (pParticle->m_flLifetime / pParticle->m_flDieTime),
		pParticle->m_uchSize,
		pParticle->m_vAngles);

	//Simulate the movement with collision
	trace_t trace;
	m_ParticleCollision.MoveParticle( pParticle->m_Pos, pParticle->m_vecVelocity, &pParticle->m_flAngSpeed, timeDelta, &trace );

	// ---------------------------------------
	// Decay towards flat
	// ---------------------------------------
	if (pParticle->m_flAngSpeed == 0 || trace.fraction != 1.0)
	{
		pParticle->m_vAngles.x = anglemod(pParticle->m_vAngles.x);
		if (pParticle->m_vAngles.x < 180)
		{
			if (fabs(pParticle->m_vAngles.x - 90) > 0.5)
			{
				pParticle->m_vAngles.x = 0.5*pParticle->m_vAngles.x + 46;
			}
		}
		else
		{
			if (fabs(pParticle->m_vAngles.x - 270) > 0.5)
			{
				pParticle->m_vAngles.x = 0.5*pParticle->m_vAngles.x + 135;
			}
		}

		pParticle->m_vAngles.y = anglemod(pParticle->m_vAngles.y);
		if (fabs(pParticle->m_vAngles.y) > 0.5)
		{
			pParticle->m_vAngles.y = 0.5*pParticle->m_vAngles.z;
		}
	}
	return true;
}

