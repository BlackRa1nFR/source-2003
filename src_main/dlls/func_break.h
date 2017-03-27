//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines a class for objects that break after taking a certain amount
//			of damage.
//
// $NoKeywords: $
//=============================================================================

#ifndef FUNC_BREAK_H
#define FUNC_BREAK_H
#pragma once

#include "entityoutput.h"


typedef enum { expRandom, expDirected} Explosions;
typedef enum { matGlass = 0, matWood, matMetal, matFlesh, matCinderBlock, matCeilingTile, matComputer, matUnbreakableGlass, matRocks, matNone, matLastMaterial } Materials;


#define	NUM_SHARDS 6 // this many shards spawned when breakable objects break;

// Spawnflags for func breakable
#define SF_BREAK_TRIGGER_ONLY				0x0001	// may only be broken by trigger
#define	SF_BREAK_TOUCH						0x0002	// can be 'crashed through' by running player (plate glass)
#define SF_BREAK_PRESSURE					0x0004	// can be broken by a player standing on it
#define SF_BREAK_PHYSICS_BREAK_IMMEDIATELY	0x0200	// the first physics collision this breakable has will immediately break it
#define SF_BREAK_DONT_TAKE_PHYSICS_DAMAGE	0x0400	// this breakable doesn't take damage from physics collisions

// Spawnflags for func_pushable (it's also func_breakable, so don't collide with those flags)
#define SF_PUSH_BREAKABLE					0x0080

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBreakable : public CBaseEntity
{
public:
	DECLARE_CLASS( CBreakable, CBaseEntity );

	// basic functions
	virtual void Spawn( void );
	bool CreateVPhysics( void );
	virtual void Precache( void );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	void BreakTouch( CBaseEntity *pOther );
	void DamageSound( void );
	void Break( CBaseEntity *pBreaker );

	// Input handlers
	void InputAddHealth( inputdata_t &inputdata );
	void InputBreak( inputdata_t &inputdata );
	void InputRemoveHealth( inputdata_t &inputdata );
	void InputSetHealth( inputdata_t &inputdata );

	// breakables use an overridden takedamage
	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	// To spark when hit
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	bool IsBreakable( void );
	bool SparkWhenHit( void );

	char const		*DamageDecal( int bitsDamageType, int gameMaterial );

	virtual void	Die( void );
	void			ResetOnGroundFlags(void);

	inline bool		Explodable( void ) { return ExplosionMagnitude() > 0; }
	inline int		ExplosionMagnitude( void ) { return m_ExplosionMagnitude; }
	inline void		ExplosionSetMagnitude( int magnitude ) { m_ExplosionMagnitude = magnitude; }

	static void MaterialSoundRandom( int entindex, Materials soundMaterial, float volume );
	static const char *MaterialSound( Materials precacheMaterial );

	static const char *pSpawnObjects[];

	DECLARE_DATADESC();

protected:
	float		m_angle;
	Materials	m_Material;
	EHANDLE m_hBreaker;			// The entity that broke us. Held as a data member because sometimes breaking is delayed.

private:

	Explosions	m_Explosion;
	int			m_idShard;
	string_t 	m_iszGibModel;
	string_t 	m_iszSpawnObject;
	int			m_ExplosionMagnitude;
	float		m_flPressureDelay;		// Delay before breaking when destoyed by pressure
	int			m_iMinHealthDmg;		// minimum damage attacker must have to cause damage
	bool		m_bTookPhysicsDamage;

protected:

	float		m_impactEnergyScale;

	COutputEvent m_OnBreak;
	COutputInt m_OnHealthChanged;
};

#endif	// FUNC_BREAK_H
