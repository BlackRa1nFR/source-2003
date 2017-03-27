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

#if !defined( SIMPLE3D_H )
#define SIMPLE3D_H
#ifdef _WIN32
#pragma once
#endif

#include "particles_simple.h"
#include "particle_collision.h"

// Particle3D

class Particle3D : public Particle
{
public:
	QAngle		m_vAngles;
	float		m_flAngSpeed;		// Is same on all axis
	float		m_flDieTime;		// How long it lives for.
	float		m_flLifetime;		// How long it has been alive for so far.
	Vector		m_vecVelocity;
	
	byte		m_uchFrontColor[3];	
	byte		m_uchBackColor[3];	
	byte		m_uchSize;			
	byte		m_pad;				// Pad to 8 bytes.
};

//
// CSimple3DEmitter
//

class CSimple3DEmitter : public CSimpleEmitter
{
public:
	CSimple3DEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	static CSmartPtr<CSimple3DEmitter>	Create( const char *pDebugName );

	virtual	bool		SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey );
	CParticleCollision	m_ParticleCollision;

private:
	CSimple3DEmitter( const CSimple3DEmitter & );

};

#endif	//SIMPLE3D_H