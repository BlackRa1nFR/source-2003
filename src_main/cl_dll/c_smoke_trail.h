// This defines the client-side SmokeTrail entity. It can also be used without
// an entity, in which case you must pass calls to it and set its position each frame.

#ifndef PARTICLE_SMOKETRAIL_H
#define PARTICLE_SMOKETRAIL_H

#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "particles_simple.h"
#include "c_baseentity.h"
#include "baseparticleentity.h"

//
// Smoke Trail
//

class C_SmokeTrail : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_SmokeTrail, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();
	
					C_SmokeTrail();
	virtual			~C_SmokeTrail();

public:

	//For attachments
	void			GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles );

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
	// Effect parameters. These will assume default values but you can change them.
	float			m_SpawnRate;			// How many particles per second.

	Vector			m_StartColor;			// Fade between these colors.
	Vector			m_EndColor;
	float			m_Opacity;

	float			m_ParticleLifetime;		// How long do the particles live?
	float			m_StopEmitTime;			// When do I stop emitting particles? (-1 = never)
	
	float			m_MinSpeed;				// Speed range.
	float			m_MaxSpeed;
	
	float			m_StartSize;			// Size ramp.
	float			m_EndSize;

	float			m_SpawnRadius;

	Vector			m_VelocityOffset;		// Emit the particles in a certain direction.

	bool			m_bEmit;				// Keep emitting particles?

	int				m_nAttachment;

private:
	C_SmokeTrail( const C_SmokeTrail & );

	PMaterialHandle	m_MaterialHandle[2];
	TimedEvent		m_ParticleSpawn;

	CParticleMgr	*m_pParticleMgr;
	CSmartPtr<CSimpleEmitter> m_pSmokeEmitter;
};

//==================================================
// C_RocketTrail
//==================================================

class C_RocketTrail : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_RocketTrail, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();
	
					C_RocketTrail();
	virtual			~C_RocketTrail();

public:

	//For attachments
	void			GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles );

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
	// Effect parameters. These will assume default values but you can change them.
	float			m_SpawnRate;			// How many particles per second.

	Vector			m_StartColor;			// Fade between these colors.
	Vector			m_EndColor;
	float			m_Opacity;

	float			m_ParticleLifetime;		// How long do the particles live?
	float			m_StopEmitTime;			// When do I stop emitting particles? (-1 = never)
	
	float			m_MinSpeed;				// Speed range.
	float			m_MaxSpeed;
	
	float			m_StartSize;			// Size ramp.
	float			m_EndSize;

	float			m_SpawnRadius;

	Vector			m_VelocityOffset;		// Emit the particles in a certain direction.

	bool			m_bEmit;				// Keep emitting particles?
	bool			m_bDamaged;				// Has been shot down (should be on fire, etc)

	int				m_nAttachment;

	Vector			m_vecLastPosition;		// Last known position of the rocket

private:
	C_RocketTrail( const C_RocketTrail & );

	PMaterialHandle	m_MaterialHandle[2];
	TimedEvent		m_ParticleSpawn;

	CParticleMgr	*m_pParticleMgr;
	CSmartPtr<CSimpleEmitter> m_pRocketEmitter;
};

class SporeSmokeEffect;


//==================================================
// SporeEffect
//==================================================

class SporeEffect : public CSimpleEmitter
{
public:
							SporeEffect( const char *pDebugName );
	static SporeEffect*		Create( const char *pDebugName );

	virtual void			UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual Vector			UpdateColor( SimpleParticle *pParticle, float timeDelta );
	virtual float			UpdateAlpha( SimpleParticle *pParticle, float timeDelta );

private:
	SporeEffect( const SporeEffect & );
};

//==================================================
// C_SporeExplosion
//==================================================

class C_SporeExplosion : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_SporeExplosion, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();
	
	C_SporeExplosion( void );
	virtual	~C_SporeExplosion( void );

public:

// C_BaseEntity
public:
	virtual	void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void );

// IPrototypeAppEffect
public:
	virtual void	Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs );

// IParticleEffect
public:
	virtual void	Update( float fTimeDelta );
	virtual bool	SimulateAndRender( Particle *pParticle, ParticleDraw *pDraw, float &sortKey );

public:
	float	m_flSpawnRate;
	float	m_flParticleLifetime;
	float	m_flStartSize;
	float	m_flEndSize;
	float	m_flSpawnRadius;

	bool	m_bEmit;

private:
	C_SporeExplosion( const C_SporeExplosion & );

	void	AddParticles( void );

	PMaterialHandle		m_hMaterial;
	TimedEvent			m_teParticleSpawn;

	SporeEffect			*m_pSporeEffect;
	CParticleMgr		*m_pParticleMgr;
};

#endif
