
// Particle system entities can derive from this to handle some of the mundane
// functionality of hooking into the engine's entity system.

#ifndef PARTICLE_BASEEFFECT_H
#define PARTICLE_BASEEFFECT_H

#include "predictable_entity.h"
#include "baseentity_shared.h"

#if defined( CLIENT_DLL )
#define CBaseParticleEntity C_BaseParticleEntity

#include "particlemgr.h"

#endif 

class CBaseParticleEntity : public CBaseEntity
#if defined( CLIENT_DLL )
, public IParticleEffect
#endif
{
public:
	DECLARE_CLASS( CBaseParticleEntity, CBaseEntity );
	DECLARE_PREDICTABLE();
	DECLARE_NETWORKCLASS();

	CBaseParticleEntity();

// CBaseEntity overrides.
public:
#if !defined( CLIENT_DLL )
	virtual bool		ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );
#else
// Default IParticleEffect overrides.
public:

	virtual bool	SimulateAndRender( Particle *pParticle, ParticleDraw* pParticleDraw, float &sortKey );
	virtual void	GetSortOrigin( Vector &vSortOrigin );
public:
	CParticleEffectBinding	m_ParticleEffect;
#endif

	virtual void		Activate();
	virtual void		Think();	

public:
	void				FollowEntity(int index);
	
	// UTIL_Remove will be called after the specified amount of time.
	// If you pass in -1, the entity will never go away automatically.
	void				SetLifetime(float lifetime);
private:
	CBaseParticleEntity( const CBaseParticleEntity & ); // not defined, not accessible
};



#endif


