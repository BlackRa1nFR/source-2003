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
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#if !defined( FXFLECKS_H )
#define FXFLECKS_H
#ifdef _WIN32
#pragma once
#endif

#include "particles_simple.h"
#include "particlemgr.h"
#include "particle_collision.h"

// FleckParticle

class FleckParticle : public Particle
{
public:
	byte		m_uchColor[3];
	byte		m_uchSize;
	float		m_flRoll;
	float		m_flRollDelta;
	float		m_flDieTime;	// How long it lives for.
	float		m_flLifetime;	// How long it has been alive for so far.
	Vector		m_vecVelocity;
};

//
// CFleckParticles
//

class CFleckParticles : public CSimpleEmitter
{
public:

							CFleckParticles( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	static CSmartPtr<CFleckParticles> Create( const char *pDebugName )	{return new CFleckParticles( pDebugName );}

	virtual	bool		SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey );

	//Setup for point emission
	virtual void		Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags = 0 );

	CParticleCollision m_ParticleCollision;

private:
	CFleckParticles( const CFleckParticles & ); // not defined, not accessible
};

#endif	//FXFLECKS_H