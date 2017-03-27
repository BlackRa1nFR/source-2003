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

#include "particles_simple.h"
#include "particlemgr.h"

#include "fx_fleck.h"

#define	bitsPARTICLE_TRAIL_VELOCITY_DAMPEN	0x00000001	//Dampen the velocity as the particles move
#define	bitsPARTICLE_TRAIL_COLLIDE			0x00000002	//Do collision with simulation
#define	bitsPARTICLE_TRAIL_FADE				0x00000004	//Fade away

class TrailParticle : public Particle
{
public:

	float		m_flDieTime;			// How long it lives for.
	float		m_flLifetime;			// How long it has been alive for so far.
	Vector		m_vecVelocity;
	float		m_flLength;				// Length of the tail (in seconds!)
	float		m_flWidth;				// Width of the spark
	float		m_flColor[4];			// Particle color
};

//
// CTrailParticles
//

class CTrailParticles : public CSimpleEmitter
{
	DECLARE_CLASS( CTrailParticles, CSimpleEmitter );
public:
	CTrailParticles( const char *pDebugName );
	
	static CTrailParticles	*Create( const char *pDebugName )	{	return new CTrailParticles( pDebugName );	}

	virtual	bool	SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey );

	//Setup for point emission
	virtual void	Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags );
	
	void SetFlag( int flags )				{	m_fFlags |= flags;	}
	void SetVelocityDampen( float dampen )	{	m_flVelocityDampen = dampen;	}
	void SetGravity( float gravity )		{	m_ParticleCollision.SetGravity( gravity );	}
	void SetCollisionDamped( float dampen )	{	m_ParticleCollision.SetCollisionDampen( dampen );	}
	void SetAngularCollisionDampen( float dampen )	{ m_ParticleCollision.SetAngularCollisionDampen( dampen );	}

	CParticleCollision	m_ParticleCollision;

protected:

	int					m_fFlags;
	float				m_flVelocityDampen;

private:
	CTrailParticles( const CTrailParticles & ); // not defined, not accessible
};

//
// Sphere trails
//

class CSphereTrails : public CSimpleEmitter
{
	DECLARE_CLASS( CSphereTrails, CSimpleEmitter );
public:
	CSphereTrails( const char *pDebugName, const Vector &origin, float innerRadius, float outerRadius, float speed, int entityIndex, int attachment );
	
	virtual bool SimulateAndRender( Particle *pInParticle, ParticleDraw *pDraw, float &sortKey );
	virtual void Update( float flTimeDelta );
	virtual void StartRender();

	void AddStreaks( float count );
	Vector	m_particleOrigin;
	float	m_life;
	float	m_outerRadius;
	float	m_innerRadius;
	float	m_effectSpeed;
	float	m_streakSpeed;
	float	m_count;
	float	m_growth;
	int		m_entityIndex;
	int		m_attachment;
	PMaterialHandle m_hMaterial;
	Vector	m_boneOrigin;
	float	m_dieTime;

private:
	CSphereTrails( const CSphereTrails & ); // not defined, not accessible
};

