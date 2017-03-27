//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FX_EXPLOSION_H
#define FX_EXPLOSION_H
#ifdef _WIN32
#pragma once
#endif

#include "particle_collision.h"
#include "glow_overlay.h"

//JDW: For now we're clamping this value
#define	EXPLOSION_FORCE_MAX	2
#define	EXPLOSION_FORCE_MIN	2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_BaseExplosionEffect
{
private:
	static C_BaseExplosionEffect	m_instance;

public:
					~C_BaseExplosionEffect( void ) {}

	static	C_BaseExplosionEffect &Instance( void )	{	return m_instance;	}

	virtual void	Create( const Vector &position, float force, float scale, int flags );

protected:
					C_BaseExplosionEffect( void )	{}

	virtual void	PlaySound( void );

	virtual void	CreateCore( void );
	virtual void	CreateDebris( void );
	virtual void	CreateMisc( void );
	virtual void	CreateDynamicLight( void );

	float			ScaleForceByDeviation( Vector &deviant, Vector &source, float spread, float *force = NULL );

	float			Probe( const Vector &origin, Vector *direction, float strength );
	void			GetForceDirection( const Vector &origin, float magnitude, Vector *resultDirection, float *resultForce );

protected:

	Vector	m_vecOrigin;
	Vector	m_vecDirection;
	float	m_flForce;
	int		m_fFlags;
};

//Singleton accessor
extern C_BaseExplosionEffect &BaseExplosionEffect( void );


//
// CExplosionOverlay
//

class CExplosionOverlay : public CWarpOverlay
{
public:
	
	virtual bool Update( void );

public:

	float	m_flLifetime;
	Vector	m_vBaseColors[CGlowOverlay::MAX_SPRITES];

};

//-----------------------------------------------------------------------------
// Purpose: Water explosion
//-----------------------------------------------------------------------------
class C_WaterExplosionEffect : public C_BaseExplosionEffect
{
	typedef C_BaseExplosionEffect BaseClass;
public:
	static	C_WaterExplosionEffect &Instance( void )	{	return m_waterinstance;	}

	virtual void	Create( const Vector &position, float force, float scale, int flags );

protected:
	virtual void	CreateCore( void );
	virtual void	CreateDebris( void );
	virtual void	CreateMisc( void );
	virtual void	PlaySound( void );

private:
	Vector	m_vecWaterSurface;
	static C_WaterExplosionEffect	m_waterinstance;
};

//Singleton accessor
extern C_WaterExplosionEffect &WaterExplosionEffect( void );

#endif // FX_EXPLOSION_H
