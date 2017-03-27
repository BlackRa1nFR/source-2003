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

#if !defined( PARTICLE_COLLISION_H )
#define PARTICLE_COLLISION_H
#ifdef _WIN32
#pragma once
#endif

#include "particles_simple.h"
#include "particlemgr.h"

#define	MAX_COLLISION_PLANES	6

//
// CBaseSimpleCollision
//

class CBaseSimpleCollision
{
public:

	CBaseSimpleCollision( void );

	static	CBaseSimpleCollision *Create( void )	{	return	new CBaseSimpleCollision;	}

	virtual void	Setup( const Vector &origin, float speed, float gravity );
	virtual void	TraceLine( const Vector &start, const Vector &end, trace_t *pTrace, bool coarse = true );

	void	ClearActivePlanes( void	);

protected:

	virtual	void	TestForPlane( const Vector &start, const Vector &dir, float speed, float gravity );
	virtual	void	ConsiderPlane( cplane_t *plane );

	VPlane	m_collisionPlanes[MAX_COLLISION_PLANES];
	int		m_nActivePlanes;
};

//
// CParticleCollision
//

class CParticleCollision : public CBaseSimpleCollision
{
public:

	CParticleCollision( void );

	static	CParticleCollision *Create( void )	{	return	new CParticleCollision;	}

	virtual void	Setup( const Vector &origin, const Vector *dir, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen );
	virtual bool	MoveParticle( Vector &origin, Vector &velocity, float *rollDelta, float timeDelta, trace_t *pTrace );

	void	SetGravity( float gravity )					{	m_flGravity = gravity;			}
	void	SetCollisionDampen( float dampen )			{	m_flCollisionDampen = dampen;	}
	void	SetAngularCollisionDampen( float dampen )	{	m_flAngularCollisionDampen = dampen;}

protected:

	float	m_flGravity;
	float	m_flCollisionDampen;
	float	m_flAngularCollisionDampen;
};

#endif //PARTICLE_COLLISION_H