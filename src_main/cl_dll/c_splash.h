//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Id: $
//
// Contains the splash effect
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef PARTICLE_SPLASH_H
#define PARTICLE_SPLASH_H

#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "particles_simple.h"
#include "particle_collision.h"
#include "c_baseentity.h"
#include "baseparticleentity.h"

class CFleckParticles;

class C_Splash : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_Splash, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();
	
					C_Splash();
	virtual			~C_Splash();

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
	void			Think(void);			// Used to remove entity
	void			InitParticleCollisions(void);

public:
	// Effect parameters. These will assume default values but you can change them.
	float				m_flSpawnRate;			// How many particles per second.

	Vector				m_vStartColor;			// Fade between these colors.
	Vector				m_vEndColor;

	float				m_flParticleLifetime;		// How long do the particles live?
	float				m_flStopEmitTime;			// When do I stop emitting particles? (-1 = never)
	float				m_flStartEmitTime;		// When did I start emitting particles
	
	float				m_flSpeed;				// Speed
	float				m_flSpeedRange;
	
	float				m_flWidthMin;			// Width range
	float				m_flWidthMax;

	float				m_flNoise;

	Vector				m_VelocityOffset;		// Emit the particles in a certain direction.

	bool				m_bEmit;				// Keep emitting particles?
	int					m_nNumDecals;			// How many decals to drop
	int					m_nDecalsRemaining;		// Number of decals remaining

private:
	C_Splash( const C_Splash & ); // not defined, not accessible

	void				PlaySplashSound(Vector vPos);
	float				m_flNextSoundTime;	

	PMaterialHandle		m_MaterialHandle;

	TimedEvent			m_ParticleSpawn;
	float				m_flSplashEndZ;			// Z value where splash disappears
	
	Vector				m_Pos;
	CParticleMgr*		m_pParticleMgr;

	CParticleCollision	m_ParticleCollision;
};

#endif