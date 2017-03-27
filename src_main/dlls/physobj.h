//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHYSOBJ_H
#define PHYSOBJ_H
#ifdef _WIN32
#pragma once
#endif

#ifndef PHYSICS_H
#include "physics.h"
#endif

#include "entityoutput.h"
#include "func_break.h"

// ---------------------------------------------------------------------
//
// CPhysBox -- physically simulated brush rectangular solid
//
// ---------------------------------------------------------------------

// UNDONE: Hook collisions into the physics system to generate touch functions and take damage on falls
// UNDONE: Base class PhysBrush
class CPhysBox : public CBreakable
{
DECLARE_CLASS( CPhysBox, CBreakable );

public:
	DECLARE_SERVERCLASS();

	void	Spawn ( void );
	bool	CreateVPhysics();
	void	Move( const Vector &force );
	virtual int ObjectCaps();
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	virtual int DrawDebugTextOverlays(void);

	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );
	int		OnTakeDamage( const CTakeDamageInfo &info );

	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser );
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, bool wasLaunched );

	bool		 HasPreferredCarryAngles( void );
	const QAngle &PreferredCarryAngles( void ) { return m_angPreferredCarryAngles; }

	// inputs
	void InputWake( inputdata_t &inputdata );
	void InputSleep( inputdata_t &inputdata );
	void InputEnableMotion( inputdata_t &inputdata );
	void InputDisableMotion( inputdata_t &inputdata );
	void InputForceDrop( inputdata_t &inputdata );

	DECLARE_DATADESC();
	
protected:
	int				m_damageType;
	float			m_massScale;
	string_t		m_iszOverrideScript;
	int				m_damageToEnableMotion;
	QAngle			m_angPreferredCarryAngles;

	// Outputs
	COutputEvent	m_OnDamaged;
	COutputEvent	m_OnAwakened;
	COutputEvent	m_OnPhysGunPickup;
	COutputEvent	m_OnPhysGunDrop;

	CHandle<CBasePlayer>	m_hCarryingPlayer;	// Player who's carrying us
};

// ---------------------------------------------------------------------
//
// CPhysExplosion -- physically simulated explosion
//
// ---------------------------------------------------------------------
class CPhysExplosion : public CPointEntity
{
public:
	DECLARE_CLASS( CPhysExplosion, CPointEntity );

	void	Spawn ( void );
	void	Explode( CBaseEntity *pActivator );

	CBaseEntity *FindEntity( CBaseEntity *pEntity, CBaseEntity *pActivator );

	// Input handlers
	void InputExplode( inputdata_t &inputdata );

	DECLARE_DATADESC();
private:

	float		m_damage;
	float		m_radius;
	string_t	m_targetEntityName;
};


//==================================================
// CPhysImpact
//==================================================

class CPhysImpact : public CPointEntity
{
public:
	DECLARE_CLASS( CPhysImpact, CPointEntity );

	void		Spawn( void );
	//void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void		Activate( void );

	void		InputImpact( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:

	void		PointAtEntity( void );

	float		m_damage;
	float		m_distance;
	string_t	m_directionEntityName;
};

//-----------------------------------------------------------------------------
// Purpose: A magnet that creates constraints between itself and anything it touches 
//-----------------------------------------------------------------------------

struct magnetted_objects_t
{
	IPhysicsConstraint *pConstraint;
	EHANDLE			   hEntity;

	DECLARE_SIMPLE_DATADESC();
};

class CPhysMagnet : public CBaseAnimating, public IPhysicsConstraintEvent
{
	DECLARE_CLASS( CPhysMagnet, CBaseAnimating );
public:
	DECLARE_DATADESC();

	CPhysMagnet();
	~CPhysMagnet();

	void	Spawn( void );
	void	Precache( void );
	void	Touch( CBaseEntity *pOther );
	void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void	DoMagnetSuck( CBaseEntity *pOther );
	void	SetConstraintGroup( IPhysicsConstraintGroup *pGroup );

	bool	IsOn( void ) { return m_bActive; }
	int		GetNumAttachedObjects( void );
	float	GetTotalMassAttachedObjects( void );

	// Checking for hitting something
	void	ResetHasHitSomething( void ) { m_bHasHitSomething = false; }
	bool	HasHitSomething( void ) { return m_bHasHitSomething; }

	// Inputs
	void	InputToggle( inputdata_t &inputdata );
	void	InputTurnOn( inputdata_t &inputdata );
	void	InputTurnOff( inputdata_t &inputdata );

	void	InputConstraintBroken( inputdata_t &inputdata );

	void	DetachAll( void );

// IPhysicsConstraintEvent
public:
	void	ConstraintBroken( IPhysicsConstraint *pConstraint );
	
protected:
	// Outputs
	COutputEvent	m_OnMagnetAttach;
	COutputEvent	m_OnMagnetDetach;

	// Keys
	float			m_massScale;
	string_t		m_iszOverrideScript;
	float			m_forceLimit;
	float			m_torqueLimit;

	CUtlVector< magnetted_objects_t >	m_MagnettedEntities;
	IPhysicsConstraintGroup				*m_pConstraintGroup;

	bool			m_bActive;
	bool			m_bHasHitSomething;
	float			m_flTotalMass;
	float			m_flRadius;
	float			m_flNextSuckTime;
	int				m_iMaxObjectsAttached;
};

#endif // PHYSOBJ_H
