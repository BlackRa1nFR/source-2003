// This defines the client-side SmokeTrail entity. It can also be used without
// an entity, in which case you must pass calls to it and set its position each frame.

#ifndef PARTICLE_STORM_H
#define PARTICLE_STORM_H

#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "particles_simple.h"
#include "c_baseentity.h"
#include "baseparticleentity.h"
#include "ParticleSphereRenderer.h"

class C_ParticleStorm : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_ParticleStorm, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();
	
					C_ParticleStorm();
	virtual			~C_ParticleStorm();

public:

	// Call this to move the source..
	// Set bInitial if this is the first call (or if it's not moving in a continuous motion).
	void			SetPos(const Vector &pos, bool bInitial);

	// Enable/disable emission.
	void			SetEmit(bool bEmit);

	// Change the spawn rate.
	void			SetSpawnRate(float rate);


// C_BaseEntity.
public:
	virtual	void	OnDataChanged(DataUpdateType_t updateType);
	virtual bool	ShouldDraw();


// IPrototypeAppEffect.
public:
	virtual void	Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs);

// IParticleEffect.
public:
	virtual void	Update(float fTimeDelta);
	virtual bool	SimulateAndRender(Particle *pParticle, ParticleDraw *pDraw, float &sortKey);


public:
	CParticleSphereRenderer	m_Renderer;

	// Effect parameters. These will assume default values but you can change them.
	float			m_SpawnRate;			// How many particles per second.

	Vector			m_StartColor;			// Fade between these colors.
	Vector			m_EndColor;

	float			m_ParticleLifetime;		// How long do the particles live?
	
	float			m_MinSpeed;				// Speed range.
	float			m_MaxSpeed;
	
	float			m_StartSize;			// Size ramp.
	float			m_EndSize;

	float			m_SpawnRadius;

	Vector			m_VelocityOffset;		// Emit the particles in a certain direction.
	Vector			m_vecVelocity;

	bool			m_bEmit;				// Keep emitting particles?
	Vector			m_vecAttachVel;
	float			m_flSuck;
	float			m_flDim;
	int				m_nTargetEntity;

private:
	C_ParticleStorm( const C_ParticleStorm & );

	PMaterialHandle m_MaterialHandle;
	TimedEvent		m_ParticleSpawn;

	Vector			m_Pos;
	Vector			m_LastPos;	// This is stored so we can spawn particles in between the previous and new position
								// to eliminate holes in the trail.
	CParticleMgr		*m_pParticleMgr;
};


#endif
